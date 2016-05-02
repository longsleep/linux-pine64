#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/power_supply.h>
#include "../axp-core.h"
#include "../axp-charger.h"
#include "axp22-charger.h"

struct axp_charger_interrupts {
	char *name;
	irq_handler_t isr;
};

static struct axp_config_info axp22_config;

static int axp22_get_ac_voltage(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp22_get_ac_current(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp22_set_ac_vhold(struct axp_charger_dev *cdev, int vol)
{
	return 0;
}

static int axp22_get_ac_vhold(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp22_set_ac_ihold(struct axp_charger_dev *cdev, int cur)
{
	return 0;
}

static int axp22_get_ac_ihold(struct axp_charger_dev *cdev)
{
	return 0;
}

static struct axp_ac_info axp22_ac_info = {
	.det_bit         = 7,
	.det_offset      = AXP22_CHARGE_STATUS,
	.valid_bit       = 6,
	.valid_offset    = AXP22_CHARGE_STATUS,
	.in_short_bit    = 1,
	.in_short_offset = AXP22_CHARGE_STATUS,
	.get_ac_voltage  = axp22_get_ac_voltage,
	.get_ac_current  = axp22_get_ac_current,
	.set_ac_vhold    = axp22_set_ac_vhold,
	.get_ac_vhold    = axp22_get_ac_vhold,
	.set_ac_ihold    = axp22_set_ac_ihold,
	.get_ac_ihold    = axp22_get_ac_ihold,
};

static int axp22_get_usb_voltage(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp22_get_usb_current(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp22_set_usb_vhold(struct axp_charger_dev *cdev, int vol)
{
	u8 tmp;
	struct axp_regmap *map = cdev->chip->regmap;

	if (vol) {
		axp_regmap_set_bits(map, AXP22_CHARGE_VBUS, 0x40);
		if (vol >= 4000 && vol <= 4700) {
			tmp = (vol - 4000)/100;
			axp_regmap_update(map, AXP22_CHARGE_VBUS, tmp<<3, 0x7<<3);
		} else {
			pr_err("set usb limit voltage error, %d mV\n",
				axp22_config.pmu_usbpc_vol);
		}
	} else {
		axp_regmap_clr_bits(map, AXP22_CHARGE_VBUS, 0x40);
	}

	return 0;

}

static int axp22_get_usb_vhold(struct axp_charger_dev *cdev)
{
	u8 tmp;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, AXP22_CHARGE_VBUS, &tmp);
	tmp = (tmp >> 3) & 0x7;

	return 4000 + tmp * 100;
}

static int axp22_set_usb_ihold(struct axp_charger_dev *cdev, int cur)
{
	struct axp_regmap *map = cdev->chip->regmap;

	if (cur) {
		if (cur >= 900)
			axp_regmap_clr_bits(map, AXP22_CHARGE_VBUS, 0x3);
		else if (cur < 900)
			axp_regmap_update(map, AXP22_CHARGE_VBUS, 0x1, 0x3);
	} else {
		axp_regmap_set_bits(map, AXP22_CHARGE_VBUS, 0x3);
	}

	return 0;
}

static int axp22_get_usb_ihold(struct axp_charger_dev *cdev)
{
	u8 tmp;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, AXP22_CHARGE_VBUS, &tmp);
	tmp = tmp & 0x3;
	if(tmp == 0x1)
		return 500;
	else if(tmp == 0)
		return 900;
	else
		return 0;
}

static struct axp_usb_info axp22_usb_info = {
	.det_bit         = 5,
	.det_offset      = AXP22_CHARGE_STATUS,
	.valid_bit       = 4,
	.valid_offset    = AXP22_CHARGE_STATUS,
	.get_usb_voltage = axp22_get_usb_voltage,
	.get_usb_current = axp22_get_usb_current,
	.set_usb_vhold   = axp22_set_usb_vhold,
	.get_usb_vhold   = axp22_get_usb_vhold,
	.set_usb_ihold   = axp22_set_usb_ihold,
	.get_usb_ihold   = axp22_get_usb_ihold,
};

static int axp22_get_rest_cap(struct axp_charger_dev *cdev)
{
	u8 val, temp_val[2], batt_max_cap_val[2];
	int batt_max_cap, coulumb_counter;
	int rest_vol;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, AXP22_CAP, &val);
	rest_vol = (int) (val & 0x7F);

	axp_regmap_reads(map, 0xe2, 2, temp_val);
	coulumb_counter = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;

	axp_regmap_reads(map, 0xe0, 2, temp_val);
	batt_max_cap = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;

	/* Avoid the power stay in 100% for a long time. */
	if (coulumb_counter > batt_max_cap) {
		batt_max_cap_val[0] = temp_val[0] | (0x1<<7);
		batt_max_cap_val[1] = temp_val[1];
		axp_regmap_writes(map, 0xe2, 2, batt_max_cap_val);
		DBG_PSY_MSG(DEBUG_SPLY, "Axp22 coulumb_counter = %d\n",
						batt_max_cap);
	}

	return rest_vol;
}

static int axp22_get_bat_health(struct axp_charger_dev *cdev)
{
	u8 val;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, AXP22_FAULT_LOG1, &val);
	if (val & AXP22_FAULT_LOG_BATINACT)
		return POWER_SUPPLY_HEALTH_DEAD;
	else if (val & AXP22_FAULT_LOG_OVER_TEMP)
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (val & AXP22_FAULT_LOG_COLD)
		return POWER_SUPPLY_HEALTH_COLD;
	else
		return POWER_SUPPLY_HEALTH_GOOD;
}

static inline int axp22_vbat_to_mV(u32 reg)
{
	return ((int)((( reg >> 8) << 4 ) | (reg & 0x000F))) * 1100 / 1000;
}

static int axp22_get_vbat(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	u32 res;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, AXP22_VBATH_RES, 2, tmp);
	res = (tmp[0] << 8) | tmp[1];

	return axp22_vbat_to_mV(res);
}

