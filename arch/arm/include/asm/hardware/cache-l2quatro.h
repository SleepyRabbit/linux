/*
 * arch/arm/include/mach/hardware/cache-l2quatro.h
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
#ifndef __CACHE_L2QUATRO_H
#define __CACHE_L2QUATRO_H

/* L2C Control register */
#define L2C_L2CCTL_OFF            0x00000000
#define L2C_L2CCTL                0xF00A4000
#define L2C_L2CCTL__EN__SHIFT       0
#define L2C_L2CCTL__EN__WIDTH       3
#define L2C_L2CCTL__EN__MASK        0x00000007
#define L2C_L2CCTL__EN__HW_DEFAULT  0x0

/* Tag Most Significant Bit register */
#define L2C_TAG_OFF               0x00000004
#define L2C_TAG                   0xF00A4004
#define L2C_TAG__MSB__SHIFT       0
#define L2C_TAG__MSB__WIDTH       1
#define L2C_TAG__MSB__MASK        0x00000001
#define L2C_TAG__MSB__HW_DEFAULT  0x0

/* Multiple Hit Configuration Register */
#define L2C_MHCFG_OFF             0x00000008
#define L2C_MHCFG                 0xF00A4008
#define L2C_MHCFG__MODE__SHIFT       0
#define L2C_MHCFG__MODE__WIDTH       2
#define L2C_MHCFG__MODE__MASK        0x00000003
#define L2C_MHCFG__MODE__HW_DEFAULT  0x0

/* Address Filtering Min Address */
#define L2C_ADDFLTRMIN_OFF        0x0000000C
#define L2C_ADDFLTRMIN            0xF00A400C
#define L2C_ADDFLTRMIN__ADDR__SHIFT       4
#define L2C_ADDFLTRMIN__ADDR__WIDTH       28
#define L2C_ADDFLTRMIN__ADDR__MASK        0xFFFFFFF0
#define L2C_ADDFLTRMIN__ADDR__HW_DEFAULT  0x0

/* Address Filtering Max Address */
#define L2C_ADDFLTRMAX_OFF        0x00000010
#define L2C_ADDFLTRMAX            0xF00A4010
#define L2C_ADDFLTRMAX__ADDR__SHIFT       4
#define L2C_ADDFLTRMAX__ADDR__WIDTH       28
#define L2C_ADDFLTRMAX__ADDR__MASK        0xFFFFFFF0
#define L2C_ADDFLTRMAX__ADDR__HW_DEFAULT  0x0

/* L2C Address Filter Control register */
#define L2C_ADDFLTRCTL_OFF        0x00000014
#define L2C_ADDFLTRCTL            0xF00A4014
#define L2C_ADDFLTRCTL__EN__SHIFT       0
#define L2C_ADDFLTRCTL__EN__WIDTH       1
#define L2C_ADDFLTRCTL__EN__MASK        0x00000001
#define L2C_ADDFLTRCTL__EN__HW_DEFAULT  0x0

/* Lock Away Configuration register */
#define L2C_LACFG_OFF             0x00000018
#define L2C_LACFG                 0xF00A4018
#define L2C_LACFG__INDEX__SHIFT       0
#define L2C_LACFG__INDEX__WIDTH       3
#define L2C_LACFG__INDEX__MASK        0x00000007
#define L2C_LACFG__INDEX__HW_DEFAULT  0x0

/* Lock Control register */
#define L2C_LOCKCTL_OFF           0x0000001C
#define L2C_LOCKCTL               0xF00A401C
#define L2C_LOCKCTL__EN__SHIFT       0
#define L2C_LOCKCTL__EN__WIDTH       1
#define L2C_LOCKCTL__EN__MASK        0x00000001
#define L2C_LOCKCTL__EN__HW_DEFAULT  0x0

