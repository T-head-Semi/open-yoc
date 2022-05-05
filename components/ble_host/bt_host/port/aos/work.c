/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <common/log.h>
#include "errno.h"

#if !(defined(CONFIG_BT_WORK_INDEPENDENCE) && CONFIG_BT_WORK_INDEPENDENCE)
struct k_work_q g_work_queue;

extern void event_callback(uint8_t event_type);
static void k_work_submit_to_queue(struct k_work_q *work_q, struct k_work *work)
{
    sys_snode_t *node = NULL;
    struct k_work *delayed_work = NULL;
    struct k_work *prev_delayed_work = NULL;
    //uint32_t now = k_uptime_get_32();

    if (!atomic_test_and_set_bit(work->flags, K_WORK_STATE_PENDING)) {
        SYS_SLIST_FOR_EACH_NODE(&g_work_queue.queue.queue_list, node) {
            delayed_work = (struct k_work *)node;
            if ((work->timeout + work->start_ms) < (delayed_work->start_ms + delayed_work->timeout)) {
                break;
            }
            prev_delayed_work = delayed_work;
        }

        sys_slist_insert(&g_work_queue.queue.queue_list,
                         (sys_snode_t *)prev_delayed_work, (sys_snode_t *)work);
        delayed_work = k_queue_first_entry(&g_work_queue.queue);

        if (delayed_work &&
            (k_uptime_get_32() <= delayed_work->start_ms + delayed_work->timeout)) {
            event_callback(K_POLL_TYPE_EARLIER_WORK);
        }
    }
}

static void k_work_rm_from_queue(struct k_work_q *work_q, struct k_work *work)
{
    k_queue_remove(&work_q->queue, work);
}

int k_work_q_start(void)
{
    k_queue_init(&g_work_queue.queue);
    return 0;
}

int k_work_init(struct k_work *work, k_work_handler_t handler)
{
    atomic_clear_bit(work->flags, K_WORK_STATE_PENDING);
    work->handler = handler;
    work->start_ms = 0;
    work->timeout = 0;
    return 0;
}

void k_work_submit(struct k_work *work)
{
    k_delayed_work_submit((struct k_delayed_work *)work, 0);
}

void k_delayed_work_init(struct k_delayed_work *work, k_work_handler_t handler)
{
    k_work_init(&work->work, handler);
}

int k_delayed_work_submit(struct k_delayed_work *work, uint32_t delay)
{
    int err = 0;
    int key = irq_lock();

    if (atomic_test_bit(work->work.flags, K_WORK_STATE_PENDING)) {
        k_delayed_work_cancel(work);
    }

    work->work.start_ms = k_uptime_get_32();
    work->work.timeout = delay;
    k_work_submit_to_queue(&g_work_queue, (struct k_work *)work);

//done:
    irq_unlock(key);
    return err;
}

int k_delayed_work_cancel(struct k_delayed_work *work)
{
    int key = irq_lock();
	work->work.timeout = 0;
    atomic_clear_bit(work->work.flags, K_WORK_STATE_PENDING);
    k_work_rm_from_queue(&g_work_queue, (struct k_work *)work);
    irq_unlock(key);
    return 0;
}

s32_t k_delayed_work_remaining_get(struct k_delayed_work *work)
{
    int32_t remain;

    if (work == NULL) {
        return 0;
    }

    remain = work->work.timeout - (k_uptime_get_32() - work->work.start_ms);
    if (remain < 0) {
        remain = 0;
    }
    return remain;
}

#else

#include <aos/kernel.h>

#ifndef CONFIG_BLUETOOTH_WORK_QUEUE_STACK_SIZE
#ifdef CONFIG_BT_MESH
#define CONFIG_BLUETOOTH_WORK_QUEUE_STACK_SIZE (1500)
#else
#define CONFIG_BLUETOOTH_WORK_QUEUE_STACK_SIZE (1024)
#endif
#endif

#ifndef CONFIG_BLUETOOTH_WORK_QUEUE_PRIO
#define CONFIG_BLUETOOTH_WORK_QUEUE_PRIO (AOS_DEFAULT_APP_PRI - 1)
#endif

static struct k_thread workq_thread_data;
static BT_STACK_NOINIT(work_q_stack, CONFIG_BLUETOOTH_WORK_QUEUE_STACK_SIZE);
static struct k_work_q g_work_queue_main;

