/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

#ifndef __MBUS_H__
#define __MBUS_H__

#include <sunxi_hal_mbus.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef hal_mbus_status_t mbus_status_t;

mbus_status_t csi_mbus_pmu_get_value(enum mbus_pmu type, unsigned int *value);
mbus_status_t csi_mbus_pmu_enable(void);

#ifdef __cplusplus
}
#endif

#endif