/* L2C Status register */
#define L2C_L2CSTAT_OFF           0x00000020
#define L2C_L2CSTAT               0xF00A4020
#define L2C_L2CSTAT__VMHEC__SHIFT       4
#define L2C_L2CSTAT__VMHEC__WIDTH       1
#define L2C_L2CSTAT__VMHEC__MASK        0x00000010
#define L2C_L2CSTAT__VMHEC__HW_DEFAULT  0x0
#define L2C_L2CSTAT__IBRERR__SHIFT       3
#define L2C_L2CSTAT__IBRERR__WIDTH       1
#define L2C_L2CSTAT__IBRERR__MASK        0x00000008
#define L2C_L2CSTAT__IBRERR__HW_DEFAULT  0x0
#define L2C_L2CSTAT__TBRERR__SHIFT       2
#define L2C_L2CSTAT__TBRERR__WIDTH       1
#define L2C_L2CSTAT__TBRERR__MASK        0x00000004
#define L2C_L2CSTAT__TBRERR__HW_DEFAULT  0x0
#define L2C_L2CSTAT__TPRERR__SHIFT       1
#define L2C_L2CSTAT__TPRERR__WIDTH       1
#define L2C_L2CSTAT__TPRERR__MASK        0x00000002
#define L2C_L2CSTAT__TPRERR__HW_DEFAULT  0x0
#define L2C_L2CSTAT__MHEC__SHIFT       0
#define L2C_L2CSTAT__MHEC__WIDTH       1
#define L2C_L2CSTAT__MHEC__MASK        0x00000001
#define L2C_L2CSTAT__MHEC__HW_DEFAULT  0x0

/* Clear Configuration register */
#define L2C_CLRCFG_OFF            0x00000024
#define L2C_CLRCFG                0xF00A4024
#define L2C_CLRCFG__TM_INDEX__SHIFT       0
#define L2C_CLRCFG__TM_INDEX__WIDTH       3
#define L2C_CLRCFG__TM_INDEX__MASK        0x00000007
#define L2C_CLRCFG__TM_INDEX__HW_DEFAULT  0x7

/* Clear Control register */
#define L2C_CLRCTL_OFF            0x00000028
#define L2C_CLRCTL                0xF00A4028
#define L2C_CLRCTL__RRTM_START__SHIFT       1
#define L2C_CLRCTL__RRTM_START__WIDTH       1
#define L2C_CLRCTL__RRTM_START__MASK        0x00000002
#define L2C_CLRCTL__RRTM_START__HW_DEFAULT  0x0
#define L2C_CLRCTL__TM_START__SHIFT       0
#define L2C_CLRCTL__TM_START__WIDTH       1
#define L2C_CLRCTL__TM_START__MASK        0x00000001
#define L2C_CLRCTL__TM_START__HW_DEFAULT  0x0

/* Clear Status */
#define L2C_CLRSTAT_OFF           0x0000002C
#define L2C_CLRSTAT               0xF00A402C
#define L2C_CLRSTAT__RRTM_DONE__SHIFT       3
#define L2C_CLRSTAT__RRTM_DONE__WIDTH       1
#define L2C_CLRSTAT__RRTM_DONE__MASK        0x00000008
#define L2C_CLRSTAT__RRTM_DONE__HW_DEFAULT  0x0
#define L2C_CLRSTAT__RRTM_BUSY__SHIFT       2
#define L2C_CLRSTAT__RRTM_BUSY__WIDTH       1
#define L2C_CLRSTAT__RRTM_BUSY__MASK        0x00000004
#define L2C_CLRSTAT__RRTM_BUSY__HW_DEFAULT  0x0
#define L2C_CLRSTAT__TM_DONE__SHIFT       1
#define L2C_CLRSTAT__TM_DONE__WIDTH       1
#define L2C_CLRSTAT__TM_DONE__MASK        0x00000002
#define L2C_CLRSTAT__TM_DONE__HW_DEFAULT  0x0
#define L2C_CLRSTAT__TM_BUSY__SHIFT       0
#define L2C_CLRSTAT__TM_BUSY__WIDTH       1
#define L2C_CLRSTAT__TM_BUSY__MASK        0x00000001
#define L2C_CLRSTAT__TM_BUSY__HW_DEFAULT  0x0

