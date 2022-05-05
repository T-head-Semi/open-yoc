/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <stdlib.h>
#include <string.h>
#include <devices/wifi.h>
#include <devices/device.h>
#include "app_main.h"
#ifdef CONFIG_WIFI_XR829
#include <devices/xr829.h>
#else
#include <devices/rtl8723ds.h>
#endif

#define TAG "app"

int g_net_gotip = 0;

/* app & cli & event */
void user_local_event_cb(uint32_t event_id, const void *param, void *context)
{
    if (event_id == EVENT_NETMGR_GOT_IP) {
        g_net_gotip = 1;
        LOGI(TAG, "Got IP");
    } else if (event_id == EVENT_NETMGR_NET_DISCON) {
        g_net_gotip = 0;
        LOGI(TAG, "Net down");
    } else {
        ;
    }
}

static void wifi_cb_sta_connect_fail(aos_dev_t *dev, wifi_err_code_t err, void *arg)
{
    LOGI(TAG, "%s\n", __FUNCTION__);
}

static void wifi_cb_status_change(aos_dev_t *dev, wifi_event_id_t stat, void *arg)
{
    LOGI(TAG, "%s\n", __FUNCTION__);
}

void print_encrypt_mode(wifi_encrypt_type_t encryptmode)
{
    switch (encryptmode) {
        case WIFI_ENC_TKIP:
            printf("TKIP");
            break;

        case WIFI_ENC_AES:
            printf("CCMP");
            break;

        case WIFI_ENC_TKIP_AES_MIX:
            printf("TKIP+CCMP");
            break;

        default:
            printf("ERR");
            break;
    }
}

static void wifi_cb_scan_compeleted(aos_dev_t *dev, uint16_t number, wifi_ap_record_t *ap_records)
{
    int i;

    printf("\nbssid / channel / signal dbm / flags / ssid\n\n");

    for (i = 0; i < number; i++) {
        wifi_ap_record_t *ap_record = &ap_records[i];

        printf("%02x:%02x:%02x:%02x:%02x:%02x %02d %d ",
               ap_record->bssid[0], ap_record->bssid[1], ap_record->bssid[2],
               ap_record->bssid[3], ap_record->bssid[4], ap_record->bssid[5],
               ap_record->channel, ap_record->rssi);

        switch (ap_record->authmode) {
            case WIFI_AUTH_OPEN:
                printf("[OPEN]");
                break;

            case WIFI_AUTH_WEP:
                printf("[WEP]");
                break;

            case WIFI_AUTH_WPA_PSK:
                printf("[WPA-PSK-");
                print_encrypt_mode(ap_record->encryptmode);
                printf("]");
                break;

            case WIFI_AUTH_WPA2_PSK:
                printf("[WPA2-PSK-");
                print_encrypt_mode(ap_record->encryptmode);
                printf("]");
                break;

            case WIFI_AUTH_WPA_WPA2_PSK:
                printf("[WPA-PSK-");
                print_encrypt_mode(ap_record->encryptmode);
                printf("]");
                printf("[WPA2-PSK-");
                print_encrypt_mode(ap_record->encryptmode);
                printf("]");
                break;

            default:
                printf("[NOT SUPPORT]");
                break;
        }

        printf("    %s\n",  ap_record->ssid);
    }

}

static void wifi_cb_fatal_err(aos_dev_t *dev, void *arg)
{


}

static wifi_event_func evt_cb = {
    .sta_connect_fail = wifi_cb_sta_connect_fail,
    .status_change = wifi_cb_status_change,
    .scan_compeleted = wifi_cb_scan_compeleted,
    .fatal_err = wifi_cb_fatal_err
};


