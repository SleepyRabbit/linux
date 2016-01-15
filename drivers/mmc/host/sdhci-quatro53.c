/*
 * SDHCI Support for Quatro Soc
 *
 * Copyright (c) 2015, 2015, The Linux Foundation. All rights reserved.
 *
 * Portions Copyright (c) 2009 Intel Corporation
 * Portions Copyright (c) 2007, 2011 Freescale Semiconductor, Inc.
 * Portions Copyright (c) 2009 MontaVista Software, Inc.
 * Portions Copyright (C) 2010 ST Microelectronics
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
#include <linux/highmem.h>
#include <linux/platform_device.h>

#include <linux/mmc/host.h>

#include <linux/io.h>
#include "sdhci.h"
#include "sdhci-quatro53.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#ifdef __KERNEL__
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/slab.h>
#endif
#include <linux/dma-mapping.h>
#include <linux/time.h>
#include <linux/string.h>

#define SDIO_XIN0_CLK           0
#define SDIO_SYSPLL_CLKOUT1     1
#define SDIO_VIDPLL_CLKOUT      2

struct sdhci_quatro53 {
	struct sdhci_host	*host;
	struct platform_device	*pdev;
	void __iomem * core_ioaddr;
};

static unsigned char core_initialized[2] = {0, 0};

/*****************************************************************************\
 *                                                                           *
 * SDHCI core callbacks                                                      *
 *                                                                           *
\*****************************************************************************/
static unsigned int q53sd_get_max_clk(struct sdhci_host *host)
{
	unsigned int q53sd_sdmclk = 96000000;

	return q53sd_sdmclk;
}

static void q53sd_enable_1_8v(struct sdhci_host *host, u32 enable)
{
	struct sdhci_quatro53 *q53_host = sdhci_priv(host);
	void __iomem * core_ioaddr = q53_host->core_ioaddr;
	unsigned int tmp;

	if (enable) {
		tmp = ioread32(core_ioaddr + SDIO0_HRS24_OFF);
		tmp |= 1;
		iowrite32(tmp, core_ioaddr + SDIO0_HRS24_OFF);
	} else {
		tmp = ioread32(core_ioaddr + SDIO0_HRS24_OFF);
		tmp &= (~0x1);
		iowrite32(tmp, core_ioaddr + SDIO0_HRS24_OFF);
	}

	return;
}

static struct sdhci_ops sdhci_q53sd_ops = {
	.get_max_clock      = q53sd_get_max_clk,
	.set_clock = sdhci_set_clock,
	.set_bus_width = sdhci_set_bus_width,
	.reset = sdhci_reset,
	.set_uhs_signaling = sdhci_set_uhs_signaling,
	.enable_1_8v_signal = q53sd_enable_1_8v,
};

/*****************************************************************************\
 *                                                                           *
 * Device probing/removal                                                    *
 *                                                                           *
\*****************************************************************************/
#define SDIO_MAX_INCLKDIV 16
typedef struct tag_clk_div_ctrl {
	unsigned char   neg_pwl_start;
	unsigned char   neg_pwh_start;
	unsigned char   pos_pwl_start;
	unsigned char   div_value;
} CLK_DIV_CTRL, *PCLK_DIV_CTRL;
static CLK_DIV_CTRL clkdivctrl_table[SDIO_MAX_INCLKDIV]={
	{0, 0, 0, 0},// padding and not applicable
	{0, 0, 0, 1},
	{0, 0, 2, 2},
	{2, 1, 2, 3},
	{0, 0, 3, 4},
	{3, 1, 3, 5},
	{0, 0, 4, 6},
	{4, 1, 4, 7},
	{0, 0, 5, 8},
	{5, 1, 5, 9},
	{0, 0, 6,10},
	{6, 1, 6,11},
	{0, 0, 7,12},
	{7, 1, 7,13},
	{0, 0, 8,14},
	{8, 1, 8,15},
};
static int q53_sdio_init(struct platform_device *pdev,
							struct sdhci_host *host)
{
	unsigned int reg_val = 0;
	unsigned long timeout;
	struct sdhci_quatro53 *q53_host = sdhci_priv(host);
	void __iomem * core_ioaddr = q53_host->core_ioaddr;
	void __iomem * ext_ctl = NULL;
	void __iomem * clk_dis_ctl = NULL;
	void __iomem * clk_dis_sta = NULL;
	void __iomem * clk_mux_ctl = NULL;
	void __iomem * clk_div_ctl = NULL;
	struct resource *iomem0;
	struct resource *iomem1;
	struct resource *iomem2;
	struct resource *iomem3;
	struct resource *iomem4;
	unsigned char source = SDIO_SYSPLL_CLKOUT1, div_value = 11;
	unsigned char slot = 0, core_index = 0, slot_index = 0;

