/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/kobject.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/stringify.h>
#include <linux/debugfs.h>
#include <linux/msm_tsens.h>
#include <asm/atomic.h>
#include <asm/page.h>
#include <mach/msm_dcvs.h>

#define CORE_HANDLE_OFFSET (0xA0)
#define __err(f, ...) pr_err("MSM_DCVS: %s: " f, __func__, __VA_ARGS__)
#define __info(f, ...) pr_info("MSM_DCVS: %s: " f, __func__, __VA_ARGS__)
#define MAX_PENDING	(5)

struct core_attribs {
	struct kobj_attribute core_id;
	struct kobj_attribute idle_enabled;
	struct kobj_attribute freq_change_enabled;
	struct kobj_attribute actual_freq;
	struct kobj_attribute freq_change_us;

	struct kobj_attribute disable_pc_threshold;
	struct kobj_attribute em_win_size_min_us;
	struct kobj_attribute em_win_size_max_us;
	struct kobj_attribute em_max_util_pct;
	struct kobj_attribute group_id;
	struct kobj_attribute max_freq_chg_time_us;
	struct kobj_attribute slack_mode_dynamic;
	struct kobj_attribute slack_time_min_us;
	struct kobj_attribute slack_time_max_us;
	struct kobj_attribute slack_weight_thresh_pct;
	struct kobj_attribute ss_iobusy_conv;
	struct kobj_attribute ss_win_size_min_us;
	struct kobj_attribute ss_win_size_max_us;
	struct kobj_attribute ss_util_pct;

	struct kobj_attribute active_coeff_a;
	struct kobj_attribute active_coeff_b;
	struct kobj_attribute active_coeff_c;
	struct kobj_attribute leakage_coeff_a;
	struct kobj_attribute leakage_coeff_b;
	struct kobj_attribute leakage_coeff_c;
	struct kobj_attribute leakage_coeff_d;

	struct attribute_group attrib_group;
};

enum pending_freq_state {
	/*
	 * used by the thread to check if pending_freq was updated while it was
	 * setting previous frequency - this is written to and used by the
	 * freq updating thread
	 */
	NO_OUTSTANDING_FREQ_CHANGE = 0,

	/*
	 * This request is set to indicate that the governor is stopped and no
	 * more frequency change requests are accepted untill it starts again.
	 * This is checked/used by the threads that want to change the freq
	 */
	STOP_FREQ_CHANGE = -1,

	/*
	 * Any other +ve value means that a freq change was requested and the
	 * thread has not gotten around to update it
	 *
	 * Any other -ve value means that this is the last freq change i.e. a
	 * freq change was requested but the thread has not run yet and
	 * meanwhile the governor was stopped.
	 */
};

struct dcvs_core {
	spinlock_t	idle_state_change_lock;
	/* 0 when not idle (busy)  1 when idle and -1 when governor starts and
	 * we dont know whether the next call is going to be idle enter or exit
	 */
	int		idle_entered;

	enum msm_dcvs_core_type type;
	/* this is the number in each type for example cpu 0,1,2 and gpu 0,1 */
	int type_core_num;
	char core_name[CORE_NAME_MAX];
	uint32_t actual_freq;
	uint32_t freq_change_us;

	uint32_t max_time_us; /* core param */

	struct msm_dcvs_algo_param algo_param;
	struct msm_dcvs_energy_curve_coeffs coeffs;

	/* private */
	ktime_t time_start;
	struct task_struct *task;
	struct core_attribs attrib;
	uint32_t dcvs_core_id;
	struct msm_dcvs_core_info *info;
	int sensor;
	wait_queue_head_t wait_q;

	int (*set_frequency)(int type_core_num, unsigned int freq);
	unsigned int (*get_frequency)(int type_core_num);
	int (*idle_enable)(int type_core_num,
			enum msm_core_control_event event);

	spinlock_t	pending_freq_lock;
	int pending_freq;

	struct hrtimer	slack_timer;
};

static int msm_dcvs_enabled = 1;
module_param_named(enable, msm_dcvs_enabled, int, S_IRUGO | S_IWUSR | S_IWGRP);

static struct dentry		*debugfs_base;

static struct dcvs_core core_list[CORES_MAX];

static struct kobject *cores_kobj;

