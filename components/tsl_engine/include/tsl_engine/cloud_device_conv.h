/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __CLOUD_DEVICE_CONV_H__
#define __CLOUD_DEVICE_CONV_H__

#include <tsl_engine/device.h>

__BEGIN_DECLS__

#define TLS_CONVERT_SCRIPT_PATH   "/lfs/tsl_convert.js"
#define CLOUD_TO_DEVICE_METHOD    "cloud_to_device"
#define DEVICE_CLOUD_TO_METHOD    "device_to_cloud"

/**
 * @brief  convert tsl from cloud to device
 * @param  [in] dev
 * @param  [in] in     input data
 * @param  [in] isize  input size of in data
 * @return 0/-1
 */
int cloud_to_device(device_t *dev, const char *in, size_t isize);

/**
 * @brief  convert tsl from device to cloud
 * @param  [in] dev
 * @param  [in] in     input data
 * @param  [in] isize  input size of in data
 * @return 0/-1
 */
int device_to_cloud(device_t *dev, const char *in, size_t isize);


__END_DECLS__

#endif /* __CLOUD_DEVICE_CONV_H__ */

