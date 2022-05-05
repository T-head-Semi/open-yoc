/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifdef CONFIG_DRIVERS_TOUCH
#include <stdio.h>
#include <aos/list.h>
#include <aos/debug.h>
#include <aos/hal/touch.h>
#include <aos/kernel.h>
#include <drv/touchscreen.h>
#include <drv/iic.h>
#ifdef AOS_COMP_DEBUG
#include <debug/dbg.h>
#else
#define printk printf
#endif

#ifndef TOUCH_SAMPLE_HZ
#define TOUCH_SAMPLE_HZ (50)
#endif
#ifndef TOUCH_IIC_PORT
#define TOUCH_IIC_PORT 2
#endif
#ifndef TOUCH_IIC_ADDR
#define TOUCH_IIC_ADDR 0x5D
#endif

#define LIST_LOCK()     aos_mutex_lock(&s_touch_ctx.mutex, AOS_WAIT_FOREVER)
#define LIST_UNLOCK()   aos_mutex_unlock(&s_touch_ctx.mutex)

typedef struct cb_node {
    touch_cb cb;
    void    *arg;
    slist_t  node;
} cb_node_t ;

static struct touch_context {
    csi_touchscreen_t   touchscreen;
    csi_iic_t           iic;
    aos_mutex_t         mutex;
    slist_t             head;
} s_touch_ctx;

static void touch_run(void* arg)
{
    cb_node_t *node;
    struct touch_message msg = {0};

    while (1) {
        aos_sem_wait((aos_sem_t *)(s_touch_ctx.touchscreen.isr_sem), AOS_WAIT_FOREVER);
        if (csi_touchscreen_read_point(&s_touch_ctx.touchscreen, &msg) == 0) {
            LIST_LOCK();
            slist_for_each_entry(&s_touch_ctx.head, node, cb_node_t, node) {
                if (node && node->cb) {
                    node->cb(&msg, node->arg);
                }
            }
            LIST_UNLOCK();
        }
    }
}

static cb_node_t *cb_find(touch_cb cb)
{
    cb_node_t *node;

    LIST_LOCK();
    slist_for_each_entry(&s_touch_ctx.head, node, cb_node_t, node) {
        if (node->cb == cb) {
            LIST_UNLOCK();
            return node;
        }
    }
    LIST_UNLOCK();

    return NULL;
}

int hal_touch_init(touch_cb callback, void *arg)
{
    static int g_touch_init = 0;
    aos_task_t task;
    aos_status_t status;
    csi_error_t ret;
    cb_node_t *node = NULL;

    if (g_touch_init == 1) {
        return 0;
    }

    aos_mutex_new(&s_touch_ctx.mutex);

    if (callback != NULL) {
        node = malloc(sizeof(cb_node_t));

        if (node == NULL) {
            return -1;
        }

        node->cb  = callback;
        node->arg = arg;

        LIST_LOCK();
        slist_add_tail(&node->node, &s_touch_ctx.head);
        LIST_UNLOCK();
    }

    ret = csi_iic_init(&s_touch_ctx.iic, TOUCH_IIC_PORT);
    if (ret != 0) {
        goto err;
    }
    s_touch_ctx.touchscreen.iic_port = TOUCH_IIC_PORT;
    s_touch_ctx.touchscreen.iic_addr = TOUCH_IIC_ADDR;
    ret = csi_touchscreen_init(&s_touch_ctx.touchscreen);
    if (ret != 0) {
        goto err;
    }

    status = aos_task_new_ext(&task, "touch", touch_run, NULL, 2048, 16);
    if (status != 0) {
        goto err;
    }

    g_touch_init = 1;

    return 0;

err:
    if (node) {
        free(node);
    }

    return -1;
}

int hal_touch_cb_register(touch_cb callback, void *arg)
{
    cb_node_t *node;

    aos_check_param(callback);

    if (cb_find(callback)) {
        return -1;
    }

    if ((node = malloc(sizeof(cb_node_t))) == NULL) {
        return -1;
    }

    node->cb  = callback;
    node->arg = arg;

    LIST_LOCK();
    slist_add_tail(&node->node, &s_touch_ctx.head);
    LIST_UNLOCK();

    return 0;
}

int hal_touch_cb_unregister(touch_cb callback)
{
    cb_node_t *node;

    aos_check_param(callback);

    if ((node = cb_find(callback)) != NULL) {
        slist_del(&node->node, &s_touch_ctx.head);
        free(node);

        return 0;
    }

    return -1;
}
#endif
