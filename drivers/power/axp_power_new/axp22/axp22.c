#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/reboot.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mfd/core.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include "../axp-core.h"
#include "axp22.h"

static struct axp_dev *axp22_pm_power;
static int power_start = 0;

static struct axp_regmap_irq_chip axp22_regmap_irq_chip = {
	.name           = "axp22_irq_chip",
	.status_base    = AXP22_INTSTS1,
	.enable_base      = AXP22_INTEN1,
	.num_regs       = 5,
};

static struct resource axp22_pek_resources[] = {
	{AXP22_IRQ_PEKRE,  AXP22_IRQ_PEKRE,  "PEK_DBR",      IORESOURCE_IRQ,},
	{AXP22_IRQ_PEKFE,  AXP22_IRQ_PEKFE,  "PEK_DBF",      IORESOURCE_IRQ,},
};

static struct resource axp22_charger_resources[] = {
	{AXP22_IRQ_USBIN,  AXP22_IRQ_USBIN,  "usb in",       IORESOURCE_IRQ,},
	{AXP22_IRQ_USBRE,  AXP22_IRQ_USBRE,  "usb out",      IORESOURCE_IRQ,},
	{AXP22_IRQ_ACIN,   AXP22_IRQ_ACIN,   "ac in",        IORESOURCE_IRQ,},
	{AXP22_IRQ_ACRE,   AXP22_IRQ_ACRE,   "ac out",       IORESOURCE_IRQ,},
	{AXP22_IRQ_BATIN,  AXP22_IRQ_BATIN,  "bat in",       IORESOURCE_IRQ,},
	{AXP22_IRQ_BATRE,  AXP22_IRQ_BATRE,  "bat out",      IORESOURCE_IRQ,},
	{AXP22_IRQ_TEMLO,  AXP22_IRQ_TEMLO,  "bat temp low", IORESOURCE_IRQ,},
	{AXP22_IRQ_TEMOV,  AXP22_IRQ_TEMOV,  "bat temp over",IORESOURCE_IRQ,},
	{AXP22_IRQ_CHAST,  AXP22_IRQ_CHAST,  "charging",     IORESOURCE_IRQ,},
	{AXP22_IRQ_CHAOV,  AXP22_IRQ_CHAOV,  "charge over",  IORESOURCE_IRQ,},
};

static struct mfd_cell axp22_cells[] = {
	{
		.name          = "axp-powerkey",
		.num_resources = ARRAY_SIZE(axp22_pek_resources),
		.resources     = axp22_pek_resources,
		.of_compatible = "allwinner,axp-powerkey",
	}, {
		.name          = "axp22-regulator",
		.of_compatible = "allwinner,axp22-regulator",
	},
	{
		.name          = "axp22-charger",
		.num_resources = ARRAY_SIZE(axp22_charger_resources),
		.resources     = axp22_charger_resources,
		.of_compatible = "allwinner,axp22-charger",
	},
	{
		.name          = "axp22-gpio",
		.of_compatible = "allwinner,axp22-gpio",
	},
};

void axp22_power_off(void)
{
	uint8_t val;

	printk("[axp] send power-off command!\n");
	mdelay(20);

	if (power_start != 1) {
		axp_regmap_read(axp22_pm_power->regmap, AXP22_STATUS, &val);
		if (val & 0xF0) {
			axp_regmap_read(axp22_pm_power->regmap,
					AXP22_MODE_CHGSTATUS, &val);
			if (val & 0x20) {
				pr_info("[axp] set flag!\n");
				axp_regmap_read(axp22_pm_power->regmap,
					AXP22_BUFFERC, &val);
				if (0x0d != val)
					axp_regmap_write(axp22_pm_power->regmap,
						AXP22_BUFFERC, 0x0f);

				mdelay(20);
				pr_info("[axp] reboot!\n");
				machine_restart(NULL);
				pr_warn("[axp] warning!!! arch can't reboot,\"\
					\" maybe some error happend!\n");
			}
		}
	}

	axp_regmap_read(axp22_pm_power->regmap, AXP22_BUFFERC, &val);
	if (0x0d != val)
		axp_regmap_write(axp22_pm_power->regmap, AXP22_BUFFERC, 0x00);

	mdelay(20);
	axp_regmap_set_bits(axp22_pm_power->regmap, AXP22_OFF_CTL, 0x80);
	mdelay(20);
	pr_warn("[axp] warning!!! axp can't power-off,\"\
		\" maybe some error happend!\n");
}

