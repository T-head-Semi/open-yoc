/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _APP_MAIN_H_
#define _APP_MAIN_H_

#include <stdint.h>
#include <uservice/uservice.h>
#include <yoc/netmgr_service.h>
#include <uservice/eventid.h>

extern netmgr_hdl_t app_netmgr_hdl;

void board_cli_init();
void board_yoc_init(void);
void app_network_init(void);
void app_fota_init(void);
void app_fota_start(void);
void app_exception_event(uint32_t event_id);

/* user define event */
#define EVENT_NTP_RETRY_TIMER       (EVENT_USER + 1)
#define EVENT_NET_CHECK_TIMER       (EVENT_USER + 2)
#define EVENT_NET_NTP_SUCCESS       (EVENT_USER + 3)

#endif