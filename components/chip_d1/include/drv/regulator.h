/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

#ifndef __REGULATOR_H__
#define __REGULATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum REGULATOR_TYPE_ENUM REGULATOR_TYPE_ENUM_e;
typedef enum REGULATOR_ID_ENUM REGULATOR_ID_ENUM_e;
typedef struct regulator_dev regulator_dev_t;

int csi_regulator_get_voltage(regulator_dev_t *rdev, int *vol_uV);

int csi_regulator_set_voltage(regulator_dev_t *rdev, int target_uV);

int csi_regulator_enable(regulator_dev_t *rdev);

int csi_regulator_disable(regulator_dev_t *rdev);

int csi_regulator_get(REGULATOR_TYPE_ENUM_e regulator_type, REGULATOR_ID_ENUM_e regulator_id, regulator_dev_t *rdev);

#ifdef __cplusplus
}
#endif

#endif