#ifndef _LINUX_AXP20_REGU_H_
#define _LINUX_AXP20_REGU_H_

#include <linux/regulator/driver.h>
#include <linux/mfd/axp-mfd.h>
#include <linux/regulator/machine.h>

/* AXP20 Regulator Registers */
#define AXP20_LDO1      POWER20_STATUS
#define AXP20_LDO2      POWER20_LDO24OUT_VOL
#define AXP20_LDO3      POWER20_LDO3OUT_VOL
#define AXP20_LDO4      POWER20_LDO24OUT_VOL
#define AXP20_BUCK2     POWER20_DC2OUT_VOL
#define AXP20_BUCK3     POWER20_DC3OUT_VOL
#define AXP20_LDOIO0    POWER20_GPIO0_VOL

#define AXP20_LDO1EN    POWER20_STATUS
#define AXP20_LDO2EN    POWER20_LDO234_DC23_CTL
#define AXP20_LDO3EN    POWER20_LDO234_DC23_CTL
#define AXP20_LDO4EN    POWER20_LDO234_DC23_CTL
#define AXP20_BUCK2EN   POWER20_LDO234_DC23_CTL
#define AXP20_BUCK3EN   POWER20_LDO234_DC23_CTL
#define AXP20_LDOIOEN   POWER20_GPIO0_CTL


#define AXP20_BUCKMODE  POWER20_DCDC_MODESET
#define AXP20_BUCKFREQ  POWER20_DCDC_FREQSET


struct axp_reg_init {
	struct regulator_init_data axp_reg_init_data;
	struct axp_regulator_info *info;
};

/* The values of the various regulator constraints are obviously dependent
 * on exactly what is wired to each ldo.  Unfortunately this information is
 * not generally available.  More information has been requested from Xbow
 * but as of yet they haven't been forthcoming.
 *
 * Some of these are clearly Stargate 2 related (no way of plugging
 * in an lcd on the IM2 for example!).
 */

struct axp_consumer_supply {
	char supply[20];    /* consumer supply - e.g. "vcc" */
};

#define AXP_LDO(_pmic, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2, new_level, mode_addr, freq_addr) \
{                                   \
	.desc   = {                         \
		.name   = #_pmic"_LDO" #_id,                    \
		.type   = REGULATOR_VOLTAGE,                \
		.id = _pmic##_ID_LDO##_id,              \
		.n_voltages = (step1) ? ( (switch_vol) ? ((switch_vol - min) / step1 +  \
				(max - switch_vol) / step2  +1):    \
				((max - min) / step1 +1) ): 1,      \
		.owner  = THIS_MODULE,                  \
	},                              \
	.min_uV     = (min) * 1000,                 \
	.max_uV     = (max) * 1000,                 \
	.step1_uV   = (step1) * 1000,               \
	.vol_reg    = _pmic##_##vreg,               \
	.vol_shift  = (shift),                  \
	.vol_nbits  = (nbits),                  \
	.enable_reg = _pmic##_##ereg,               \
	.enable_bit = (ebit),                   \
	.switch_uV  = (switch_vol)*1000,                \
	.step2_uV   = (step2)*1000,                 \
	.new_level_uV   = (new_level)*1000,             \
}

#define AXP_BUCK(_pmic, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2, new_level, mode_addr, freq_addr) \
{                                   \
	.desc   = {                         \
		.name   = #_pmic"_BUCK" #_id,                   \
		.type   = REGULATOR_VOLTAGE,                \
		.id = _pmic##_ID_BUCK##_id,             \
		.n_voltages = (step1) ? ( (switch_vol) ? ((new_level)?((switch_vol - min) / step1 + \
				(max - new_level) / step2  +2) : ((switch_vol - min) / step1 +  \
				(max - switch_vol) / step2  +1)):   \
				((max - min) / step1 +1) ): 1,      \
	},                              \
	.min_uV     = (min) * 1000,                 \
	.max_uV     = (max) * 1000,                 \
	.step1_uV   = (step1) * 1000,               \
	.vol_reg    = _pmic##_##vreg,               \
	.vol_shift  = (shift),                  \
	.vol_nbits  = (nbits),                  \
	.enable_reg = _pmic##_##ereg,               \
	.enable_bit = (ebit),                   \
	.switch_uV  = (switch_vol)*1000,                \
	.step2_uV   = (step2)*1000,                 \
	.new_level_uV   = (new_level)*1000,             \
	.mode_reg   = mode_addr,                    \
	.freq_reg   = freq_addr,                    \
}

