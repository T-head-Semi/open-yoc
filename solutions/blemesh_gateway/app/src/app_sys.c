/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <yoc/lpm.h>

#include "app_main.h"

#include "app_sys.h"

#define TAG "APP_SYS"

static int boot_reason = -1;

void app_sys_reboot()
{
    app_sys_set_boot_reason(BOOT_REASON_SOFT_RESET);
    aos_reboot();
}

int app_sys_set_boot_reason(int reason)
{
    CHECK_PARAM(reason < BOOT_REASON_NONE && reason >= BOOT_REASON_SOFT_RESET, -1);
    int ret;
    int cur_reason;

    ret = aos_kv_getint("SYS_BOOT_REASON", &cur_reason);
    if (ret != 0 || cur_reason != reason) {
        boot_reason = reason;
        ret = aos_kv_setint("SYS_BOOT_REASON", reason);
        CHECK_RET_WITH_RET(ret == 0, -1);
    }

    return 0;
}

int app_sys_get_boot_reason()
{
    return boot_reason;
}

void app_sys_init()
{
    int reason;
    int ret;

    ret = aos_kv_getint("SYS_BOOT_REASON", &reason);
    if (ret != 0 || reason >= BOOT_REASON_NONE || reason < BOOT_REASON_SOFT_RESET) {
        reason = BOOT_REASON_POWER_ON;
    }

#if defined(SIMULATED_POWER_KEY) && SIMULATED_POWER_KEY
    /* simulted power key */
    if (reason == BOOT_REASON_POWER_KEY) {
        int val;
#ifdef POWER_KEY_ADC
        /* fix me: get rid of the first reading*/
        app_adc_value_read(POWER_KEY_PIN, &val);
        aos_msleep(10);
        app_adc_value_read(POWER_KEY_PIN, &val);
        if (val > POWER_KEY_VAL_MAX || val < POWER_KEY_VAL_MIN) {
            LOGD(TAG, "power key (adc) not pressed, continue sleeping");
            app_lpm_sleep(LPM_POLICY_DEEP_SLEEP, 1);
            aos_msleep(10000);
        }
#else
        app_gpio_read(POWER_KEY_PIN, &val);
        if (val == 1) {
            LOGD(TAG, "power key (gpio) not pressed, continue sleeping");
            app_lpm_sleep(LPM_POLICY_DEEP_SLEEP, 1);
            aos_msleep(10000);
        }
#endif
    }
#endif

    aos_kv_setint("SYS_BOOT_REASON", BOOT_REASON_POWER_ON);

    boot_reason = reason;
}