static void force_stop_slack_timer(struct dcvs_core *core)
{
	unsigned long flags;

	spin_lock_irqsave(&core->idle_state_change_lock, flags);
	hrtimer_cancel(&core->slack_timer);
	spin_unlock_irqrestore(&core->idle_state_change_lock, flags);
}

static void stop_slack_timer(struct dcvs_core *core)
{
	unsigned long flags;

	spin_lock_irqsave(&core->idle_state_change_lock, flags);
	/* err only for cpu type's GPU's can do idle exit consecutively */
	if (core->idle_entered == 1 && !(core->dcvs_core_id >= GPU_OFFSET))
		__err("%s trying to reenter idle", core->core_name);
	core->idle_entered = 1;
	hrtimer_cancel(&core->slack_timer);
	core->idle_entered = 1;
	spin_unlock_irqrestore(&core->idle_state_change_lock, flags);
}

static void start_slack_timer(struct dcvs_core *core, int slack_us)
{
	unsigned long flags1, flags2;
	int ret;

	spin_lock_irqsave(&core->idle_state_change_lock, flags2);

	spin_lock_irqsave(&core->pending_freq_lock, flags1);

	/* err only for cpu type's GPU's can do idle enter consecutively */
	if (core->idle_entered == 0 && !(core->dcvs_core_id >= GPU_OFFSET))
		__err("%s trying to reexit idle", core->core_name);
	core->idle_entered = 0;
	/*
	 * only start the timer if governor is not stopped
	 */
	if (slack_us != 0
		&& !(core->pending_freq < NO_OUTSTANDING_FREQ_CHANGE)) {
		ret = hrtimer_start(&core->slack_timer,
				ktime_set(0, slack_us * 1000),
				HRTIMER_MODE_REL_PINNED);
		if (ret) {
			pr_err("%s Failed to start timer ret = %d\n",
					core->core_name, ret);
		}
	}
	spin_unlock_irqrestore(&core->pending_freq_lock, flags1);

	spin_unlock_irqrestore(&core->idle_state_change_lock, flags2);
}

static void restart_slack_timer(struct dcvs_core *core, int slack_us)
{
	unsigned long flags1, flags2;
	int ret;

	spin_lock_irqsave(&core->idle_state_change_lock, flags2);

	hrtimer_cancel(&core->slack_timer);

	spin_lock_irqsave(&core->pending_freq_lock, flags1);

	/*
	 * only start the timer if idle is not entered
	 * and governor is not stopped
	 */
	if (slack_us != 0 && (core->idle_entered != 1)
		&& !(core->pending_freq < NO_OUTSTANDING_FREQ_CHANGE)) {
		ret = hrtimer_start(&core->slack_timer,
				ktime_set(0, slack_us * 1000),
				HRTIMER_MODE_REL_PINNED);
		if (ret) {
			pr_err("%s Failed to start timer ret = %d\n",
					core->core_name, ret);
		}
	}
	spin_unlock_irqrestore(&core->pending_freq_lock, flags1);
	spin_unlock_irqrestore(&core->idle_state_change_lock, flags2);
}

