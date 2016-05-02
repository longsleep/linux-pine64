/*
 * Copyright (C) 2015 Allwinnertech, z.q <zengqi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "disp_format_convert.h"
#ifdef SUPPORT_WB
#include "include.h"

#define FORMAT_MANAGER_NUM 1
static struct format_manager fmt_mgr[FORMAT_MANAGER_NUM];

struct format_manager *disp_get_format_manager(unsigned int id)
{
	return &fmt_mgr[id];
}


#if defined(__LINUX_PLAT__)
s32 disp_format_convert_finish_proc(int irq, void *parg)
#else
s32 disp_format_convert_finish_proc(void *parg)
#endif
{
	struct format_manager *mgr = disp_get_format_manager(0);

	if (NULL == mgr)
		return DISP_IRQ_RETURN;

	if (0 == disp_al_get_eink_wb_status(0)) {
		disp_al_clear_eink_wb_interrupt(0);
		mgr->write_back_finish = 1;
		wake_up_interruptible(&(mgr->write_back_queue));
	}

	return DISP_IRQ_RETURN;
}

static s32 disp_format_convert_enable(unsigned int id)
{
	struct format_manager *mgr = &fmt_mgr[id];
	s32 ret = -1;

	if (NULL == mgr) {
		printk(KERN_WARNING"%s: input param is null\n", __func__);
		return -1;
	}

	printk("write back irq no=%d\n", mgr->irq_num);
	ret = disp_sys_register_irq(mgr->irq_num, 0, disp_format_convert_finish_proc,(void*)mgr, 0, 0);
	if (0 != ret) {
		printk(KERN_ERR"%s L%d: fail to format convert irq\n", __func__, __LINE__);
		return ret;
	}

	disp_sys_enable_irq(mgr->irq_num);

	ret = clk_prepare_enable(mgr->clk);

	if (0 != ret)
		DE_WRN("fail enable mgr's clock!\n");

	disp_al_de_clk_enable(mgr->disp);				//enable de clk
	disp_al_write_back_clk_init(mgr->disp);		//enable write back clk
	return 0;
}


static s32 disp_format_convert_disable(unsigned int id)
{
	struct format_manager *mgr = &fmt_mgr[id];
	s32 ret = -1;

	if (NULL == mgr) {
		printk(KERN_WARNING"%s: input param is null\n", __func__);
		return -1;
	}

	__debug("write back irq no=%d\n", mgr->irq_num);
	disp_al_write_back_clk_exit(mgr->disp);		//disable write back clk
	ret = disp_al_de_clk_disable(mgr->disp);		//disable de clk
	if (0 != ret)
		return ret;

	disp_sys_disable_irq(mgr->irq_num);
	disp_sys_unregister_irq(mgr->irq_num,disp_format_convert_finish_proc,(void*)mgr);

	return 0;
}

static s32 disp_format_convert_start(unsigned int id, struct image_format *src, struct image_format *dest)
{
	struct format_manager *mgr = &fmt_mgr[id];
	s32 ret = -1;

	long timerout = (100 * HZ)/1000;		/*100ms*/

	if ((NULL == src) || (NULL == dest) || (NULL == mgr)) {
		__wrn(KERN_WARNING"input param is null\n");
		return -1;
	}

	if ((DISP_FORMAT_ARGB_8888 == src->format) && (DISP_FORMAT_8BIT_GRAY == dest->format)) {
		__debug("src_addr = 0x%p, dest_addr = 0x%p\n",(void*)src->addr1, (void*)dest->addr1);
		disp_al_rtmx_set_addr(0, (unsigned int)src->addr1);
		disp_al_set_eink_wb_param(0, src->width, src->height, (unsigned int)dest->addr1);//800,600
		disp_al_enable_eink_wb(0);

#ifdef __UBOOT_PLAT__
		/*wait write back complete*/
		while(EWB_OK != disp_al_get_eink_wb_status(0)) {
			msleep(1);
		}
#else
		/*enable inttrrupt*/
		disp_al_enable_eink_wb_interrupt(0);

		timerout = wait_event_interruptible_timeout(mgr->write_back_queue, (1 == mgr->write_back_finish),timerout);
		mgr->write_back_finish = 0;
		if (0 == timerout) {
			__wrn("wait write back timeout\n");
			disp_al_disable_eink_wb_interrupt(0);
			disp_al_disable_eink_wb(0);
			ret = -1;
			return ret;
		}

		disp_al_disable_eink_wb_interrupt(0);
#endif

		disp_al_disable_eink_wb(0);
		ret = 0;
	} else {
		__wrn("src_format(0x%x), dest_format(0x%x) is not support\n", src->format, dest->format);
	}

	return ret;
}

s32 disp_init_format_convert_manager(disp_bsp_init_para * para)
{
	s32 ret = -1;
	unsigned int i = 0;
	struct format_manager *mgr;

	for (i = 0; i < FORMAT_MANAGER_NUM; i++) {
		mgr = &fmt_mgr[i];
		memset(mgr, 0, sizeof(struct format_manager));

		mgr->disp = i;
		init_waitqueue_head(&(mgr->write_back_queue));
		mgr->write_back_finish = 0;
		mgr->irq_num = para->irq_no[DISP_MOD_DE];
		printk("irq_num=%d\n",mgr->irq_num);
		if (mgr->irq_num == 0)
			mgr->irq_num = 55;
		//mgr->irq_num = 55;//test

		mgr->enable = disp_format_convert_enable;
		mgr->disable = disp_format_convert_disable;
		mgr->start_convert = disp_format_convert_start;
		mgr->clk = para->mclk[DISP_MOD_DE];

		disp_al_set_rtmx_base(i, para->reg_base[DISP_MOD_DE]);
		disp_al_set_eink_wb_base(i, para->reg_base[DISP_MOD_DE]);

/*init wait for enable.
		if (mgr->enable) {
			ret = mgr->enable(i);
			if (0 != ret) {
				printk(KERN_ERR"%s: format convert manager init fail\n", __func__);
				break;
			}
		}
*/
	}

	return ret;
}

void disp_exit_format_convert_manager(void)
{
	unsigned int i = 0;
	struct format_manager *mgr = NULL;

	for (i = 0; i < FORMAT_MANAGER_NUM; i++) {
		if (mgr->disable) {
			mgr->disable(i);
		}
	}

	return;
}

#endif

