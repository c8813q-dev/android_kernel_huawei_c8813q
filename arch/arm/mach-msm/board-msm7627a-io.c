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

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio_event.h>
#include <linux/leds.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/i2c.h>
#include <linux/input/rmi_platformdata.h>
#include <linux/input/rmi_i2c.h>
#include <linux/delay.h>
#include <linux/atmel_maxtouch.h>
#include <linux/input/ft5x06_ts.h>
#include <linux/leds-msm-tricolor.h>
#include <asm/gpio.h>
#include <asm/mach-types.h>
#include <mach/rpc_server_handset.h>
#include <mach/pmic.h>
#include <mach/socinfo.h>
#include "devices.h"
#include <linux/input/synaptics_dsx.h>

#include "board-msm7627a.h"
#include "devices-msm7x2xa.h"

/* update Qualcomm 1020 baseline houming modified
  */
#include <mach/rpc_pmapp.h>
//add keypad driver
#include "msm-keypad-devices.h"
#include <linux/hardware_self_adapt.h>
#include <linux/touch_platform_config.h>
#ifdef CONFIG_HUAWEI_NFC_PN544
#include <linux/nfc/pn544.h>
#endif

#include <linux/cyttsp4_bus.h>
#include <linux/cyttsp4_core.h>
#include <linux/cyttsp4_btn.h>
#include <linux/cyttsp4_mt.h>
#define MSM_7x27A_TOUCH_RESET_PIN 96 //13
#define MSM_7X27A_TOUCH_INT_PIN 82
atomic_t touch_detected_yet = ATOMIC_INIT(0);

//delete this line

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_INCLUDE_FW
#include "linux/cyttsp4_img.h"
static struct cyttsp4_touch_firmware cyttsp4_firmware = {
	.img = cyttsp4_img,
	.size = ARRAY_SIZE(cyttsp4_img),
	.ver = cyttsp4_ver,
	.vsize = ARRAY_SIZE(cyttsp4_ver),
};
#else
static struct cyttsp4_touch_firmware cyttsp4_firmware = {
	.img = NULL,
	.size = 0,
	.ver = NULL,
	.vsize = 0,
};
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_AUTO_LOAD_TOUCH_PARAMS
#if 0
#include <linux/cyttsp4_params.h>
static struct touch_settings cyttsp4_sett_param_regs = {
       .data = (uint8_t *)&cyttsp4_param_regs[0],
       .size = ARRAY_SIZE(cyttsp4_param_regs),
       .tag = 0,
};

static struct touch_settings cyttsp4_sett_param_size = {
       .data = (uint8_t *)&cyttsp4_param_size[0],
       .size = ARRAY_SIZE(cyttsp4_param_size),
       .tag = 0,
};
#endif

#include <linux/Config_G610_V0004.h>
static struct touch_settings cyttsp4_sett_truly_regs = {
       .data = (uint8_t *)&cyttsp4_param_regs[0],
       .size = ARRAY_SIZE(cyttsp4_param_regs),
       .tag = 0,
};

struct cyttsp4_sett_param_map cyttsp4_config_param_map[] = {
    
	[0] = {
			  .id = 0,
			  .param = &cyttsp4_sett_truly_regs,
		  },
	[1] = {
			  .id = 2,
			  .param = &cyttsp4_sett_truly_regs,//&cyttsp4_sett_truly_regs,
		  },
	[2] = {
			  .id = 4,
			  .param = &cyttsp4_sett_truly_regs,//&cyttsp4_sett_truly_regs,
		  },
	[3] = {
			  .id = 6,
			  .param = &cyttsp4_sett_truly_regs,//&cyttsp4_sett_truly_regs,
		  },
	[4] = {
			  .param = NULL,
		  },
};

static struct touch_settings cyttsp4_sett_param_regs = {
       .data = NULL,
       .size = 0,
       .tag = 0,
};

static struct touch_settings cyttsp4_sett_param_size = {
       .data = NULL,
       .size = 0,
       .tag = 0,
};

#else
static struct touch_settings cyttsp4_sett_param_regs = {
       .data = NULL,
       .size = 0,
       .tag = 0,
};

static struct touch_settings cyttsp4_sett_param_size = {
       .data = NULL,
       .size = 0,
       .tag = 0,
};

struct cyttsp4_sett_param_map cyttsp4_config_param_map[] = {
    
	[0] = {
			  .id = 0,
			  .param = NULL,
		  },
	
	[1] = {
			  .id = 2,
			  .param = NULL,
		  },
    [2] = {
			  .param = NULL,
		  },
		  
};
#endif

static struct cyttsp4_loader_platform_data _cyttsp4_loader_platform_data = {
	.fw = &cyttsp4_firmware,
	.param_regs = &cyttsp4_sett_param_regs,
	.param_size = &cyttsp4_sett_param_size,
	.param_map = cyttsp4_config_param_map,
	.flags = 0,
};


#define CYTTSP4_USE_I2C
/* #define CYTTSP4_USE_SPI */

#ifdef CYTTSP4_USE_I2C
#define CYTTSP4_I2C_NAME "cyttsp4_i2c_adapter"
#define CYTTSP4_I2C_TCH_ADR 0x1A
#define CYTTSP4_LDR_TCH_ADR 0x1A
#define CYTTSP4_I2C_IRQ_GPIO  82 /* J6.9, C19, GPMC_AD14/GPIO_38 */
#define CYTTSP4_I2C_RST_GPIO  96 /* J6.10, D18, GPMC_AD13/GPIO_37 */
#endif

#ifdef CYTTSP4_USE_SPI
#define CYTTSP4_SPI_NAME "cyttsp4_spi_adapter"
/* Change GPIO numbers when using I2C and SPI at the same time
 * Following is possible alternative:
 * IRQ: J6.17, C18, GPMC_AD12/GPIO_36
 * RST: J6.24, D17, GPMC_AD11/GPIO_35
 */
#define CYTTSP4_SPI_IRQ_GPIO 38 /* J6.9, C19, GPMC_AD14/GPIO_38 */
#define CYTTSP4_SPI_RST_GPIO 37 /* J6.10, D18, GPMC_AD13/GPIO_37 */
#endif

/* Check GPIO numbers if both I2C and SPI are enabled */
#if defined(CYTTSP4_USE_I2C) && defined(CYTTSP4_USE_SPI)
#if CYTTSP4_I2C_IRQ_GPIO == CYTTSP4_SPI_IRQ_GPIO || \
	CYTTSP4_I2C_RST_GPIO == CYTTSP4_SPI_RST_GPIO
#error "GPIO numbers should be different when both I2C and SPI are on!"
#endif
#endif
#define CY_MAXX 720
#define CY_MAXY 1280
#define CY_MINX 0
#define CY_MINY 0

#define CY_ABS_MIN_X CY_MINX
#define CY_ABS_MIN_Y CY_MINY
#define CY_ABS_MAX_X CY_MAXX
#define CY_ABS_MAX_Y CY_MAXY
#define CY_ABS_MIN_P 0
#define CY_ABS_MAX_P 255
#define CY_ABS_MIN_W 0
#define CY_ABS_MAX_W 255

#define CY_ABS_MIN_T 0

#define CY_ABS_MAX_T 15

#define CY_IGNORE_VALUE 0xFFFF
/* Cypress cyttsp4 end */

#define ATMEL_TS_I2C_NAME "maXTouch"
#define ATMEL_X_OFFSET 13
#define ATMEL_Y_OFFSET 0

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_RMI4_I2C) || \
defined(CONFIG_TOUCHSCREEN_SYNAPTICS_RMI4_I2C_MODULE)

#ifndef CLEARPAD3000_ATTEN_GPIO
#define CLEARPAD3000_ATTEN_GPIO (48)
#define CLEARPAD3000_ATTEN_GPIO_EVBD_PLUS (115)
#endif

#ifndef CLEARPAD3000_RESET_GPIO
#define CLEARPAD3000_RESET_GPIO (26)
#endif

#define KP_INDEX(row, col) ((row)*ARRAY_SIZE(kp_col_gpios) + (col))

/******************** SYNAPTICS *********************************/

/*	Synaptics Thin Driver	*/

#define CLEARPAD3000_ADDR 0x20

static unsigned char synaptic_rmi4_button_codes[] = {KEY_MENU, KEY_HOME,
							KEY_BACK};

static struct synaptics_rmi4_capacitance_button_map synaptic_rmi4_button_map = {
	.nbuttons = ARRAY_SIZE(synaptic_rmi4_button_codes),
	.map = synaptic_rmi4_button_codes,
};

