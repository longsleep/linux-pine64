#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/mfd/core.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include "axp-core.h"

#ifdef CONFIG_AXP_TWI_USED
static s32 __axp_read_i2c(struct i2c_client *client, u32 reg, u8 *val)
{
	s32 ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading at 0x%02x\n", reg);
		return ret;
	}

	*val = (u8)ret;

	return 0;
}

static s32 __axp_reads_i2c(struct i2c_client *client, int reg,int len, u8 *val)
{
	s32 ret;

	ret = i2c_smbus_read_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading from 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

static s32 __axp_write_i2c(struct i2c_client *client, int reg, u8 val)
{
	s32 ret;

	//axp_reg_debug(reg, 1, &val);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writing 0x%02x to 0x%02x\n",
				val, reg);
		return ret;
	}

	return 0;
}

static s32 __axp_writes_i2c(struct i2c_client *client, int reg, int len, u8 *val)
{
	s32 ret;

	//axp_reg_debug(reg, len, val);
	ret = i2c_smbus_write_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writings to 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

static inline s32 __axp_read_rsb(char devaddr, u32 reg, u8 *val, bool syncflag){return 0;}
static inline s32 __axp_reads_rsb(char devaddr, int reg,int len, u8 *val, bool syncflag){return 0;}
static inline s32 __axp_write_rsb(char devaddr, int reg, u8 val, bool syncflag){return 0;}
static inline s32 __axp_writes_rsb(char devaddr, int reg, int len, u8 *val, bool syncflag){return 0;}
#else
static inline s32 __axp_read_i2c(struct i2c_client *client, u32 reg, u8 *val){return 0;}
static inline s32 __axp_reads_i2c(struct i2c_client *client, int reg,int len, u8 *val){return 0;}
static inline s32 __axp_write_i2c(struct i2c_client *client, int reg, u8 val){return 0;}
static inline s32 __axp_writes_i2c(struct i2c_client *client, int reg, int len, u8 *val){return 0;}

static s32 __axp_read_rsb(char devaddr, u32 reg, u8 *val, bool syncflag)
{
	s32 ret;
	u8 addr = (u8)reg;
	u8 data = 0;
	arisc_rsb_block_cfg_t rsb_data;
	u32 data_temp;

	rsb_data.len = 1;
	rsb_data.datatype = RSB_DATA_TYPE_BYTE;

	if (syncflag)
		rsb_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
	else
		rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

	rsb_data.devaddr = devaddr;
	rsb_data.regaddr = &addr;
	rsb_data.data = &data_temp;

	/* write axp registers */
	ret = arisc_rsb_read_block_data(&rsb_data);
	if (ret != 0) {
		printk("failed read to 0x%02x\n", reg);
		return ret;
	}

	data = (u8)data_temp;
	*val = data;

	return 0;
}

static s32 __axp_reads_rsb(char devaddr, int reg, int len, u8 *val, bool syncflag)
{
	s32 ret, i, rd_len;
	u8 addr[AXP_TRANS_BYTE_MAX];
	u8 data[AXP_TRANS_BYTE_MAX];
	u8 *cur_data = val;
	arisc_rsb_block_cfg_t rsb_data;
	u32 data_temp[AXP_TRANS_BYTE_MAX];

	/* fetch first register address */
	while (len > 0) {
		rd_len = min(len, AXP_TRANS_BYTE_MAX);
		for (i = 0; i < rd_len; i++)
			addr[i] = reg++;

		rsb_data.len = rd_len;
		rsb_data.datatype = RSB_DATA_TYPE_BYTE;

		if (syncflag)
			rsb_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
		else
			rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

		rsb_data.devaddr = devaddr;
		rsb_data.regaddr = addr;
		rsb_data.data = data_temp;

		/* read axp registers */
		ret = arisc_rsb_read_block_data(&rsb_data);
		if (ret != 0) {
			printk("failed reads to 0x%02x\n", reg);
			return ret;
		}

		for (i = 0; i < rd_len; i++)
			data[i] = (u8)data_temp[i];

		/* copy data to user buffer */
		memcpy(cur_data, data, rd_len);
		cur_data = cur_data + rd_len;

		/* process next time read */
		len -= rd_len;
	}

	return 0;
}

static s32 __axp_write_rsb(char devaddr, int reg, u8 val, bool syncflag)
{
	s32 ret;
	u8 addr = (u8)reg;
	arisc_rsb_block_cfg_t rsb_data;
	u32 data;

	//axp_reg_debug(reg, 1, &val);

	data = (unsigned int)val;
	rsb_data.len = 1;
	rsb_data.datatype = RSB_DATA_TYPE_BYTE;

	if (syncflag)
		rsb_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
	else
		rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

	rsb_data.devaddr = devaddr;
	rsb_data.regaddr = &addr;
	rsb_data.data = &data;

	/* write axp registers */
	ret = arisc_rsb_write_block_data(&rsb_data);
	if (ret != 0) {
		printk("failed writing to 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

static s32 __axp_writes_rsb(char devaddr, int reg, int len, u8 *val, bool syncflag)
{
	s32 ret = 0, i, first_flag, wr_len;
	u8 addr[AXP_TRANS_BYTE_MAX];
	u8 data[AXP_TRANS_BYTE_MAX];
	arisc_rsb_block_cfg_t rsb_data;
	u32 data_temp[AXP_TRANS_BYTE_MAX];

	//axp_reg_debug(reg, len, val);

	/* fetch first register address */
	first_flag = 1;
	addr[0] = (u8)reg;
	len = len + 1;  /* + first reg addr */
	len = len >> 1; /* len = len / 2 */

	while (len > 0) {
		wr_len = min(len, AXP_TRANS_BYTE_MAX);
		for (i = 0; i < wr_len; i++) {
			if (first_flag) {
				/* skip the first reg addr */
				data[i] = *val++;
				first_flag = 0;
			} else {
				addr[i] = *val++;
				data[i] = *val++;
			}
		}

		for (i = 0; i < wr_len; i++)
			data_temp[i] = (unsigned int)data[i];

		rsb_data.len = wr_len;
		rsb_data.datatype = RSB_DATA_TYPE_BYTE;

		if (syncflag)
			rsb_data.msgattr = ARISC_MESSAGE_ATTR_HARDSYN;
		else
			rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;

		rsb_data.devaddr = devaddr;
		rsb_data.regaddr = addr;
		rsb_data.data = data_temp;

		/* write axp registers */
		ret = arisc_rsb_write_block_data(&rsb_data);
		if (ret != 0) {
			printk("failed writings to 0x%02x\n", reg);
			return ret;
		}

		/* process next time write */
		len -= wr_len;
	}

	return 0;
}
#endif

static s32 _axp_write(struct axp_regmap *map, s32 reg, u8 val, bool sync)
{
	s32 ret = 0;

	if (map->type == AXP_REGMAP_I2C)
		ret = __axp_write_i2c(map->client, reg, val);
	else if(map->type == AXP_REGMAP_RSB)
		ret = __axp_write_rsb(map->rsbaddr, reg, val, sync);

	return ret;
}

static s32 _axp_writes(struct axp_regmap *map, s32 reg, s32 len, u8 *val, bool sync)
{
	s32 ret = 0, i;
	s32 wr_len, rw_reg;
	u8 wr_val[32];

	while (len) {
		wr_len = min(len, 15);
		rw_reg = reg ++;
		wr_val[0] = *val++;

		for (i = 1; i < wr_len; i++) {
			wr_val[i*2-1] = reg++;
			wr_val[i*2] = *val++;
		}

		if (map->type == AXP_REGMAP_I2C)
			ret = __axp_writes_i2c(map->client, rw_reg, 2*wr_len-1, wr_val);
		else if (map->type == AXP_REGMAP_RSB)
			ret = __axp_writes_rsb(map->rsbaddr, rw_reg, 2*wr_len-1, wr_val, sync);

		if (ret)
			return ret;

		len -= wr_len;
	}

	return 0;
}

static s32 _axp_read(struct axp_regmap *map, s32 reg, u8 *val, bool sync)
{
	s32 ret = 0;

	if (map->type == AXP_REGMAP_I2C)
		ret = __axp_read_i2c(map->client, reg, val);
	else if (map->type == AXP_REGMAP_RSB)
		ret = __axp_read_rsb(map->rsbaddr, reg, val, sync);

	return ret;
}

static s32 _axp_reads(struct axp_regmap *map, s32 reg, s32 len, u8 *val, bool sync)
{
	s32 ret = 0;

	if (map->type == AXP_REGMAP_I2C)
		ret = __axp_reads_i2c(map->client, reg, len, val);
	else if(map->type == AXP_REGMAP_RSB)
		ret = __axp_reads_rsb(map->rsbaddr, reg, len, val, sync);

	return ret;
}

s32 axp_regmap_write(struct axp_regmap *map, s32 reg, u8 val)
{
	s32 ret = 0;

	mutex_lock(&map->lock);
	ret = _axp_write(map, reg, val, false);
	mutex_unlock(&map->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_write);

s32 axp_regmap_writes(struct axp_regmap *map, s32 reg, s32 len, u8 *val)
{
	s32 ret = 0;

	mutex_lock(&map->lock);
	ret = _axp_writes(map, reg, len, val, false);
	mutex_unlock(&map->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_writes);

s32 axp_regmap_read(struct axp_regmap *map, s32 reg, u8 *val)
{
	return _axp_read(map, reg, val, false);
}
EXPORT_SYMBOL_GPL(axp_regmap_read);

s32 axp_regmap_reads(struct axp_regmap *map, s32 reg, s32 len, u8 *val)
{
	return _axp_reads(map, reg, len, val, false);
}
EXPORT_SYMBOL_GPL(axp_regmap_reads);

s32 axp_regmap_set_bits(struct axp_regmap *map, s32 reg, u8 bit_mask)
{
	u8 reg_val;
	s32 ret = 0;

	mutex_lock(&map->lock);
	ret = _axp_read(map, reg, &reg_val, false);

	if (ret)
		goto out;

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = _axp_write(map, reg, reg_val, false);
	}

out:
	mutex_unlock(&map->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_set_bits);

s32 axp_regmap_clr_bits(struct axp_regmap *map, s32 reg, u8 bit_mask)
{
	u8 reg_val;
	s32 ret = 0;

	mutex_lock(&map->lock);
	ret = _axp_read(map, reg, &reg_val, false);

	if (ret)
		goto out;

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = _axp_write(map, reg, reg_val, false);
	}

out:
	mutex_unlock(&map->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_clr_bits);

s32 axp_regmap_update(struct axp_regmap *map, s32 reg, u8 val, u8 mask)
{
	u8 reg_val;
	s32 ret = 0;

	mutex_lock(&map->lock);
	ret = _axp_read(map, reg, &reg_val, false);
	if (ret)
		goto out;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = _axp_write(map, reg, reg_val, false);
	}

out:
	mutex_unlock(&map->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_update);


s32 axp_regmap_set_bits_sync(struct axp_regmap *map, s32 reg, u8 bit_mask)
{
	u8 reg_val;
	s32 ret = 0;
	unsigned long irqflags;

	spin_lock_irqsave(&map->spinlock, irqflags);
	ret = _axp_read(map, reg, &reg_val, true);
	if (ret)
		goto out;

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = _axp_write(map, reg, reg_val, true);
	}

out:
	spin_unlock_irqrestore(&map->spinlock, irqflags);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_set_bits_sync);

s32 axp_regmap_clr_bits_sync(struct axp_regmap *map, s32 reg, u8 bit_mask)
{
	u8 reg_val;
	s32 ret = 0;
	unsigned long irqflags;

	spin_lock_irqsave(&map->spinlock, irqflags);
	ret = _axp_read(map, reg, &reg_val, true);
	if (ret)
		goto out;

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = _axp_write(map, reg, reg_val, true);
	}

out:
	spin_unlock_irqrestore(&map->spinlock, irqflags);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_clr_bits_sync);

s32 axp_regmap_update_sync(struct axp_regmap *map, s32 reg, u8 val, u8 mask)
{
	u8 reg_val;
	s32 ret = 0;
	unsigned long irqflags;

	spin_lock_irqsave(&map->spinlock, irqflags);
	ret = _axp_read(map, reg, &reg_val, true);
	if (ret)
		goto out;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = _axp_write(map, reg, reg_val, true);
	}

out:
	spin_unlock_irqrestore(&map->spinlock, irqflags);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_regmap_update_sync);

struct axp_regmap *axp_regmap_init_i2c(struct device *dev)
{
	struct axp_regmap *map = NULL;

	map = devm_kzalloc(dev, sizeof(*map), GFP_KERNEL);
	if (IS_ERR_OR_NULL(map)) {
		pr_err("%s: not enough memory!\n", __func__);
		return NULL;
	}

	map->type = AXP_REGMAP_I2C;
	map->client = to_i2c_client(dev);
	spin_lock_init(&map->spinlock);
	mutex_init(&map->lock);

	return map;
}
EXPORT_SYMBOL_GPL(axp_regmap_init_i2c);

struct axp_regmap *axp_regmap_init_rsb(struct device *dev, u8 addr)
{
	struct axp_regmap *map = NULL;

	map = devm_kzalloc(dev, sizeof(*map), GFP_KERNEL);
	if (IS_ERR_OR_NULL(map)) {
		pr_err("%s: not enough memory!\n", __func__);
		return NULL;
	}

	map->type = AXP_REGMAP_RSB;
	map->rsbaddr = addr;
	spin_lock_init(&map->spinlock);
	mutex_init(&map->lock);

	return map;
}
EXPORT_SYMBOL_GPL(axp_regmap_init_rsb);

static void axp_irq_work(struct work_struct *work)
{
	u64 irqs = 0;
	u8 reg_val[8];
	u32 i, j;
	void *idata;
	struct axp_irq_chip_data *irq_data = container_of(work,
			struct axp_irq_chip_data, irq_work);

	axp_regmap_reads(irq_data->map, irq_data->chip->status_base,
			irq_data->chip->num_regs, reg_val);

	for (i = 0; i < irq_data->chip->num_regs; i++)
		irqs |= (u64)reg_val[i] << (i * AXP_REG_WIDTH);

	for_each_set_bit(j, (unsigned long *)&irqs, irq_data->num_irqs) {
		if (irq_data->irqs[j].handler) {
			idata = irq_data->irqs[j].data;
			irq_data->irqs[j].handler(0, idata);
		}
	}

	axp_regmap_writes(irq_data->map, irq_data->chip->status_base,
			irq_data->chip->num_regs, reg_val);

#ifdef CONFIG_AXP_NMI_USED
	clear_nmi_status();
	enable_nmi();
#endif
}

static irqreturn_t axp_irq(int irq, void *data)
{
	struct axp_irq_chip_data *irq_data = (struct axp_irq_chip_data *)data;

#ifdef CONFIG_AXP_NMI_USED
	disable_nmi();
#endif

	schedule_work(&irq_data->irq_work);

	return IRQ_HANDLED;
}

struct axp_irq_chip_data *axp_irq_chip_register(struct axp_regmap *map,
			int irq_no, int irq_flags,
			struct axp_regmap_irq_chip *irq_chip)
{
	struct axp_irq_chip_data *irq_data = NULL;
	struct axp_regmap_irq *irqs = NULL;
	int i, err;

	irq_data = kzalloc(sizeof(*irq_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(irq_data)) {
		pr_err("axp irq data: not enough memory for irq data\n");
		return NULL;
	}

	irq_data->map = map;
	irq_data->chip = irq_chip;
	irq_data->num_irqs = AXP_REG_WIDTH * irq_chip->num_regs;

	irqs = kzalloc(irq_chip->num_regs * AXP_REG_WIDTH * sizeof(*irqs), GFP_KERNEL);
	if (IS_ERR_OR_NULL(irqs)) {
		pr_err("axp irq data: not enough memory for irq disc\n");
		goto free_irq_data;
	}

	irq_data->irqs = irqs;
	mutex_init(&irq_data->lock);
	INIT_WORK(&irq_data->irq_work, axp_irq_work);

	/* disable all irq */
	for (i = 0; i < irq_chip->num_regs; i++)
		axp_regmap_clr_bits(map, irq_chip->enable_base + i, 0xff);

	err = request_irq(irq_no, axp_irq, irq_flags, irq_chip->name, irq_data);
	if (err)
		goto free_irqs;

#ifdef CONFIG_AXP_NMI_USED
	set_nmi_trigger(IRQF_TRIGGER_LOW);
	enable_nmi();
#endif

	return irq_data;

free_irqs:
	kfree(irqs);
free_irq_data:
	kfree(irq_data);

	return NULL;
}
EXPORT_SYMBOL_GPL(axp_irq_chip_register);

void axp_irq_chip_unregister(int irq, struct axp_irq_chip_data *irq_data)
{
	int i;
	struct axp_regmap *map = irq_data->map;

	free_irq(irq, irq_data);

	/* disable all irq and clear all irq pending */
	for(i = 0; i < irq_data->chip->num_regs; i++){
		axp_regmap_clr_bits(map, irq_data->chip->enable_base + i, 0xff);
		axp_regmap_write(map, irq_data->chip->status_base + i, 0xff);
	}

	kfree(irq_data->irqs);
	kfree(irq_data);

#ifdef CONFIG_AXP_NMI_USED
	disable_nmi();
#endif
}
EXPORT_SYMBOL_GPL(axp_irq_chip_unregister);

int axp_request_irq(struct axp_dev *adev, int irq_no, irq_handler_t handler, void *data)
{
	struct axp_irq_chip_data *irq_data = adev->irq_data;
	struct axp_regmap_irq *irqs = irq_data->irqs;
	int reg, ret;
	u8 mask;

	if (!irq_data || irq_no < 0 || irq_no >= irq_data->num_irqs || !handler)
		return -1;

	mutex_lock(&irq_data->lock);
	irqs[irq_no].handler = handler;
	irqs[irq_no].data = data;
	reg = irq_no / AXP_REG_WIDTH;
	reg += irq_data->chip->enable_base;
	mask = 1 << (irq_no % AXP_REG_WIDTH);
	ret = axp_regmap_set_bits(adev->regmap, reg, mask);
	mutex_unlock(&irq_data->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_request_irq);

int axp_enable_irq(struct axp_dev *adev, int irq_no)
{
	struct axp_irq_chip_data *irq_data = adev->irq_data;
	int reg, ret = 0;
	u8 mask;

	if (!irq_data || irq_no < 0 || irq_no >= irq_data->num_irqs)
		return -1;

	if (irq_data->irqs[irq_no].handler) {
		mutex_lock(&irq_data->lock);
		reg = irq_no / AXP_REG_WIDTH;
		reg += irq_data->chip->enable_base;
		mask = 1 << (irq_no % AXP_REG_WIDTH);
		ret = axp_regmap_set_bits(adev->regmap, reg, mask);
		mutex_unlock(&irq_data->lock);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(axp_enable_irq);

int axp_disable_irq(struct axp_dev *adev, int irq_no)
{
	struct axp_irq_chip_data *irq_data = adev->irq_data;
	int reg, ret = 0;
	u8 mask;

	if (!irq_data || irq_no < 0 || irq_no >= irq_data->num_irqs)
		return -1;

	mutex_lock(&irq_data->lock);
	reg = irq_no / AXP_REG_WIDTH;
	reg += irq_data->chip->enable_base;
	mask = 1 << (irq_no % AXP_REG_WIDTH);
	ret = axp_regmap_clr_bits(adev->regmap, reg, mask);
	mutex_unlock(&irq_data->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_disable_irq);

int axp_free_irq(struct axp_dev *adev, int irq_no)
{
	struct axp_irq_chip_data *irq_data = adev->irq_data;
	int reg;
	u8 mask;

	if (!irq_data || irq_no < 0 || irq_no >= irq_data->num_irqs)
		return -1;

	mutex_lock(&irq_data->lock);
	if (irq_data->irqs[irq_no].handler) {
		reg = irq_no / AXP_REG_WIDTH;
		reg += irq_data->chip->enable_base;
		mask = 1 << (irq_no % AXP_REG_WIDTH);
		axp_regmap_clr_bits(adev->regmap, reg, mask);
		irq_data->irqs[irq_no].data = NULL;
		irq_data->irqs[irq_no].handler = NULL;
	}
	mutex_unlock(&irq_data->lock);

	return 0;
}
EXPORT_SYMBOL_GPL(axp_free_irq);

static u8 axp_reg_addr = 0;
static ssize_t axp_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 val;
	struct axp_dev *axp_dev = dev_get_drvdata(dev);

	axp_regmap_read(axp_dev->regmap, axp_reg_addr, &val);
	return sprintf(buf, "REG[%x]=0x%x\n", axp_reg_addr, val);
}

static ssize_t axp_reg_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	s32 tmp;
	u8 val;
	struct axp_dev *axp_dev = dev_get_drvdata(dev);

	tmp = simple_strtoul(buf, NULL, 16);
	if (tmp < 256) {
		axp_reg_addr = tmp;
	} else {
		val = tmp & 0x00FF;
		axp_reg_addr = (tmp >> 8) & 0x00FF;
		axp_regmap_write(axp_dev->regmap, axp_reg_addr, val);
	}

	return count;
}

static u32 data2 = 2;
static ssize_t axp_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 val;
	s32 count = 0, i = 0;
	struct axp_dev *axp_dev = dev_get_drvdata(dev);

	for (i = 0; i < data2; i++) {
		axp_regmap_read(axp_dev->regmap, axp_reg_addr+i, &val);
		count += sprintf(buf+count, "REG[0x%x]=0x%x\n",
				axp_reg_addr+i, val);
	}

	return count;
}

static ssize_t axp_regs_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	u32 data1 = 0;
	u8 val[2];
	char *endp;
	struct axp_dev *axp_dev = dev_get_drvdata(dev);

	data1 = simple_strtoul(buf, &endp, 16);
	if (*endp != ' ') {
		printk("%s: %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	buf = endp + 1;
	data2 = simple_strtoul(buf, &endp, 10);

	if (data1 < 256) {
		axp_reg_addr = data1;
	} else {
		axp_reg_addr = (data1 >> 16) & 0xFF;
		val[0] = (data1 >> 8) & 0xFF;
		val[1] = data1 & 0xFF;
		axp_regmap_writes(axp_dev->regmap, axp_reg_addr, 2, val);
	}

	return count;
}

static struct device_attribute axp_dump_attrs[] = {
	AXP_DUMP_ATTR(axp_reg),
	AXP_DUMP_ATTR(axp_regs),
};

int axp_mfd_add_devices(struct axp_dev *axp_dev)
{
	int ret, j;

	ret = mfd_add_devices(axp_dev->dev, -1,
		axp_dev->cells, axp_dev->nr_cells, NULL, 0, NULL);
	if(ret)
		goto fail;

	dev_set_drvdata(axp_dev->dev, axp_dev);
	for (j = 0; j < ARRAY_SIZE(axp_dump_attrs); j++) {
		ret = device_create_file(axp_dev->dev, (axp_dump_attrs+j));
		if (ret)
			goto sysfs_failed;
	}

	return 0;

sysfs_failed:
	while (j--)
		device_remove_file(axp_dev->dev, (axp_dump_attrs+j));
	mfd_remove_devices(axp_dev->dev);
fail:
	return ret;
}
EXPORT_SYMBOL_GPL(axp_mfd_add_devices);

int axp_mfd_remove_devices(struct axp_dev *axp_dev)
{
	int j;

	for (j = 0; j < ARRAY_SIZE(axp_dump_attrs); j++)
		device_remove_file(axp_dev->dev, (axp_dump_attrs+j));
	mfd_remove_devices(axp_dev->dev);

	return 0;
}
EXPORT_SYMBOL_GPL(axp_mfd_remove_devices);

int axp_debug;
static ssize_t axpdebug_store(struct class *class,
			struct class_attribute *attr,
			const char *buf, size_t count)
{
	int val, err;

	err = kstrtoint(buf, 16, &val);
	if (err)
		return err;

	axp_debug = val;

	return count;
}

static ssize_t axpdebug_show(struct class *class,
			struct class_attribute *attr,   char *buf)
{
	return sprintf(buf, "%x\n", axp_debug);
}

static struct class_attribute axppower_class_attrs[] = {
	__ATTR(axpdebug, S_IRUGO|S_IWUSR, axpdebug_show, axpdebug_store),
	__ATTR_NULL
};

struct class axppower_class = {
	.name = "axppower",
	.class_attrs = axppower_class_attrs,
};

static s32 __init axp_core_init(void)
{
	class_register(&axppower_class);
	return 0;
}
arch_initcall(axp_core_init);

MODULE_DESCRIPTION("ALLWINNERTECH axp board");
MODULE_AUTHOR("Qin Yongshen");
MODULE_LICENSE("GPL");
