#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include "axp-core.h"
#include "axp-charger.h"

#define BATRDC          100
#define INTCHGCUR       300000      /* set initial charging current limite */
#define SUSCHGCUR       1000000     /* set suspend charging current limite */
#define RESCHGCUR       INTCHGCUR   /* set resume charging current limite */
#define CLSCHGCUR       SUSCHGCUR   /* set shutdown charging current limite */
#define INTCHGVOL       4200000     /* set initial charing target voltage */
#define INTCHGENDRATE   10          /* set initial charing end current rate */
#define INTCHGENABLED   1           /* set initial charing enabled */
#define INTADCFREQ      25          /* set initial adc frequency */
#define INTADCFREQC     100         /* set initial coulomb adc coufrequency */
#define INTCHGPRETIME   50          /* set initial pre-charging time */
#define INTCHGCSTTIME   480         /* set initial pre-charging time */
#define BATMAXVOL       4200000     /* set battery max design volatge */
#define BATMINVOL       3500000     /* set battery min design volatge */

#define OCVREG0         0x00        /* 2.99V */
#define OCVREG1         0x00        /* 3.13V */
#define OCVREG2         0x00        /* 3.27V */
#define OCVREG3         0x00        /* 3.34V */
#define OCVREG4         0x00        /* 3.41V */
#define OCVREG5         0x00        /* 3.48V */
#define OCVREG6         0x00        /* 3.52V */
#define OCVREG7         0x00        /* 3.55V */
#define OCVREG8         0x04        /* 3.57V */
#define OCVREG9         0x05        /* 3.59V */
#define OCVREGA         0x06        /* 3.61V */
#define OCVREGB         0x07        /* 3.63V */
#define OCVREGC         0x0a        /* 3.64V */
#define OCVREGD         0x0d        /* 3.66V */
#define OCVREGE         0x1a        /* 3.70V */
#define OCVREGF         0x24        /* 3.73V */
#define OCVREG10        0x29        /* 3.77V */
#define OCVREG11        0x2e        /* 3.78V */
#define OCVREG12        0x32        /* 3.80V */
#define OCVREG13        0x35        /* 3.84V */
#define OCVREG14        0x39        /* 3.85V */
#define OCVREG15        0x3d        /* 3.87V */
#define OCVREG16        0x43        /* 3.91V */
#define OCVREG17        0x49        /* 3.94V */
#define OCVREG18        0x4f        /* 3.98V */
#define OCVREG19        0x54        /* 4.01V */
#define OCVREG1A        0x58        /* 4.05V */
#define OCVREG1B        0x5c        /* 4.08V */
#define OCVREG1C        0x5e        /* 4.10V */
#define OCVREG1D        0x60        /* 4.12V */
#define OCVREG1E        0x62        /* 4.14V */
#define OCVREG1F        0x64        /* 4.15V */

int axp_charger_dt_parse(struct device_node *node,
			struct axp_config_info *axp_config)
{
	if (!of_device_is_available(node)) {
		axp_config->pmu_used = 0;
		pr_err("%s: pmu_used = %u\n", __func__, axp_config->pmu_used);
		return -EPERM;
	} else {
		axp_config->pmu_used = 1;
	}

	if (of_property_read_u32(node, "pmu_battery_rdc",
		&axp_config->pmu_battery_rdc))
		axp_config->pmu_battery_rdc = BATRDC;

	if (of_property_read_u32(node, "pmu_battery_cap",
		&axp_config->pmu_battery_cap))
		axp_config->pmu_battery_cap = 4000;

	if (of_property_read_u32(node, "pmu_batdeten",
		&axp_config->pmu_batdeten))
		axp_config->pmu_batdeten = 1;

	if (of_property_read_u32(node, "pmu_chg_ic_temp",
		&axp_config->pmu_chg_ic_temp))
		axp_config->pmu_chg_ic_temp = 0;

	if (of_property_read_u32(node, "pmu_runtime_chgcur",
		&axp_config->pmu_runtime_chgcur))
		axp_config->pmu_runtime_chgcur = INTCHGCUR / 1000;
	axp_config->pmu_runtime_chgcur *= 1000;

