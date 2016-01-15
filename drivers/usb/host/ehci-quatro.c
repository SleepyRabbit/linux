/*
 * drivers/usb/host/ehci-quatro.c
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/mbus.h>

/* these are the same for all quatro socs (check?)
*/
#define USB20MPHUSBRST__RST__MASK        0x00000001
#define USB20MPHUSBRST__RST__HW_DEFAULT  0x0

#ifdef CONFIG_SOC_QUATRO5300

#define RSTGENSWRSTSTATIC2        0xF0032064 /* move to device tree! */
#define USB20USBRST               0xF0014014 /* this too */

#define USB20MPHID_OFF            0x00000000
#define USB20MPHHWGENERAL_OFF     0x00000004
#define USB20MPHUSBCMD_OFF        0x00000140
#define USB20MPHUSBSTS_OFF        0x00000144
#define USB20MPHUSBMODE_OFF       0x000001A8
#define USB20MPHUSBRST_OFF        0x00004014
#define USB20MPHUSBP0_OFF         0x00005020
#define USB20MPHUSBPWR0_OFF       0x00005024
#define USB20MPHUSBPWR1_OFF       0x00006024

#elif defined CONFIG_SOC_QUATRO4500

#elif defined CONFIG_SOC_QUATRO4300

#else

 - configuration error -

#endif

static u32 rdl(struct usb_hcd* hcd, u32 off)
{
	u32 rv = __raw_readl(hcd->regs + (off));
	/*printk("qehci rr %04X (%08x) => %08x\n", off, (u32)(hcd->regs + (off)), rv);*/
	return rv;
}

static void wrl(struct usb_hcd* hcd, u32 off, u32 val)
{
	/*printk("qehci wr %04X (%08x) <= %08x\n", off, (u32)(hcd->regs + (off)), val);*/
	__raw_writel((val), hcd->regs + (off));
	wmb();
}

static void quatro_usb_setup(struct usb_hcd *hcd)
{
	int waiter;
	
	/* power on
	*/
	/* this driver depends on boot code to take the usb ehci
	* controller out of reset.  if it is done here, the hcd
	* code times out, because there is a long delay between
	* taking the h/w out of reset and being usable by hcd code
	*/
	wrl(hcd, USB20MPHUSBP0_OFF, 0x2 | (rdl(hcd, USB20MPHUSBP0_OFF) & ~0x4));

	/* reset interface and wait for reset over
	*/
	wrl(hcd, USB20MPHUSBRST_OFF, USB20MPHUSBRST__RST__MASK);
	udelay(20);
	wrl(hcd, USB20MPHUSBRST_OFF, USB20MPHUSBRST__RST__HW_DEFAULT);
	udelay(30);
	waiter = 0;
	do {
		;
	} while(! (rdl(hcd, USB20MPHUSBRST_OFF) & 0x80000000));

	/* stop the controller
	*/
	wrl(hcd, USB20MPHUSBCMD_OFF, rdl(hcd, USB20MPHUSBCMD_OFF) & ~0x1);
	
	wrl(hcd, USB20MPHUSBPWR0_OFF, 0x2 | rdl(hcd, USB20MPHUSBPWR0_OFF));
	wrl(hcd, USB20MPHUSBPWR1_OFF, 0x2 | rdl(hcd, USB20MPHUSBPWR1_OFF));

	/* Stop and reset controller
	*/
	wrl(hcd, USB20MPHUSBCMD_OFF, rdl(hcd, USB20MPHUSBCMD_OFF) & ~0x1);
	wrl(hcd, USB20MPHUSBCMD_OFF, rdl(hcd, USB20MPHUSBCMD_OFF) | 0x2);
	do {
		;
	} while(rdl(hcd, USB20MPHUSBCMD_OFF) & 0x2);

	/* clear pending ints
	*/
	wrl(hcd, USB20MPHUSBSTS_OFF, 0xFFFF);

	/* Put it into Host Mode 
	*/
#ifdef CONFIG_SOC_QUATRO5300
	wrl(hcd, USB20MPHUSBMODE_OFF, 0x5003);
#else
	wrl(hcd, USB20MPHUSBMODE_OFF, 0x3);
#endif
	printk("===== ehco set\n");
}

static int ehci_quatro_setup(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int retval;

	retval = ehci_halt(ehci);
	if (retval)
		return retval;

	/*
	 * data structure init
	 */
	retval = ehci_init(hcd);
	printk("ehci init=%d\n", retval);
	if (retval)
		return retval;

	hcd->has_tt = 1;

	ehci_reset(ehci);

	ehci_setup(hcd);

	return retval;
}

