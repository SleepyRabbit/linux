/*
 * arch/arm/mach-quatro/quatro55xx_pm.c - Quatro55xx power management support
 *
 * Created by:	Nicolas Pitre, October 2012
 * Copyright:	(C) 2012-2013  Linaro Limited
 *
 * Some portions of this file were originally written by Achin Gupta
 * Copyright:   (C) 2012  ARM Limited
 *
 * Portions Copyright 2014, 2015 Linux Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

// #define DEBUG

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/irqchip/arm-gic.h>

#include <asm/mcpm.h>
#include <asm/proc-fns.h>
#include <asm/cacheflush.h>
#include <asm/cputype.h>
#include <asm/cp15.h>
#include <asm/cacheflush.h>

#include <linux/arm-cci.h>
#include "core.h"

/*
 * We can't use regular spinlocks. In the switcher case, it is possible
 * for an outbound CPU to call power_down() after its inbound counterpart
 * is already live using the same logical CPU number which trips lockdep
 * debugging.
 */
static arch_spinlock_t quatro55xx_pm_lock = __ARCH_SPIN_LOCK_UNLOCKED;

#define QUATRO_A15GPF_BASE               0x043B0000
#define QUATRO_A15EVA0_OFF               0x00000040
#define QUATRO_A15EVA1_OFF               0x00000044

#define QUATRO_RSTGEN_BASE               0x04010000
#define QUATRO_RSTGENCFG_PAD_INTERNAL_OFF 0x00000024
#define QUATRO_RSTGENCFG_PAD_INTERNAL__REV__SHIFT       6
#define QUATRO_RSTGENCFG_PAD_INTERNAL__REV__WIDTH       2
#define QUATRO_RSTGENCFG_PAD_INTERNAL__REV__MASK        0x000000C0

#define QUATRO55XX_CLUSTERS		2
#define QUATRO55XX_MAX_CPUS_PER_CLUSTER	2

static unsigned int quatro55xx_nr_cpus[QUATRO55XX_CLUSTERS];

/* Keep per-cpu usage count to cope with unordered up/down requests */
static int quatro55xx_pm_use_count[QUATRO55XX_MAX_CPUS_PER_CLUSTER][QUATRO55XX_CLUSTERS];

#define quatro55xx_cluster_unused(cluster) \
	(!quatro55xx_pm_use_count[0][cluster] && \
	 !quatro55xx_pm_use_count[1][cluster])

static int quatro55xx_chip_version(void)
{
	unsigned long reg_value;
	void __iomem* mapped_address;
	int ret_val;
	
	mapped_address = ioremap(QUATRO_RSTGEN_BASE, QUATRO_RSTGENCFG_PAD_INTERNAL_OFF+0x4); /* TODO: do something better here, maybe dl -related. */

	reg_value = readl((void *)mapped_address+QUATRO_RSTGENCFG_PAD_INTERNAL_OFF);
	
	iounmap(mapped_address);

	ret_val = (reg_value & QUATRO_RSTGENCFG_PAD_INTERNAL__REV__MASK)>> QUATRO_RSTGENCFG_PAD_INTERNAL__REV__SHIFT;

	return ret_val;
}

static void __init quatro55xx_a0_pm_prepare_cpus(unsigned int max_cpus)
{
	struct device_node *node;
	struct resource res;
	unsigned int rsize;
	int ret;
	unsigned long __iomem *sram_base_addr;

	node = of_find_compatible_node(NULL, NULL, "csr,quatro55xx-smp-sram");
	if (!node) {
		pr_err("%s: could not find sram dt node\n", __func__);
		return;
	}

	ret = of_address_to_resource(node, 0, &res);
	if (ret < 0) {
		pr_err("%s: could not get address for node %s\n",
		       __func__, node->full_name);
		return;
	}
	
	rsize = resource_size(&res);
	if (rsize < 8) {
		pr_err("%s: reserved block with size 0x%x is too small.\n",
		       __func__, rsize);
		return;
	}

	sram_base_addr = (unsigned long*)of_iomap(node, 0);
	sram_base_addr[0] = sram_base_addr[1] = (unsigned long)virt_to_phys(quatro_secondary_startup);
	outer_clean_range(__pa(sram_base_addr), __pa(sram_base_addr+8));

	return;
}

