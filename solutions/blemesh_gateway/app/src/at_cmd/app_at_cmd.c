/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <yoc/atserver.h>
#include <at_cmd.h>
#include <wifi_provisioning.h>
#include <smartliving/exports/iot_export_linkkit.h>
#include <cJSON.h>
#include "app_main.h"
#include "../app_sys.h"
#include <at_mesh.h>
#include <ota_server.h>
#include "gateway/ota.h"
#include "devices/wifi.h"
#include "devices/driver.h"
#include "devices/netdrv.h"
#include "at_internal.h"
// #include "w800_devops.h"
#include "ota_server.h"
#ifdef CONFIG_BT_MESH_PROVISIONER
#include "inc/provisioner_main.h"
#endif


#define TAG "app"

extern cJSON *cJSON_GetObjectItemByPath(cJSON *object, const char *path);
extern void ymodem_mode_start(void);

extern int ymodem_ota_prepare(int ota_type, int protocol, int ota_channel);
extern int app_firmware_search(char *name);

typedef enum {
    APP_AT_CMD_IWS_START,
    APP_AT_CMD_WJAP,
} APP_AT_CMD_TYPE;

typedef struct at_aui_info {
    int micEn;
    cJSON *js_info;
} at_aui_info_t;

static int g_at_init = 0;
static at_aui_info_t g_at_aui_ctx = {1, NULL};

#define AT_RESP_CME_ERR(errno)    atserver_send("\r\n+CME ERROR:%d\r\n", errno)

#define  ADDR_MAC_SZIE       30
#define  MAX_MAC_FILTER_SIZE 6

static uint8_t dev_addr_str[ADDR_MAC_SZIE * MAX_MAC_FILTER_SIZE] = {0};


void event_publish_app_at_cmd(APP_AT_CMD_TYPE type)
{
    event_publish(EVENT_APP_AT_CMD, (void *)type);
}

/* remove double quote in first character and last character */
static char *remove_double_quote(char *str)
{
    char   *str_new_p = str;
    uint8_t len       = strlen(str);

    if ('\"' == *str) {
        str_new_p = str + 1;
    }

    if ('\"' == str[len - 1]) {
        str[len - 1] = '\0';
    }

    return str_new_p;
}

/* calculate number of '"', not include '\"' */
static uint8_t cal_quote_num(char *str)
{
    uint8_t cnt = 0;
    uint8_t last_slash = 0;

    while ('\0' != *str) {
        if ('\\' == *str) {
            last_slash = 1;
            str++;
            continue;
        }

        if ('\"' == *str && 0 == last_slash) {
            cnt++;
        }

        str++;
        last_slash = 0;
    }

    return cnt;
}

/* replace '"' with '\"',  don't touch ogirinal '\"' */
static void add_slash_to_quote(const char *oldStr, char *newStr)
{
    uint8_t last_slash = 0;

    while ('\0' != *oldStr) {
        if ('\\' == *oldStr) {
            last_slash = 1;
            *newStr = *oldStr;
        } else if ('\"' == *oldStr && 0 == last_slash) {
            *newStr++ = '\\';
            *newStr = '\"';
            last_slash = 0;
        } else {
            *newStr = *oldStr;
            last_slash = 0;
        }

        oldStr++;
        newStr++;
    }

    *newStr = '\0';
}

static void convert_json_and_send(char *cmd, cJSON *js_info)
{
    if (NULL == cmd || NULL == js_info) {
        return;
    }

    char *str_js = cJSON_PrintUnformatted(g_at_aui_ctx.js_info);
    uint8_t quote_num = cal_quote_num(str_js);
    char new_str_js[strlen(str_js) + quote_num + 1];
    add_slash_to_quote(str_js, new_str_js);

    atserver_send("%s:%s\r\n", cmd, new_str_js);
    AT_BACK_OK();
    free(str_js);
}

void iws_start_handler(char *cmd, int type, char *data)
{
    int input_data = 0;
    extern int wifi_prov_method;

    if (type == TEST_CMD) {
        AT_BACK_RET_OK(cmd, "\"type\"");
    } else if (type == READ_CMD) {
        AT_BACK_RET_OK_INT(cmd, wifi_prov_method);
    } else if (type == WRITE_CMD) {
        AT_BACK_OK();
        input_data = atoi(data);

        if ((input_data >= WIFI_PROVISION_MAX) || (input_data < WIFI_PROVISION_MIN)
            || (input_data == WIFI_PROVISION_SOFTAP)) {
            AT_RESP_CME_ERR(100);
        } else {
            wifi_prov_method = input_data;
            //event_publish_app_at_cmd(APP_AT_CMD_IWS_START);
            aos_kv_setint("wprov_method", wifi_prov_method);

            if (wifi_prov_method == WIFI_PROVISION_SL_BLE) {
                aos_kv_del("AUTH_AC_AS");
                aos_kv_del("AUTH_KEY_PAIRS");
            }

            app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
            aos_reboot();
        }
    } else {
        AT_RESP_CME_ERR(100);
    }
}

void at_cmd_stop_iwss(void *arg)
{
    if (wifi_prov_get_status() != WIFI_PROV_STOPED) {
        wifi_prov_stop();
    }

    aos_task_exit(0);
}

void iws_stop_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == EXECUTE_CMD) {
        aos_task_new("IWSSTOP", at_cmd_stop_iwss, NULL, 4 * 1024);
        AT_BACK_OK();
    }
}

