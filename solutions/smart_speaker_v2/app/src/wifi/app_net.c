/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */
#include <aos/kv.h>
#include <devices/wifi.h>
#include <yoc/netmgr.h>
#include <yoc/netmgr_service.h>

#include <wifi_provisioning.h>
#include <softap_prov.h>
#include <ntp.h>

#include "app_main.h"
#include "wifi/wifi_prov/wifi_prov.h"
#include "wifi/multi_ssid/wifi_config.h"
#include "player/app_player.h"
#include "sys/app_sys.h"
#include "app_net.h"


#if defined(CONFIG_WIFI_SMARTLIVING) && CONFIG_WIFI_SMARTLIVING
#include <smartliving/sl_wifi_prov.h>
#endif
#if defined(EN_COMBO_NET) && EN_COMBO_NET
#include "combo_net.h"
#endif

#define TAG "app_net"

#define NET_RESET_DELAY_TIME         30  /* 最大网络重连时间 second*/
#define NET_LPM_RECONNECT_DELAY_TIME 180 /*second*/
#define MAX_NET_RESET_ERR_TIMES      -1 /* -1无效，若设置该值次数，达到该次数后系统重启 */
#define MAX_NET_RETRY_TIMES          3 /* 该值在开启低功耗模式生效，达到次数允许进入睡眠 */

netmgr_hdl_t     app_netmgr_hdl;
static int       ntp_sync_flag;
static int       net_reset_err_times  = 0;
static int       net_reset_delay_time = 2;
static int       net_retry_cnt        = -1;
static int       net_connecting       = 1;
static int       net_lpm_en           = 0;
static aos_sem_t g_wifi_config_sem;

static aos_mutex_t lock_deinit;
static char *      ntp_server[3] = {"ntp1.aliyun.com", "ntp2.aliyun.com", "0.cn.pool.ntp.org"};

int app_wifi_internet_is_connected()
{
    return (ntp_sync_flag == 1);
}

void app_wifi_set_lpm(int lpm_on)
{
    if (app_netmgr_hdl) {
        aos_dev_t *dev = netmgr_get_dev(app_netmgr_hdl);
        hal_wifi_set_lpm(dev, lpm_on ? WIFI_LPM_KEEP_LINK : WIFI_LPM_NONE);
    }
}

void app_wifi_internet_set_connected(int connected)
{
    ntp_sync_flag = connected;
}

static int wifi_network_inited()
{
    return (app_netmgr_hdl != NULL);
}


// void wifi_reconnect()
// {
//     event_publish(EVENT_NET_RECONNECT, NULL);
// }

static void network_reset_handle(uint32_t event_id, int reason)
{
    switch (event_id) {
    case EVENT_NET_RECONNECT:
    case EVENT_NETMGR_NET_DISCON:
        net_reset_err_times++;
        LOGD(TAG, "net reset cnt %d", net_reset_err_times);

        if (net_reset_err_times >= MAX_NET_RESET_ERR_TIMES && MAX_NET_RESET_ERR_TIMES > 0) {
            LOGD(TAG, "Net Reset times %d, reboot", net_reset_err_times);
            //do reboot
            app_sys_reboot(BOOT_REASON_SOFT_RESET);
        } else {
            if (net_lpm_en && net_reset_err_times > MAX_NET_RETRY_TIMES) {
                LOGD(TAG, "Net Reset times %d, goto sleep", net_reset_err_times);
                // if in low power mode, net should stop retry to allow system to goto sleep
                event_publish(EVENT_NET_LPM_RECONNECT, NULL);
            } else {
                if (reason == NET_DISCON_REASON_WIFI_NOEXIST) {
                    LOGD(TAG, "AP not found");
                } else if (reason == NET_DISCON_REASON_WIFI_PSK_ERR) {
                    LOGD(TAG, "password error");
                }

                if (!wifi_is_pairing()) {
                    char *ssid, *psk;
                    net_connecting = 1;

                    net_retry_cnt = (net_retry_cnt + 1) % 4;
                    if (net_retry_cnt == 3) {
                        int ret = app_wifi_config_select_ssid(&ssid, &psk);

                        if (ret >= 0) {
                            netmgr_config_wifi(app_netmgr_hdl, ssid, strlen(ssid), psk,
                                               strlen(psk));
                        }
                    }

                    netmgr_reset(app_netmgr_hdl, net_reset_delay_time);

                    /* double delay time to reconnect */
                    net_reset_delay_time *= 2;
                    if (net_reset_delay_time > NET_RESET_DELAY_TIME) {
                        net_reset_delay_time = NET_RESET_DELAY_TIME;
                    }
                }
            }
        }
        break;
    case EVENT_NETMGR_GOT_IP:
        net_retry_cnt        = -1;
        net_connecting       = 0;
        net_reset_err_times  = 0;
        net_reset_delay_time = 2;
        break;
    case EVENT_NET_LPM_RECONNECT:
        LOGD(TAG, "reconnect %d s later", NET_LPM_RECONNECT_DELAY_TIME);
        net_connecting       = 0;
        net_reset_err_times  = 0;
        net_reset_delay_time = 2;
        event_publish_delay(EVENT_NETMGR_NET_DISCON, NULL, NET_LPM_RECONNECT_DELAY_TIME * 1000);
    default:
        break;
    }
}