	if (!strcmp(pdev->name,"f0007200.sdhci")) {
		slot = SLOT_SD0S1;
	} else if (!strcmp(pdev->name,"f0086100.sdhci")) {
		slot = SLOT_SD1S0;
	} else {
		dev_err(&pdev->dev, "device address is not supported\n");
		goto err_reset;
	}
	core_index = slot / 2;
	slot_index = slot % 2;

	if (!core_initialized[core_index]) {
		core_initialized[core_index] = 1;

		/* set core specific register */
		reg_val = ioread32(core_ioaddr + SDIO0_HRS0_OFF);
		reg_val |= 1;
		iowrite32(reg_val, (core_ioaddr + SDIO0_HRS0_OFF));
		timeout = 100;
		while (ioread32(core_ioaddr + SDIO0_HRS0_OFF) & 0x1) {
			if (timeout == 0) {
				dev_err(&pdev->dev, "to reset SDIO core failed\n");
				goto err_reset;
			}
			timeout--;
			mdelay(1);
		}
		iowrite32(0x00A25A80, core_ioaddr + SDIO0_HRS1_OFF);
		iowrite32(0x00000004, core_ioaddr + SDIO0_HRS2_OFF);

		/* set sdmclk */
		iomem1 = platform_get_resource(pdev, IORESOURCE_MEM, 2);
		if (!iomem1) {
			goto err_reset;
		}
		if (!request_mem_region(iomem1->start, resource_size(iomem1),
			mmc_hostname(host->mmc))) {
			dev_err(&pdev->dev, "cannot request region\n");
			goto err_reset;
		}
		clk_dis_ctl = ioremap(iomem1->start, resource_size(iomem1));
		
		iomem2 = platform_get_resource(pdev, IORESOURCE_MEM, 3);
		if (!iomem2) {
			goto err_reset;
		}
		if (!request_mem_region(iomem2->start, resource_size(iomem2),
			mmc_hostname(host->mmc))) {
			dev_err(&pdev->dev, "cannot request region\n");
			goto err_reset;
		}
		clk_dis_sta = ioremap(iomem2->start, resource_size(iomem2));

		iomem3 = platform_get_resource(pdev, IORESOURCE_MEM, 4);
		if (!iomem3) {
			goto err_reset;
		}
		if (!request_mem_region(iomem3->start, resource_size(iomem3),
			mmc_hostname(host->mmc))) {
			dev_err(&pdev->dev, "cannot request region\n");
			goto err_reset;
		}
		clk_mux_ctl = ioremap(iomem3->start, resource_size(iomem3));

        iomem4 = platform_get_resource(pdev, IORESOURCE_MEM, 5);
		if (!iomem4) {
			goto err_reset;
		}
		if (!request_mem_region(iomem4->start, resource_size(iomem4),
			mmc_hostname(host->mmc))) {
			dev_err(&pdev->dev, "cannot request region\n");
			goto err_reset;
		}
		clk_div_ctl = ioremap(iomem4->start, resource_size(iomem4));

		reg_val = ioread32(clk_dis_ctl);
		reg_val |= (CLKGENCLKDISCTRL6__SDIO0_SDM__MASK << core_index);
		iowrite32(reg_val, clk_dis_ctl);
		do {
			reg_val = ioread32(clk_dis_sta);
		}
		while (!(reg_val & (CLKGENCLKDISSTAT6__SDIO0_SDM__MASK << core_index)));

		reg_val = ioread32(clk_div_ctl);
		reg_val &= (~CLKGENCLKDIVCTRL_SDIO0_SDM__DIV_VALUE__MASK);
		iowrite32(reg_val, clk_div_ctl);

		reg_val = ioread32(clk_mux_ctl);
		reg_val &= (~(CLKGENCLKSELCTRL1__SDIO0_SDM__MASK << 2 * core_index));
		reg_val |= (source << (CLKGENCLKSELCTRL1__SDIO0_SDM__SHIFT + 2 * core_index));
		iowrite32(reg_val, clk_mux_ctl);

		reg_val = ioread32(clk_div_ctl);
		if ((div_value >= 1) && (div_value <= (SDIO_MAX_INCLKDIV - 1))) {
			reg_val &= (~CLKGENCLKDIVCTRL_SDIO0_SDM__NEG_PWL_START__MASK);
			reg_val &= (~CLKGENCLKDIVCTRL_SDIO0_SDM__NEG_PWH_START__MASK);
			reg_val &= (~CLKGENCLKDIVCTRL_SDIO0_SDM__POS_PWL_START__MASK);
			reg_val |= (clkdivctrl_table[div_value].neg_pwl_start <<
						CLKGENCLKDIVCTRL_SDIO0_SDM__NEG_PWL_START__SHIFT);
			reg_val |= (clkdivctrl_table[div_value].neg_pwh_start <<
						CLKGENCLKDIVCTRL_SDIO0_SDM__NEG_PWH_START__SHIFT);
			reg_val |= (clkdivctrl_table[div_value].pos_pwl_start <<
						CLKGENCLKDIVCTRL_SDIO0_SDM__POS_PWL_START__SHIFT);
		}
		else {
			dev_err(&pdev->dev, "unsupported divisor value.\n");
		}
		iowrite32(reg_val, clk_div_ctl);
		reg_val |= (div_value << CLKGENCLKDIVCTRL_SDIO0_SDM__DIV_VALUE__SHIFT);
		iowrite32(reg_val, clk_div_ctl);

		reg_val = ioread32(clk_dis_ctl);
		reg_val &= (~(CLKGENCLKDISCTRL6__SDIO0_SDM__MASK << core_index));
		iowrite32(reg_val, clk_dis_ctl);
		do {
			reg_val = ioread32(clk_dis_sta);
		}
		while (reg_val & (CLKGENCLKDISSTAT6__SDIO0_SDM__MASK << core_index));
		iounmap(clk_div_ctl);
		release_mem_region(iomem4->start, resource_size(iomem4));
		iounmap(clk_mux_ctl);
		release_mem_region(iomem3->start, resource_size(iomem3));
		iounmap(clk_dis_sta);
		release_mem_region(iomem2->start, resource_size(iomem2));
		iounmap(clk_dis_ctl);
		release_mem_region(iomem1->start, resource_size(iomem1));
	}

