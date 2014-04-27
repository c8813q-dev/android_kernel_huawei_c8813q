/*
 * Copyright (c) Huawei Technologies Co., Ltd. 1998-2011. All rights reserved.
 *
 * File name: fairchild-fan53555.c
 * Author:  ChenDeng  ID£º00202488
 *
 * Version: 0.1
 * Date: 2012/12/21
 *
 * Description: Fairchild fan53555 chip driver.               
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/regulator/driver.h>
#include <linux/regmap.h>
#include <linux/regulator/fairchild-fan53555.h>

extern const char *dcdc_type;

/* registers */
#define REG_FAN53555_PID		       0x03
#define REG_FAN53555_PROGVSEL1		0x01
#define REG_FAN53555_PROGVSEL0		0x00
#define REG_FAN53555_CONTROL           0x02

/* constraints */
#define FAN53555_MIN_VOLTAGE_UV      603000
#define FAN53555_STEP_VOLTAGE_UV	12826
#define FAN53555_MIN_SLEW_NS		200
#define FAN53555_MAX_SLEW_NS		3200

/* bits */
#define FAN53555_ENABLE			       BIT(7)
#define FAN53555_DVS_PWM_MODE		BIT(5)
#define FAN53555_PWM_MODE		       BIT(6)
#define FAN53555_PGOOD_DISCHG		BIT(7)

#define FAN53555_VOUT_SEL_MASK		0x3F
#define FAN53555_SLEW_MASK		       0x70
#define FAN53555_SLEW_SHIFT		0x4

#define FAN53555_NVOLTAGES	    64	  /* Numbers of voltages */
/*
 *  FAN53555 slew rate table:
 *  0 000 = 12.826mV step / 0.2us
 *  1 001 = 12.826mV step / 0.4us
 *  2 010 = 12.826mV step / 0.8us
 *  3 011 = 12.826mV step / 1.6us
 *  4 100 = 12.826mV step / 3.2us
 *  5 101 = 12.826mV step / 6.4us
 *  6 110 = 12.826mV step / 12.8us
 *  7 111 = 12.826mV step / 25.6us
 */
#define FAN53555_SLEW0_NS            200
#define FAN53555_SLEW1_NS            400
#define FAN53555_SLEW2_NS            800
#define FAN53555_SLEW3_NS            1600
#define FAN53555_SLEW4_NS            3200

#define DEBUG_IS_ON 0

struct fan53555_info {
	struct regulator_dev *regulator;
	struct regulator_init_data *init_data;
	struct regmap *regmap;
	struct device *dev;
	unsigned int vsel_reg;
	unsigned int mode_bit;
	int curr_voltage;
	int slew_rate;
};

static void dump_registers(struct fan53555_info *dd,
			unsigned int reg, const char *func)
{
	unsigned int val = 0;

	regmap_read(dd->regmap, reg, &val);
	dev_dbg(dd->dev, "%s: FAN53555: Reg = %x, Val = %x\n", func, reg, val);
}

static void fan53555_slew_delay(struct fan53555_info *dd,
					int prev_uV, int new_uV)
{
	u8 val;
	int delay;
	
	val = abs(prev_uV - new_uV) / FAN53555_STEP_VOLTAGE_UV;
	delay =  (val * dd->slew_rate / 1000) + 1;
#if DEBUG_IS_ON
       printk("--fan53555_slew_delay: slew_rate:%d, delay:%d \n",dd->slew_rate,delay);
#endif
	dev_dbg(dd->dev, "Slew Delay = %d\n", delay);

	udelay(delay);
}

static int fan53555_enable(struct regulator_dev *rdev)
{
	int rc;
	struct fan53555_info *dd = rdev_get_drvdata(rdev);
#if DEBUG_IS_ON
       printk("--fan53555_enable\n");
#endif
	rc = regmap_update_bits(dd->regmap, dd->vsel_reg,
				FAN53555_ENABLE, FAN53555_ENABLE);
	if (rc)
		dev_err(dd->dev, "Unable to enable regualtor rc(%d)", rc);

	dump_registers(dd, dd->vsel_reg, __func__);

	return rc;
}