static inline int axp22_ibat_to_mA(u32 reg)
{
	return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) ;
}

static inline int axp22_icharge_to_mA(u32 reg)
{
	return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F)));
}

static int axp22_get_ibat(struct axp_charger_dev *cdev)
{
	u8 tmp[4];
	u32 res, dis_res;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, AXP22_IBATH_REG, 4, tmp);
	res = (tmp[0] << 8) | tmp[1];
	dis_res = (tmp[2] << 8) | tmp[3];

	return ABS(axp22_icharge_to_mA(res) - axp22_ibat_to_mA(dis_res));
}

static int axp22_set_chg_cur(struct axp_charger_dev *cdev, int cur)
{
	uint8_t tmp = 0;
	struct axp_regmap *map = cdev->chip->regmap;

	if (cur == 0)
		axp_regmap_clr_bits(map, AXP22_CHARGE_CONTROL1, 0x80);
	else
		axp_regmap_set_bits(map, AXP22_CHARGE_CONTROL1, 0x80);

	printk("current_limit = %d\n", cur);

	if (cur >= 300000 && cur <= 2550000) {
		tmp = (cur - 300000) / 150000;
		axp_regmap_update(map, AXP22_CHARGE_CONTROL1, tmp, 0x0F);
	} else if (cur < 300000) {
		axp_regmap_clr_bits(map, AXP22_CHARGE_CONTROL1, 0x0F);
	} else {
		axp_regmap_set_bits(map, AXP22_CHARGE_CONTROL1, 0x0F);
	}

	return 0;
}

static int axp22_set_chg_vol(struct axp_charger_dev *cdev, int vol)
{
	uint8_t tmp = 0;
	struct axp_regmap *map = cdev->chip->regmap;

	if (vol < 4200) {
		tmp &= ~(3 << 5);
	} else if (vol < 4220) {
		tmp &= ~(3 << 5);
		tmp |= 1 << 6;
	} else if (vol < 4240) {
		tmp &= ~(3 << 5);
		tmp |= 1 << 5;
	} else {
		tmp |= 3 << 5;
	}

	axp_regmap_update(map, AXP22_CHARGE_CONTROL1, tmp, 0x3<<5);

	return 0;
}

static struct axp_battery_info axp22_batt_info = {
	.chgstat_bit          = 6,
	.chgstat_offset       = AXP22_MODE_CHGSTATUS,
	.det_bit              = 5,
	.det_offset           = AXP22_MODE_CHGSTATUS,
	.cur_direction_bit    = 2,
	.cur_direction_offset = AXP22_CHARGE_STATUS,
	.get_rest_cap         = axp22_get_rest_cap,
	.get_bat_health       = axp22_get_bat_health,
	.get_vbat             = axp22_get_vbat,
	.get_ibat             = axp22_get_ibat,
	.set_chg_cur          = axp22_set_chg_cur,
	.set_chg_vol          = axp22_set_chg_vol,
};

