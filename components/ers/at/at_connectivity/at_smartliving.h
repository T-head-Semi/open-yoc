/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _AT_SMARTLIVING_H
#define _AT_SMARTLIVING_H

#include <uservice/eventid.h>

#define EVENT_APP_AT_CMD            (EVENT_USER + 26)

/* wifi provisioning method */
#define WIFI_PROVISION_MIN              0
#define WIFI_PROVISION_SL_SMARTCONFIG   0
#define WIFI_PROVISION_SOFTAP           1
#define WIFI_PROVISION_SL_BLE           2
#define WIFI_PROVISION_SL_DEV_AP        3
#define WIFI_PROVISION_MAX              4

/* boot reason */
#define BOOT_REASON_POWER_ON        0
#define BOOT_REASON_SOFT_RESET      1
#define BOOT_REASON_POWER_KEY       2
#define BOOT_REASON_WAKE_STANDBY    3
#define BOOT_REASON_WIFI_CONFIG     4
#define BOOT_REASON_NONE            5

typedef struct at_aui_info {
    int micEn;
    cJSON *js_info;
} at_aui_info_t;

typedef int (*idm_at_post_property)(int device_id, char *message, int len);
typedef int (*idm_at_post_event)(int device_id, char *evtid, int evtid_len, char *evt_payload, int len);
typedef int (*idm_is_connected)(void);

void idm_au_handler(char *cmd, int type, char *data);
void idm_pid_handler(char *cmd, int type, char *data);
void idm_con_handler(char *cmd, int type, char *data);
void idm_cls_handler(char *cmd, int type, char *data);
void idm_sta_handler(char *cmd, int type, char *data);
void at_cmd_dev_info(char *cmd, int type, char *data);
void at_cmd_aui_cfg(char *cmd, int type, char *data);
void at_cmd_aui_fmt(char *cmd, int type, char *data);
void at_cmd_wwv_en(char *cmd, int type, char *data);
void at_cmd_aui_kws(char *cmd, int type, char *data);
void at_cmd_aui_ctrl(char *cmd, int type, char *data);
void wjap_handler(char *cmd, int type, char *data);
void wjapd_handler(char *cmd, int type, char *data);
void wjapq_handler(char *cmd, int type, char *data);
void idm_pp_handler(char *cmd, int type, char *data);
void idm_ps_handler(char *cmd, int type, char *data);
void idm_ep_handler(char *cmd, int type, char *data);
void iws_start_handler(char *cmd, int type, char *data);
void iws_stop_handler(char *cmd, int type, char *data);
void at_cmd_aui_micen(char *cmd, int type, char *data);
void at_cmd_post_property_cb_reg(int (*cb)(int device_id, char *message, int len));
void at_cmd_post_event_cb_reg(int (*cb)(int device_id, char *evtid, int evtid_len, char *evt_payload, int len));
void at_cmd_is_connected_cb_reg(int (*cb)(void));


#endif

