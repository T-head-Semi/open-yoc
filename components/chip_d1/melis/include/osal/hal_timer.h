#ifndef SUNXI_HAL_TIMER_H
#define SUNXI_HAL_TIMER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#ifndef CONFIG_KERNEL_NONE
#include <aos/kernel.h>
#else
typedef void * aos_timer_t;
#endif
typedef struct {
    aos_timer_t timer;
} *osal_timer_t;
typedef void (*timeout_func)(void *timer, void *parameter); // FIXME: attention the timer handle.

#define OSAL_TIMER_FLAG_ONE_SHOT     AOS_TIMER_NONE
#define OSAL_TIMER_FLAG_PERIODIC     AOS_TIMER_REPEAT

#define OSAL_TIMER_CTRL_SET_TIME     0x01

osal_timer_t osal_timer_create(const char *name,
                               timeout_func timer_callback,
                               void *parameter,
                               unsigned int timeout_ms,
                               unsigned char flag);

int osal_timer_delete(osal_timer_t timer);
int osal_timer_start(osal_timer_t timer);
int osal_timer_stop(osal_timer_t timer);
int osal_timer_control(osal_timer_t timer, int cmd, void *arg);


int hal_sleep(unsigned int secs);
int hal_usleep(unsigned int usecs);
int hal_msleep(unsigned int msecs);
void hal_udelay(unsigned int us);


#ifdef __cplusplus
}
#endif
#endif
