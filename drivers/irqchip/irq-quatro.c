/*
 * drivers/irqchip/irq-quatro.c
 *
 * Copyright (c) 2014, 2015 Linux Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/cpu.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/irqdomain.h>
#include <linux/syscore_ops.h>
#include <asm/mach/arch.h>
#include <asm/exception.h>
#include <asm/smp_plat.h>
#include <asm/mach/irq.h>

#include "irqchip.h"

#if defined(CONFIG_SOC_QUATRO4310)
	#define QUATRO_NUM_IRQS		64
	#define MECRA_OFF	0x10
	#define EXCLRA_OFF	0x1C
	#define EXC0A_OFF	0x3C
	#define EXC1A_OFF	0x40
	#define EXMSK0A_OFF	0x54
	#define EXMSK1A_OFF	0x58
	#define EXMSK_SHIFT 3
#elif defined(CONFIG_SOC_QUATRO4500)
	#define QUATRO_NUM_IRQS		128
	#define MECRA_OFF	0x10
	#define EXCLRA_OFF	0x20
	#define EXSETA_OFF	0x30
	#define EXC0A_OFF	0x50
	#define EXC1A_OFF	0x54
	#define EXC2A_OFF	0x58
	#define EXC3A_OFF	0x5C
	#define EXMSK0A_OFF	0x90
	#define EXMSK1A_OFF	0x94
	#define EXMSK2A_OFF	0x98
	#define EXMSK3A_OFF	0x9C
	#define EXMSK_SHIFT 4
	#define IRQ_A2BREQ   (32 + 28)
	#define IRQ_B2AREQ   (32 + 29)
#elif defined(CONFIG_SOC_QUATRO5300)
	#define QUATRO_NUM_IRQS		128
	#define MECRA_OFF	0x10
	#define EXCLRA_OFF	0x20
	#define EXC0A_OFF	0x48
	#define EXC1A_OFF	0x4C
	#define EXMSK0A_OFF	0x68
	#define EXMSK1A_OFF	0x6C
	#define EXMSK_SHIFT 3
#endif


static u8 __iomem *irqc_base;
static struct irq_domain *quatro_irqdomain;

#ifdef CONFIG_SMP
static int ipifunc[2];
static u8 affinity[QUATRO_NUM_IRQS]; /* cpu affinity */
#endif

static inline void quatro_clear_hwirq(int hwirq)
{
	unsigned mask = 1 << (hwirq & 0x1f);
	unsigned reg = EXCLRA_OFF + ((hwirq >> 5) << 2);

	__raw_writel(mask, irqc_base + reg);
}

/* used by modules to clear those interrupt types that
*  can't be cleared until something is read, like DSP mailbox, etc.
*/
inline void quatro_clear_irq(int irq)
{
	struct irq_desc *desc;
	
	desc = irq_to_desc(irq);
	if (desc)
	{
		quatro_clear_hwirq(desc->irq_data.hwirq);
	}
}
EXPORT_SYMBOL_GPL(quatro_clear_irq);

static void quatro_ack_irq(struct irq_data *d)
{
	quatro_clear_hwirq(d->hwirq);
}

static void quatro_mask_irq(struct irq_data *d)
{
	unsigned oldmask, mask = 1 << (d->hwirq & 0x1f);
	unsigned reg = (d->hwirq >> 5) << EXMSK_SHIFT;

#if defined(CONFIG_SMP)&& defined(EXMSK3A_OFF)
	if (affinity[d->hwirq] == 1)
		reg += EXMSK3A_OFF;
	else
#endif
	reg += EXMSK1A_OFF;
	
	oldmask = __raw_readl(irqc_base + reg);
	__raw_writel(oldmask &~ mask, irqc_base + reg);
}

static void quatro_unmask_irq(struct irq_data *d)
{
	unsigned oldmask, mask = 1 << (d->hwirq & 0x1f);
	unsigned reg = (d->hwirq >> 5) << EXMSK_SHIFT;

#if defined(CONFIG_SMP)&& defined(EXMSK3A_OFF)
	if (affinity[d->hwirq] == 1)
		reg += EXMSK3A_OFF;
	else
#endif
	reg += EXMSK1A_OFF;

	oldmask = __raw_readl(irqc_base + reg);
	__raw_writel(oldmask | mask, irqc_base + reg);
}

