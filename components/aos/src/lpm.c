/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <aos/kernel.h>
#include <yoc/lpm.h>

#include <drv/common.h>
#define TAG "LPM"

extern int sys_soc_init(pm_ctx_t *pm_ctx);
extern int sys_soc_suspend(pm_ctx_t *pm_ctx);
extern int sys_soc_resume(pm_ctx_t *pm_ctx, int pm_state);

static pm_ctx_t g_pm_ctx;

static void lpm_handle(void)
{
    int pm_state;
    int ticks;

    if (g_pm_ctx.policy == LPM_POLICY_NO_POWER_SAVE) {
        return;
    }

    if (g_pm_ctx.wake_cb) {
        g_pm_ctx.wake_cb(g_pm_ctx.policy, LPM_EVENT_SUSPEND);
    }

    aos_kernel_sched_suspend();

    g_pm_ctx.suspend_tick = aos_kernel_suspend();

    //how many pm_state is decided by SOC implement
    pm_state = sys_soc_suspend(&g_pm_ctx);
    ticks    = sys_soc_resume(&g_pm_ctx, pm_state);

    if (ticks) {
        aos_kernel_resume(ticks);
    }

    g_pm_ctx.agree_halt = 0;

    aos_kernel_sched_resume();

    if (g_pm_ctx.wake_cb) {
        g_pm_ctx.wake_cb(g_pm_ctx.policy, LPM_EVENT_WAKUP);
    }

    return;
}

void lpm_idle_hook(void)
{
    if ((g_pm_ctx.agree_halt == 1) && (g_pm_ctx.policy != LPM_POLICY_NO_POWER_SAVE)) {
        g_pm_ctx.agree_halt = 2;
        aos_sem_signal(&g_pm_ctx.sem);
    }
}

void lpm_entry(void *arg)
{
    while (1) {
        aos_sem_wait(&g_pm_ctx.sem, AOS_WAIT_FOREVER);

        if (g_pm_ctx.lpm_handle) {
            g_pm_ctx.lpm_handle();
        }
    }
}

/******************************************
// YoC PM APIs
******************************************/
void pm_init(lpm_event_cb_t pm_cb)
{
    int ret;
    memset(&g_pm_ctx, 0, sizeof(pm_ctx_t));
    ret = aos_mutex_new(&g_pm_ctx.mutex);
    aos_check(!ret, EIO);

    g_pm_ctx.lpm_handle = lpm_handle;
    g_pm_ctx.wake_cb    = pm_cb;

    sys_soc_init(&g_pm_ctx);

    ret = aos_sem_new(&g_pm_ctx.sem, 0);
    aos_check(!ret, EIO);

    ret = aos_task_new_ext(&g_pm_ctx.task_handle, "lpm", lpm_entry, NULL, 4096, 60);
    aos_check(!ret, EIO);
}

void pm_config_mask(uint32_t lpm_dev_mask)
{
    if (g_pm_ctx.lpm_handle == NULL) {
        LOGE(TAG, "pm not initialized");
        return;
    }
    g_pm_ctx.lpm_dev_mask |= lpm_dev_mask;
}

void pm_config_policy(pm_policy_t policy)
{
    if (g_pm_ctx.lpm_handle == NULL) {
        LOGE(TAG, "pm not initialized");
        return;
    }
    aos_mutex_lock(&g_pm_ctx.mutex, AOS_WAIT_FOREVER);
    g_pm_ctx.policy = policy;
    aos_mutex_unlock(&g_pm_ctx.mutex);
}

pm_policy_t pm_get_policy(void)
{
    if (g_pm_ctx.lpm_handle == NULL) {
        LOGE(TAG, "pm not initialized");
        return LPM_POLICY_NO_POWER_SAVE;
    }

    pm_policy_t policy;
    aos_mutex_lock(&g_pm_ctx.mutex, AOS_WAIT_FOREVER);
    policy = g_pm_ctx.policy;
    aos_mutex_unlock(&g_pm_ctx.mutex);
    return policy;
}

//standby mode is managed by application, poweroff of a device is also managed by application.
//application should close device first, then agree deep sleep.
void pm_agree_halt(uint32_t ms)
{
    if (g_pm_ctx.lpm_handle == NULL) {
        LOGE(TAG, "pm not initialized");
        return;
    }

    aos_mutex_lock(&g_pm_ctx.mutex, AOS_WAIT_FOREVER);
    if (ms < 100)
        ms = 100;

    g_pm_ctx.alarm_ms   = ms;
    g_pm_ctx.agree_halt = 1;
    aos_mutex_unlock(&g_pm_ctx.mutex);
}
