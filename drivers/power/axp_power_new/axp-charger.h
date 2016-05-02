#ifndef AXP_CHARGER_H
#define AXP_CHARGER_H

struct axp_charger_dev;

enum AW_CHARGE_TYPE {
	CHARGE_AC,
	CHARGE_USB_20,
	CHARGE_USB_30,
	CHARGE_MAX
};

struct axp_config_info {
	u32 pmu_used;
	u32 pmu_battery_rdc;
	u32 pmu_battery_cap;
	u32 pmu_batdeten;
	u32 pmu_chg_ic_temp;
	u32 pmu_runtime_chgcur;
	u32 pmu_suspend_chgcur;
	u32 pmu_shutdown_chgcur;
	u32 pmu_init_chgvol;
	u32 pmu_init_chgend_rate;
	u32 pmu_init_chg_enabled;
	u32 pmu_init_bc_en;
	u32 pmu_init_adc_freq;
	u32 pmu_init_adcts_freq;
	u32 pmu_init_chg_pretime;
	u32 pmu_init_chg_csttime;
	u32 pmu_batt_cap_correct;
	u32 pmu_chg_end_on_en;
	u32 ocv_coulumb_100;

	u32 pmu_bat_para1;
	u32 pmu_bat_para2;
	u32 pmu_bat_para3;
	u32 pmu_bat_para4;
	u32 pmu_bat_para5;
	u32 pmu_bat_para6;
	u32 pmu_bat_para7;
	u32 pmu_bat_para8;
	u32 pmu_bat_para9;
	u32 pmu_bat_para10;
	u32 pmu_bat_para11;
	u32 pmu_bat_para12;
	u32 pmu_bat_para13;
	u32 pmu_bat_para14;
	u32 pmu_bat_para15;
	u32 pmu_bat_para16;
	u32 pmu_bat_para17;
	u32 pmu_bat_para18;
	u32 pmu_bat_para19;
	u32 pmu_bat_para20;
	u32 pmu_bat_para21;
	u32 pmu_bat_para22;
	u32 pmu_bat_para23;
	u32 pmu_bat_para24;
	u32 pmu_bat_para25;
	u32 pmu_bat_para26;
	u32 pmu_bat_para27;
	u32 pmu_bat_para28;
	u32 pmu_bat_para29;
	u32 pmu_bat_para30;
	u32 pmu_bat_para31;
	u32 pmu_bat_para32;

	u32 pmu_ac_vol;
	u32 pmu_ac_cur;
	u32 pmu_usbpc_vol;
	u32 pmu_usbpc_cur;
	u32 pmu_pwroff_vol;
	u32 pmu_pwron_vol;
	u32 pmu_powkey_off_time;
	u32 pmu_powkey_off_en;
	u32 pmu_powkey_off_delay_time;
	u32 pmu_powkey_off_func;
	u32 pmu_powkey_long_time;
	u32 pmu_powkey_on_time;
	u32 pmu_pwrok_time;
	u32 pmu_reset_shutdown_en;
	u32 pmu_battery_warning_level1;
	u32 pmu_battery_warning_level2;
	u32 pmu_restvol_adjust_time;
	u32 pmu_ocv_cou_adjust_time;
	u32 pmu_chgled_func;
	u32 pmu_chgled_type;
	u32 pmu_vbusen_func;
	u32 pmu_reset;
	u32 pmu_irq_wakeup;
	u32 pmu_hot_shutdown;
	u32 pmu_inshort;
	u32 power_start;

	u32 pmu_bat_temp_enable;
	u32 pmu_bat_charge_ltf;
	u32 pmu_bat_charge_htf;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;
	u32 pmu_bat_temp_para1;
	u32 pmu_bat_temp_para2;
	u32 pmu_bat_temp_para3;
	u32 pmu_bat_temp_para4;
	u32 pmu_bat_temp_para5;
	u32 pmu_bat_temp_para6;
	u32 pmu_bat_temp_para7;
	u32 pmu_bat_temp_para8;
	u32 pmu_bat_temp_para9;
	u32 pmu_bat_temp_para10;
	u32 pmu_bat_temp_para11;
	u32 pmu_bat_temp_para12;
	u32 pmu_bat_temp_para13;
	u32 pmu_bat_temp_para14;
	u32 pmu_bat_temp_para15;
	u32 pmu_bat_temp_para16;
};

