/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef DEVICE_XR829_H
#define DEVICE_XR829_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int wl_en;
    int power;
} xr829_config_t;

/**
 * @brief  register wifi driver of rtl8723 
 * @param  [in] config
 * @return  
 */
extern void wifi_xr829_register(xr829_config_t* config);


#ifdef __cplusplus
}
#endif

#endif