/* Cache Line Invalidate Start Address */
#define L2C_CLISTART_OFF          0x00000030
#define L2C_CLISTART              0xF00A4030
#define L2C_CLISTART__ADDR__SHIFT       5
#define L2C_CLISTART__ADDR__WIDTH       27
#define L2C_CLISTART__ADDR__MASK        0xFFFFFFE0
#define L2C_CLISTART__ADDR__HW_DEFAULT  0x0

/* Cache Line Invalidate Stop Address */
#define L2C_CLISTOP_OFF           0x00000034
#define L2C_CLISTOP               0xF00A4034
#define L2C_CLISTOP__ADDR__SHIFT       5
#define L2C_CLISTOP__ADDR__WIDTH       27
#define L2C_CLISTOP__ADDR__MASK        0xFFFFFFE0
#define L2C_CLISTOP__ADDR__HW_DEFAULT  0x0

/* Cache Line Invalidate Configuration */
#define L2C_CLICFG_OFF            0x00000038
#define L2C_CLICFG                0xF00A4038
#define L2C_CLICFG__MODE__SHIFT       0
#define L2C_CLICFG__MODE__WIDTH       2
#define L2C_CLICFG__MODE__MASK        0x00000003
#define L2C_CLICFG__MODE__HW_DEFAULT  0x0

/* Cache Line Invalidate Control */
#define L2C_CLICTL_OFF            0x0000003C
#define L2C_CLICTL                0xF00A403C
#define L2C_CLICTL__START__SHIFT       0
#define L2C_CLICTL__START__WIDTH       1
#define L2C_CLICTL__START__MASK        0x00000001
#define L2C_CLICTL__START__HW_DEFAULT  0x0

/* Cache Line Invalidate Status */
#define L2C_CLISTAT_OFF           0x00000040
#define L2C_CLISTAT               0xF00A4040
#define L2C_CLISTAT__DONE__SHIFT       1
#define L2C_CLISTAT__DONE__WIDTH       1
#define L2C_CLISTAT__DONE__MASK        0x00000002
#define L2C_CLISTAT__DONE__HW_DEFAULT  0x0
#define L2C_CLISTAT__BUSY__SHIFT       0
#define L2C_CLISTAT__BUSY__WIDTH       1
#define L2C_CLISTAT__BUSY__MASK        0x00000001
#define L2C_CLISTAT__BUSY__HW_DEFAULT  0x0

/* Performance Statistics Counter Configuration */
#define L2C_PSCCFG_OFF            0x00000044
#define L2C_PSCCFG                0xF00A4044
#define L2C_PSCCFG__HMSEL__SHIFT       0
#define L2C_PSCCFG__HMSEL__WIDTH       1
#define L2C_PSCCFG__HMSEL__MASK        0x00000001
#define L2C_PSCCFG__HMSEL__HW_DEFAULT  0x0

/* Performance Statistics Counter Control */
#define L2C_PSCCTL_OFF            0x00000048
#define L2C_PSCCTL                0xF00A4048
#define L2C_PSCCTL__EN__SHIFT       0
#define L2C_PSCCTL__EN__WIDTH       1
#define L2C_PSCCTL__EN__MASK        0x00000001
#define L2C_PSCCTL__EN__HW_DEFAULT  0x0

/* Performance Statistic Cycle Counter */
#define L2C_PSCCNT_OFF            0x0000004C
#define L2C_PSCCNT                0xF00A404C
#define L2C_PSCCNT__STAT__SHIFT       0
#define L2C_PSCCNT__STAT__WIDTH       32
#define L2C_PSCCNT__STAT__MASK        0xFFFFFFFF
#define L2C_PSCCNT__STAT__HW_DEFAULT  0x0

