/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <yoc/button.h>
#include <aos/hal/gpio.h>

#include "internal.h"

#define b_gpio_param(b) ((gpio_button_param_t*)(b->param))

static button_t *button_gpio_find(int pin_id)
{
    button_t *b = NULL;

    slist_for_each_entry(&g_button_srv.button_head, b, button_t, next) {
        button_ops_t *ops = &gpio_ops;

        if ((b->ops == ops) && b_gpio_param(b)->pin_id == pin_id) {
            return b;
        }
    }

    return NULL;
}

void pin_event(void *arg)
{
    button_t *button = NULL;
    int idx = (size_t)arg;

    button = button_gpio_find(idx);
    hal_gpio_clear_irq(b_gpio_param(button)->pin_hdl);

    if (button != NULL && (b_gpio_param(button)->pin_hdl != NULL)) {
        button_irq(button);
    }
}

static int gpio_irq_disable(button_t *button)
{
    if (b_gpio_param(button)->active_level == LOW_LEVEL) {
        hal_gpio_disable_irq(b_gpio_param(button)->pin_hdl);
    } else {
        hal_gpio_disable_irq(b_gpio_param(button)->pin_hdl);
    }

    return 0;
}

static int gpio_irq_enable(button_t *button)
{
    if (b_gpio_param(button)->active_level == LOW_LEVEL) {
        hal_gpio_enable_irq(b_gpio_param(button)->pin_hdl, IRQ_TRIGGER_FALLING_EDGE, pin_event, (void *)(size_t)b_gpio_param(button)->pin_id);
    } else {
        hal_gpio_enable_irq(b_gpio_param(button)->pin_hdl, IRQ_TRIGGER_RISING_EDGE, pin_event, (void *)(size_t)b_gpio_param(button)->pin_id);
    }

    return 0;
}

static int gpio_pin_read(button_t *button)
{
    uint32_t val = 0;
    bool is_pressed = false;

    hal_gpio_input_get(b_gpio_param(button)->pin_hdl, &val);

    if (val == b_gpio_param(button)->active_level) {
        is_pressed = true;
    }

    return is_pressed;
}

static int gpio_pin_init(button_t *button)
{
    gpio_dev_t *gpio = (gpio_dev_t *)aos_malloc_check(sizeof(gpio_dev_t));
    
    gpio->port = b_gpio_param(button)->pin_id;

    if (b_gpio_param(button)->active_level == LOW_LEVEL) {
        gpio->config = INPUT_PULL_UP;
    } else {
        gpio->config = INPUT_PULL_DOWN;
    }

    hal_gpio_init(gpio);

    b_gpio_param(button)->pin_hdl = gpio;

    gpio_irq_enable(button);

    return 0;
}

static int gpio_pin_deinit(button_t *button)
{
    hal_gpio_finalize(b_gpio_param(button)->pin_hdl);

    return 0;
}
button_ops_t gpio_ops = {
    .init = gpio_pin_init,
    .deinit = gpio_pin_deinit,
    .read = gpio_pin_read,
    .irq_disable = gpio_irq_disable,
    .irq_enable = gpio_irq_enable,
};