#ifdef CONFIG_SMP
static int quatro_set_affinity(struct irq_data *d, const struct cpumask *mask_val,
			    bool force)
{
	unsigned oldmask, newmask = 1 << (d->hwirq & 0x1f);
	unsigned reg = (d->hwirq >> 5) << EXMSK_SHIFT;

	unsigned int cpu;

	if (!force)
		cpu = cpumask_any_and(mask_val, cpu_online_mask);
	else
		cpu = cpumask_first(mask_val);

	if (cpu < 0 || cpu > 1)
		return -EINVAL;
	
	if (affinity[d->hwirq] == 1)
		reg += EXMSK3A_OFF;
	else
		reg += EXMSK1A_OFF;
	
	oldmask = __raw_readl(irqc_base + reg);
	quatro_mask_irq(d);
	affinity[d->hwirq] = cpu;
	if (oldmask & newmask)
		quatro_unmask_irq(d);
	return IRQ_SET_MASK_OK;
}
#endif

static struct irq_chip irq_quatro_chip_ops = {
	.irq_ack = quatro_ack_irq,
	.irq_mask = quatro_mask_irq,
	.irq_unmask = quatro_unmask_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity = quatro_set_affinity,
#endif
};

asmlinkage void __exception_irq_entry quatro_handle_irq(struct pt_regs *regs)
{
	u32  exc;
	u32  irq, irqnr;
	u8 __iomem *excsrc;
	static DEFINE_PER_CPU(int, percpu_exorder);
	int cpu;

#ifdef CONFIG_SMP
	/* only look at exception causes pertaining
	 * to this particular cpu
	 */
	cpu = smp_processor_id();
	switch (cpu)
	{
	default:
	case 0:
		excsrc = irqc_base + EXC1A_OFF;	
		break;
	case 1:
		excsrc = irqc_base + EXC3A_OFF;	
		break;
	}
#else
	cpu = 0;
	excsrc = irqc_base + EXC1A_OFF;	
#endif
	irq = QUATRO_NUM_IRQS + 1;

	switch (per_cpu(percpu_exorder, cpu))
	{
	case 0:
		if ((exc = __raw_readl(excsrc + 0)) != 0)
			irq = 0;
		else if ((exc = __raw_readl(excsrc + (1 << EXMSK_SHIFT))) != 0)
			irq = 32;
		#if (QUATRO_NUM_IRQS >= 64)
		else if ((exc = __raw_readl(excsrc + (2 << EXMSK_SHIFT))) != 0)
			irq = 64;
		#endif
		#if (QUATRO_NUM_IRQS >= 96)
		else if ((exc = __raw_readl(excsrc + (3 << EXMSK_SHIFT))) != 0)
			irq = 96;
		#endif
		break;
	case 32:
		if ((exc = __raw_readl(excsrc + (1 << EXMSK_SHIFT))) != 0)
			irq = 32;
		#if (QUATRO_NUM_IRQS >= 64)
		else if ((exc = __raw_readl(excsrc + (2 << EXMSK_SHIFT))) != 0)
			irq = 64;
		#endif
		#if (QUATRO_NUM_IRQS >= 96)
		else if ((exc = __raw_readl(excsrc + (3 << EXMSK_SHIFT))) != 0)
			irq = 96;
		#endif
		else if ((exc = __raw_readl(excsrc + 0)) != 0)
			irq = 0;
		break;
	case 64:
		if ((exc = __raw_readl(excsrc + (2 << EXMSK_SHIFT))) != 0)
			irq = 64;
		#if (QUATRO_NUM_IRQS >= 96)
		else if ((exc = __raw_readl(excsrc + (3 << EXMSK_SHIFT))) != 0)
			irq = 96;
		#endif
		else if ((exc = __raw_readl(excsrc + 0)) != 0)
			irq = 0;
		else if ((exc = __raw_readl(excsrc + (1 << EXMSK_SHIFT))) != 0)
			irq = 32;
		break;
	case 96:
		if ((exc = __raw_readl(excsrc + (3 << EXMSK_SHIFT))) != 0)
			irq = 96;
		else if ((exc = __raw_readl(excsrc + 0)) != 0)
			irq = 0;
		else if ((exc = __raw_readl(excsrc + (1 << EXMSK_SHIFT))) != 0)
			irq = 32;
		else if ((exc = __raw_readl(excsrc + (2 << EXMSK_SHIFT))) != 0)
			irq = 64;
		break;
	default:
		/* can't happen */
		exc = 0;
		panic("Bad exorder\n");
		break;
	}
	per_cpu(percpu_exorder, cpu) += 32;
	if (per_cpu(percpu_exorder, cpu) >= QUATRO_NUM_IRQS)
		per_cpu(percpu_exorder, cpu) = 0;
	
	if (irq > QUATRO_NUM_IRQS) {
		printk("1A %08X\n", __raw_readl(irqc_base + EXMSK1A_OFF + (0 << EXMSK_SHIFT)));
		printk("1B %08X\n", __raw_readl(irqc_base + EXMSK1A_OFF + (1 << EXMSK_SHIFT)));
		printk("1C %08X\n", __raw_readl(irqc_base + EXMSK1A_OFF + (2 << EXMSK_SHIFT)));
		printk("1D %08X\n", __raw_readl(irqc_base + EXMSK1A_OFF + (3 << EXMSK_SHIFT)));
#ifdef EXMSK3A_OFF
		printk("3A %08X\n", __raw_readl(irqc_base + EXMSK3A_OFF + (0 << EXMSK_SHIFT)));
		printk("3B %08X\n", __raw_readl(irqc_base + EXMSK3A_OFF + (1 << EXMSK_SHIFT)));
		printk("3C %08X\n", __raw_readl(irqc_base + EXMSK3A_OFF + (2 << EXMSK_SHIFT)));
		printk("3D %08X\n", __raw_readl(irqc_base + EXMSK3A_OFF + (3 << EXMSK_SHIFT)));
#endif
		panic("No interrupt!\n");
	}

	/* this code adds the lowest set bit number in exc to irq. it
	*  wants to do this in as few instructions as possible which
	*  is why there is basically a binary search here for the low bit
	*/
	if (! (exc << 16)) {
		exc >>= 16;
		irq += 16;
	}
	if (! (exc & 0xFF)) {
		exc >>= 8;
		irq += 8;
	}
	if (! (exc & 0xF)) {
		exc >>= 4;
		irq += 4;
	}
	if (! (exc & 3)) {
		exc >>= 2;
		irq += 2;
	}
	if (! (exc & 1)) {
		irq += 1;
	}
#ifdef CONFIG_SMP
	if (irq == IRQ_A2BREQ || irq == IRQ_B2AREQ) {
		unsigned mask = 1 << (irq & 0x1f);
		unsigned reg = EXCLRA_OFF + ((irq >> 5) << 2);
		int tocpu = (irq == IRQ_B2AREQ) ? 0 : 1;

		__raw_writel(mask, irqc_base + reg);
		handle_IPI(ipifunc[tocpu], regs); 
		return;
	}
#endif
	irqnr = irq_find_mapping(quatro_irqdomain, irq);
	handle_IRQ(irqnr, regs);
}

