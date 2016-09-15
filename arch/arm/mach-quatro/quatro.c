/*
 * arch/arm/mach-quatro/quatro.c
 *
 * Copyright (c) 2014 - 2016 Linux Foundation.
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
#include <asm/sizes.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <asm/mcpm.h>
#include "quatro.h"
#include <asm/mach/map.h>
#ifdef CONFIG_SOC_QUATRO5300
#include <asm/hardware/cache-l2quatro.h>
#endif

#define QUATRO_DDRMEMCTRL                0x04314800
#define QUATRO_EXTENDED_ADDRESS_MODE_OFF 0x00000080
#define QUATRO_EXTENDED_ADDRESS_MODE__EAM__MASK        0x00000001

static struct map_desc quatro_io_desc[] __initdata = {
	/* uart, for early printk */
	{
		.virtual = (unsigned long) QUATRO_UART0_VIRT_BASE,
		.pfn = __phys_to_pfn(QUATRO_UART0_PHYS_BASE),
		.length = QUATRO_UART0_SIZE,
		.type = MT_DEVICE,
	},
#ifdef CONFIG_SOC_QUATRO4500
#ifdef CONFIG_SMP
	/* MISC register, used for SMP */
	{
		.virtual = (unsigned long) QUATRO_MISC_VIRT_BASE,
		.pfn = __phys_to_pfn(QUATRO_MISC_PHYS_BASE),
		.length = 64,
		.type = MT_DEVICE,
	},
#endif
	/* SCU on 4500 SMP */
	{
		.virtual = (unsigned long) QUATRO_SCU_VIRT_BASE,
		.pfn = __phys_to_pfn(QUATRO_SCU_PHYS_BASE),
		.length = QUATRO_SCU_SIZE,
		.type = MT_DEVICE,
	},
#endif
};

#ifndef CONFIG_SOC_QUATRO5500
static void __init quatro_init_late(void)
{
	//quatro_pm_init();
}
#endif

#ifdef CONFIG_SOC_QUATRO5300
/* This is a workaround for a barrier errata in the 53xx SoC.
   To be sure a write barrier is complete, a read from
   non-cached RAM is necessary.
*/
volatile unsigned long barrier_workaround_val_g = 0;
volatile unsigned long *barrier_workaround_addr_g = 0;
EXPORT_SYMBOL(barrier_workaround_val_g);
EXPORT_SYMBOL(barrier_workaround_addr_g);
#endif

static void __init quatro_init_machine(void)
{
#ifdef CONFIG_SOC_QUATRO5300
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "csr,quatro53-barrier-workaround");
	if (!node) {
		pr_err("%s: could not find quatro53-barrier-workaround devicetree node\n", __func__);
		return;
	}
	barrier_workaround_addr_g = (unsigned long*)of_iomap(node, 0);
#ifdef CONFIG_CACHE_L2QUATRO
	if (l2quatro_of_init() != 0) {
		pr_err("%s: could not find or map quatro-l2cache in devicetree node\n", __func__);
		return;
	}
#endif
#endif

	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

#ifndef CONFIG_SOC_QUATRO5500
static __init void quatro_map_io(void)
{
	iotable_init(quatro_io_desc, ARRAY_SIZE(quatro_io_desc));
}
#endif

#ifdef CONFIG_SOC_QUATRO4310
static const char *q4310_dt_match[] __initconst = {
	"csr,quatro-4310",
	NULL
};

DT_MACHINE_START(Q4310_DT, "Quatro 4310 (Flattened Device Tree)")
	/* Maintainer: Brian Dodge <brian.dodge@csr.com> */
	.map_io         = quatro_map_io,
	.init_machine	= quatro_init_machine,
	.init_late	= quatro_init_late,
	.dt_compat      = q4310_dt_match,
MACHINE_END
#endif

#ifdef CONFIG_SOC_QUATRO4500
static const char *q4500_dt_match[] __initconst = {
	"csr,quatro-4500",
	NULL
};

DT_MACHINE_START(Q4500_DT, "Quatro 4500 (Flattened Device Tree)")
	/* Maintainer: Brian Dodge <brian.dodge@csr.com> */
	.map_io         = quatro_map_io,
	.init_machine	= quatro_init_machine,
	.init_late	= quatro_init_late,
	.dt_compat      = q4500_dt_match,
	.smp		=	smp_ops(quatro_smp_ops),
MACHINE_END
#endif

#ifdef CONFIG_SOC_QUATRO5300
static const char *q5300_dt_match[] __initconst = {
	"csr,quatro-5300",
	NULL
};

extern asmlinkage void __exception_irq_entry quatro_handle_irq(struct pt_regs *regs);

DT_MACHINE_START(Q5300_DT, "Quatro 5300 (Flattened Device Tree)")
	/* Maintainer: Brian Dodge <brian.dodge@csr.com> */
	.handle_irq	= quatro_handle_irq,
	.map_io         = quatro_map_io,
	.init_machine	= quatro_init_machine,
	.init_late	= quatro_init_late,
	.dt_compat      = q5300_dt_match,
MACHINE_END
#endif

#ifdef CONFIG_SOC_QUATRO5500
static const char *q5500_dt_match[] __initconst = {
	"csr,quatro-5500",
	NULL
};

static void __init quatro55xx_smp_init_cpus(void)
{
	int index;

	for (index=0; index<3; index++) {
		set_cpu_possible(index, true);
	}
}

static bool __init quatro55xx_smp_init_ops(void)
{
	struct device_node *node;
	node = of_find_compatible_node(NULL, NULL, "arm,cci-400");
	if (node && of_device_is_available(node)) {
		mcpm_smp_set_ops();
		return true;
	} 
	else {
	  return false;
	}
}

struct smp_operations __initdata quatro55xx_smp_ops = {
	.smp_init_cpus		= quatro55xx_smp_init_cpus,
};

DT_MACHINE_START(Q5500_DT, "Quatro 5500 (Flattened Device Tree)")
	.init_machine	= quatro_init_machine,
	.dt_compat      = q5500_dt_match,
	.smp		= smp_ops(quatro55xx_smp_ops),
	.smp_init	= smp_init_ops(quatro55xx_smp_init_ops),
MACHINE_END
#endif
