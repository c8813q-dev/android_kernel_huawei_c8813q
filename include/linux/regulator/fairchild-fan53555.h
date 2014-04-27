/*
 * Copyright (c) Huawei Technologies Co., Ltd. 1998-2011. All rights reserved.
 *
 * File name: fairchild-fan53555.h
 * Author:  ChenDeng  ID£º00202488
 *
 * Version: 0.1
 * Date: 2012/12/21
 *
 * Description: Fairchild fan53555 chip driver.               
 */

#ifndef __FAN53555_H__
#define __FAN53555_H__

enum {
	FAN53555_VSEL0,
	FAN53555_VSEL1,
};

struct fan53555_platform_data {
	struct regulator_init_data *init_data;
	int default_vsel;
	int slew_rate_ns;
	int discharge_enable;
	int rearm_disable;
};

#endif
