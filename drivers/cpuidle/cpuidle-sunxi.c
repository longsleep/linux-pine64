/*
 * SUNXI generic CPU idle driver.
 *
 * Copyright (C) 2014 Allinertech Ltd.
 * Author: pannan <pannan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpuidle.h>
#include <linux/cpumask.h>
#include <linux/cpu_pm.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <asm/cpuidle.h>
#include <asm/suspend.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <linux/clockchips.h>
#include <linux/irqchip/arm-gic.h>

#define BOOT_CPU_HP_FLAG_REG		(0xb8)
#define CPU_SOFT_ENTRY_REG0			(0xbc)
#define CPUCFG_CPUIDLE_EN_REG		(0x140)
#define CPUCFG_CORE_FLAG_REG		(0x144)
#define CPUCFG_PWR_SWITCH_DELAY_REG	(0x150)

#ifndef CONFIG_SMP
#define SUNXI_CPUCFG_PBASE			(0x01C25C00)
#define SUNXI_SYSCTL_PBASE			(0x01C00000)
static void __iomem *sunxi_cpucfg_base;
static void __iomem *sunxi_sysctl_base;
#else
extern void __iomem *sunxi_cpucfg_base;
extern void __iomem *sunxi_sysctl_base;
#endif

#define GIC_DIST_PBASE				(0x01C81000)
static void __iomem *gic_distbase;

static inline bool sunxi_idle_pending_sgi(void)
{
	u32 pending_set;

	pending_set = readl_relaxed(gic_distbase + GIC_DIST_PENDING_SET);
	return pending_set & 0xFFFF ? true : false;
}

static int sunxi_idle_core_power_down(unsigned long val)
{
	unsigned int value;
	unsigned int target_cpu = raw_smp_processor_id();

	pr_debug("cpu%d power down\n", target_cpu);

	if (sunxi_idle_pending_sgi())
		return 1;

	/* disable gic cpu interface */
	gic_cpu_if_down();

	/* set core flag */
	writel_relaxed(0x1 << target_cpu, sunxi_cpucfg_base + CPUCFG_CORE_FLAG_REG);

	/* disable the data cache */
	asm("mrc p15, 0, %0, c1, c0, 0" : "=r" (value) );
	value &= ~(1<<2);
	asm volatile("mcr p15, 0, %0, c1, c0, 0\n" : : "r" (value));

	/* clean and ivalidate L1 cache */
	flush_cache_all();

	/* execute a CLREX instruction */
	asm("clrex" : : : "memory", "cc");

	/* step4: switch cpu from SMP mode to AMP mode, aim is to disable cache coherency */
	asm("mrc p15, 0, %0, c1, c0, 1" : "=r" (value) );
	value &= ~(1<<6);
	asm volatile("mcr p15, 0, %0, c1, c0, 1\n" : : "r" (value));

	/* execute an ISB instruction */
	isb();

	/* execute a DSB instruction */
	dsb();

	/* execute a WFI instruction */
	while (1) {
		asm("wfi" : : : "memory", "cc");
	}

	return 1;
}

static int sunxi_idle_enter_c1(struct cpuidle_device *dev,
				  struct cpuidle_driver *drv, int idx)
{
	int ret;

	writel_relaxed((void *)(virt_to_phys(cpu_resume)),
							sunxi_sysctl_base + CPU_SOFT_ENTRY_REG0);

	cpu_pm_enter();
	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_ENTER, &dev->cpu);
	smp_wmb();

	pr_debug("cpu%d enter C1\n", dev->cpu);
	ret = cpu_suspend(0, sunxi_idle_core_power_down);
	pr_debug("cpu%d exit C1\n", dev->cpu);

	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_EXIT, &dev->cpu);
	cpu_pm_exit();

	return idx;
}

static struct cpuidle_driver sunxi_idle_driver = {
	.name = "sunxi_idle",
	.owner = THIS_MODULE,
	.states[0] = ARM_CPUIDLE_WFI_STATE,
	.states[1] = {
		.enter                  = sunxi_idle_enter_c1,
		.exit_latency           = 30,
		.target_residency       = 500,
		.flags                  = CPUIDLE_FLAG_TIME_VALID,
		.name                   = "CPD",
		.desc                   = "CORE POWER DOWN",
	},
	.state_count = 2,
};

static void sunxi_idle_iomap_init(void)
{
	gic_distbase = ioremap(GIC_DIST_PBASE, SZ_4K);
#ifndef CONFIG_SMP
	sunxi_cpucfg_base = ioremap(SUNXI_CPUCFG_PBASE, SZ_1K);
	sunxi_sysctl_base = ioremap(SUNXI_SYSCTL_PBASE, SZ_1K);
	pr_debug("%s: cpucfg_base=0x%p sysctl_base=0x%p\n", __func__,
					sunxi_cpucfg_base, sunxi_sysctl_base);
#endif
}

static void sunxi_idle_hw_init(void)
{
	/* write hotplug flag, cpu0 use */
	writel_relaxed(0xFA50392F, sunxi_sysctl_base + BOOT_CPU_HP_FLAG_REG);

	/* set delay0 to 5us */
	writel_relaxed(0x5, sunxi_cpucfg_base + CPUCFG_PWR_SWITCH_DELAY_REG);

	/* enable cpuidle */
	writel_relaxed(0x16AA0000, sunxi_cpucfg_base + CPUCFG_CPUIDLE_EN_REG);
	writel_relaxed(0xAA160001, sunxi_cpucfg_base + CPUCFG_CPUIDLE_EN_REG);
}

static int __init sunxi_idle_init(void)
{
	int ret;

	if (!of_machine_is_compatible("arm,sun8iw11p1"))
		return -ENODEV;

	sunxi_idle_iomap_init();

	ret = cpuidle_register(&sunxi_idle_driver, NULL);
	if (!ret)
		sunxi_idle_hw_init();

	return ret;
}
device_initcall(sunxi_idle_init);
