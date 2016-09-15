/*
 * arch/arm/mm/cache-l2quatro.c - Quatro L2 cache controller support
 *
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#include <asm/cacheflush.h>
#include <asm/cp15.h>
#include <asm/hardware/cache-l2quatro.h>
#include <asm/cacheflush.h>
#include <linux/kobject.h>
#include <linux/stat.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define CACHE_LINE_SIZE		32

static int 	l2_cache_mode;
static volatile void __iomem *l2quatro_base;
static DEFINE_SPINLOCK(l2quatro_lock);

extern void Enable_L2SPD_ASM(void);
extern u32 asm_size;
struct kobject *l2_kobj;
static int l2_kobj_created = 0;

int cache_l2quatro_enabled(void)
{
	return l2_cache_mode;
}
EXPORT_SYMBOL(cache_l2quatro_enabled);

static inline void l2_writel(unsigned long val, unsigned long reg)
{
	writel(val, l2quatro_base + reg);
}
static inline unsigned long l2_readl(unsigned long reg)
{
	return readl(l2quatro_base + reg);
}

static void l2quatro_inv_range(unsigned long start, unsigned long end)
{
	unsigned long value;
	unsigned long flags;
    
	if (start == end) return;

	spin_lock_irqsave(&l2quatro_lock, flags);

	/* align start/end on cache line boundaries.  end address
	 * is non-inclusive in call but inclusive in our op, so dec by 1
	 */
	start &= ~(CACHE_LINE_SIZE - 1);
	end += CACHE_LINE_SIZE - 1;
	end &= ~(CACHE_LINE_SIZE - 1);
	end--;

	/* Don't do anything unless the L2 cache is on */
	value = l2_readl(L2C_L2CCTL_OFF);
	if ((value & 7) == 6) {	   
		/* set up to invalidate a memory region */
		l2_writel(0x1, L2C_CLICFG_OFF);

		/* set start address */
		l2_writel(start, L2C_CLISTART_OFF);

		/* set end address */
		l2_writel(end, L2C_CLISTOP_OFF);

		/* start invalidate operation */
		l2_writel(0x1, L2C_CLICTL_OFF);
		dmb(); /* Intentionally not calling mb(), barrier
			  workaround not applicable here. */

		/* wait for operation to finish */
		do {
			value = l2_readl(L2C_CLISTAT_OFF);
		} while ((value & 2) != 2);

		/* clear the control register */
		l2_writel(0x0, L2C_CLICTL_OFF);
	}
	spin_unlock_irqrestore(&l2quatro_lock, flags);
}

static void l2quatro_clean_range(unsigned long start, unsigned long end)
{
	/* Quatro L2 cache is write-through, no clean needed */
}

static void l2quatro_flush_range(unsigned long start, unsigned long end)
{
	/* no clean needed, so just invalidate */
	l2quatro_inv_range(start, end);
}

static ssize_t l2_show(struct kobject* kobj, struct kobj_attribute * attr,
		char *buf)
{
	volatile u32 __iomem val;
	char *str;

	val = l2_readl(L2C_L2CCTL_OFF);
	if( (val & 0x7) == 0x6) 
		str = "enabled\n";		
	else 
		str = "disabled\n";
	strcpy(buf, str);
	return strlen(str);
}

static ssize_t l2_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t n)
{
	if(strncmp(buf,"enable", strlen("enable")) == 0)
		l2quatro_init(1);
	else if(strncmp(buf, "disable", strlen("disable")) == 0)
		l2quatro_init(0);
	else
		printk("invalid input\n");
	return 0;
}

static struct kobj_attribute l2_attr = __ATTR(state, S_IRUGO|S_IWUGO, l2_show, l2_store);

static int l2_attr_created = 0;