#define AXP_DCDC(_pmic, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2, new_level, mode_addr, freq_addr) \
{                                   \
	.desc   = {                         \
		.name   = #_pmic"_DCDC" #_id,                   \
		.type   = REGULATOR_VOLTAGE,                \
		.id = _pmic##_ID_DCDC##_id,             \
		.n_voltages = (step1) ? ( (switch_vol) ? ((new_level)?((switch_vol - min) / step1 + \
				(max - new_level) / step2  +2) : ((switch_vol - min) / step1 +  \
				(max - switch_vol) / step2  +1)):   \
				((max - min) / step1 +1) ): 1,      \
		.owner  = THIS_MODULE,                  \
	},                              \
	.min_uV     = (min) * 1000,                 \
	.max_uV     = (max) * 1000,                 \
	.step1_uV   = (step1) * 1000,               \
	.vol_reg    = _pmic##_##vreg,               \
	.vol_shift  = (shift),                  \
	.vol_nbits  = (nbits),                  \
	.enable_reg = _pmic##_##ereg,               \
	.enable_bit = (ebit),                   \
	.switch_uV  = (switch_vol)*1000,                \
	.step2_uV   = (step2)*1000,                 \
	.new_level_uV   = (new_level)*1000,             \
	.mode_reg   = mode_addr,                    \
	.freq_reg   = freq_addr,                    \
}

#define AXP_SW(_pmic, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2, new_level, mode_addr, freq_addr) \
{                                   \
	.desc   = {                         \
		.name   = #_pmic"_SW" #_id,                 \
		.type   = REGULATOR_VOLTAGE,                \
		.id = _pmic##_ID_SW##_id,               \
		.n_voltages = (step1) ? ( (switch_vol) ? ((new_level)?((switch_vol - min) / step1 + \
				(max - new_level) / step2  +2) : ((switch_vol - min) / step1 +  \
				(max - switch_vol) / step2  +1)):   \
				((max - min) / step1 +1) ): 1,      \
		.owner  = THIS_MODULE,                  \
	},                              \
	.min_uV     = (min) * 1000,                 \
	.max_uV     = (max) * 1000,                 \
	.step1_uV   = (step1) * 1000,               \
	.vol_reg    = _pmic##_##vreg,               \
	.vol_shift  = (shift),                  \
	.vol_nbits  = (nbits),                  \
	.enable_reg = _pmic##_##ereg,               \
	.enable_bit = (ebit),                   \
	.switch_uV  = (switch_vol)*1000,                \
	.step2_uV   = (step2)*1000,                 \
	.new_level_uV   = (new_level)*1000,             \
	.mode_reg   = mode_addr,                    \
	.freq_reg   = freq_addr,                    \
}

#define AXP_REGU_ATTR(_name)                        \
{                                   \
	.attr = { .name = #_name,.mode = 0644 },            \
	.show =  _name##_show,                      \
	.store = _name##_store,                     \
}

struct axp_regulator_info {
	struct regulator_desc desc;
	int min_uV;
	int max_uV;
	int step1_uV;
	int vol_reg;
	int vol_shift;
	int vol_nbits;
	int enable_reg;
	int enable_bit;
	int switch_uV;
	int step2_uV;
	int new_level_uV;
	int mode_reg;
	int freq_reg;
};

extern struct axp_funcdev_info *axp20_regu_init(void);
extern s32 axp_regu_device_tree_parse(char * pmu_type, struct axp_reg_init *axp_init_data);
#endif
