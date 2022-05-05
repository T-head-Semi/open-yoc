/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <yoc/atserver.h>
#include <at_cmd.h>
#include <cJSON.h>
#include <aos/kv.h>
#include <ulog/ulog.h>
#include <devices/wifi.h>
#include <devices/driver.h>
#include <devices/netdrv.h>
#include <w800_devops.h>
#include "at_internal.h"
#include "at_factory_test.h"

#define TAG "at_fct"

typedef struct {
    uint8_t ssid[MAX_SSID_SIZE + 1];
    int max_rssi;
    int min_rssi;
} wscan_param_st;

at_cmd_gpio_test at_cmd_gpio_test_cb = NULL;

void at_cmd_gpio_test_cb_reg(int (*cb)(void))
{
    at_cmd_gpio_test_cb = cb;
}

static wscan_param_st _g_wscan_param = {0};

static void wifi_cb_sta_connect_fail(aos_dev_t *dev, wifi_err_code_t err, void *arg)
{
    LOGI(TAG, "%s\n", __FUNCTION__);
}

static void wifi_cb_status_change(aos_dev_t *dev, wifi_event_id_t stat, void *arg)
{
    LOGI(TAG, "%s\n", __FUNCTION__);
}

static void print_encrypt_mode(wifi_encrypt_type_t encryptmode)
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
    int ret = -1;
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

        if (0 == strcmp(ap_record->ssid, _g_wscan_param.ssid)) {
            ret = 1;

            if (_g_wscan_param.max_rssi >= ap_record->rssi && _g_wscan_param.min_rssi <= ap_record->rssi) {
                ret = 0;
            }
        }
    }

    if (ret) {
        AT_BACK_ERRNO(ret);
    } else {
        AT_BACK_OK();
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

static void wifihal_scan()
{
    aos_dev_t *wifi_dev = device_find("wifi", 0);

    if (wifi_dev == NULL) {
        wifi_w800_register(NULL);
        wifi_dev = device_find("wifi", 0);
    }

    aos_dev_t *dev = wifi_dev;
    wifi_scan_config_t scan_config;

    memset(scan_config.ssid, 0, sizeof(scan_config.ssid));
    memset(scan_config.bssid, 0, sizeof(scan_config.bssid));
    scan_config.channel = 0;
    scan_config.show_hidden = 0;
    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    scan_config.scan_time.active.max = 200;
    scan_config.scan_time.active.min = 100;
    scan_config.scan_time.passive = 150;


    hal_wifi_install_event_cb(dev, &evt_cb);
    LOGI(TAG, "wifi block scan");
    hal_wifi_start_scan(dev, NULL, 1);
    LOGI(TAG, "wifi block scan done");
}


static void wifihal_connect_to_ap(const char *ssid, const char *psk)
{
    aos_dev_t *wifi_dev = device_find("wifi", 0);

    if (wifi_dev == NULL) {
        wifi_w800_register(NULL);
        wifi_dev = device_find("wifi", 0);
    }

    aos_dev_t *dev = wifi_dev;

    aos_kv_setstring("wifi_ssid", ssid);
    aos_kv_setstring("wifi_psk", psk);

    if (hal_wifi_reset(dev) < 0) {
        event_publish(EVENT_NETMGR_NET_DISCON, NULL);
        AT_BACK_ERRNO(-1);
        return;
    }

    wifi_config_t *wifi_config = aos_zalloc(sizeof(wifi_config_t));

    if (wifi_config == NULL) {
        AT_BACK_ERRNO(-1);
        return;
    }

    wifi_config->mode = WIFI_MODE_STA;
    strcpy(wifi_config->ssid, ssid);
    strcpy(wifi_config->password, psk);
    int ret = hal_wifi_start(dev, wifi_config);
    aos_free(wifi_config);

    if (ret == 0) {
        LOGI(TAG, "ssid{%s}, psk{%s}\n", ssid, psk);
    } else {
        LOGW(TAG, "no ap info");
        AT_BACK_ERRNO(-1);
    }
}


void user_at_wscan_handler(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int max_rssi, min_rssi;
        char *ssid = data;
        char *token = strchr(data, ',');

        if (token) {
            *token = '\0';
            token++;
        } else {
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        data = token;
        max_rssi = atoi(data);
        token = strchr(data, ',');

        if (token) {
            *token = '\0';
            token++;
        } else {
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        data = token;
        min_rssi = atoi(data);
        LOGI(TAG, "ssid: %s, max_rssi %d, min_rssi %d\n", ssid, max_rssi, min_rssi);
        _g_wscan_param.max_rssi = max_rssi;
        _g_wscan_param.min_rssi = min_rssi;
        strcpy(_g_wscan_param.ssid, ssid);
        wifihal_scan();
    } else {
        AT_BACK_ERRNO(AT_ERR_INVAL);
    }
}

static void event_app_connect_handler(uint32_t event_id, const void *param, void *context)
{
    LOGI(TAG, "===>connect success");

    atserver_send("+EVENT=%s,%s\r\n", "NET", "LINK_UP");
    AT_BACK_OK();
}

void user_at_wjoin_handler(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        char *ssid = data;
        char *token = strchr(data, ',');

        if (token) {
            *token = '\0';
            token++;
        } else {
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        event_subscribe(EVENT_WIFI_LINK_UP, event_app_connect_handler, NULL);

        wifihal_connect_to_ap(ssid, token);
    } else {
        AT_BACK_ERRNO(AT_ERR_INVAL);
    }
}

void user_at_gpio_handler(char *cmd, int type, char *data)
{
    int ret = -1;

    if (type == EXECUTE_CMD) {
        if (at_cmd_gpio_test_cb) {
            ret = at_cmd_gpio_test_cb();
        }

        if (ret == 0) {
            AT_BACK_OK();
        } else {
            AT_BACK_ERRNO(AT_ERR_STATUS);
        }
    } else {
        AT_BACK_ERRNO(AT_ERR_INVAL);
    }
}


void user_at_z_handler(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        AT_BACK_OK();
        aos_msleep(100);
        aos_reboot();
    } else {
        AT_BACK_ERRNO(AT_ERR_INVAL);
    }
}