static const struct hc_driver ehci_quatro_hc_driver = {
	.description = hcd_name,
	.product_desc = "Quatro EHCI",
	.hcd_priv_size = sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq = ehci_irq,
	.flags = HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.reset = ehci_quatro_setup,
	.start = ehci_run,
	.stop = ehci_stop,
	.shutdown = ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue = ehci_urb_enqueue,
	.urb_dequeue = ehci_urb_dequeue,
	.endpoint_disable = ehci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number = ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data = ehci_hub_status_data,
	.hub_control = ehci_hub_control,
	.bus_suspend = ehci_bus_suspend,
	.bus_resume = ehci_bus_resume,
	.relinquish_port = ehci_relinquish_port,
	.port_handed_over = ehci_port_handed_over,
};

static int __init ehci_quatro_drv_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	void __iomem *regs;
	int irq, err;

	if (usb_disabled())
		return -ENODEV;

	pr_debug("Initializing quatro-SoC USB Host Controller\n");

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(&pdev->dev,
			"Found HC with no IRQ. Check %s setup!\n",
			dev_name(&pdev->dev));
		err = -ENODEV;
		goto err1;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev,
			"Found HC with no register addr. Check %s setup!\n",
			dev_name(&pdev->dev));
		err = -ENODEV;
		goto err1;
	}

	if (!request_mem_region(res->start, res->end - res->start + 1,
				ehci_quatro_hc_driver.description)) {
		dev_dbg(&pdev->dev, "controller already in use\n");
		err = -EBUSY;
		goto err1;
	}

	regs = ioremap(res->start, res->end - res->start + 1);
	if (regs == NULL) {
		dev_dbg(&pdev->dev, "error mapping memory\n");
		err = -EFAULT;
		goto err2;
	}

	hcd = usb_create_hcd(&ehci_quatro_hc_driver,
			&pdev->dev, dev_name(&pdev->dev));
	if (!hcd) {
		err = -ENOMEM;
		goto err3;
	}
	hcd->rsrc_start = res->start;
	hcd->rsrc_len = res->end - res->start + 1;
	hcd->regs = regs;

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs + 0x100;
	ehci->regs = hcd->regs + 0x100 +
		HC_LENGTH(ehci, ehci_readl(ehci, &ehci->caps->hc_capbase));
	ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);
	hcd->has_tt = 1;
	ehci->sbrn = 0x20;

	/* setup quatro USB controller
	 */
	quatro_usb_setup(hcd);

	err = usb_add_hcd(hcd, irq, IRQF_DISABLED);
	printk("adder=%d\n", err);
	if (err)
		goto err4;

	return 0;
err4:
	usb_put_hcd(hcd);
err3:
	iounmap(regs);
err2:
	release_mem_region(res->start, res->end - res->start + 1);
err1:
	dev_err(&pdev->dev, "init %s fail, %d\n",
		dev_name(&pdev->dev), err);

	return err;
}

#ifdef CONFIG_PM
static int ehci_quatro_drv_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	bool do_wakeup = device_may_wakeup(dev);

	return ehci_suspend(hcd, do_wakeup);
}

static int ehci_quatro_drv_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);

	ehci_resume(hcd, false);
	return 0;
}

static const struct dev_pm_ops quatro_ehci_pmops = {
	.suspend	= ehci_quatro_drv_suspend,
	.resume		= ehci_quatro_drv_resume,
};

#define QUATRO_EHCI_PMOPS (&quatro_ehci_pmops)

#else
#define QUATRO_EHCI_PMOPS NULL
#endif

static int __exit ehci_quatro_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);

	return 0;
}

static const struct of_device_id csr_quatro_id_table[] = {
	{ .compatible = "csr,quatro-ehci" },
	{}
};
MODULE_DEVICE_TABLE(of, csr_quatro_id_table);

static struct platform_driver ehci_quatro_driver_ops = {
	.probe		= ehci_quatro_drv_probe,
	.remove		= ehci_quatro_drv_remove,
	.driver		= {
		.name	= "quatro-ehci",
		.owner	= THIS_MODULE,
		.pm	= QUATRO_EHCI_PMOPS,
		.of_match_table = of_match_ptr(csr_quatro_id_table),
	},
};

