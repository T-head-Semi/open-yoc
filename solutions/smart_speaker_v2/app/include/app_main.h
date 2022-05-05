/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _APP_MAIN_H_
#define _APP_MAIN_H_

#include <uservice/eventid.h>

/*************
 * USER EVENT LIST
 ************/
/* event id define */
#define EVENT_NTP_RETRY_TIMER            (EVENT_USER + 1)
#define EVENT_NET_CHECK_TIMER            (EVENT_USER + 2)
#define EVENT_NET_NTP_SUCCESS            (EVENT_USER + 3)
#define EVENT_NET_LPM_RECONNECT          (EVENT_USER + 4)
#define EVENT_NET_RECONNECT              (EVENT_USER + 5)
#define EVENT_NET_CONFIG_NETWORK_TIMEOUT (EVENT_USER + 6)

#define EVENT_LPM_CHECK      (EVENT_USER + 11)
#define EVENT_BATTERY_CHECK  (EVENT_USER + 12)
#define EVENT_CPUUSAGE_CHECK (EVENT_USER + 13)
#define EVENT_PA_CHECK       (EVENT_USER + 14)
#define EVENT_RTC_UPDATE     (EVENT_USER + 15)
#define EVENT_SYSMEM_CHECK   (EVENT_USER + 16)

/* media event */
#define EVENT_MEDIA_START         (EVENT_USER + 21)
#define EVENT_MEDIA_MUSIC_FINISH  (EVENT_USER + 22)
#define EVENT_MEDIA_SYSTEM_FINISH (EVENT_USER + 23)
#define EVENT_MEDIA_MUSIC_ERROR   (EVENT_USER + 24)
#define EVENT_MEDIA_SYSTEM_ERROR  (EVENT_USER + 25)

/* long press button event */
#define EVENT_BUTTON_LONG_PRESSED (EVENT_USER + 31)

/* bt init event */
#define EVENT_BT_FINISHED (1 << 0)

void board_cli_init(void);
void board_yoc_init(void);

#endif