static void quatro55xx_pm_set_boot_vector(void)
{
	void __iomem *mapped_address;

	quatro55xx_a0_pm_prepare_cpus(3);

	if (quatro55xx_chip_version() > 0) {
		unsigned long reg_value;

		mapped_address = ioremap(QUATRO_A15GPF_BASE, QUATRO_A15EVA1_OFF+0x4); /* TODO: do something better here, maybe dl -related. */

		writel(quatro_secondary_startup, (void*)mapped_address+QUATRO_A15EVA0_OFF);
		writel(quatro_secondary_startup, (void*)mapped_address+QUATRO_A15EVA1_OFF);
		
		iounmap(mapped_address);
	}
}

/**
 * quatro55xx_set_resume_addr_pm(() - set the jump address used for warm boot
 *
 * @cluster: mpidr[15:8] bitfield describing cluster affinity level
 * @cpu: mpidr[7:0] bitfield describing cpu affinity level
 * @addr: physical resume address
 */
static void quatro55xx_set_resume_addr_pm(u32 cluster, u32 cpu, u32 addr)
{
	quatro55xx_smp_entrypoint = addr;
	mb();
	flush_cache_all();
	return;
}

static DEFINE_SPINLOCK(power_lock);
static unsigned long const power_up_constants[] = {0x3, 0x5}; /* Clear these bits to turn on A15 0 or 1. */
static unsigned long const power_dn_constants[] = {0x2, 0x4}; /* Set these bits to turn off A15 0 or 1. */

/* static TODO: restore this */ void power_up_cpu_pm(unsigned int cpu)
{
	unsigned long reg_value;
	unsigned long reg_value_other;
	int first_a15 = 0;
	void __iomem* mapped_address;

	if (cpu != 0 && cpu != 1)
		return;

	mapped_address = ioremap(0x04010000, 0x16c);
	spin_lock(&power_lock);

	reg_value = readl((void *)mapped_address+0x164);
	if (reg_value & 0x1) {
		/* This is the first A15 to power up, need an extra reset asserted as it gets power. */
		first_a15 = 1;
		reg_value_other = readl((void *)mapped_address+0x080);
		reg_value_other |= 1<<12; /* A15 reset */
		reg_value_other |= 1<<14; /* "medium" reset */
		writel(reg_value_other, (void *)mapped_address+0x080);
	}
	    
	/* Apply power. */
	reg_value &= ~(power_up_constants[cpu]);
	writel(reg_value, (void *)mapped_address+0x164); /* TODO: do something better here, maybe dl -related. */
	
	/* Wait until power is up. */
	do {
		reg_value = readl((void *)mapped_address+0x168) & power_up_constants[cpu];
	} while (reg_value != 0);

	/* Lift the isolate. */
	reg_value = readl((void *)mapped_address+0x160);
	reg_value &= ~(power_up_constants[cpu]);
	writel(reg_value, (void *)mapped_address+0x160); /* TODO: do something better here, maybe dl-related. */

	spin_unlock(&power_lock);
	iounmap(mapped_address);

	if (first_a15) {
		/* And lift that extra reset now that first A15 has power. */
		reg_value_other = readl((void *)mapped_address+0x080);
		/* First lift "medium" reset so it can do its stuff first, then lift A15 reset. */
		writel(reg_value_other & ~(1<<14), (void *)mapped_address+0x080);
		reg_value_other = readl((void *)mapped_address+0x080);
		writel(reg_value_other & ~(1<<12), (void *)mapped_address+0x080);
	}

#ifdef CONFIG_QUATRO5500_A15_DIV2
	mapped_address = ioremap(0x043B0020, 0x4);
	writel(2, (void *)mapped_address);
	iounmap(mapped_address);
#endif

	return;
}

static DEFINE_SPINLOCK(reset_lock);
static unsigned long const reset_constants[] = {0x55, 0xaa};

static void apply_reset_cpu_pm(unsigned int cpu)
{
	unsigned long reg_value;
	void __iomem* mapped_address;

	if (cpu != 0 && cpu != 1)
		return;

	mapped_address = ioremap(0x043B0024, 4);
	spin_lock(&reset_lock);

	reg_value = readl((void *)mapped_address); /* TODO: do something better here, maybe dl-related. */
	reg_value &= ~(reset_constants[cpu]);
	writel(reg_value, (void *)mapped_address); /* TODO: do something better here, maybe dl-related. */
		
	if ((reg_value & 0xff) == 0) {
		/* The last CPU has been reset, apply common resets. */
		reg_value &= ~0x300;  /* Shared debug, timer, and L2. */
		writel(reg_value, (void *)mapped_address); /* TODO: do something better here, maybe dl-related. */
	}

	spin_unlock(&reset_lock);
	iounmap(mapped_address);

	return;
}