static struct synaptics_rmi4_platform_data rmi4_platformdata = {
	.irq_flags = IRQF_TRIGGER_FALLING,
	.irq_gpio = CLEARPAD3000_ATTEN_GPIO_EVBD_PLUS,
	.capacitance_button_map = &synaptic_rmi4_button_map,
};

static struct i2c_board_info rmi4_i2c_devices[] = {
	{
		I2C_BOARD_INFO("synaptics_rmi4_i2c",
			CLEARPAD3000_ADDR),
		.platform_data = &rmi4_platformdata,
	},
};

/******************** SYNAPTICS *********************************/
static unsigned int kp_row_gpios[] = {31, 32, 33, 34, 35};
static unsigned int kp_col_gpios[] = {36, 37, 38, 39, 40};

static const unsigned short keymap[ARRAY_SIZE(kp_col_gpios) *
					  ARRAY_SIZE(kp_row_gpios)] = {
	[KP_INDEX(0, 0)] = KEY_7,
	[KP_INDEX(0, 1)] = KEY_DOWN,
	[KP_INDEX(0, 2)] = KEY_UP,
	[KP_INDEX(0, 3)] = KEY_RIGHT,
	[KP_INDEX(0, 4)] = KEY_ENTER,

	[KP_INDEX(1, 0)] = KEY_LEFT,
	[KP_INDEX(1, 1)] = KEY_SEND,
	[KP_INDEX(1, 2)] = KEY_1,
	[KP_INDEX(1, 3)] = KEY_4,
	[KP_INDEX(1, 4)] = KEY_CLEAR,

	[KP_INDEX(2, 0)] = KEY_6,
	[KP_INDEX(2, 1)] = KEY_5,
	[KP_INDEX(2, 2)] = KEY_8,
	[KP_INDEX(2, 3)] = KEY_3,
	[KP_INDEX(2, 4)] = KEY_NUMERIC_STAR,

	[KP_INDEX(3, 0)] = KEY_9,
	[KP_INDEX(3, 1)] = KEY_NUMERIC_POUND,
	[KP_INDEX(3, 2)] = KEY_0,
	[KP_INDEX(3, 3)] = KEY_2,
	[KP_INDEX(3, 4)] = KEY_SLEEP,

	[KP_INDEX(4, 0)] = KEY_BACK,
	[KP_INDEX(4, 1)] = KEY_HOME,
	[KP_INDEX(4, 2)] = KEY_MENU,
	[KP_INDEX(4, 3)] = KEY_VOLUMEUP,
	[KP_INDEX(4, 4)] = KEY_VOLUMEDOWN,
};

/* SURF keypad platform device information */
#ifndef CONFIG_HUAWEI_GPIO_KEYPAD  /*added by lishubin update to 1215*/
static struct gpio_event_matrix_info kp_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keymap,
	.output_gpios	= kp_row_gpios,
	.input_gpios	= kp_col_gpios,
	.noutputs	= ARRAY_SIZE(kp_row_gpios),
	.ninputs	= ARRAY_SIZE(kp_col_gpios),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info[] = {
	&kp_matrix_info.info
};

static struct gpio_event_platform_data kp_pdata = {
	.name		= "7x27a_kp",
	.info		= kp_info,
	.info_count	= ARRAY_SIZE(kp_info)
};

static struct platform_device kp_pdev = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &kp_pdata,
	},
};
#endif /*added by lishubin update to 1215*/

/* 8625 keypad device information */
static unsigned int kp_row_gpios_8625[] = {31};
static unsigned int kp_col_gpios_8625[] = {36, 37};

static const unsigned short keymap_8625[] = {
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
};

static const unsigned short keymap_8625_evt[] = {
	KEY_VOLUMEDOWN,
	KEY_VOLUMEUP,
};

static struct gpio_event_matrix_info kp_matrix_info_8625 = {
	.info.func      = gpio_event_matrix_func,
	.keymap         = keymap_8625,
	.output_gpios   = kp_row_gpios_8625,
	.input_gpios    = kp_col_gpios_8625,
	.noutputs       = ARRAY_SIZE(kp_row_gpios_8625),
	.ninputs        = ARRAY_SIZE(kp_col_gpios_8625),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags          = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info_8625[] = {
	&kp_matrix_info_8625.info,
};

static struct gpio_event_platform_data kp_pdata_8625 = {
	.name           = "7x27a_kp",
	.info           = kp_info_8625,
	.info_count     = ARRAY_SIZE(kp_info_8625)
};

static struct platform_device kp_pdev_8625 = {
	.name   = GPIO_EVENT_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data  = &kp_pdata_8625,
	},
};

#define LED_GPIO_PDM 96

#define MXT_TS_IRQ_GPIO         48
#define MXT_TS_RESET_GPIO       26
#define MXT_TS_EVBD_IRQ_GPIO    115
#define MAX_VKEY_LEN		100

static ssize_t mxt_virtual_keys_register(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	char *virtual_keys = __stringify(EV_KEY) ":" __stringify(KEY_MENU) \
		":60:840:120:80" ":" __stringify(EV_KEY) \
		":" __stringify(KEY_HOME)   ":180:840:120:80" \
		":" __stringify(EV_KEY) ":" \
		__stringify(KEY_BACK) ":300:840:120:80" \
		":" __stringify(EV_KEY) ":" \
		__stringify(KEY_SEARCH)   ":420:840:120:80" "\n";

	return snprintf(buf, strnlen(virtual_keys, MAX_VKEY_LEN) + 1 , "%s",
			virtual_keys);
}

static struct kobj_attribute mxt_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.atmel_mxt_ts",
		.mode = S_IRUGO,
	},
	.show = &mxt_virtual_keys_register,
};

static struct attribute *mxt_virtual_key_properties_attrs[] = {
	&mxt_virtual_keys_attr.attr,
	NULL,
};

static struct attribute_group mxt_virtual_key_properties_attr_group = {
	.attrs = mxt_virtual_key_properties_attrs,
};

struct kobject *mxt_virtual_key_properties_kobj;

static int mxt_vkey_setup(void)
{
	int retval = 0;

	mxt_virtual_key_properties_kobj =
		kobject_create_and_add("board_properties", NULL);
	if (mxt_virtual_key_properties_kobj)
		retval = sysfs_create_group(mxt_virtual_key_properties_kobj,
				&mxt_virtual_key_properties_attr_group);
	if (!mxt_virtual_key_properties_kobj || retval)
		pr_err("failed to create mxt board_properties\n");

	return retval;
}

static const u8 mxt_config_data[] = {
	/* T6 Object */
	0, 0, 0, 0, 0, 0,
	/* T38 Object */
	16, 1, 0, 0, 0, 0, 0, 0,
	/* T7 Object */
	32, 16, 50,
	/* T8 Object */
	30, 0, 20, 20, 0, 0, 20, 0, 50, 0,
	/* T9 Object */
	3, 0, 0, 18, 11, 0, 32, 75, 3, 3,
	0, 1, 1, 0, 10, 10, 10, 10, 31, 3,
	223, 1, 11, 11, 15, 15, 151, 43, 145, 80,
	100, 15, 0, 0, 0,
	/* T15 Object */
	131, 0, 11, 11, 1, 1, 0, 45, 3, 0,
	0,
	/* T18 Object */
	0, 0,
	/* T19 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	/* T23 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/* T25 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T40 Object */
	0, 0, 0, 0, 0,
	/* T42 Object */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* T46 Object */
	0, 2, 32, 48, 0, 0, 0, 0, 0,
	/* T47 Object */
	1, 20, 60, 5, 2, 50, 40, 0, 0, 40,
	/* T48 Object */
	1, 12, 80, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
	0, 0, 0, 0, 16, 65, 3, 1, 1, 0,
	10, 10, 10, 0, 0, 15, 15, 154, 58, 145,
	80, 100, 15, 3,
};

static const u8 mxt_config_data_evt[] = {
	/* T6 Object */
	0, 0, 0, 0, 0, 0,
	/* T38 Object */
	20, 1, 0, 25, 9, 12, 0, 0,
	/* T7 Object */
	24, 12, 10,
	/* T8 Object */
	30, 0, 20, 20, 0, 0, 0, 0, 10, 192,
	/* T9 Object */
	131, 0, 0, 18, 11, 0, 16, 70, 2, 1,
	0, 2, 1, 62, 10, 10, 10, 10, 107, 3,
	223, 1, 2, 2, 20, 20, 172, 40, 139, 110,
	10, 15, 0, 0, 0,
	/* T15 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
	/* T18 Object */
	0, 0,
	/* T19 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	/* T23 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/* T25 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T40 Object */
	0, 0, 0, 0, 0,
	/* T42 Object */
	3, 20, 45, 40, 128, 0, 0, 0,
	/* T46 Object */
	0, 2, 16, 16, 0, 0, 0, 0, 0,
	/* T47 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* T48 Object */
	1, 12, 64, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
	0, 0, 0, 0, 16, 65, 3, 1, 1, 0,
	10, 10, 10, 0, 0, 15, 15, 154, 58, 145,
	80, 100, 15, 3,
};

