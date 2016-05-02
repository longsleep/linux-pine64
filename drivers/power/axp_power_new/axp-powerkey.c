#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/input.h>

#include "axp-core.h"

struct axp_onkey_info {
	struct input_dev *idev;
	struct axp_dev *chip;
};

struct axp_onkey_interrupts {
	char *name;
	irqreturn_t (*isr)(int irq, void *data);
};

static irqreturn_t axp_key_up_handler(int irq, void *data)
{
	struct axp_onkey_info *info = (struct axp_onkey_info *)data;
	input_report_key(info->idev, KEY_POWER, 0);
	input_sync(info->idev);
	return IRQ_HANDLED;
}

static irqreturn_t axp_key_down_handler(int irq, void *data)
{
	struct axp_onkey_info *info = (struct axp_onkey_info *)data;
	input_report_key(info->idev, KEY_POWER, 1);
	input_sync(info->idev);
	return IRQ_HANDLED;
}

static irqreturn_t axp_presslong_handler(int irq, void *data)
{
	struct axp_onkey_info *info = (struct axp_onkey_info *)data;
	input_report_key(info->idev, KEY_POWER, 1);
	input_sync(info->idev);
	ssleep(2);
	input_report_key(info->idev, KEY_POWER, 0);
	input_sync(info->idev);
	return IRQ_HANDLED;
}

static irqreturn_t axp_pressshort_handler(int irq, void *data)
{
	struct axp_onkey_info *info = (struct axp_onkey_info *)data;
	input_report_key(info->idev, KEY_POWER, 1);
	input_sync(info->idev);
	msleep(100);
	input_report_key(info->idev, KEY_POWER, 0);
	input_sync(info->idev);
	return IRQ_HANDLED;
}

static struct axp_onkey_interrupts axp_onkey_irq[] = {
	{"PEK_DBR", axp_key_up_handler},
	{"PEK_DBF", axp_key_down_handler},
	{"PEK_LONG", axp_presslong_handler},
	{"PEK_SHORT", axp_pressshort_handler},
};

static int axp_onkey_probe(struct platform_device *pdev)
{
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct axp_onkey_info *info;
	struct input_dev *powerkey_dev;
	int err = 0, i, irq;

	info = kzalloc(sizeof(struct axp_onkey_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->chip = axp_dev;

	/* register input device */
	powerkey_dev = input_allocate_device();
	if (!powerkey_dev) {
		pr_err("alloc powerkey input device error\n");
		goto out;
	}

	powerkey_dev->name = "axp_powerkey";
	powerkey_dev->phys = "m1kbd/input2";
	powerkey_dev->id.bustype = BUS_HOST;
	powerkey_dev->id.vendor = 0x0001;
	powerkey_dev->id.product = 0x0001;
	powerkey_dev->id.version = 0x0100;
	powerkey_dev->open = NULL;
	powerkey_dev->close = NULL;
	powerkey_dev->dev.parent = &pdev->dev;
	set_bit(EV_KEY, powerkey_dev->evbit);
	set_bit(EV_REL, powerkey_dev->evbit);
	set_bit(KEY_POWER, powerkey_dev->keybit);

	err= input_register_device(powerkey_dev);
	if (err) {
		printk("Unable to Register the axp_powerkey\n");
		goto out_reg;
	}

	info->idev = powerkey_dev;

	for (i = 0; i < ARRAY_SIZE(axp_onkey_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_onkey_irq[i].name);
		if (irq < 0)
			continue;

		err = axp_request_irq(axp_dev, irq, axp_onkey_irq[i].isr, info);
		if (err != 0) {
			dev_err(&pdev->dev, "failed to request %s IRQ %d: %d\n"
					, axp_onkey_irq[i].name, irq, err);
			goto out_irq;
		}

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
				axp_onkey_irq[i].name, irq, err);
	}

	platform_set_drvdata(pdev, info);

	return 0;

out_irq:
	for (i = i - 1; i >= 0; i--) {
		irq = platform_get_irq_byname(pdev, axp_onkey_irq[i].name);
		if (irq < 0)
			continue;
		axp_free_irq(axp_dev,irq);
	}
out_reg:
	input_free_device(powerkey_dev);
out:
	kfree(info);
	return err;
}

static int axp_onkey_remove(struct platform_device *pdev)
{
	int i, irq;
	struct axp_onkey_info *info = platform_get_drvdata(pdev);
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	for (i = 0; i < ARRAY_SIZE(axp_onkey_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_onkey_irq[i].name);
		if (irq < 0)
			continue;
		axp_free_irq(axp_dev, irq);
	}

	input_unregister_device(info->idev);
	kfree(info);

	return 0;
}

static const struct of_device_id axp_powkey_dt_ids[] = {
	{ .compatible = "allwinner,axp-powerkey", },
	{},
};
MODULE_DEVICE_TABLE(of, axp_powkey_dt_ids);

static struct platform_driver axp_onkey_driver = {
	.driver = {
		.name = "axp-powerkey",
		.of_match_table = axp_powkey_dt_ids,
	},
	.probe  = axp_onkey_probe,
	.remove = axp_onkey_remove,
};

module_platform_driver(axp_onkey_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("onkey Driver for axp PMIC");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
