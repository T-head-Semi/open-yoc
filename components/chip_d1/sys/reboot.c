/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     reboot.c
 * @brief    source file for the reboot
 * @version  V1.0
 * @date     7. April 2020
 ******************************************************************************/

#include <soc.h>
#include <drv/wdt.h>

void drv_reboot(void)
{
    csi_wdt_t wdt_handle;
    uint32_t irq_flag = 0U;

    irq_flag = csi_irq_save();

    csi_wdt_init(&wdt_handle, 0U);
    csi_wdt_set_timeout(&wdt_handle, 0U);
    csi_wdt_start(&wdt_handle);

    /* waiting for reboot */
    while (1);


    csi_irq_restore(irq_flag);
}