static struct mxt_config_info mxt_config_array[] = {
	{
		.config		= mxt_config_data,
		.config_length	= ARRAY_SIZE(mxt_config_data),
		.family_id	= 0x81,
		.variant_id	= 0x01,
		.version	= 0x10,
		.build		= 0xAA,
	},
};

static int mxt_key_codes[MXT_KEYARRAY_MAX_KEYS] = {
	[0] = KEY_HOME,
	[1] = KEY_MENU,
	[9] = KEY_BACK,
	[10] = KEY_SEARCH,
};

static struct mxt_platform_data mxt_platform_data = {
	.config_array		= mxt_config_array,
	.config_array_size	= ARRAY_SIZE(mxt_config_array),
	.panel_minx		= 0,
	.panel_maxx		= 479,
	.panel_miny		= 0,
	.panel_maxy		= 799,
	.disp_minx		= 0,
	.disp_maxx		= 479,
	.disp_miny		= 0,
	.disp_maxy		= 799,
	.irqflags		= IRQF_TRIGGER_FALLING,
	.i2c_pull_up		= true,
	.reset_gpio		= MXT_TS_RESET_GPIO,
	.irq_gpio		= MXT_TS_IRQ_GPIO,
	.key_codes		= mxt_key_codes,
};

/* update Qualcomm 1020 baseline houming modified
  */
/* -------------------- huawei touch -------------------- */
#define IC_PM_ON   1
#define IC_PM_OFF  0

static int power_switch(int pm)
{
#if 0
	int rc = 0;
	
	if (IC_PM_ON == pm)
	{
		
		vreg_l12 = vreg_get(NULL,"gp2");
		if (IS_ERR(vreg_l12)) 
		{
			pr_err("%s:l12 power init get failed\n", __func__);
			goto err_power_fail;
		}
		
		rc = vreg_set_level(vreg_l12, 2850);
		if(rc)
		{
			pr_err("%s:l12 power init faild\n",__func__);
			goto err_power_fail;
		}

		
		rc = vreg_enable(vreg_l12);
		if (rc) 
		{
			pr_err("%s:l12 power init failed \n", __func__);
		}

		mdelay(50);     

	}
	else if(IC_PM_OFF == pm)
	{
		if(NULL != vreg_l12)
		{
			rc = vreg_disable(vreg_l12);
			if (rc)
			{
				pr_err("%s:l12 power disable failed \n", __func__);
			}
		}
	}
	else 
	{
		rc = -EPERM;
		pr_err("%s:l12 power switch not support yet!\n", __func__);	
	}
err_power_fail:
	return rc;
#endif
	return 0;
}

/* modify function name and gpio config */
/*Multipe use gpio_request function has been mistakes */

/* use gpio free to release */
static int set_touch_interrupt_gpio(void)
{
    int gpio_config = 0;
    int ret = 0;

	gpio_config = GPIO_CFG(MSM_7X27A_TOUCH_INT_PIN,0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA);
	ret = gpio_tlmm_config(gpio_config, GPIO_CFG_ENABLE);
	if (ret)
	{
		pr_err("%s:touch int gpio config failed\n", __func__);
		return ret;
	}
	ret = gpio_request(MSM_7X27A_TOUCH_INT_PIN, "TOUCH_INT");
	if (ret)
	{
		pr_err("%s:touch int gpio request failed\n", __func__);
		return ret;
	}
	ret = gpio_direction_input(MSM_7X27A_TOUCH_INT_PIN);
	if (ret)
	{
		pr_err("%s:touch int gpio input failed\n", __func__);
		return ret;
	}

    gpio_free(MSM_7X27A_TOUCH_INT_PIN);
	return ret;
}
/*we use this to detect the probe is detected*/
static void set_touch_probe_flag(int detected)
{
	if(detected >= 0)
	{
		atomic_set(&touch_detected_yet, 1);
	}
	else
	{
		atomic_set(&touch_detected_yet, 0);
	}
	
	return;
}
static int read_touch_probe_flag(void)
{	
	return atomic_read(&touch_detected_yet);
}

/*this function reset touch panel */
/*Multipe use gpio_request function has been mistakes */

/* use gpio free to release */
static int touch_reset(void)
{
	int ret = 0;
    int gpio_config = 0;	
	gpio_config = GPIO_CFG(MSM_7x27A_TOUCH_RESET_PIN,0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA);
	ret = gpio_tlmm_config(gpio_config, GPIO_CFG_ENABLE);
	if (ret)
	{
		pr_err("%s:touch int gpio config failed\n", __func__);
		return ret;
	}
	ret = gpio_request(MSM_7x27A_TOUCH_RESET_PIN, "TOUCH_RESET");
	if (ret)
	{
		pr_err("%s:touch int gpio request failed\n", __func__);
		return ret;
	}
	
	ret = gpio_direction_output(MSM_7x27A_TOUCH_RESET_PIN, 1);
	mdelay(5);
	ret = gpio_direction_output(MSM_7x27A_TOUCH_RESET_PIN, 0);
	mdelay(10);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
	ret = gpio_direction_output(MSM_7x27A_TOUCH_RESET_PIN, 1);
	mdelay(50);//must more than 10ms.

	gpio_free(MSM_7x27A_TOUCH_RESET_PIN);
	return ret;
}
/*this function return reset gpio at 7x30 platform */
static int get_touch_reset_gpio(void)
{
	return MSM_7x27A_TOUCH_RESET_PIN;
}

/*this function get the tp  resolution*/
static int get_touch_resolution(struct tp_resolution_conversion *tp_resolution_type)
{	
    /* when sub boardid equals HW_VER_SUB_V1 G520 support qhd */
    hw_product_sub_type product_sub_type = get_hw_sub_board_id();

    if( machine_is_msm8x25_C8950D()
        || machine_is_msm8x25_G610C()
        || (machine_is_msm8x25_G520U() && HW_VER_SUB_V1 == product_sub_type) )
	{
	    tp_resolution_type->lcd_x = LCD_X_QHD;
		tp_resolution_type->lcd_y = LCD_Y_QHD;   
		tp_resolution_type->lcd_all = LCD_ALL_QHD_45INCHTP;
	}
	else if( machine_is_msm8x25_U8951()			
            || machine_is_msm8x25_C8813()
            || machine_is_msm8x25_G520U() 
            || machine_is_msm8x25_C8813Q() )
	{
	    tp_resolution_type->lcd_x = LCD_X_FWVGA;
		tp_resolution_type->lcd_y = LCD_Y_FWVGA;   
		tp_resolution_type->lcd_all = LCD_ALL_FWVGA_45INCHTP;
	}
	else
	{
	    tp_resolution_type->lcd_x = LCD_X_WVGA;
		tp_resolution_type->lcd_y = LCD_Y_WVGA;   
		tp_resolution_type->lcd_all = LCD_ALL_WVGA_4INCHTP;
	}
	return 1;
}
static struct touch_hw_platform_data touch_hw_data = 
{
	.touch_power = power_switch,
	.set_touch_interrupt_gpio = set_touch_interrupt_gpio,
	.set_touch_probe_flag = set_touch_probe_flag,
	.read_touch_probe_flag = read_touch_probe_flag,
	.touch_reset = touch_reset,
	.get_touch_reset_gpio = get_touch_reset_gpio,
	.get_touch_resolution = get_touch_resolution,
};

/* -------------------- huawei sensors -------------------- */
static int gs_init_flag = 0;   /*gsensor is not initialized*/
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_ADI_ADXL346
static int gsensor_support_dummyaddr_adi346(void)
{
    int ret = -1;	/*default value means actual address*/

    ret = (int)GS_ADI346;

    return ret;
}

static struct gs_platform_data gs_adi346_platform_data = {
    .adapt_fn = gsensor_support_dummyaddr_adi346,
    .slave_addr = (0xA6 >> 1),  /*i2c slave address*/
    .dev_id = 0x00,    /*WHO AM I*/
    .init_flag = &gs_init_flag,
    .get_compass_gs_position=get_compass_gs_position,
};
#endif

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_KXTIK1004
static int gsensor_support_dummyaddr_kxtik(void)
{
    int ret = -1;	/*default value means actual address*/
    ret = (int)GS_KXTIK1004;
    return ret;
}

