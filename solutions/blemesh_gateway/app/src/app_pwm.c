/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <drv/pwm.h>

#include "app_main.h"

#define TAG "app"

#define PWM_NUM 3

typedef struct {
    pwm_handle_t handle;
    uint8_t      ch;
} app_pwm_info_t;

static app_pwm_info_t app_pwm_info[PWM_NUM] = {
    {NULL, -1},
    {NULL, -1},
    {NULL, -1},
};

static int8_t get_free_info_id(uint8_t ch)
{
    int8_t idx = -2;
    for (uint8_t i = 0; i < PWM_NUM; i++) {
        if (ch == app_pwm_info[i].ch) {
            LOGD(TAG, "ch %d, i %d", ch, i);
            return -1;
        }
        if (NULL == app_pwm_info[i].handle) {
            idx = i;
        }
    }
    return idx;
}

static pwm_handle_t ch_to_handle(uint8_t ch)
{
    for (uint8_t i = 0; i < PWM_NUM; i++) {
        if (ch == app_pwm_info[i].ch) {
            return app_pwm_info[i].handle;
        }
    }

    return NULL;
}

int8_t app_pwm_init(pin_name_e pwm_pin, pin_func_e pin_func, uint8_t ch)
{
    if (drv_pinmux_config(pwm_pin, pin_func) != 0) {
        LOGE(TAG, "pinmux config failed! pin %d", pwm_pin);
        return -1;
    }

    int8_t pwm_id = get_free_info_id(ch);

    if (-2 == pwm_id) {
        LOGE(TAG, "No free pwm handle! Only %d handles suppoerted", PWM_NUM);
        return -1;
    } else if (-1 == pwm_id) {
        /* channel has already been initialized, return directly */
        return 0;
    }

    app_pwm_info[pwm_id].handle = csi_pwm_initialize(ch);
    app_pwm_info[pwm_id].ch     = ch;

    return 0;
}

int8_t app_pwm_deinit(void)
{
    csi_pwm_uninitialize((pwm_handle_t)0); /* real handle is not needed */
    for (uint8_t i = 0; i < PWM_NUM; i++) {
        app_pwm_info[i].ch     = -1;
        app_pwm_info[i].handle = NULL;
    }

    return 0;
}

int8_t app_pwm_start(uint8_t ch)
{
    pwm_handle_t handle = ch_to_handle(ch);
    if (NULL == handle) {
        LOGE(TAG, "Cannot find a matched handle! ch %d", ch);
        return -1;
    }

    csi_pwm_start(handle, ch);

    return 0;
}

int8_t app_pwm_stop(uint8_t ch)
{
    pwm_handle_t handle = ch_to_handle(ch);
    if (NULL == handle) {
        LOGE(TAG, "Cannot find a matched handle! ch %d", ch);
        return -1;
    }

    csi_pwm_stop(handle, ch);

    return 0;
}

int8_t app_pwm_config(uint8_t ch, uint32_t period, uint32_t duty)
{
    if(duty > 100) {
        LOGE(TAG, "duty [%d] should be in range 0 ~ 100", duty);
        return -1;
    }
    uint32_t pulse_width_us = period * duty / 100;

    /* find a valid channel */
    pwm_handle_t handle = ch_to_handle(ch);
    if (NULL == handle) {
        LOGE(TAG, "Cannot find a matched handle! ch %d", ch);
        return -1;
    }

    return csi_pwm_config(handle, ch, period, pulse_width_us);
}