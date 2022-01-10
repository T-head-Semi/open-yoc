#ifndef SUNXI_HAL_THREAD_H
#define SUNXI_HAL_THREAD_H

#ifdef __cplusplus
extern "C"
{
#endif

#define HAL_THREAD_STACK_SIZE    (0x2000)
#define HAL_THREAD_PRIORITY      (    15)
#define HAL_THREAD_TIMESLICE     (    10)

void *kthread_create(void (*threadfn)(void *data), void *data, const char *namefmt, ...);
int kthread_stop(void *thread);
int kthread_start(void *thread);
int kthread_wakeup(void *thread);
int kthread_suspend(void *thread);

void *kthread_run(void (*threadfn)(void *data), void *data, const char *namefmt, ...);


//#define in_interrupt(...)   rt_interrupt_get_nest()

#ifdef __cplusplus
}
#endif
#endif