/* static TODO: delete me */ void power_down_cpu_pm(unsigned int cpu)
{
	unsigned long reg_value;
	void __iomem* mapped_address;
	int loop_count = 0; // TODO: delete me

	if (cpu != 0 && cpu != 1) {
		return;
	}

	mapped_address = ioremap(0x04010000, 0x16c);
	spin_lock(&power_lock);

	/* Isolate. */
	reg_value = readl((void *)mapped_address+0x160);
	reg_value |= power_dn_constants[cpu];
	writel(reg_value, (void *)mapped_address+0x160); /* TODO: do something better here, maybe dl-related. */
	reg_value = readl((void *)mapped_address+0x160);

	/* Remove power. */
	reg_value = readl((void *)mapped_address+0x164);
	reg_value |= power_dn_constants[cpu];
	writel(reg_value, (void *)mapped_address+0x164); /* TODO: do something better here, maybe dl-related. */
	reg_value = readl((void *)mapped_address+0x164);
	
	/* Wait until power is down. */
	do {
		loop_count++;
		reg_value = readl((void *)mapped_address+0x168);
		reg_value &= power_dn_constants[cpu];
	} while (reg_value == 0);

	spin_unlock(&power_lock);
	iounmap(mapped_address);

	apply_reset_cpu_pm(cpu);

	return;
}

/* quatro55xx_cpu_in_wfi_pm(u32 cpu, u32 cluster)
 *
 * @cpu: mpidr[7:0] bitfield describing CPU affinity level within cluster
 * @cluster: mpidr[15:8] bitfield describing cluster affinity level
 *
 * @return: non-zero if and only if the specified CPU is in WFI
 *
 * Take care when interpreting the result of this function: a CPU might
 * be in WFI temporarily due to idle, and is not necessarily safely
 * parked.
 */

/* static TODO: delete me */ int quatro55xx_cpu_in_wfi_pm(u32 cpu, u32 cluster)
{
	unsigned long reg_value;
	void __iomem* mapped_address;
	unsigned long comparison_value;
	int in_wfi;

	if ((cpu != 0 && cpu != 1) || cluster != 1)
		return 0;

	//http://bling.zma.zoran.com/asic/fire10/A0/html/regs/sys/A15SLPCSR.html
	mapped_address = ioremap(0x043B0000, 0x30); /* TODO: do something better here, maybe dl -related. */

	reg_value = readl((void *)mapped_address+0x28);
	comparison_value = 1 << (2+cpu);

	iounmap(mapped_address);
	
	in_wfi = reg_value & comparison_value;
	return in_wfi;
}

static void lift_reset_cpu_pm(unsigned int cpu)
{
	unsigned long reg_value;
	void __iomem* mapped_address;

	if (cpu != 0 && cpu != 1)
		return;

	mapped_address = ioremap(0x043B0024, 4);
	spin_lock(&reset_lock);

	/* Lifting at least one CPU, so lift common resets. */
	reg_value = readl((void *)mapped_address); /* TODO: do something better here, maybe dl-related. */
	reg_value |= 0x300;  /* Shared debug, timer, and L2. */
	writel(reg_value, (void *)mapped_address); /* TODO: do something better here, maybe dl-related. */
		
	reg_value |= reset_constants[cpu];
	writel(reg_value, (void *)mapped_address); /* TODO: do something better here, maybe dl-related. */

	spin_unlock(&reset_lock);
	iounmap(mapped_address);

	return;
}

static void quatro55xx_pm_boot_secondary(unsigned int cpu) //, struct task_struct *idle)
{
	power_up_cpu_pm(cpu);

	/* Tell boot ROM where to execute from. */
	/* TODO: refactor this once we have B0 silicon to set both A0 and B0 start addresses here. */
	quatro55xx_pm_set_boot_vector();

	lift_reset_cpu_pm(cpu);
}