/* Performance Statistic Hit/Miss Counter */
#define L2C_PSHMCNT_OFF           0x00000050
#define L2C_PSHMCNT               0xF00A4050
#define L2C_PSHMCNT__STAT__SHIFT       0
#define L2C_PSHMCNT__STAT__WIDTH       32
#define L2C_PSHMCNT__STAT__MASK        0xFFFFFFFF
#define L2C_PSHMCNT__STAT__HW_DEFAULT  0x0

/* Performance Statistic Wait Cycle Counter */
#define L2C_PSWCCNT_OFF           0x00000054
#define L2C_PSWCCNT               0xF00A4054
#define L2C_PSWCCNT__STAT__SHIFT       0
#define L2C_PSWCCNT__STAT__WIDTH       32
#define L2C_PSWCCNT__STAT__MASK        0xFFFFFFFF
#define L2C_PSWCCNT__STAT__HW_DEFAULT  0x0

/* Performance Statistic Multiple Hit Counter */
#define L2C_PSMHCNT_OFF           0x00000058
#define L2C_PSMHCNT               0xF00A4058
#define L2C_PSMHCNT__STAT__SHIFT       0
#define L2C_PSMHCNT__STAT__WIDTH       32
#define L2C_PSMHCNT__STAT__MASK        0xFFFFFFFF
#define L2C_PSMHCNT__STAT__HW_DEFAULT  0x0

/* Performance Statistic Multiple Hit Detected Counter */
#define L2C_PSMHDCNT_OFF          0x0000005C
#define L2C_PSMHDCNT              0xF00A405C
#define L2C_PSMHDCNT__STAT__SHIFT       0
#define L2C_PSMHDCNT__STAT__WIDTH       32
#define L2C_PSMHDCNT__STAT__MASK        0xFFFFFFFF
#define L2C_PSMHDCNT__STAT__HW_DEFAULT  0x0

/* L2C Interrupt Flag Register */
#define L2C_L2CINTFR_OFF          0x00000060
#define L2C_L2CINTFR              0xF00A4060
#define L2C_L2CINTFR__IBRE__SHIFT       7
#define L2C_L2CINTFR__IBRE__WIDTH       1
#define L2C_L2CINTFR__IBRE__MASK        0x00000080
#define L2C_L2CINTFR__IBRE__HW_DEFAULT  0x0
#define L2C_L2CINTFR__TBRE__SHIFT       6
#define L2C_L2CINTFR__TBRE__WIDTH       1
#define L2C_L2CINTFR__TBRE__MASK        0x00000040
#define L2C_L2CINTFR__TBRE__HW_DEFAULT  0x0
#define L2C_L2CINTFR__TPRE__SHIFT       5
#define L2C_L2CINTFR__TPRE__WIDTH       1
#define L2C_L2CINTFR__TPRE__MASK        0x00000020
#define L2C_L2CINTFR__TPRE__HW_DEFAULT  0x0
#define L2C_L2CINTFR__CLID__SHIFT       4
#define L2C_L2CINTFR__CLID__WIDTH       1
#define L2C_L2CINTFR__CLID__MASK        0x00000010
#define L2C_L2CINTFR__CLID__HW_DEFAULT  0x0
#define L2C_L2CINTFR__RRTMD__SHIFT       3
#define L2C_L2CINTFR__RRTMD__WIDTH       1
#define L2C_L2CINTFR__RRTMD__MASK        0x00000008
#define L2C_L2CINTFR__RRTMD__HW_DEFAULT  0x0
#define L2C_L2CINTFR__TMD__SHIFT       2
#define L2C_L2CINTFR__TMD__WIDTH       1
#define L2C_L2CINTFR__TMD__MASK        0x00000004
#define L2C_L2CINTFR__TMD__HW_DEFAULT  0x0
#define L2C_L2CINTFR__VMHEC__SHIFT       1
#define L2C_L2CINTFR__VMHEC__WIDTH       1
#define L2C_L2CINTFR__VMHEC__MASK        0x00000002
#define L2C_L2CINTFR__VMHEC__HW_DEFAULT  0x0
#define L2C_L2CINTFR__MHEC__SHIFT       0
#define L2C_L2CINTFR__MHEC__WIDTH       1
#define L2C_L2CINTFR__MHEC__MASK        0x00000001
#define L2C_L2CINTFR__MHEC__HW_DEFAULT  0x0

