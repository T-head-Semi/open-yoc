/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _APP_MAIN_H_
#define _APP_MAIN_H_

#include <uservice/uservice.h>
#include <yoc/netmgr_service.h>
#include <uservice/eventid.h>
#include <aos/cli.h>
#include <aos/kv.h>

void board_cli_init();

#include <stdint.h>
extern netmgr_hdl_t app_netmgr_hdl;

void app_exception_event(uint32_t event_id);
void board_yoc_init(void);

void app_wifi_init(void);

/* user define event */
#define EVENT_DATA_REPORT  (EVENT_USER + 1)
#define EVENT_CHANNEL_CHECK (EVENT_USER + 2)
#define EVENT_CHANNEL_CLOSE (EVENT_USER + 3)

#endif