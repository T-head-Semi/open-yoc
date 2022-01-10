#ifndef SUNXI_HAL_MAILBOX_H
#define SUNXI_HAL_MAILBOX_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#ifndef CONFIG_KERNEL_NONE
#include <aos/kernel.h>

typedef struct {
    aos_queue_t queue;
} *hal_mailbox_t;
typedef aos_workqueue_t hal_workqueue;
typedef aos_work_t hal_work;
#else
typedef void * hal_mailbox_t;
typedef long hal_workqueue;
typedef long hal_work;
#endif

void hal_work_init(hal_work *work, void (*work_func)(void *work_data), void *work_data);
hal_workqueue *hal_workqueue_create(const char *name, unsigned short stack_size, unsigned char priority);
int hal_workqueue_dowork(hal_workqueue *queue, hal_work *work);

hal_mailbox_t hal_mailbox_create(const char *name, unsigned int size);
int hal_mailbox_delete(hal_mailbox_t mailbox);
int hal_mailbox_send(hal_mailbox_t mailbox, unsigned int value);
int hal_mailbox_send_wait(hal_mailbox_t mailbox, unsigned int value, int timeout);
int hal_mailbox_recv(hal_mailbox_t mailbox, unsigned int *value, int timeout);
int hal_is_mailbox_empty(hal_mailbox_t mailbox);


#ifdef __cplusplus
}
#endif
#endif