static int __msm_dcvs_change_freq(struct dcvs_core *core)
{
	int ret = 0;
	unsigned long flags = 0;
	int requested_freq = 0;
	ktime_t time_start;
	uint32_t slack_us = 0;
	uint32_t ret1 = 0;

	spin_lock_irqsave(&core->pending_freq_lock, flags);
repeat:
	BUG_ON(!core->pending_freq);
	if (core->pending_freq == STOP_FREQ_CHANGE)
		BUG();

	requested_freq = core->pending_freq;
	time_start = core->time_start;
	core->time_start = ns_to_ktime(0);

	if (requested_freq < 0) {
		requested_freq = -1 * requested_freq;
		core->pending_freq = STOP_FREQ_CHANGE;
	} else {
		core->pending_freq = NO_OUTSTANDING_FREQ_CHANGE;
	}

	if (requested_freq == core->actual_freq)
		goto out;

	spin_unlock_irqrestore(&core->pending_freq_lock, flags);

	/**
	 * Call the frequency sink driver to change the frequency
	 * We will need to get back the actual frequency in KHz and
	 * the record the time taken to change it.
	 */
	ret = core->set_frequency(core->type_core_num, requested_freq);
	if (ret <= 0)
		__err("Core %s failed to set freq %u\n",
				core->core_name, requested_freq);
		/* continue to call TZ to get updated slack timer */
	else
		core->actual_freq = ret;

	core->freq_change_us = (uint32_t)ktime_to_us(
					ktime_sub(ktime_get(), time_start));

	/**
	 * Disable low power modes if the actual frequency is >
	 * disable_pc_threshold.
	 */
	if (core->actual_freq > core->algo_param.disable_pc_threshold) {
		core->idle_enable(core->type_core_num,
				MSM_DCVS_DISABLE_HIGH_LATENCY_MODES);
	} else if (core->actual_freq <= core->algo_param.disable_pc_threshold) {
		core->idle_enable(core->type_core_num,
				MSM_DCVS_ENABLE_HIGH_LATENCY_MODES);
	}

	/**
	 * Update algorithm with new freq and time taken to change
	 * to this frequency and that will get us the new slack
	 * timer
	 */
	ret = msm_dcvs_scm_event(core->dcvs_core_id,
			MSM_DCVS_SCM_CLOCK_FREQ_UPDATE,
			core->actual_freq, core->freq_change_us,
			&slack_us, &ret1);
	if (ret) {
		__err("Error sending core (%s) dcvs_core_id = %d freq change (%u) reqfreq = %d slack_us=%d ret = %d\n",
				core->core_name, core->dcvs_core_id,
				core->actual_freq, requested_freq,
				slack_us, ret);
	}

	/* TODO confirm that we get a valid freq from SM even when the above
	 * FREQ_UPDATE fails
	 */
	restart_slack_timer(core, slack_us);
	spin_lock_irqsave(&core->pending_freq_lock, flags);

	/**
	 * By the time we are done with freq changes, we could be asked to
	 * change again. Check before exiting.
	 */
	if (core->pending_freq != NO_OUTSTANDING_FREQ_CHANGE
		&& core->pending_freq != STOP_FREQ_CHANGE) {
		goto repeat;
	}

out:   /* should always be jumped to with the spin_lock held */
	spin_unlock_irqrestore(&core->pending_freq_lock, flags);

	return ret;
}

static int __msm_dcvs_report_temp(struct dcvs_core *core)
{
	struct msm_dcvs_core_info *info = core->info;
	struct tsens_device tsens_dev;
	int ret;
	unsigned long temp = 0;

	tsens_dev.sensor_num = core->sensor;
	ret = tsens_get_temp(&tsens_dev, &temp);
	if (!ret) {
		tsens_dev.sensor_num = 0;
		ret = tsens_get_temp(&tsens_dev, &temp);
		if (!ret)
			return -ENODEV;
	}

	ret = msm_dcvs_scm_set_power_params(core->dcvs_core_id,
			&info->power_param,
			&info->freq_tbl[0], &core->coeffs);
	return ret;
}

static int msm_dcvs_do_freq(void *data)
{
	struct dcvs_core *core = (struct dcvs_core *)data;
	static struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1};

	sched_setscheduler(current, SCHED_FIFO, &param);

	while (!kthread_should_stop()) {
		wait_event(core->wait_q, !(core->pending_freq == 0 ||
					  core->pending_freq == -1) ||
					  kthread_should_stop());

		if (kthread_should_stop())
			break;

		__msm_dcvs_change_freq(core);
		__msm_dcvs_report_temp(core);
	}

	return 0;
}

/* freq_pending_lock should be held */
static void request_freq_change(struct dcvs_core *core, int new_freq)
{
	if (new_freq == NO_OUTSTANDING_FREQ_CHANGE) {
		if (core->pending_freq != STOP_FREQ_CHANGE) {
			__err("%s gov started with earlier pending freq %d\n",
					core->core_name, core->pending_freq);
		}
		core->pending_freq = NO_OUTSTANDING_FREQ_CHANGE;
		return;
	}

	if (new_freq == STOP_FREQ_CHANGE) {
		if (core->pending_freq == NO_OUTSTANDING_FREQ_CHANGE)
			core->pending_freq = STOP_FREQ_CHANGE;
		else if (core->pending_freq > 0)
			core->pending_freq = -1 * core->pending_freq;
		return;
	}

	if (core->pending_freq < 0) {
		/* a value less than 0 means that the governor has stopped
		 * and no more freq changes should be requested
		 */
		return;
	}

	if (core->actual_freq != new_freq && core->pending_freq != new_freq) {
		core->pending_freq = new_freq;
		core->time_start = ktime_get();
		wake_up(&core->wait_q);
	}
}

