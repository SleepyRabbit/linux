/*
 * arch/arm/mach-quatro/platsmp.c
 *
 * Copyright (c) 2014, 2015 Linux Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/io.h>
#include <linux/smp.h>
#include <asm/sizes.h>
#include <asm/cacheflush.h>
#include <asm/smp_scu.h>

#include <linux/of.h>
#include <linux/of_platform.h>
#include "quatro.h"

#if defined(QUATRO_SCU_VIRT_BASE)
static void __iomem *scu_base = IOMEM(QUATRO_SCU_VIRT_BASE);
static void __iomem *misc_base = IOMEM(QUATRO_MISC_VIRT_BASE);
#else
static void __iomem *scu_base;
static void __iomem *misc_base;
#endif

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	sync_cache_w(&pen_release);
}

static DEFINE_SPINLOCK(boot_lock);

static void quatro_secondary_init(unsigned int cpu)
{
    
	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

static int quatro_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	/*
	 * set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * the boot monitor puts secondary cores into WFI, so we need
	 * to send a soft interrupt to wake the secondary core.
	 * Use smp_cross_call() for this, since there's little
	 * point duplicating the code here
	 */
	arch_send_wakeup_ipi_mask(get_cpu_mask(cpu));

	/*
	 * The secondary processor is waiting to be released from
	 * the holding pen - release it, then wait for it to flag
	 * that it has been released by resetting pen_release.
	 *
	 * Note that "pen_release" is the hardware CPU ID, whereas
	 * "cpu" is Linux's internal ID.
	 */
	write_pen_release(cpu);

	timeout = jiffies + (6 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;

		udelay(10);
	}
	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
static void __init quatro_smp_init_cpus(void)
{
	unsigned int i, ncores;

	ncores = scu_get_core_count(scu_base);
	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);
}

static void __init quatro_smp_prepare_cpus(unsigned int max_cpus)
{
	scu_enable(scu_base);
	/*
	 * Write the address of secondary startup into the system-wide location
	 * (presently it is the MISC register). The BootMonitor waits until it receives a
	 * soft interrupt, and then the secondary CPU branches to this address.
	 */
	__raw_writel(virt_to_phys(quatro_secondary_startup), misc_base);
}

void __ref quatro_cpu_die(unsigned int cpu)
{
}

struct smp_operations quatro_smp_ops __initdata = {
	.smp_init_cpus		= quatro_smp_init_cpus,
	.smp_prepare_cpus	= quatro_smp_prepare_cpus,
	.smp_secondary_init	= quatro_secondary_init,
	.smp_boot_secondary	= quatro_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= quatro_cpu_die,
#endif
};