static struct gs_platform_data gs_kxtik_platform_data = {
    .adapt_fn = gsensor_support_dummyaddr_kxtik,
    .slave_addr = (0x1E >> 1),  /*i2c slave address*/
    .dev_id = 0x05,    /*WHO AM I*/
    .init_flag = &gs_init_flag,
    .get_compass_gs_position=get_compass_gs_position,
};
#endif

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_MMA8452
static struct gs_platform_data gs_mma8452_platform_data = {
    .adapt_fn = NULL,
    .slave_addr = (0x38 >> 1),  /*i2c slave address*/
    .dev_id = 0x2A,    /*WHO AM I*/
    .init_flag = &gs_init_flag,
    .get_compass_gs_position=get_compass_gs_position,
};
#endif

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_ST_LIS3XH
static struct gs_platform_data gs_st_lis3xh_platform_data = {
    .adapt_fn = NULL,
    .slave_addr = (0x30 >> 1),  /*i2c slave address*/
    .dev_id = 0x00,    /*WHO AM I*/
    .init_flag = &gs_init_flag,
    .get_compass_gs_position=get_compass_gs_position,
};
#endif

#ifdef CONFIG_HUAWEI_FEATURE_PROXIMITY_EVERLIGHT_APS_9900
static int aps9900_gpio_config_interrupt(void)
{
    int gpio_config = 0;
    int ret = 0;
  
    gpio_config = GPIO_CFG(MSM_7X27A_APS9900_INT, 0, GPIO_CFG_INPUT,GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
    ret = gpio_tlmm_config(gpio_config, GPIO_CFG_ENABLE);
    return ret; 
}

static struct aps9900_hw_platform_data aps9900_hw_data = {
    .aps9900_gpio_config_interrupt = aps9900_gpio_config_interrupt,
};
#endif

/* -------------------- huawei nfc -------------------- */
#ifdef CONFIG_HUAWEI_NFC_PN544
/* this function is used to reset pn544 by controlling the ven pin */
static int pn544_ven_reset(void)
{
	int ret=0;
	int gpio_config=0;
	
	gpio_config = GPIO_CFG(GPIO_NFC_VEN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
	ret = gpio_tlmm_config(gpio_config, GPIO_CFG_ENABLE);
	ret = gpio_request(GPIO_NFC_VEN, "gpio 113 for NFC pn544");
	ret = gpio_direction_output(GPIO_NFC_VEN,0);
	/* pull up first, then pull down for 10 ms, and enable last */
	gpio_set_value(GPIO_NFC_VEN, 1);
	mdelay(5);
	gpio_set_value(GPIO_NFC_VEN, 0);

	mdelay(10);
	gpio_set_value(GPIO_NFC_VEN, 1);
	mdelay(5);
	return 0;
}

static int pn544_interrupt_gpio_config(void)
{
	int ret=0;
	int gpio_config=0;
	gpio_config = GPIO_CFG(GPIO_NFC_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA);
	ret = gpio_tlmm_config(gpio_config, GPIO_CFG_ENABLE);
	ret = gpio_request(GPIO_NFC_INT, "gpio 114 for NFC pn544");
	ret = gpio_direction_input(GPIO_NFC_INT);
	return 0;
}

static int pn544_fw_download_pull_down(void)
{
	gpio_set_value(GPIO_NFC_LOAD, 1);
	mdelay(5);
	gpio_set_value(GPIO_NFC_LOAD, 0);
	mdelay(5);
	return 0;	
}

static int pn544_fw_download_pull_high(void)
{
	gpio_set_value(GPIO_NFC_LOAD, 0);
	mdelay(5);
	gpio_set_value(GPIO_NFC_LOAD, 1);
	mdelay(5);
	return 0;
}

static int pn544_clock_output_ctrl(int vote)
{
       const char * id = "nfcp";
	pmapp_clock_vote(id, PMAPP_CLOCK_ID_D1,
		vote?PMAPP_CLOCK_VOTE_ON:PMAPP_CLOCK_VOTE_OFF);
	return 0;
}

// expand func function: add close PMU output function
// mode = 0 : close for clock pmu request mode,  mode = 1 : Set for clock pmu request mode
static int pn544_clock_output_mode_ctrl(int mode)
{
       const char * id = "nfcp";
	pmapp_clock_vote(id, PMAPP_CLOCK_ID_D1, 
		mode?PMAPP_CLOCK_VOTE_PIN_CTRL:PMAPP_CLOCK_VOTE_OFF);
	return 0;
}

static struct pn544_nfc_platform_data pn544_hw_data = 
{
	.pn544_ven_reset = pn544_ven_reset,
	.pn544_interrupt_gpio_config = pn544_interrupt_gpio_config,
	.pn544_fw_download_pull_down = pn544_fw_download_pull_down,
	.pn544_fw_download_pull_high = pn544_fw_download_pull_high,
	.pn544_clock_output_ctrl = pn544_clock_output_ctrl,
	.pn544_clock_output_mode_ctrl = pn544_clock_output_mode_ctrl,
};

#endif


/* Cypress cyttsp4 start */

static int cyttsp4_xres(struct cyttsp4_core_platform_data *pdata,
		struct device *dev)
{
	int rst_gpio = pdata->rst_gpio;
	int rc = 0;

	gpio_set_value(rst_gpio, 1);
	msleep(20);
	gpio_set_value(rst_gpio, 0);
	msleep(40);
	gpio_set_value(rst_gpio, 1);
	msleep(20);
	dev_info(dev,
		"%s: RESET CYTTSP gpio=%d r=%d\n", __func__,
		pdata->rst_gpio, rc);
	return rc;
}

static int cyttsp4_init(struct cyttsp4_core_platform_data *pdata,
		int on, struct device *dev)
{
	int rst_gpio = pdata->rst_gpio;
	int irq_gpio = pdata->irq_gpio;
	int rc = 0;

	if (on) {
		rc = gpio_request(rst_gpio, NULL);
		if (rc < 0) {
			gpio_free(rst_gpio);
			rc = gpio_request(rst_gpio, NULL);
		}
		if (rc < 0) {
			dev_err(dev,
				"%s: Fail request gpio=%d\n", __func__,
				rst_gpio);
		} else {
			rc = gpio_direction_output(rst_gpio, 1);
			if (rc < 0) {
				pr_err("%s: Fail set output gpio=%d\n",
					__func__, rst_gpio);
				gpio_free(rst_gpio);
			} else {
				rc = gpio_request(irq_gpio, NULL);
				if (rc < 0) {
					gpio_free(irq_gpio);
					rc = gpio_request(irq_gpio,
						NULL);
				}
				if (rc < 0) {
					dev_err(dev,
						"%s: Fail request gpio=%d\n",
						__func__, irq_gpio);
					gpio_free(rst_gpio);
				} else {
					gpio_direction_input(irq_gpio);
				}
			}
		}
	} else {
		gpio_free(rst_gpio);
		gpio_free(irq_gpio);
	}

	dev_info(dev,
		"%s: INIT CYTTSP RST gpio=%d and IRQ gpio=%d r=%d\n",
		__func__, rst_gpio, irq_gpio, rc);
	return rc;
}

static int cyttsp4_wakeup(struct cyttsp4_core_platform_data *pdata,
		struct device *dev, atomic_t *ignore_irq)
{
	int irq_gpio = pdata->irq_gpio;
	int rc = 0;

	if (ignore_irq)
		atomic_set(ignore_irq, 1);
	rc = gpio_direction_output(irq_gpio, 0);
	if (rc < 0) {
		if (ignore_irq)
			atomic_set(ignore_irq, 0);
		dev_err(dev,
			"%s: Fail set output gpio=%d\n",
			__func__, irq_gpio);
	} else {
		udelay(2000);
		rc = gpio_direction_input(irq_gpio);
		if (ignore_irq)
			atomic_set(ignore_irq, 0);
		if (rc < 0) {
			dev_err(dev,
				"%s: Fail set input gpio=%d\n",
				__func__, irq_gpio);
		}
	}

	dev_info(dev,
		"%s: WAKEUP CYTTSP gpio=%d r=%d\n", __func__,
		irq_gpio, rc);
	return rc;
}

static int cyttsp4_sleep(struct cyttsp4_core_platform_data *pdata,
		struct device *dev, atomic_t *ignore_irq)
{
	return 0;
}

static int cyttsp4_power(struct cyttsp4_core_platform_data *pdata,
		int on, struct device *dev, atomic_t *ignore_irq)
{
	if (on)
		return cyttsp4_wakeup(pdata, dev, ignore_irq);

	return cyttsp4_sleep(pdata, dev, ignore_irq);
}

static int cyttsp4_irq_stat(struct cyttsp4_core_platform_data *pdata,
		struct device *dev)
{
	return gpio_get_value(pdata->irq_gpio);
}

/* Button to keycode conversion */
static u16 cyttsp4_btn_keys[] = {
	/* use this table to map buttons to keycodes (see input.h) */
	KEY_BACK,		/* 102 */
	KEY_HOME,		/* 139 */
	KEY_MENU,		/* 158 */
	KEY_SEARCH,		/* 217 */
	KEY_VOLUMEDOWN,		/* 114 */
	KEY_VOLUMEUP,		/* 115 */
	KEY_CAMERA,		/* 212 */
	KEY_POWER		/* 116 */
};

static struct touch_settings cyttsp4_sett_btn_keys = {
	.data = (uint8_t *)&cyttsp4_btn_keys[0],
	.size = ARRAY_SIZE(cyttsp4_btn_keys),
	.tag = 0,
};

static struct cyttsp4_core_platform_data _cyttsp4_core_platform_data = {
	.irq_gpio = CYTTSP4_I2C_IRQ_GPIO,
	.rst_gpio = CYTTSP4_I2C_RST_GPIO,
	.xres = cyttsp4_xres,
	.init = cyttsp4_init,
	.power = cyttsp4_power,
	.irq_stat = cyttsp4_irq_stat,
	.sett = {
		NULL,	/* Reserved */
		NULL,	/* Command Registers */
		NULL,	/* Touch Report */
		NULL,	/* Cypress Data Record */
		NULL,	/* Test Record */
		NULL,	/* Panel Configuration Record */
		NULL, /* &cyttsp4_sett_param_regs, */
		NULL, /* &cyttsp4_sett_param_size, */
		NULL,	/* Reserved */
		NULL,	/* Reserved */
		NULL,	/* Operational Configuration Record */
		NULL, /* &cyttsp4_sett_ddata, *//* Design Data Record */
		NULL, /* &cyttsp4_sett_mdata, *//* Manufacturing Data Record */
		NULL,	/* Config and Test Registers */
		&cyttsp4_sett_btn_keys,	/* button-to-keycode table */
	},
	.loader_pdata = &_cyttsp4_loader_platform_data,
};


static struct cyttsp4_core_info cyttsp4_core_info __initdata = {
	.name = CYTTSP4_CORE_NAME,
	.id = "main_ttsp_core",
	.adap_id = CYTTSP4_I2C_NAME,
	.platform_data = &_cyttsp4_core_platform_data,
};


static const uint16_t cyttsp4_abs[] = {
	ABS_MT_POSITION_X, CY_ABS_MIN_X, CY_ABS_MAX_X, 0, 0,
	ABS_MT_POSITION_Y, CY_ABS_MIN_Y, CY_ABS_MAX_Y, 0, 0,
	ABS_MT_PRESSURE, CY_ABS_MIN_P, CY_ABS_MAX_P, 0, 0,
	CY_IGNORE_VALUE, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
	ABS_MT_TRACKING_ID, CY_ABS_MIN_T, CY_ABS_MAX_T, 0, 0,
	ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0,
	ABS_MT_TOUCH_MINOR, 0, 255, 0, 0,
	ABS_MT_ORIENTATION, -128, 127, 0, 0,
};

struct touch_framework cyttsp4_framework = {
	.abs = (uint16_t *)&cyttsp4_abs[0],
	.size = ARRAY_SIZE(cyttsp4_abs),
	.enable_vkeys = 0,
};

static struct cyttsp4_mt_platform_data _cyttsp4_mt_platform_data = {
	.frmwrk = &cyttsp4_framework,
	.flags = 0x40, //0x38,
	.inp_dev_name = CYTTSP4_MT_NAME,
};

struct cyttsp4_device_info cyttsp4_mt_info __initdata = {
	.name = CYTTSP4_MT_NAME,
	.core_id = "main_ttsp_core",
	.platform_data = &_cyttsp4_mt_platform_data,
};

static struct cyttsp4_btn_platform_data _cyttsp4_btn_platform_data = {
	.inp_dev_name = CYTTSP4_BTN_NAME,
};

struct cyttsp4_device_info cyttsp4_btn_info __initdata = {
	.name = CYTTSP4_BTN_NAME,
	.core_id = "main_ttsp_core",
	.platform_data = &_cyttsp4_btn_platform_data,
};

static void  huawei_cyttsp4_init(void)
{
	/* Register core and devices */
	cyttsp4_register_core_device(&cyttsp4_core_info);
	cyttsp4_register_device(&cyttsp4_mt_info);
	cyttsp4_register_device(&cyttsp4_btn_info);
}


/* Cypress cyttsp4 end */



static struct i2c_board_info huawei_i2c_board_info[] __initdata = 
{
/* -------------------- huawei touch -------------------- */
#ifdef CONFIG_HUAWEI_MELFAS_TOUCHSCREEN
	{
		I2C_BOARD_INFO("melfas-ts", 0x23),
		.platform_data = &touch_hw_data,
		.irq = MSM_GPIO_TO_INT(MSM_7X27A_TOUCH_INT_PIN),
        .flags = true,   /*support multi point*/
	},
#endif

#ifdef CONFIG_HUAWEI_FEATURE_RMI_TOUCH
	{
		I2C_BOARD_INFO("Synaptics_rmi", 0x70),
		.platform_data = &touch_hw_data,
		.irq = MSM_GPIO_TO_INT(MSM_7X27A_TOUCH_INT_PIN),
        .flags = true,
	},
	/* synaptics IC s2000 for U8661
	 * I2C ADDR	: 0x24
	 */
	{
		I2C_BOARD_INFO("Synaptics_rmi", 0x24),
		.platform_data = &touch_hw_data,
		.irq = MSM_GPIO_TO_INT(MSM_7X27A_TOUCH_INT_PIN),
        .flags = true,
	},
	/* only use i2c addr is 0x20 in board*/
	{
		I2C_BOARD_INFO("Synaptics_rmi", 0x20),
		.platform_data = &touch_hw_data,
		.irq = MSM_GPIO_TO_INT(MSM_7X27A_TOUCH_INT_PIN),
        .flags = true,
	},
#endif

#ifdef CONFIG_HUAWEI_GT968_TOUCHSCREEN
    {
        I2C_BOARD_INFO("Goodix-TS", 0x5d),
		.platform_data = &touch_hw_data,
		.irq = MSM_GPIO_TO_INT(MSM_7X27A_TOUCH_INT_PIN),
        .flags = true,
    },
#endif
	{
		I2C_BOARD_INFO(CYTTSP4_I2C_NAME, CYTTSP4_I2C_TCH_ADR),
		.irq = MSM_GPIO_TO_INT( CYTTSP4_I2C_IRQ_GPIO),
		.platform_data = CYTTSP4_I2C_NAME,
	},

/* -------------------- huawei keypad -------------------- */
#ifdef CONFIG_QWERTY_KEYPAD_ADP5587
	{
		I2C_BOARD_INFO("adp5587", 0x34),
		.irq = MSM_GPIO_TO_INT(40)
	},
#endif 

/* -------------------- huawei sensors -------------------- */
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_ADI_ADXL346
    {
        I2C_BOARD_INFO("gs_adi346", 0xA8>>1),  /* actual address 0xA6, fake address 0xA8*/
	     .platform_data = &gs_adi346_platform_data,
        .irq = MSM_GPIO_TO_INT(19)    //MEMS_INT1
    },
#endif 
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_KXTIK1004
	{ 
	    I2C_BOARD_INFO("gs_kxtik", 0x1E >> 1),  /* actual address 0x0F*/
	    .platform_data = &gs_kxtik_platform_data,
	    .irq = MSM_GPIO_TO_INT(19)     //MEMS_INT1
	},
#endif

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_MMA8452
    {
        I2C_BOARD_INFO("gs_mma8452", 0x38 >> 1),
        .platform_data = &gs_mma8452_platform_data,
        .irq = MSM_GPIO_TO_INT(19)    //MEMS_INT1
    },
#endif	

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_ACCELEROMETER_ST_LIS3XH
    {
        I2C_BOARD_INFO("gs_st_lis3xh", 0x30 >> 1),
	 .platform_data = &gs_st_lis3xh_platform_data,
        .irq = MSM_GPIO_TO_INT(19)    //MEMS_INT1
    },
#endif

/*fack address,because IIC is interrupt with bluetooth*/
#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AK8975
    {
        I2C_BOARD_INFO("akm8975", 0x0D),//7 bit addr, no write bit
        .irq = MSM_GPIO_TO_INT(18)
    },
#endif 

#ifdef CONFIG_HUAWEI_FEATURE_SENSORS_AK8963
    {
        I2C_BOARD_INFO("akm8963", 0x0E),//7 bit addr, no write bit
        .irq = MSM_GPIO_TO_INT(18)
    },
#endif 

#ifdef CONFIG_HUAWEI_FEATURE_PROXIMITY_EVERLIGHT_APS_12D
	{   
		I2C_BOARD_INFO("aps-12d", 0x88 >> 1),  
	},
#endif

#ifdef CONFIG_HUAWEI_FEATURE_PROXIMITY_EVERLIGHT_APS_9900
	{   
		I2C_BOARD_INFO("aps-9900", 0x39),
	    .irq = MSM_GPIO_TO_INT(MSM_7X27A_APS9900_INT),
        .platform_data = &aps9900_hw_data,
	},
#endif

/* -------------------- huawei camera flash -------------------- */
/*Register i2c information for flash tps61310*/
#ifdef CONFIG_HUAWEI_FEATURE_TPS61310
	{
		I2C_BOARD_INFO("tps61310" , 0x33),
	},
#endif

/*Add new i2c information for flash lm3642*/
#ifdef CONFIG_HUAWEI_FEATURE_LM3642
	{
		I2C_BOARD_INFO("lm3642" , 0x63),
	},
#endif

/* -------------------- huawei nfc -------------------- */
#ifdef CONFIG_HUAWEI_NFC_PN544
	{
		I2C_BOARD_INFO(PN544_DRIVER_NAME, PN544_I2C_ADDR),
		.irq = MSM_GPIO_TO_INT(GPIO_NFC_INT),
		.platform_data = &pn544_hw_data,
	},
#endif
};

static struct i2c_board_info mxt_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("atmel_mxt_ts", 0x4a),
		.platform_data = &mxt_platform_data,
		.irq = MSM_GPIO_TO_INT(MXT_TS_IRQ_GPIO),
	},
};

