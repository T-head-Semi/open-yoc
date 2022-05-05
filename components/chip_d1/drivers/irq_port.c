/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     irq_port.c
 * @brief    CSI Source File for irq port
 * @version  V1.0
 * @date     17. Mar 2020
 * @name     dw_uart
 ******************************************************************************/

#include <csi_config.h>
#include <stdbool.h>
#include <csi_core.h>
#include <soc.h>

void soc_irq_enable(uint32_t irq_num)
{
#if defined(CONFIG_SYSTEM_SECURE)
    csi_plic_enable_sirq(PLIC_BASE, (int32_t)irq_num);
#elif defined(CONFIG_MMU)
    csi_vic_enable_irq((int32_t)irq_num);
#else
    csi_plic_enable_irq(PLIC_BASE, (int32_t)irq_num);
#endif
}

void soc_irq_disable(uint32_t irq_num)
{
#if defined(CONFIG_SYSTEM_SECURE)
    csi_plic_disable_sirq(PLIC_BASE, (int32_t)irq_num);
#elif defined(CONFIG_MMU)
    csi_vic_disable_irq((int32_t)irq_num);
#else
    csi_plic_disable_irq(PLIC_BASE, (int32_t)irq_num);
#endif
}

bool soc_irq_is_enabled(uint32_t irq_num)
{
    bool ret;

    if (csi_plic_get_enabled_irq(PLIC_BASE, (int32_t)irq_num)) {
        ret = true;
    } else {
        ret = false;
    }

    return ret;
}

void soc_irq_priority(uint32_t irq_num, uint32_t priority)
{
    csi_plic_set_prio(PLIC_BASE, (int32_t)irq_num, priority);
}

void soc_irq_enable_wakeup(uint32_t irq_num)
{
}

void soc_irq_disable_wakeup(uint32_t irq_num)
{
}

uint32_t soc_irq_get_irq_num(void)
{
#ifdef __CSKY__
    return ((__get_PSR() >> 16U) & 0xFFU) - 32U;
#elif defined(__riscv)
#if defined(CONFIG_CPU_C906) || defined(CONFIG_CPU_C910)
#ifdef CONFIG_MMU
    uint32_t num = PLIC->PLIC_H0_SCLAIM;
#else
    uint32_t num = PLIC->PLIC_H0_MCLAIM;
#endif
    return num;
#endif
    return (__get_MCAUSE() & 0x3FF);
#endif
}

void soc_irq_end(uint32_t irq_num)
{
#if defined(__riscv)
#if defined(CONFIG_CPU_C906) || defined(CONFIG_CPU_C910)
#ifdef CONFIG_MMU
    PLIC->PLIC_H0_SCLAIM = irq_num;
#else
    PLIC->PLIC_H0_MCLAIM = irq_num;
#endif
#endif
#endif
}