static int quatro55xx_pm_power_up(unsigned int cpu, unsigned int cluster)
{
	pr_debug("%s: cpu %u cluster %u\n", __func__, cpu, cluster);
	if (cluster >= QUATRO55XX_CLUSTERS || cpu >= quatro55xx_nr_cpus[cluster]) {
	}

	/*
	 * Since this is called with IRQs enabled, and no arch_spin_lock_irq
	 * variant exists, we need to disable IRQs manually here.
	 */
	local_irq_disable();
	arch_spin_lock(&quatro55xx_pm_lock);

	quatro55xx_pm_use_count[cpu][cluster]++;
	quatro55xx_set_resume_addr_pm(cluster, cpu,
				      virt_to_phys(mcpm_entry_point));
	quatro55xx_pm_boot_secondary(cpu);

	arch_spin_unlock(&quatro55xx_pm_lock);
	local_irq_enable();

	return 0;
}

static void quatro55xx_pm_down(u64 residency)
{
	unsigned int mpidr, cpu, cluster;
	bool last_man = false;

	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	pr_debug("%s: cpu %u cluster %u\n", __func__, cpu, cluster);
	BUG_ON(cluster >= QUATRO55XX_CLUSTERS || cpu >= QUATRO55XX_MAX_CPUS_PER_CLUSTER);

	__mcpm_cpu_going_down(cpu, cluster);

	arch_spin_lock(&quatro55xx_pm_lock);
	BUG_ON(__mcpm_cluster_state(cluster) != CLUSTER_UP);
	quatro55xx_pm_use_count[cpu][cluster]--;
	if (quatro55xx_pm_use_count[cpu][cluster] == 0) {
		if (quatro55xx_cluster_unused(cluster)) {
			last_man = true;
		}
	}

	if (last_man && __mcpm_outbound_enter_critical(cpu, cluster)) {
		arch_spin_unlock(&quatro55xx_pm_lock);

		if (read_cpuid_part_number() == ARM_CPU_PART_CORTEX_A15) {
			/*
			 * On the Cortex-A15 we need to disable
			 * L2 prefetching before flushing the cache.
			 */
			asm volatile(
			"mcr	p15, 1, %0, c15, c0, 3 \n\t"
			"isb	\n\t"
			"dsb	"
			: : "r" (0x400) );
		}

		v7_exit_coherency_flush(all);

		cci_disable_port_by_cpu(mpidr);

		__mcpm_outbound_leave_critical(cluster, CLUSTER_DOWN);
	} else {
		/*
		 * If last man then undo any setup done previously.
		 */
		if (last_man) {
		}

		arch_spin_unlock(&quatro55xx_pm_lock);

		v7_exit_coherency_flush(louis);
	}

	__mcpm_cpu_down(cpu, cluster);

	/* Now we are prepared for power-down, let them do it: */
	wfi();

	/* Not dead at this point?  Let our caller cope. */
}

static void quatro55xx_pm_power_down(void)
{
	quatro55xx_cpu_in_wfi_pm(1,1);
	quatro55xx_pm_down(0);
}

static int quatro55xx_core_in_reset_pm(unsigned int cpu, unsigned int cluster)
{
	unsigned long reg_value;
	void __iomem* mapped_address;

	if ((cpu != 0 && cpu != 1) || cluster != 1)
		return 0;

	mapped_address = ioremap(0x043B0000, 0x40); /* TODO: do something better here, maybe dl -related. */

	reg_value = readl((void *)mapped_address+0x24);

	iounmap(mapped_address);

	if (reg_value && reset_constants[cpu] == reset_constants[cpu]) {
		return 0;
	}
	else {
		return 1;
	}
}

#define POLL_MSEC 10
#define TIMEOUT_MSEC 1000