static int fan53555_disable(struct regulator_dev *rdev)
{
	int rc;
	struct fan53555_info *dd = rdev_get_drvdata(rdev);
#if DEBUG_IS_ON
       printk("--fan53555_disable\n");
#endif
	rc = regmap_update_bits(dd->regmap, dd->vsel_reg,
					FAN53555_ENABLE, 0);
	if (rc)
		dev_err(dd->dev, "Unable to disable regualtor rc(%d)", rc);

	dump_registers(dd, dd->vsel_reg, __func__);

	return rc;
}

static int fan53555_get_voltage(struct regulator_dev *rdev)
{
	unsigned int val;
	int rc;
	struct fan53555_info *dd = rdev_get_drvdata(rdev);

	rc = regmap_read(dd->regmap, dd->vsel_reg, &val);
	if (rc) {
		dev_err(dd->dev, "Unable to get volatge rc(%d)", rc);
		return rc;
	}
	dd->curr_voltage = ((val & FAN53555_VOUT_SEL_MASK) *
			FAN53555_STEP_VOLTAGE_UV) + FAN53555_MIN_VOLTAGE_UV;
#if DEBUG_IS_ON
       printk("--fan53555_get_voltage- curr_voltage: %d \n", dd->curr_voltage);
#endif
	dump_registers(dd, dd->vsel_reg, __func__);

	return dd->curr_voltage;
}

static int fan53555_set_voltage(struct regulator_dev *rdev,
			int min_uV, int max_uV, unsigned *selector)
{
	int rc, set_val, new_uV;
	struct fan53555_info *dd = rdev_get_drvdata(rdev);

	set_val = DIV_ROUND_UP(min_uV - FAN53555_MIN_VOLTAGE_UV,
					FAN53555_STEP_VOLTAGE_UV);
	new_uV = (set_val * FAN53555_STEP_VOLTAGE_UV) +
					FAN53555_MIN_VOLTAGE_UV;
#if DEBUG_IS_ON
	printk("--fan53555_set_voltage: min_uV: %d; max_uV: %d; new_uV:%d; set_val:%d \n",
	           min_uV, max_uV, new_uV, set_val);
#endif
	if ((new_uV - max_uV) > FAN53555_STEP_VOLTAGE_UV) {
		dev_err(dd->dev, "Unable to set volatge (%d %d)\n",
							min_uV, max_uV);
		return -EINVAL;
	}

	rc = regmap_update_bits(dd->regmap, dd->vsel_reg,
		FAN53555_VOUT_SEL_MASK, (set_val & FAN53555_VOUT_SEL_MASK));
	if (rc) {
		dev_err(dd->dev, "Unable to set volatge (%d %d)\n",
							min_uV, max_uV);
	} else {
		fan53555_slew_delay(dd, dd->curr_voltage, new_uV);
		dd->curr_voltage = new_uV;
	}

	dump_registers(dd, dd->vsel_reg, __func__);

	return rc;
}

static int fan53555_set_mode(struct regulator_dev *rdev,
					unsigned int mode)
{
	int rc;
	struct fan53555_info *dd = rdev_get_drvdata(rdev);

	/* only FAST and NORMAL mode types are supported */
	if (mode != REGULATOR_MODE_FAST && mode != REGULATOR_MODE_NORMAL) {
		dev_err(dd->dev, "Mode %d not supported\n", mode);
		return -EINVAL;
	}
#if DEBUG_IS_ON
	printk("--fan53555_set_mode: VSEL_reg:%d \n",dd->vsel_reg);
#endif
	rc = regmap_update_bits(dd->regmap, dd->vsel_reg, dd->mode_bit,
			(mode == REGULATOR_MODE_FAST) ? dd->mode_bit : 0);
	if (rc) {
		dev_err(dd->dev, "Unable to set operating mode rc(%d)", rc);
		return rc;
	}

	dump_registers(dd, dd->vsel_reg, __func__);

	return rc;
}