static struct power_supply_info battery_data = {
	.name ="PTI PL336078",
	.technology = POWER_SUPPLY_TECHNOLOGY_LiFe,
	.voltage_max_design = 4200000,
	.voltage_min_design = 3500000,
	.use_for_apm = 1,
};

static struct axp_supply_info axp22_spy_info = {
	.ac   = &axp22_ac_info,
	.usb  = &axp22_usb_info,
	.batt = &axp22_batt_info,
};

static int axp22_charger_init(struct axp_dev *axp_dev)
{
	u8 ocv_cap[32];
	u8 val;
	int cur_coulomb_counter, rdc;
	struct axp_regmap *map = axp_dev->regmap;

	if (axp22_config.pmu_init_chgend_rate == 10)
		val &= ~(1 << 4);
	else
		val |= 1 << 4;

	val &= 0x7F;
	axp_regmap_write(map, AXP22_CHARGE_CONTROL1,val);

	if(axp22_config.pmu_init_chg_pretime < 30)
		axp22_config.pmu_init_chg_pretime = 30;

	if(axp22_config.pmu_init_chg_csttime < 360)
		axp22_config.pmu_init_chg_csttime = 360;

	val = ((((axp22_config.pmu_init_chg_pretime - 40) / 10) << 6)
			| ((axp22_config.pmu_init_chg_csttime - 360) / 120));
	axp_regmap_update(map, AXP22_CHARGE_CONTROL2, val, 0xC2);

	/* adc set */
	val = AXP22_ADC_BATVOL_ENABLE | AXP22_ADC_BATCUR_ENABLE;
	if (0 != axp22_config.pmu_bat_temp_enable)
		val = val | AXP22_ADC_TSVOL_ENABLE;
	axp_regmap_update(map, AXP22_ADC_CONTROL, val,
						AXP22_ADC_BATVOL_ENABLE
						| AXP22_ADC_BATCUR_ENABLE
						| AXP22_ADC_TSVOL_ENABLE);

	axp_regmap_read(map, AXP22_ADC_CONTROL3, &val);
	switch (axp22_config.pmu_init_adc_freq / 100) {
	case 1:
		val &= ~(3 << 6);
		break;
	case 2:
		val &= ~(3 << 6);
		val |= 1 << 6;
		break;
	case 4:
		val &= ~(3 << 6);
		val |= 2 << 6;
		break;
	case 8:
		val |= 3 << 6;
		break;
	default:
		break;
	}

	if (0 != axp22_config.pmu_bat_temp_enable)
		val &= (~(1 << 2));
	axp_regmap_write(map, AXP22_ADC_CONTROL3, val);

	/* bat para */
	axp_regmap_write(map, AXP22_WARNING_LEVEL,
		((axp22_config.pmu_battery_warning_level1 - 5) << 4)
		+ axp22_config.pmu_battery_warning_level2);
	ocv_cap[0]  = axp22_config.pmu_bat_para1;
	ocv_cap[1]  = axp22_config.pmu_bat_para2;
	ocv_cap[2]  = axp22_config.pmu_bat_para3;
	ocv_cap[3]  = axp22_config.pmu_bat_para4;
	ocv_cap[4]  = axp22_config.pmu_bat_para5;
	ocv_cap[5]  = axp22_config.pmu_bat_para6;
	ocv_cap[6]  = axp22_config.pmu_bat_para7;
	ocv_cap[7]  = axp22_config.pmu_bat_para8;
	ocv_cap[8]  = axp22_config.pmu_bat_para9;
	ocv_cap[9]  = axp22_config.pmu_bat_para10;
	ocv_cap[10] = axp22_config.pmu_bat_para11;
	ocv_cap[11] = axp22_config.pmu_bat_para12;
	ocv_cap[12] = axp22_config.pmu_bat_para13;
	ocv_cap[13] = axp22_config.pmu_bat_para14;
	ocv_cap[14] = axp22_config.pmu_bat_para15;
	ocv_cap[15] = axp22_config.pmu_bat_para16;
	ocv_cap[16] = axp22_config.pmu_bat_para17;
	ocv_cap[17] = axp22_config.pmu_bat_para18;
	ocv_cap[18] = axp22_config.pmu_bat_para19;
	ocv_cap[19] = axp22_config.pmu_bat_para20;
	ocv_cap[20] = axp22_config.pmu_bat_para21;
	ocv_cap[21] = axp22_config.pmu_bat_para22;
	ocv_cap[22] = axp22_config.pmu_bat_para23;
	ocv_cap[23] = axp22_config.pmu_bat_para24;
	ocv_cap[24] = axp22_config.pmu_bat_para25;
	ocv_cap[25] = axp22_config.pmu_bat_para26;
	ocv_cap[26] = axp22_config.pmu_bat_para27;
	ocv_cap[27] = axp22_config.pmu_bat_para28;
	ocv_cap[28] = axp22_config.pmu_bat_para29;
	ocv_cap[29] = axp22_config.pmu_bat_para30;
	ocv_cap[30] = axp22_config.pmu_bat_para31;
	ocv_cap[31] = axp22_config.pmu_bat_para32;
	axp_regmap_writes(map, 0xC0, 32, ocv_cap);

	axp_regmap_read(map, AXP22_POK_SET, &val);
	if (axp22_config.pmu_powkey_on_time < 1000)
		val &= 0x3f;
	else if (axp22_config.pmu_powkey_on_time < 2000) {
		val &= 0x3f;
		val |= 0x40;
	} else if (axp22_config.pmu_powkey_on_time < 3000) {
		val &= 0x3f;
		val |= 0x80;
	} else {
		val &= 0x3f;
		val |= 0xc0;
	}
	axp_regmap_write(map,AXP22_POK_SET,val);

	/* pok long time set*/
	if(axp22_config.pmu_powkey_long_time < 1000)
		axp22_config.pmu_powkey_long_time = 1000;

	if(axp22_config.pmu_powkey_long_time > 2500)
		axp22_config.pmu_powkey_long_time = 2500;

	axp_regmap_read(map,AXP22_POK_SET,&val);
	val &= 0xcf;
	val |= (((axp22_config.pmu_powkey_long_time - 1000) / 500) << 4);
	axp_regmap_write(map,AXP22_POK_SET,val);

	/* pek offlevel poweroff en set*/
	if (axp22_config.pmu_powkey_off_en)
		axp22_config.pmu_powkey_off_en = 1;
	else
		axp22_config.pmu_powkey_off_en = 0;

	axp_regmap_read(map, AXP22_POK_SET, &val);
	val &= 0xf7;
	val |= (axp22_config.pmu_powkey_off_en << 3);
	axp_regmap_write(map, AXP22_POK_SET, val);

	/*Init offlevel restart or not */
	if (axp22_config.pmu_powkey_off_func)
		axp_regmap_set_bits(map, AXP22_POK_SET, 0x04); /* restart */
	else
		axp_regmap_clr_bits(map, AXP22_POK_SET, 0x04); /* not restart*/

	/* pek delay set */
	axp_regmap_read(map, AXP22_OFF_CTL, &val);
	val &= 0xfc;
	if (axp22_config.pmu_pwrok_time < 32)
		val |= ((axp22_config.pmu_pwrok_time / 8) - 1);
	else
		val |= ((axp22_config.pmu_pwrok_time / 32) + 1);
	axp_regmap_write(map, AXP22_OFF_CTL, val);

	/* pek offlevel time set */
	if (axp22_config.pmu_powkey_off_time < 4000)
		axp22_config.pmu_powkey_off_time = 4000;

	if (axp22_config.pmu_powkey_off_time > 10000)
		axp22_config.pmu_powkey_off_time =10000;
	axp22_config.pmu_powkey_off_time = (axp22_config.pmu_powkey_off_time - 4000) / 2000 ;

	axp_regmap_read(map, AXP22_POK_SET, &val);
	val &= 0xfc;
	val |= axp22_config.pmu_powkey_off_time ;
	axp_regmap_write(map, AXP22_POK_SET, val);

	/*Init 16's Reset PMU en */
	if (axp22_config.pmu_reset)
		axp_regmap_set_bits(map, 0x8F, 0x08); /* enable */
	else
		axp_regmap_clr_bits(map, 0x8F, 0x08); /* disable */

	/*Init IRQ wakeup en*/
	if (axp22_config.pmu_irq_wakeup)
		axp_regmap_set_bits(map, 0x8F, 0x80); /* enable */
	else
		axp_regmap_clr_bits(map, 0x8F, 0x80); /* disable */

	/*Init N_VBUSEN status*/
	if (axp22_config.pmu_vbusen_func)
		axp_regmap_set_bits(map, 0x8F, 0x10); /* output */
	else
		axp_regmap_clr_bits(map, 0x8F, 0x10); /* input */

	/*Init InShort status*/
	if (axp22_config.pmu_inshort)
		axp_regmap_set_bits(map, 0x8F, 0x60); /* InShort */
	else
		axp_regmap_clr_bits(map, 0x8F, 0x60); /* auto detect */

	/*Init CHGLED function*/
	if (axp22_config.pmu_chgled_func)
		axp_regmap_set_bits(map, 0x32, 0x08); /* control by charger */
	else
		axp_regmap_clr_bits(map, 0x32, 0x08); /* drive MOTO */

	/*set CHGLED Indication Type*/
	if (axp22_config.pmu_chgled_type)
		axp_regmap_set_bits(map, 0x34, 0x10); /* Type B */
	else
		axp_regmap_clr_bits(map, 0x34, 0x10); /* Type A */

	/*Init PMU Over Temperature protection*/
	if (axp22_config.pmu_hot_shutdown)
		axp_regmap_set_bits(map, 0x8f, 0x04); /* enable */
	else
		axp_regmap_clr_bits(map, 0x8f, 0x04); /* disable */

	/*Init battery capacity correct function*/
	if (axp22_config.pmu_batt_cap_correct)
		axp_regmap_set_bits(map, 0xb8, 0x20); /* enable */
	else
		axp_regmap_clr_bits(map, 0xb8, 0x20); /* disable */

	/* Init battery regulator enable or not when charge finish*/
	if (axp22_config.pmu_chg_end_on_en)
		axp_regmap_set_bits(map, 0x34, 0x20); /* enable */
	else
		axp_regmap_clr_bits(map, 0x34, 0x20); /* disable */

	if (!axp22_config.pmu_batdeten)
		axp_regmap_clr_bits(map, AXP22_PDBC, 0x40);
	else
		axp_regmap_set_bits(map, AXP22_PDBC, 0x40);

	/* RDC initial */
	axp_regmap_read(map, AXP22_RDC0, &val);
	if ((axp22_config.pmu_battery_rdc) && (!(val & 0x40))) {
		rdc = (axp22_config.pmu_battery_rdc * 10000 + 5371) / 10742;
		axp_regmap_write(map, AXP22_RDC0, ((rdc >> 8) & 0x1F)|0x80);
		axp_regmap_write(map, AXP22_RDC1, rdc & 0x00FF);
	}

	axp_regmap_read(map, AXP22_BATCAP0, &val);
	if ((axp22_config.pmu_battery_cap) && (!(val & 0x80))) {
		cur_coulomb_counter = axp22_config.pmu_battery_cap
					* 1000 / 1456;
		axp_regmap_write(map, AXP22_BATCAP0,
					((cur_coulomb_counter >> 8) | 0x80));
		axp_regmap_write(map, AXP22_BATCAP1,
					cur_coulomb_counter & 0x00FF);
	} else if (!axp22_config.pmu_battery_cap) {
		axp_regmap_write(map, AXP22_BATCAP0, 0x00);
		axp_regmap_write(map, AXP22_BATCAP1, 0x00);
	}

	return 0;
}