static int synaptics_touchpad_setup(void);

static struct msm_gpio clearpad3000_cfg_data[] = {
	{GPIO_CFG(CLEARPAD3000_ATTEN_GPIO, 0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_6MA), "rmi4_attn"},
	{GPIO_CFG(CLEARPAD3000_RESET_GPIO, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_8MA), "rmi4_reset"},
};

static struct rmi_XY_pair rmi_offset = {.x = 0, .y = 0};
static struct rmi_range rmi_clipx = {.min = 48, .max = 980};
static struct rmi_range rmi_clipy = {.min = 7, .max = 1647};
static struct rmi_f11_functiondata synaptics_f11_data = {
	.swap_axes = false,
	.flipX = false,
	.flipY = false,
	.offset = &rmi_offset,
	.button_height = 113,
	.clipX = &rmi_clipx,
	.clipY = &rmi_clipy,
};

#define MAX_LEN		100

static ssize_t clearpad3000_virtual_keys_register(struct kobject *kobj,
		     struct kobj_attribute *attr, char *buf)
{
	char *virtual_keys = __stringify(EV_KEY) ":" __stringify(KEY_MENU) \
			     ":60:830:120:60" ":" __stringify(EV_KEY) \
			     ":" __stringify(KEY_HOME)   ":180:830:120:60" \
				":" __stringify(EV_KEY) ":" \
				__stringify(KEY_SEARCH) ":300:830:120:60" \
				":" __stringify(EV_KEY) ":" \
			__stringify(KEY_BACK)   ":420:830:120:60" "\n";