static int  ntp_retry_count = 0;
static void network_normal_handle(uint32_t event_id, const void *param)
{
    static int synced_before = 0;

    int wifi_pair_need_broadcast = wifi_pair_get_broadcast();

    switch (event_id) {
    case EVENT_NETMGR_GOT_IP: {
        event_publish(EVENT_NTP_RETRY_TIMER, NULL);

        /* 配网成功后广播通知 */
        if (wifi_pair_need_broadcast == 1) {
            wifi_pair_set_broadcast(2);

            if (wifi_pair_get_prov_type() == WIFI_PROVISION_SOFTAP) {
                static uint8_t mac[6] = {0};
                aos_dev_t *    dev    = netmgr_get_dev(app_netmgr_hdl);
                hal_wifi_get_mac_addr(dev, mac);
                wifi_pair_broadcast(mac);
            }
#ifdef EN_COMBO_NET
            if (wifi_pair_get_prov_type() == WIFI_PROVISION_BREEZE) {
                combo_ap_conn_notify();
            }
#endif
        }
    } break;

    case EVENT_NETMGR_NET_DISCON: {
        LOGD(TAG, "Net down");
        /* 不主动语音提示异常，等有交互再提示 */
        app_wifi_internet_set_connected(0);
    } break;

    case EVENT_NET_CHECK_TIMER: {
        if (app_wifi_internet_is_connected() == 0) {
            /* (配网超时)您输入的密码好像不太对，请检查后再试一次吧  */
            local_audio_play(LOCAL_AUDIO_NET_CFG_FAIL);
            app_wifi_config_del(app_wifi_config_get_cur_ssid());
        } else {
            LOGI(TAG, "wifi connection: check ok");
        }
    } break;

    case EVENT_NTP_RETRY_TIMER:
        if (ntp_sync_time(ntp_server[ntp_retry_count % 3]) == 0) {
            ntp_retry_count = 0;
#if defined(CONFIG_RTC_EN) && CONFIG_RTC_EN
            /* 网络对时成功,同步到RTC中 */
            rtc_from_system();
#endif
            if (app_wifi_internet_is_connected() == 0) {
                /* 同步到时间,确认网络成功,提示音和升级只在第一次启动 */
                app_wifi_internet_set_connected(1);

                if (!synced_before || wifi_pair_need_broadcast) {
                    if (wifi_pair_need_broadcast) {
#if defined(CONFIG_WIFI_SMARTLIVING) && CONFIG_WIFI_SMARTLIVING
                        LOGD(TAG, "wifi provisiong success");
                        wifi_prov_sl_set_connected(1);
#if !(defined(CONFIG_SMARTLIVING_DEMO) && CONFIG_SMARTLIVING_DEMO)
                        wifi_prov_sl_start_report();
#endif
#endif
                        app_wifi_config_save();
                    }

                    synced_before            = 1;
                    wifi_pair_set_broadcast(0);

                    local_audio_play(LOCAL_AUDIO_NET_SUCC);
                    event_publish(EVENT_NET_NTP_SUCCESS, NULL);

#if defined(CONFIG_SMARTLIVING_DEMO) && CONFIG_SMARTLIVING_DEMO
                    extern int smartliving_client_control(const int start);
                    smartliving_client_control(1);
#endif

#if defined(APP_FOTA_EN) && APP_FOTA_EN
                    app_fota_start();
#endif
                }
            }
        } else {
            /* 同步时间失败重试 如果wifi没有连接 不需要重试NTP */
            if (net_connecting == 0) {
                ntp_retry_count++;
                if (ntp_retry_count >= 3)
                    event_publish_delay(EVENT_NTP_RETRY_TIMER, NULL, 6000);
                else
                    event_publish_delay(EVENT_NTP_RETRY_TIMER, NULL, 50);
            }
        }
        break;
    default:
        break;
    }
}

