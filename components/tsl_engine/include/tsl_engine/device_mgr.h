/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */
#ifndef __DEVICE_MGR_H__
#define __DEVICE_MGR_H__

#include <tsl_engine/device_mgr_typedef.h>

__BEGIN_DECLS__

/**
 * @brief  get the device-manager
 * @return NULL on error
 */
dev_mgr_t *dev_mgr_get_instance();

/**
 * @brief  start the dev mgr
 * @return 0/-1
 */
int dev_mgr_start();

/**
 * @brief  stop the dev mgr
 * @return 0/-1
 */
int dev_mgr_stop();

/**
 * @brief  flush the dev mgr, will update dev status
 * @return 0/-1
 */
int dev_mgr_flush();

/**
 * @brief  add device to the mgr dev-list
 * @param  [in] dev
 * @return 0/-1
 */
int dev_mgr_add_device(device_t *dev);

/**
 * @brief  remove device from the mgr dev-list
 * @param  [in] dev
 * @return 0/-1
 */
int dev_mgr_del_device(device_t *dev);

/**
 * @brief  find device by dev type and name
 * @param  [in] type
 * @param  [in] name
 * @return NULL on error
 */
device_t *dev_mgr_find_device(int type, const char *name);

__END_DECLS__

#endif /* __DEVICE_MGR_H__ */

