/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef __HAL_TOUCH_H__
#define __HAL_TOUCH_H__

#ifdef CONFIG_DRIVERS_TOUCH
#include <drv/touchscreen.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*touch_cb)(touch_message_t touch_msg, void* arg);

int hal_touch_init(touch_cb callback, void *arg);

int hal_touch_cb_register(touch_cb callback, void *arg);

int hal_touch_cb_unregister(touch_cb callback);

#endif

#ifdef __cplusplus
}
#endif

#endif