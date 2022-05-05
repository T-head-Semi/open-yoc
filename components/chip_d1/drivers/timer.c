
/*******************************************************
 * @file    dw_timer.c
 * @brief   source file for timer csi driver
 * @version V1.0
 * @date    23. Sep 2020
 * ******************************************************/

#include <drv/timer.h>
#include <drv/irq.h>
#include <sunxi_hal_timer.h>

static void hal_timer_irq_callback(void *param)
{
    csi_timer_t *timer = (csi_timer_t *)param;

    if (timer->callback) {
        timer->callback(timer, timer->arg);
    }
}

csi_error_t csi_timer_init(csi_timer_t *timer, uint32_t idx)
{
    CSI_PARAM_CHK(timer, CSI_ERROR);
    if (idx >= SUNXI_TMR_NUM) {
        return CSI_ERROR;
    }

    aw_hal_timer_init(idx);
    timer->dev.idx = idx;

    return CSI_OK;
}

void csi_timer_uninit(csi_timer_t *timer)
{
    CSI_PARAM_CHK_NORETVAL(timer);

    aw_hal_timer_uninit(timer->dev.idx);
}

csi_error_t csi_timer_start(csi_timer_t *timer, uint32_t timeout_us)
{
    CSI_PARAM_CHK(timer, CSI_ERROR);
    hal_timer_status_t status = aw_hal_timer_set_periodic(timer->dev.idx, timeout_us, hal_timer_irq_callback, timer);
    if (status != HAL_TIMER_STATUS_OK) {
        return CSI_ERROR;
    }
    return CSI_OK;
}

void csi_timer_stop(csi_timer_t *timer)
{
    CSI_PARAM_CHK_NORETVAL(timer);
    aw_hal_timer_stop(timer->dev.idx);
}

uint32_t csi_timer_get_remaining_value(csi_timer_t *timer)
{
    CSI_PARAM_CHK(timer, CSI_ERROR);
    return 0;
}

uint32_t csi_timer_get_load_value(csi_timer_t *timer)
{
    CSI_PARAM_CHK(timer, CSI_ERROR);
    return 0;
}

bool csi_timer_is_running(csi_timer_t *timer)
{
    CSI_PARAM_CHK(timer, CSI_ERROR);
    return 0;
}

csi_error_t csi_timer_attach_callback(csi_timer_t *timer, void *callback, void *arg)
{
    CSI_PARAM_CHK(timer, CSI_ERROR);

    timer->arg = arg;
    timer->callback = callback;

    return CSI_OK;
}

void csi_timer_detach_callback(csi_timer_t *timer)
{
    CSI_PARAM_CHK_NORETVAL(timer);

    timer->callback = NULL;
    timer->arg = NULL;
}

