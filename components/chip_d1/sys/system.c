/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */


/******************************************************************************
 * @file     system.c
 * @brief    CSI Device System Source File
 * @version  V1.0
 * @date     02. Oct 2018
 ******************************************************************************/

#include <soc.h>
#include <csi_core.h>
#include <drv/tick.h>
#include <drv/porting.h>
#include <drv/irq.h>
#include <drv/dma.h>
// #include "csr.h"

#if (defined(CONFIG_KERNEL_RHINO) || defined(CONFIG_KERNEL_FREERTOS)) && defined(CONFIG_KERNEL_NONE)
#error "Please check the current system is baremetal or not!!!"
#endif

extern void plic_init(void);

extern void section_data_copy(void);
extern void section_ram_code_copy(void);
extern void section_bss_clear(void);

static void pmp_init(void)
{
    long addr;

    /* 0 ~ 0x20000 can not access */
    addr = 0x20000 >> 2;
    __set_PMPADDR0(addr);
    __set_PMPxCFG(0, 0x88);
}

static void section_init(void)
{
#ifdef CONFIG_XIP
    section_data_copy();
    section_ram_code_copy();
    csi_dcache_clean();
    csi_icache_invalid();
#endif

    section_bss_clear();
}

static void cache_init(void)
{
    /* invalid cache */
    csi_icache_invalid();
    csi_dcache_invalid();
    /* enable cache */
    csi_dcache_enable();
    csi_icache_enable();
}

/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */

static void interrupt_init(void)
{
    /*PLIC 权限控制寄存器,PLIC_CTRL*/
    PLIC->PLIC_PER = 0x1;
    /*机器模式中断响应/完成寄存器*/
    for (int i = 0; i < 1023; i++) {
        PLIC->PLIC_H0_MCLAIM = i;
    }
    /*中断的优先级配置寄存器*/
    for (int i = 0; i < 1023; i++) {
        PLIC->PLIC_PRIO[i] = 31;
    }

    /* set hart threshold 0, allow all interrupt */
    PLIC->PLIC_H0_MTH = 0;

    /* enable msoft interrupt ; Machine_Software_IRQn*/
    uint32_t mie = __get_MIE();
    mie |= (1 << 11 | 1 << 7 | 1 << 3);
    __set_MIE(mie);

    plic_init();
}

void SystemInit(void)
{
    pmp_init();
    section_init();
    cache_init();
    soc_set_sys_freq(24000000);
    interrupt_init();
    csi_tick_init();
}
