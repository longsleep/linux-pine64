#ifndef AXP_GPIO_H_
#define AXP_GPIO_H_

#include <linux/kernel.h>
#include <linux/pinctrl/pinconf-sunxi.h>

#define AXP_PINCTRL_GPIO(_num)  PINCTRL_PIN(_num, "GPIO"#_num)

/*
 * GPIO Registers.
 */

#define AXP_PIN_NAME_MAX_LEN    (8)

struct axp_desc_function {
	const char *name;
	u8         muxval;
};

struct axp_desc_pin {
	struct pinctrl_pin_desc  pin;
	struct axp_desc_function *functions;
};

struct axp_pinctrl_desc {
	const struct axp_desc_pin *pins;
	int           npins;
};

struct axp_pinctrl_function {
	const char  *name;
	const char  **groups;
	unsigned    ngroups;
};

struct axp_pinctrl_group {
	const char  *name;
	unsigned long   config;
	unsigned    pin;
};

struct axp_gpio_ops {
	int (*gpio_get_data)(struct axp_dev *axp_dev, int gpio);
	int (*gpio_set_data)(struct axp_dev *axp_dev, int gpio, int value);
	int (*pmx_set)(struct axp_dev *axp_dev, int gpio, int mux);
	int (*pmx_get)(struct axp_dev *axp_dev, int gpio);
};

struct axp_pinctrl {
	struct device                *dev;
	struct pinctrl_dev           *pctl_dev;
	struct gpio_chip             gpio_chip;
	struct axp_dev               *axp_dev;
	struct axp_gpio_ops          *ops;
	struct axp_pinctrl_desc      *desc;
	struct axp_pinctrl_function  *functions;
	unsigned                     nfunctions;
	struct axp_pinctrl_group     *groups;
	unsigned                     ngroups;
};

#define AXP_PIN_DESC(_pin, ...)         \
	{                                   \
		.pin = _pin,                    \
		.functions = (struct axp_desc_function[]){  \
			__VA_ARGS__, { } },         \
	}

#define AXP_FUNCTION(_val, _name)       \
	{                           \
		.name = _name,          \
		.muxval = _val,         \
	}

struct axp_pinctrl *axp_pinctrl_register(struct device *dev,
			struct axp_dev *axp_dev,
			struct axp_pinctrl_desc *desc,
			struct axp_gpio_ops *ops);
int axp_pinctrl_unregister(struct axp_pinctrl *pctl);

extern s32 axp_debug;

#endif