static unsigned int fan53555_get_mode(struct regulator_dev *rdev)
{
	unsigned int val;
	int rc;
	struct fan53555_info *dd = rdev_get_drvdata(rdev);

	rc = regmap_read(dd->regmap, dd->vsel_reg, &val);
	if (rc) {
		dev_err(dd->dev, "Unable to get regulator mode rc(%d)\n", rc);
		return rc;
	}
#if DEBUG_IS_ON
       printk("--fan53555_get_mode: VSEL_reg:%d, val:%d \n", dd->vsel_reg, val);
#endif

	dump_registers(dd, dd->vsel_reg, __func__);

	if (val & dd->mode_bit)
		return REGULATOR_MODE_FAST;

	return REGULATOR_MODE_NORMAL;
}

static struct regulator_ops fan53555_ops = {
	.set_voltage = fan53555_set_voltage,
	.get_voltage = fan53555_get_voltage,
	.enable = fan53555_enable,
	.disable = fan53555_disable,
	.set_mode = fan53555_set_mode,
	.get_mode = fan53555_get_mode,
};

static struct regulator_desc rdesc = {
	.name = "fan53555",
	.owner = THIS_MODULE,
	.n_voltages = FAN53555_NVOLTAGES,
	.ops = &fan53555_ops,
};

static int __devinit fan53555_init(struct fan53555_info *dd,
			const struct fan53555_platform_data *pdata)
{
	int rc;
	unsigned int val;

	switch (pdata->default_vsel) {
	case FAN53555_VSEL0:
		dd->vsel_reg = REG_FAN53555_PROGVSEL0;
		dd->mode_bit = FAN53555_PWM_MODE;
	break;
	case FAN53555_VSEL1:
		dd->vsel_reg = REG_FAN53555_PROGVSEL1;
		dd->mode_bit = FAN53555_PWM_MODE;
	break;
	default:
		dev_err(dd->dev, "Invalid VSEL ID %d\n", pdata->default_vsel);
		return -EINVAL;
	}

	/* get the current programmed voltage */
	rc = regmap_read(dd->regmap, dd->vsel_reg, &val);
	if (rc) {
		dev_err(dd->dev, "Unable to get volatge rc(%d)", rc);
		return rc;
	}
	dd->curr_voltage = ((val & FAN53555_VOUT_SEL_MASK) *
			FAN53555_STEP_VOLTAGE_UV) + FAN53555_MIN_VOLTAGE_UV;

#if DEBUG_IS_ON
       printk("--fan53555_init: current_voltage: %d \n", dd->curr_voltage);
#endif

	/* set discharge */
	rc = regmap_update_bits(dd->regmap, REG_FAN53555_CONTROL,
					FAN53555_PGOOD_DISCHG,
					(pdata->discharge_enable ?
					FAN53555_PGOOD_DISCHG : 0));
	if (rc) {
		dev_err(dd->dev, "Unable to set Active Discharge rc(%d)\n", rc);
		return -EINVAL;
	}

	/* set slew rate */
	/*
	 *  FAN53555 slew rate table:
	 *  0 000 = 12.826mV step / 0.2us
	 *  1 001 = 12.826mV step / 0.4us
	 *  2 010 = 12.826mV step / 0.8us
	 *  3 011 = 12.826mV step / 1.6us
	 *  4 100 = 12.826mV step / 3.2us
	 *  5 101 = 12.826mV step / 6.4us
	 *  6 110 = 12.826mV step / 12.8us
	 *  7 111 = 12.826mV step / 25.6us
	 */
	if (pdata->slew_rate_ns < FAN53555_MIN_SLEW_NS ||
			pdata->slew_rate_ns > FAN53555_MAX_SLEW_NS) {
		dev_err(dd->dev, "Invalid slew rate %d\n", pdata->slew_rate_ns);
		return -EINVAL;
	}

	switch (pdata->slew_rate_ns) 
	{
	    case FAN53555_SLEW0_NS:
	        val = 0;
	    break;
	
	    case FAN53555_SLEW1_NS:
	        val = 1;
	    break;

	    case FAN53555_SLEW2_NS:
	        val = 2;
	    break;

	    case FAN53555_SLEW3_NS:
	        val = 3;
	    break;

	    case FAN53555_SLEW4_NS:
	        val = 4;
	    break;
	
	    default:
	        dev_err(dd->dev, "Invalid slew rate %d\n", pdata->slew_rate_ns);
	        return -EINVAL;
	}
	
	dd->slew_rate = pdata->slew_rate_ns;
#if DEBUG_IS_ON
	printk("--fan53555_init: slew_rate: %d \n", dd->slew_rate);
#endif
	rc = regmap_update_bits(dd->regmap, REG_FAN53555_CONTROL,
			FAN53555_SLEW_MASK, val << FAN53555_SLEW_SHIFT);
	if (rc)
		dev_err(dd->dev, "Unable to set slew rate rc(%d)\n", rc);

	dump_registers(dd, dd->vsel_reg, __func__);
	dump_registers(dd, REG_FAN53555_CONTROL, __func__);

	return rc;
}

