/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     gpio_pin.c
 * @brief
 * @version
 * @date     2020-06-28
 ******************************************************************************/
#include <string.h>
#include <drv/gpio_pin.h>
// #include <drv/gpio.h>
#include <drv/irq.h>
#include <drv/pin.h>
#include <hal_gpio.h>

typedef struct {
    int irq_num;
    csi_gpio_pin_t *pin;
} irq_callback_mgr_t;
static irq_callback_mgr_t g_irq_callback_mgr[GPIO_MAX_PIN];

static irqreturn_t __irq_handlers(int irq, void *arg)
{
    for (int i = 0; i < GPIO_MAX_PIN; i++) {
        if (g_irq_callback_mgr[i].irq_num == irq) {
            if (g_irq_callback_mgr[i].pin->callback) {
                g_irq_callback_mgr[i].pin->callback(g_irq_callback_mgr[i].pin, arg);
            }
            break;
        }
    }
	return 0;
}

csi_error_t csi_gpio_pin_init(csi_gpio_pin_t *pin, pin_name_t pin_name)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);
    if (pin_name > GPIO_MAX_PIN) {
        return CSI_ERROR;
    }
    pin->pin_idx = pin_name;
    g_irq_callback_mgr[pin->pin_idx].irq_num = -1;
    return CSI_OK;
}

void csi_gpio_pin_uninit(csi_gpio_pin_t *pin)
{
    CSI_PARAM_CHK_NORETVAL(pin);
    uint32_t irq;
    pin->callback = NULL;
    pin->arg = NULL;
    if (g_irq_callback_mgr[pin->pin_idx].irq_num != -1) {
        irq = g_irq_callback_mgr[pin->pin_idx].irq_num;
        hal_gpio_irq_disable(irq);
        hal_gpio_irq_free(irq);
    }
}

csi_error_t csi_gpio_pin_attach_callback(csi_gpio_pin_t *pin, void *callback, void *arg)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);
    uint32_t irq;

    if (g_irq_callback_mgr[pin->pin_idx].irq_num == -1) {
        if (hal_gpio_to_irq(pin->pin_idx, &irq)) {
            return CSI_ERROR;
        }
        g_irq_callback_mgr[pin->pin_idx].irq_num = irq;
    }
    pin->callback = callback;
    pin->arg = arg;
    g_irq_callback_mgr[pin->pin_idx].pin = pin;
    return CSI_OK;
}

csi_error_t csi_gpio_pin_dir(csi_gpio_pin_t *pin, csi_gpio_dir_t dir)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);
    hal_gpio_set_direction(pin->pin_idx, (gpio_direction_t) dir);
    return CSI_OK;
}

csi_error_t csi_gpio_pin_mode(csi_gpio_pin_t *pin, csi_gpio_mode_t mode)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);
    gpio_pull_status_t pull;

    if (mode == GPIO_MODE_PULLNONE) {
        pull = GPIO_PULL_DOWN_DISABLED;
    } else if (mode == GPIO_MODE_PULLDOWN) {
        pull = GPIO_PULL_DOWN;
    } else if (mode == GPIO_MODE_PULLUP) {
        pull = GPIO_PULL_UP;
    } else {
        return CSI_UNSUPPORTED;
    }

    hal_gpio_set_pull(pin->pin_idx, pull);

    return CSI_OK;
}

csi_error_t csi_gpio_pin_irq_mode(csi_gpio_pin_t *pin, csi_gpio_irq_mode_t mode)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);
    uint32_t irq;
    gpio_interrupt_mode_t irq_mode;

    if (mode == GPIO_IRQ_MODE_RISING_EDGE) {
        irq_mode = IRQ_TYPE_EDGE_RISING;
    } else if (mode == GPIO_IRQ_MODE_FALLING_EDGE) {
        irq_mode = IRQ_TYPE_EDGE_FALLING;
    } else if (mode == GPIO_IRQ_MODE_BOTH_EDGE) {
        irq_mode = IRQ_TYPE_EDGE_BOTH;
    } else if (mode == GPIO_IRQ_MODE_LOW_LEVEL) {
        irq_mode = IRQ_TYPE_LEVEL_LOW;
    } else if (mode == GPIO_IRQ_MODE_HIGH_LEVEL) {
        irq_mode = IRQ_TYPE_LEVEL_HIGH;
    } else {
        return CSI_ERROR;
    }
    if (g_irq_callback_mgr[pin->pin_idx].irq_num == -1) {
        if (hal_gpio_to_irq(pin->pin_idx, &irq)) {
            return CSI_ERROR;
        }
        g_irq_callback_mgr[pin->pin_idx].irq_num = irq;
    }
    irq = g_irq_callback_mgr[pin->pin_idx].irq_num;
	if (hal_gpio_irq_request(irq, __irq_handlers, irq_mode, pin->arg) < 0) {
        return CSI_ERROR;
    }
    return CSI_OK;
}

csi_error_t csi_gpio_pin_irq_enable(csi_gpio_pin_t *pin, bool enable)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);
    uint32_t irq;

    irq = g_irq_callback_mgr[pin->pin_idx].irq_num;
    if (enable) {
        if (hal_gpio_irq_enable(irq)) {
            return CSI_ERROR;
        }
    } else {
        if (hal_gpio_irq_disable(irq)) {
            return CSI_ERROR;
        }
    }
    return CSI_OK;
}

csi_error_t csi_gpio_pin_debounce(csi_gpio_pin_t *pin, bool enable)
{
    return CSI_OK;
}

void csi_gpio_pin_write(csi_gpio_pin_t *pin, csi_gpio_pin_state_t value)
{
    CSI_PARAM_CHK_NORETVAL(pin);
    hal_gpio_set_data(pin->pin_idx, value);
}

csi_gpio_pin_state_t csi_gpio_pin_read(csi_gpio_pin_t *pin)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);
    gpio_data_t data;

    hal_gpio_get_data(pin->pin_idx, &data);
    return (csi_gpio_pin_state_t)data;
}

