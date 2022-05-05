#include "timer_port.h"
#include "ulog/ulog.h"

#define TAG "OTA_TIMER"


#if defined(CONFIG_KERNEL_TIMER) &&  CONFIG_KERNEL_TIMER > 0
int ota_timer_init(ota_timer_t *timer, ota_timer_handler_t handle, void *args)
{
    int ret = 0;
    ret = aos_timer_new_ext(&timer->timer.aos_timer, handle, args, 1000, 0, 0);
    if(ret) {
        LOGE(TAG,"Aos timer create faild %d",ret);
    }
    return ret;
}

int ota_timer_start(ota_timer_t *timer, uint32_t timeout)
{
    int ret = 0;
    ret = aos_timer_change_once(&timer->timer.aos_timer, timeout);
    if(ret) {
        LOGE(TAG,"Aos timer change faild %d",ret);
        return ret;
    }

    return  aos_timer_start(&timer->timer.aos_timer);
}

int ota_timer_stop(ota_timer_t *timer)
{
    int ret = 0;
    ret = aos_timer_stop(&timer->timer.aos_timer);
    if(ret) {
        LOGE(TAG,"Aos timer stop faild %d",ret);
    }
    return ret;
}

void ota_timer_free(ota_timer_t *timer)
{
    aos_timer_stop(&timer->timer.aos_timer);
    aos_timer_free(&timer->timer.aos_timer);
}

#else

int ota_timer_init(ota_timer_t *timer, ota_timer_handler_t handle, void *args)
{
    k_timer_init(&timer->timer.k_timer, (k_timer_handler_t)handle, args);
    return 0;
}

int ota_timer_start(ota_timer_t *timer, uint32_t timeout)
{
    k_timer_start(&timer->timer.k_timer, timeout);
    return  0;
}

int ota_timer_stop(ota_timer_t *timer)
{
    k_timer_stop(&timer->timer.k_timer);
    return 0;
}

void ota_timer_free(ota_timer_t *timer)
{
    return;
}

#endif

