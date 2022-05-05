/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <yoc/atserver.h>
#include <at_cmd.h>
#include <wifi_provisioning.h>
#include <smartliving/exports/iot_export_linkkit.h>
#include <cJSON.h>
#include <aos/kv.h>
#include <ulog/ulog.h>
#include "at_smartliving.h"
#include "at_network.h"

#define TAG "at_smartliving"

extern cJSON *cJSON_GetObjectItemByPath(cJSON *object, const char *path);

static at_aui_info_t g_at_aui_ctx = {1, NULL};

#define AT_RESP_CME_ERR(errno)    atserver_send("\r\n+CME ERROR:%d\r\n", errno)

idm_at_post_property at_cmd_post_property = NULL;
idm_at_post_event at_cmd_post_event = NULL;
idm_is_connected at_cmd_is_cloud_connected = NULL;

void at_cmd_post_property_cb_reg(int (*cb)(int device_id, char *message, int len))
{
    at_cmd_post_property = cb;
}

void at_cmd_post_event_cb_reg(int (*cb)(int device_id, char *evtid, int evtid_len, char *evt_payload, int len))
{
    at_cmd_post_event = cb;
}

void at_cmd_is_connected_cb_reg(int (*cb)(void))
{
    at_cmd_is_cloud_connected = cb;
}

void event_publish_app_at_cmd(APP_AT_CMD_TYPE type)
{
    event_publish(EVENT_APP_AT_CMD, (void *)type);
}

/* remove double quote in first character and last character */
static char *remove_double_quote(char *str)
{
    char *  str_new_p = str;
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
    while('\0' != *str) {
        if('\\' == *str) {
            last_slash = 1;
            str++;
            continue;
        }
        if('\"' == *str && 0 == last_slash) {
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
    while('\0' != *oldStr) {
        if('\\' == *oldStr) {
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
    if(NULL == cmd || NULL == js_info) {
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
            event_publish_app_at_cmd(APP_AT_CMD_IWS_START);
        }
    } else {
        AT_RESP_CME_ERR(100);
    }
}

void iws_stop_handler(char *cmd, int type, char *data) {
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == EXECUTE_CMD) {
        if (wifi_prov_get_status() != WIFI_PROV_STOPED) {
            wifi_prov_stop();
        }
        AT_BACK_OK();
    }
}

void idm_au_handler(char *cmd, int type, char *data) {
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

void idm_con_handler(char *cmd, int type, char *data) {
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == EXECUTE_CMD) {
        event_publish_app_at_cmd(APP_AT_CMD_CON);
        AT_BACK_OK();
    }
}

void idm_cls_handler(char *cmd, int type, char *data) {
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == EXECUTE_CMD) {
        event_publish_app_at_cmd(APP_AT_CMD_CLS);
        AT_BACK_OK();
    }
}

void idm_sta_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == READ_CMD) {
        AT_BACK_RET_OK_INT(cmd, at_cmd_is_cloud_connected() << 1);
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
        if(NULL != g_at_aui_ctx.js_info) {
            convert_json_and_send("+AUICTRL", g_at_aui_ctx.js_info);
        } else {
            atserver_send("+AUICTRL:\r\n");
            AT_BACK_OK();
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
        
        int rst = at_cmd_post_property(0, msg, strlen(msg));
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
        
        int rst = at_cmd_post_event(0, evt_id, strlen(evt_id), payload, strlen(payload));
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
    atserver_send("+IDMSTA:%d\r\nOK\r\n", at_cmd_is_cloud_connected() << 1);
}

void app_at_kws_notify(uint8_t notify)
{
    /*
        AT协议为 --->"+AUIKWS=1\r\n"
    */
    atserver_send("+AUIKWS:%d\r\n", notify);
    AT_BACK_OK();
}

void app_at_ctrl_notify(cJSON *js)
{
    /*
        AT协议为 --->"+AUICTRL=1\r\n"
    */

    if(NULL != g_at_aui_ctx.js_info) {
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