static void k_work_submit_to_queue(struct k_work_q *work_q,
                                   struct k_work *work)
{
    if (!atomic_test_and_set_bit(work->flags, K_WORK_STATE_PENDING)) {
        k_fifo_put(&work_q->fifo, work);
#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
        k_sem_give(&g_work_queue_main.sem);
#endif
    }
}

static void work_queue_thread(void *arg)
{
    struct k_work *work;
    UNUSED(arg);

    while (1) {
#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
        k_sem_take(&g_work_queue_main.sem, K_FOREVER);
        work = k_fifo_get(&g_work_queue_main.fifo, K_FOREVER);
        if (!work)
        {
            continue;
        }
#else
		work = k_fifo_get(&g_work_queue_main.fifo, K_FOREVER);
#endif
        if (atomic_test_and_clear_bit(work->flags, K_WORK_STATE_PENDING)) {
            if (work->handler)
            {
                work->handler(work);
            }
            else
            {
                printf("work handler is NULL\n");
            }
        }

        k_yield();
    }
}

int k_work_q_start(void)
{
#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
    k_sem_init(&g_work_queue_main.sem, 0, 0);
#endif
    k_fifo_init(&g_work_queue_main.fifo);
    return k_thread_spawn(&workq_thread_data ,"work queue", (uint32_t *)work_q_stack,
                          K_THREAD_STACK_SIZEOF(work_q_stack),
                          work_queue_thread, NULL, CONFIG_BLUETOOTH_WORK_QUEUE_PRIO);
}

int k_work_init(struct k_work *work, k_work_handler_t handler)
{
    ASSERT(work, "work is NULL");

    atomic_clear_bit(work->flags, K_WORK_STATE_PENDING);
    work->handler = handler;
    return 0;
}

void k_work_submit(struct k_work *work)
{
    k_work_submit_to_queue(&g_work_queue_main, work);
}

static void work_timeout(void *timer, void *args)
{
    struct k_delayed_work *w = (struct k_delayed_work *)args;

    /* submit work to workqueue */
    krhino_timer_stop(&w->timer);
    if (w->work_q)
    {
        k_work_submit_to_queue(w->work_q, &w->work);
    }
    /* detach from workqueue, for cancel to return appropriate status */
    w->work_q = NULL;
}

void k_delayed_work_init(struct k_delayed_work *work, k_work_handler_t handler)
{
    ASSERT(work, "delay work is NULL");
    k_work_init(&work->work, handler);
    krhino_timer_create(&work->timer, "AOS", work_timeout,
                            krhino_ms_to_ticks(1000), 0, work, 0);
    work->work_q = NULL;
}

static int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
        struct k_delayed_work *work,
        uint32_t delay)
{
    int key = irq_lock();
    int err;

    /* Work cannot be active in multiple queues */
    if (work->work_q && work->work_q != work_q) {
        err = -EADDRINUSE;
        goto done;
    }

    /* Cancel if work has been submitted */
    if (work->work_q == work_q) {
        err = k_delayed_work_cancel(work);

        if (err < 0) {
            goto done;
        }
    }

    /* Attach workqueue so the timeout callback can submit it */
    work->work_q = work_q;

    if (!delay) {
        /* Submit work if no ticks is 0 */
        k_work_submit_to_queue(work_q, &work->work);
        work->work_q = NULL;
    } else {
        /* Add timeout */
        work->start_ms = k_uptime_get_32();
        work->timeout = delay;
        krhino_timer_change(&work->timer, krhino_ms_to_ticks(delay), 0);
        krhino_timer_start(&work->timer);
    }

    err = 0;

done:
    irq_unlock(key);
    return err;
}

int k_delayed_work_submit(struct k_delayed_work *work, uint32_t delay)
{
    return k_delayed_work_submit_to_queue(&g_work_queue_main, work, delay);
}

int k_delayed_work_cancel(struct k_delayed_work *work)
{
    int err = 0;
    int key = irq_lock();

    if (atomic_test_bit(work->work.flags, K_WORK_STATE_PENDING)) {
        err = -EINPROGRESS;
        goto exit;
    }

    if (!work->work_q) {
        err = -EINVAL;
        goto exit;
    }

    work->work_q = NULL;
    krhino_timer_stop(&work->timer);

exit:
    irq_unlock(key);
    return err;
}

s32_t k_delayed_work_remaining_get(struct k_delayed_work *work)
{
    int32_t remain;

    if (work == NULL) {
        return 0;
    }

    remain = work->timeout - (k_uptime_get() - work->start_ms);

    if (remain < 0) {
        remain = 0;
    }

    return remain;
}

#endif