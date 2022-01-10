/*
 * Copyright (C) 2020 FishSemi Inc. All rights reserved.
 */
/******************************************************************************
 * @file     usrsock.h
 * @brief    user sock device header
 * @version  V1.0
 * @date     7. Feb 2020
 ******************************************************************************/

#ifndef DEVICE_USRSOCK_PAI_H
#define DEVICE_USRSOCK_PAI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <devices/device.h>

#define usrsock_open(name) device_open(name)
#define usrsock_open_id(name, id) device_open_id(name, id)
#define usrsock_close(dev) device_close(dev)

void usrsock_initialize(void);
int usrsock_write(aos_dev_t *dev, const void *data, size_t size);
int usrsock_read(aos_dev_t *dev, void *data, size_t size);
void usrsock_set_event(aos_dev_t *dev,
                       void (*event)(aos_dev_t *dev, int event_id, void *priv),
                       void *priv);

#ifdef __cplusplus
}
#endif

#endif