static irqreturn_t axp_ac_usb_in_isr(int irq, void *data)
{
	struct axp_charger_dev *chg_dev = data;
	axp_change(chg_dev);
	axp_usbac_in(chg_dev);
	return IRQ_HANDLED;
}

static irqreturn_t axp_ac_usb_out_isr(int irq, void *data)
{
	struct axp_charger_dev *chg_dev = data;
	axp_change(chg_dev);
	axp_usbac_out(chg_dev);
	return IRQ_HANDLED;
}

static irqreturn_t axp_capchange_isr(int irq, void *data)
{
	struct axp_charger_dev *chg_dev = data;
	axp_capchange(chg_dev);
	return IRQ_HANDLED;
}

static irqreturn_t axp_change_isr(int irq, void *data)
{
	struct axp_charger_dev *chg_dev = data;
	axp_change(chg_dev);
	return IRQ_HANDLED;
}

static struct axp_charger_interrupts axp_charger_irq[] = {
	{"usb in",       axp_ac_usb_in_isr},
	{"usb out",      axp_ac_usb_out_isr},
	{"ac in",        axp_ac_usb_in_isr},
	{"ac out",       axp_ac_usb_out_isr},
	{"bat in",       axp_capchange_isr},
	{"bat out",      axp_capchange_isr},
	{"bat temp low", axp_change_isr},
	{"bat temp over",axp_change_isr},
	{"charging",     axp_change_isr},
	{"charge over",  axp_change_isr},
};

