#include "drv_tv.h"
#include "de_tvec.h"

#define TVE_DEVICE_NUM 2
void __iomem * tve_reg_base[TVE_DEVICE_NUM] = {NULL};

/*tv encoder registers offset*/
#define TVE_000    (0x000)
#define TVE_004    (0x004)
#define TVE_008    (0x008)
#define TVE_00C    (0x00c)
#define TVE_010    (0x010)
#define TVE_014    (0x014)
#define TVE_018    (0x018)
#define TVE_01C    (0x01c)
#define TVE_020    (0x020)
#define TVE_024    (0x024)
#define TVE_030    (0X030)
#define TVE_034    (0x034)
#define TVE_038    (0x038)
#define TVE_03C    (0x03c)
#define TVE_040    (0x040)
#define TVE_044    (0x044)
#define TVE_048    (0x048)
#define TVE_04C    (0x04c)
#define TVE_0F8    (0x0f8)
#define TVE_0FC    (0x0fc)
#define TVE_100    (0x100)
#define TVE_104    (0x104)
#define TVE_108    (0x108)
#define TVE_10C    (0x10c)
#define TVE_110    (0x110)
#define TVE_114    (0x114)
#define TVE_118    (0x118)
#define TVE_11C    (0x11c)
#define TVE_120    (0x120)
#define TVE_124    (0x124)
#define TVE_128    (0x128)
#define TVE_12C    (0x12c)
#define TVE_130    (0x130)
#define TVE_134    (0x134)
#define TVE_138    (0x138)
#define TVE_13C    (0x13C)
#define TVE_300    (0x300)
#define TVE_304    (0x304)
#define TVE_308    (0x308)
#define TVE_3A0    (0x3a0)

s32 tve_low_set_reg_base(u32 sel,void __iomem * address)
{
	tve_reg_base[sel] = address;

	return 0;
}

//tve
// init module
////////////////////////////////////////////////////////////////////////////////

s32  tve_low_init(u32 sel,u32 cali,s32 offset)
{
	u32 dac_set= 784*1100/(1000+30+offset);
	TVE_WUINT32(sel, TVE_304,cali<<16 | 0xc<<8);
	TVE_WUINT32(sel, TVE_300,dac_set<<16);
	TVE_WUINT32(sel, TVE_008,0x433e12b1);
	TVE_WUINT32(sel, TVE_000,0x80000000);
	return 0;
}

s32 tve_low_exit(u32 sel)
{
	return 0;
}

// open module
////////////////////////////////////////////////////////////////////////////////

s32 tve_low_open(u32 sel)
{
	TVE_CLR_BIT(sel,TVE_000,0x1<<31);
	TVE_SET_BIT(sel,TVE_000,0x1<<0);
	TVE_SET_BIT(sel,TVE_008,0x1<<0);
	return 0;
}

s32 tve_low_close(u32 sel)
{
	TVE_CLR_BIT(sel,TVE_000, 0x1<<0 );
	//TVE_CLR_BIT(sel,TVE_008, 0x1<<0 );
	TVE_SET_BIT(sel,TVE_000, 0x1<<31);
	return 0;
}

s32 tve_resync_enable(u32 sel)
{
	TVE_SET_BIT(sel,TVE_130,0x1<<30);
    return 0;
}

s32 tve_resync_disable(u32 sel)
{
	TVE_CLR_BIT(sel,TVE_130,0x1<<30);
    return 0;
}