static int msm_dcvs_update_freq(struct dcvs_core *core,
		enum msm_dcvs_scm_event event, uint32_t param0,
		uint32_t *ret1)
{
	int ret = 0;
	unsigned long flags = 0;
	uint32_t new_freq = -EINVAL;

	spin_lock_irqsave(&core->pending_freq_lock, flags);

	ret = msm_dcvs_scm_event(core->dcvs_core_id, event, param0,
				core->actual_freq, &new_freq, ret1);
	if (ret) {
		if (ret == -13)
			ret = 0;
		else
			__err("Error (%d) sending SCM event %d for core %s\n",
				ret, event, core->core_name);
		goto out;
	}

	if (new_freq == 0) {
		/*
		 * sometimes TZ gives us a 0 freq back,
		 * do not queue up a request
		 */
		goto out;
	}

	request_freq_change(core, new_freq);

out:
	spin_unlock_irqrestore(&core->pending_freq_lock, flags);

	return ret;
}

static enum hrtimer_restart msm_dcvs_core_slack_timer(struct hrtimer *timer)
{
	int ret = 0;
	struct dcvs_core *core = container_of(timer,
					struct dcvs_core, slack_timer);
	uint32_t ret1;

	/**
	 * Timer expired, notify TZ
	 * Dont care about the third arg.
	 */
	ret = msm_dcvs_update_freq(core, MSM_DCVS_SCM_QOS_TIMER_EXPIRED, 0,
				   &ret1);
	if (ret)
		__err("Timer expired for core %s but failed to notify.\n",
				core->core_name);

	return HRTIMER_NORESTART;
}

/* Helper functions and macros for sysfs nodes for a core */
#define CORE_FROM_ATTRIBS(attr, name) \
	container_of(container_of(attr, struct core_attribs, name), \
		struct dcvs_core, attrib);

#define DCVS_PARAM_SHOW(_name, v) \
static ssize_t msm_dcvs_attr_##_name##_show(struct kobject *kobj, \
		struct kobj_attribute *attr, char *buf) \
{ \
	struct dcvs_core *core = CORE_FROM_ATTRIBS(attr, _name); \
	return snprintf(buf, PAGE_SIZE, "%d\n", v); \
}

#define DCVS_ALGO_PARAM(_name) \
static ssize_t msm_dcvs_attr_##_name##_show(struct kobject *kobj,\
		struct kobj_attribute *attr, char *buf) \
{ \
	struct dcvs_core *core = CORE_FROM_ATTRIBS(attr, _name); \
	return snprintf(buf, PAGE_SIZE, "%d\n", core->algo_param._name); \
} \
static ssize_t msm_dcvs_attr_##_name##_store(struct kobject *kobj, \
		struct kobj_attribute *attr, const char *buf, size_t count) \
{ \
	int ret = 0; \
	uint32_t val = 0; \
	struct dcvs_core *core = CORE_FROM_ATTRIBS(attr, _name); \
	ret = kstrtouint(buf, 10, &val); \
	if (ret) { \
		__err("Invalid input %s for %s\n", buf, __stringify(_name));\
	} else { \
		uint32_t old_val = core->algo_param._name; \
		core->algo_param._name = val; \
		ret = msm_dcvs_scm_set_algo_params(core->dcvs_core_id, \
				&core->algo_param); \
		if (ret) { \
			core->algo_param._name = old_val; \
			__err("Error(%d) in setting %d for algo param %s\n",\
					ret, val, __stringify(_name)); \
		} \
	} \
	return count; \
}

#define DCVS_ENERGY_PARAM(_name) \
static ssize_t msm_dcvs_attr_##_name##_show(struct kobject *kobj,\
		struct kobj_attribute *attr, char *buf) \
{ \
	struct dcvs_core *core = CORE_FROM_ATTRIBS(attr, _name); \
	return snprintf(buf, PAGE_SIZE, "%d\n", core->coeffs._name); \
} \
static ssize_t msm_dcvs_attr_##_name##_store(struct kobject *kobj, \
		struct kobj_attribute *attr, const char *buf, size_t count) \
{ \
	int ret = 0; \
	int32_t val = 0; \
	struct dcvs_core *core = CORE_FROM_ATTRIBS(attr, _name); \
	ret = kstrtoint(buf, 10, &val); \
	if (ret) { \
		__err("Invalid input %s for %s\n", buf, __stringify(_name));\
	} else { \
		int32_t old_val = core->coeffs._name; \
		core->coeffs._name = val; \
		ret = msm_dcvs_scm_set_power_params(core->dcvs_core_id, \
			&core->info->power_param, &core->info->freq_tbl[0], \
				&core->coeffs); \
		if (ret) { \
			core->coeffs._name = old_val; \
			__err("Error(%d) in setting %d for coeffs param %s\n",\
					ret, val, __stringify(_name)); \
		} \
	} \
	return count; \
}

