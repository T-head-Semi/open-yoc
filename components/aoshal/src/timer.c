/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "soc.h"
#include "aos/hal/timer.h"
#include "drv/timer.h"

#define CONFIG_TIMER_NUM  4

typedef struct {
    csi_timer_t     csi_tim_handle;
    timer_dev_t     *hal_tim_handle;
} timer_handlers_t;

static timer_handlers_t  timer_handlers[CONFIG_TIMER_NUM];

typedef struct {
    csi_timer_t *csi_tim_handle;
    void *arg;
} timer_callback_t;

static void hal_timer_cb_func(csi_timer_t *timer, void *arg)
{
    timer_callback_t hal_tim_cb;

    hal_tim_cb.csi_tim_handle = timer;
    hal_tim_cb.arg = arg;

    timer_handlers[timer->dev.idx].hal_tim_handle->config.cb(&hal_tim_cb);
    if((unsigned long)arg==TIMER_RELOAD_MANU)
    {
        csi_timer_stop(timer);
    }
}

int32_t hal_timer_init(timer_dev_t *tim)
{
    if (!tim) {
        return -1;
    }

    int32_t ret = 0;

    ret = csi_timer_init(&timer_handlers[tim->port].csi_tim_handle, tim->port);

    if (ret < 0) {
        return -1;
    }

    timer_handlers[tim->port].hal_tim_handle = tim;

    csi_timer_attach_callback(&timer_handlers[tim->port].csi_tim_handle, hal_timer_cb_func,(void *)((unsigned long) tim->config.reload_mode));

    return 0;
}

/**
 * start a hardware timer
 *
 * @param[in]  tim  timer device
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t hal_timer_start(timer_dev_t *tim)
{
    if (tim == NULL) {
        return -1;
    }

    return csi_timer_start(&timer_handlers[tim->port].csi_tim_handle, tim->config.period);
}

/**
 * stop a hardware timer
 *
 * @param[in]  tim  timer device
 *
 * @return  none
 */
void hal_timer_stop(timer_dev_t *tim)
{
    if (tim == NULL) {
        return;
    }

    csi_timer_stop(&timer_handlers[tim->port].csi_tim_handle);
}

/**
 * change the config of a hardware timer
 *
 * @param[in]  tim   timer device
 * @param[in]  para  timer config
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t hal_timer_para_chg(timer_dev_t *tim, timer_config_t para)
{
    if (tim == NULL) {
        return -1;
    }

    return csi_timer_start(&timer_handlers[tim->port].csi_tim_handle, para.period);
}

/**
 * De-initialises an TIMER interface, Turns off an TIMER hardware interface
 *
 * @param[in]  tim  timer device
 *
 * @return  0 : on success, EIO : if an error occurred with any step
 */
int32_t hal_timer_finalize(timer_dev_t *tim)
{
    if (tim == NULL) {
        return -1;
    }

    csi_timer_uninit(&timer_handlers[tim->port].csi_tim_handle);

    return 0;
}