void idm_au_handler(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        // "PK","DN","DS","PS"
        if (*data != '\"'
            || strlen(data) > PRODUCT_KEY_LEN + DEVICE_NAME_LEN
            + DEVICE_SECRET_LEN + PRODUCT_SECRET_LEN + 11) {
            AT_RESP_CME_ERR(50);
            return;
        }

        char *buffer[4];
        char *start = ++data;
        char *token = NULL;

        // PK
        token = strstr(start, "\",\"");

        if (!token || token - start > PRODUCT_KEY_LEN) {
            AT_RESP_CME_ERR(50);
            return;
        }

        *token = '\0';
        buffer[0] = start;

        // DN
        start = token + 3;
        token = strstr(start, "\",\"");

        if (!token || token - start > DEVICE_NAME_LEN) {
            AT_RESP_CME_ERR(50);
            return;
        }

        *token = '\0';
        buffer[1] = start;

        // DS
        start = token + 3;
        token = strstr(start, "\",\"");

        if (!token || token - start > DEVICE_SECRET_LEN) {
            AT_RESP_CME_ERR(50);
            return;
        }

        *token = '\0';
        buffer[2] = start;

        // PS
        start = token + 3;
        token = strstr(start, "\"");

        if (!token || token - start > PRODUCT_SECRET_LEN) {
            AT_RESP_CME_ERR(50);
            return;
        }

        *token = '\0';
        buffer[3] = start;

        HAL_SetProductKey(buffer[0]);
        HAL_SetDeviceName(buffer[1]);
        HAL_SetDeviceSecret(buffer[2]);
        HAL_SetProductSecret(buffer[3]);
        AT_BACK_OK();
    } else if (type == READ_CMD) {
        char pk[PRODUCT_KEY_LEN + 1];
        char dn[DEVICE_NAME_LEN + 1];
        char ds[DEVICE_SECRET_LEN + 1];
        char ps[PRODUCT_SECRET_LEN + 1];
        HAL_GetProductKey(pk);
        HAL_GetDeviceName(dn);
        HAL_GetDeviceSecret(ds);
        HAL_GetProductSecret(ps);
        atserver_send("+IDMAU:%s,%s,%s,%s\r\nOK\r\n", pk, dn, ds, ps);
    }
}
#if defined(EN_COMBO_NET) && (EN_COMBO_NET == 1)
void idm_pid_handler(char *cmd, int type, char *data)
{
    extern int g_combo_pid;

    if (type == TEST_CMD) {
        AT_BACK_RET_OK(cmd, "\"pid\"");
    } else if (type == READ_CMD) {
        aos_kv_getint("hal_devinfo_pid", &g_combo_pid);
        AT_BACK_RET_OK_INT(cmd, g_combo_pid);
    } else if (type == WRITE_CMD) {
        AT_BACK_OK();
        g_combo_pid = atoi(data);
        aos_kv_setint("hal_devinfo_pid", g_combo_pid);
    } else {
        AT_RESP_CME_ERR(100);
    }
}
#endif

void idm_con_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == EXECUTE_CMD) {
        smartliving_client_control(1);
        AT_BACK_OK();
    }
}

void idm_cls_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == EXECUTE_CMD) {
        smartliving_client_control(0);
        AT_BACK_OK();
    }
}

void idm_sta_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == READ_CMD) {
        AT_BACK_RET_OK_INT(cmd, smartliving_client_is_connected() << 1);
    }
}

void at_cmd_reinit_network(void *arg)
{
    app_network_reinit();

    aos_task_exit(0);
}

static void event_app_at_cmd_handler(uint32_t event_id, const void *param, void *context)
{
    if (event_id == EVENT_APP_AT_CMD) {
        APP_AT_CMD_TYPE type = (APP_AT_CMD_TYPE)param;

        switch (type) {
        case APP_AT_CMD_IWS_START:
            smartliving_client_control(0);
            wifi_pair_start();
            break;

        case APP_AT_CMD_WJAP:
            aos_task_new("WJAP", at_cmd_reinit_network, NULL, 4 * 1024);
            break;

        default:
            break;
        }
    }
}

void at_cmd_dev_info(char *cmd, int type, char *data)
{
    int ret;

    if (type == TEST_CMD) {
        AT_BACK_STR("+DEVINFO:\"str\"\r\n");
    } else if (type == READ_CMD) {
        char devInfo[256] = {0};
        ret                        = aos_kv_getstring("devInfo", devInfo, sizeof(devInfo));
        AT_BACK_RET_OK(cmd, devInfo);
    } else if (type == WRITE_CMD) {
        data = remove_double_quote(data);
        aos_kv_setstring("devInfo", data);
        AT_BACK_OK();
    }
}

void at_cmd_aui_cfg(char *cmd, int type, char *data)
{
    //int ret;
    if (type == TEST_CMD) {
        AT_BACK_STR("+AUICFG:\"per,vol,spd,pit\"\r\n");
    } else if (type == READ_CMD) {
        char auiCfg[32] = {0};
        aos_kv_getstring("auiCfg", auiCfg, sizeof(auiCfg));
        AT_BACK_RET_OK(cmd, auiCfg);
    } else if (type == WRITE_CMD) {
        data = remove_double_quote(data);
        aos_kv_setstring("auiCfg", data);
        AT_BACK_OK();
    }
}

void at_cmd_aui_fmt(char *cmd, int type, char *data)
{
    int auiFmt = 0;

    if (type == TEST_CMD) {
        AT_BACK_STR("+AUIFMT:<fmt>\r\n");
    } else if (type == READ_CMD) {
        aos_kv_getint("auiFmt", &auiFmt);
        AT_BACK_RET_OK_INT(cmd, auiFmt);
    } else if (type == WRITE_CMD) {
        data = remove_double_quote(data);
        auiFmt = atoi(data);
        aos_kv_setint("auiFmt", auiFmt);
        AT_BACK_OK();
    }
}

