/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <string.h>
#include <aos/kernel.h>
#include <aos/debug.h>
#include <ulog/ulog.h>

#include "wifi_provisioning.h"
#include "exports/iot_export_linkkit.h"
#include "sl_wifi_prov.h"

#define EVT_LINKKIT_QUIT            (1 << 0)

#define EVT_NET_CONNECTED            (1 << 0)
#define EVT_NET_DISCONNECTED         (1 << 1)

#define TAG "SL_PROV"

static int sl_dev_ap_start(wifi_prov_cb cb);
static int sl_smartconfig_start(wifi_prov_cb cb);
static void sl_dev_ap_stop();
static void sl_smartconfig_stop();
#if (((defined CONFIG_WIFI_PROV_BLE) && CONFIG_WIFI_PROV_BLE) || (defined CONFIG_WIFI_PROV_BREEZE) && CONFIG_WIFI_PROV_BREEZE)
static int sl_bleconfig_start(wifi_prov_cb cb);
static void sl_bleconfig_stop();
#endif

static wifi_prov_cb prov_cb;
static uint32_t current_method = 0;
static aos_event_t evt_linkkit_quit;
static aos_event_t evt_net_connected;
static int linkkit_running = 0, linkkit_quit = 1;
static int callback_running = 0;

static wifi_prov_t awss_dp_priv = {
    .name        = "sl_dev_ap",
    .start       = sl_dev_ap_start,
    .stop        = sl_dev_ap_stop,
};

static wifi_prov_t awss_sf_priv = {
    .name        = "sl_smartconfig",
    .start       = sl_smartconfig_start,
    .stop        = sl_smartconfig_stop,
};

#if (defined CONFIG_WIFI_PROV_BLE) && CONFIG_WIFI_PROV_BLE
static wifi_prov_t awss_ble_priv = {
    .name        = "sl_ble",
    .start       = sl_bleconfig_start,
    .stop        = sl_bleconfig_stop,
};
#endif

#if (defined CONFIG_WIFI_PROV_BREEZE) && CONFIG_WIFI_PROV_BREEZE		
static wifi_prov_t awss_ble_breeze = {		
    .name        = "sl_ble_breeze",		
    .start       = sl_bleconfig_start,		
    .stop        = sl_bleconfig_stop,		
};		
#endif

static void linkkit_start(void *arg)
{
    int res = 0;
    int cnt = 0;
    iotx_linkkit_dev_meta_info_t master_meta_info;
    int domain_type = 0, dynamic_register = 0, post_reply_need = 0;
    int master_devid;
    int connect_cnt = 0;

    memset(&master_meta_info, 0, sizeof(iotx_linkkit_dev_meta_info_t));
    HAL_GetProductKey(master_meta_info.product_key);
    HAL_GetDeviceName(master_meta_info.device_name);
    HAL_GetProductSecret(master_meta_info.product_secret);
    HAL_GetDeviceSecret(master_meta_info.device_secret);

    IOT_SetLogLevel(IOT_LOG_DEBUG);

    domain_type = IOTX_CLOUD_REGION_SHANGHAI;
    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);

    /* Choose Login Method */
    dynamic_register = 0;
    IOT_Ioctl(IOTX_IOCTL_SET_DYNAMIC_REGISTER, (void *)&dynamic_register);

    /* post reply doesn't need */
    post_reply_need = 0;
    IOT_Ioctl(IOTX_IOCTL_RECV_EVENT_REPLY, (void *)&post_reply_need);
    
    /* Create Master Device Resources */
    master_devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_MASTER, &master_meta_info);
    if (master_devid < 0) {
        goto END;
    }

    /* Start Connect Aliyun Server */
    do {
        res = IOT_Linkkit_Connect(master_devid);
        if (res >= 0) {
            break;
        }
        printf("IOT_Linkkit_Connect failed! retry after %d ms\n", 3000);
        aos_msleep(3000);
    } while (linkkit_running && ++connect_cnt < 3);

    cnt = 0;
    while (linkkit_running && cnt++ < 200) {
        IOT_Linkkit_Yield(300);
    }

    IOT_Linkkit_Close(master_devid);

END:
    linkkit_running = 0;
    linkkit_quit = 1;
    aos_event_set(&evt_linkkit_quit, EVT_LINKKIT_QUIT, AOS_EVENT_OR);
    return;
}

