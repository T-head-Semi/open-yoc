/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef WORK_H
#define WORK_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include "ble_os.h"

#if !(defined(CONFIG_BT_WORK_INDEPENDENCE) && CONFIG_BT_WORK_INDEPENDENCE)

struct k_work_q {
    struct k_queue queue;
};

int k_work_q_start();

enum {
    K_WORK_STATE_PENDING,
};
struct k_work;
/* work define*/
typedef void (*k_work_handler_t)(struct k_work *work);

struct k_work {
    void *_reserved;
    k_work_handler_t handler;
    atomic_t flags[1];
    uint32_t start_ms;
    uint32_t timeout;
    int      index;
};

#define _K_WORK_INITIALIZER(work_handler) \
        { \
        ._reserved = NULL, \
        .handler = work_handler, \
        .flags = { 0 } \
        }

#define K_WORK_INITIALIZER DEPRECATED_MACRO _K_WORK_INITIALIZER


/*delay work define*/
struct k_delayed_work {
    struct k_work work;
};

#else

struct k_work_q {
#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
    struct k_sem sem;
#endif
    struct kfifo fifo;
};

int k_work_q_start();

enum {
    K_WORK_STATE_PENDING,
};

struct k_work;

/* work define*/
typedef void (*k_work_handler_t)(struct k_work *work);
struct k_work {
    sys_snode_t node;
    k_work_handler_t handler;
    atomic_t flags[1];
    int      index;
};

/*delay work define*/
struct k_delayed_work {
    struct k_work work;
    struct k_work_q *work_q;
    uint32_t start_ms;
    uint32_t timeout;
    _timer_t timer;
    uint64_t tick_end;
};

#endif

int k_work_init(struct k_work *work, k_work_handler_t handler);
void k_work_submit(struct k_work *work);

void k_delayed_work_init(struct k_delayed_work *work, k_work_handler_t handler);
int k_delayed_work_submit(struct k_delayed_work *work, uint32_t delay);
int k_delayed_work_cancel(struct k_delayed_work *work);
int k_delayed_work_remaining_get(struct k_delayed_work *work);

#ifdef __cplusplus
}
#endif

#endif /* WORK_H */