void at_cmd_aui_micen(char *cmd, int type, char *data)
{
#ifdef NEVER

    if (type == TEST_CMD) {
        AT_BACK_STR("+AUIMICEN:\"val\"\r\n");
    } else if (type == READ_CMD) {
        AT_BACK_RET_OK_INT(cmd, g_at_aui_ctx.micEn);
    } else if (type == WRITE_CMD) {
        data = remove_double_quote(data);
        g_at_aui_ctx.micEn = atoi(data);
        aui_mic_set_wake_enable(g_at_aui_ctx.micEn);
        AT_BACK_OK();
    }

#endif /* NEVER */
}

void at_cmd_wwv_en(char *cmd, int type, char *data)
{
    int auiWWVen = 0;

    if (type == TEST_CMD) {
        AT_BACK_STR("+AUIWWVEN:<en>\r\n");
    } else if (type == READ_CMD) {
        aos_kv_getint("auiWWVen", &auiWWVen);
        AT_BACK_RET_OK_INT(cmd, auiWWVen);
    } else if (type == WRITE_CMD) {
        data = remove_double_quote(data);
        auiWWVen = atoi(data);
        aos_kv_setint("auiWWVen", auiWWVen);
        AT_BACK_OK();
    }
}

void at_cmd_aui_kws(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    }
}

void at_cmd_aui_ctrl(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == READ_CMD) {
        if (NULL != g_at_aui_ctx.js_info) {
            convert_json_and_send("+AUICTRL", g_at_aui_ctx.js_info);
        } else {
            atserver_send("+AUICTRL:\r\n");
            AT_BACK_OK();
        }
    }
}

void wjap_handler(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        char *token = strchr(data, ',');
        int ret = -1;

        if (token) {
            *token = '\0';
            int len = token - data;

            if (len <= 32 && len > 0) {
                ++token;

                if (strlen(token) <= 64) {
                    aos_kv_setstring("wifi_ssid", data);
                    aos_kv_setstring("wifi_psk", token);
                    ret = 0;
                    event_publish_app_at_cmd(APP_AT_CMD_WJAP);
                }
            }
        }

        if (ret) {
            AT_RESP_CME_ERR(50);
        } else {
            AT_BACK_OK();
        }
    }
}

void wjapd_handler(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        extern int wifi_reset();
        wifi_reset();
        AT_BACK_OK();
    }
}

void wjapq_handler(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        char ssid[33];
        char psk[65];
        int rst = aos_kv_getstring("wifi_ssid", ssid, sizeof(ssid));

        if (rst >= 0) {
            rst = aos_kv_getstring("wifi_psk", psk, sizeof(psk));
        }

        if (rst < 0) {
            AT_BACK_ERR();
        } else {
            AT_BACK_RET_OK2(cmd, ssid, psk);
        }
    }
}

void idm_pp_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_RET_OK(cmd, "\"device_id\",\"message\"");
    } else if (type == WRITE_CMD) {
        if (strncmp(data, "0,\"", 3) != 0) {
            AT_RESP_CME_ERR(50);
            return;
        }

        char *msg = data + 3;
        int len = strlen(msg);

        if (msg == NULL || len == 0 || msg[len - 1] != '\"') {
            AT_RESP_CME_ERR(50);
            return;
        }

        msg[len - 1] = '\0';

        char *str = msg;
        int count = 0;

        while (str && (str = strstr(str, "\\\"")) != NULL) {
            *str = ' ';
            str += 2;

            if (!str || (str = strstr(str, "\\\"")) == NULL) {
                AT_RESP_CME_ERR(50);
                return;
            }

            *str++ = '\"';
            *str++ = ' ';
            ++count;
        }

        if (count == 0) {
            AT_RESP_CME_ERR(50);
            return;
        }

        cJSON *root = cJSON_Parse(msg);

        if (root == NULL) {
            AT_RESP_CME_ERR(50);
            return;
        }

        cJSON_Delete(root);

        int rst = user_at_post_property(0, msg, strlen(msg));

        if (rst > 0) {
            AT_BACK_RET_OK_INT(cmd, rst);
        } else {
            AT_RESP_CME_ERR(100);
        }
    }
}

void idm_ps_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    }
}

void idm_ep_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_RET_OK(cmd, "\"device_id\",\"event_id\",\"event_payload\"");
    } else if (type == WRITE_CMD) {
        if (strncmp(data, "0,\"", 3) != 0) {
            AT_RESP_CME_ERR(50);
            return;
        }

        char *evt_id = data + 3;
        char *payload = strstr(evt_id, "\",\"");

        if (payload == NULL || payload == evt_id || payload[strlen(payload) - 1] != '\"') {
            AT_RESP_CME_ERR(50);
            return;
        }

        payload[strlen(payload) - 1] = '\0';
        *payload = '\0';
        payload += 3;

        char *str = payload;
        int count = 0;

        while (str && (str = strstr(str, "\\\"")) != NULL) {
            *str = ' ';
            str += 2;

            if (!str || (str = strstr(str, "\\\"")) == NULL) {
                AT_RESP_CME_ERR(50);
                return;
            }

            *str++ = '\"';
            *str++ = ' ';
            ++count;
        }

        if (count == 0) {
            AT_RESP_CME_ERR(50);
            return;
        }

        cJSON *root = cJSON_Parse(payload);

        if (root == NULL) {
            AT_RESP_CME_ERR(50);
            return;
        }

        cJSON_Delete(root);

        int rst = user_at_post_event(0, evt_id, strlen(evt_id), payload, strlen(payload));

        if (rst >= 0) {
            AT_BACK_RET_OK_INT(cmd, rst);
        } else {
            AT_RESP_CME_ERR(100);
        }
    }
}

