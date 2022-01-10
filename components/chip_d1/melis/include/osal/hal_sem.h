#ifndef SUNXI_HAL_SEM_H
#define SUNXI_HAL_SEM_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stddef.h>
#include <stdint.h>
#ifndef CONFIG_KERNEL_NONE
#include <aos/kernel.h>

typedef struct {
    aos_sem_t sem;
} *hal_sem_t;
#else
typedef void * hal_sem_t;
#endif

hal_sem_t hal_sem_create(unsigned int cnt);
int hal_sem_delete(hal_sem_t sem);
int hal_sem_getvalue(hal_sem_t sem, int *val);
int hal_sem_post(hal_sem_t sem);
int hal_sem_timedwait(hal_sem_t sem, int timeout_ms);
int hal_sem_trywait(hal_sem_t sem);
int hal_sem_wait(hal_sem_t sem);
int hal_sem_clear(hal_sem_t sem);

#ifdef __cplusplus
}
#endif
#endif