static int axp22_charger_probe(struct platform_device *pdev)
{
	int ret, i, irq;
	struct axp_charger_dev* chg_dev;
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	if (pdev->dev.of_node) {
		/* get dt and sysconfig */
		ret = axp_charger_dt_parse(pdev->dev.of_node, &axp22_config);
	} else {
		pr_err("axp22 charger device tree err!\n");
		return -EBUSY;
	}

	axp22_ac_info.ac_vol = axp22_config.pmu_ac_vol;
	axp22_ac_info.ac_cur = axp22_config.pmu_ac_cur;
	axp22_usb_info.usb_pc_vol = axp22_config.pmu_usbpc_vol;
	axp22_usb_info.usb_pc_cur = axp22_config.pmu_usbpc_cur;
	axp22_usb_info.usb_ad_vol = axp22_config.pmu_ac_vol;
	axp22_usb_info.usb_ad_cur = axp22_config.pmu_ac_cur;
	axp22_batt_info.runtime_chgcur = axp22_config.pmu_runtime_chgcur;
	axp22_batt_info.suspend_chgcur= axp22_config.pmu_suspend_chgcur;
	axp22_batt_info.shutdown_chgcur= axp22_config.pmu_shutdown_chgcur;
	battery_data.voltage_max_design = axp22_config.pmu_init_chgvol;
	battery_data.voltage_min_design = axp22_config.pmu_pwroff_vol;
	battery_data.energy_full_design = axp22_config.pmu_battery_cap;

	axp22_charger_init(axp_dev);

	chg_dev = axp_power_supply_register(&pdev->dev, axp_dev,
					&battery_data, &axp22_spy_info);
	if (IS_ERR_OR_NULL(chg_dev))
		goto fail;

	for (i = 0; i < ARRAY_SIZE(axp_charger_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_charger_irq[i].name);
		if (irq < 0)
			continue;

		ret = axp_request_irq(axp_dev, irq,
				axp_charger_irq[i].isr, chg_dev);
		if (ret != 0) {
			dev_err(&pdev->dev, "failed to request %s IRQ %d: %d\n",
					axp_charger_irq[i].name, irq, ret);
			goto out_irq;
		}

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_charger_irq[i].name, irq, ret);
	}

	platform_set_drvdata(pdev, chg_dev);

	return 0;