/* L2C Interrupt Clear Register */
#define L2C_L2CINTCR_OFF          0x00000064
#define L2C_L2CINTCR              0xF00A4064
#define L2C_L2CINTCR__IBRE__SHIFT       7
#define L2C_L2CINTCR__IBRE__WIDTH       1
#define L2C_L2CINTCR__IBRE__MASK        0x00000080
#define L2C_L2CINTCR__IBRE__HW_DEFAULT  0x0
#define L2C_L2CINTCR__TBRE__SHIFT       6
#define L2C_L2CINTCR__TBRE__WIDTH       1
#define L2C_L2CINTCR__TBRE__MASK        0x00000040
#define L2C_L2CINTCR__TBRE__HW_DEFAULT  0x0
#define L2C_L2CINTCR__TPRE__SHIFT       5
#define L2C_L2CINTCR__TPRE__WIDTH       1
#define L2C_L2CINTCR__TPRE__MASK        0x00000020
#define L2C_L2CINTCR__TPRE__HW_DEFAULT  0x0
#define L2C_L2CINTCR__CLID__SHIFT       4
#define L2C_L2CINTCR__CLID__WIDTH       1
#define L2C_L2CINTCR__CLID__MASK        0x00000010
#define L2C_L2CINTCR__CLID__HW_DEFAULT  0x0
#define L2C_L2CINTCR__RRTMD__SHIFT       3
#define L2C_L2CINTCR__RRTMD__WIDTH       1
#define L2C_L2CINTCR__RRTMD__MASK        0x00000008
#define L2C_L2CINTCR__RRTMD__HW_DEFAULT  0x0
#define L2C_L2CINTCR__TMD__SHIFT       2
#define L2C_L2CINTCR__TMD__WIDTH       1
#define L2C_L2CINTCR__TMD__MASK        0x00000004
#define L2C_L2CINTCR__TMD__HW_DEFAULT  0x0
#define L2C_L2CINTCR__VMHEC__SHIFT       1
#define L2C_L2CINTCR__VMHEC__WIDTH       1
#define L2C_L2CINTCR__VMHEC__MASK        0x00000002
#define L2C_L2CINTCR__VMHEC__HW_DEFAULT  0x0
#define L2C_L2CINTCR__MHEC__SHIFT       0
#define L2C_L2CINTCR__MHEC__WIDTH       1
#define L2C_L2CINTCR__MHEC__MASK        0x00000001
#define L2C_L2CINTCR__MHEC__HW_DEFAULT  0x0

