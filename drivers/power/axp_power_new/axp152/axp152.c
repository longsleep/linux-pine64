#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mfd/core.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include "../axp-core.h"
#include "axp152.h"


static struct axp_dev *axp15_pm_power;

static struct axp_regmap_irq_chip axp15_regmap_irq_chip = {
	.name			= "axp15_irq_chip",
	.status_base		= POWER15_INTSTS1,
	.mask_base		= POWER15_INTEN1,
	.num_regs		= 3,

};

static struct resource axp15_pek_resources[] = {
	{
		.name	= "PEK_DBR",
		.start	= AXP15_IRQ_POK_POS,
		.end	= AXP15_IRQ_POK_POS,
		.flags	= IORESOURCE_IRQ,
	}, {
		.name	= "PEK_DBF",
		.start	= AXP15_IRQ_POK_NEG,
		.end	= AXP15_IRQ_POK_NEG,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mfd_cell axp15_cells[] = {
	{
		.name                   = "axp-powerkey",
		.num_resources          = ARRAY_SIZE(axp15_pek_resources),
		.resources              = axp15_pek_resources,
		.of_compatible          = "allwinner,axp-powerkey",
	}, {
		.name                   = "axp15-regulator",
		.of_compatible          = "allwinner,axp15-regulator",
	},
};

void axp15_power_off(void)
{
	axp_regmap_write(axp15_pm_power->regmap, POWER15_OFF_CTL,
		     0x80);
}

static int axp15_init_chip(struct axp_dev *axp15)
{
	int err;
	u8 chip_id;
	char val[] = {0xff,0xff,0xff};
	err = axp_regmap_read(axp15->regmap, POWER15_IC_TYPE, &chip_id);
	if (err) {
		printk("[AXP15-MFD] try to read chip id failed!\n");
		return err;
	}else{
		dev_info(axp15->dev, "AXP (CHIP ID: 0x%02x) detected\n", chip_id);
	}
	axp_regmap_writes(axp15->regmap, POWER15_INTEN1, 3,val);
	/* enable dcdc2 dvm */
	err =  axp_regmap_update(axp15->regmap, 0x25,1<<2, 1<<2);
	if(err){
		printk(KERN_ERR "[AXP15-MFD] try to read reg[25H] failed!\n");
		return err;
	}else{
		printk("[AXP15-MFD] enable dcdc2 dvm.\n");
	}
	return 0;
}

static int axp152_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	int ret;
	struct axp_dev *axp15;
	struct device_node *node = client->dev.of_node;

	if (node) {
		/* get dt and sysconfig */
		if (!of_device_is_available(node)) {
			pr_err("%s: AXP152 driver is disable\n", __func__);
			return -EPERM;
		}
	}else{
		pr_err("AXP152 device tree err!\n");
		return -EBUSY;
	}
	axp15 = devm_kzalloc(&client->dev, sizeof(*axp15), GFP_KERNEL);
	if (!axp15)
		return -ENOMEM;

	axp15->dev = &client->dev;
	axp15->nr_cells = ARRAY_SIZE(axp15_cells);
	axp15->cells = axp15_cells;

	axp15->regmap = axp_regmap_init_i2c(&client->dev);
	if (IS_ERR(axp15->regmap)) {
		ret = PTR_ERR(axp15->regmap);
		dev_err(&client->dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	axp15->irq_data = axp_irq_chip_register(axp15->regmap, client->irq,
				  IRQF_SHARED|IRQF_DISABLED |IRQF_NO_SUSPEND,
				  &axp15_regmap_irq_chip);
	if (IS_ERR(axp15->irq_data)) {
		ret = PTR_ERR(axp15->irq_data);
		dev_err(&client->dev, "axp init irq failed: %d\n", ret);
		return ret;
	}

	ret = axp15_init_chip(axp15);
	if(ret)
		goto fail_init;

	ret = axp_mfd_add_devices(axp15);
	if (ret) 
		goto fail_init;

	axp15_pm_power = axp15;

	if (!pm_power_off) {
		pm_power_off = axp15_power_off;
	}

	return 0;
fail_init:
	dev_err(axp15->dev, "failed to add MFD devices: %d\n", ret);
	axp_irq_chip_unregister(client->irq, axp15->irq_data);
	return ret;
}

static int axp152_remove(struct i2c_client *client)
{
	struct axp_dev *axp15 = i2c_get_clientdata(client);

	if (axp15 == axp15_pm_power) {
		axp15_pm_power = NULL;
		pm_power_off = NULL;
	}

	axp_mfd_remove_devices(axp15);
	axp_irq_chip_unregister(client->irq, axp15->irq_data);
	return 0;
}

static const struct i2c_device_id axp152_id_table[] = {
	{ "axp152", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, axp152_id_table);

static const struct of_device_id axp152_dt_ids[] = {
	{ .compatible = "allwinner,axp152", },
	{},
};
MODULE_DEVICE_TABLE(of, axp152_dt_ids);

static struct i2c_driver axp152_driver = {
	.driver	= {
		.name	= "axp152",
		.owner	= THIS_MODULE,
		.of_match_table	= axp152_dt_ids,
	},
	.probe		= axp152_probe,
	.remove		= axp152_remove,
	.id_table	= axp152_id_table,
};

static int __init axp152_i2c_init(void)
{
	int ret;
	ret = i2c_add_driver(&axp152_driver);
	if (ret != 0)
		pr_err("Failed to register axp152 I2C driver: %d\n", ret);
	return ret;
}
subsys_initcall(axp152_i2c_init);

static void __exit axp152_i2c_exit(void)
{
	i2c_del_driver(&axp152_driver);
}
module_exit(axp152_i2c_exit);

MODULE_DESCRIPTION("PMIC Driver for AXP152");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_LICENSE("GPL");

