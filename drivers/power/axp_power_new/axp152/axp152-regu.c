/*
 * Regulators driver for allwinnertech AXP152
 *
 * Copyright (C) 2014 allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/module.h>
#include "../axp-core.h"
#include "../axp-regulator.h"
#include "axp152.h"
#include "axp152-regu.h"

/* Reverse engineered partly from Platformx drivers */
enum axp_regls{
	vcc_dcdc1,
	vcc_dcdc2,
	vcc_dcdc3,
	vcc_dcdc4,

	vcc_ldo1,
	vcc_ldo2,
	vcc_ldo3,
	vcc_ldo4,
	vcc_ldo5,
	vcc_ldo6,
	vcc_ldo7,

	vcc_15x_max,
};

struct axp15_regulators {
	struct regulator_dev *regulators[vcc_15x_max];
	struct axp_dev *chip;
};

static const int axp15_ldo0_table[] = { 5000, 3300, 2800, 2500};
static const int axp15_dcdc1_table[] = { 1700, 1800, 1900, 2000, 2100, 2400, 2500, 2600,\
	2700, 2800, 3000, 3100, 3200, 3300, 3400, 3500};
static const int axp15_aldo12_table[] = { 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900,\
	2000, 2500, 2700, 2800, 3000, 3100, 3200, 3300};

#define AXP15_LDO(_id, min, max, step1, vreg, shift, nbits, ereg, emask, enval, disval, switch_vol, step2, new_level, mode_addr, freq_addr, dvm_ereg, dvm_ebit)	\
	AXP_LDO(AXP152, _id, min, max, step1, vreg, shift, nbits, ereg, emask, enval, disval, switch_vol, step2, new_level, mode_addr, freq_addr, dvm_ereg, dvm_ebit)

#define AXP15_LDO_SEL(_id, min, max, vreg, shift, nbits, ereg, emask, enval, disval, vtable, mode_addr, freq_addr, dvm_ereg, dvm_ebit)	\
	AXP_LDO_SEL(AXP152, _id, min, max, vreg, shift, nbits, ereg, emask, enval, disval, vtable, mode_addr, freq_addr, dvm_ereg, dvm_ebit)

#define AXP15_DCDC(_id, min, max, step1, vreg, shift, nbits, ereg, emask, enval, disval, switch_vol, step2, new_level, mode_addr, freq_addr, dvm_ereg, dvm_ebit)	\
	AXP_DCDC(AXP152, _id, min, max, step1, vreg, shift, nbits, ereg, emask, enval, disval, switch_vol, step2, new_level, mode_addr, freq_addr, dvm_ereg, dvm_ebit)

#define AXP15_DCDC_SEL(_id, min, max, vreg, shift, nbits, ereg, emask, enval, disval, vtable, mode_addr, freq_addr, dvm_ereg, dvm_ebit)	\
	AXP_DCDC_SEL(AXP152, _id, min, max, vreg, shift, nbits, ereg, emask, enval, disval, vtable, mode_addr, freq_addr, dvm_ereg, dvm_ebit)