s32 tve_low_set_tv_mode(u32 sel, u8 mode, u32 cali)
{
	tve_low_dac_auto_cali(sel, cali);
	if(14==mode)
	{
		TVE_WUINT32(sel,TVE_000,0x00000300);
		TVE_WUINT32(sel,TVE_004,0x07070000);
		TVE_WUINT32(sel,TVE_00C,0x30001400);
		TVE_WUINT32(sel,TVE_010,0x21F07C1F);
		TVE_WUINT32(sel,TVE_014,0x00760020);
		TVE_WUINT32(sel,TVE_018,0x00000016);
		TVE_WUINT32(sel,TVE_01C,0x0016020D);
		TVE_WUINT32(sel,TVE_020,0x00F0011A);
		TVE_WUINT32(sel,TVE_100,0x00000001);
		TVE_WUINT32(sel,TVE_104,0x00000000);
		TVE_WUINT32(sel,TVE_108,0x00000002);
		TVE_WUINT32(sel,TVE_10C,0x0000004F);
		TVE_WUINT32(sel,TVE_110,0x00000000);
		TVE_WUINT32(sel,TVE_114,0x0016447E);
		TVE_WUINT32(sel,TVE_118,0x0000A0A0);
		TVE_WUINT32(sel,TVE_11C,0x001000F0);
		TVE_WUINT32(sel,TVE_120,0x01E80320);
		TVE_WUINT32(sel,TVE_124,0x000005A0);
		TVE_WUINT32(sel,TVE_128,0x00010000);
		TVE_WUINT32(sel,TVE_12C,0x00000101);
		TVE_WUINT32(sel,TVE_130,0x20050368);
		TVE_WUINT32(sel,TVE_134,0x00000000);
		TVE_WUINT32(sel,TVE_138,0x00000000);
		TVE_WUINT32(sel,TVE_13C,0x00000000);
	}
	else
	{
		TVE_WUINT32(sel,TVE_000,0x00000300);
		TVE_WUINT32(sel,TVE_004,0x07070001);
		TVE_WUINT32(sel,TVE_00C,0x30001400);
		TVE_WUINT32(sel,TVE_010,0x2A098ACB);
		TVE_WUINT32(sel,TVE_014,0x008A0018);
		TVE_WUINT32(sel,TVE_018,0x00000016);
		TVE_WUINT32(sel,TVE_01C,0x00160271);
		TVE_WUINT32(sel,TVE_020,0x00FC00FC);
		TVE_WUINT32(sel,TVE_100,0x00000000);
		TVE_WUINT32(sel,TVE_104,0x00000001);
		TVE_WUINT32(sel,TVE_108,0x00000005);
		TVE_WUINT32(sel,TVE_10C,0x00002929);
		TVE_WUINT32(sel,TVE_110,0x00000000);
		TVE_WUINT32(sel,TVE_114,0x0016447E);
		TVE_WUINT32(sel,TVE_118,0x0000A8A8);
		TVE_WUINT32(sel,TVE_11C,0x001000FC);
		TVE_WUINT32(sel,TVE_120,0x01E80320);
		TVE_WUINT32(sel,TVE_124,0x000005A0);
		TVE_WUINT32(sel,TVE_128,0x00010000);
		TVE_WUINT32(sel,TVE_12C,0x00000101);
		TVE_WUINT32(sel,TVE_130,0x2005000A);
		TVE_WUINT32(sel,TVE_134,0x00000000);
		TVE_WUINT32(sel,TVE_138,0x00000000);
		TVE_WUINT32(sel,TVE_13C,0x00000000);
		TVE_WUINT32(sel,TVE_3A0,0x00030001);
	}

	return 0;
}

s32 tve_low_set_vga_mode(u32 sel)
{
	return 0;
}

//0:unconnected; 1:connected; 3:short to ground
s32 tve_low_get_dac_status(u32 sel)  //index modify to sel
{
	u32   readval;
	readval = TVE_RUINT32(sel,TVE_038);
	return (readval & 0x3);
}

s32 tve_low_dac_autocheck_enable(u32 sel, u8 index)
{
	TVE_WUINT32(sel, TVE_0F8,0x00000280);
	TVE_WUINT32(sel, TVE_0FC,0x028F00FF);		//20ms x 10
	TVE_WUINT32(sel, TVE_03C,0x00000009);		//1.0v refer for 0.71v/1.43v detect
	TVE_WUINT32(sel, TVE_030,0x00000001);		//detect enable
	return 0;
}

s32 tve_low_dac_autocheck_disable(u32 sel,u8 index)
{
	TVE_WUINT32(sel, TVE_030,0x0);
	TVE_WUINT32(sel, TVE_0F8,0x0);
	return 0;
}

