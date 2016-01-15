/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
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

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/reboot.h>

#include <asm/system_misc.h>

static void __iomem *quatro_rstgenporpowerdown;

static void do_quatro_restart(enum reboot_mode reboot_mode, const char *cmd)
{
	writel(0x86753099, quatro_rstgenporpowerdown);
	writel(0, quatro_rstgenporpowerdown);
	mdelay(10000);
}

static void do_quatro_poweroff(void)
{
	/* TODO: Add poweroff capability */
	do_quatro_restart(REBOOT_HARD, NULL);
}

static int quatro_restart_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *mem;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	quatro_rstgenporpowerdown = devm_ioremap_resource(dev, mem);
	if (IS_ERR(quatro_rstgenporpowerdown))
		return PTR_ERR(quatro_rstgenporpowerdown);

	pm_power_off = do_quatro_poweroff;
	arm_pm_restart = do_quatro_restart;
	return 0;
}

static const struct of_device_id of_quatro_restart_match[] = {
	{ .compatible = "csr,quatro-rstgenporpowerdown", },
	{},
};
MODULE_DEVICE_TABLE(of, of_quatro_restart_match);

static struct platform_driver quatro_restart_driver = {
	.probe = quatro_restart_probe,
	.driver = {
		.name = "quatro-restart",
		.of_match_table = of_match_ptr(of_quatro_restart_match),
	},
};

static int __init quatro_restart_init(void)
{
	return platform_driver_register(&quatro_restart_driver);
}
device_initcall(quatro_restart_init);
