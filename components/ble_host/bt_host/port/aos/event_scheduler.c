/*
 * Copyright (C) 2015-2019 Alibaba Group Holding Limited
 */

#include <stdint.h>
#include <ble_os.h>
#include "work.h"
#include "misc/util.h"

extern struct k_work_q g_work_queue;
extern void process_events(struct k_poll_event *ev, int count);
extern int bt_conn_prepare_events(struct k_poll_event events[]);
void scheduler_loop(struct k_poll_event *events)
{
#if !(defined(CONFIG_BT_WORK_INDEPENDENCE) && CONFIG_BT_WORK_INDEPENDENCE)
    struct k_work *work;
    uint32_t now;
#endif
    int ev_count;
    int delayed_ms = 0;

    while (1) {
        ev_count = 0;
        delayed_ms = K_FOREVER;
#if !(defined(CONFIG_BT_WORK_INDEPENDENCE) && CONFIG_BT_WORK_INDEPENDENCE)
        if (k_queue_is_empty(&g_work_queue.queue) == 0) {
            work = k_queue_first_entry(&g_work_queue.queue);
            now = k_uptime_get_32();

            delayed_ms = 0;
            if (now < (work->start_ms + work->timeout)) {
                delayed_ms = work->start_ms + work->timeout - now;
            }

            if (delayed_ms == 0) {
                delayed_ms = 1;
            }
        }
#endif
        events[0].state = K_POLL_STATE_NOT_READY;
#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
        events[1].state = K_POLL_STATE_NOT_READY;
        ev_count = 2;
#else
        ev_count = 1;
#endif
        if (IS_ENABLED(CONFIG_BT_CONN)) {
            ev_count += bt_conn_prepare_events(&events[ev_count]);
        }

        k_poll(events, ev_count, delayed_ms);

        process_events(events, ev_count);
#if !(defined(CONFIG_BT_WORK_INDEPENDENCE) && CONFIG_BT_WORK_INDEPENDENCE)
        if (k_queue_is_empty(&g_work_queue.queue) == 0) {
            work = k_queue_first_entry(&g_work_queue.queue);
            now = k_uptime_get_32();

            if (now >= (work->start_ms + work->timeout)) {
                k_queue_remove(&g_work_queue.queue, work);
                if (atomic_test_and_clear_bit(work->flags, K_WORK_STATE_PENDING) && work->handler) {
                    work->handler(work);
                }
            }
        }
#endif
        k_yield();
    }
}