static struct axp_regulator_info axp15_regulator_info[] = {
	AXP15_DCDC_SEL( 1,   1700,   3500,          DCDC1,      0,    4,   DCDC1EN,     1<<7, 1<<7, 0,  axp15_dcdc1,  0, 0, 0, 0),//buck1
	AXP15_DCDC(     2,   700,    2275,   25,    DCDC2,      0,    6,   DCDC2EN,     1<<6, 1<<6, 0,                     0, 0, 0, 0, 0, 0, 0),//buck2
	AXP15_DCDC(     3,   700,    3500,   50,    DCDC3,      0,    6,   DCDC3EN,     1<<5, 1<<5, 0,                     0, 0, 0, 0, 0, 0, 0),//buck3
	AXP15_DCDC(     4,   700,    3500,   25,    DCDC4,      0,    7,   DCDC4EN,     1<<4, 1<<4, 0,                     0, 0, 0, 0, 0, 0, 0),//buck4
	AXP15_LDO_SEL(  0,   2500,   5000,          LDO0,       4,    2,   LDO0EN,      1<<7, 1<<7, 0,  axp15_ldo0,   0, 0, 0, 0),//ldo0
	AXP15_LDO(      1,   1200,   3100,   100,   RTC,        0,    0,   RTCLDOEN,    1<<0, 1<<0, 0,                     0, 0, 0, 0, 0, 0, 0),//ldo1 for rtc
	AXP15_LDO_SEL(  2,   1200,   3300,          ANALOG1,    4,    4,   ANALOG1EN,   1<<3, 1<<3, 0,  axp15_aldo12, 0, 0, 0, 0),//ldo2 for analog1
	AXP15_LDO_SEL(  3,   1200,   3300,          ANALOG2,    0,    4,   ANALOG2EN,   1<<2, 1<<2, 0,  axp15_aldo12, 0, 0, 0, 0),//ldo3 for analog2
	AXP15_LDO(      4,   700,    3500,   100,   DIGITAL1,   0,    5,   DIGITAL1EN,  1<<1, 1<<1, 0,                     0, 0, 0, 0, 0, 0, 0),//ldo2 for DigtalLDO1
	AXP15_LDO(      5,   700,    3500,   100,   DIGITAL2,	0,    5,   DIGITAL2EN,  1<<0, 1<<0, 0,                     0, 0, 0, 0, 0, 0, 0),//ldo3 for DigtalLDO2
	AXP15_LDO(      IO0, 1800,   3300,   100,   LDOIO0,     0,    4,   LDOI0EN,     0x07, 0x02, 0x07,                  0, 0, 0, 0, 0, 0, 0),//ldoio0
};

