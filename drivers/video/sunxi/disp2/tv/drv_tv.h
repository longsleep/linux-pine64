#ifndef  _DRV_TV_H_
#define  _DRV_TV_H_

#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "asm-generic/int-ll64.h"
#include "linux/kernel.h"
#include "linux/mm.h"
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()??kthread_run()
#include <linux/err.h> //IS_ERR()??PTR_ERR()
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/switch.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/compat.h>
#include <video/sunxi_display2.h>
#include "de_tvec.h"

enum hpd_status
{
	STATUE_CLOSE = 0,
	STATUE_OPEN  = 1,
};

#define    SCREEN_COUNT    2
#define    DAC_COUNT       4
#define    DAC_INVALID_SOURCE 0xff

int tv_open(struct inode *inode, struct file *file);
int tv_release(struct inode *inode, struct file *file);
ssize_t tv_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t tv_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
int tv_mmap(struct file *file, struct vm_area_struct * vma);
long tv_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

extern s32 tv_init(u32 sel, struct platform_device *pdev);
extern s32 tv_exit(void);

//extern interface exported by display driver
extern s32 disp_tv_register(struct disp_tv_func * func);
extern unsigned int disp_boot_para_parse(const char *name);

struct tv_screen_t
{
	enum  disp_tv_mode      tv_mode;
	void __iomem *          base_address;
	u32                     sid;
	u32                     cali_offset;
	u32                     enable;
	struct clk*		clk;
	bool			suspend;
	bool			used;
};

struct tv_info_t
{
	struct device           *dev;
	enum disp_tv_dac_source dac_source[DAC_COUNT];
	struct tv_screen_t      screen[SCREEN_COUNT];
	struct work_struct      hpd_work;
	struct clk*		clk;
	struct clk*		clk_parent;
	void __iomem*		base_address;
	u32			tv_number;
};

extern struct tv_info_t g_tv_info;

s32 tv_init(u32 sel, struct platform_device *pdev);
s32 tv_exit(void);
s32 tv_get_mode(u32 sel);
s32 tv_set_mode(u32 sel, enum disp_tv_mode tv_mod);
s32 tv_get_input_csc(u32 sel);
s32 tv_get_video_timing_info(u32 sel, struct disp_video_timings **video_info);
s32 tv_enable(u32 sel);
s32 tv_disable(u32 sel);
s32 tv_suspend(u32 sel);
s32 tv_resume(u32 sel);
s32 tv_mode_support(u32 sel, enum disp_tv_mode mode);
void tv_report_hpd_work(void);
s32 tv_detect_thread(void *parg);
s32 tv_detect_enable(u32 sel);
s32 tv_detect_disable(u32 sel);
s32 tv_get_dac_hpd(u32 sel);
s32 tv_hot_plugging_detect (u32 state);

#endif
