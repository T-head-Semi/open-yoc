/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

#ifndef __THERMAL_H__
#define __THERMAL_H__

#ifdef __cplusplus
extern "C" {
#endif

int csi_ths_init(void);
int csi_ths_uninit(void);
int csi_ths_get_temp(unsigned int num, int *temp);

#ifdef __cplusplus
}
#endif

#endif