	return snprintf(buf, strnlen(virtual_keys, MAX_LEN) + 1 , "%s",
			virtual_keys);
}

static struct kobj_attribute clearpad3000_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.sensor00fn11",
		.mode = S_IRUGO,
	},
	.show = &clearpad3000_virtual_keys_register,
};

static struct attribute *virtual_key_properties_attrs[] = {
	&clearpad3000_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group virtual_key_properties_attr_group = {
	.attrs = virtual_key_properties_attrs,
};

struct kobject *virtual_key_properties_kobj;

static struct rmi_functiondata synaptics_functiondata[] = {
	{
		.function_index = RMI_F11_INDEX,
		.data = &synaptics_f11_data,
	},
};

static struct rmi_functiondata_list synaptics_perfunctiondata = {
	.count = ARRAY_SIZE(synaptics_functiondata),
	.functiondata = synaptics_functiondata,
};

static struct rmi_sensordata synaptics_sensordata = {
	.perfunctiondata = &synaptics_perfunctiondata,
	.rmi_sensor_setup	= synaptics_touchpad_setup,
};

static struct rmi_i2c_platformdata synaptics_platformdata = {
	.i2c_address = 0x2c,
	.irq_type = IORESOURCE_IRQ_LOWLEVEL,
	.sensordata = &synaptics_sensordata,
};

static struct i2c_board_info synaptic_i2c_clearpad3k[] = {
	{
	I2C_BOARD_INFO("rmi4_ts", 0x2c),
	.platform_data = &synaptics_platformdata,
	},
};

static int synaptics_touchpad_setup(void)
{
	int retval = 0;

	virtual_key_properties_kobj =
		kobject_create_and_add("board_properties", NULL);
	if (virtual_key_properties_kobj)
		retval = sysfs_create_group(virtual_key_properties_kobj,
				&virtual_key_properties_attr_group);
	if (!virtual_key_properties_kobj || retval)
		pr_err("failed to create ft5202 board_properties\n");

	retval = msm_gpios_request_enable(clearpad3000_cfg_data,
		    sizeof(clearpad3000_cfg_data)/sizeof(struct msm_gpio));
	if (retval) {
		pr_err("%s:Failed to obtain touchpad GPIO %d. Code: %d.",
				__func__, CLEARPAD3000_ATTEN_GPIO, retval);
		retval = 0; /* ignore the err */
	}
	synaptics_platformdata.irq = gpio_to_irq(CLEARPAD3000_ATTEN_GPIO);

	gpio_set_value(CLEARPAD3000_RESET_GPIO, 0);
	usleep(10000);
	gpio_set_value(CLEARPAD3000_RESET_GPIO, 1);
	usleep(50000);

	return retval;
}
#endif

#ifndef CONFIG_HUAWEI_KERNEL /*added by lishubin update to 1215*/
static struct regulator_bulk_data regs_atmel[] = {
	{ .supply = "ldo12", .min_uV = 2700000, .max_uV = 3300000 },
	{ .supply = "smps3", .min_uV = 1800000, .max_uV = 1800000 },
};

#define ATMEL_TS_GPIO_IRQ 82

static int atmel_ts_power_on(bool on)
{
	int rc = on ?
		regulator_bulk_enable(ARRAY_SIZE(regs_atmel), regs_atmel) :
		regulator_bulk_disable(ARRAY_SIZE(regs_atmel), regs_atmel);

	if (rc)
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, on ? "en" : "dis", rc);
	else
		msleep(50);

	return rc;
}

