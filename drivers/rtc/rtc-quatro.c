/*
 * A driver for the RTC embedded in the Quatro 4230, 43XX, 45XX processors
 *
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/slab.h>

#define RTC_CNT			0x10
#define RTC_CTL			0x1C

#define RTC_BUSY		0x0001
#define RTC_START		0x0001
#define RTC_READ		0x0002
#define RTC_INIT		0x0004

#define RTC_READ_START	(RTC_READ | RTC_START)
#define RTC_WRITE_START	(RTC_START)

/* 1 second read-busy timeout */
#define RTC_READ_TO 100
	
struct rtc_quatro {
	struct rtc_device	*rtc;
	volatile u8	__iomem	*regs;
	unsigned long		irq;
	int			accessable;
	int			stuck;
	struct mutex		lock;
};

static void rtc_delay(void)
{
	volatile int a;
	
	for(a = 0; a < 100; a++)
		;
}

static inline u32 rtc_readl(struct rtc_quatro *rtc, int reg)
{
	return readl(rtc->regs + reg);
}

static inline void rtc_writel(struct rtc_quatro *rtc, int reg, u32 val)
{
	writel(val, rtc->regs + reg);
}

static inline u8 rtc_readb(struct rtc_quatro *rtc, int reg, int byte)
{
	return readb(rtc->regs + reg + byte);
}

static void rtc_writeb(struct rtc_quatro *rtc, int reg, int byte, u32 val)
{
	if (likely(!rtc->stuck))
		writeb(val, rtc->regs + reg + byte);

	rtc_delay();
}
		
static int quatro_busywait(struct rtc_quatro *rtc)
{
	int timeout;
	u8 ctl;
	
	/* make sure busy not set in ctl for up to 1 second */
	timeout = 0;
	do {	
		ctl = rtc_readb(rtc, RTC_CTL, 0);
		if (ctl & RTC_BUSY)
			msleep(1);
	} while ((ctl & RTC_BUSY) && timeout++ < RTC_READ_TO);
	
	if (ctl & RTC_BUSY) {
		printk(KERN_ERR " RTC is not connected\n");
		rtc->stuck = 1;
		return -EBUSY;
	}
	return 0;
}

static void quatro_rtc_access_enable(struct rtc_quatro *rtc)
{
	if (rtc->accessable)
		return;

	/* "connect" bit */
	rtc_writeb(rtc, RTC_CTL, 2, 0x01);
	udelay(1);
	/* enable */
	rtc_writeb(rtc,	RTC_CTL, 3, 0x80);
	rtc_writeb(rtc,	RTC_CTL, 3, 0x40);
	rtc_writeb(rtc,	RTC_CTL, 3, 0xC0);
	rtc_writeb(rtc,	RTC_CTL, 3, 0x00);	
	rtc_writeb(rtc,	RTC_CTL, 3, 0x80);
	rtc_writeb(rtc,	RTC_CTL, 3, 0x40);
	rtc_writeb(rtc,	RTC_CTL, 3, 0xC0);
	rtc_writeb(rtc,	RTC_CTL, 3, 0x00);		
	udelay(1);

	rtc->accessable = 1;
}

static void quatro_rtc_access_disable(struct rtc_quatro *rtc)
{
	if (!rtc->accessable)
		return;

	rtc->accessable = 0;

	/* disable */
	rtc_writeb(rtc,	RTC_CTL, 3, 0x80);
	rtc_writeb(rtc,	RTC_CTL, 3, 0x00);
	rtc_writeb(rtc,	RTC_CTL, 3, 0x80);
	rtc_writeb(rtc,	RTC_CTL, 3, 0x00);
	rtc_writeb(rtc,	RTC_CTL, 3, 0x80);
	rtc_writeb(rtc,	RTC_CTL, 3, 0x00);
	rtc_writeb(rtc,	RTC_CTL, 3, 0x80);
	rtc_writeb(rtc,	RTC_CTL, 3, 0x00);
	/* disconnect */
	rtc_writeb(rtc, RTC_CTL, 2, 0x00);
}

static int quatro_rtc_readtime(struct device *dev, struct rtc_time *tm)
{
	struct rtc_quatro *rtc = dev_get_drvdata(dev);
	unsigned long now;

	mutex_lock(&rtc->lock);
	if (!rtc->stuck)
		quatro_busywait(rtc);
	now = rtc_readl(rtc, RTC_CNT);
	mutex_unlock(&rtc->lock);

	rtc_time_to_tm(now, tm);

	return 0;
}

static int quatro_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	struct rtc_quatro *rtc = dev_get_drvdata(dev);
	unsigned long now;
	int ret;

	if (rtc->stuck)
		return -EBUSY;

	ret = rtc_tm_to_time(tm, &now);
	if (ret)
		return ret;

	mutex_lock(&rtc->lock);

	quatro_rtc_access_enable(rtc);
	/* set count register */
	rtc_writel(rtc, RTC_CNT, now);
	/* start a write of the shadow */
	rtc_writeb(rtc, RTC_CTL, 0, RTC_WRITE_START);
	quatro_busywait(rtc);
	/* set the init bit */
	rtc_writeb(rtc, RTC_CTL, 0, RTC_INIT);
	quatro_busywait(rtc);
	quatro_rtc_access_disable(rtc);

	mutex_unlock(&rtc->lock);

	return 0;
}

