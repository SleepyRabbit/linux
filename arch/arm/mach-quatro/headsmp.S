/*
 * arch/arm/mach-quatro/headsmp.S
 *
 * Copyright (c) 2012 ST Microelectronics Limited
 * Shiraz Hashim <shiraz.linux.kernel@gmail.com>
 *
 * Portions copyright (c) 2014, 2015 Linux Foundation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/linkage.h>
#include <linux/init.h>

/*	__INIT */

	.text
/*
 * quatro specific entry point for secondary CPUs. This provides
 * a "holding pen" into which all secondary cores are held until we're
 * ready for them to initialise.
 */

#ifdef CONFIG_SOC_QUATRO5500
ENTRY(quatro_secondary_startup)
	b out_of_vector
	ldr pc,=4
	ldr pc,=8
	ldr pc,=12
	ldr pc,=16
	ldr pc,=20
	ldr pc,=24
	ldr pc,=28

out_of_vector:
	mov r0,#0x80000000	@ Put in SVC mode, disable FIQ, IRQ.
	orr r0,r0,#0x100
	orr r0,r0,#0xf0
	orr r0,r0,#0x3
	msr CPSR,r0

	mov r0,#0x0         /* disable */
	mov r1,#0x04300000  /* GICC_CTLR */
	orr r1,#0x00002000
	str r0,[r1]

	mov r0,#0xf0        /* Allow low-priority interrupts */
	mov r1,#0x04300000  /* GICC_PMR */
	orr r1,#0x00002000
	orr r1,#0x00000004
	str r0,[r1]

	mov r0,#0x1         /* enable */
	mov r1,#0x04300000  /* GICC_CTLR */
	orr r1,#0x00002000
	str r0,[r1]

	/* Need to change L2 data and tag latency to 3-cycles. */
	mrc p15,#1,r0,c9,c0,#2 /* Read L2CTLR */

	mov r1,   #0xff000000
	orr r1,r1,#0x00ff0000
	orr r1,r1,#0x0000fe00
	orr r1,r1,#0x00000038
	and r0,r0,r1           /* Clear 08-06, and 02-00 bits. */

	orr r0,r0,#0x00000002  /* Set data RAM latency to 3-cycles. */
	orr r0,r0,#0x00000080  /* Set data tag latency to 3-cycles. */
	orr r0,r0,#0x00200000  /* Turn on ecc and parity. */

	mcr p15,#1,r0,c9,c0,#2 /* Write L2CTLR */

	bl	v7_invalidate_l1

	ldr	r1, 1f
	bx	r1
ENDPROC(quatro_secondary_startup)

	.align 2
	.globl quatro55xx_smp_entrypoint
quatro55xx_smp_entrypoint:
1:	.space 4
#else
ENTRY(quatro_secondary_startup)
	mrc	p15, 0, r0, c0, c0, 5
	and	r0, r0, #15
	adr	r4, 1f
	ldmia	r4, {r5, r6}
	sub	r4, r4, r5
	add	r6, r6, r4
pen:	ldr	r7, [r6]
	cmp	r7, r0
	bne	pen

	.if 0
	/* re-enable coherency */
	mrc	p15, 0, r0, c1, c0, 1
	orr	r0, r0, #(1 << 6) | (1 << 0)
	mcr	p15, 0, r0, c1, c0, 1
	.endif
	/*
	 * we've been released from the holding pen: secondary_stack
	 * should now contain the SVC stack for this core
	 */
	b	secondary_startup

	.align
1:	.long	.
	.long	pen_release
ENDPROC(quatro_secondary_startup)
#endif