/* L2C Interrupt Enable Register */
#define L2C_L2CINTER_OFF          0x00000068
#define L2C_L2CINTER              0xF00A4068
#define L2C_L2CINTER__IBRE__SHIFT       7
#define L2C_L2CINTER__IBRE__WIDTH       1
#define L2C_L2CINTER__IBRE__MASK        0x00000080
#define L2C_L2CINTER__IBRE__HW_DEFAULT  0x0
#define L2C_L2CINTER__TBRE__SHIFT       6
#define L2C_L2CINTER__TBRE__WIDTH       1
#define L2C_L2CINTER__TBRE__MASK        0x00000040
#define L2C_L2CINTER__TBRE__HW_DEFAULT  0x0
#define L2C_L2CINTER__TPRE__SHIFT       5
#define L2C_L2CINTER__TPRE__WIDTH       1
#define L2C_L2CINTER__TPRE__MASK        0x00000020
#define L2C_L2CINTER__TPRE__HW_DEFAULT  0x0
#define L2C_L2CINTER__CLID__SHIFT       4
#define L2C_L2CINTER__CLID__WIDTH       1
#define L2C_L2CINTER__CLID__MASK        0x00000010
#define L2C_L2CINTER__CLID__HW_DEFAULT  0x0
#define L2C_L2CINTER__RRTMD__SHIFT       3
#define L2C_L2CINTER__RRTMD__WIDTH       1
#define L2C_L2CINTER__RRTMD__MASK        0x00000008
#define L2C_L2CINTER__RRTMD__HW_DEFAULT  0x0
#define L2C_L2CINTER__TMD__SHIFT       2
#define L2C_L2CINTER__TMD__WIDTH       1
#define L2C_L2CINTER__TMD__MASK        0x00000004
#define L2C_L2CINTER__TMD__HW_DEFAULT  0x0
#define L2C_L2CINTER__VMHEC__SHIFT       1
#define L2C_L2CINTER__VMHEC__WIDTH       1
#define L2C_L2CINTER__VMHEC__MASK        0x00000002
#define L2C_L2CINTER__VMHEC__HW_DEFAULT  0x0
#define L2C_L2CINTER__MHEC__SHIFT       0
#define L2C_L2CINTER__MHEC__WIDTH       1
#define L2C_L2CINTER__MHEC__MASK        0x00000001
#define L2C_L2CINTER__MHEC__HW_DEFAULT  0x0

