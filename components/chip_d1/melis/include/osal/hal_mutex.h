#ifndef SUNXI_HAL_MUTEX_H
#define SUNXI_HAL_MUTEX_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
#ifndef CONFIG_KERNEL_NONE
#include <aos/kernel.h>
typedef struct {
    aos_mutex_t mtx;
} *hal_mutex_t;
#else
typedef void * hal_mutex_t;
#endif

hal_mutex_t hal_mutex_create(void);
int hal_mutex_delete(hal_mutex_t mutex);
int hal_mutex_lock(hal_mutex_t  mutex);
int hal_mutex_unlock(hal_mutex_t  mutex);
int hal_mutex_trylock(hal_mutex_t  mutex);
int hal_mutex_timedwait(hal_mutex_t mutex, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