void app_at_cmd_property_report_set(const char *msg, const int len)
{
    atserver_send("+IDMPS:0,%d,%.*s\r\nOK\r\n", len, len, msg);
}

void app_at_cmd_property_report_reply(const int packet_id, const int code, const char *reply, const int len)
{
    if (len) {
        atserver_send("+IDMPP:0,%d,%d,%d,%.*s\r\n", packet_id, code, len, len, reply);
    } else {
        atserver_send("+IDMPP:0,%d,%d\r\n", packet_id, code);
    }
}

void app_at_cmd_event_report_reply(const int packet_id, const int code, const char *evtid, const int evtid_len, const char *reply, const int len)
{
    if (len) {
        atserver_send("+IDMEP:0,%d,%d,%.*s,%d,%.*s\r\n", packet_id, code, evtid_len, evtid, len, len, reply);
    } else {
        atserver_send("+IDMEP:0,%d,%d,%.*s\r\n", packet_id, code, evtid_len, evtid);
    }
}

void app_at_cmd_sta_report()
{
    atserver_send("+IDMSTA:%d\r\nOK\r\n", smartliving_client_is_connected() << 1);
}


void user_at_add_ota_fw_handler(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret =  0;
        int32_t ota_type = 0;
        int32_t ota_protocol = 0;
        int32_t ota_channel = 0;
        int16_t input_num = 0;

        input_num = atserver_scanf("%x,%x,%x", &ota_type,&ota_protocol,&ota_channel);
        if(ota_type == OTA_FOR_BLE_NODE) {
            if(input_num < 2) {
                ota_protocol =  AIS_PROTOCOL;
                ota_channel  =  OTA_BY_GATT;
            } else if(input_num < 3) {
                if(ota_protocol == AIS_PROTOCOL) {
                    ota_channel  =  OTA_BY_GATT;
                } else if(ota_protocol == MESH_OTA_PROTOCOL) {
                    ota_channel = OTA_BY_MESH_EXT_2M;
                } else {
                    LOGE(TAG,"Unsupport protocol %02x",ota_protocol);
                    AT_BACK_ERRNO(AT_ERR_INVAL);
                    return;
                }
            } else {
                if(ota_protocol == AIS_PROTOCOL && (ota_channel == OTA_BY_MESH_EXT_1M || ota_channel == OTA_BY_MESH_EXT_2M || ota_channel == OTA_BY_MESH_LEGACY) ||  \
                   ota_protocol == MESH_OTA_PROTOCOL && (ota_channel == OTA_BY_GATT || ota_channel == OTA_BY_UART || ota_channel == OTA_BY_HCI_UART)                                                                                                                                               ) {
                    LOGE(TAG,"Unsupport ota channel %02x for protocol %02x",ota_channel,ota_protocol);
                    AT_BACK_ERRNO(AT_ERR_INVAL);
                    return;
                }
            }
        }

        ret = ymodem_ota_prepare(ota_type, ota_protocol, ota_channel);

        if (ret < 0) {
            AT_BACK_ERR();
            return;
        }

        ymodem_mode_start();
        AT_BACK_OK();
    } else {
        AT_BACK_RET_OK(cmd, "<ota_type>,[protocol],[channel]");
    }
}


void user_get_ota_fw_index(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret =  0;
        int16_t index = -1;

        if (!data) {
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        index = app_firmware_search(data);

        if (index < 0) {
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        } else {
            AT_BACK_OK();
            atserver_send("\r\n+ADDOTAFW:%d\r\n", index);
        }
    } else {
        AT_BACK_RET_OK(cmd, "<firmware_name>");
    }
}


void user_at_rm_ota_firmware_handler(char *cmd, int type, char *data)
{
    int ota_type = 0;
    int img_idx = 0;


    if (type == WRITE_CMD) {
        int32_t index;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x", &index);

        if (input_num < 1) {
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        ret = ota_server_upgrade_firmware_rm(index);

        if (ret < 0) {
            AT_BACK_ERRNO(ret);
            return;
        } else {
            AT_BACK_OK();
        }
    } else {
        AT_BACK_RET_OK(cmd, "<index>");
    }
}

#define ERR_CHAR 0XFF

static uint8_t char2u8(char c)
{
    if (c >= '0' && c <= '9') {
        return (c - '0');
    } else if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    } else {
        return ERR_CHAR;
    }
}

static int str2bt_dev_addr(const char *str, mac_t *dev)
{
    uint8_t i, j;
    uint8_t tmp;

    if (strlen(str) != 19 || !dev) {
        return -EINVAL;
    }

    for (i = 5, j = 1; j <= 17 ; str++, j++) {
        if (!(j % 3) && (*str != ':')) {
            return -EINVAL;
        } else if (*str == ':') {
            i--;
            continue;
        }

        dev->val[i] = dev->val[i] << 4;

        tmp = char2u8(*str);

        if (tmp == ERR_CHAR) {
            return -EINVAL;
        }

        dev->val[i] |= tmp;

    }

    str++;

    dev->type = char2u8(*str);

    if (dev->type != 0 &&  dev->type != 1) {
        return -EINVAL;
    }

    return 0;
}

static int grep_mac_addr_from_str(char *mac_str, device_info *devices, uint8_t size)
{
    uint8_t dev_index = 0;
    char mac_str_temp[20] = {0x0};
    uint16_t str_size = strlen(mac_str);

    int ret = 0;

    for (dev_index = 0 ; dev_index < size && mac_str != '\0'; dev_index++) {
        memcpy(mac_str_temp, mac_str, 19);
        ret = str2bt_dev_addr(mac_str_temp, &devices[dev_index].addr);
        if (ret) {
            return -1;
        }

        mac_str += 20;
    }

    if (dev_index != size) {
        return -1;
    } else {
        return 0;
    }
}

