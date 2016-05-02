#ifndef AXP_CORE_H_
#define AXP_CORE_H_
#include <linux/interrupt.h>

#define AXP_REG_WIDTH     (8)
#define AXP_ADD_WIDTH     (8)
#define ABS(x)		((x) >0 ? (x) : -(x) )

enum axp_regmap_type{
	AXP_REGMAP_I2C,
	AXP_REGMAP_RSB,
};

struct axp_regmap{
	enum axp_regmap_type type;
	struct i2c_client    *client;
	struct mutex         lock;
	spinlock_t           spinlock;
	u8                   rsbaddr;
};

struct axp_regmap_irq {
	irq_handler_t handler;
	void *data;
};

struct axp_regmap_irq_chip {
	const char *name;
	unsigned int status_base;
	unsigned int enable_base;
	int num_regs;
};

struct axp_irq_chip_data {
	struct mutex lock;
	struct work_struct irq_work;
	struct axp_regmap *map;
	struct axp_regmap_irq_chip *chip;
	struct axp_regmap_irq *irqs;
	int num_irqs;
};

struct axp_dev {
	struct device            *dev;
	struct axp_regmap        *regmap;
	int                      nr_cells;
	struct mfd_cell          *cells;
	struct axp_irq_chip_data *irq_data;
};

#define AXP_DUMP_ATTR(_name)				\
{							\
	.attr = { .name = #_name,.mode = 0644 },	\
	.show =  _name##_show,				\
	.store = _name##_store,				\
}

enum {
	DEBUG_SPLY = 1U << 0,
	DEBUG_REGU = 1U << 1,
	DEBUG_INT  = 1U << 2,
	DEBUG_CHG  = 1U << 3,
};

#define DBG_PSY_MSG(level_mask, fmt, arg...)	if (unlikely(axp_debug & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

struct axp_regmap *axp_regmap_init_i2c(struct device *dev);
struct axp_regmap *axp_regmap_init_rsb(struct device *dev, u8 addr);
struct axp_irq_chip_data *axp_irq_chip_register(struct axp_regmap *map,
	int irq, int irq_flags, struct axp_regmap_irq_chip *irq_chip);
void axp_irq_chip_unregister(int irq, struct axp_irq_chip_data *irq_data);

int axp_regmap_write(struct axp_regmap *map, s32 reg, u8 val);
int axp_regmap_writes(struct axp_regmap *map, s32 reg, s32 len, u8 *val);
int axp_regmap_read(struct axp_regmap *map, s32 reg, u8 *val);
int axp_regmap_reads(struct axp_regmap *map, s32 reg, s32 len, u8 *val);
int axp_regmap_update(struct axp_regmap *map, s32 reg, u8 val, u8 mask);
int axp_regmap_set_bits(struct axp_regmap *map, s32 reg, u8 bit_mask);
int axp_regmap_clr_bits(struct axp_regmap *map, s32 reg, u8 bit_mask);
int axp_regmap_update_sync(struct axp_regmap *map, s32 reg, u8 val, u8 mask);
int axp_regmap_set_bits_sync(struct axp_regmap *map, s32 reg, u8 bit_mask);
int axp_regmap_clr_bits_sync(struct axp_regmap *map, s32 reg, u8 bit_mask);
int axp_mfd_add_devices(struct axp_dev *axp_dev);
int axp_mfd_remove_devices(struct axp_dev *axp_dev);
int axp_request_irq(struct axp_dev *adev, int irq_no, irq_handler_t handler, void *data);
int axp_free_irq(struct axp_dev *adev, int irq_no);

#ifdef CONFIG_AXP_NMI_USED
extern void clear_nmi_status(void);
extern void disable_nmi(void);
extern void enable_nmi(void);
extern void set_nmi_trigger(u32 trigger);
#endif

#endif /* AXP_CORE_H_ */