struct axp_ac_info {
	int det_bit;    /* ac detect */
	int det_offset;
	int valid_bit;  /* ac vali */
	int valid_offset;
	int in_short_bit;
	int in_short_offset;
	int ac_vol;
	int ac_cur;
	int (*get_ac_voltage)(struct axp_charger_dev *cdev);
	int (*get_ac_current)(struct axp_charger_dev *cdev);
	int (*set_ac_vhold)(struct axp_charger_dev *cdev, int vol);
	int (*get_ac_vhold)(struct axp_charger_dev *cdev);
	int (*set_ac_ihold)(struct axp_charger_dev *cdev, int cur);
	int (*get_ac_ihold)(struct axp_charger_dev *cdev);
};

struct axp_usb_info {
	int det_bit;
	int det_offset;
	int valid_bit;
	int valid_offset;
	int usb_pc_vol;
	int usb_pc_cur;
	int usb_ad_vol;
	int usb_ad_cur;
	int (*get_usb_voltage)(struct axp_charger_dev *cdev);
	int (*get_usb_current)(struct axp_charger_dev *cdev);
	int (*set_usb_vhold)(struct axp_charger_dev *cdev, int vol);
	int (*get_usb_vhold)(struct axp_charger_dev *cdev);
	int (*set_usb_ihold)(struct axp_charger_dev *cdev, int cur);
	int (*get_usb_ihold)(struct axp_charger_dev *cdev);
};

struct axp_battery_info {
	int chgstat_bit;
	int chgstat_offset;
	int det_bit;
	int det_offset;
	int cur_direction_bit;
	int cur_direction_offset;
	int polling_delay;
	int runtime_chgcur;
	int suspend_chgcur;
	int shutdown_chgcur;
	int (*get_rest_cap)(struct axp_charger_dev *cdev);
	int (*get_bat_health)(struct axp_charger_dev *cdev);
	int (*get_vbat)(struct axp_charger_dev *cdev);
	int (*get_ibat)(struct axp_charger_dev *cdev);
	int (*set_chg_cur)(struct axp_charger_dev *cdev, int cur);
	int (*set_chg_vol)(struct axp_charger_dev *cdev, int vol);
	int (*pre_time_set)(struct axp_charger_dev *cdev,int min);
	int (*pos_time_set)(struct axp_charger_dev *cdev,int min);
};

struct axp_supply_info {
	struct axp_ac_info *ac;
	struct axp_usb_info *usb;
	struct axp_battery_info *batt;
};

struct axp_charger_dev {
	struct power_supply batt;
	struct power_supply ac;
	struct power_supply usb;
	struct power_supply_info *battery_info;
	struct axp_supply_info *spy_info;
	struct device *dev;
	struct axp_dev *chip;
	struct timer_list usb_status_timer;
	struct delayed_work work;
	struct delayed_work usbwork;
	unsigned int interval;
	spinlock_t charger_lock;

	int rest_vol;
	int usb_vol;
	int usb_cur;
	int ac_vol;
	int ac_cur;
	int bat_vol;
	int bat_cur;
	int bat_discur;

	bool bat_det;
	bool ac_det;
	bool usb_det;
	bool ac_valid;
	bool usb_valid;
	bool ext_valid;
	bool in_short;
	bool charging;
	bool ac_charging;
	bool usb_pc_charging;
	bool usb_adapter_charging;
	bool bat_current_direction;
};

struct axp_charger_dev *axp_power_supply_register(struct device *dev,
					struct axp_dev *axp_dev,
					struct power_supply_info *battery_info,
					struct axp_supply_info *info);
void axp_power_supply_unregister(struct axp_charger_dev *chg_dev);
void axp_change(struct axp_charger_dev *chg_dev);
void axp_usbac_in(struct axp_charger_dev *chg_dev);
void axp_usbac_out(struct axp_charger_dev *chg_dev);
void axp_capchange(struct axp_charger_dev *chg_dev);
void axp_charger_suspend(struct axp_charger_dev *chg_dev);
void axp_charger_resume(struct axp_charger_dev *chg_dev);
void axp_charger_shutdown(struct axp_charger_dev *chg_dev);
int axp_charger_dt_parse(struct device_node *node, struct axp_config_info *axp_config);

extern int axp_debug;
#endif /* AXP_ChARGER_H */
