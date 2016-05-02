/*
 * Regulators driver for allwinnertech AXP22X
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
#include "axp22.h"
#include "axp22-regu.h"

enum AXP_REGLS {
	VCC_DCDC1,
	VCC_DCDC2,
	VCC_DCDC3,
	VCC_DCDC4,
	VCC_DCDC5,
	VCC_LDO1,
	VCC_LDO2,
	VCC_LDO3,
	VCC_LDO4,
	VCC_LDO5,
	VCC_LDO6,
	VCC_LDO7,
	VCC_LDO8,
	VCC_LDO9,
	VCC_LDO10,
	VCC_LDO11,
	VCC_LDOIO0,
	VCC_LDOIO1,
	VCC_DC1SW,
	VCC_LDO12,
	VCC_22X_MAX,
};

struct axp22_regulators {
	struct regulator_dev *regulators[VCC_22X_MAX];
	struct axp_dev *chip;
};

#define AXP22_LDO(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit) \
	AXP_LDO(AXP22, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit)

#define AXP22_DCDC(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, mode_bit, freq_addr, dvm_ereg, dvm_ebit) \
	AXP_DCDC(AXP22, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, mode_bit, freq_addr, dvm_ereg, dvm_ebit)

#define AXP22_SW(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit) \
	AXP_SW(AXP22, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit)

static struct axp_regulator_info axp22_regulator_info[] = {
	AXP22_DCDC(1,  1600, 3400, 100,  DCDC1, 0, 5,  DCDC1EN, 0x02, 0x02,
			0x00,     0,    0,   0,   0x80,  0x0, 0x37, 0, 0),
	AXP22_DCDC(2,   600, 1540,  20,  DCDC2, 0, 6,  DCDC2EN, 0x04, 0x04,
			0x00,     0,    0,   0,   0x80,  0x2, 0x37, 0, 0),
	AXP22_DCDC(3,   600, 1860,  20,  DCDC3, 0, 6,  DCDC3EN, 0x08, 0x08,
			0x00,     0,    0,   0,   0x80,  0x4, 0x37, 0, 0),
	AXP22_DCDC(4,   600, 1540,  20,  DCDC4, 0, 6,  DCDC4EN, 0x10, 0x10,
			0x00,     0,    0,   0,   0x80,  0x8, 0x37, 0, 0),
	AXP22_DCDC(5,  1000, 2550,  50,  DCDC5, 0, 5,  DCDC5EN, 0x20, 0x20,
			0x00,     0,    0,   0,   0x80, 0x10, 0x37, 0, 0),
	AXP22_LDO(1,   3000, 3000,   0,   LDO1, 0, 0,   LDO1EN, 0x01, 0x01,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(2,    700, 3300, 100,   LDO2, 0, 5,   LDO2EN, 0x40, 0x40,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(3,    700, 3300, 100,   LDO3, 0, 5,   LDO3EN, 0x80, 0x80,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(4,    700, 3300, 100,   LDO4, 0, 5,   LDO4EN, 0x80, 0x80,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(5,    700, 3300, 100,   LDO5, 0, 5,   LDO5EN, 0x08, 0x08,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(6,    700, 3300, 100,   LDO6, 0, 5,   LDO6EN, 0x10, 0x10,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(7,    700, 3300, 100,   LDO7, 0, 5,   LDO7EN, 0x20, 0x20,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(8,    700, 3300, 100,   LDO8, 0, 5,   LDO8EN, 0x40, 0x40,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(9,    700, 3300, 100,   LDO9, 0, 5,   LDO9EN, 0x01, 0x01,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(10,   700, 3300, 100,  LDO10, 0, 5,  LDO10EN, 0x02, 0x02,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(11,   700, 3300, 100,  LDO11, 0, 5,  LDO11EN, 0x04, 0x04,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(IO0,  700, 3300, 100, LDOIO0, 0, 5, LDOIO0EN, 0x07, 0x03,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(IO1,  700, 3300, 100, LDOIO1, 0, 5, LDOIO1EN, 0x07, 0x03,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_SW(1,    3300, 3300, 100,  DC1SW, 0, 0,  DC1SWEN, 0x80, 0x80,
			0x00,     0,    0,   0,      0, 0, 0, 0),
	AXP22_LDO(12,   700, 1400, 100,  LDO12, 0, 3,  LDO12EN, 0x01, 0x01,
			0x00,     0,    0,   0,      0, 0, 0, 0),
};

static struct regulator_init_data axp_regl_init_data[] = {
	[VCC_DCDC1] = {
		.constraints = {
			.name = "axp22_dcdc1",
			.min_uV = 1600000,
			.max_uV = 3400000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC2] = {
		.constraints = {
			.name = "axp22_dcdc2",
			.min_uV = 600000,
			.max_uV = 1540000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC3] = {
		.constraints = {
			.name = "axp22_dcdc3",
			.min_uV = 600000,
			.max_uV = 1860000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC4] = {
		.constraints = {
			.name = "axp22_dcdc4",
			.min_uV = 600000,
			.max_uV = 1540000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC5] = {
		.constraints = {
			.name = "axp22_dcdc5",
			.min_uV = 1000000,
			.max_uV = 2550000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO1] = {
		.constraints = {
			.name = "axp22_rtc",
			.min_uV = 3000000,
			.max_uV = 3000000,
		},
	},
	[VCC_LDO2] = {
		.constraints = {
			.name = "axp22_aldo1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO3] = {
		.constraints = {
			.name = "axp22_aldo2",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO4] = {
		.constraints = {
			.name = "axp22_aldo3",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO5] = {
		.constraints = {
			.name = "axp22_dldo1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO6] = {
		.constraints = {
			.name = "axp22_dldo2",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO7] = {
		.constraints = {
			.name = "axp22_dldo3",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO8] = {
		.constraints = {
			.name = "axp22_dldo4",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO9] = {
		.constraints = {
			.name = "axp22_eldo1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO10] = {
		.constraints = {
			.name = "axp22_eldo2",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO11] = {
		.constraints = {
			.name = "axp22_eldo3",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDOIO0] = {
		.constraints = {
			.name = "axp22_ldoio0",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDOIO1] = {
		.constraints = {
			.name = "axp22_ldoio1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DC1SW] = {
		.constraints = {
			.name = "axp22_dc1sw",
			.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO12] = {
		.constraints = {
			.name = "axp22_dc5ldo",
			.min_uV = 700000,
			.max_uV = 1400000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
	},
};

static ssize_t workmode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	uint8_t val;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	ret = axp_regmap_read(regmap, info->mode_reg, &val);
	if (ret)
		return sprintf(buf, "IO ERROR\n");

	if ((val & info->mode_mask) == info->mode_mask)
		return sprintf(buf, "PWM\n");
	else
		return sprintf(buf, "AUTO\n");
}

static ssize_t workmode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	uint8_t val;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;
	unsigned int mode;
	int ret;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	ret = sscanf(buf, "%u", &mode);
	if (ret != 1)
		return -EINVAL;

	val = !!mode;
	if (val)
		axp_regmap_set_bits(regmap, info->mode_reg, info->mode_mask);
	else
		axp_regmap_clr_bits(regmap, info->mode_reg, info->mode_mask);

	return count;
}

static ssize_t frequency_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	uint8_t val;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	ret = axp_regmap_read(regmap, info->freq_reg, &val);
	if (ret)
		return ret;

	ret = val & 0x0F;

	return sprintf(buf, "%d\n", (ret * 5 + 50));
}

static ssize_t frequency_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	uint8_t val, tmp;
	int var, err;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	err = kstrtoint(buf, 10, &var);
	if (err)
		return err;

	if (var < 50)
		var = 50;

	if (var > 100)
		var = 100;

	val = (var - 50) / 5;
	val &= 0x0F;

	axp_regmap_read(regmap, info->freq_reg, &tmp);
	tmp &= 0xF0;
	val |= tmp;
	axp_regmap_write(regmap, info->freq_reg, val);

	return count;
}

static struct device_attribute axp_regu_attrs[] = {
	AXP_REGU_ATTR(workmode),
	AXP_REGU_ATTR(frequency),
};

static int axp_regu_create_attrs(struct device *dev)
{
	int j, ret;

	for (j = 0; j < ARRAY_SIZE(axp_regu_attrs); j++) {
		ret = device_create_file(dev, &axp_regu_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}

	return 0;

sysfs_failed:
	while (j--)
		device_remove_file(dev, &axp_regu_attrs[j]);
	return ret;
}

static int axp22_regulator_probe(struct platform_device *pdev)
{
	s32 i, ret = 0;
	struct axp_regulator_info *info;
	struct axp22_regulators *regu_data;
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

	for (i = 0; i < VCC_22X_MAX; i++) {
		info = &axp22_regulator_info[i];
		regu_data->regulators[i] = axp_regulator_register(&pdev->dev, axp_dev->regmap, &axp_regl_init_data[i], info);

		if (IS_ERR(regu_data->regulators[i])) {
			dev_err(&pdev->dev, "failed to register regulator %s\n",info->desc.name);
			while (--i >= 0)
				axp_regulator_unregister(regu_data->regulators[i]);

			return -1;
		}

		if (info->desc.id >= AXP_DCDC_ID_START) {
			ret = axp_regu_create_attrs(
						&regu_data->regulators[i]->dev);
			if (ret)
				dev_err(&pdev->dev,
					"failed to register regulator attr %s\n",
					info->desc.name);
		}
	}

	return 0;
}

static int axp22_regulator_remove(struct platform_device *pdev)
{
	struct axp22_regulators *regu_data = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < VCC_22X_MAX; i++)
		regulator_unregister(regu_data->regulators[i]);

	return 0;
}
static const struct of_device_id axp22_regu_dt_ids[] = {
	{ .compatible = "allwinner,axp22-regulator", },
	{},
};
MODULE_DEVICE_TABLE(of, axp22_regu_dt_ids);

static struct platform_driver axp22_regulator_driver = {
	.driver     = {
		.name   = "axp22-regulator",
		.of_match_table = axp22_regu_dt_ids,
	},
	.probe      = axp22_regulator_probe,
	.remove     = axp22_regulator_remove,
};

static int __init axp22_regulator_initcall(void)
{
	int ret;

	ret = platform_driver_register(&axp22_regulator_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
fs_initcall(axp22_regulator_initcall);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_DESCRIPTION("Regulator Driver for axp22x PMIC");
MODULE_ALIAS("platform:axp22-regulator");
