/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef _APP_NET_INIT_
#define _APP_NET_INIT_
#include <stdint.h>

/*************
 * wifi & net 
 ************/

typedef enum {
    MODE_WIFI_TEST    = -2,
    MODE_WIFI_CLOSE   = -1,
    MODE_WIFI_NORMAL  = 0,
    MODE_WIFI_PAIRING = 1
} wifi_mode_e;

typedef enum { COMBO_BLE_SET = 0, COMBO_BLE_GET, COMBO_BLE_MAX } combo_ble_operation_e;

/**
 * network init
 *
 * @return wifi mode
 */
wifi_mode_e app_network_init(void);

/**
 * get network mode
 *
 * @return wifi mode
 */
wifi_mode_e app_network_check(void);

int app_wifi_driver_init(void);

/**
 * get wifi connect status
 *
 * @return 1: connnecting
 */
int app_wifi_connecting();

/**
 * set wifi low power enable
 *
 * @param [in] lpm_en 1:can enter low power mode, 0: cannot enter low power mode
 * 
 * @return 1: connnecting
 */
void app_wifi_lpm_enable(int lpm_en);

/**
 * set wifi low power
 *
 * @param [in] lpm_on 1:lpm on, 0:lpm off
 */
void app_wifi_set_lpm(int lpm_on);

/**
 * get internet connect status
 * 
 * @return 1: connected, 0: not connected
 */
int  app_wifi_internet_is_connected();

/**
 * set internet connect status
 * 
 * @return void
 */
void app_wifi_internet_set_connected(int connected);

/**
 * Scan signal to obtain the best connection hotspot
 * 
 * @return void
 */
void app_wifi_network_init_list();

/**
 * get wifi mac address
 * 
 * @param [out] mac address buffer
 * 
 * @return 0: success
 */
int app_wifi_getmac(uint8_t mac[6]);

/**
 * get BT mac address
 *
 * @param [out] mac address buffer
 *
 * @return 0: success
 */
int app_bt_getmac(char *mac);


#endif