static void user_local_event_cb(uint32_t event_id, const void *param, void *context)
{
    if ((wifi_is_pairing() == 0) && wifi_network_inited()) {
        network_normal_handle(event_id, NULL);
        if(param) {
            network_reset_handle(event_id, *(int *)param);
        } else {
            network_reset_handle(event_id, -1);
        }
    } else {
        LOGE(TAG, "Critical network status callback %d", event_id);
    }
}

// static void wifi_network_deinit()
// {
//     LOGI(TAG, "Stop wifi network");

//     aos_mutex_lock(&lock_deinit, AOS_WAIT_FOREVER);

//     if (app_netmgr_hdl == NULL) {
//         goto DFIN;
//     }

//     netmgr_stop(app_netmgr_hdl);
//     netmgr_dev_wifi_deinit(app_netmgr_hdl);
//     netmgr_service_deinit();

//     app_netmgr_hdl = NULL;

// DFIN:
//     aos_mutex_unlock(&lock_deinit);
// }

static void scan_compeleted(aos_dev_t *dev, uint16_t number, wifi_ap_record_t *ap_records)
{
    wifi_ap_record_t wifiApRecord;
    for (int j = 0; j < number; ++j) {
        for (int i = j + 1; i < number; ++i) {
            if (ap_records[i].rssi > ap_records[j].rssi) {
                wifiApRecord  = ap_records[i];
                ap_records[i] = ap_records[j];
                ap_records[j] = wifiApRecord;
            }
        }
    }

    for (int j = 0; j < number; ++j) {
        app_wifi_config_add_ap((char *)ap_records[j].ssid);
    }

    aos_sem_signal(&g_wifi_config_sem);
}

static wifi_event_func wifi_event = {NULL, NULL, scan_compeleted, NULL};

void app_wifi_network_init_list()
{
    char *ssid, *psk;

    LOGI(TAG, "Start wifi network");
    aos_dev_t *dev = netmgr_get_dev(app_netmgr_hdl);
    hal_net_set_hostname(dev, "T-head");

    if (app_wifi_config_get_ssid_num() != 1) {

        hal_wifi_install_event_cb(dev, &wifi_event);
        aos_check(!aos_sem_new(&g_wifi_config_sem, 0), EIO);

        hal_wifi_start_scan(dev, NULL, 1);

        aos_sem_wait(&g_wifi_config_sem, AOS_WAIT_FOREVER);
        hal_wifi_install_event_cb(dev, NULL);

        aos_sem_free(&g_wifi_config_sem);

        int ret = app_wifi_config_get_last_ssid_psk(&ssid, &psk);

        if (ret < 0) {
            return;
        }
    } else {
        int ret = app_wifi_config_get_last_ssid_psk(&ssid, &psk);

        if (ret < 0) {
            //return;
            ssid = "Yoc";
            psk = "testtest";
        }
    }

    netmgr_config_wifi(app_netmgr_hdl, ssid, strlen(ssid), psk, strlen(psk));

    LOGI(TAG, "SSID=%s PASS=%s", ssid, psk);
    netmgr_start(app_netmgr_hdl);

    return;
}

int app_wifi_connecting()
{
    return net_connecting;
}

void app_wifi_lpm_enable(int lpm_en)
{
    net_lpm_en = lpm_en;
}

int app_wifi_getmac(uint8_t mac[6])
{
    int ret = -1;

    aos_dev_t *dev = netmgr_get_dev(app_netmgr_hdl);

    if (dev) {
        hal_wifi_set_mode(dev, WIFI_MODE_STA);
        ret = hal_wifi_get_mac_addr(dev, mac);
    } else {
        LOGE(TAG, "get mac failed! Device not found!");
    }

    return ret;
}

