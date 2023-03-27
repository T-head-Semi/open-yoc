/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */
#ifndef _APP_EVENT_MGR_H_
#define _APP_EVENT_MGR_H_

#include <uservice/uservice.h>
#include <uservice/eventid.h>

/*系统*/
#define EVENT_STATUS_STARTING   (EVENT_USER + 100) /*系统启动*/
#define EVENT_KEY_PRESSED       (EVENT_USER + 101) /*有按键触发*/
#define EVENT_PLAYER_CHANGE     (EVENT_USER + 102) /*播放状态变化, 播放 暂停 静音*/

/*配网*/
#define EVENT_STATUS_WIFI_PROV_START   (EVENT_USER + 110) /*进入网络配置模式*/
#define EVENT_STATUS_WIFI_PROV_ALREADY_START (EVENT_USER + 111) /*重复进入配网模式*/
#define EVENT_STATUS_WIFI_PROV_TIMEOUT (EVENT_USER + 112) /*配网超时*/
#define EVENT_STATUS_WIFI_PROV_FAILED  (EVENT_USER + 113) /*配网失败*/
#define EVENT_STATUS_WIFI_PROV_RECVED  (EVENT_USER + 114) /*收到密码开始连接*/

/*网络连接*/
#define EVENT_STATUS_WIFI_CONN_SUCCESS (EVENT_USER + 120) /*连接路由器成功*/
#define EVENT_STATUS_WIFI_CONN_FAILED  (EVENT_USER + 121) /*网络连接失败，交互过程中网络异常*/
#define EVENT_STATUS_NTP_SUCCESS       (EVENT_USER + 122) /*网络连接成功*/

/*蓝牙*/
#define EVENT_STATUS_BT_CONNECTED (EVENT_USER + 130) /*蓝牙连接成功*/
#define EVENT_STATUS_BT_DISCONNECTED  (EVENT_USER + 131) /*蓝牙已经断开*/

/*唤醒*/
#define EVENT_STATUS_SESSION_START (EVENT_USER + 140) /*唤醒，进入语音交互*/
#define EVENT_STATUS_SESSION_STOP  (EVENT_USER + 141) /*断句*/

/*交互*/
#define EVENT_STATUS_NLP_NOTHING (EVENT_USER + 150) /*没听清楚*/
#define EVENT_STATUS_NLP_UNKNOWN (EVENT_USER + 151) /*NLP解析失败或无法执行的命令*/

/* 播放器事件 */
#define EVENT_MEDIA_START         (EVENT_USER + 160)
#define EVENT_MEDIA_SYSTEM_START  (EVENT_USER + 161)
#define EVENT_MEDIA_MUSIC_FINISH  (EVENT_USER + 162)
#define EVENT_MEDIA_SYSTEM_FINISH (EVENT_USER + 163)
#define EVENT_MEDIA_MUSIC_ERROR   (EVENT_USER + 164)
#define EVENT_MEDIA_SYSTEM_ERROR  (EVENT_USER + 165)

/* 调试 */
#define EVENT_DEBUG_EVENT_BLOCK   (EVENT_USER + 200)

typedef struct _status_event_ {
    int eventid;
    const char *name;
} status_event_t;

/**
 * @brief 事件管理初始化
 *
 * @return void
 */
void app_event_init();

/**
 * @brief 发送事件
 * @param eventid 事件ID
 * @return void
 */
void app_event_update(int eventid);

#endif