#define DCVS_RO_ATTRIB(i, _name) \
	core->attrib._name.attr.name = __stringify(_name); \
	core->attrib._name.attr.mode = S_IRUGO; \
	core->attrib._name.show = msm_dcvs_attr_##_name##_show; \
	core->attrib._name.store = NULL; \
	core->attrib.attrib_group.attrs[i] = &core->attrib._name.attr;

#define DCVS_RW_ATTRIB(i, _name) \
	core->attrib._name.attr.name = __stringify(_name); \
	core->attrib._name.attr.mode = S_IRUGO | S_IWUSR; \
	core->attrib._name.show = msm_dcvs_attr_##_name##_show; \
	core->attrib._name.store = msm_dcvs_attr_##_name##_store; \
	core->attrib.attrib_group.attrs[i] = &core->attrib._name.attr;

/**
 * Function declarations for different attributes.
 * Gets used when setting the attribute show and store parameters.
 */
DCVS_PARAM_SHOW(core_id, core->dcvs_core_id)
DCVS_PARAM_SHOW(idle_enabled, (core->idle_enable != NULL))
DCVS_PARAM_SHOW(freq_change_enabled, (core->set_frequency != NULL))
DCVS_PARAM_SHOW(actual_freq, (core->actual_freq))
DCVS_PARAM_SHOW(freq_change_us, (core->freq_change_us))

DCVS_ALGO_PARAM(disable_pc_threshold)
DCVS_ALGO_PARAM(em_win_size_min_us)
DCVS_ALGO_PARAM(em_win_size_max_us)
DCVS_ALGO_PARAM(em_max_util_pct)
DCVS_ALGO_PARAM(group_id)
DCVS_ALGO_PARAM(max_freq_chg_time_us)
DCVS_ALGO_PARAM(slack_mode_dynamic)
DCVS_ALGO_PARAM(slack_time_min_us)
DCVS_ALGO_PARAM(slack_time_max_us)
DCVS_ALGO_PARAM(slack_weight_thresh_pct)
DCVS_ALGO_PARAM(ss_iobusy_conv)
DCVS_ALGO_PARAM(ss_win_size_min_us)
DCVS_ALGO_PARAM(ss_win_size_max_us)
DCVS_ALGO_PARAM(ss_util_pct)

DCVS_ENERGY_PARAM(active_coeff_a)
DCVS_ENERGY_PARAM(active_coeff_b)
DCVS_ENERGY_PARAM(active_coeff_c)
DCVS_ENERGY_PARAM(leakage_coeff_a)
DCVS_ENERGY_PARAM(leakage_coeff_b)
DCVS_ENERGY_PARAM(leakage_coeff_c)
DCVS_ENERGY_PARAM(leakage_coeff_d)

