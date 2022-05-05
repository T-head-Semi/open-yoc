/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _AT_NETWORK_H
#define _AT_NETWORK_H

#include <uservice/eventid.h>

#define EVENT_NET_PING_RESULT       (EVENT_USER + 5)
/* app at cmd */
#define EVENT_APP_AT_CMD            (EVENT_USER + 26)
#define EVENT_NETWORK_AT_CMD        (EVENT_USER + 26)

typedef enum {
    APP_AT_CMD_IWS_START,
    APP_AT_CMD_WJAP,
    APP_AT_CMD_WDAP,
    APP_AT_CMD_CON,
    APP_AT_CMD_CLS,
} APP_AT_CMD_TYPE;

void wgip_handler(char *cmd, int type, char *data);
void wscan_handler(char *cmd, int type, char *data);
void ping_handler(char *cmd, int type, char *data);
void httpget_handler(char *cmd, int type, char *data);
void httphead_handler(char *cmd, int type, char *data);
void httpota_handler(char *cmd, int type, char *data);
void ntp_handler(char *cmd, int type, char *data);
void sta_mac_handler(char *cmd, int type, char *data);
void rst_handler(char *cmd, int type, char *data);
void dns_handler(char *cmd, int type, char *data);
void link_info_handler(char *cmd, int type, char *data);
void link_status_handler(char *cmd, int type, char *data);
void wifi_info_handler(char *cmd, int type, char *data);
void wjap_handler(char *cmd, int type, char *data);
void wjapd_handler(char *cmd, int type, char *data);
void wjapq_handler(char *cmd, int type, char *data);
void wdap_handler(char *cmd, int type, char *data);
void ver_handler(char *cmd, int type, char *data);
int at_network_evt_sub(void);

#endif