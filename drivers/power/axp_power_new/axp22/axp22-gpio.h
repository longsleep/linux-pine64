#ifndef AXP22_GPIO_H
#define AXP22_GPIO_H
#include "axp22.h"

#define AXP_GPIO0_CFG        (AXP22_GPIO0_CTL)     /* 0x90 */
#define AXP_GPIO1_CFG        (AXP22_GPIO1_CTL)     /* 0x92 */
#define AXP_GPIO2_CFG        (AXP22_HOTOVER_CTL)   /* 0x8F */
#define AXP_GPIO2_STA        (AXP22_IPS_SET)       /* 0x30 */
#define AXP_GPIO01_STATE     (AXP22_GPIO01_SIGNAL) /* 0x94 */

#endif