static int quatro_irq_domain_map(struct irq_domain *d, unsigned int virq,
				irq_hw_number_t hw)
{
	irq_set_chip_and_handler(virq, &irq_quatro_chip_ops, handle_level_irq);
	set_irq_flags(virq, IRQF_VALID);
	return 0;
}

#ifdef CONFIG_SMP
/*
 * We use a2b req and b2a req
 */
void quatro_cross_call(const struct cpumask *cpumask,
				      unsigned int irqin)
{
	int cpu;
	u32 irq, mask, reg;

	/* Step through logical CPU mask and interrupt the 
	 * physical cpu
	 */
	for_each_cpu(cpu, cpumask) {
		/*
		 * Ensure that stores to Normal memory are visible to the
		 * other CPUs before issuing the IPI.
		 */
		dsb();
	
		switch(cpu)
		{
		case 1: /* 0 calling 1 */
			/* force an A to B request event */
			ipifunc[1] = irqin;
			irq = IRQ_A2BREQ;
			mask = 1 << (irq & 0x1f);
			reg = (irq >> 5) << 2;
			__raw_writel(mask, irqc_base + EXSETA_OFF + reg);
			break;
	
		case 0: /* 1 calling 0 */
			/* force an B to A request event */
			ipifunc[0] = irqin;
			irq = IRQ_B2AREQ;
			mask = 1 << (irq & 0x1f);
			reg = (irq >> 5) << 2;
			__raw_writel(mask, irqc_base + EXSETA_OFF + reg);
			break;
			
		default:
			panic("cross call for bad cpu\n");
			break;
		}
	}
}