/* L2C FIFO Status register */
#define L2C_L2CFSTAT_OFF          0x00000070
#define L2C_L2CFSTAT              0xF00A4070
#define L2C_L2CFSTAT__IBRFPHE__SHIFT       19
#define L2C_L2CFSTAT__IBRFPHE__WIDTH       1
#define L2C_L2CFSTAT__IBRFPHE__MASK        0x00080000
#define L2C_L2CFSTAT__IBRFPHE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__IBRFPPE__SHIFT       18
#define L2C_L2CFSTAT__IBRFPPE__WIDTH       1
#define L2C_L2CFSTAT__IBRFPPE__MASK        0x00040000
#define L2C_L2CFSTAT__IBRFPPE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__IBRFF__SHIFT       17
#define L2C_L2CFSTAT__IBRFF__WIDTH       1
#define L2C_L2CFSTAT__IBRFF__MASK        0x00020000
#define L2C_L2CFSTAT__IBRFF__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__IBRFE__SHIFT       16
#define L2C_L2CFSTAT__IBRFE__WIDTH       1
#define L2C_L2CFSTAT__IBRFE__MASK        0x00010000
#define L2C_L2CFSTAT__IBRFE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__IBLFPHE__SHIFT       15
#define L2C_L2CFSTAT__IBLFPHE__WIDTH       1
#define L2C_L2CFSTAT__IBLFPHE__MASK        0x00008000
#define L2C_L2CFSTAT__IBLFPHE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__IBLFPPE__SHIFT       14
#define L2C_L2CFSTAT__IBLFPPE__WIDTH       1
#define L2C_L2CFSTAT__IBLFPPE__MASK        0x00004000
#define L2C_L2CFSTAT__IBLFPPE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__IBLFF__SHIFT       13
#define L2C_L2CFSTAT__IBLFF__WIDTH       1
#define L2C_L2CFSTAT__IBLFF__MASK        0x00002000
#define L2C_L2CFSTAT__IBLFF__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__IBLFE__SHIFT       12
#define L2C_L2CFSTAT__IBLFE__WIDTH       1
#define L2C_L2CFSTAT__IBLFE__MASK        0x00001000
#define L2C_L2CFSTAT__IBLFE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__IBCFPHE__SHIFT       11
#define L2C_L2CFSTAT__IBCFPHE__WIDTH       1
#define L2C_L2CFSTAT__IBCFPHE__MASK        0x00000800
#define L2C_L2CFSTAT__IBCFPHE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__IBCFPPE__SHIFT       10
#define L2C_L2CFSTAT__IBCFPPE__WIDTH       1
#define L2C_L2CFSTAT__IBCFPPE__MASK        0x00000400
#define L2C_L2CFSTAT__IBCFPPE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__IBCFF__SHIFT       9
#define L2C_L2CFSTAT__IBCFF__WIDTH       1
#define L2C_L2CFSTAT__IBCFF__MASK        0x00000200
#define L2C_L2CFSTAT__IBCFF__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__IBCFE__SHIFT       8
#define L2C_L2CFSTAT__IBCFE__WIDTH       1
#define L2C_L2CFSTAT__IBCFE__MASK        0x00000100
#define L2C_L2CFSTAT__IBCFE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__TBRFPHE__SHIFT       7
#define L2C_L2CFSTAT__TBRFPHE__WIDTH       1
#define L2C_L2CFSTAT__TBRFPHE__MASK        0x00000080
#define L2C_L2CFSTAT__TBRFPHE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__TBRFPPE__SHIFT       6
#define L2C_L2CFSTAT__TBRFPPE__WIDTH       1
#define L2C_L2CFSTAT__TBRFPPE__MASK        0x00000040
#define L2C_L2CFSTAT__TBRFPPE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__TBRFF__SHIFT       5
#define L2C_L2CFSTAT__TBRFF__WIDTH       1
#define L2C_L2CFSTAT__TBRFF__MASK        0x00000020
#define L2C_L2CFSTAT__TBRFF__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__TBRFE__SHIFT       4
#define L2C_L2CFSTAT__TBRFE__WIDTH       1
#define L2C_L2CFSTAT__TBRFE__MASK        0x00000010
#define L2C_L2CFSTAT__TBRFE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__AFPHE__SHIFT       3
#define L2C_L2CFSTAT__AFPHE__WIDTH       1
#define L2C_L2CFSTAT__AFPHE__MASK        0x00000008
#define L2C_L2CFSTAT__AFPHE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__AFPPE__SHIFT       2
#define L2C_L2CFSTAT__AFPPE__WIDTH       1
#define L2C_L2CFSTAT__AFPPE__MASK        0x00000004
#define L2C_L2CFSTAT__AFPPE__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__AFF__SHIFT       1
#define L2C_L2CFSTAT__AFF__WIDTH       1
#define L2C_L2CFSTAT__AFF__MASK        0x00000002
#define L2C_L2CFSTAT__AFF__HW_DEFAULT  0x0
#define L2C_L2CFSTAT__AFE__SHIFT       0
#define L2C_L2CFSTAT__AFE__WIDTH       1
#define L2C_L2CFSTAT__AFE__MASK        0x00000001
#define L2C_L2CFSTAT__AFE__HW_DEFAULT  0x0

/* Round Robin Tag Memory */
#define L2C_RRTMEM_OFF            0x00000800
#define L2C_RRTMEM                0xF00A4800
#define L2C_RRTMEM_ENDOFF         0x00000FFF
#define L2C_RRTMEM_END            0xF00A4FFF
#define L2C_RRTMEM__RRCNT__SHIFT       3
#define L2C_RRTMEM__RRCNT__WIDTH       3
#define L2C_RRTMEM__RRCNT__MASK        0x00000038
#define L2C_RRTMEM__RRCNT__HW_DEFAULT  undefined
#define L2C_RRTMEM__RRCNTB__SHIFT       0
#define L2C_RRTMEM__RRCNTB__WIDTH       3
#define L2C_RRTMEM__RRCNTB__MASK        0x00000007
#define L2C_RRTMEM__RRCNTB__HW_DEFAULT  undefined

void l2quatro_init(int enable);
int l2quatro_of_init(void);
int cache_l2quatro_enabled(void);

#endif

