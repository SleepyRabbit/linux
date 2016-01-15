/*
 * arch/arm/mach-quatro/quatro.h
 *
 * Copyright (c) 2014, 2015 Linux Foundation.
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

#ifndef __QUATRO_H__
#define __QUATRO_H__

#include <linux/init.h>
#include <linux/reboot.h>

#include <asm/mach/time.h>
#include <asm/exception.h>

/* just UART0 for early printk support 
*/
#if defined(CONFIG_SOC_QUATRO5500)

    #define QUATRO_UART0_PHYS_BASE 0x040B0000
    #define QUATRO_UART0_VIRT_BASE IOMEM(0xfe0b0000)
    #define QUATRO_UART0_SIZE SZ_4K

#elif defined(CONFIG_SOC_QUATRO5300)

    #define QUATRO_UART0_PHYS_BASE 0xf0037000
    #define QUATRO_UART0_VIRT_BASE IOMEM(0xfe837000)
    #define QUATRO_UART0_SIZE SZ_4K

#elif defined(CONFIG_SOC_QUATRO4500)

    #define QUATRO_UART0_PHYS_BASE 0xe0010000
    #define QUATRO_UART0_VIRT_BASE IOMEM(0xfe010000)
    #define QUATRO_UART0_SIZE SZ_4K
    #define QUATRO_MISC_PHYS_BASE 0xe0205004
    #define QUATRO_MISC_VIRT_BASE IOMEM(0xfe205004)
    #define QUATRO_MISC_SIZE SZ_4
    #define QUATRO_SCU_PHYS_BASE 0xe8300000
    #define QUATRO_SCU_VIRT_BASE IOMEM(0xfe300000)
    #define QUATRO_SCU_SIZE SZ_8K

#else
#error
#endif

#ifdef QUATRO_SCU_PHYS_BASE
/*  
*   MPCore private memory which is a chip configuration option 
*   (see the MPCore Tech Ref Manal)
*/
#define MPC_SYSTEM_ADDR     QUATRO_SCU_PHYS_BASE

/*  
**  Private MPCore Sub-Systems (which are offset from the MPC_SYSTEM_ADDR)
*/
#define ARM_MPC_SCU_BASE (MPC_SYSTEM_ADDR + 0x0000) // SCU registers
#define ARM_MPC_INT_BASE (MPC_SYSTEM_ADDR + 0x0100) // CPU interrupt interfaces
#define ARM_MPC_INT_CPU0 (MPC_SYSTEM_ADDR + 0x0200) // CPU 0 interrupt intrface
#define ARM_MPC_INT_CPU1 (MPC_SYSTEM_ADDR + 0x0300) // CPU 1 interrupt intrface
#define ARM_MPC_INT_CPU2 (MPC_SYSTEM_ADDR + 0x0400) // CPU 2 interrupt intrface
#define ARM_MPC_INT_CPU3 (MPC_SYSTEM_ADDR + 0x0500) // CPU 3 interrupt intrface
#define ARM_MPC_TIM_BASE (MPC_SYSTEM_ADDR + 0x0600) // CPU timer and watchdog
#define ARM_MPC_TIM_CPU0 (MPC_SYSTEM_ADDR + 0x0700) // CPU0 timer and watchdog
#define ARM_MPC_TIM_CPU1 (MPC_SYSTEM_ADDR + 0x0800) // CPU1 timer and watchdog
#define ARM_MPC_TIM_CPU2 (MPC_SYSTEM_ADDR + 0x0900) // CPU2 timer and watchdog
#define ARM_MPC_TIM_CPU3 (MPC_SYSTEM_ADDR + 0x0A00) // CPU3 timer and watchdog
#define ARM_MPC_INT_GLOB (MPC_SYSTEM_ADDR + 0x1000) // Global Interrupt distributor

/*
**  Some of the 32 bit registers within the private MPCore sub-systems.
*/
#define SCU_CONTROL_OFF     0x00         /*  Control 0x00001FFE  R/W    */
#define SCU_CONFIG_OFF      0x04         /*  Configuration       R/O    */
#define SCU_CPUSTATUS_OFF   0x08         /*  SCU CPU Status -    R/W    */
#define SCU_INVALIDATE_OFF  0x0C         /*  Invalidate all -    W/O    */
#define SCU_PMCR_OFF        0x10         /*  PerfMon Control Regs */

#define SCU_CONTROL_REG     (ARM_MPC_SCU_BASE + SCU_CONTROL_OFF)
#define SCU_CONFIG_REG      (ARM_MPC_SCU_BASE + SCU_CONFIG_OFF)
#define SCU_CPUSTATUS_REG   (ARM_MPC_SCU_BASE + SCU_CPUSTATUS_OFF)
#define SCU_INVALIDATE_REG  (ARM_MPC_SCU_BASE + SCU_INVALIDATE_OFF)
#define SCU_PMCR_REG        (ARM_MPC_SCU_BASE + SCU_PMCR_OFF)

#define SCU_EVENTMONITORS_VA_BASE  __io(scu_p2v(SCU_PMCR_REG))
#endif

extern struct smp_operations   quatro_smp_ops;
extern void quatro_map_scu(void);
extern void quatro_secondary_startup(void);
extern void quatro_cpu_die(unsigned int cpu);

extern void __init quatro_of_irq_init(void);
extern asmlinkage void __exception_irq_entry quatro_handle_irq(struct pt_regs *regs);

#endif