static int sl_prov_cb(int success, char *ssid, char *password, int timeout)
{
    if (current_method == 0) {
        return -1;
    }

	callback_running = 1;
	aos_event_set(&evt_net_connected, 0, AOS_EVENT_AND);

    if (success) {
        wifi_prov_result_t res;
        memset(&res, 0, sizeof(wifi_prov_result_t));
        strncpy(res.ssid, ssid, sizeof(res.ssid) - 1);
        strncpy(res.password, password, sizeof(res.password) - 1);

        prov_cb(current_method, WIFI_RPOV_EVENT_GOT_RESULT, &res);
    } else {
        prov_cb(current_method, WIFI_PROV_EVENT_TIMEOUT, NULL);
    }

    unsigned int flags = 0;
    aos_event_get(&evt_net_connected, EVT_NET_CONNECTED | EVT_NET_DISCONNECTED, AOS_EVENT_OR_CLEAR, &flags, timeout);

	callback_running = 0;
    return (flags & EVT_NET_CONNECTED) ? 0 : -1;
}

static void sl_prov_thread(void *args)
{
    wifi_prov_t *priv = (wifi_prov_t *)args;

    if (priv->method_id == awss_dp_priv.method_id) {    
        awss_dev_ap_start(sl_prov_cb, 300000);
    } else if (priv->method_id == awss_sf_priv.method_id) {
        awss_start(sl_prov_cb);
    } else {
        LOGE(TAG, "wifi prov method id mismatch: %d", priv->method_id);
        current_method = 0;
    }
}

/**
    User starts the smartconfig process
*/
static int sl_smartconfig_start(wifi_prov_cb cb)
{
    int ret;

    awss_config_press();
    prov_cb = cb;
    ret = aos_task_new("slsfprov", sl_prov_thread, &awss_sf_priv, 4096);
    CHECK_RET_WITH_RET(ret == 0, -1);

    current_method = awss_sf_priv.method_id;
    return 0;
}

static void sl_smartconfig_stop()
{
    if (!callback_running) {
        LOGD(TAG, "call awss stop\n");
        awss_stop();
    }
}

/**
    User starts the dev_ap process
*/
static int sl_dev_ap_start(wifi_prov_cb cb)
{    
    int ret;
    awss_config_press();
    prov_cb = cb;
    ret = aos_task_new("sldpprov", sl_prov_thread, &awss_dp_priv, 4096);
    CHECK_RET_WITH_RET(ret == 0, -1);

    current_method = awss_dp_priv.method_id;
    return 0;
}

static void sl_dev_ap_stop()
{
    if (!callback_running) {
        LOGD(TAG, "call awss_dev_ap stop\n");
        awss_dev_ap_stop();
    }
}

/**
    User starts the smartconfig process
*/
#if (((defined CONFIG_WIFI_PROV_BLE) && CONFIG_WIFI_PROV_BLE) || (defined CONFIG_WIFI_PROV_BREEZE) && CONFIG_WIFI_PROV_BREEZE)
static int sl_bleconfig_start(wifi_prov_cb cb)
{
    int ret;

    LOGD(TAG, "call ble config start");
    
    extern int combo_net_init(wifi_prov_cb cb);
    ret = combo_net_init(cb);
    return ret;
}

static void sl_bleconfig_stop()
{
    if (!callback_running) {
        LOGD(TAG, "call ble config stop");
        awss_stop();
    }
}
#endif

int wifi_prov_sl_register()
{
    aos_event_new(&evt_linkkit_quit, 0);
    aos_event_new(&evt_net_connected, 0);

    wifi_prov_method_register(&awss_sf_priv);
    wifi_prov_method_register(&awss_dp_priv);
#if (defined CONFIG_WIFI_PROV_BLE) && CONFIG_WIFI_PROV_BLE
    wifi_prov_method_register(&awss_ble_priv);
#endif
#if (defined CONFIG_WIFI_PROV_BREEZE) && CONFIG_WIFI_PROV_BREEZE		
    wifi_prov_method_register(&awss_ble_breeze);		
#endif

    return 0;
}

void wifi_prov_sl_start_report()
{
    aos_task_t tsk;

    if (linkkit_running) {
        return;
    }

    aos_event_set(&evt_linkkit_quit, 0, AOS_EVENT_AND);
    if (!linkkit_running && !linkkit_quit) {
        unsigned int flags;
        aos_event_get(&evt_linkkit_quit, EVT_LINKKIT_QUIT, AOS_EVENT_OR_CLEAR, &flags, AOS_WAIT_FOREVER);
    }

    /* these two varibles are for async quit of linkkit thread,
        linkkit_running to command to quit, linkkit_quit to inform the thread actually quit */
    linkkit_running = 1;
    linkkit_quit = 0;

    //aos_task_new_ext(&tsk, "linkkit", linkkit_start, NULL, 1024*6, AOS_DEFAULT_APP_PRI);
}

void wifi_prov_sl_stop_report()
{
    linkkit_running = 0;
}

void wifi_prov_sl_set_connected(int connected)
{
    aos_event_set(&evt_net_connected, connected ? EVT_NET_CONNECTED : EVT_NET_DISCONNECTED, AOS_EVENT_OR);
}
