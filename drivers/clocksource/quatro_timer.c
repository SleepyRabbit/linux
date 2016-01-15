/*
 * drivers/clocksource/quatro_timer.c
 *
 * Copyright (c) 2014, 2015 Linux Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/bitops.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sched_clock.h>
#include <linux/clocksource.h>
#include <linux/bitops.h>
#include <asm/irq.h>

/* Quatro timer block has:

   - High resolution timer 0 (64-bit) - used for HRT if configured
   - High resolution timer 1 (32-bit) - used as clocksource (free running schedule clock)
   - System timer 0 - used as clockevent (global timer)
   - System timer 1 - CPU 0 local timer
   - System timer 2 - CPU 1 local timer (if we have that many CPUs),
   - System timer 3 -  ...
   - System timer 4 -  ...
   - System timer 5 -  ...
   - System timer 6 -  ... up to 5 cpus if needed
   - Watchdog timer - unused by this driver.

*/
#define QUATRO_TIMER_MAX_CPU 5 /* for local timer purposes */

#define XTAL_MHZ 24
#define XTAL_HZ (XTAL_MHZ*1000*1000)

#ifdef CONFIG_SOC_QUATRO5500

#define SYSCG_BASE       	0x04020400
#define SYSCG_CLKMUXCTRL1_OFF	0x30
#define SYSCG_CLKSWSTAT1_OFF	0x14

#define SYSPLL_BASE      0x04020000
#define SYSPLL_DIV12_OFF 0x18

#define SYSCG_CLKSWSTAT1__SYS_IS_MUX_CLK__MASK	0x00040000
#define SYSCG_CLKSWSTAT1__SYS_IS_LP_CLK__MASK	0x00020000
#define SYSCG_CLKSWSTAT1__SYS_IS_XIN0_CLK__MASK	0x00010000
#define SYSCG_CLKMUXCTRL1__SYS__SHIFT       	0
#define SYSCG_CLKMUXCTRL1__SYS__MASK        	0x00000003

u8 __iomem *clkgen, *pllbase;

static u32 sysPllDivider(int d)
{
#ifdef Q5500EVE
	/* Hard-coded dividers; used by emulators since analog PLL is not emulated */
	switch (d) {
	case 1:
		return 4;
	case 2: case 3:
		return d;
	case 4: case 5: case 6: case 7: case 8:
		return d + 1;
	default:
		return 0;
	}
#else
	if (d >= 1 && d <= 8) {
		u32 shift = (d & 1) * 4;
		u32 clk = SYSPLL_DIV12_OFF + ((d - 1) / 2) * 4;

		return ((readl(pllbase + clk) >> shift) & 0xf) + 1;
	}
	return 0;
#endif /* Q5500EVE */
}

static u32 lowPowerClockDivider(void)
{
	/** @todo Calculate Low Power Divider from SYSCG_CLKDIVCTRL_LP */
	return 16;
}

static u32 sysPllOutputFrequency(int output)
{
	// target pll output at 2.4GHz
    return 2400UL*1000*1000;
}

static unsigned sysFrequency(void)
{
	u32 sw;
	u32 freq;
	
	clkgen  = ioremap(SYSCG_BASE, 4096);
	pllbase = ioremap(SYSPLL_BASE, 4096);

	sw = readl(clkgen + SYSCG_CLKSWSTAT1_OFF);
	if ((sw & SYSCG_CLKSWSTAT1__SYS_IS_MUX_CLK__MASK) != 0) {
		u32 mux = ((readl(clkgen + SYSCG_CLKMUXCTRL1_OFF) &
					SYSCG_CLKMUXCTRL1__SYS__MASK) >>
					SYSCG_CLKMUXCTRL1__SYS__SHIFT);
		freq = sysPllOutputFrequency(0) / sysPllDivider(mux + 5); // 5..8
	}
	else if ((sw & SYSCG_CLKSWSTAT1__SYS_IS_LP_CLK__MASK) != 0)
		freq = sysPllOutputFrequency(0) / lowPowerClockDivider();
	else if ((sw & SYSCG_CLKSWSTAT1__SYS_IS_XIN0_CLK__MASK) != 0)
		freq = XTAL_MHZ * 1000 * 1000;
	else
		freq = 0;
	iounmap(pllbase);
	iounmap(clkgen);
	return freq;
}
#endif /* CONFIG_SOC_QUATRO5500 */

#ifdef CONFIG_SOC_QUATRO5300

#define CLKGEN_BASE 0xF0031000
#define CLKGENSYSPLLCTRL1_OFF 0x00000090