out_irq:
	for (i = i - 1; i >= 0; i--) {
		irq = platform_get_irq_byname(pdev, axp_charger_irq[i].name);
		if (irq < 0)
			continue;
		axp_free_irq(axp_dev, irq);
	}
fail:
	return -1;
}

static int axp22_charger_remove(struct platform_device *pdev)
{
	int i, irq;
	struct axp_charger_dev* chg_dev = platform_get_drvdata(pdev);
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	for (i = 0; i < ARRAY_SIZE(axp_charger_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_charger_irq[i].name);
		if (irq < 0)
			continue;
		axp_free_irq(axp_dev, irq);
	}

	axp_power_supply_unregister(chg_dev);

	return 0;
}

static int axp22_suspend(struct platform_device *dev, pm_message_t state)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);

	axp_charger_suspend(chg_dev);

	return 0;
}

static int axp22_resume(struct platform_device *dev)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);
	struct axp_regmap *map = chg_dev->chip->regmap;
	int pre_rest_vol;

	pre_rest_vol = chg_dev->rest_vol;
	axp_charger_resume(chg_dev);

	if (chg_dev->rest_vol - pre_rest_vol) {
		pr_info("battery vol change: %d->%d\n",
				pre_rest_vol, chg_dev->rest_vol);
		axp_regmap_write(map, AXP22_DATA_BUFFER1,
				chg_dev->rest_vol | 0x80);
	}

	return 0;
}

static void axp22_shutdown(struct platform_device *dev)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);
	axp_charger_shutdown(chg_dev);
}

static const struct of_device_id axp22_charger_dt_ids[] = {
	{ .compatible = "allwinner,axp22-charger", },
	{},
};
MODULE_DEVICE_TABLE(of, axp22_charger_dt_ids);

static struct platform_driver axp22_charger_driver = {
	.driver     = {
		.name   = "axp22-charger",
		.of_match_table = axp22_charger_dt_ids,
	},
	.probe    = axp22_charger_probe,
	.remove   = axp22_charger_remove,
	.suspend  = axp22_suspend,
	.resume   = axp22_resume,
	.shutdown = axp22_shutdown,
};

static int __init axp22_charger_initcall(void)
{
	int ret;

	ret = platform_driver_register(&axp22_charger_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
fs_initcall(axp22_charger_initcall);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_DESCRIPTION("Charger Driver for axp22x PMIC");
MODULE_ALIAS("platform:axp22-charger");
