/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <aos/kernel.h>
#include <ulog/ulog.h>
#include <drv/gpio_pin.h>
#include <soc.h>
#include "board.h"

static csi_gpio_pin_t led_pin;
static int led_init(void)
{
    int ret = 0;
    
    ret = csi_gpio_pin_init(&led_pin, LED_PIN);
    if (ret) {
        printf("init err");
        return -1;
}
    /* Set pull-up mode */
    ret = csi_gpio_pin_mode(&led_pin, GPIO_MODE_PULLNONE);
    if (ret) {
        printf("init err");
        return -1;
}
    /* Set input mode */
    ret = csi_gpio_pin_dir(&led_pin, GPIO_DIRECTION_OUTPUT);
     if (ret) {
        printf("init err");
        return -1;
}

    return 0;
}

static void led_blink(void *arg)
{
    while (1) {
        csi_gpio_pin_write(&led_pin, GPIO_PIN_LOW);  
        aos_msleep(1000);
        csi_gpio_pin_write(&led_pin, GPIO_PIN_HIGH);
        aos_msleep(1000);
    }
}

void led_task_run(void)
{
    led_init();
    aos_task_new("led-task", led_blink, NULL, 4 * 1024);
}