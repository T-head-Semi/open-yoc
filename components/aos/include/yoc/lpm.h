/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef YOC_LPM_H
#define YOC_LPM_H

#include <time.h>
#include <aos/list.h>
#include <aos/aos.h>
#include <drv/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KV_LPM_POLICY   "lpm_policy"
#define KV_LPM_LS_THRES "lpm_ls"
#define KV_LPM_DS_THRES "lpm_ds"

typedef enum {
    LPM_EVENT_WAKUP,
    LPM_EVENT_SUSPEND,
} lpm_event_e;

typedef enum {
    LPM_POLICY_NO_POWER_SAVE,
    LPM_POLICY_LOW_POWER,
    LPM_POLICY_DEEP_SLEEP,
} pm_policy_t;

typedef struct {
    slist_t next;

    void *dev;
    void *param;
} pm_dev_node_t;

#define LPM_DEV_MASK_GENERAL_ADC   (1 << 0)
#define LPM_DEV_MASK_GENERAL_GPIO1 (1 << 1)
#define LPM_DEV_MASK_GENERAL_GPIO2 (1 << 2)

typedef void (*lpm_event_cb_t)(pm_policy_t policy, lpm_event_e event);

typedef struct {
    pm_policy_t policy;     //0: no low power, 1: low power, 2: support deep sleep
    uint32_t    agree_halt; //execution context will be lost
    uint32_t    alarm_ms;
    uint32_t    ls_ms; //low power sleep threshold
    uint32_t    ds_ms; //deep sleep threshold
    uint32_t    suspend_tick;
    uint32_t    lpm_dev_mask; /* 标记位置一对应的模块不进入低功耗 */
    aos_mutex_t mutex;
    aos_task_t  task_handle;
    aos_sem_t   sem;
    void (*lpm_handle)(void);
    lpm_event_cb_t wake_cb;
} pm_ctx_t;

void pm_init(lpm_event_cb_t pm_cb);
void pm_config_mask(uint32_t lpm_dev_mask);
void pm_config_policy(pm_policy_t policy);
void pm_agree_halt(uint32_t ms);
pm_policy_t pm_get_policy(void);

#ifdef CONFIG_CSI_V2
void setup_wake_irq(csi_gpio_t *handle, csi_gpio_mode_t mode);
#else
void setup_wake_irq(gpio_pin_handle_t handle, gpio_irq_mode_e mode);
#endif

#ifdef __cplusplus
}
#endif

#endif
