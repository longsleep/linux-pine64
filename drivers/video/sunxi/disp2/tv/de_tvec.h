#ifndef __DE_TVE_H__
#define __DE_TVE_H__

#define TVE_GET_REG_BASE(sel)                   (tve_reg_base[sel])
#define TVE_WUINT32(sel,offset,value)           (*((volatile u32 *)( TVE_GET_REG_BASE(sel) + (offset) ))=(value))
#define TVE_RUINT32(sel,offset)                 (*((volatile u32 *)( TVE_GET_REG_BASE(sel) + (offset) )))
#define TVE_SET_BIT(sel,offset,bit)             (*((volatile u32 *)( TVE_GET_REG_BASE(sel) + (offset) )) |= (bit))
#define TVE_CLR_BIT(sel,offset,bit)             (*((volatile u32 *)( TVE_GET_REG_BASE(sel) + (offset) )) &= (~(bit)))
#define TVE_INIT_BIT(sel,offset,c,s)            (*((volatile u32 *)( TVE_GET_REG_BASE(sel) + (offset) )) = \
                                                (((*(volatile u32 *)( TVE_GET_REG_BASE(sel) + (offset) )) & (~(c))) | (s)))

s32 tve_low_set_reg_base(u32 sel,void __iomem * address);
s32 tve_low_init(u32 sel,u32 cali,s32 offset);
s32 tve_low_exit(u32 sel);
s32 tve_low_open(u32 sel);
s32 tve_low_close(u32 sel);
s32 tve_resync_enable(u32 sel);
s32 tve_resync_disable(u32 sel);
s32 tve_low_set_tv_mode(u32 sel, u8 mode, u32 cali);
s32 tve_low_set_vga_mode(u32 sel);
s32 tve_low_get_dac_status(u32 sel);
s32 tve_low_dac_autocheck_enable(u32 sel, u8 index);
s32 tve_low_dac_autocheck_disable(u32 sel,u8 index);
s32 tve_low_dac_auto_cali(u32 sel, u32 cali);
s32 tve_low_enhance(u32 sel, u32 type);
u32 tve_low_get_sid(u32 index);

#endif