static int grep_unicast_addr_from_str(char *unicast_str, device_info *devices, uint8_t size)
{
    uint8_t dev_index = 0;
    char unicast_str_temp[7] = {0x0};
    uint16_t str_size = strlen(unicast_str);
    uint8_t temp_length = 0;
    char* temp_head = NULL;
    int ret = 0;

    for (dev_index = 0 ; dev_index < size; dev_index++) {
        temp_head = unicast_str;
        temp_length = 0;
        while (*unicast_str != '\0' && *unicast_str != ',') {
            unicast_str++;
            temp_length++;
        }
        if(*unicast_str == ',') {
            unicast_str++;
        }
        if(temp_length == 0) {
            return dev_index;
        } else if(temp_length > 6) {
            return -EINVAL;
        }
        memcpy(unicast_str_temp, temp_head, temp_length);
        devices[dev_index].unicast_addr = strtoul(temp_head, NULL, 16);
#ifdef CONFIG_BT_MESH_PROVISIONER
			struct bt_mesh_node_t * node = bt_mesh_provisioner_get_node_info(devices[dev_index].unicast_addr);
			if(!node) {
				LOGE(TAG, "Not found the node %04x in the provisioner", devices[dev_index].unicast_addr);
				return -EINVAL;
			} else {
				memcpy(devices[dev_index].addr.val, node->addr_val, 6);
				devices[dev_index].addr.type = node->addr_type;
				devices[dev_index].old_version = node->version;
			}
#endif

    }

    return dev_index;
}



void at_report_firmware_index(int index)
{
    aos_msleep(200);
    atserver_send("\r\n+ADDOTAFW:%d\r\n", index);
}

static inline int addr_val_to_str(mac_t addr, char *str, size_t len)
{
    return snprintf(str, len, "%02X:%02X:%02X:%02X:%02X:%02X,%X", addr.val[5], addr.val[4], addr.val[3], addr.val[2], addr.val[1], addr.val[0], addr.type);
}

void at_report_ota_status(uint16_t index, uint8_t status, mac_t addr, char *version)
{
    char addr_str[40] = {0};
    addr_val_to_str(addr, addr_str, sizeof(addr_str));
    atserver_send("\r\n+DEVOTA:%02x,%02x,%s,%s\r\n", index, status, addr_str, version);
}

void at_report_fw_status(uint8_t index, uint8_t status)
{
    atserver_send("\r\n+FW:%02x,%02x\r\n", index, status);
}

