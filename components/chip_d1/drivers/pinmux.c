/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */


/******************************************************************************
 * @file     pinmux.c
 * @brief    source file for the pinmux
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <drv/pin.h>
#include <drv/gpio.h>
#include "soc.h"
#include "hal_gpio.h"

csi_error_t csi_pin_set_mux(pin_name_t pin_name, pin_func_t pin_func)
{
    int ret = hal_gpio_pinmux_set_function(pin_name, (gpio_muxsel_t)pin_func);
    if (ret < 0) {
        return CSI_ERROR;
    }
    return CSI_OK;
}

csi_error_t csi_pin_mode(pin_name_t pin_name, csi_gpio_mode_t mode)
{
    gpio_pull_status_t pull_status;
    csi_error_t ret = CSI_OK;

    switch (mode) {
        case GPIO_MODE_OPEN_DRAIN:
        case GPIO_MODE_PUSH_PULL:
            ret = CSI_UNSUPPORTED;
            break;
        case GPIO_MODE_PULLUP:
            pull_status = GPIO_PULL_UP;
            break;
        case GPIO_MODE_PULLDOWN:
            pull_status = GPIO_PULL_DOWN;
            break;
        case GPIO_MODE_PULLNONE:
            pull_status = GPIO_PULL_DOWN_DISABLED;
            break;
        default:
            ret = CSI_ERROR;
            break;
    }
    if (ret == CSI_OK) {
        if (hal_gpio_set_pull(pin_name, pull_status) < 0) {
            ret = CSI_ERROR;
        }
    }
    return ret;
}

pin_func_t csi_pin_get_mux(pin_name_t pin_name)
{
    gpio_muxsel_t muxsel;

    int ret = hal_gpio_pinmux_get_function(pin_name, &muxsel);
    if (ret < 0) {
        return 0;
    }
    return (pin_func_t)muxsel;
}

csi_error_t csi_pin_speed(pin_name_t pin_name, csi_pin_speed_t speed)
{
    return CSI_UNSUPPORTED;
}

csi_error_t csi_pin_drive(pin_name_t pin_name, csi_pin_drive_t drive)
{
    int ret = hal_gpio_set_driving_level(pin_name, (gpio_driving_level_t)drive);
    if (ret < 0) {
        return CSI_ERROR;
    }
    return CSI_OK;
}
