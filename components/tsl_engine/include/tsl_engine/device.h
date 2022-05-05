/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __dev_H__
#define __dev_H__

#include <tsl_engine/device_typedef.h>

__BEGIN_DECLS__

/**
 * @brief  regist device class
 * @param  [in] cls
 * @return 0/-1
 */
int dev_cls_register(const device_t *cls);

/**
 * @brief  alloc a device by dev_type
 * @param  [in] type
 * @param  [in] name
 * @return NULL on error
 */
device_t *dev_new(int type, const char *name);

/**
 * @brief  config the device
 * @param  [in] dev
 * @param  [in] conf
 * @return 0/-1
 */
int dev_config(device_t *dev, const dev_conf_t *conf);

/**
 * @brief  add a key-value pair to the dev kv-map
 * @param  [in] dev
 * @param  [in] key
 * @param  [in] data : value of the key is a pointer, may be need free by the caller after free
 * @return 0/-1
 */
int dev_kv_set(device_t *dev, const char *key, void *data);

/**
 * @brief  get value from dev kv-map by the key
 * @param  [in] dev
 * @param  [in] key
 * @return
 */
void *dev_kv_get(device_t *dev, const char *key);

/**
 * @brief  device ioctl interface
 * @param  [in] dev
 * @param  [in] cmd
 * @param  [in] arg
 * @param  [in] arg_size
 * @return 0/-1
 */
int dev_ioctl(device_t *dev, int cmd, void *arg, size_t *arg_size);

/**
 * @brief  open the device
 * @param  [in] dev
 * @return 0/-1
 */
int dev_open(device_t *dev);

/**
 * @brief  send data to the real dev
 * @param  [in] dev
 * @param  [in] data
 * @param  [in] size
 * @return -1 on error
 */
int dev_send_to_device(device_t *dev, const uint8_t *data, size_t size);

/**
 * @brief  send data to the tsl-cloud
 * @param  [in] dev
 * @param  [in] data
 * @param  [in] size
 * @return -1 on error
 */
int dev_send_to_cloud(device_t *dev, const uint8_t *data, size_t size);

/**
 * @brief  close the device
 * @param  [in] dev
 * @return
 */
void dev_close(device_t *dev);

/**
 * @brief  free the device
 * @param  [in] dev
 * @return
 */
void dev_free(device_t *dev);

__END_DECLS__

#endif /* __dev_H__ */

