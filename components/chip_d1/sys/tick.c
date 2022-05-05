/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */
/******************************************************************************
 * @file     tick.c
 * @brief    Source File for tick
 * @version  V1.0
 * @date     7. April 2020
 ******************************************************************************/

#include <soc.h>
#include <csi_config.h>
#include <sys_clk.h>
#include <drv/common.h>
#include <drv/irq.h>
#include <drv/tick.h>
#include <drv/porting.h>
#include <drv/timer.h>

#define __WEAK         __attribute__((weak))

static csi_dev_t tick_dev;

static volatile uint32_t csi_tick = 0U;
static volatile uint32_t last_time_ms = 0U;
static volatile uint64_t last_time_us = 0U;

void csi_tick_increase(void)
{
    csi_tick++;
}

static void tick_irq_handler(csi_timer_t *timer_handle, void *arg)
{
    csi_tick_increase();
    csi_clint_config(CORET_BASE, (soc_get_coretim_freq()/ CONFIG_SYSTICK_HZ), CORET_IRQn);
#if defined(CONFIG_KERNEL_RHINO)
    extern void krhino_tick_proc(void);
    krhino_tick_proc();
#elif defined(CONFIG_KERNEL_FREERTOS)
    xPortSysTickHandler();
#elif defined(CONFIG_KERNEL_UCOS)
    OSTimeTick();
#endif
}

csi_error_t csi_tick_init(void)
{
    csi_tick = 0U;
    tick_dev.irq_num = (uint8_t)CORET_IRQn;

    csi_plic_set_prio(PLIC_BASE, CORET_IRQn, 31U);

    csi_irq_attach((uint32_t)tick_dev.irq_num, &tick_irq_handler, &tick_dev);
    csi_clint_config(CORET_BASE, (soc_get_coretim_freq() / CONFIG_SYSTICK_HZ), CORET_IRQn);
    csi_irq_enable((uint32_t)tick_dev.irq_num);

    return CSI_OK;
}

void csi_tick_uninit(void)
{
    csi_irq_disable((uint32_t)tick_dev.irq_num);
    csi_irq_detach((uint32_t)tick_dev.irq_num);
}

uint32_t csi_tick_get(void)
{
    return csi_tick;
}

uint32_t csi_tick_get_ms(void)
{
    uint32_t time;

    time = (uint32_t)(csi_clint_get_value() * 1000U / (uint64_t)soc_get_coretim_freq());
    last_time_ms = time;
    return time;
}

uint64_t csi_tick_get_us(void)
{
    uint64_t time;

    time = csi_clint_get_value() * 1000U * 1000U / (uint64_t)soc_get_coretim_freq();
    last_time_us = time;
    return time;
}

static void _500usdelay(void)
{
    uint64_t start = csi_clint_get_value();
    uint64_t cur;
    uint32_t cnt = (soc_get_coretim_freq() / 1000U / 2U);

    while (1) {
        cur = csi_clint_get_value();

        if (start > cur) {
            if ((start - cur) >= cnt) {
                break;
            }
        } else {
            if (cur - start >= cnt) {
                break;
            }
        }
    }
}

__WEAK void mdelay(uint32_t ms)
{
    while (ms) {
        ms--;
        _500usdelay();
        _500usdelay();
    }
}

static void _10udelay(void)
{
    uint64_t start = csi_clint_get_value();
    uint32_t cnt = (soc_get_coretim_freq() / 1000U / 100U);

    while (1) {
        uint64_t cur = csi_clint_get_value();

        if (start > cur) {
            if ((start - cur) >= cnt) {
                break;
            }
        } else {
            if (cur - start >= cnt) {
                break;
            }
        }
    }
}

/**
 * Ps: At least delay over 10us
*/
void udelay(uint32_t us)
{
    us /= 10U;

    while (us) {
        us--;
        _10udelay();
    }
}

