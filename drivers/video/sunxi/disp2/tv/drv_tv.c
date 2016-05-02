#include "drv_tv.h"

#include <linux/regulator/consumer.h>
#include <linux/clk-private.h>

static struct mutex mlock;

static struct cdev *tv_cdev;
static dev_t devid ;
static struct class *tv_class;

struct tv_info_t g_tv_info;

static struct disp_video_timings video_timing[] =
{
	/*vic		 tv_mode		  PCLK   AVI	x	 y	HT  HBP HFP HST VT VBP VFP VST*/
	{0,   DISP_TV_MOD_NTSC,27000000,  0,  720,   480,   858,   60,   16,   62,  525,   30,  9,  6,  0,   0,   0,   0,   0},
	{0,   DISP_TV_MOD_PAL  ,27000000,  0,  720,   576,   864,   68,   12,   64,  625,   39,  5,  5,  0,   0,   0,   0,   0},
};

#define TVE_CHECK_PARAM(sel) if(sel >= g_tv_info.tv_number) \
	{\
		pr_warn("%s, sel(%d) is out of range\n", __func__, sel); \
		return -1;\
	}

//#define TVDEBUG
#if defined(TVDEBUG)
#define TV_DBG(fmt, arg...)     pr_warn("%s()%d - "fmt, __func__, __LINE__, ##arg)
#else
#define TV_DBG(fmt, arg...)
#endif
#define TV_ERR(fmt, arg...)     pr_err("%s()%d - "fmt, __func__, __LINE__, ##arg)

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
static struct task_struct * g_tve_task;
static u32 g_tv_hpd = 0;

static struct switch_dev cvbs_switch_dev = {
	.name = "cvbs",
};

static struct switch_dev svideo_switch_dev = {
	.name = "svideo",
};

static struct switch_dev ypbpr_switch_dev = {
	.name = "ypbpr",
};

static struct switch_dev vga_switch_dev = {
	.name = "vga",
};

void tv_report_hpd_work(void)
{
	switch(g_tv_hpd) {

	case DISP_TV_NONE:
		switch_set_state(&cvbs_switch_dev, STATUE_CLOSE);
		switch_set_state(&svideo_switch_dev, STATUE_CLOSE);
		switch_set_state(&ypbpr_switch_dev, STATUE_CLOSE);
		switch_set_state(&vga_switch_dev, STATUE_CLOSE);
		break;

	case DISP_TV_CVBS:
		switch_set_state(&cvbs_switch_dev, STATUE_OPEN);
		break;

	case DISP_TV_SVIDEO:
		switch_set_state(&svideo_switch_dev, STATUE_OPEN);
		break;

	case DISP_TV_YPBPR:
		switch_set_state(&ypbpr_switch_dev, STATUE_OPEN);
		break;

	default:
		switch_set_state(&cvbs_switch_dev, STATUE_CLOSE);
		switch_set_state(&svideo_switch_dev, STATUE_CLOSE);
		switch_set_state(&ypbpr_switch_dev, STATUE_CLOSE);
		switch_set_state(&vga_switch_dev, STATUE_CLOSE);
		break;
	}
}

s32 tv_detect_thread(void *parg)
{
	s32 hpd;
	int i = 0;
	while(1) {
		if(kthread_should_stop()) {
			break;
		}
		if(!g_suspend) {
			for(i=0;i<SCREEN_COUNT;i++)
				hpd = tv_get_dac_hpd(i);
			if(hpd != g_tv_hpd) {
				g_tv_hpd = hpd;
				tv_report_hpd_work();
			}
		}
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(20);
	}
	return 0;
}

s32 tv_detect_enable(u32 sel)
{
	int i = 0;

	tve_low_dac_autocheck_enable(sel, 0);
	if (!g_tve_task) {
		g_tve_task = kthread_create(tv_detect_thread, (void*)0, "tve detect");
		if(IS_ERR(g_tve_task)) {
			s32 err = 0;
			err = PTR_ERR(g_tve_task);
			g_tve_task = NULL;
			return err;
		}
		else {
			pr_debug("g_tve_task is ok!\n");
		}
		wake_up_process(g_tve_task);
	}
	return 0;
}

