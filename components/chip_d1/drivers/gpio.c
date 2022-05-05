/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     dw_gpio.c
 * @brief
 * @version
 * @date     2020-01-08
 ******************************************************************************/

#include <string.h>

#include <csi_config.h>
#include <drv/gpio.h>
#include <drv/irq.h>
#include <drv/pin.h>
#include <hal_gpio.h>

// extern csi_error_t csi_pin_mode(pin_name_t pin_name, csi_gpio_mode_t mode);

csi_error_t csi_gpio_init(csi_gpio_t *gpio, uint32_t port_idx)
{
    CSI_PARAM_CHK(gpio, CSI_ERROR);
    gpio->dev.idx = 0;
    if (aw_hal_gpio_init() != 0) {
        return CSI_ERROR;
    }
    return CSI_OK;
}

void csi_gpio_uninit(csi_gpio_t *gpio)
{
    // return CSI_UNSUPPORTED;
}

csi_error_t csi_gpio_dir(csi_gpio_t *gpio, uint32_t pin_mask, csi_gpio_dir_t dir)
{
    return CSI_UNSUPPORTED;
}

csi_error_t csi_gpio_mode(csi_gpio_t *gpio, uint32_t pin_mask, csi_gpio_mode_t mode)
{
    return CSI_UNSUPPORTED;
}

csi_error_t csi_gpio_irq_mode(csi_gpio_t *gpio, uint32_t pin_mask, csi_gpio_irq_mode_t mode)
{
    return CSI_UNSUPPORTED;
}

csi_error_t csi_gpio_irq_enable(csi_gpio_t *gpio, uint32_t pin_mask, bool enable)
{
    return CSI_UNSUPPORTED;
}

csi_error_t csi_gpio_debonce(csi_gpio_t *gpio, uint32_t pin_mask, bool enable)
{
    return CSI_UNSUPPORTED;
}

void csi_gpio_toggle(csi_gpio_t *gpio, uint32_t pin_mask)
{
    CSI_PARAM_CHK_NORETVAL(gpio);
    CSI_PARAM_CHK_NORETVAL(pin_mask);

    ///< TODO：翻转GPIO口输出状态，例如，当前GPIO的输出状态是高电平，翻转之后则为低电平
}

void  csi_gpio_write(csi_gpio_t *gpio, uint32_t pin_mask, csi_gpio_pin_state_t value)
{
    CSI_PARAM_CHK_NORETVAL(gpio);
    CSI_PARAM_CHK_NORETVAL(pin_mask);

    ///< TODO：根据value写数据寄存器

}

uint32_t csi_gpio_read(csi_gpio_t *gpio, uint32_t pin_mask)
{
    CSI_PARAM_CHK(gpio, CSI_ERROR);
    CSI_PARAM_CHK(pin_mask, CSI_ERROR);
    uint32_t status = 0U;

    ///< TODO：读取相应引脚的信号输入状态

    return status;

}

csi_error_t  csi_gpio_attach_callback(csi_gpio_t *gpio, void *callback, void *arg)
{
    return CSI_UNSUPPORTED;
}

void csi_gpio_detach_callback(csi_gpio_t *gpio)
{
}

#ifdef CONFIG_PM
csi_error_t dw_gpio_pm_action(csi_dev_t *dev, csi_pm_dev_action_t action)
{
    return CSI_UNSUPPORTED;
}

csi_error_t csi_gpio_enable_pm(csi_gpio_t *gpio)
{
    ///< TODO：注册GPIO低功耗处理函数dw_gpio_pm_action
    return CSI_UNSUPPORTED;
}

void csi_gpio_disable_pm(csi_gpio_t *gpio)
{
}
#endif
