/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */
#ifndef __TOUCH_LL_H__
#define __TOUCH_LL_H__

#include <hal_sem.h>

typedef struct {
    uint16_t slave_addr;
    uint16_t iic_port;
    hal_sem_t isr_sem;
} touch_config_t;

typedef struct touch_cls touch_cls_t;
struct touch_cls {
    touch_config_t config;

    int (*init)(touch_cls_t *touch_cls, touch_config_t *config);
    int (*read_point)(touch_cls_t *touch_cls, touch_message_t msg);
    void (*deinit)(touch_cls_t *touch_cls);
};

void touch_gt9xx_register(touch_cls_t *touch_cls);

#endif