	if (of_property_read_u32(node, "pmu_suspend_chgcur",
		&axp_config->pmu_suspend_chgcur))
		axp_config->pmu_suspend_chgcur = 1200;
	axp_config->pmu_suspend_chgcur *= 1000;

	if (of_property_read_u32(node, "pmu_shutdown_chgcur",
		&axp_config->pmu_shutdown_chgcur))
		axp_config->pmu_shutdown_chgcur = 1200;
	axp_config->pmu_shutdown_chgcur *= 1000;

	if (of_property_read_u32(node, "pmu_init_chgvol",
		&axp_config->pmu_init_chgvol))
		axp_config->pmu_init_chgvol = INTCHGVOL / 1000;
	axp_config->pmu_init_chgvol *= 1000;

	if (of_property_read_u32(node, "pmu_init_chgend_rate",
		&axp_config->pmu_init_chgend_rate))
		axp_config->pmu_init_chgend_rate = INTCHGENDRATE;

	if (of_property_read_u32(node, "pmu_init_chg_enabled",
		&axp_config->pmu_init_chg_enabled))
		axp_config->pmu_init_chg_enabled = 1;

	if (of_property_read_u32(node, "pmu_init_bc_en",
		&axp_config->pmu_init_bc_en))
		axp_config->pmu_init_bc_en = 0;

	if (of_property_read_u32(node, "pmu_init_adc_freq",
		&axp_config->pmu_init_adc_freq))
		axp_config->pmu_init_adc_freq = INTADCFREQ;

	if (of_property_read_u32(node, "pmu_init_adcts_freq",
		&axp_config->pmu_init_adcts_freq))
		axp_config->pmu_init_adcts_freq = INTADCFREQC;

	if (of_property_read_u32(node, "pmu_init_chg_pretime",
		&axp_config->pmu_init_chg_pretime))
		axp_config->pmu_init_chg_pretime = INTCHGPRETIME;

	if (of_property_read_u32(node, "pmu_init_chg_csttime",
		&axp_config->pmu_init_chg_csttime))
		axp_config->pmu_init_chg_csttime = INTCHGCSTTIME;

	if (of_property_read_u32(node, "pmu_batt_cap_correct",
		&axp_config->pmu_batt_cap_correct))
		axp_config->pmu_batt_cap_correct = 1;

	if (of_property_read_u32(node, "pmu_chg_end_on_en",
		&axp_config->pmu_chg_end_on_en))
		axp_config->pmu_chg_end_on_en = 0;

	if (of_property_read_u32(node, "ocv_coulumb_100",
		&axp_config->ocv_coulumb_100))
		axp_config->ocv_coulumb_100 = 0;

	if (of_property_read_u32(node, "pmu_bat_para1",
		&axp_config->pmu_bat_para1))
		axp_config->pmu_bat_para1 = OCVREG0;

	if (of_property_read_u32(node, "pmu_bat_para2",
		&axp_config->pmu_bat_para2))
		axp_config->pmu_bat_para2 = OCVREG1;

	if (of_property_read_u32(node, "pmu_bat_para3",
		&axp_config->pmu_bat_para3))
		axp_config->pmu_bat_para3 = OCVREG2;

	if (of_property_read_u32(node, "pmu_bat_para4",
		&axp_config->pmu_bat_para4))
		axp_config->pmu_bat_para4 = OCVREG3;

	if (of_property_read_u32(node, "pmu_bat_para5",
		&axp_config->pmu_bat_para5))
		axp_config->pmu_bat_para5 = OCVREG4;

	if (of_property_read_u32(node, "pmu_bat_para6",
		&axp_config->pmu_bat_para6))
		axp_config->pmu_bat_para6 = OCVREG5;

	if (of_property_read_u32(node, "pmu_bat_para7",
		&axp_config->pmu_bat_para7))
		axp_config->pmu_bat_para7 = OCVREG6;

