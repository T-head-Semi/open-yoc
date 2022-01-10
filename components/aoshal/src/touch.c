/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifdef CONFIG_DRIVERS_TOUCH
#include <stdio.h>
#include "aos/hal/touch.h"
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


static csi_touchscreen_t g_touchscreen;
static csi_iic_t g_touch_iic;

static void touch_run(void* arg)
{
    csi_error_t ret;
    touch_cb cb_func = arg;
    struct touch_message msg = {0};

    ret = csi_iic_init(&g_touch_iic, TOUCH_IIC_PORT);
    if (ret != 0) {
        printk("csi iic touchscreen init failed.\r\n");
        return;
    }
    g_touchscreen.iic_port = TOUCH_IIC_PORT;
    g_touchscreen.iic_addr = TOUCH_IIC_ADDR;
    ret = csi_touchscreen_init(&g_touchscreen);
    if (ret != 0) {
        printk("csi touchscreen init failed.\r\n");
        return;
    }
    printk("touchscreen init ok ^_^\r\n");

    while (1) {
        aos_sem_wait(&g_touchscreen.isr_sem->sem, AOS_WAIT_FOREVER);
        if (csi_touchscreen_read_point(&g_touchscreen, &msg) == 0) {
            if (cb_func)
                cb_func(&msg);
        }
    }
}
static int g_touch_init = 0;
int hal_touch_init(touch_cb callback)
{
    aos_task_t task;
    aos_status_t status;

    if (g_touch_init == 1) {
        return 0;
    }
    g_touch_init = 1;

    status = aos_task_new_ext(&task, "touch", touch_run, callback, 2048, 16);
    if (status != 0) {
        return -1;
    }
    return 0;
}
#endif