void user_at_add_ota_node_handler(char *cmd, int type, char *data)
{
    int ota_type = 0;
    int img_idx = 0;


    if (type == WRITE_CMD) {
        int32_t size;
        int32_t index;
        int16_t input_num;
        device_info *device = NULL;
        int16_t ret;
        input_num = atserver_scanf("%x,%x,%[^\n]", &index, &size, dev_addr_str);

        if (input_num < 3) {
            LOGE(TAG, "Err input num %d", input_num);
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        if (size > MAX_MAC_FILTER_SIZE || !size || size < 0) {
            LOGE(TAG, "Err input arg");
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        if (strlen(dev_addr_str) != (19 * size + size - 1)) {
            LOGE(TAG, "Err input addr str");
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        device = (device_info *)aos_zalloc(size * sizeof(device_info));

        if (!device) {
            LOGE(TAG, "Malloc devices faild");
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        ret = grep_mac_addr_from_str(dev_addr_str, device, size);

        if (ret < 0) {
            LOGE(TAG, "Grep addr faild");
            aos_free(device);
            AT_BACK_ERRNO(ret);
            return;
        }

        ret = ota_server_upgrade_device_add(index, size, device);

        if (ret < 0) {
            LOGE(TAG, "Add ota device faild %d", ret);
            aos_free(device);
            AT_BACK_ERRNO(ret);
            return;
        } else {
            AT_BACK_OK();
        }
    } else {
        AT_BACK_RET_OK(cmd, "<index>,<size>,<mac>,<type>...");
    }
}

void user_at_rm_ota_node_handler(char *cmd, int type, char *data)
{
    int ota_type = 0;
    int img_idx = 0;


    if (type == WRITE_CMD) {
        int32_t size;
        int32_t index;
        int16_t input_num;
        device_info *device = NULL;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%[^\n]", &index, &size, dev_addr_str);

        if (input_num < 3) {
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        if (size > MAX_MAC_FILTER_SIZE || !size || size < 0) {
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        if (strlen(dev_addr_str) != (19 * size + size - 1)) {
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        device = (device_info *)aos_zalloc(size * sizeof(device_info));

        if (!device) {
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        ret = grep_mac_addr_from_str(dev_addr_str, device, size);

        if (ret < 0) {
            aos_free(device);
            AT_BACK_ERRNO(ret);
            return;
        }

        ret = ota_server_upgrade_device_rm(index, size, device);

        if (ret < 0) {
            aos_free(device);
            AT_BACK_ERRNO(ret);
            return;
        } else {
            AT_BACK_OK();
        }
    } else {
        AT_BACK_RET_OK(cmd, "<index>,<size>,<mac>,<type>...");
    }
}



void user_at_start_ota_handler(char *cmd, int type, char *data)
{
    int ota_type = 0;
    int img_idx = 0;


    if (type == WRITE_CMD) {
        int err = 0;
        int32_t size;
        int32_t index;
        int16_t input_num;
        ota_firmware *firmware = NULL;
        int16_t ret;

        input_num = atserver_scanf("%x", &index);

        if(input_num < 1) {
            AT_BACK_ERRNO(AT_ERR_INVAL);
        }

        firmware = ota_server_upgrade_firmware_get(index);
        if(!firmware) {
            LOGE(TAG,"No firmware found for index %d",index);
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        err =	ota_server_upgrade_start(index);
        if (err) {
            LOGE(TAG, "Ota dev start faild %d", err);
            AT_BACK_ERRNO(ret);
        } else {
            LOGD(TAG, "Ota dev start success for image %d ", index);
            AT_BACK_OK();
        }

        return;
    } else {
        AT_BACK_RET_OK(cmd, "<index>");
    }
}

void user_at_add_ota_node_by_unicast_addr_handler(char *cmd, int type, char *data)
{
    int ota_type = 0;
    int img_idx = 0;


    if (type == WRITE_CMD) {
        int32_t size;
        int32_t index;
        int16_t input_num;
        device_info *device = NULL;
        int16_t ret;
        int set_size = 0;
        input_num = atserver_scanf("%x,%x,%[^\n]", &index, &size, dev_addr_str);
        if (input_num < 3) {
            LOGE(TAG, "Err input num %d", input_num);
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        device = (device_info *)aos_zalloc(size * sizeof(device_info));
        if (!device) {
            LOGE(TAG, "Malloc devices faild");
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        set_size = grep_unicast_addr_from_str(dev_addr_str,device,size);
        if(set_size < 0) {
            LOGE(TAG, "Grep device unicast addr faild");
            aos_free(device);
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        ret = ota_server_upgrade_device_add(index, (set_size < size ? set_size : size), device);

        if (ret < 0) {
            LOGE(TAG, "Add ota device faild %d", ret);
            aos_free(device);
            AT_BACK_ERRNO(ret);
            return;
        } else {
            AT_BACK_OK();
        }
    } else {
        AT_BACK_RET_OK(cmd, "<index>,<size>,<unicast_addr>...");
    }
}

void user_at_rm_ota_node_by_unicast_addr_handler(char *cmd, int type, char *data)
{
    int ota_type = 0;
    int img_idx = 0;


    if (type == WRITE_CMD) {
        int32_t size;
        int32_t index;
        int16_t input_num;
        device_info *device = NULL;
        int16_t ret;
        int set_size = 0;
        input_num = atserver_scanf("%x,%x,%[^\n]", &index, &size, dev_addr_str);

        if (input_num < 3) {
            LOGE(TAG, "Err input num %d", input_num);
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        device = (device_info *)aos_zalloc(size * sizeof(device_info));
        if (!device) {
            LOGE(TAG, "Malloc devices faild");
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        set_size = grep_unicast_addr_from_str(dev_addr_str,device,size);
        if(set_size < 0) {
            LOGE(TAG, "Grep device unicast addr faild");
            aos_free(device);
            AT_BACK_ERRNO(AT_ERR_INVAL);
            return;
        }

        ret = ota_server_upgrade_device_rm(index, (set_size < size ? set_size : size), device);

        if (ret < 0) {
            LOGE(TAG, "Add ota device faild %d", ret);
            aos_free(device);
            AT_BACK_ERRNO(ret);
            return;
        } else {
            AT_BACK_OK();
        }
    } else {
        AT_BACK_RET_OK(cmd, "<index>,<size>,<unicast_addr>...");
    }
}


void user_gateway_rst(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        AT_BACK_OK();
        extern void app_restore_factory_setting();
        app_restore_factory_setting();
    }
}


/********************************************************************************/
/* for fct test start
********************************************************************************/
typedef struct {
    uint8_t ssid[MAX_SSID_SIZE + 1];
    int max_rssi;
    int min_rssi;
} wscan_param_st;

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


static void user_at_wscan_handler(char *cmd, int type, char *data)
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

static void user_at_wjoin_handler(char *cmd, int type, char *data)
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

#define PSRAM_TEST_VALUE 0xA5

static int psram_test(void)
{
    uint32_t addr = 0x30000000;
    int i = 0;

    memset((uint8_t *)addr, PSRAM_TEST_VALUE, 0x1000);

    for (i = 0; i < 0x1000; i++) {
        if (*(uint8_t *)(addr + i) != PSRAM_TEST_VALUE) {
            LOGI(TAG, "read val %#x", *(uint8_t *)(addr + i));
            return -1;
        }
    }

    addr = 0x30400000;
    memset((uint8_t *)addr, PSRAM_TEST_VALUE, 0x1000);

    for (i = 0; i < 0x1000; i++) {
        if (*(uint8_t *)(addr + i) != PSRAM_TEST_VALUE) {
            LOGI(TAG, "read val %#x", *(uint8_t *)(addr + i));
            return -1;
        }
    }

    addr = 0x307FF000;
    memset((uint8_t *)addr, PSRAM_TEST_VALUE, 0x1000);

    for (i = 0; i < 0x1000; i++) {
        if (*(uint8_t *)(addr + i) != PSRAM_TEST_VALUE) {
            LOGI(TAG, "read val %#x", *(uint8_t *)(addr + i));
            return -1;
        }
    }

    return 0;
}

static void user_at_psram_handler(char *cmd, int type, char *data)
{
    int ret = 0;

    if (type == EXECUTE_CMD) {
        ret = psram_test();

        if (ret == 0) {
            AT_BACK_OK();
        } else {
            AT_BACK_ERRNO(AT_ERR_STATUS);
        }
    } else {
        AT_BACK_ERRNO(AT_ERR_INVAL);
    }
}



static int gpio_test(void)
{
    gpio_pin_handle_t     handle1, handle2;

    drv_pinmux_config(PA1, PIN_FUNC_GPIO);
    drv_pinmux_config(PA4, PIN_FUNC_GPIO);
    handle1 = csi_gpio_pin_initialize(PA1, NULL);
    handle2 = csi_gpio_pin_initialize(PA4, NULL);

    if ((handle1 == NULL) || (handle2 == NULL)) {
        LOGE(TAG, "gpio init fail");
        return -1;
    }

    csi_gpio_pin_config_mode(handle1, GPIO_MODE_PUSH_PULL);
    csi_gpio_pin_config_direction(handle1, GPIO_DIRECTION_OUTPUT);
    csi_gpio_pin_write(handle1, 1);

    aos_msleep(100);

    bool bval = false;
    csi_gpio_pin_config_mode(handle2, GPIO_MODE_PULLNONE);
    csi_gpio_pin_config_direction(handle2, GPIO_DIRECTION_INPUT);
    csi_gpio_pin_read(handle2, &bval);

    if (bval != 1) {
        LOGE(TAG, "gpio read err");
        return -1;
    }

    csi_gpio_pin_write(handle1, 0);
    aos_msleep(100);
    csi_gpio_pin_read(handle2, &bval);

    if (bval != 0) {
        LOGE(TAG, "gpio read err");
        return -1;
    }

    csi_gpio_pin_uninitialize(handle1);
    csi_gpio_pin_uninitialize(handle2);

    drv_pinmux_config(PB9, PIN_FUNC_GPIO);
    drv_pinmux_config(PB10, PIN_FUNC_GPIO);
    handle1 = csi_gpio_pin_initialize(PB9, NULL);
    handle2 = csi_gpio_pin_initialize(PB10, NULL);

    if ((handle1 == NULL) || (handle2 == NULL)) {
        LOGE(TAG, "gpio init fail");
        return -1;
    }

    csi_gpio_pin_config_mode(handle1, GPIO_MODE_PUSH_PULL);
    csi_gpio_pin_config_direction(handle1, GPIO_DIRECTION_OUTPUT);
    csi_gpio_pin_write(handle1, 1);

    aos_msleep(100);

    bval = false;
    csi_gpio_pin_config_mode(handle2, GPIO_MODE_PULLNONE);
    csi_gpio_pin_config_direction(handle2, GPIO_DIRECTION_INPUT);
    csi_gpio_pin_read(handle2, &bval);

    if (bval != 1) {
        LOGE(TAG, "gpio read err");
        return -1;
    }

    csi_gpio_pin_write(handle1, 0);
    aos_msleep(100);
    csi_gpio_pin_read(handle2, &bval);

    if (bval != 0) {
        LOGE(TAG, "gpio read err");
        return -1;
    }

    csi_gpio_pin_uninitialize(handle1);
    csi_gpio_pin_uninitialize(handle2);

    return 0;
}

static void user_at_gpio_handler(char *cmd, int type, char *data)
{
    int ret = 0;

    if (type == EXECUTE_CMD) {
        ret = gpio_test();

        if (ret == 0) {
            AT_BACK_OK();
        } else {
            AT_BACK_ERRNO(AT_ERR_STATUS);
        }
    } else {
        AT_BACK_ERRNO(AT_ERR_INVAL);
    }
}


static void user_at_z_handler(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        AT_BACK_OK();
        aos_msleep(100);
        aos_reboot();
    } else {
        AT_BACK_ERRNO(AT_ERR_INVAL);
    }
}

/********************************************************************************/
/* for fct test end
********************************************************************************/

const atserver_cmd_t at_cmd[] = {
    AT,
    AT_HELP,
    AT_CGMR,
    AT_FWVER,
    AT_SYSTIME,
    AT_SAVE,
    AT_FACTORYW,
    AT_FACTORYR,
    AT_REBOOT,
    AT_EVENT,
    AT_ECHO,
#ifdef CONFIG_YOC_LPM
    AT_SLEEP,
#endif
    AT_MODEL,
    AT_KVGET,
    AT_KVSET,
    AT_KVDEL,
    AT_KVGETINT,
    AT_KVSETINT,
    AT_KVDELINT,

    AT_CIPSTART,
    AT_CIPSTOP,
    AT_CIPRECVCFG,
    AT_CIPID,
    AT_CIPSTATUS,
    AT_CIPSEND,
#ifdef CONFIG_YOC_LPM
    AT_CIPSENDPSM,
#endif
    AT_CIPRECV,
    AT_BTMESH_LOG_LEVEL,
    AT_BTMESH_AT_EN,
    AT_BTMESH_RST_DEV,
    AT_BTMESH_QUERY_STA,
    AT_BTMESH_PROV_CONFIG,
    AT_BTMESH_PROV_AUTO_PROV,
    AT_BTMESH_PROV_EN,
    AT_BTMESH_APPKEY_SET,
    AT_BTMESH_CLEAR_RPL,
    AT_BTMESH_PROV_FILTER_DEV,
    AT_BTMESH_PROV_SHOW_DEV,
    AT_BTMESH_ADDDEV,
    AT_BTMESH_DELDEV,
    AT_BTMESH_OOB,
    AT_BTMESH_AUTOCONFIG,
    AT_BTMESH_GET_NODE_VERSION,
    AT_BTMESH_ADD_NODE,
    AT_BTMESH_GET_NODE_INFO,
    AT_BTMESH_CFG_COMP_GET,
    AT_BTMESH_NETKEY_GET,
    AT_BTMESH_NETKEY_SET,
    AT_BTMESH_APPKEY_GET,
    AT_BTMESH_APPKEY_ADD,
    AT_BTMESH_APPKEY_BIND,
    AT_BTMESH_APPKEY_UNBIND,
    AT_BTMESH_CFG_RELAY,
    AT_BTMESH_CFG_PROXY,
    AT_BTMESH_CFG_FRIEND,
    AT_BTMESH_SUB_GET,
    AT_BTMESH_SUB_SET,
    AT_BTMESH_SUB_DEL,
    AT_BTMESH_SUBLIST_OVERWRITE,
    AT_BTMESH_PUB_GET,
    AT_BTMESH_PUB_SET,
    AT_BTMESH_RST,
    AT_BTMESH_ONOFF,
    AT_BTMESH_LEVEL,
    AT_BTMESH_LEVEL_MOVE,
    AT_BTMESH_LEVEL_DELTA,
    AT_BTMESH_LIGHTNESS_RANGE,
    AT_BTMESH_LIGHTNESS_DEF,
    AT_BTMESH_LIGHTNESS,
    AT_BTMESH_LIGHTNESS_LIN,
    AT_BTMESH_LIGHT_CTL_RANGE,
    AT_BTMESH_LIGHT_CTL_DEF,
    AT_BTMESH_LIGHT_CTL,
    AT_BTMESH_LIGHT_CTL_TEMP,
    AT_BTMESH_TRS,
    AT_BTMESH_PROV_FILTER_MAC,
    AT_BTMESH_PROV_FILTER_MAC_ADD,
    AT_BTMESH_PROV_FILTER_MAC_RM,
    AT_BTMESH_PROV_FILTER_MAC_CLEAR,
#ifdef CONFIG_MESH_LPM
    AT_BTMESH_PROV_SET_NODE_LPM_FLAG,
#endif

    {"AT+IDMAU", idm_au_handler},
#if defined(EN_COMBO_NET) && (EN_COMBO_NET == 1)
    {"AT+IDMPID", idm_pid_handler},
#endif
    {"AT+IWSSTART", iws_start_handler},
    {"AT+IWSSTOP", iws_stop_handler},
    {"AT+DEVINFO", at_cmd_dev_info},
    {"AT+AUICFG", at_cmd_aui_cfg},
    {"AT+AUIFMT", at_cmd_aui_fmt},
    {"AT+AUIMICEN", at_cmd_aui_micen},
    {"AT+AUIWWVEN", at_cmd_wwv_en},
    {"AT+AUIKWS", at_cmd_aui_kws},
    {"AT+AUICTRL", at_cmd_aui_ctrl},
    {"AT+WJAP", wjap_handler},
    {"AT+WJAPD", wjapd_handler},
    {"AT+WJAPQ", wjapq_handler},
    {"AT+IDMCON", idm_con_handler},
    {"AT+IDMCLS", idm_cls_handler},
    {"AT+IDMSTA", idm_sta_handler},
    {"AT+IDMPP", idm_pp_handler},
    {"AT+IDMPS", idm_ps_handler},
    {"AT+IDMEP", idm_ep_handler},
    {"AT+ADDOTAFW", user_at_add_ota_fw_handler},
    {"AT+GETOTAFWID", user_get_ota_fw_index},
    {"AT+ADDOTANODE", user_at_add_ota_node_handler},
    {"AT+RMOTANODE", user_at_rm_ota_node_handler},
    {"AT+ADDOTANODEBYUNICAST", user_at_add_ota_node_by_unicast_addr_handler},
    {"AT+RMOTANODEBYUNICAST", user_at_rm_ota_node_by_unicast_addr_handler},
    {"AT+OTASTART", user_at_start_ota_handler},
    {"AT+RMOTAFW", user_at_rm_ota_firmware_handler},
    {"AT+WSCAN", user_at_wscan_handler},
    {"AT+WJOIN", user_at_wjoin_handler},
    {"AT+TPSRAM", user_at_psram_handler},
    {"AT+TGPIO", user_at_gpio_handler},
    {"AT+Z", user_at_z_handler},
    {"AT+GWRST",user_gateway_rst},
    // TODO:
    AT_NULL,
};




void app_at_kws_notify(uint8_t notify)
{
    /*
        AT协议为 --->"+AUIKWS=1\r\n"
    */
    if (0 == g_at_init) {
        return;
    }

    atserver_send("+AUIKWS:%d\r\n", notify);
    AT_BACK_OK();
}

void app_at_ctrl_notify(cJSON *js)
{
    /*
        AT协议为 --->"+AUICTRL=1\r\n"
    */
    if (0 == g_at_init) {
        return;
    }

    if (NULL != g_at_aui_ctx.js_info) {
        cJSON_Delete(g_at_aui_ctx.js_info);
    }

    g_at_aui_ctx.js_info = cJSON_CreateObject();
    cJSON *action = cJSON_GetObjectItemByPath(js, "payload.action");

    if (cJSON_IsString(action)) {
        cJSON_AddStringToObject(g_at_aui_ctx.js_info, "action", action->valuestring);
    } else {
        LOGD(TAG, "cJSON object doesn't have 'action' item, return!\n");
        cJSON_Delete(g_at_aui_ctx.js_info);
        g_at_aui_ctx.js_info = NULL;
        return;
    }

    cJSON *action_params = cJSON_GetObjectItemByPath(js, "payload.action_params");
    char *str_params = cJSON_PrintUnformatted(action_params);

    if (NULL != str_params) {
        cJSON_AddStringToObject(g_at_aui_ctx.js_info, "action_params", str_params);
    }

    convert_json_and_send("+AUICTRL", g_at_aui_ctx.js_info);

    free(str_params);
}

void app_event_cb(uint32_t event, void *p_arg)
{
    switch (event) {
    default:
        break;
    }
}

int app_at_cmd_init()
{
    int ret;
    atserver_add_command(at_cmd);
    event_subscribe(EVENT_APP_AT_CMD, event_app_at_cmd_handler, NULL);
    return 0;
}