	if (of_property_read_u32(node, "pmu_bat_para8",
		&axp_config->pmu_bat_para8))
		axp_config->pmu_bat_para8 = OCVREG7;

	if (of_property_read_u32(node, "pmu_bat_para9",
		&axp_config->pmu_bat_para9))
		axp_config->pmu_bat_para9 = OCVREG8;

	if (of_property_read_u32(node, "pmu_bat_para10",
		&axp_config->pmu_bat_para10))
		axp_config->pmu_bat_para10 = OCVREG9;

	if (of_property_read_u32(node, "pmu_bat_para11",
		&axp_config->pmu_bat_para11))
		axp_config->pmu_bat_para11 = OCVREGA;

	if (of_property_read_u32(node, "pmu_bat_para12",
		&axp_config->pmu_bat_para12))
		axp_config->pmu_bat_para12 = OCVREGB;

	if (of_property_read_u32(node, "pmu_bat_para13",
		&axp_config->pmu_bat_para13))
		axp_config->pmu_bat_para13 = OCVREGC;

	if (of_property_read_u32(node, "pmu_bat_para14",
		&axp_config->pmu_bat_para14))
		axp_config->pmu_bat_para14 = OCVREGD;

	if (of_property_read_u32(node, "pmu_bat_para15",
		&axp_config->pmu_bat_para15))
		axp_config->pmu_bat_para15 = OCVREGE;

	if (of_property_read_u32(node, "pmu_bat_para16",
		&axp_config->pmu_bat_para16))
		axp_config->pmu_bat_para16 = OCVREGF;

	//Add 32 Level OCV para 20121128 by evan
	if (of_property_read_u32(node, "pmu_bat_para17",
		&axp_config->pmu_bat_para17))
		axp_config->pmu_bat_para17 = OCVREG10;

	if (of_property_read_u32(node, "pmu_bat_para18",
		&axp_config->pmu_bat_para18))
		axp_config->pmu_bat_para18 = OCVREG11;

	if (of_property_read_u32(node, "pmu_bat_para19",
		&axp_config->pmu_bat_para19))
		axp_config->pmu_bat_para19 = OCVREG12;

	if (of_property_read_u32(node, "pmu_bat_para20",
		&axp_config->pmu_bat_para20))
		axp_config->pmu_bat_para20 = OCVREG13;

	if (of_property_read_u32(node, "pmu_bat_para21",
		&axp_config->pmu_bat_para21))
		axp_config->pmu_bat_para21 = OCVREG14;

	if (of_property_read_u32(node, "pmu_bat_para22",
		&axp_config->pmu_bat_para22))
		axp_config->pmu_bat_para22 = OCVREG15;

	if (of_property_read_u32(node, "pmu_bat_para23",
		&axp_config->pmu_bat_para23))
		axp_config->pmu_bat_para23 = OCVREG16;

	if (of_property_read_u32(node, "pmu_bat_para24",
		&axp_config->pmu_bat_para24))
		axp_config->pmu_bat_para24 = OCVREG17;

	if (of_property_read_u32(node, "pmu_bat_para25",
		&axp_config->pmu_bat_para25))
		axp_config->pmu_bat_para25 = OCVREG18;

	if (of_property_read_u32(node, "pmu_bat_para26",
		&axp_config->pmu_bat_para26))
		axp_config->pmu_bat_para26 = OCVREG19;

	if (of_property_read_u32(node, "pmu_bat_para27",
		&axp_config->pmu_bat_para27))
		axp_config->pmu_bat_para27 = OCVREG1A;

	if (of_property_read_u32(node, "pmu_bat_para28",
		&axp_config->pmu_bat_para28))
		axp_config->pmu_bat_para28 = OCVREG1B;

	if (of_property_read_u32(node, "pmu_bat_para29",
		&axp_config->pmu_bat_para29))
		axp_config->pmu_bat_para29 = OCVREG1C;

	if (of_property_read_u32(node, "pmu_bat_para30",
		&axp_config->pmu_bat_para30))
		axp_config->pmu_bat_para30 = OCVREG1D;