static unsigned sysPllOutputFrequency(void)
{
	unsigned pll_ctrl, divr, fvcoref, divf, fvcoout, divq, fclkout;
	u8 __iomem *clkgen;
	
	clkgen = ioremap(CLKGEN_BASE, SZ_256);
	if (! clkgen) {
		panic("Failed to map timer base\n");
		return 0;
	}
	pll_ctrl = __raw_readl(clkgen + CLKGENSYSPLLCTRL1_OFF);
	divr = (pll_ctrl >> 12) & 7;
	fvcoref = (XTAL_HZ) / (divr + 1);
	divf = pll_ctrl & 0x1ff;
	fvcoout = fvcoref * 2 * (divf + 1);
	divq = (pll_ctrl >> 20) & 7;
	fclkout = fvcoout >> divq;
	return fclkout;
}

static unsigned sysFrequency(void)
{
	return sysPllOutputFrequency() / 4;
}

#endif /* CONFIG_SOC_QUATRO5300 */

#ifdef CONFIG_SOC_QUATRO4500

#define CLKGEN_BASE 0xE0208000
#define SYSTEM_PLL_DIV_CONFIG_OFF 0x0020

static unsigned sysPllOutputFrequency(void)
{
	unsigned long a, f, ldiv, m, n;
	volatile unsigned long div_val;
	u8 __iomem *clkgen;

	clkgen = ioremap(CLKGEN_BASE, SZ_256);
	if (! clkgen) {
		panic("Failed to map timer base\n");
		return 0;
	}   
	div_val = __raw_readl(clkgen + SYSTEM_PLL_DIV_CONFIG_OFF);
	
	a =  (div_val >> 29) & 0x7;   /* sc  */
	f = (div_val >> 8) & 0xFF;    /* fcd */
	ldiv = (div_val >> 16) & 0xF; /* vcd */
	m =  (div_val >> 24) & 0x1F;  /* dr  */
	n = (div_val & 0xFF);   	  /* cdc */
	
	/* Formula to calculate operating frequency of Quatro 45xx PLLs in
	 * "Integer-N" or "Fractional-N" mode:
	 * freqMHz = ( Fref/(m+1) ) * ( (8*(n+1) + a + f/256 ) )/(ldiv+1) );
	 *
	 * refactored:
	 */
	return ( 1000 * 						\
		 ( ( 1000 * XTAL_MHZ * (8*(n+1) + a + (f>>8) ) )	\
		   / ( (m+1) * (ldiv+1) ) ) 				\
		);
}

static unsigned sysFrequency(void)
{
	return sysPllOutputFrequency() / 2;
}

#endif /* CONFIG_SOC_QUATRO4500 */
	
/**** System Timer functions, see bottom section for HRT (simple clock, hrt, function)
*/
#define SYT_LOAD	0x00000001
#define SYT_START   	0x00000002
#define SYT_CONT_MODE   0x00000004

#define SYT_TCN_OFF 0
#define SYT_CNT_OFF 4
#define SYT_CTL_OFF 8

/* offset between each timer, in register space
*/
#define SYT_OFF_PERCPU(n) ((n + 1) * 12)

struct quatro_timer {
	u8 __iomem *base;
	u32 mode;
	u32 freq;
	int irq;
	u32 cpj; /* cycles per jiffy */
	struct clock_event_device evt;
	struct irqaction act;
	char name[32];
};

static DEFINE_PER_CPU(struct quatro_timer, percpu_quatro_tick);

static int quatro_time_set_next_event(unsigned long event,
	struct clock_event_device *evt_dev)
{
	struct quatro_timer *timer = container_of(evt_dev,
		struct quatro_timer, evt);

	writel_relaxed(0, timer->base + SYT_CTL_OFF);
	writel_relaxed(event, timer->base + SYT_TCN_OFF);
	writel_relaxed(timer->mode | SYT_LOAD | SYT_START, timer->base + SYT_CTL_OFF);
	return 0;
}

static void quatro_time_set_mode(enum clock_event_mode mode,
	struct clock_event_device *evt_dev)
{
	struct quatro_timer *timer = container_of(evt_dev,
		struct quatro_timer, evt);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		timer->mode = SYT_CONT_MODE;
		quatro_time_set_next_event(timer->cpj, evt_dev);
		break;
	case CLOCK_EVT_MODE_RESUME:
	case CLOCK_EVT_MODE_ONESHOT:
		timer->mode = 0;
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
		timer->mode = 0;
		writel_relaxed(0, timer->base + SYT_CTL_OFF);
		break;
	default:
		WARN(1, "%s: unhandled event mode %d\n", __func__, mode);
		break;
	}
}

static irqreturn_t quatro_time_interrupt(int irq, void *dev_id)
{
	struct quatro_timer *timer = dev_id;
	void (*event_handler)(struct clock_event_device *);
	
	/* printk(KERN_CRIT "ti b=%p i=%d cpu=%d\n", timer->base, timer->irq, smp_processor_id()); */
	event_handler = ACCESS_ONCE(timer->evt.event_handler);
	if (event_handler)
		event_handler(&timer->evt);
	return IRQ_HANDLED;
}