static void __init __invalidate_icache(void)
{
	__asm__("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));
}

static int __init invalidate_and_disable_icache(void)
{
	u32 cr;

	cr = get_cr();
	if (cr & CR_I) {
		set_cr(cr & ~CR_I);
		__invalidate_icache();
		return 1;
	}
	return 0;
}

static void __init enable_icache(void)
{
	u32 cr;

	cr = get_cr();
	set_cr(cr | CR_I);
}

#define L2FIFOSEMPTYMASK ((1<<16)|(1<<12)|(1<<8)|(1<<4)|(1<<0))
void  l2quatro_init(int enable)
{
	unsigned long flags;
	int ret;
	unsigned long value;
	int icache_enabled;

	l2_cache_mode = 0;
	icache_enabled = 0;

	if(! l2_kobj_created) {
		l2_kobj = kobject_create_and_add("L2cache", NULL);
	}
	l2_kobj_created = 1;

	if(!l2_kobj) {
		printk("Failed to create l2_kobj\n");
		goto failed;
	}

	if(!l2_attr_created) {
		ret = sysfs_create_file(l2_kobj, &l2_attr.attr);
		if(ret) {
			printk("failed to create sysfs for l2_attr\n");
			goto failed;
		}	
		l2_attr_created = 1;
	}
	smp_mb();

	if (enable) {
		/* disable L2 cache and L2 cache scratchpad */	
		l2_writel(0x0, L2C_L2CCTL_OFF);
		if (l2_cache_mode) {
			/* already enabled, is this a bug or an ignore?
			 * in either case, this isn't reentrant, so this
			 * function might have to handle that? */
			return;
		}
		spin_lock_irqsave(&l2quatro_lock, flags);
		icache_enabled = invalidate_and_disable_icache();

		l2_cache_mode = 1;

		/* Note -- these are all magic values defined in the
		   register catalog. */

		/* enable scratch pad; start tag memory clear */
		l2_writel(0x5, L2C_L2CCTL_OFF);

		/* set up to clear all 8 sets */
		l2_writel(0x7, L2C_CLRCFG_OFF);

		/* start clear of tag memory and round robin tag memory
		   and wait for operation to finish */
		l2_writel(0x3, L2C_CLRCTL_OFF);
		dmb();  /* Intentionally not calling mb(), barrier
			   workaround not applicable here. */
		do {
			value = l2_readl(L2C_CLRSTAT_OFF);
		} while ((value & 0xA) != 0xA);

		/* clear control register */
		l2_writel(0x0, L2C_CLRCTL_OFF);

		/* make sure we cache the lower 2G region of address space */
		l2_writel(0x0, L2C_TAG_OFF);

		/* Lock no sets */
		l2_writel(0x0, L2C_LACFG_OFF);

		/* enable scratchpad access and L2 cache */
		l2_writel(0x6, L2C_L2CCTL_OFF);

		dmb();  /* Intentionally not calling mb(),
			   barrier workaround not applicable
			   here. */

		smp_mb();
		spin_unlock_irqrestore(&l2quatro_lock, flags);
	} else { 
		/*disable*/
		spin_lock_irqsave(&l2quatro_lock, flags);
		/* disable L2 cache but leave L2 cache scratchpad enabled */
		l2_writel(0x4, L2C_L2CCTL_OFF);
		/* wait for all L2 cache FIFOs to be empty */    
		do {
			value = l2_readl(L2C_L2CFSTAT_OFF);
		} while ((value & L2FIFOSEMPTYMASK) != L2FIFOSEMPTYMASK);
		/*disable L2 cache and l2 cache scratchpad*/
		l2_writel(0x0, L2C_L2CCTL_OFF);
		l2_cache_mode = 0;
		smp_mb();
		spin_unlock_irqrestore(&l2quatro_lock, flags);
	}
	if (icache_enabled) {
		enable_icache();
	}
	outer_cache.inv_range = l2quatro_inv_range;
	outer_cache.clean_range = l2quatro_clean_range;
	outer_cache.flush_range = l2quatro_flush_range;

	printk(KERN_INFO "Quatro L2 cache controller %sabled\n", enable ? "en" : "dis");
	return;

failed:
	printk("%s failed\n", __FUNCTION__);
	return;
}

static const struct of_device_id l2quatro_ids[] = {
	{ .compatible = "csr,quatro-l2cache" },
	{ }
};

int __init l2quatro_of_init(void)
{
	struct device_node *node;
	struct resource res;

	node = of_find_matching_node(NULL, l2quatro_ids);
	if (!node)
		return -ENODEV;

	if (of_address_to_resource(node, 0, &res))
		return -ENODEV;

	l2quatro_base = ioremap(res.start, resource_size(&res));
	if (!l2quatro_base)
		return -ENOMEM;

	l2quatro_init(1);

	return 0;
}
