/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include "app_main.h"
#include "app_pwm.h"
#include <drv/pwm.h>
#include "devices/device.h"
#include <board_config.h>

#define TAG "app"

#define DEVICE_LED_NAME "ledrgb"
#define PWM_PERIOD 100

typedef struct pwm_led {
    uint8_t status;     /* 0 - stop, 1 - start */
    int32_t brightness; /* 10 - 100 */
    uint8_t pin;
    uint8_t channel;
    int pin_func;
} pwm_led_t;

static aos_dev_t *led_dev      = NULL;
static int        g_app_led_en = 1;
#ifdef APP_PWM_EN
// PWM0
static pwm_led_t  g_pwm_led_ctx = {0, 60, 0, 0, 0};
#else
static pwm_led_t  g_pwm_led_ctx = {0};
#endif

int app_pwm_led_set_stop();

int8_t app_pwm_led_init()
{
    int8_t ret = 0;

    int brightness;
    if (aos_kv_getint("led_brightness", &brightness) == 0) {
        g_pwm_led_ctx.brightness = brightness;
    }

    ret = app_pwm_init(g_pwm_led_ctx.pin, g_pwm_led_ctx.pin_func, g_pwm_led_ctx.channel);
    if (ret != 0) {
        LOGE(TAG, "PWM Init failed, pin %d channel %d", g_pwm_led_ctx.pin,
             g_pwm_led_ctx.channel);
    }

    app_pwm_led_set_stop();

    aos_msleep(20);
    ret = app_pwm_start(g_pwm_led_ctx.channel);
    if (ret != 0) {
        LOGE(TAG, "PWM start failed, channel %d", g_pwm_led_ctx.channel);
    }

    g_pwm_led_ctx.status = 0;

    return ret;
}

// int8_t app_pwm_led_deinit(void)
// {
//     return app_pwm_deinit();
// }

int app_pwm_led_set_config(int brightness)
{
    int ret = app_pwm_config(g_pwm_led_ctx.channel, PWM_PERIOD,  brightness);
    if (ret != 0) {
        LOGE(TAG, "PWM config failed, channel %d", g_pwm_led_ctx.channel);
    }

    return ret;
}

int app_pwm_led_set_stop()
{
    LOGD(TAG, "PWM led off.");
    return app_pwm_led_set_config(0);
}

int app_pwm_led_set_normal()
{
    LOGD(TAG, "PWM led set to brightness %d", g_pwm_led_ctx.brightness);
    int brightness = g_pwm_led_ctx.brightness;
    if (brightness == PWM_LED_LIGHT_MAX) {
        brightness = 99; // Max 99. 
    }

    return app_pwm_led_set_config(brightness);
}

int8_t app_pwm_led_control(const uint8_t flag)
{
    /* flag: 0 - stop, 1 -start */
    int8_t ret = 0;
    if(0 == flag){
        if (0 == g_pwm_led_ctx.status) {
            /* already stopped */
            return ret;
        }

        ret = app_pwm_led_set_stop();
        if(0 == ret) {
            g_pwm_led_ctx.status = 0;
            //user_post_property();
        }
    } else if(1 == flag){
        LOGD(TAG, "PWM led on.");
        ret = app_pwm_led_set_normal();
        if(0 == ret) {
            g_pwm_led_ctx.status = 1;
            //user_post_property();
        }
    } else{
        LOGE(TAG, "flag  %d is incorrect!", flag);
        ret = -1;
    }

    return ret;
}

int8_t app_pwm_led_set_brightness(int32_t brightness)
{
    if (brightness < PWM_LED_LIGHT_MIN) {
        brightness = PWM_LED_LIGHT_MIN;
    } else if (brightness > PWM_LED_LIGHT_MAX) {
        brightness = PWM_LED_LIGHT_MAX;
    }

    if (g_pwm_led_ctx.brightness != brightness) {
        if (aos_kv_setint("led_brightness", brightness) != 0) {
            LOGE(TAG, "set led_brightness failed");
        }
    }

    g_pwm_led_ctx.brightness = brightness;

    return app_pwm_led_control(1);
}

int8_t app_pwm_led_get_brightness()
{
    return g_pwm_led_ctx.brightness;
}

int8_t app_pwm_led_get_status()
{
    return g_pwm_led_ctx.status;
}