static int msm_dcvs_setup_core_sysfs(struct dcvs_core *core)
{
	int ret = 0;
	struct kobject *core_kobj = NULL;
	const int attr_count = 27;

	BUG_ON(!cores_kobj);

	core->attrib.attrib_group.attrs =
		kzalloc(attr_count * sizeof(struct attribute *), GFP_KERNEL);

	if (!core->attrib.attrib_group.attrs) {
		ret = -ENOMEM;
		goto done;
	}

	DCVS_RO_ATTRIB(0, core_id);
	DCVS_RO_ATTRIB(1, idle_enabled);
	DCVS_RO_ATTRIB(2, freq_change_enabled);
	DCVS_RO_ATTRIB(3, actual_freq);
	DCVS_RO_ATTRIB(4, freq_change_us);

	DCVS_RW_ATTRIB(5, disable_pc_threshold);
	DCVS_RW_ATTRIB(6, em_win_size_min_us);
	DCVS_RW_ATTRIB(7, em_win_size_max_us);
	DCVS_RW_ATTRIB(8, em_max_util_pct);
	DCVS_RW_ATTRIB(9, group_id);
	DCVS_RW_ATTRIB(10, max_freq_chg_time_us);
	DCVS_RW_ATTRIB(11, slack_mode_dynamic);
	DCVS_RW_ATTRIB(12, slack_time_min_us);
	DCVS_RW_ATTRIB(13, slack_time_max_us);
	DCVS_RW_ATTRIB(14, slack_weight_thresh_pct);
	DCVS_RW_ATTRIB(15, ss_iobusy_conv);
	DCVS_RW_ATTRIB(16, ss_win_size_min_us);
	DCVS_RW_ATTRIB(17, ss_win_size_max_us);
	DCVS_RW_ATTRIB(18, ss_util_pct);

	DCVS_RW_ATTRIB(19, active_coeff_a);
	DCVS_RW_ATTRIB(20, active_coeff_b);
	DCVS_RW_ATTRIB(21, active_coeff_c);
	DCVS_RW_ATTRIB(22, leakage_coeff_a);
	DCVS_RW_ATTRIB(23, leakage_coeff_b);
	DCVS_RW_ATTRIB(24, leakage_coeff_c);
	DCVS_RW_ATTRIB(25, leakage_coeff_d);

	core->attrib.attrib_group.attrs[26] = NULL;

	core_kobj = kobject_create_and_add(core->core_name, cores_kobj);
	if (!core_kobj) {
		ret = -ENOMEM;
		goto done;
	}

	ret = sysfs_create_group(core_kobj, &core->attrib.attrib_group);
	if (ret)
		__err("Cannot create core %s attr group\n", core->core_name);

done:
	if (ret) {
		kfree(core->attrib.attrib_group.attrs);
		kobject_del(core_kobj);
	}

	return ret;
}

/* Return the core and initialize non platform data specific numbers in it */
static struct dcvs_core *msm_dcvs_add_core(enum msm_dcvs_core_type type,
								int num)
{
	struct dcvs_core *core = NULL;
	int i;
	char name[CORE_NAME_MAX];

	switch (type) {
	case MSM_DCVS_CORE_TYPE_CPU:
		i = CPU_OFFSET + num;
		BUG_ON(i >= GPU_OFFSET);
		snprintf(name, CORE_NAME_MAX, "cpu%d", num);
		break;
	case MSM_DCVS_CORE_TYPE_GPU:
		i = GPU_OFFSET + num;
		BUG_ON(i >= CORES_MAX);
		snprintf(name, CORE_NAME_MAX, "gpu%d", num);
		break;
	default:
		return NULL;
	}

	core = &core_list[i];
	core->dcvs_core_id = i;
	strlcpy(core->core_name, name, CORE_NAME_MAX);
	spin_lock_init(&core->pending_freq_lock);
	spin_lock_init(&core->idle_state_change_lock);
	hrtimer_init(&core->slack_timer,
			CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED);
	core->slack_timer.function = msm_dcvs_core_slack_timer;
	return core;
}

/* Return the core if found or add to list if @add_to_list is true */
static struct dcvs_core *msm_dcvs_get_core(int offset)
{
	/* if the handle is still not set bug */
	BUG_ON(core_list[offset].dcvs_core_id == -1);
	return &core_list[offset];
}


