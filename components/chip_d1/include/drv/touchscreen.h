/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

#ifndef __TOUCHSCREEN_H__
#define __TOUCHSCREEN_H__

#include <drv/common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TOUCH_EVENT_UP      (0x01)
#define TOUCH_EVENT_DOWN    (0x02)
#define TOUCH_EVENT_MOVE    (0x03)
#define TOUCH_EVENT_NONE    (0x80)

struct touch_message
{
    uint16_t x;
    uint16_t y;
    uint8_t event;
};
typedef struct touch_message *touch_message_t;

typedef struct csi_touchscreen csi_touchscreen_t;
struct csi_touchscreen {
    csi_dev_t dev;
    uint16_t iic_port;
    uint16_t iic_addr;
    void *isr_sem;
    void *priv;
};

csi_error_t csi_touchscreen_init(csi_touchscreen_t *touch);

csi_error_t csi_touchscreen_read_point(csi_touchscreen_t *touch, touch_message_t msg);

csi_error_t csi_touchscreen_deinit(csi_touchscreen_t *touch);

#ifdef __cplusplus
}
#endif

#endif