	if (of_property_read_u32(node, "pmu_bat_para31",
		&axp_config->pmu_bat_para31))
		axp_config->pmu_bat_para31 = OCVREG1E;

	if (of_property_read_u32(node, "pmu_bat_para32",
		&axp_config->pmu_bat_para32))
		axp_config->pmu_bat_para32 = OCVREG1F;

	if (of_property_read_u32(node, "pmu_ac_vol",
		&axp_config->pmu_ac_vol))
		axp_config->pmu_ac_vol = 4400;

	if (of_property_read_u32(node, "pmu_usbpc_vol",
		&axp_config->pmu_usbpc_vol))
		axp_config->pmu_usbpc_vol = 4400;

	if (of_property_read_u32(node, "pmu_ac_cur",
		&axp_config->pmu_ac_cur))
		axp_config->pmu_ac_cur = 0;

	if (of_property_read_u32(node, "pmu_usbpc_cur",
		&axp_config->pmu_usbpc_cur))
		axp_config->pmu_usbpc_cur = 0;

	if (of_property_read_u32(node, "pmu_pwroff_vol",
		&axp_config->pmu_pwroff_vol))
		axp_config->pmu_pwroff_vol = 3300;
	axp_config->pmu_pwroff_vol *= 1000;

	if (of_property_read_u32(node, "pmu_pwron_vol",
		&axp_config->pmu_pwron_vol))
		axp_config->pmu_pwron_vol = 2900;

	if (of_property_read_u32(node, "pmu_powkey_off_time",
		&axp_config->pmu_powkey_off_time))
		axp_config->pmu_powkey_off_time = 6000;

	//offlevel restart or not 0:not restart 1:restart
	if (of_property_read_u32(node, "pmu_powkey_off_func",
		&axp_config->pmu_powkey_off_func))
		axp_config->pmu_powkey_off_func = 0;

	//16's power restart or not 0:not restart 1:restart
	if (of_property_read_u32(node, "pmu_powkey_off_en",
		&axp_config->pmu_powkey_off_en))
		axp_config->pmu_powkey_off_en = 1;

	if (of_property_read_u32(node, "pmu_powkey_off_delay_time",
		&axp_config->pmu_powkey_off_delay_time))
		axp_config->pmu_powkey_off_delay_time = 0;

	if (of_property_read_u32(node, "pmu_powkey_long_time",
		&axp_config->pmu_powkey_long_time))
		axp_config->pmu_powkey_long_time = 1500;

	if (of_property_read_u32(node, "pmu_pwrok_time",
		&axp_config->pmu_pwrok_time))
		axp_config->pmu_pwrok_time    = 64;

	if (of_property_read_u32(node, "pmu_powkey_on_time",
		&axp_config->pmu_powkey_on_time))
		axp_config->pmu_powkey_on_time = 1000;

	if (of_property_read_u32(node, "pmu_reset_shutdown_en",
		&axp_config->pmu_reset_shutdown_en))
		axp_config->pmu_reset_shutdown_en = 0;

	if (of_property_read_u32(node, "pmu_battery_warning_level1",
		&axp_config->pmu_battery_warning_level1))
		axp_config->pmu_battery_warning_level1 = 15;

	if (of_property_read_u32(node, "pmu_battery_warning_level2",
		&axp_config->pmu_battery_warning_level2))
		axp_config->pmu_battery_warning_level2 = 0;

	if (of_property_read_u32(node, "pmu_restvol_adjust_time",
		&axp_config->pmu_restvol_adjust_time))
		axp_config->pmu_restvol_adjust_time = 30;

	if (of_property_read_u32(node, "pmu_ocv_cou_adjust_time",
		&axp_config->pmu_ocv_cou_adjust_time))
		axp_config->pmu_ocv_cou_adjust_time = 60;

	if (of_property_read_u32(node, "pmu_chgled_func",
		&axp_config->pmu_chgled_func))
		axp_config->pmu_chgled_func = 0;