s32 tv_detect_disable(u32 sel)
{
	int i = 0;
	if(g_tve_task) {
		kthread_stop(g_tve_task);
		g_tve_task = NULL;
		tve_low_dac_autocheck_disable(sel, 0);
	}
	return 0;
}
#else
void tv_report_hpd_work(void)
{
	pr_debug("there is null report hpd work,you need support the switch class!");
}

s32 tv_detect_thread(void *parg)
{
	pr_debug("there is null tv_detect_thread,you need support the switch class!");
	return -1;
}

s32 tv_detect_enable(u32 sel)
{
	pr_debug("there is null tv_detect_enable,you need support the switch class!");
	return -1;
}

s32 tv_detect_disable(u32 sel)
{
	pr_debug("there is null tv_detect_disable,you need support the switch class!");
		return -1;
}
#endif

s32 tv_get_dac_hpd(u32 sel)
{
	u8 dac = 0;
	u32 ret = DISP_TV_NONE;

	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	dac = tve_low_get_dac_status(sel);
	if(dac>1) {
		dac = 0;
	}
	if(g_tv_info.dac_source[0] == DISP_TV_DAC_SRC_COMPOSITE && dac == 1) {
		ret = DISP_TV_CVBS;
	}
	return  ret;
}

s32 tv_get_video_info(s32 mode)
{
	s32 i,count;
	count = sizeof(video_timing)/sizeof(struct disp_video_timings);
	for(i=0;i<count;i++) {
		if(mode == video_timing[i].tv_mode)
			return i;
	}
	return -1;
}

s32 tv_get_list_num(void)
{
	return sizeof(video_timing)/sizeof(struct disp_video_timings);
}

s32 tv_set_enhance_mode(u32 sel, u32 mode)
{
	return tve_low_enhance(sel, mode);
}

static void tve_clk_init(u32 sel)
{
	g_tv_info.clk_parent = clk_get_parent(g_tv_info.clk);
}


static int tve_clk_enable(u32 sel)
{
	int ret;

	ret = clk_prepare_enable(g_tv_info.clk);
	if (0 != ret) {
		pr_warn("fail to enable tve's clk!\n");
		return ret;
	}
	ret = clk_prepare_enable(g_tv_info.screen[sel].clk);
	if (0 != ret) {
		pr_warn("fail to enable tve%d's clk!\n", sel);
		return ret;
	}

	return 0;
}

static int tve_clk_disable(u32 sel)
{
	clk_prepare_enable(g_tv_info.screen[sel].clk);
	clk_prepare_enable(g_tv_info.clk);

	return 0;
}

static void tve_clk_config(u32 sel, u32 tv_mode)
{
	struct disp_video_timings *info = video_timing;
	int i, list_num;
	bool find = false;
	unsigned long rate = 0;

	list_num = tv_get_list_num();
	for(i=0; i<list_num; i++) {
		if(info->tv_mode == tv_mode) {
			find = true;
			break;
		}
		info ++;
	}

	if (find) {
		rate = info->pixel_clk;
		if (clk_set_rate(g_tv_info.screen[sel].clk, rate))
			pr_warn("fail to set rate(%ld) fo tve%d's clock!\n", rate, sel);
	}
}

