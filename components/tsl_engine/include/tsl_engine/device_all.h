/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __DEVICE_ALL_H__
#define __DEVICE_ALL_H__

#include <aos/aos.h>
#include <tsl_engine/device.h>

__BEGIN_DECLS__

/**
 * @brief  regist device for mesh
 * @return 0/-1
 */
int device_register_mesh();

/**
 * @brief  regist all deviceer
 * @return 0/-1
 */
static inline void device_register_all()
{
#if defined(CONFIG_TSL_DEVICER_MESH)
    device_register_mesh();
#endif
}

__END_DECLS__

#endif /* __DEVICE_ALL_H__ */