static int axp22_init_chip(struct axp_dev *axp22)
{
	uint8_t chip_id;
	int err;

	err = axp_regmap_read(axp22->regmap, AXP22_IC_TYPE, &chip_id);
	if (err) {
		pr_err("[AXP22-MFD] try to read chip id failed!\n");
		return err;
	}

	if (((chip_id & 0xc0) == 0x00) &&
		(((chip_id & 0x0f) == 0x06)
			|| ((chip_id & 0x0f) == 0x07)
			|| ((chip_id & 0xff) == 0x42))
		)
		pr_info("[AXP22] chip id detect 0x%x !\n", chip_id);
	else
		pr_info("[AXP22] chip id not detect 0x%x !\n", chip_id);

	return 0;
}

static int axp22_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret;
	struct axp_dev *axp22;
	struct device_node *node = client->dev.of_node;

	if (node) {
		/* get dt and sysconfig */
		if (!of_device_is_available(node)) {
			pr_err("%s: AXP22x driver is disable\n", __func__);
			return -EPERM;
		}
	} else {
		pr_err("AXP22x device tree err!\n");
		return -EBUSY;
	}

	axp22 = devm_kzalloc(&client->dev, sizeof(*axp22), GFP_KERNEL);
	if (!axp22)
		return -ENOMEM;

	axp22->dev = &client->dev;
	axp22->nr_cells = ARRAY_SIZE(axp22_cells);
	axp22->cells = axp22_cells;

	axp22->regmap = axp_regmap_init_i2c(&client->dev);
	if (IS_ERR(axp22->regmap)) {
		ret = PTR_ERR(axp22->regmap);
		dev_err(&client->dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	axp22->irq_data = axp_irq_chip_register(axp22->regmap, client->irq,
						IRQF_SHARED
						| IRQF_DISABLED
						| IRQF_NO_SUSPEND,
						&axp22_regmap_irq_chip);
	if (IS_ERR(axp22->irq_data)) {
		ret = PTR_ERR(axp22->irq_data);
		dev_err(&client->dev, "axp init irq failed: %d\n", ret);
		return ret;
	}

	ret = axp22_init_chip(axp22);
	if (ret)
		goto fail_init;

	ret = axp_mfd_add_devices(axp22);
	if (ret)
		goto fail_init;

	axp22_pm_power = axp22;

	if (!pm_power_off)
		pm_power_off = axp22_power_off;

	return 0;

fail_init:
	dev_err(axp22->dev, "failed to add MFD devices: %d\n", ret);
	axp_irq_chip_unregister(client->irq, axp22->irq_data);
	return ret;
}

static int axp22_remove(struct i2c_client *client)
{
	struct axp_dev *axp22 = i2c_get_clientdata(client);

	if (axp22 == axp22_pm_power) {
		axp22_pm_power = NULL;
		pm_power_off = NULL;
	}

	axp_mfd_remove_devices(axp22);
	axp_irq_chip_unregister(client->irq, axp22->irq_data);

	return 0;
}

static const struct i2c_device_id axp22_id_table[] = {
	{ "axp22", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, axp152_id_table);

static const struct of_device_id axp22_dt_ids[] = {
	{ .compatible = "allwinner,axp22", },
	{},
};
MODULE_DEVICE_TABLE(of, axp22_dt_ids);

static struct i2c_driver axp22_driver = {
	.driver = {
		.name   = "axp22",
		.owner  = THIS_MODULE,
		.of_match_table = axp22_dt_ids,
	},
	.probe      = axp22_probe,
	.remove     = axp22_remove,
	.id_table   = axp22_id_table,
};

static int __init axp22_i2c_init(void)
{
	int ret;
	ret = i2c_add_driver(&axp22_driver);
	if (ret != 0)
		pr_err("Failed to register axp152 I2C driver: %d\n", ret);
	return ret;
}
subsys_initcall(axp22_i2c_init);

static void __exit axp22_i2c_exit(void)
{
	i2c_del_driver(&axp22_driver);
}
module_exit(axp22_i2c_exit);

MODULE_DESCRIPTION("PMIC Driver for AXP22X");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_LICENSE("GPL");