#endif 

static struct irq_domain_ops quatro_irq_domain_ops = {
	.map = quatro_irq_domain_map,
	.xlate = irq_domain_xlate_onecell,
};

static int __init quatro_irq_init(struct device_node *np, struct device_node *parent)
{
	int irq;
	u32 reg;
#if defined(CONFIG_SMP) && defined(EXMSK3A_OFF)
	u32 mask, oldmask;
#endif
	irqc_base = of_iomap(np, 0);

	if (! irqc_base)
		panic("unable to map intc cpu registers\n");

	/* clear and disable all irqs
	*/
	for(irq = 0; irq < QUATRO_NUM_IRQS; irq+= 32) {
		reg = (irq >> 5) << EXMSK_SHIFT;

		__raw_writel(0, irqc_base + EXMSK0A_OFF + reg);
		__raw_writel(0, irqc_base + EXMSK1A_OFF + reg);
#if defined(EXMSK3B)
		__raw_writel(0, irqc_base + EXMSK2A_OFF + reg);
		__raw_writel(0, irqc_base + EXMSK3A_OFF + reg);
		
#endif
	}
	__raw_writel(0xffffffff, irqc_base + EXCLRA_OFF);
	__raw_writel(0xffffffff, irqc_base + EXCLRA_OFF + 4);
	__raw_writel(0xffffffff, irqc_base + EXCLRA_OFF + 8);
	__raw_writel(0xffffffff, irqc_base + EXCLRA_OFF + 12);
	
#ifdef CONFIG_SMP
	for(irq = 0; irq < QUATRO_NUM_IRQS; irq++)
		affinity[irq] = 0;
#endif
#if defined(CONFIG_SMP) && defined(EXMSK3A_OFF)
	/* reenable the A->B on cpu1 and B->A req interrupt on cpu0 here since no-one else will
	*/
	irq = IRQ_A2BREQ;
	mask = 1 << (irq & 0x1f);
	reg = (irq >> 5) << EXMSK_SHIFT;
	oldmask = __raw_readl(irqc_base + EXMSK3A_OFF + reg);
	__raw_writel(oldmask | mask, irqc_base + EXMSK3A_OFF + reg);

	irq = IRQ_B2AREQ;
	mask = 1 << (irq & 0x1f);
	reg = (irq >> 5) << EXMSK_SHIFT;
	oldmask = __raw_readl(irqc_base + EXMSK1A_OFF + reg);
	__raw_writel(oldmask | mask, irqc_base + EXMSK1A_OFF + reg);

	/* and set affinity for a->b to cpu 1 */
	affinity[IRQ_A2BREQ] = 1;

	/* setup system cross call function for IPI
	*/
	set_smp_cross_call(quatro_cross_call);
	
#endif
	quatro_irqdomain = irq_domain_add_linear(np, QUATRO_NUM_IRQS, &quatro_irq_domain_ops, NULL);
	set_handle_irq(quatro_handle_irq);
	return 0;
}
IRQCHIP_DECLARE(quatro_intc, "csr,quatro-intc", quatro_irq_init);