	if (of_property_read_u32(node, "pmu_chgled_type",
		&axp_config->pmu_chgled_type))
		axp_config->pmu_chgled_type = 0;

	if (of_property_read_u32(node, "pmu_vbusen_func",
		&axp_config->pmu_vbusen_func))
		axp_config->pmu_vbusen_func = 1;

	if (of_property_read_u32(node, "pmu_reset",
		&axp_config->pmu_reset))
		axp_config->pmu_reset = 0;

	if (of_property_read_u32(node, "pmu_irq_wakeup",
			&axp_config->pmu_irq_wakeup))
		axp_config->pmu_irq_wakeup = 0;

	if (of_property_read_u32(node, "pmu_hot_shutdown",
		&axp_config->pmu_hot_shutdown))
		axp_config->pmu_hot_shutdown = 1;

	if (of_property_read_u32(node, "pmu_inshort",
		&axp_config->pmu_inshort))
		axp_config->pmu_inshort = 0;

	if (of_property_read_u32(node, "power_start",
		&axp_config->power_start))
		axp_config->power_start = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_enable",
		&axp_config->pmu_bat_temp_enable))
		axp_config->pmu_bat_temp_enable = 0;

	if (of_property_read_u32(node, "pmu_bat_charge_ltf",
		&axp_config->pmu_bat_charge_ltf))
		axp_config->pmu_bat_charge_ltf = 0xA5;

	if (of_property_read_u32(node, "pmu_bat_charge_htf",
		&axp_config->pmu_bat_charge_htf))
		axp_config->pmu_bat_charge_htf = 0x1F;

	if (of_property_read_u32(node, "pmu_bat_shutdown_ltf",
		&axp_config->pmu_bat_shutdown_ltf))
		axp_config->pmu_bat_shutdown_ltf = 0xFC;

	if (of_property_read_u32(node, "pmu_bat_shutdown_htf",
		&axp_config->pmu_bat_shutdown_htf))
		axp_config->pmu_bat_shutdown_htf = 0x16;

	if (of_property_read_u32(node, "pmu_bat_temp_para1",
		&axp_config->pmu_bat_temp_para1))
		axp_config->pmu_bat_temp_para1 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para2",
		&axp_config->pmu_bat_temp_para2))
		axp_config->pmu_bat_temp_para2 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para3",
		&axp_config->pmu_bat_temp_para3))
		axp_config->pmu_bat_temp_para3 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para4",
		&axp_config->pmu_bat_temp_para4))
		axp_config->pmu_bat_temp_para4 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para5",
		&axp_config->pmu_bat_temp_para5))
		axp_config->pmu_bat_temp_para5 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para6",
		&axp_config->pmu_bat_temp_para6))
		axp_config->pmu_bat_temp_para6 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para7",
		&axp_config->pmu_bat_temp_para7))
		axp_config->pmu_bat_temp_para7 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para8",
		&axp_config->pmu_bat_temp_para8))
		axp_config->pmu_bat_temp_para8 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para9",
		&axp_config->pmu_bat_temp_para9))
		axp_config->pmu_bat_temp_para9 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para10",
		&axp_config->pmu_bat_temp_para10))
		axp_config->pmu_bat_temp_para10 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para11",
		&axp_config->pmu_bat_temp_para11))
		axp_config->pmu_bat_temp_para11 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para12",
		&axp_config->pmu_bat_temp_para12))
		axp_config->pmu_bat_temp_para12 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para13",
		&axp_config->pmu_bat_temp_para13))
		axp_config->pmu_bat_temp_para13 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para14",
		&axp_config->pmu_bat_temp_para14))
		axp_config->pmu_bat_temp_para14 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para15",
		&axp_config->pmu_bat_temp_para15))
		axp_config->pmu_bat_temp_para15 = 0;

	if (of_property_read_u32(node, "pmu_bat_temp_para16",
		&axp_config->pmu_bat_temp_para16))
		axp_config->pmu_bat_temp_para16 = 0;

	return 0;
}
EXPORT_SYMBOL_GPL(axp_charger_dt_parse);