int msm_dcvs_register_core(
	enum msm_dcvs_core_type type,
	int type_core_num,
	struct msm_dcvs_core_info *info,
	int (*set_frequency)(int type_core_num, unsigned int freq),
	unsigned int (*get_frequency)(int type_core_num),
	int (*idle_enable)(int type_core_num,
					enum msm_core_control_event event),
	int sensor)
{
	int ret = -EINVAL;
	struct dcvs_core *core = NULL;
	uint32_t ret1;
	uint32_t ret2;

	core = msm_dcvs_add_core(type, type_core_num);
	if (!core)
		return ret;

	core->type = type;
	core->type_core_num = type_core_num;
	core->set_frequency = set_frequency;
	core->get_frequency = get_frequency;
	core->idle_enable = idle_enable;
	core->pending_freq = STOP_FREQ_CHANGE;

	core->info = info;
	memcpy(&core->algo_param, &info->algo_param,
			sizeof(struct msm_dcvs_algo_param));

	memcpy(&core->coeffs, &info->energy_coeffs,
			sizeof(struct msm_dcvs_energy_curve_coeffs));

	/*
	 * The tz expects cpu0 to represent bit 0 in the mask, however the
	 * dcvs_core_id needs to start from 1, dcvs_core_id = 0 is used to
	 * indicate that this request is not associated with any core.
	 * mpdecision
	 */
	info->core_param.core_bitmask_id
				= 1 << (core->dcvs_core_id - CPU_OFFSET);
	core->sensor = sensor;

	ret = msm_dcvs_scm_register_core(core->dcvs_core_id, &info->core_param);
	if (ret) {
		__err("%s: scm register core fail handle = %d ret = %d\n",
					__func__, core->dcvs_core_id, ret);
		goto bail;
	}

	ret = msm_dcvs_scm_set_algo_params(core->dcvs_core_id,
							&info->algo_param);
	if (ret) {
		__err("%s: scm algo params failed ret = %d\n", __func__, ret);
		goto bail;
	}

	ret = msm_dcvs_scm_set_power_params(core->dcvs_core_id,
				&info->power_param,
				&info->freq_tbl[0], &core->coeffs);
	if (ret) {
		__err("%s: scm power params failed ret = %d\n", __func__, ret);
		goto bail;
	}

	ret = msm_dcvs_scm_event(core->dcvs_core_id, MSM_DCVS_SCM_CORE_ONLINE,
				core->actual_freq, 0, &ret1, &ret2);
	if (ret)
		goto bail;

	ret = msm_dcvs_setup_core_sysfs(core);
	if (ret) {
		__err("Unable to setup core %s sysfs\n", core->core_name);
		goto bail;
	}
	core->idle_entered = -1;
	init_waitqueue_head(&core->wait_q);
	core->task = kthread_run(msm_dcvs_do_freq, (void *)core,
			"msm_dcvs/%d", core->dcvs_core_id);
	ret = core->dcvs_core_id;
	return ret;
bail:
	core->dcvs_core_id = -1;
	return -EINVAL;
}
EXPORT_SYMBOL(msm_dcvs_register_core);

void msm_dcvs_update_limits(int dcvs_core_id)
{
	struct dcvs_core *core;

	if (dcvs_core_id < CPU_OFFSET || dcvs_core_id > CORES_MAX) {
		__err("%s invalid dcvs_core_id = %d returning -EINVAL\n",
				__func__, dcvs_core_id);
		return;
	}

	core = msm_dcvs_get_core(dcvs_core_id);
	core->actual_freq = core->get_frequency(core->type_core_num);
}

int msm_dcvs_freq_sink_start(int dcvs_core_id)
{
	int ret = -EINVAL;
	struct dcvs_core *core = NULL;
	uint32_t ret1;
	unsigned long flags;

	if (dcvs_core_id < CPU_OFFSET || dcvs_core_id > CORES_MAX) {
		__err("%s invalid dcvs_core_id = %d returning -EINVAL\n",
				__func__, dcvs_core_id);
		return -EINVAL;
	}

	core = msm_dcvs_get_core(dcvs_core_id);
	if (!core)
		return ret;

	core->actual_freq = core->get_frequency(core->type_core_num);

	spin_lock_irqsave(&core->pending_freq_lock, flags);
	/* mark that we are ready to accept new frequencies */
	request_freq_change(core, NO_OUTSTANDING_FREQ_CHANGE);
	spin_unlock_irqrestore(&core->pending_freq_lock, flags);

	spin_lock_irqsave(&core->idle_state_change_lock, flags);
	core->idle_entered = -1;
	spin_unlock_irqrestore(&core->idle_state_change_lock, flags);

	/* Notify TZ to start receiving idle info for the core */
	ret = msm_dcvs_update_freq(core, MSM_DCVS_SCM_DCVS_ENABLE, 1, &ret1);

	core->idle_enable(core->type_core_num, MSM_DCVS_ENABLE_IDLE_PULSE);
	return 0;
}
EXPORT_SYMBOL(msm_dcvs_freq_sink_start);

