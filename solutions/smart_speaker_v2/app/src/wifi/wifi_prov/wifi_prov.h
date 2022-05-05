/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef __WIFI_PROV__
#define __WIFI_PROV__
#include <stdint.h>

typedef struct {
    char    ssid[33];
    char    password[65];
    uint8_t bssid[6];
    uint8_t channel;
    uint8_t auth_mode;
    char    auth[64];
} wifi_prov_res_t;

/**
 * start wifi pair
 */
void wifi_pair_start(void);

/**
 * stop wifi pair
 */
void wifi_pair_stop(void);

/**
 * get wifi pairing status
 * 
 * @return 1: pairing, 0: not pairing
 */
int wifi_is_pairing();

#define WIFI_PROVISION_SOFTAP    1
#define WIFI_PROVISION_BREEZE    2
#define WIFI_PROVISION_SL_DEV_AP 3

void wifi_pair_set_prov_type(int type);
int  wifi_pair_get_prov_type();

void wifi_pair_set_broadcast(int status);
int  wifi_pair_get_broadcast();
int  wifi_pair_broadcast(uint8_t *mac);

#endif