	/* set SDIOX_EXTCTL */
	iomem0 = platform_get_resource(pdev, IORESOURCE_MEM, 6);
	if (!iomem0) {
		goto err_reset;
	}
	if (!request_mem_region(iomem0->start, resource_size(iomem0),
		mmc_hostname(host->mmc))) {
		dev_err(&pdev->dev, "cannot request region\n");
		goto err_reset;
	}
	ext_ctl = ioremap(iomem0->start, resource_size(iomem0));

	reg_val = ioread32(ext_ctl);
	if (slot == SLOT_SD1S0) {
		reg_val |= (1 << SDIO1_EXTCTL__S0_CLE_OVL__SHIFT);
		reg_val |= (1 << SDIO1_EXTCTL__S0_CD_OVL__SHIFT);
		reg_val |= (1 << SDIO1_EXTCTL__S0_WP_OVL__SHIFT);
	} else {
		reg_val |= (1 << SDIO1_EXTCTL__S1_CLE_OVL__SHIFT);
		reg_val |= (1 << SDIO1_EXTCTL__S1_CD_OVL__SHIFT);
		reg_val |= (1 << SDIO1_EXTCTL__S1_WP_OVL__SHIFT);
	}
	iowrite32(reg_val, ext_ctl);
	iounmap(ext_ctl);
	release_mem_region(iomem0->start, resource_size(iomem0));

	return 0;

err_reset:
	dev_err(&pdev->dev, "q55_sdio_init failed.\n");
	return -1;
}