static irqreturn_t quatro_rtc_interrupt(int irq, void *dev_id)
{
	return IRQ_NONE;
}

static struct rtc_class_ops quatro_rtc_ops = {
	.read_time	= quatro_rtc_readtime,
	.set_time	= quatro_rtc_settime,
};

static int __init quatro_rtc_probe(struct platform_device *pdev)
{
	struct rtc_quatro *rtc;
	/*struct device_node *np = pdev->dev.of_node;*/
	struct resource	*regs;
	int irq, ret;
	u32 ctl;
	
	rtc = devm_kzalloc(&pdev->dev, sizeof(struct rtc_quatro), GFP_KERNEL);
	if (rtc == NULL) {
		dev_dbg(&pdev->dev, "out of memory\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, rtc);

	mutex_init(&rtc->lock);
	rtc->accessable = 0;
	rtc->stuck = 0;
	
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_dbg(&pdev->dev, "could not get irq\n");
		ret = -ENXIO;
		goto out;
	}
	rtc->irq = irq;

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (regs == NULL) {
		dev_dbg(&pdev->dev, "no mmio resource defined\n");
		ret = -ENXIO;
		goto out;
	}
	rtc->regs = devm_ioremap_resource(&pdev->dev, regs);
	if (rtc->regs == NULL) {
		ret = -ENOMEM;
		dev_dbg(&pdev->dev, "could not map I/O memory\n");
		goto out;
	}
	/*
	 * We can't take any op requests until registered, so don't
	 * bother locking.
	 */
	quatro_rtc_access_enable(rtc);
	ctl = rtc_readb(rtc, RTC_CTL, 0);
	if ((ctl & RTC_INIT) == 0) {
		/* rtc never set! */
		dev_dbg(&pdev->dev, "rtc never set, first-time init\n");
		/* set count register */
		rtc_writel(rtc, RTC_CNT, 0);
		/* write 0 to the shadow */
		rtc_writeb(rtc, RTC_CTL, 0, RTC_WRITE_START);
		quatro_busywait(rtc);
		rtc_writeb(rtc, RTC_CTL, 0, RTC_INIT);
		quatro_busywait(rtc);
	}
	/* start a read of the counter to set shadow */
	rtc_writeb(rtc, RTC_CTL, 0, RTC_READ_START);
	quatro_busywait(rtc);
	quatro_rtc_access_disable(rtc);
	smp_wmb();		/* Paranoid. */

	ret = request_irq(irq, quatro_rtc_interrupt, IRQF_DISABLED, "rtc", rtc);
	if (ret) {
		dev_dbg(&pdev->dev, "could not request irq %d\n", irq);
		goto out_iounmap;
	}

	rtc->rtc = rtc_device_register(pdev->name, &pdev->dev,
				       &quatro_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc->rtc)) {
		dev_dbg(&pdev->dev, "could not register rtc device\n");
		ret = PTR_ERR(rtc->rtc);
		goto out_free_irq;
	}
	device_init_wakeup(&pdev->dev, 1);

	dev_info(&pdev->dev, "Quatro RTC at MMIO %08lx irq %ld\n",
		 (unsigned long)rtc->regs, rtc->irq);

	return 0;

out_free_irq:
	free_irq(irq, rtc);
out_iounmap:
	iounmap(rtc->regs);
out:
	kfree(rtc);
	return ret;
}

static int __exit quatro_rtc_remove(struct platform_device *pdev)
{
	struct rtc_quatro *rtc = platform_get_drvdata(pdev);

	device_init_wakeup(&pdev->dev, 0);

	free_irq(rtc->irq, rtc);
	iounmap(rtc->regs);
	rtc_device_unregister(rtc->rtc);
	kfree(rtc);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id quatro_rtc_of_match[] = {
	{ .compatible = "csr,quatro-rtc"},
	{},
};
MODULE_DEVICE_TABLE(of, sirfsoc_rtc_of_match);

static struct platform_driver quatro_rtc_driver = {
	.driver = {
		.name = "quatro-rtc",
		.owner = THIS_MODULE,
		/*.pm = &quatro_rtc_pm_ops,*/
		.of_match_table = quatro_rtc_of_match,
	},
	.probe = quatro_rtc_probe,
	.remove = quatro_rtc_remove,
};
module_platform_driver(quatro_rtc_driver);

MODULE_DESCRIPTION("Quatro SoC RTC driver");
MODULE_AUTHOR("Cambridge Silicon Radio Ltd.");
MODULE_DESCRIPTION("Real time clock for Quatro 4xxx");
MODULE_LICENSE("GPL");