static int atmel_ts_platform_init(struct i2c_client *client)
{
	int rc;
	struct device *dev = &client->dev;

	rc = regulator_bulk_get(dev, ARRAY_SIZE(regs_atmel), regs_atmel);
	if (rc) {
		dev_err(dev, "%s: could not get regulators: %d\n",
				__func__, rc);
		goto out;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_atmel), regs_atmel);
	if (rc) {
		dev_err(dev, "%s: could not set voltages: %d\n",
				__func__, rc);
		goto reg_free;
	}

	rc = gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc) {
		dev_err(dev, "%s: gpio_tlmm_config for %d failed\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto reg_free;
	}

	/* configure touchscreen interrupt gpio */
	rc = gpio_request(ATMEL_TS_GPIO_IRQ, "atmel_maxtouch_gpio");
	if (rc) {
		dev_err(dev, "%s: unable to request gpio %d\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto ts_gpio_tlmm_unconfig;
	}

	rc = gpio_direction_input(ATMEL_TS_GPIO_IRQ);
	if (rc < 0) {
		dev_err(dev, "%s: unable to set the direction of gpio %d\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto free_ts_gpio;
	}
	return 0;

free_ts_gpio:
	gpio_free(ATMEL_TS_GPIO_IRQ);
ts_gpio_tlmm_unconfig:
	gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
reg_free:
	regulator_bulk_free(ARRAY_SIZE(regs_atmel), regs_atmel);
out:
	return rc;
}

static int atmel_ts_platform_exit(struct i2c_client *client)
{
	gpio_free(ATMEL_TS_GPIO_IRQ);
	gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	regulator_bulk_free(ARRAY_SIZE(regs_atmel), regs_atmel);
	return 0;
}

static u8 atmel_ts_read_chg(void)
{
	return gpio_get_value(ATMEL_TS_GPIO_IRQ);
}

static u8 atmel_ts_valid_interrupt(void)
{
	return !atmel_ts_read_chg();
}


static struct maxtouch_platform_data atmel_ts_pdata = {
	.numtouch = 4,
	.init_platform_hw = atmel_ts_platform_init,
	.exit_platform_hw = atmel_ts_platform_exit,
	.power_on = atmel_ts_power_on,
	.display_res_x = 480,
	.display_res_y = 864,
	.min_x = ATMEL_X_OFFSET,
	.max_x = (505 - ATMEL_X_OFFSET),
	.min_y = ATMEL_Y_OFFSET,
	.max_y = (863 - ATMEL_Y_OFFSET),
	.valid_interrupt = atmel_ts_valid_interrupt,
	.read_chg = atmel_ts_read_chg,
};

static struct i2c_board_info atmel_ts_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO(ATMEL_TS_I2C_NAME, 0x4a),
		.platform_data = &atmel_ts_pdata,
		.irq = MSM_GPIO_TO_INT(ATMEL_TS_GPIO_IRQ),
	},
};
#endif /*added by lishubin update to 1215*/

static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_pdev = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

#define FT5X06_IRQ_GPIO		48
#define FT5X06_RESET_GPIO	26

#define FT5X16_IRQ_GPIO		122
#define FT5X16_IRQ_GPIO_EVBD	115
#define FT5X16_IRQ_GPIO_SKUD	121

static ssize_t
ft5x06_virtual_keys_register(struct kobject *kobj,
			     struct kobj_attribute *attr,
			     char *buf)
{
	return snprintf(buf, 200,
	__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":40:510:80:60"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":120:510:80:60"
	":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":200:510:80:60"
	":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":280:510:80:60"
	"\n");
}

static ssize_t ft5x16_virtual_keys_register(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 200, \
	__stringify(EV_KEY) ":" __stringify(KEY_HOME) ":68:992:135:64" \
	":" __stringify(EV_KEY) ":" __stringify(KEY_MENU) ":203:992:135:64" \
	":" __stringify(EV_KEY) ":" __stringify(KEY_BACK) ":338:992:135:64" \
	":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":473:992:135:64" \
	"\n");
}

static struct kobj_attribute ft5x06_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.ft5x06_ts",
		.mode = S_IRUGO,
	},
	.show = &ft5x06_virtual_keys_register,
};

static struct attribute *ft5x06_virtual_key_properties_attrs[] = {
	&ft5x06_virtual_keys_attr.attr,
	NULL,
};

static struct attribute_group ft5x06_virtual_key_properties_attr_group = {
	.attrs = ft5x06_virtual_key_properties_attrs,
};

struct kobject *ft5x06_virtual_key_properties_kobj;

static struct ft5x06_ts_platform_data ft5x06_platformdata = {
	.x_max		= 320,
	.y_max		= 480,
	.reset_gpio	= FT5X06_RESET_GPIO,
	.irq_gpio	= FT5X06_IRQ_GPIO,
	.irqflags	= IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
};

static struct i2c_board_info ft5x06_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("ft5x06_ts", 0x38),
		.platform_data = &ft5x06_platformdata,
		.irq = MSM_GPIO_TO_INT(FT5X06_IRQ_GPIO),
	},
};

static void __init ft5x06_touchpad_setup(void)
{
	int rc;
	int irq_gpio;

	if (machine_is_qrd_skud_prime()) {
		irq_gpio = FT5X16_IRQ_GPIO;

		ft5x06_platformdata.x_max = 540;
		ft5x06_platformdata.y_max = 960;
		ft5x06_platformdata.irq_gpio = FT5X16_IRQ_GPIO;

		ft5x06_device_info[0].irq = MSM_GPIO_TO_INT(FT5X16_IRQ_GPIO);

		ft5x06_virtual_keys_attr.show = &ft5x16_virtual_keys_register;
	} else if(machine_is_msm8625q_evbd()) {
		irq_gpio = FT5X16_IRQ_GPIO_EVBD;

		ft5x06_platformdata.x_max = 540;
		ft5x06_platformdata.y_max = 960;
		ft5x06_platformdata.irq_gpio = FT5X16_IRQ_GPIO_EVBD;

		ft5x06_device_info[0].irq = MSM_GPIO_TO_INT(FT5X16_IRQ_GPIO_EVBD);

		ft5x06_virtual_keys_attr.show = &ft5x16_virtual_keys_register;
	} else if(machine_is_msm8625q_skud()) {
		irq_gpio = FT5X16_IRQ_GPIO_SKUD;

		ft5x06_platformdata.x_max = 540;
		ft5x06_platformdata.y_max = 960;
		ft5x06_platformdata.irq_gpio = FT5X16_IRQ_GPIO_SKUD;

		ft5x06_device_info[0].irq = MSM_GPIO_TO_INT(FT5X16_IRQ_GPIO_SKUD);

		ft5x06_virtual_keys_attr.show = &ft5x16_virtual_keys_register;
	} else
		irq_gpio = FT5X06_IRQ_GPIO;

	rc = gpio_tlmm_config(GPIO_CFG(irq_gpio, 0,
			GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
			GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc)
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, irq_gpio);

	rc = gpio_tlmm_config(GPIO_CFG(FT5X06_RESET_GPIO, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
			GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc)
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, FT5X06_RESET_GPIO);

	ft5x06_virtual_key_properties_kobj =
			kobject_create_and_add("board_properties", NULL);

	if (ft5x06_virtual_key_properties_kobj)
		rc = sysfs_create_group(ft5x06_virtual_key_properties_kobj,
				&ft5x06_virtual_key_properties_attr_group);

	if (!ft5x06_virtual_key_properties_kobj || rc)
		pr_err("%s: failed to create board_properties\n", __func__);

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				ft5x06_device_info,
				ARRAY_SIZE(ft5x06_device_info));
}

/* SKU3/SKU7 keypad device information */
#define KP_INDEX_SKU3(row, col) ((row)*ARRAY_SIZE(kp_col_gpios_sku3) + (col))
static unsigned int kp_row_gpios_sku3[] = {31, 32};
static unsigned int kp_col_gpios_sku3[] = {36, 37};

static const unsigned short keymap_sku3[] = {
	[KP_INDEX_SKU3(0, 0)] = KEY_VOLUMEUP,
	[KP_INDEX_SKU3(0, 1)] = KEY_VOLUMEDOWN,
	[KP_INDEX_SKU3(1, 1)] = KEY_CAMERA,
};

static unsigned int kp_row_gpios_skud[] = {31, 32};
static unsigned int kp_col_gpios_skud[] = {37};

static unsigned int kp_row_gpios_evbdp[] = {42, 37};
static unsigned int kp_col_gpios_evbdp[] = {31};

static const unsigned short keymap_skud[] = {
	[KP_INDEX_SKU3(0, 0)] = KEY_VOLUMEUP,
	[KP_INDEX_SKU3(0, 1)] = KEY_VOLUMEDOWN,
};


static struct gpio_event_matrix_info kp_matrix_info_sku3 = {
	.info.func      = gpio_event_matrix_func,
	.keymap         = keymap_sku3,
	.output_gpios   = kp_row_gpios_sku3,
	.input_gpios    = kp_col_gpios_sku3,
	.noutputs       = ARRAY_SIZE(kp_row_gpios_sku3),
	.ninputs        = ARRAY_SIZE(kp_col_gpios_sku3),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags          = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
				GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info_sku3[] = {
	&kp_matrix_info_sku3.info,
};
static struct gpio_event_platform_data kp_pdata_sku3 = {
	.name           = "7x27a_kp",
	.info           = kp_info_sku3,
	.info_count     = ARRAY_SIZE(kp_info_sku3)
};

static struct platform_device kp_pdev_sku3 = {
	.name   = GPIO_EVENT_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data  = &kp_pdata_sku3,
	},
};

static struct led_info ctp_backlight_info = {
	.name           = "button-backlight",
	.flags          = PM_MPP__I_SINK__LEVEL_40mA << 16 | PM_MPP_7,
};