static struct regmap_config fan53555_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int __devinit fan53555_regulator_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	int rc;
	unsigned int val = 0;
	struct fan53555_info *dd;
	const struct fan53555_platform_data *pdata;

	pdata = client->dev.platform_data;
	if (!pdata) {
		dev_err(&client->dev, "Platform data not specified\n");
		return -EINVAL;
	}

	dd = devm_kzalloc(&client->dev, sizeof(*dd), GFP_KERNEL);
	if (!dd) {
		dev_err(&client->dev, "Unable to allocate memory\n");
		return -ENOMEM;
	}
	
	dd->regmap = devm_regmap_init_i2c(client, &fan53555_regmap_config);
	if (IS_ERR(dd->regmap)) 
	{
		if (dd)
		{
		    devm_kfree(&client->dev, dd);
		    dd = NULL;
		}
		dev_err(&client->dev, "Error allocating regmap\n");
		return PTR_ERR(dd->regmap);
	}
	
	rc = regmap_read(dd->regmap, REG_FAN53555_PID, &val);
	
	if (rc) 
	{
        if (dd)
        {
            devm_kfree(&client->dev, dd);
            dd = NULL;
        }
		dev_err(&client->dev, "Unable to identify FAN53555, rc(%d)\n", rc);
		return rc;
	}
	printk("--fan53555_regulator_probe: Read fan53555 ver(%d)\n", val);

	dd->init_data = pdata->init_data;
	dd->dev = &client->dev;
	i2c_set_clientdata(client, dd);

	rc = fan53555_init(dd, pdata);
	if (rc) 
	{
	    if (dd)
		{
		    devm_kfree(&client->dev, dd);
		    dd = NULL;
		}
		dev_err(&client->dev, "Unable to intialize the regulator\n");
		return -EINVAL;
	}

	dd->regulator = regulator_register(&rdesc, &client->dev,
					dd->init_data, dd, NULL);
	if (IS_ERR(dd->regulator)) 
	{
        if (dd)
        {
            devm_kfree(&client->dev, dd);
            dd = NULL;
        }
		dev_err(&client->dev, "Unable to register regulator rc(%ld)",
						PTR_ERR(dd->regulator));
		return PTR_ERR(dd->regulator);
	}

	dcdc_type = rdesc.name;
	return 0;
}

static int __devexit fan53555_regulator_remove(struct i2c_client *client)
{
	struct fan53555_info *dd = i2c_get_clientdata(client);
#if DEBUG_IS_ON
       printk("--fan53555_regulator_remove called.\n");
#endif
	regulator_unregister(dd->regulator);

	return 0;
}

static const struct i2c_device_id fan53555_id[] = {
	{"fan53555", -1},
	{ },
};

static struct i2c_driver fan53555_regulator_driver = {
	.driver = {
		.name = "fan53555-regulator",
	},
	.probe = fan53555_regulator_probe,
	.remove = __devexit_p(fan53555_regulator_remove),
	.id_table = fan53555_id,
};
static int __init fan53555_regulator_init(void)
{
	return i2c_add_driver(&fan53555_regulator_driver);
}
fs_initcall(fan53555_regulator_init);

static void __exit fan53555_regulator_exit(void)
{
	i2c_del_driver(&fan53555_regulator_driver);
}
module_exit(fan53555_regulator_exit);

