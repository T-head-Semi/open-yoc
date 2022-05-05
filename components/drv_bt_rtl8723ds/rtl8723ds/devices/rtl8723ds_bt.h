/*
 * Copyright (C) 2022 Alibaba Group Holding Limited
 */
#ifndef DEVICE_STL8723_BT_H
#define DEVICE_STL8723_BT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rtl8723ds_bt_config_t {
    int uart_id;
    int bt_dis_pin;
} rtl8723ds_bt_config;

/**
 * @brief  register bluetooth driver of rtl8723 
 * @param  [in] config
 * @return  
 */
extern void bt_rtl8723ds_register(rtl8723ds_bt_config* config);


#ifdef __cplusplus
}
#endif

#endif