static int quatro_local_timer_setup(struct quatro_timer *timer)
{
	unsigned int cpu = smp_processor_id();

	timer->evt.name = timer->name;
	timer->evt.cpumask = cpumask_of(cpu);
	timer->evt.set_mode = quatro_time_set_mode;
	timer->evt.set_next_event = quatro_time_set_next_event;
	timer->evt.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	timer->evt.rating = 450; /* higher rating lets linux pick this timer */

	timer->evt.irq = timer->irq;
	if (request_irq(timer->evt.irq, quatro_time_interrupt,
				IRQF_TIMER | IRQF_NOBALANCING,
				timer->evt.name, timer)) {
		pr_err("quatro_local_timer: cannot register IRQ %d\n",
			timer->evt.irq);
		return -EIO;
	}
	irq_force_affinity(timer->evt.irq, cpumask_of(cpu));
	clockevents_config_and_register(&timer->evt, timer->freq,
					0xf, 0x7fffffff);
	pr_info("quatro: local timer registered at %d Hz\n", timer->freq);
	return 0;
}

static void quatro_local_timer_stop(struct quatro_timer *timer)
{
	timer->evt.set_mode(CLOCK_EVT_MODE_UNUSED, &timer->evt);
	free_irq(timer->evt.irq, timer);
}