static int quatro55xx_pm_wait_for_powerdown(unsigned int cpu, unsigned int cluster)
{
	unsigned tries;

	pr_debug("%s: cpu %u cluster %u\n", __func__, cpu, cluster);
	BUG_ON(cluster >= QUATRO55XX_CLUSTERS || cpu >= QUATRO55XX_MAX_CPUS_PER_CLUSTER);

	for (tries = 0; tries < TIMEOUT_MSEC / POLL_MSEC; ++tries) {
		/*
		 * Only examine the hardware state if the target CPU has
		 * caught up at least as far as quatro55xx_pm_down():
		 */
		if (ACCESS_ONCE(quatro55xx_pm_use_count[cpu][cluster]) == 0) {
			pr_debug("%s(cpu=%u, cluster=%u): RESET_CTRL = 0x%08X\n",
				 __func__, cpu, cluster,
				/* readl_relaxed(scc + RESET_CTRL) */ 0);

			/*
			 * We need the CPU to reach WFI, but the power
			 * controller may put the cluster in reset and
			 * power it off as soon as that happens, before
			 * we have a chance to see STANDBYWFI.
			 *
			 * So we need to check for both conditions:
			 */
			if (quatro55xx_core_in_reset_pm(cpu, cluster) ||
				quatro55xx_cpu_in_wfi_pm(cpu, cluster)) {
				power_down_cpu_pm(cpu);
				return 0; /* success: the CPU is halted */
			}
		}

		/* Otherwise, wait and retry: */
		msleep(POLL_MSEC);
	}

	return -ETIMEDOUT; /* timeout */
}

static void quatro55xx_pm_suspend(u64 residency)
{
	unsigned int mpidr, cpu, cluster;

	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);
	quatro55xx_set_resume_addr_pm(cluster, cpu, virt_to_phys(mcpm_entry_point));
	quatro55xx_pm_down(residency);
}

static void quatro55xx_pm_powered_up(void)
{
	unsigned int mpidr, cpu, cluster;
	unsigned long flags;

	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	pr_debug("%s: cpu %u cluster %u\n", __func__, cpu, cluster);
	BUG_ON(cluster >= QUATRO55XX_CLUSTERS || cpu >= QUATRO55XX_MAX_CPUS_PER_CLUSTER);

	local_irq_save(flags);
	arch_spin_lock(&quatro55xx_pm_lock);

	if (!quatro55xx_pm_use_count[cpu][cluster])
		quatro55xx_pm_use_count[cpu][cluster] = 1;

	quatro55xx_set_resume_addr_pm(cluster, cpu, 0);

	arch_spin_unlock(&quatro55xx_pm_lock);
	local_irq_restore(flags);
}

static const struct mcpm_platform_ops quatro55xx_pm_power_ops = {
	.power_up		= quatro55xx_pm_power_up,
	.power_down		= quatro55xx_pm_power_down,
	.wait_for_powerdown	= quatro55xx_pm_wait_for_powerdown,
	.suspend		= quatro55xx_pm_suspend,
	.powered_up		= quatro55xx_pm_powered_up,
};

static bool __init quatro55xx_pm_usage_count_init(void)
{
	unsigned int mpidr, cpu, cluster;

	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	pr_debug("%s: cpu %u cluster %u\n", __func__, cpu, cluster);
	if (cluster >= QUATRO55XX_CLUSTERS || cpu >= quatro55xx_nr_cpus[cluster]) {
		pr_err("%s: boot CPU is out of bound!\n", __func__);
		return false;
	}
	quatro55xx_pm_use_count[cpu][cluster] = 1;
	return true;
}

/*
 * Enable cluster-level coherency, in preparation for turning on the MMU.
 */
static void __naked quatro55xx_pm_power_up_setup(unsigned int affinity_level)
{
	asm volatile (" \n"
"	cmp	r0, #1 \n"
"	bxne	lr \n"
"	b	cci_enable_port_for_self ");
}

static int __init quatro55xx_pm_init(void)
{
	int ret;

	quatro55xx_nr_cpus[0] = 1; /// TODO: do this dynamically
	quatro55xx_nr_cpus[1] = 2; /// TODO: do this dynamically

	if (!cci_probed()) {
		return -ENODEV;
	}

	if (!quatro55xx_pm_usage_count_init()) {
		return -EINVAL;
	}

	ret = mcpm_platform_register(&quatro55xx_pm_power_ops);
	if (!ret) {
		mcpm_sync_init(quatro55xx_pm_power_up_setup);
		if (quatro55xx_chip_version() == 0) {
			// quatro55xx_a0_pm_prepare_cpus(3);
		}
		else {
			// quatro55xx_b0_pm_prepare_cpus(3); TODO: remove this
		}
		pr_info("Quatro55xx power management initialized\n");
	}
	else {
		pr_info("Quatro55xx power management failed, bad return %d from mcpm_platform_register()\n", (int)mcpm_platform_register);
	}

	return ret;
}

early_initcall(quatro55xx_pm_init);
