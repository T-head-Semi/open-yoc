/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _APP_MAIN_H_
#define _APP_MAIN_H_

#include <app_config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <aos/aos.h>
#include <aos/cli.h>
#include <aos/kv.h>

// #include <pin_name.h>
#include <drv/gpio.h>
// #include <pinmux.h>
// #include <devices/devicelist.h>

#include <yoc/yoc.h>
#include <uservice/eventid.h>
#include <uservice/uservice.h>

/* wifi & net */
typedef enum {
    MODE_WIFI_TEST = -2,
    MODE_WIFI_CLOSE = -1,
    MODE_WIFI_NORMAL = 0,
    MODE_WIFI_PAIRING = 1
} wifi_mode_e;

wifi_mode_e app_network_init(void);

/* Smart Living */
/**
 * control smartliving client
 *
 * @param[in]  flag          0: stop, others: start
 *
 * @return  0: success
 */
int smartliving_client_control(const int flag);

/**
 * Get connection status of smartliving client
 *
 * @return 0: disconnected
 */
int smartliving_client_is_connected(void);

/**
 * report property if changed for demo
 *
 * @return  0: success
 */
void user_post_property(void);

/**
 * smartliving report property of AT command
 *
 * @param[in]  device_id     Device ID, should be 0
 * @param[in]  message       pointer to message
 * @param[in]  len           lenth of message
 *
 * @return >0: packet id, others: fail
 */
int user_at_post_property(int device_id, char *message, int len);

/**
 * smartliving report event of AT command
 *
 * @param[in]  device_id     Device ID, should be 0
 * @param[in]  evtid         pointer to Event ID
 * @param[in]  evtid_len     lenth of Event ID
 * @param[in]  evt_payload   pointer to event payload
 * @param[in]  len           lenth of event payload
 *
 * @return >0: Packet ID, others: fail
 */
int user_at_post_event(int device_id, char *evtid, int evtid_len, char *evt_payload, int len);



void wifi_pair_start(void);
wifi_mode_e app_network_reinit();



/**
 * report property of the key number pressed for demo
 *
 * @param[in]  key_num       Key Number
 */
void user_post_key_press_event(int key_num);

void app_button_init(void);

void app_at_server_init(utask_t *task, const char *device_name);
int app_at_parser_init(utask_t *task, const char *device_name);
void app_pinmap_usart_init(int32_t uart_idx);
int app_at_cmd_init(void);
void app_at_cmd_sta_report();
void app_at_cmd_property_report_set(const char *msg, const int len);
void app_at_cmd_property_report_reply(const int packet_id, const int code, const char *reply, const int len);
void app_at_cmd_event_report_reply(const int packet_id, const int code, const char *evtid, const int evtid_len, const char *reply, const int len);


/* led */
void app_set_led_state(int state);
void app_set_led_enabled(int en);
void app_status_init(void);
void app_status_update(void);

/********************* PWM LED ****************************/
/**
 * PWM LED Initialize
 * @return 0 on success
 */
int8_t app_pwm_led_init();

/**
 * PWM LED Control
 * @param [in] flag      : on(1)/off(0)
 * @return 0 on success
 */
int8_t app_pwm_led_control(const uint8_t flag);

/**
 * PWM LED Set Brightness
 * @param [in] brightness : 0 ~ 100
 * @return 0 on success
 */
int8_t app_pwm_led_set_brightness(int32_t brightness);

/**
 * PWM LED Get Brightness
 * @return 0 ~ 100 for current brightness
 */
int8_t app_pwm_led_get_brightness();

/**
 * PWM LED Get Status
 * @return on(1)/off(0)
 */
int8_t app_pwm_led_get_status();

void cli_reg_cmd_w800(void);

void cli_reg_cmd_kp(void);

void combo_net_post_ext(void);

void app_rpt_fwver(char *version);

/* wifi provisioning method */
#define WIFI_PROVISION_MIN              0
#define WIFI_PROVISION_SL_SMARTCONFIG   0
#define WIFI_PROVISION_SOFTAP           1
#define WIFI_PROVISION_SL_BLE           2
#define WIFI_PROVISION_SL_DEV_AP        3
#define WIFI_PROVISION_MAX              4


/* event id define */
#define EVENT_NTP_RETRY_TIMER       (EVENT_USER + 1)
#define EVENT_NET_CHECK_TIMER       (EVENT_USER + 2)
#define EVENT_NET_NTP_SUCCESS       (EVENT_USER + 3)
#define EVENT_NET_LPM_RECONNECT     (EVENT_USER + 4)

/* app at cmd */
#define EVENT_APP_AT_CMD            (EVENT_USER + 26)

#define PWM_LED_LIGHT_MIN 10
#define PWM_LED_LIGHT_MAX 100

typedef enum {
    COMBO_BLE_SET = 0,
    COMBO_BLE_GET,
    COMBO_BLE_MAX
} combo_ble_operation_e;

#endif