static struct led_platform_data ctp_backlight_pdata = {
	.leds = &ctp_backlight_info,
	.num_leds = 1,
};

static struct platform_device pmic_mpp_leds_pdev = {
	.name   = "pmic-mpp-leds",
	.id     = -1,
	.dev    = {
		.platform_data  = &ctp_backlight_pdata,
	},
};

static struct led_info tricolor_led_info[] = {
	[0] = {
		.name           = "red",
		.flags          = LED_COLOR_RED,
	},
	[1] = {
		.name           = "green",
		.flags          = LED_COLOR_GREEN,
	},
};

static struct led_platform_data tricolor_led_pdata = {
	.leds = tricolor_led_info,
	.num_leds = ARRAY_SIZE(tricolor_led_info),
};

static struct platform_device tricolor_leds_pdev = {
	.name   = "msm-tricolor-leds",
	.id     = -1,
	.dev    = {
		.platform_data  = &tricolor_led_pdata,
	},
};

void __init msm7627a_add_io_devices(void)
{
	/* touchscreen */
#ifndef CONFIG_HUAWEI_KERNEL
	if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa()) {
		atmel_ts_pdata.min_x = 0;
		atmel_ts_pdata.max_x = 480;
		atmel_ts_pdata.min_y = 0;
		atmel_ts_pdata.max_y = 320;
	}

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				atmel_ts_i2c_info,
				ARRAY_SIZE(atmel_ts_i2c_info));
#else
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID, 
		huawei_i2c_board_info,
		ARRAY_SIZE(huawei_i2c_board_info));
#endif

#ifndef CONFIG_HUAWEI_GPIO_KEYPAD
	/* keypad */
	platform_device_register(&kp_pdev);
#else
	platform_device_register(&keypad_device_default);
#endif
	/* headset */
	platform_device_register(&hs_pdev);

	/* LED: configure it as a pdm function */
	if (gpio_tlmm_config(GPIO_CFG(LED_GPIO_PDM, 3,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE))
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, LED_GPIO_PDM);
	else
		platform_device_register(&led_pdev);

	/* Vibrator */
#ifndef CONFIG_HUAWEI_KERNEL
	if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa()
					|| machine_is_msm8625_ffa())
#endif
		msm_init_pmic_vibrator();
	huawei_cyttsp4_init();
}

void __init qrd7627a_add_io_devices(void)
{
	int rc;

	/* touchscreen */
	if (machine_is_msm7627a_qrd1()) {
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					synaptic_i2c_clearpad3k,
					ARRAY_SIZE(synaptic_i2c_clearpad3k));
	} else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() ||
			machine_is_msm8625_evt()) {
		/* Use configuration data for EVT */
		if (machine_is_msm8625_evt()) {
			mxt_config_array[0].config = mxt_config_data_evt;
			mxt_config_array[0].config_length =
					ARRAY_SIZE(mxt_config_data_evt);
			mxt_platform_data.panel_maxy = 875;
			mxt_platform_data.need_calibration = true;
			mxt_vkey_setup();
		}

		rc = gpio_tlmm_config(GPIO_CFG(MXT_TS_IRQ_GPIO, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config for %d failed\n",
				__func__, MXT_TS_IRQ_GPIO);
		}

		rc = gpio_tlmm_config(GPIO_CFG(MXT_TS_RESET_GPIO, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config for %d failed\n",
				__func__, MXT_TS_RESET_GPIO);
		}

		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					mxt_device_info,
					ARRAY_SIZE(mxt_device_info));
	} else if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7()
				|| machine_is_qrd_skud_prime()
				|| machine_is_msm8625q_skud()
				|| machine_is_msm8625q_evbd()) {
		ft5x06_touchpad_setup();
		/* evbd+ can support synaptic as well */
		if (machine_is_msm8625q_evbd() &&
			(socinfo_get_platform_type() == 0x13)) {
			/* for QPR EVBD+ with synaptic touch panel */
			/* TODO: Add  gpio request to the driver
				to support proper dynamic touch detection */
			gpio_tlmm_config(
				GPIO_CFG(CLEARPAD3000_ATTEN_GPIO_EVBD_PLUS, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);

			gpio_tlmm_config(
				GPIO_CFG(CLEARPAD3000_RESET_GPIO, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);

			gpio_set_value(CLEARPAD3000_RESET_GPIO, 0);
			usleep(10000);
			gpio_set_value(CLEARPAD3000_RESET_GPIO, 1);
			usleep(50000);

			i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				rmi4_i2c_devices,
				ARRAY_SIZE(rmi4_i2c_devices));
		}
		else {
			if (machine_is_msm8625q_evbd()) {
				mxt_config_array[0].config = mxt_config_data;
				mxt_config_array[0].config_length =
				ARRAY_SIZE(mxt_config_data);
				mxt_platform_data.panel_maxy = 875;
				mxt_platform_data.need_calibration = true;
				mxt_platform_data.irq_gpio = MXT_TS_EVBD_IRQ_GPIO;
				mxt_vkey_setup();

			rc = gpio_tlmm_config(GPIO_CFG(MXT_TS_EVBD_IRQ_GPIO, 0,
						GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
						GPIO_CFG_8MA), GPIO_CFG_ENABLE);
			if (rc) {
				pr_err("%s: gpio_tlmm_config for %d failed\n",
						__func__, MXT_TS_EVBD_IRQ_GPIO);
			}

			rc = gpio_tlmm_config(GPIO_CFG(MXT_TS_RESET_GPIO, 0,
						GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
						GPIO_CFG_8MA), GPIO_CFG_ENABLE);
			if (rc) {
				pr_err("%s: gpio_tlmm_config for %d failed\n",
						__func__, MXT_TS_RESET_GPIO);
			}

			i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				mxt_device_info,
				ARRAY_SIZE(mxt_device_info));
			}
		}
	}

	/* handset and power key*/
	/* ignore end key as this target doesn't need it */
	hs_platform_data.ignore_end_key = true;
	platform_device_register(&hs_pdev);

	/* vibrator */
#ifdef CONFIG_MSM_RPC_VIBRATOR
	msm_init_pmic_vibrator();
#endif

	/* keypad */

	if (machine_is_qrd_skud_prime() || machine_is_msm8625q_evbd()
		|| machine_is_msm8625q_skud()) {
		kp_matrix_info_sku3.keymap = keymap_skud;
		kp_matrix_info_sku3.output_gpios = kp_row_gpios_skud;
		kp_matrix_info_sku3.input_gpios = kp_col_gpios_skud;
		kp_matrix_info_sku3.noutputs = ARRAY_SIZE(kp_row_gpios_skud);
		kp_matrix_info_sku3.ninputs = ARRAY_SIZE(kp_col_gpios_skud);
		/* keypad info for EVBD+ */
		if (machine_is_msm8625q_evbd() &&
			(socinfo_get_platform_type() == 0x13)) {
			gpio_tlmm_config(GPIO_CFG(37, 0,
						GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
						GPIO_CFG_8MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(42, 0,
						GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
						GPIO_CFG_8MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(31, 0,
						GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
						GPIO_CFG_8MA), GPIO_CFG_ENABLE);
			kp_matrix_info_sku3.output_gpios = kp_row_gpios_evbdp;
			kp_matrix_info_sku3.input_gpios = kp_col_gpios_evbdp;
			kp_matrix_info_sku3.noutputs = ARRAY_SIZE(kp_row_gpios_evbdp);
			kp_matrix_info_sku3.ninputs = ARRAY_SIZE(kp_col_gpios_evbdp);
		}
	}

	if (machine_is_msm8625_evt())
		kp_matrix_info_8625.keymap = keymap_8625_evt;

	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() ||
			machine_is_msm8625_evt())
		platform_device_register(&kp_pdev_8625);
	else if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7()
		|| machine_is_qrd_skud_prime() || machine_is_msm8625q_evbd()
		|| machine_is_msm8625q_skud())
		platform_device_register(&kp_pdev_sku3);

	/* leds */

	if (machine_is_qrd_skud_prime() || machine_is_msm8625q_evbd()
		|| machine_is_msm8625q_skud()) {
		ctp_backlight_info.flags =
			PM_MPP__I_SINK__LEVEL_40mA << 16 | PM_MPP_8;
	}

	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() ||
		machine_is_msm8625_evt() || machine_is_qrd_skud_prime()
		|| machine_is_msm8625q_evbd() || machine_is_msm8625q_skud())
		platform_device_register(&pmic_mpp_leds_pdev);

	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() ||
		machine_is_msm8625_evt() || machine_is_msm8625q_evbd())
		platform_device_register(&tricolor_leds_pdev);
}