int msm_dcvs_freq_sink_stop(int dcvs_core_id)
{
	int ret = -EINVAL;
	struct dcvs_core *core = NULL;
	uint32_t ret1;
	uint32_t freq;
	unsigned long flags;

	if (dcvs_core_id < 0 || dcvs_core_id > CORES_MAX) {
		pr_err("%s invalid dcvs_core_id = %d returning -EINVAL\n",
				__func__, dcvs_core_id);
		return -EINVAL;
	}

	core = msm_dcvs_get_core(dcvs_core_id);
	if (!core) {
		__err("couldn't find core for coreid = %d\n", dcvs_core_id);
		return ret;
	}

	core->idle_enable(core->type_core_num, MSM_DCVS_DISABLE_IDLE_PULSE);
	/* Notify TZ to stop receiving idle info for the core */
	ret = msm_dcvs_scm_event(core->dcvs_core_id, MSM_DCVS_SCM_DCVS_ENABLE,
				0, core->actual_freq, &freq, &ret1);
	core->idle_enable(core->type_core_num,
			MSM_DCVS_ENABLE_HIGH_LATENCY_MODES);
	spin_lock_irqsave(&core->pending_freq_lock, flags);
	/* flush out all the pending freq changes */
	request_freq_change(core, STOP_FREQ_CHANGE);
	spin_unlock_irqrestore(&core->pending_freq_lock, flags);
	force_stop_slack_timer(core);

	return 0;
}
EXPORT_SYMBOL(msm_dcvs_freq_sink_stop);

int msm_dcvs_idle(int dcvs_core_id, enum msm_core_idle_state state,
						uint32_t iowaited)
{
	int ret = 0;
	struct dcvs_core *core = NULL;
	uint32_t timer_interval_us = 0;
	uint32_t r0, r1;

	if (dcvs_core_id < CPU_OFFSET || dcvs_core_id > CORES_MAX) {
		pr_err("invalid dcvs_core_id = %d ret -EINVAL\n", dcvs_core_id);
		return -EINVAL;
	}

	core = msm_dcvs_get_core(dcvs_core_id);

	switch (state) {
	case MSM_DCVS_IDLE_ENTER:
		stop_slack_timer(core);
		ret = msm_dcvs_scm_event(core->dcvs_core_id,
				MSM_DCVS_SCM_IDLE_ENTER, 0, 0, &r0, &r1);
		if (ret < 0 && ret != -13)
			__err("Error (%d) sending idle enter for %s\n",
					ret, core->core_name);
		break;

	case MSM_DCVS_IDLE_EXIT:
		ret = msm_dcvs_update_freq(core, MSM_DCVS_SCM_IDLE_EXIT,
						iowaited, &timer_interval_us);
		if (ret)
			__err("Error (%d) sending idle exit for %s\n",
					ret, core->core_name);
		start_slack_timer(core, timer_interval_us);
		break;
	}

	return ret;
}
EXPORT_SYMBOL(msm_dcvs_idle);

static int __init msm_dcvs_late_init(void)
{
	struct kobject *module_kobj = NULL;
	int ret = 0;

	module_kobj = kset_find_obj(module_kset, KBUILD_MODNAME);
	if (!module_kobj) {
		pr_err("%s: cannot find kobject for module %s\n",
				__func__, KBUILD_MODNAME);
		ret = -ENOENT;
		goto err;
	}

	cores_kobj = kobject_create_and_add("cores", module_kobj);
	if (!cores_kobj) {
		__err("Cannot create %s kobject\n", "cores");
		ret = -ENOMEM;
		goto err;
	}

	debugfs_base = debugfs_create_dir("msm_dcvs", NULL);
	if (!debugfs_base) {
		__err("Cannot create debugfs base %s\n", "msm_dcvs");
		ret = -ENOENT;
		goto err;
	}

err:
	if (ret) {
		kobject_del(cores_kobj);
		cores_kobj = NULL;
		debugfs_remove(debugfs_base);
	}

	return ret;
}
late_initcall(msm_dcvs_late_init);

static int __init msm_dcvs_early_init(void)
{
	int ret = 0;
	int i;

	if (!msm_dcvs_enabled) {
		__info("Not enabled (%d)\n", msm_dcvs_enabled);
		return 0;
	}


	/* Only need about 32kBytes for normal operation */
	ret = msm_dcvs_scm_init(SZ_32K);
	if (ret) {
		__err("Unable to initialize DCVS err=%d\n", ret);
		goto done;
	}

	for (i = 0; i < CORES_MAX; i++)
		core_list[i].dcvs_core_id = -1;
done:
	return ret;
}
postcore_initcall(msm_dcvs_early_init);