static int quatro_timer_cpu_notify(struct notifier_block *self,
					   unsigned long action, void *hcpu)
{
	struct quatro_timer *timer;

	/*
	 * Grab cpu pointer in each case to avoid spurious
	 * preemptible warnings
	 */
	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_STARTING:
		timer = this_cpu_ptr(&percpu_quatro_tick);
		quatro_local_timer_setup(timer);
		break;
	case CPU_DYING:
		timer = this_cpu_ptr(&percpu_quatro_tick);
		quatro_local_timer_stop(timer);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block quatro_local_timer_cpu_nb = {
	.notifier_call = quatro_timer_cpu_notify,
};

static void __init quatro_timer_init(struct device_node *node)
{
	void __iomem *base;
	u32 freq, cycles_per_jiffy;
	int irq, nr_irqs, i;
	struct quatro_timer *timer;

	base = of_iomap(node, 0);
	if (!base)
		panic("Can't remap registers");

	freq = sysFrequency();

	/* ticks / jiffy = ticks/second * second/jiffy
	*/
	cycles_per_jiffy = (freq + (HZ / 2)) / HZ;

	clocksource_mmio_init(base + SYT_CNT_OFF, node->name,
		freq, 300, 32, clocksource_mmio_readl_up);

	nr_irqs = of_irq_count(node);
	if (nr_irqs < 1)
		panic("No IRQ for timers\n");
	irq = irq_of_parse_and_map(node, 0);
	if (irq <= 0)
		panic("Can't parse IRQ");

	timer = kzalloc(sizeof(*timer), GFP_KERNEL);
	if (!timer)
		panic("Can't allocate timer struct\n");

	strncpy(timer->name, node->name, sizeof(timer->name));
	timer->base = base;
	timer->mode = 0;
	timer->freq = freq;
	timer->cpj = cycles_per_jiffy;
	timer->evt.name = node->name;
	timer->evt.rating = 300;
	timer->evt.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	timer->evt.set_mode = quatro_time_set_mode;
	timer->evt.set_next_event = quatro_time_set_next_event;
	timer->evt.cpumask = cpu_all_mask; /*cpumask_of(0);*/

	timer->act.name = node->name;
	timer->act.flags = IRQF_TIMER | IRQF_IRQPOLL;
	timer->act.dev_id = timer;
	timer->act.handler = quatro_time_interrupt;

	writel_relaxed(0, timer->base + SYT_CTL_OFF);

	if (request_irq(irq, quatro_time_interrupt,
				IRQF_TIMER | IRQF_NOBALANCING,
				timer->evt.name, timer))
		panic("quatro_timer: cannot register IRQ %d\n", irq);

	clockevents_config_and_register(&timer->evt, freq, 0xf, 0xffffffff);

	pr_info("quatro: system timer cpj=%d (irq = %d) freq=%d\n", cycles_per_jiffy, irq, freq);
	
	/* pre-configure each local timer, each local timer is specified
	 * simply as an extra irq in the device tree
	 */
	for (i = 0; i < nr_irqs - 1; i++) {
		timer = &per_cpu(percpu_quatro_tick, i);
		if (! timer) {
			/*panic("Too many irqs for cpu count\n");*/
			break;
		}
		snprintf(timer->name, sizeof(timer->name), "qt lcl %d\n", i);
		timer->irq  = irq_of_parse_and_map(node, i + 1);
		timer->freq = freq;
		timer->mode = 0;
		timer->base = base + SYT_OFF_PERCPU(i);
		timer->cpj  = cycles_per_jiffy;
	}
	/* setup local timer on this boot cpu if more than one timer */
	if (nr_irqs > 1)
		if (! register_cpu_notifier(&quatro_local_timer_cpu_nb))
			quatro_local_timer_setup(this_cpu_ptr(&percpu_quatro_tick));
}
CLOCKSOURCE_OF_DECLARE(quatro, "csr,quatro-sys-timer", quatro_timer_init);

/***** HRT/CLK 
*/

#define HRT_STOP		0x00000000
#define HRT_CLEAR   	0x00000001
#define HRT_START   	0x00000002

#define HRTPRE1_OFF 	0x00000000
#define HRTCNT1_OFF 	0x00000004
#define HRTCTL1_OFF 	0x00000008

#define HRTPRE0_OFF 	0x00000000
#define HRTCNT0H_OFF	0x00000004
#define HRTCNT0L_OFF	0x00000008
#define HRTCTL0_OFF 	0x0000000C

struct quatro_clock {
	u8 __iomem *base;
	struct clocksource cs;
};

static void __init quatro_hrt1_init(struct device_node *node)
{
	u8 __iomem *base;
	u32 freq;
	struct quatro_clock *clock;

	base = of_iomap(node, 0);
	if (!base)
		panic("Can't remap registers");

	freq = sysFrequency();

	clocksource_mmio_init(base + HRTCNT1_OFF, node->name,
		freq / 32, 300, 32, clocksource_mmio_readl_up);

	clock = kzalloc(sizeof(*clock), GFP_KERNEL);
	if (!clock)
		panic("Can't allocate clock struct\n");

	clock->base = base;
	clock->cs.name = node->name;
	clock->cs.rating = 300;
	clock->cs.mask = CLOCKSOURCE_MASK(32);
	clock->cs.flags = CLOCK_SOURCE_IS_CONTINUOUS;

	/* start hrt1 free-running at about 8Mhz (1/32 of system clock) */
	writel_relaxed(HRT_CLEAR, clock->base + HRTCTL1_OFF);
	writel_relaxed(31, clock->base + HRTPRE1_OFF);
	writel_relaxed(HRT_START, clock->base + HRTCTL1_OFF);

	clocksource_register_hz(&clock->cs, freq / 32);

	pr_info("quatro: system clock at %u Hz\n", freq / 32);
}
CLOCKSOURCE_OF_DECLARE(quatrohrt1, "csr,quatro-hrt32", quatro_hrt1_init);

static struct quatro_clock *hrt0_sched_clock;

static u64 quatro_sched_clock_read(void)
{
	u32 lo, hi;
	u32 hi2 = readl_relaxed(hrt0_sched_clock->base + HRTCNT0H_OFF);

	do {
		hi = hi2;
		lo = readl_relaxed(hrt0_sched_clock->base + HRTCNT0L_OFF);
		hi2 = readl_relaxed(hrt0_sched_clock->base + HRTCNT0H_OFF);
	} while (hi != hi2);

	return ((cycle_t)hi << 32) | lo;	
}

static cycle_t quatro_hrt0_read(struct clocksource *cs)
{
	return quatro_sched_clock_read();
}

static void __init quatro_hrt0_init(struct device_node *node)
{
	u8 __iomem *base;
	u32 freq;
	struct quatro_clock *clock;

	base = of_iomap(node, 0);
	if (!base)
		panic("Can't remap registers");

	freq = sysFrequency();

	clock = kzalloc(sizeof(*clock), GFP_KERNEL);
	if (!clock)
		panic("Can't allocate clock struct\n");

	clock->base = base;
	clock->cs.name = node->name;
	clock->cs.rating = 300;
	clock->cs.mask = CLOCKSOURCE_MASK(64);
	clock->cs.flags = CLOCK_SOURCE_IS_CONTINUOUS;
	clock->cs.read = quatro_hrt0_read;
	hrt0_sched_clock = clock; /* Need a copy for
				   * quatro_sched_clock_read() */

	/* start hrt0 free-running at system clock) */
	writel_relaxed(HRT_CLEAR, clock->base + HRTCTL0_OFF);
	writel_relaxed(0, clock->base + HRTPRE0_OFF);
	writel_relaxed(HRT_START, clock->base + HRTCTL0_OFF);

	clocksource_register_hz(&clock->cs, freq);

	sched_clock_register(quatro_sched_clock_read, 64, freq);

	pr_info("quatro: hrt at %u Hz\n", freq);
}
CLOCKSOURCE_OF_DECLARE(quatrohrt0, "csr,quatro-hrt64", quatro_hrt0_init);