void wifi_monitor_rx(wifi_promiscuous_pkt_t *buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_pkt_rx_ctrl_t *rx_ctrl = &buf->rx_ctrl;

    switch (type) {
        case WIFI_PKT_MGMT:
            //subtype = hdr->frame_ctl & 0xF0
            printf("MGMT ");
            break;

        case WIFI_PKT_DATA:
            printf("DATA ");
            break;

        case WIFI_PKT_CTRL:
            printf("CTRL\n");
            return;

        default:
            printf("Packet type not supported\n");
            return;
    }

    printf("Len=%d RSSI=%d RATE=%d CHANNEL=%d\n",
           rx_ctrl->sig_len, rx_ctrl->rssi, rx_ctrl->rate, rx_ctrl->channel);
}

static netmgr_hdl_t netmgr_hdl;

netmgr_hdl_t wifi_network_init()
{
    // aos_kv_setstring("wifi_ssid", CONFIG_WIFI_SSID);
    // aos_kv_setstring("wifi_psk", CONFIG_WIFI_PSK);

    netmgr_hdl_t netmgr_hdl = netmgr_dev_wifi_init();

    if (netmgr_hdl) {
        utask_t *task = utask_new("netmgr", 2 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);
        netmgr_service_init(task);
        netmgr_start(netmgr_hdl);
     }
    return netmgr_hdl;
}

static void wifihal_sta()
{
    netmgr_hdl = wifi_network_init();
}

static void wifihal_scan()
{
    if (netmgr_hdl == NULL) {
        netmgr_hdl = wifi_network_init();
    }
    aos_dev_t *dev = netmgr_get_dev(netmgr_hdl);


    wifi_scan_config_t scan_config;
    memset(scan_config.ssid, 0, sizeof(scan_config.ssid));
    memset(scan_config.bssid, 0, sizeof(scan_config.bssid));
    scan_config.channel = 0;
    scan_config.show_hidden = 0;
    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    scan_config.scan_time.active.max = 0;
    scan_config.scan_time.active.min = 0;
    scan_config.scan_time.passive = 0;


    hal_wifi_install_event_cb(dev, &evt_cb);
    LOGI(TAG, "wifi block scan");
    hal_wifi_start_scan(dev, &scan_config, 1);
    //LOGI(TAG, "wifi block scan done");

    aos_msleep(1000);
    LOGI(TAG, "wifi not block scan");
    hal_wifi_start_scan(dev, &scan_config, 0);
    LOGI(TAG, "wifi not block scan done");
}

static void wifihal_sta_ps()
{
    if (netmgr_hdl == NULL) {
        netmgr_hdl = wifi_network_init();
    }
    aos_dev_t *dev = netmgr_get_dev(netmgr_hdl);

    while (0 == g_net_gotip) {
        aos_msleep(100);
    }

    wifi_lpm_mode_t ps_type;
    hal_wifi_get_lpm(dev, &ps_type);
    LOGI(TAG, "Current PS=%s", (ps_type != WIFI_LPM_NONE) ? "ON" : "OFF");


    while (1) {
        aos_msleep(10 * 1000);
        ps_type = WIFI_LPM_KEEP_SOCKET;
        hal_wifi_set_lpm(dev, ps_type);
        hal_wifi_get_lpm(dev, &ps_type);
        LOGI(TAG, "Current PS=%s", (ps_type != WIFI_LPM_NONE) ? "ON" : "OFF");

        aos_msleep(10 * 1000);
        ps_type = WIFI_LPM_NONE;
        hal_wifi_set_lpm(dev, ps_type);
        hal_wifi_get_lpm(dev, &ps_type);
        LOGI(TAG, "Current PS=%s", (ps_type != WIFI_LPM_NONE) ? "ON" : "OFF");
    }
}


static void wifihal_ap_sta_list()
{
    aos_dev_t *dev = netmgr_get_dev(netmgr_hdl);
    wifi_sta_list_t sta_list;
    hal_wifi_ap_get_sta_list(dev , &sta_list);
}

static void wifihal_ap()
{
    if (netmgr_hdl == NULL)
        netmgr_hdl = netmgr_dev_wifi_init();
    aos_dev_t *dev = netmgr_get_dev(netmgr_hdl);
    //hal_wifi_set_mode(dev, WIFI_MODE_AP);
    LOGI(TAG, "Start AP");
    wifi_config_t config;
    memset(&config, 0, sizeof(wifi_config_t));
    config.mode = WIFI_MODE_AP;
    strcpy(config.ssid, "wifihal_ap");
    strcpy(config.password, "12345678");
    hal_wifi_start(dev, &config);
    LOGI(TAG, "Start AP End");
}

