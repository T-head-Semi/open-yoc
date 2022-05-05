/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __JOBJ_PUSH_DEVICE_H__
#define __JOBJ_PUSH_DEVICE_H__

#include <tsl_engine/device.h>
#include "duktape.h"

__BEGIN_DECLS__

/**
 * @brief  push javascript obj for device
 * @param  [in] ctx
 * @param  [in] dev : handle of device
 * @return 0/-1
 */
int jobj_push_device(duk_context *ctx, device_t *dev);
int jobj_push_dev_data(duk_context *ctx, dev_data_t *dev_data);


__END_DECLS__

#endif /* __JOBJ_PUSH_DEVICE_H__ */