s32 tve_low_dac_auto_cali(u32 sel, u32 cali)
{
	u32 dac_err_reg[10];
	s32 dac_err_val[10];
	u32 cali_err_1[10];
	u32 cali_err_2[10];
	s32 dac_err_auto;
	s32 dac_err_efuse;
	s32 dac_err_opti;
	u32 dac_err_set;
	u32 i;

	TVE_WUINT32(sel, TVE_000,0x80000300);		//tvclk enable
	TVE_WUINT32(sel, TVE_030,0x00000000);		//auto detect disable
	TVE_WUINT32(sel, TVE_008,0x433e12b1);		//dac setting

	TVE_WUINT32(sel, TVE_304,cali<<16 | 0xc<<8);
	TVE_SET_BIT(sel, TVE_300,0x1);				//force DAC for cali
	for(i=0;i<3;i++)
	{
		TVE_SET_BIT(sel, TVE_304,0x1<<4);
		mdelay(1);
		TVE_SET_BIT(sel, TVE_304,0x1<<0);
		mdelay(1);
		dac_err_reg[i] = (TVE_RUINT32(sel,TVE_308) >> 16) & 0x3ff;
		TVE_CLR_BIT(sel, TVE_304,0x1<<0);
		TVE_CLR_BIT(sel, TVE_304,0x1<<4);
		mdelay(1);
		if(dac_err_reg[i] & (1<<9))
			dac_err_val[i] = 0 + (dac_err_reg[i] & 0x1ff);
		else
			dac_err_val[i] = 0 - (dac_err_reg[i] & 0x1ff);
	}

	cali_err_1[0] = abs(dac_err_val[1]-dac_err_val[0]);
	cali_err_1[1] = abs(dac_err_val[2]-dac_err_val[1]);
	cali_err_1[2] = abs(dac_err_val[0]-dac_err_val[2]);

	cali_err_2[0] = cali_err_1[2] + cali_err_1[0];
	cali_err_2[1] = cali_err_1[0] + cali_err_1[1];
	cali_err_2[2] = cali_err_1[1] + cali_err_1[2];

	if(cali_err_2[0]<cali_err_2[1] && cali_err_2[0]<cali_err_2[2])
		dac_err_auto = dac_err_val[0];
	else if(cali_err_2[1]<cali_err_2[0] && cali_err_2[1]<cali_err_2[2])
		dac_err_auto = dac_err_val[1];
	else
		dac_err_auto = dac_err_val[2];

	if(cali & (1<<9))
		dac_err_efuse = 0 + (cali & 0x1ff);
	else
		dac_err_efuse = 0 - (cali & 0x1ff);

	if(abs(dac_err_auto-dac_err_efuse)<100)
		dac_err_opti = dac_err_auto;
	else
		dac_err_opti = dac_err_efuse;

	if(dac_err_opti>=0)
		dac_err_set = (1<<9) | dac_err_opti;
	else
		dac_err_set = 0 - dac_err_opti;

	TVE_WUINT32(sel, TVE_304,dac_err_set<<16 | 0xc<<8);
	TVE_CLR_BIT(sel, TVE_300,0x1);
	TVE_WUINT32(sel, TVE_030,0x1);	//enable plug detect

	return 0;
}

s32 tve_low_enhance(u32 sel, u32 mode)
{
	if(0 == mode) {
		TVE_CLR_BIT(sel,TVE_000,0xf<<10); //deflick off
		TVE_SET_BIT(sel,TVE_000,0x5<<10); //deflick is 5
		TVE_SET_BIT(sel,TVE_00C,0x1<<31); //lti on
		TVE_SET_BIT(sel,TVE_00C,0x1<<16); //notch off
	} else if(1 == mode) {
		TVE_CLR_BIT(sel,TVE_000,0xf<<10); //deflick off
		TVE_SET_BIT(sel,TVE_000,0x5<<10); //deflick is 5
		TVE_SET_BIT(sel,TVE_00C,0x1<<31); //lti on
		TVE_CLR_BIT(sel,TVE_00C,0x1<<16); //notch on
	} else if(2 == mode) {
		TVE_CLR_BIT(sel,TVE_000,0xf<<10); //deflick off
		TVE_CLR_BIT(sel,TVE_00C,0x1<<31); //lti off
		TVE_SET_BIT(sel,TVE_00C,0x1<<16); //notch off
	}
	return 0;
}

u32 tve_low_get_sid(u32 index)
{
#if 0
	u32 cali_value = 0;
	void __iomem * sid_sram_base = (void __iomem *)SUNXI_SID_VBASE + 0x200;//SUNXI_SID_VBASE = 0xf1c14000

	cali_value = get_wvalue(sid_sram_base + index);
	cali_value &= 0x000003ff;
	return cali_value;
#endif
	return 0;
}