static void wifihal_reset_test()
{
    printf("Reset test\n");
    static aos_dev_t *dev;
    if (dev == NULL)
        dev = device_open_id("wifi", 0);
    
    hal_wifi_init(dev);
    hal_wifi_reset(dev);
    wifi_config_t *wifi_config = aos_zalloc(sizeof(wifi_config_t));
    wifi_config->mode = WIFI_MODE_STA;

    #ifdef CONFIG_KV_SMART
    int ssid_len = sizeof(wifi_config->ssid);
    int psw_len = sizeof(wifi_config->password);

    if (aos_kv_get(KV_WIFI_SSID, wifi_config->ssid, &ssid_len) < 0 ||
        aos_kv_get(KV_WIFI_PSK, wifi_config->password, &psw_len) < 0) {
        strcpy(wifi_config->ssid, "test");
        strcpy(wifi_config->password, "12345678");
    }
    #else
    strcpy(wifi_config->ssid, "test");
    strcpy(wifi_config->password, "12345678");    
    #endif

    LOGD(TAG, "wifi ssid{%s}, psk{%s}", wifi_config->ssid, wifi_config->password);
	hal_wifi_start(dev, wifi_config);
    aos_free(wifi_config);
}

static void wifihal_monitor()
{
    netmgr_hdl_t netmgr_hdl = netmgr_dev_wifi_init();
    aos_dev_t *dev = netmgr_get_dev(netmgr_hdl);

    hal_wifi_start_monitor(dev, wifi_monitor_rx);

    uint8_t channel = 1;
    int i = 30;

    while (i-- > 0) {
        aos_msleep(300);

        wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
        hal_wifi_set_channel(dev, channel, second);

        /** Print channel currently set*/
        uint8_t tmp_channel;
        wifi_second_chan_t tmp_second;
        hal_wifi_get_channel(dev, &tmp_channel, &tmp_second);
        LOGI(TAG, "Switch to Channel %d", tmp_channel);

        channel++;

        if (channel == 14) {
            channel = 1;
        }
    }
    LOGI(TAG, "Switch channel stops");

}

const char* help_str = "wifihal sta:sta mode\n"
    "wifihal scan:scan sta\n"
    "wifihal sta_ps:sta in ps mode\n"
    "wifihal ap:starting an ap\n"
    "wifihal sta_list:get assoc station list\n"
    "wifihal reset_test:reset wifi test\n"
    "wifihal monitor:start monitor mode\n\n>";

static void wifihalapp_cmd(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc < 2) {
        goto help;
    }
    if (strcmp(argv[1], "sta") == 0)
        wifihal_sta();
    else if (strcmp(argv[1], "scan") == 0) {
        wifihal_scan(); 
    } else if (strcmp(argv[1], "sta_ps") == 0)
        wifihal_sta_ps();
    else if (strcmp(argv[1], "ap") == 0)
        wifihal_ap();
    else if (strcmp(argv[1], "sta_list") == 0)
        wifihal_ap_sta_list();
    else if (strcmp(argv[1], "monitor") == 0)
        wifihal_monitor();
    else if (strcmp(argv[1], "reset_test") == 0)
        wifihal_reset_test();
    else
        goto help;

    return;
help:
    
    printf("Argument failed\n%s", help_str);

}

void app_wifi_init(void)
{
    event_subscribe(EVENT_NETMGR_GOT_IP, user_local_event_cb, NULL);
    event_subscribe(EVENT_NETMGR_NET_DISCON, user_local_event_cb, NULL);

    static const struct cli_command cmd_info = {
        "wifihal",
        "wifihal test",
        wifihalapp_cmd
    };
    aos_cli_register_command(&cmd_info);

    return;
}