static struct regulator_init_data axp_regl_init_data[] = {
	[vcc_dcdc1] = {
		.constraints = { /* default 3.3V */
			.name = "axp15_dcdc1",
			.min_uV = 1700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE ,
		},
	},
	[vcc_dcdc2] = {
		.constraints = { /* default 1.24V */
			.name = "axp15_dcdc2",
			.min_uV = 700000,
			.max_uV = 2275000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[vcc_dcdc3] = {
		.constraints = { /* default 2.5V */
			.name = "axp15_dcdc3",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[vcc_dcdc4] = {
		.constraints = { /* default 2.5V */
			.name = "axp15_dcdc4",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[vcc_ldo1] = {
		.constraints = { /* board default 1.25V */
			.name = "axp15_ldo0",
			.min_uV =  2500000,
			.max_uV =  5000000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[vcc_ldo2] = {
		.constraints = { /* board default 3.0V */
			.name = "axp15_rtcldo",
			.min_uV = 1300000,
			.max_uV = 1800000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[vcc_ldo3] = {
		.constraints = {/* default is 1.8V */
			.name = "axp15_aldo1",
			.min_uV = 1200000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},

	},
	[vcc_ldo4] = {
		.constraints = {
			/* board default is 3.3V */
			.name = "axp15_aldo2",
			.min_uV = 1200000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[vcc_ldo5] = {
		.constraints = { /* default 3.3V */
			.name = "axp15_dldo1",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[vcc_ldo6] = {
		.constraints = { /* default 3.3V */
			.name = "axp15_dldo2",
			.min_uV = 700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[vcc_ldo7] = {
		.constraints = { /* default 3.3V */
			.name = "axp15_gpioldo",
			.min_uV = 1800000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
};

static ssize_t workmode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	uint8_t val;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;
	struct regulator_dev *rdev = container_of(dev,
					struct regulator_dev, dev);;
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;
	
	ret = axp_regmap_read(regmap, AXP152_BUCKMODE, &val);
	if (ret)
		return sprintf(buf, "IO ERROR\n");

	if(info->desc.id == AXP152_ID_DCDC1){
		switch (val & 0x08) {
			case 0:return sprintf(buf, "AUTO\n");
			case 8:return sprintf(buf, "PWM\n");
			default:return sprintf(buf, "UNKNOWN\n");
		}
	}
	if(info->desc.id == AXP152_ID_DCDC2){
		switch (val & 0x04) {
			case 0:return sprintf(buf, "AUTO\n");
			case 4:return sprintf(buf, "PWM\n");
			default:return sprintf(buf, "UNKNOWN\n");
		}
	}
	if(info->desc.id == AXP152_ID_DCDC3){
		switch (val & 0x02) {
			case 0:return sprintf(buf, "AUTO\n");
			case 2:return sprintf(buf, "PWM\n");
			default:return sprintf(buf, "UNKNOWN\n");
		}
	}
	else if(info->desc.id == AXP152_ID_DCDC4){
		switch (val & 0x01) {
			case 0:return sprintf(buf, "AUTO\n");
			case 1:return sprintf(buf, "PWM\n");
			default:return sprintf(buf, "UNKNOWN\n");
		}
	}
	else
		return sprintf(buf, "IO ID ERROR\n");
}

static ssize_t workmode_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	char mode;
	uint8_t val;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;
	struct regulator_dev *rdev = container_of(dev,
					struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;
	
	if(  buf[0] > '0' && buf[0] < '9' )// 1/AUTO: auto mode; 2/PWM: pwm mode;
		mode = buf[0];
	else
		mode = buf[1];

	switch(mode){
	 case 'U':
	 case 'u':
	 case '1':
		val = 0;break;
	 case 'W':
	 case 'w':
	 case '2':
	 	val = 1;break;
	 default:
	    val =0;
	}

	if(info->desc.id == AXP152_ID_DCDC1){
		if(val)
			axp_regmap_set_bits(regmap, AXP152_BUCKMODE,0x08);
		else
			axp_regmap_clr_bits(regmap, AXP152_BUCKMODE,0x08);
	}
	if(info->desc.id == AXP152_ID_DCDC2){
		if(val)
			axp_regmap_set_bits(regmap, AXP152_BUCKMODE,0x04);
		else
			axp_regmap_clr_bits(regmap, AXP152_BUCKMODE,0x04);
	}
	if(info->desc.id == AXP152_ID_DCDC3){
		if(val)
			axp_regmap_set_bits(regmap, AXP152_BUCKMODE,0x02);
		else
			axp_regmap_clr_bits(regmap, AXP152_BUCKMODE,0x02);
	}
	else if(info->desc.id == AXP152_ID_DCDC4){
		if(val)
			axp_regmap_set_bits(regmap, AXP152_BUCKMODE,0x01);
		else
			axp_regmap_clr_bits(regmap, AXP152_BUCKMODE,0x01);
	}

	return count;
}

static ssize_t frequency_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	uint8_t val;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;
	struct regulator_dev *rdev = container_of(dev,
					struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;
	
	ret = axp_regmap_read(regmap, AXP152_BUCKFREQ, &val);
	if (ret)
		return ret;
	ret = val & 0x0F;
	return sprintf(buf, "%d\n",(ret*5 + 50));
}

static ssize_t frequency_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	uint8_t val,tmp;
	int var;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;
	struct regulator_dev *rdev = container_of(dev,
					struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;
	var = simple_strtoul(buf, NULL, 10);
	if(var < 50)
		var = 50;
	if(var > 100)
		var = 100;

	val = (var -50)/5;
	val &= 0x0F;

	axp_regmap_read(regmap, AXP152_BUCKFREQ, &tmp);
	tmp &= 0xF0;
	val |= tmp;
	axp_regmap_write(regmap, AXP152_BUCKFREQ, val);
	return count;
}


static struct device_attribute axp_regu_attrs[] = {
	AXP_REGU_ATTR(workmode),
	AXP_REGU_ATTR(frequency),
};

int axp_regu_create_attrs(struct device *dev)
{
	int j,ret;
	for (j = 0; j < ARRAY_SIZE(axp_regu_attrs); j++) {
		ret = device_create_file(dev,&axp_regu_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}
	goto succeed;

sysfs_failed:
	while (j--)
		device_remove_file(dev,&axp_regu_attrs[j]);
succeed:
	return ret;
}

static int axp15_regulator_probe(struct platform_device *pdev)
{
	s32 i, ret = 0;
	struct axp_regulator_info *info;
	struct axp15_regulators *regu_data;
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	ret = axp_regu_device_tree_parse(axp_regl_init_data);
	if (ret) {
		pr_err("%s parse sysconfig or device tree err\n", __func__);
		return -EINVAL;
	}

	regu_data = devm_kzalloc(&pdev->dev, sizeof(*regu_data),
					GFP_KERNEL);
	if (!regu_data)
		return -ENOMEM;
	
	regu_data->chip = axp_dev;
	platform_set_drvdata(pdev, regu_data);
	
	for (i = 0; i < vcc_15x_max; i++){
		info = &axp15_regulator_info[i];
		if (info->desc.id == AXP152_ID_LDO4 || info->desc.id == AXP152_ID_LDO5 \
					||info->desc.id == AXP152_ID_DCDC2 ||info->desc.id == AXP152_ID_DCDC3 \
					||info->desc.id == AXP152_ID_DCDC4 ||info->desc.id == AXP152_ID_LDO1 \
					||info->desc.id == AXP152_ID_LDOIO0){
			regu_data->regulators[i] = axp_regulator_register(&pdev->dev, axp_dev->regmap,
								&axp_regl_init_data[i], info);
		}else if(info->desc.id == AXP152_ID_DCDC1 || info->desc.id == AXP152_ID_LDO0||
				info->desc.id == AXP152_ID_LDO3 || info->desc.id == AXP152_ID_LDO2){
			regu_data->regulators[i] = axp_regulator_sel_register(&pdev->dev, axp_dev->regmap,
								&axp_regl_init_data[i], info);
 		}
		
		if (IS_ERR(regu_data->regulators[i])) {
			dev_err(&pdev->dev, "failed to register regulator %s\n",info->desc.name);
			while (--i >= 0)
				axp_regulator_unregister(regu_data->regulators[i]);
			
			return -1;
		}

		if(info->desc.id == AXP152_ID_DCDC1 ||info->desc.id == AXP152_ID_DCDC2 \
				||info->desc.id == AXP152_ID_DCDC3 ||info->desc.id == AXP152_ID_DCDC4){
			ret = axp_regu_create_attrs(&regu_data->regulators[i]->dev);
			if(ret){
				dev_err(&pdev->dev, "failed to register regulator attr %s\n",info->desc.name);
			}
		}
	}

	return 0;
}

static int axp15_regulator_remove(struct platform_device *pdev)
{
	int i;
	struct axp15_regulators *regu_data=platform_get_drvdata(pdev);
	

	for (i = 0; i < vcc_15x_max; i++)
		regulator_unregister(regu_data->regulators[i]);

	return 0;
}

static const struct of_device_id axp15_regu_dt_ids[] = {
	{ .compatible = "allwinner,axp15-regulator", },
	{},
};
MODULE_DEVICE_TABLE(of, axp15_regu_dt_ids);

static struct platform_driver axp152_regulator_driver = {
	.driver		= {
		.name	= "axp15-regulator",
		.of_match_table= axp15_regu_dt_ids,
	},
	.probe		= axp15_regulator_probe,
	.remove		= axp15_regulator_remove,
};

module_platform_driver(axp152_regulator_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_DESCRIPTION("Regulator Driver for axp15x PMIC");
MODULE_ALIAS("platform:axp15-regulator");