s32 tv_init(u32 sel, struct platform_device *pdev)
{
	s32 i = 0;
	u32 sid = 0;
	char sub_key[20];
	const char *status;

	if (of_property_read_string(pdev->dev.of_node, "status", &status) < 0) {
		pr_debug("of_property_read disp_init.disp_init_enable fail\n");
		return -1;
	} else {
		if (!strcmp(status, "okay"))
			g_tv_info.screen[sel].used = true;
	}

	if(g_tv_info.screen[sel].used) {
		unsigned int value, output_type, output_mode;

		mutex_init(&mlock);
		value = disp_boot_para_parse("boot_disp");
		pr_debug("[TV]:value = %d", value);
		output_type = ((value >> 8) & 0xff) >> (sel * 16);
		output_mode = ((value) & 0xff) >> (sel * 16);

		if(output_type == DISP_OUTPUT_TYPE_TV) {
			g_tv_info.screen[sel].enable = 1;
			g_tv_info.screen[sel].tv_mode = output_mode;
			pr_debug("[TV]:g_tv_info.screen[0].tv_mode = %d", g_tv_info.screen[sel].tv_mode);
		}
#if 0
//todo, if the cali value of the two tve are the same
		type = script_get_item("tv_para", "tv_cali_offset", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT == type) {
			g_tv_info.screen[sel].cali_offset = val.val;
		}
#endif
		sid = tve_low_get_sid(0x10);
		if (0 == sid)
			g_tv_info.screen[sel].sid = 0x200;
		else
			g_tv_info.screen[sel].sid = sid;

		tve_low_set_reg_base(sel, g_tv_info.screen[sel].base_address);
		tve_clk_init(sel);
		tve_clk_config(sel, g_tv_info.screen[sel].tv_mode);
		//tve_clk_enable(i); //todo ?
		tv_detect_enable(sel);

		for(i=0; i<DAC_COUNT; i++) {
			u32 value;

			sprintf(sub_key, "tv_dac_src%d", i);
			if (of_property_read_u32(pdev->dev.of_node, sub_key, &value) < 0) {
				pr_debug("of_property_read tve%d's %s fail\n", sel, sub_key);
			} else {
				if (g_tv_info.dac_source[i] != DAC_INVALID_SOURCE)
					pr_warn("[TV]: tv%d' dac source conflict with other tv!\n", sel);
				g_tv_info.dac_source[i] = value;
			}
		}
	}
	return 0;
}

s32 tv_exit(void)
{
	s32 i;
	mutex_lock(&mlock);
	for (i=0; i<g_tv_info.tv_number; i++)
		tv_detect_disable(i);
	mutex_unlock(&mlock);
	for (i=0; i<g_tv_info.tv_number; i++)
		tv_disable(i);

	return 0;
}

s32 tv_get_mode(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	return g_tv_info.screen[sel].tv_mode;
}

s32 tv_set_mode(u32 sel, enum disp_tv_mode tv_mode)
{
	if(tv_mode >= DISP_TV_MODE_NUM) {
		return -1;
	}

	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	mutex_lock(&mlock);
	g_tv_info.screen[sel].tv_mode = tv_mode;
	mutex_unlock(&mlock);
	return  0;
}

s32 tv_get_input_csc(u32 sel)
{

	return 1;
}

s32 tv_get_video_timing_info(u32 sel, struct disp_video_timings **video_info)
{
	struct disp_video_timings *info;
	int ret = -1;
	int i, list_num;
	info = video_timing;

	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	list_num = tv_get_list_num();
	for(i=0; i<list_num; i++) {
		mutex_lock(&mlock);
		if(info->tv_mode == g_tv_info.screen[sel].tv_mode){
			*video_info = info;
			ret = 0;
			mutex_unlock(&mlock);
			break;
		}
		mutex_unlock(&mlock);
		info ++;
	}
	return ret;
}

s32 tv_enable(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	if(!g_tv_info.screen[sel].enable) {
		tve_low_set_tv_mode(sel, g_tv_info.screen[sel].tv_mode, g_tv_info.screen[sel].sid);
		tve_low_open(sel);
		mutex_lock(&mlock);
		g_tv_info.screen[sel].enable = 1;
		mutex_unlock(&mlock);
	}
	return 0;
}

s32 tv_disable(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	mutex_lock(&mlock);
	if(g_tv_info.screen[sel].enable) {
		tve_low_close(sel);
		g_tv_info.screen[sel].enable = 0;
	}
	mutex_unlock(&mlock);
	return 0;
}

s32 tv_suspend(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	mutex_lock(&mlock);
	if(g_tv_info.screen[sel].used && (!g_tv_info.screen[sel].suspend)) {
		g_tv_info.screen[sel].suspend = true;
		tv_detect_disable(sel);
		tve_clk_disable(sel);
	}
	mutex_unlock(&mlock);

	return 0;
}