int app_bt_getmac(char* mac)
{
    int ret = -1;

    aos_dev_t *dev = netmgr_get_dev(app_netmgr_hdl);

    if (dev) {
        hal_wifi_set_mode(dev, WIFI_MODE_STA);
        extern int bt_get_mac_address(char *mac);
        ret = bt_get_mac_address(mac);
    } else {
        LOGE(TAG, "get mac failed! Device not found!");
    }

    return ret;
}

static wifi_mode_e network_event_init(void)
{
    /* 系统事件订阅 */
    event_subscribe(EVENT_NETMGR_GOT_IP, user_local_event_cb, NULL);
    event_subscribe(EVENT_NETMGR_NET_DISCON, user_local_event_cb, NULL);
    event_subscribe(EVENT_NET_RECONNECT, user_local_event_cb, NULL);

    /* 使用系统事件的定时器 */
    event_subscribe(EVENT_NTP_RETRY_TIMER, user_local_event_cb, NULL);
    event_subscribe(EVENT_NET_CHECK_TIMER, user_local_event_cb, NULL);
    event_subscribe(EVENT_NET_LPM_RECONNECT, user_local_event_cb, NULL);
    event_subscribe(EVENT_NET_CONFIG_NETWORK_TIMEOUT, user_local_event_cb, NULL);
#if 0
    if (app_wifi_config_is_empty() || app_sys_get_boot_reason() == BOOT_REASON_WIFI_CONFIG) {
        if (app_sys_get_boot_reason() == BOOT_REASON_WIFI_CONFIG) {
            int last_method = 0;
            if (aos_kv_getint("wprov_method", &last_method) == 0) {
                wifi_pair_set_prov_type(last_method);
                aos_kv_del("wprov_method");
            }
        }
        wifi_pair_start();

        /* 配网组件会调用系统的重启，启动配网先把重启原因设置为软重启 */
        app_sys_set_boot_reason(BOOT_REASON_SOFT_RESET);
        return MODE_WIFI_PAIRING;
    } else {
        app_wifi_network_init_list();
    }
#else
    app_wifi_network_init_list();
#endif
    return MODE_WIFI_NORMAL;
}

wifi_mode_e app_network_init(void)
{
    int wifi_en;
    int ret;

    aos_mutex_new(&lock_deinit);

    ret = aos_kv_getint("wifi_en", &wifi_en);
    if (ret < 0) {
        /* 若没有设置KV,默认使能 */
        wifi_en = 1;
    }

    if (wifi_en == 0) {
        return MODE_WIFI_CLOSE;
    }

    /* init wifi driver and network */
    app_wifi_driver_init();

    /* 该模式，初始化驱动，但返回错误，关闭应用网络流程*/
    if (wifi_en == 2) {
        LOGI(TAG, "@@@ WiFi Debug Mode @@@");
        //extern void hal_wifi_test_enabled(int en);
        //hal_wifi_test_enabled(1);
        return MODE_WIFI_TEST;
    }

    wifi_prov_softap_register("YoC");

#if defined(CONFIG_WIFI_SMARTLIVING) && CONFIG_WIFI_SMARTLIVING
    wifi_prov_sl_register();
#endif

    utask_t *task = utask_new("netmgr", 10 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);

    app_netmgr_hdl = netmgr_dev_wifi_init();
    netmgr_service_init(task);
    return network_event_init();
}

wifi_mode_e app_network_check(void)
{
    int wifi_en = 1;
    int ret;
    ret = aos_kv_getint("wifi_en", &wifi_en);
    if (ret < 0) {
        /* 若没有设置KV,默认使能 */
        wifi_en = 1;
    }

    switch (wifi_en) {
    case 0:
        return MODE_WIFI_CLOSE;
    case 2:
        return MODE_WIFI_TEST;
    }

    app_wifi_config_init();
    if (app_wifi_config_is_empty() || app_sys_get_boot_reason() == BOOT_REASON_WIFI_CONFIG) {
        return MODE_WIFI_PAIRING;
    }
    return MODE_WIFI_NORMAL;
}