static int sdhci_q53_probe(struct platform_device *pdev)
{
	struct sdhci_host *host;
	struct sdhci_quatro53 *q53_host;
	struct resource *iomem;
	struct resource *core_iomem;
	void __iomem * ioaddr = NULL;
	void __iomem * core_ioaddr = NULL;
	int ret;

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iomem) {
		ret = -ENOMEM;
		goto err;
	}

	if (resource_size(iomem) < 0x100)
		dev_err(&pdev->dev, "Invalid iomem size. You may "
			"experience problems.\n");

	if (pdev->dev.parent)
		host = sdhci_alloc_host(pdev->dev.parent, sizeof(struct sdhci_quatro53));
	else
		host = sdhci_alloc_host(&pdev->dev, sizeof(struct sdhci_quatro53));
	if (IS_ERR(host)) {
		ret = PTR_ERR(host);
		goto err;
	}

	host->hw_name = pdev->name;
	host->ops = &sdhci_q53sd_ops;
	host->quirks = SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN
					| SDHCI_QUIRK_BROKEN_ADMA_ZEROLEN_DESC;
	host->quirks2 = SDHCI_QUIRK2_PRESET_VALUE_BROKEN
					| SDHCI_QUIRK2_BROKEN_HS200;
	if (!strcmp(pdev->name,"f0007200.sdhci")) {
		host->quirks2 |= SDHCI_QUIRK2_NO_1_8_V;
	}
	else {
		host->quirks2 |= SDHCI_QUIRK2_FORCE_CAPS2
						| SDHCI_QUIRK2_ENABLE_1_8V_ALTERNATIVE
						| SDHCI_QUIRK2_CAPABLE_OF_VOLTAGE_SWITCH;
		host->caps1 = SDHCI_SUPPORT_SDR50;
	}
	host->irq = platform_get_irq(pdev, 0);

	if (!request_mem_region(iomem->start, resource_size(iomem),
		mmc_hostname(host->mmc))) {
		dev_err(&pdev->dev, "cannot request region\n");
		ret = -EBUSY;
		goto err_request;
	}

	ioaddr = ioremap(iomem->start, resource_size(iomem));
	host->ioaddr = ioaddr;
	if (!host->ioaddr) {
		dev_err(&pdev->dev, "failed to remap registers\n");
		ret = -ENOMEM;
		goto err_remap;
	}

	core_iomem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!core_iomem) {
		ret = -ENOMEM;
		goto err_resource2;
	}
	if (!request_mem_region(core_iomem->start, resource_size(core_iomem),
		mmc_hostname(host->mmc))) {
		dev_err(&pdev->dev, "cannot request region\n");
		ret = -EBUSY;
		goto err_request2;
	}
	core_ioaddr = ioremap(core_iomem->start, resource_size(core_iomem));
	if (!core_ioaddr) {
		dev_err(&pdev->dev, "failed to remap registers\n");
		ret = -ENOMEM;
		goto err_remap2;
	}

	q53_host = sdhci_priv(host);
	q53_host->host = host;
	q53_host->pdev = pdev;
	q53_host->core_ioaddr = core_ioaddr;

	ret = q53_sdio_init(pdev, host);
	if(ret)
		goto err_add_host;

	ret = sdhci_add_host(host);
	if (ret)
		goto err_add_host;

	platform_set_drvdata(pdev, host);

	return 0;

err_add_host:
	iounmap(core_ioaddr);
err_remap2:
	release_mem_region(core_iomem->start, resource_size(core_iomem));
err_request2:
err_resource2:
	iounmap(ioaddr);
err_remap:
	release_mem_region(iomem->start, resource_size(iomem));
err_request:
	sdhci_free_host(host);
err:
	printk(KERN_ERR"Probing of sdhci-q53 %s failed: %d\n", pdev->name, ret);
	return ret;
}

static int sdhci_q53_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct resource *iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	int dead;
	u32 scratch;

	dead = 0;
	scratch = readl(host->ioaddr + SDHCI_INT_STATUS);
	if (scratch == (u32)-1)
		dead = 1;

	sdhci_remove_host(host, dead);
	iounmap(host->ioaddr);
	release_mem_region(iomem->start, resource_size(iomem));
	sdhci_free_host(host);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id csr_quatro_id_table[] = {
	{ .compatible = "csr,quatro53-sdhci" },
	{}
};

MODULE_DEVICE_TABLE(of, csr_quatro_id_table);

static struct platform_driver sdhci_q53_driver = {
	.driver = {
		.name	= "quatro53-sdhci",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(csr_quatro_id_table),
	},
	.probe		= sdhci_q53_probe,
	.remove		= sdhci_q53_remove,
};

/*****************************************************************************\
 *                                                                           *
 * Driver init/exit                                                          *
 *                                                                           *
\*****************************************************************************/

static int __init sdhci_quatro_init(void)
{
	return platform_driver_register(&sdhci_q53_driver);
}

static void __exit sdhci_quatro_exit(void)
{
	platform_driver_unregister(&sdhci_q53_driver);
}

module_init(sdhci_quatro_init);
module_exit(sdhci_quatro_exit);

MODULE_DESCRIPTION("Secure Digital Host Controller Interface Quatro driver");
MODULE_AUTHOR("TC");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("quatro:sdhci");