s32 tv_resume(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	mutex_lock(&mlock);
	if(g_tv_info.screen[sel].used && (g_tv_info.screen[sel].suspend)) {
		g_tv_info.screen[sel].suspend = false;
		tve_clk_enable(sel);
		tve_low_init(sel, g_tv_info.screen[sel].sid, g_tv_info.screen[sel].cali_offset);

		tv_detect_enable(sel);
	}
	mutex_unlock(&mlock);

	return  0;
}

s32 tv_mode_support(u32 sel, enum disp_tv_mode mode)
{
	u32 i, list_num;
	struct disp_video_timings *info;

	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	info = video_timing;
	list_num = tv_get_list_num();
	for(i=0; i<list_num; i++) {
		if(info->tv_mode == mode) {
			return 1;
		}
		info ++;
	}
	return 0;
}

s32 tv_hot_plugging_detect (u32 state)
{
	int i = 0;
	for(i=0; i<SCREEN_COUNT;i++) {
		if(state == STATUE_OPEN) {
			return tve_low_dac_autocheck_enable(i,0);
		}
		else if(state == STATUE_CLOSE){
			return tve_low_dac_autocheck_disable(i,0);
		}
	}
	return 0;
}

#if !defined(CONFIG_OF)
static struct resource tv_resource[1] =
{
	[0] = {
		.start = 0x01c16000,  //modify
		.end   = 0x01c165ff,
		.flags = IORESOURCE_MEM,
	},
};
#endif

#if !defined(CONFIG_OF)
struct platform_device tv_device =
{
	.name		   = "tv",
	.id				= -1,
	.num_resources  = ARRAY_SIZE(tv_resource),
	.resource		= tv_resource,
	.dev			= {}
};
#else
static const struct of_device_id sunxi_tv_match[] = {
	{ .compatible = "allwinner,sunxi-tv", },
	{},
};
#endif

