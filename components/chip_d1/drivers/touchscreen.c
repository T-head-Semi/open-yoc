/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     touch.c
 * @brief
 * @version
 * @date     2021-08-20
 ******************************************************************************/

#include <string.h>
#include <drv/touchscreen.h>
#include <drv/common.h>
#include "touch_ll.h"

// ctp@14 {
//     compatible = "allwinner,goodix";
//     device_type = "ctp";
//     reg = <0x14>;
//     status = "okay";
//     ctp_name = "gt9xxnew_ts";
//     ctp_twi_id = <0x2>;
//     ctp_twi_addr = <0x14>;
//     ctp_screen_max_x = <0x500>;
//     ctp_screen_max_y = <0x320>;
//     ctp_revert_x_flag = <0x1>;
//     ctp_revert_y_flag = <0x1>;
//     ctp_exchange_x_y_flag = <0x1>;
//     ctp_int_port = <&pio PG 14 GPIO_ACTIVE_HIGH>;
//     ctp_wakeup = <&pio PG 15 GPIO_ACTIVE_HIGH>;

static touch_cls_t g_touch_cls;

csi_error_t csi_touchscreen_init(csi_touchscreen_t *touch)
{
    CSI_PARAM_CHK(touch, CSI_ERROR);
    hal_sem_t halsem = hal_sem_create(0);
    CSI_PARAM_CHK(halsem, CSI_ERROR);
    touch->isr_sem = &halsem->sem;
    CSI_PARAM_CHK(touch->isr_sem, CSI_ERROR);

    touch_gt9xx_register(&g_touch_cls);
    touch_config_t config;
    config.slave_addr = touch->iic_addr;
    config.hal_isr_sem = halsem;
    config.iic_port = touch->iic_port;
    if (g_touch_cls.init(&g_touch_cls, &config)) {
        return CSI_ERROR;
    }
    return CSI_OK;
}

csi_error_t csi_touchscreen_read_point(csi_touchscreen_t *touch, touch_message_t msg)
{
    CSI_PARAM_CHK(touch, CSI_ERROR);
    CSI_PARAM_CHK(touch->isr_sem, CSI_ERROR);
    CSI_PARAM_CHK(msg, CSI_ERROR);
    if (g_touch_cls.read_point(&g_touch_cls, msg) != 0) {
        return CSI_ERROR;
    }
    return CSI_OK;
}

csi_error_t csi_touchscreen_deinit(csi_touchscreen_t *touch)
{
    CSI_PARAM_CHK(touch, CSI_ERROR);
    g_touch_cls.deinit(&g_touch_cls);
    if (g_touch_cls.config.hal_isr_sem)
        hal_sem_delete(g_touch_cls.config.hal_isr_sem);
    return CSI_OK;
}