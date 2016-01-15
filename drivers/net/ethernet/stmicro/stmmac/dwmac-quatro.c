/*
 * arch/arm/mach-quatro/dwmac-quatro.c
 *
 * Copyright (c) 2014, 2015 Linux Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/mfd/syscon.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/phy.h>
#include <linux/regmap.h>
#include "stmmac.h"

struct quatro_dwmac {
	struct platform_device *pdev;
};

static void quatro_fix_mac_speed(void *priv, unsigned int spd)
{
	struct quatro_dwmac *dwmac = priv;
	struct net_device *ndev = platform_get_drvdata(dwmac->pdev);
	struct stmmac_priv *dpriv = netdev_priv(ndev);
	struct phy_device *phydev;

	if (! dpriv)
		return;

	phydev = dpriv->phydev;

#ifdef CONFIG_SOC_QUATRO5300
	if (phydev->duplex && spd == 100) {
		pr_debug("Fix mac for 100 duplex\n");
		writel(0x3f00, dpriv->ioaddr + 0x410/*GmacTcpd*/);
	}
	else {
		pr_debug("Fix mac for non-100 or non-duplex\n");
		writel(0, dpriv->ioaddr + 0x410/*GmacTcpd*/);
	}
#endif
#ifdef CONFIG_SOC_QUATRO5500
    pr_debug("Fix Rx and Tx path delays for 5500A0 mac\n");
    writel(0x400, dpriv->ioaddr + 0x8008/*GmacRcpd*/);
    writel(0x1700, dpriv->ioaddr + 0x8010/*GmacTcpd*/);
#endif
}

static void *quatro_dwmac_setup(struct platform_device *pdev)
{
	struct quatro_dwmac *dwmac;

	dwmac = devm_kzalloc(&pdev->dev, sizeof(*dwmac), GFP_KERNEL);
	if (!dwmac)
		return ERR_PTR(-ENOMEM);
	dwmac->pdev = pdev;
	return dwmac;
}

const struct stmmac_of_data quatro_gmac_data = {
	.setup = quatro_dwmac_setup,
	.fix_mac_speed = quatro_fix_mac_speed,
};