static int __init tv_probe(struct platform_device *pdev)
{
	int ret;
	int counter = 0;
	int i = 0;
	struct disp_tv_func disp_func;

	memset(&g_tv_info, 0, sizeof(struct tv_info_t));
	if (of_property_read_u32(pdev->dev.of_node, "tv-number", &g_tv_info.tv_number) < 0) {
		dev_err(&pdev->dev, "unable to get tv-number, force to one!\n");
		g_tv_info.tv_number = 1;
	}
	g_tv_info.base_address= of_iomap(pdev->dev.of_node, counter);
	if (!g_tv_info.base_address) {
		dev_err(&pdev->dev, "unable to map tve common registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}
	counter ++;

	for (i=0; i<g_tv_info.tv_number; i++) {
		g_tv_info.screen[i].base_address= of_iomap(pdev->dev.of_node, counter);
		if (!g_tv_info.screen[i].base_address) {
			dev_err(&pdev->dev, "unable to map tve%d's registers\n", i);
			ret = -EINVAL;
			goto err_iomap;
		}
		counter ++;
	}

	counter = 0;
	g_tv_info.clk = of_clk_get(pdev->dev.of_node, counter);
	if (IS_ERR(g_tv_info.clk)) {
		dev_err(&pdev->dev, "fail to get clk for tve common module!\n");
	}
	counter ++;

	for (i=0; i<g_tv_info.tv_number; i++) {
		g_tv_info.screen[i].clk = of_clk_get(pdev->dev.of_node, counter);
		if (IS_ERR(g_tv_info.screen[i].clk)) {
			dev_err(&pdev->dev, "fail to get clk for tve%d's!\n", i);
		}
	}
	counter ++;

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
	switch_dev_register(&cvbs_switch_dev);
	switch_dev_register(&ypbpr_switch_dev);
	switch_dev_register(&svideo_switch_dev);
	switch_dev_register(&vga_switch_dev);
#endif
	for(i=0; i<DAC_COUNT; i++)
		g_tv_info.dac_source[i] = 0xff;//invalid dac source

	for (i=0; i<g_tv_info.tv_number; i++) {
		struct device_node *sub_tv = NULL;
		struct platform_device *sub_pdev = NULL;

		sub_tv = of_parse_phandle(pdev->dev.of_node, "tvs", i);
		if (!sub_tv) {
			dev_err(&pdev->dev, "fail to parse phandle for tve%d!\n", i);
			continue;
		}
		sub_pdev = of_find_device_by_node(sub_tv);
		if (!sub_pdev) {
			dev_err(&pdev->dev, "fail to find device for tve%d!\n", i);
			continue;
		}
		if (sub_pdev) {
			ret = tv_init(i, sub_pdev);
			if(ret!=0) {
				pr_debug("tve%d init is failed\n", i);
				return -1;
			}
		}
	}

	memset(&disp_func, 0, sizeof(struct disp_tv_func));
	disp_func.tv_enable = tv_enable;
	disp_func.tv_disable = tv_disable;
	disp_func.tv_resume = tv_resume;
	disp_func.tv_suspend = tv_suspend;
	disp_func.tv_get_mode = tv_get_mode;
	disp_func.tv_set_mode = tv_set_mode;
	disp_func.tv_get_video_timing_info = tv_get_video_timing_info;
	disp_func.tv_get_input_csc = tv_get_input_csc;
	disp_func.tv_mode_support = tv_mode_support;
	disp_func.tv_hot_plugging_detect = tv_hot_plugging_detect;
	disp_func.tv_set_enhance_mode = tv_set_enhance_mode;
	disp_tv_register(&disp_func);

err_iomap:
	if (g_tv_info.base_address)
		iounmap((char __iomem *)g_tv_info.base_address);
	for (i=0; i<g_tv_info.tv_number; i++) {
		if (g_tv_info.screen[i].base_address)
			iounmap((char __iomem *)g_tv_info.screen[i].base_address);
	}
	return 0;
}

static int tv_remove(struct platform_device *pdev)
{
	tv_exit();
	return 0;
}

/*int tv_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}*/

/*int tv_resume(struct platform_device *pdev)
{
	return 0;
}*/
static struct platform_driver tv_driver =
{
	.probe	  = tv_probe,
	.remove	 = tv_remove,
 //   .suspend	= tv_suspend,
  //  .resume	 = tv_resume,
	.driver	 =
	{
		.name   = "tv",
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_tv_match,
	},
};

int tv_open(struct inode *inode, struct file *file)
{
	return 0;
}

int tv_release(struct inode *inode, struct file *file)
{
	return 0;
}

ssize_t tv_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

ssize_t tv_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

int tv_mmap(struct file *file, struct vm_area_struct * vma)
{
	return 0;
}

long tv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static const struct file_operations tv_fops =
{
	.owner		= THIS_MODULE,
	.open		= tv_open,
	.release	= tv_release,
	.write	  = tv_write,
	.read		= tv_read,
	.unlocked_ioctl	= tv_ioctl,
	.mmap	   = tv_mmap,
};

int __init tv_module_init(void)
{
	int ret = 0, err;

	alloc_chrdev_region(&devid, 0, 1, "tv");
	tv_cdev = cdev_alloc();
	cdev_init(tv_cdev, &tv_fops);
	tv_cdev->owner = THIS_MODULE;
	err = cdev_add(tv_cdev, devid, 1);
	if (err) {
		pr_debug("cdev_add fail.\n");
		return -1;
	}

	tv_class = class_create(THIS_MODULE, "tv");
	if (IS_ERR(tv_class)) {
		pr_debug("class_create fail.\n");
		return -1;
	}

	g_tv_info.dev = device_create(tv_class, NULL, devid, NULL, "tv");
#if !defined(CONFIG_OF)
	ret = platform_device_register(&tv_device);
#endif
	if (ret == 0) {
		ret = platform_driver_register(&tv_driver);
	}
	return ret;
}

static void __exit tv_module_exit(void)
{
	platform_driver_unregister(&tv_driver);
#if !defined(CONFIG_OF)
	platform_device_unregister(&tv_device);
#endif
	class_destroy(tv_class);
	cdev_del(tv_cdev);
}

late_initcall(tv_module_init);
module_exit(tv_module_exit);

MODULE_AUTHOR("zengqi");
MODULE_DESCRIPTION("tv driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:tv");
