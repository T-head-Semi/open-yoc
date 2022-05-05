/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <yoc/netmgr.h>
#include <ulog/ulog.h>
#include <aos/kv.h>
#include <softap_prov.h>
#include <wifi_provisioning.h>

#include "wifi/multi_ssid/wifi_config.h"
#include "player/app_player.h"
#include "app_main.h"
#include "wifi/app_net.h"
#include "wifi_prov.h"

#define TAG "wifiprov"

extern netmgr_hdl_t app_netmgr_hdl;
static int          g_wifi_pairing;
static int          g_wifi_pair_broadcast_status;

#if defined(CONFIG_WIFI_SMARTLIVING) && CONFIG_WIFI_SMARTLIVING
#if defined(CONFIG_WIFI_PROV_BREEZE) && CONFIG_WIFI_PROV_BREEZE
int g_wifi_prov_method = WIFI_PROVISION_BREEZE;
#else
int g_wifi_prov_method = WIFI_PROVISION_SL_DEV_AP;
#endif
#else
int g_wifi_prov_method = WIFI_PROVISION_SOFTAP;
#endif

int wifi_is_pairing()
{
    return (g_wifi_pairing);
}

void wifi_pair_set_prov_type(int type)
{
    g_wifi_prov_method = type;
}
int wifi_pair_get_prov_type()
{
    return g_wifi_prov_method;
}

void wifi_pair_set_broadcast(int status)
{
    g_wifi_pair_broadcast_status = status;
}

int wifi_pair_get_broadcast()
{
    return g_wifi_pair_broadcast_status;
}

static void wifi_set_pairing(int pairing)
{
    g_wifi_pairing = pairing;
}

static void wifi_network_init(char *ssid, char *psk)
{
    LOGI(TAG, "Start wifi network");
    LOGD(TAG, "SSID=%s PASS=%s", ssid, psk);

    netmgr_config_wifi(app_netmgr_hdl, ssid, strlen(ssid), psk, strlen(psk));
    aos_dev_t *dev = netmgr_get_dev(app_netmgr_hdl);
    hal_net_set_hostname(dev, "T-head");
    netmgr_start(app_netmgr_hdl);

    return;
}

static void wifi_pair_callback(uint32_t method_id, wifi_prov_event_t event,
                               wifi_prov_result_t *result)
{
    if (event == WIFI_PROV_EVENT_TIMEOUT) {
        LOGD(TAG, "wifi pair timeout...");
        local_audio_play(LOCAL_AUDIO_NET_CFG_TIMEOUT); /* 配网超时 */

        static char wifi_ssid[32 + 1];
        int         wifi_ssid_len = sizeof(wifi_ssid);
        static char wifi_psk[64 + 1];
        int         wifi_psk_len = sizeof(wifi_psk);

        aos_kv_get("wifi_ssid", wifi_ssid, &wifi_ssid_len);
        aos_kv_get("wifi_psk", wifi_psk, &wifi_psk_len);

        if (!app_wifi_config_is_empty())
            app_wifi_network_init_list();

    } else if (event == WIFI_RPOV_EVENT_GOT_RESULT) {
        LOGD(TAG, "wifi pair got passwd...");
        local_audio_play(LOCAL_AUDIO_NET_CFG_CONN); /* 收到密码，开始连接 */

        app_wifi_config_add(result->ssid, result->password);
        wifi_network_init(result->ssid, result->password);

        event_publish_delay(EVENT_NET_CHECK_TIMER, NULL, 30 * 1000);
    }

    wifi_set_pairing(0);
}

static void _wifi_pair_thread(void *arg)
{
    int ret = 0;
    wifi_set_pairing(1);
    g_wifi_pair_broadcast_status = 1;

    LOGD(TAG, "start net config, method[%d]", g_wifi_prov_method);
    smtaudio_stop(MEDIA_ALL); // stop play when start wifi pair

#if defined(CONFIG_WIFI_SMARTLIVING) && CONFIG_WIFI_SMARTLIVING
    wifi_prov_sl_stop_report();
#endif
    wifi_prov_stop();

    app_wifi_internet_set_connected(0);

    switch (g_wifi_prov_method) {
    case WIFI_PROVISION_SOFTAP:
        ret = wifi_prov_start(wifi_prov_get_method_id("softap"), wifi_pair_callback, 120);
        break;
#if defined(CONFIG_WIFI_SMARTLIVING) && CONFIG_WIFI_SMARTLIVING
    case WIFI_PROVISION_SL_DEV_AP:
        ret = wifi_prov_start(wifi_prov_get_method_id("sl_dev_ap"), wifi_pair_callback, 120);
        break;

#if defined(CONFIG_WIFI_PROV_BREEZE) && CONFIG_WIFI_PROV_BREEZE
    case WIFI_PROVISION_BREEZE: {
        extern aos_event_t bt_event;
        unsigned int       actl_flags;
        int                result = -1;
        result =
            aos_event_get(&bt_event, EVENT_BT_FINISHED, AOS_EVENT_OR_CLEAR, &actl_flags, 10000);
        if (result == 0) { //修复蓝牙辅助配网模式下，得先初始化完蓝牙才启动配网
            wifi_prov_start(wifi_prov_get_method_id("sl_ble_breeze"), wifi_pair_callback, 120);
            aos_event_free(&bt_event);
        } else {
            ret = -1;
            LOGE(TAG, "get bt_event-flags failed.");
        }
        break;
    }
#endif
#endif
    default:
        LOGE(TAG, "unsupported wifi provisioning method!");
        wifi_set_pairing(0);
        ret = -1;
        break;
    }

    if (0 == ret) {
        local_audio_play(LOCAL_AUDIO_NET_CFG_START); /* 进入配网模式 */
    } else {
        local_audio_play(LOCAL_AUDIO_NET_CFG_FAIL); /* 配网失败 */
    }
}

void wifi_pair_start(void)
{
    aos_task_t task_handle;

    if (wifi_is_pairing()) {
        local_audio_play(LOCAL_AUDIO_NET_CFG_CONFIG); /* 正在配置网络，请稍后 */
        return;
    }

    if (0 != aos_task_new_ext(&task_handle, "pair_start", _wifi_pair_thread, NULL, 6 * 1024,
                              AOS_DEFAULT_APP_PRI)) {
        LOGE(TAG, "Create pair_start task failed.");
    }
}

void wifi_pair_stop(void)
{
    if (wifi_prov_get_status() != WIFI_PROV_STOPED) {
        wifi_prov_stop();
        wifi_set_pairing(0);

        if (!app_wifi_config_is_empty())
            app_wifi_network_init_list();
    }
}
