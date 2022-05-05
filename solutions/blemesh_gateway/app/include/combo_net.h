/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifdef EN_COMBO_NET

#ifndef __COMBO_NET_H__
#define __COMBO_NET_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <wifi_provisioning.h>

#define COMBO_EVT_CODE_AP_INFO         0x0001
#define COMBO_EVT_CODE_RESTART_ADV     0x0002
#define COMBO_EVT_CODE_RX_PAYLOAD      0x0003
#define COMBO_EVT_CODE_TSL_ACK         0x0004
#define COMBO_EVT_CODE_FULL_REPORT     0x0005

// Combo device AP connect status
#define COMBO_AP_DISCONNECTED          0        // ap connected
#define COMBO_AP_CONNECTED             1        // ap disconnected
#define COMBO_AP_CONN_UNINIT           2        // combo device not connect ap yet

// Combo device AP config need or not
#define COMBO_AWSS_NOT_NEED            0        // ap configuration is not required
#define COMBO_AWSS_NEED                1        // ap configuration is required

/* extern functions */
typedef enum {
    TOKEN_TYPE_NOT_CLOUD = 0x00,
    TOKEN_TYPE_CLOUD,
    TOKEN_TYPE_MAX,
    TOKEN_TYPE_INVALID = 0xFF
} bind_token_type_t;
#define RANDOM_MAX_LEN  (16)


typedef int (* combo_event_cb)(uint16_t evt_code);
typedef int (* combo_common_serv_cb)(const int devid, const char *serviceid, const int serviceid_len,
        const char *request, const int request_len);
typedef int (* combo_property_set_cb)(const int devid, const char *request, const int request_len);
typedef int (* combo_property_get_cb)(const int devid, const char *request, const int request_len, char **response,
        int *response_len);

int combo_net_init(wifi_prov_cb cb);
int combo_net_deinit(void);
uint8_t combo_ble_conn_state(void);
void combo_set_cloud_state(uint8_t cloud_connected);
void combo_set_ap_state(uint8_t ap_connected);
void combo_set_awss_state(uint8_t awss_running);
void combo_reg_evt_cb(combo_event_cb event_cb);
void combo_reg_common_serv_cb(combo_common_serv_cb common_serv_cb);
void combo_reg_property_set_cb(combo_property_set_cb property_set_cb);
void combo_reg_property_get_cb(combo_property_get_cb property_get_cb);
void combo_ap_conn_notify(void);
void combo_token_report_notify(void);
int combo_tsl_ack(uint16_t err_code, uint8_t *buffer, uint32_t length);
int combo_status_report(uint8_t *buffer, uint32_t length);
void combo_set_breeze_start(void);
uint8_t combo_is_breeze_start(void);
void combo_set_mesh_node_init();

int ieee80211_is_beacon(uint16_t fc);
int ieee80211_is_probe_resp(uint16_t fc);
int ieee80211_get_ssid(uint8_t *beacon_frame, uint16_t frame_len, uint8_t *ssid);
int aw_ieee80211_get_bssid(uint8_t *in, uint8_t *mac);
int awss_set_token(uint8_t token[RANDOM_MAX_LEN], bind_token_type_t token_type);
uint8_t aws_next_channel(void);

#endif
#endif
