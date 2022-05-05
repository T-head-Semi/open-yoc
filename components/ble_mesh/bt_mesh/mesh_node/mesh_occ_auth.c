/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include "stdint.h"
#include "errno.h"
#include "ulog/ulog.h"
#include "crypto_md.h"
#include "sec_crypto_auth.h"
#include "key_mgr.h"
#include "mesh_occ_auth.h"
#include <sys/time.h>
#include <common/log.h>
#ifdef   CONFIG_BT_MESH_PROVISIONER
#include "aos/kernel.h"
#include "k_types.h"
#include "k_api.h"
#include "k_err.h"
#include "k_sys.h"
#include "k_list.h"
#include "k_obj.h"
#include "k_ringbuf.h"
#include "k_buf_queue.h"
#include "k_task.h"
#include "k_sem.h"
#include "k_mutex.h"
#include "k_timer.h"
#include "misc/dlist.h"
#include "misc/slist.h"
#include "ota_server.h"
#include "ble_os_port.h"
#include "port/kport.h"
#include <http_client.h>
#include <cJSON.h>
#include "aos/kv.h"
#endif

#define TAG "MESH_OCC_AUTH"

uint8_t      g_occ_auth_init_flag = 0;

static uint8_t char2u8(char c)
{
    if (c >= '0' && c <= '9') {
        return (c - '0');
    } else if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    } else {
        return 0;
    }
}

static void str2hex(uint8_t hex[], char *s, uint8_t cnt)
{
    uint8_t i;

    if (!s) {
        return;
    }

    for (i = 0; (*s != '\0') && (i < cnt); i++, s += 2) {
        hex[i] = ((char2u8(*s) & 0x0f) << 4) | ((char2u8(*(s + 1))) & 0x0f);
    }
}

#ifdef    CONFIG_BT_MESH_PROVISIONER
typedef struct {
    uint8_t    addr[6];
    uint8_t    addr_type;
    uint8_t    uuid[16];
    uint8_t    auth_status;
	uint8_t    faild_time;
} occ_auth_node;

typedef struct {
    uint8_t       event;
    occ_auth_node node;
} occ_auth_cmd_message;

enum {
    OCC_AUTH_START,
    OCC_AUTH_CANCEL,
};

#define BUFFER_SIZE 2048

#define MAX_OCC_AUTH_FAIL_TIME 5

occ_auth_cb  g_auth_cb;
ktask_t      g_occ_auth_task;
static cpu_stack_t g_occ_auth_task_stack[CONFIG_OCC_AUTH_STACK_SIZE / 4];
static aos_queue_t    g_auth_queue;
occ_auth_cmd_message   g_auth_queue_message[CONFIG_OCC_AUTH_NODE_MAX_MSG_SIZE];

static int post_verify_msg_url(char *ota_url, int len)
{
    int ret = 0;
    char occ_url[40] = {0};
    int occ_url_len = sizeof(occ_url);

    ret = aos_kv_get("occurl", occ_url, &occ_url_len);
    if (ret != 0) {
        strcpy(occ_url, "https://occ.t-head.cn");
    }

    snprintf(ota_url, len, "%s%s", occ_url, "/api/device/gateway/deviceAuth");

    return 0;
}


static bool process_again(int status_code)
{
    switch (status_code) {
        case HttpStatus_MovedPermanently:
        case HttpStatus_Found:
        case HttpStatus_TemporaryRedirect:
        case HttpStatus_Unauthorized:
            return true;

        default:
            return false;
    }

    return false;
}


static http_errors_t _http_handle_response_code(http_client_handle_t http_client, int status_code, char *buffer, int buf_size, int data_size)
{
    http_errors_t err;

    if (status_code == HttpStatus_MovedPermanently || status_code == HttpStatus_Found || status_code == HttpStatus_TemporaryRedirect) {
        err = http_client_set_redirection(http_client);

        if (err != HTTP_CLI_OK) {
            LOGE(TAG, "URL redirection Failed");
            return err;
        }
    } else if (status_code == HttpStatus_Unauthorized) {
        return HTTP_CLI_FAIL;
    } else if (status_code == HttpStatus_NotFound || status_code == HttpStatus_Forbidden) {
        LOGE(TAG, "File not found(%d)", status_code);
        return HTTP_CLI_FAIL;
    } else if (status_code == HttpStatus_InternalError) {
        LOGE(TAG, "Server error occurred(%d)", status_code);
        return HTTP_CLI_FAIL;
    }

    // process_again() returns true only in case of redirection.
    if (data_size > 0 && process_again(status_code)) {
        /*
        *  In case of redirection, http_client_read() is called
        *  to clear the response buffer of http_client.
        */
        int data_read;

        while (data_size > buf_size) {
            data_read = http_client_read(http_client, buffer, buf_size);

            if (data_read <= 0) {
                return HTTP_CLI_OK;
            }

            data_size -= buf_size;
        }

        data_read = http_client_read(http_client, buffer, data_size);

        if (data_read <= 0) {
            return HTTP_CLI_OK;
        }
    }

    return HTTP_CLI_OK;
}

static http_errors_t _http_connect(http_client_handle_t http_client, const char *payload, char *buffer, int buf_size)
{
#define MAX_REDIRECTION_COUNT 10
    http_errors_t err = HTTP_CLI_FAIL;
    int status_code = 0, header_ret;
    int redirect_counter = 0;

    do {
        if (redirect_counter++ > MAX_REDIRECTION_COUNT) {
            LOGE(TAG, "redirect_counter is max");
            return HTTP_CLI_FAIL;
        }

        if (process_again(status_code)) {
            LOGD(TAG, "process again,status code:%d", status_code);
        }

        err = http_client_open(http_client, strlen(payload));

        if (err != HTTP_CLI_OK) {
            LOGE(TAG, "Failed to open HTTP connection");
            return err;
        }

        int wlen = http_client_write(http_client, payload, strlen(payload));

        if (wlen < 0) {
            LOGE(TAG, "Write payload failed");
            return HTTP_CLI_FAIL;
        }

        LOGD(TAG, "write payload ok...");
        header_ret = http_client_fetch_headers(http_client);

        if (header_ret < 0) {
            LOGE(TAG, "header_ret:%d", header_ret);
            return header_ret;
        }

        LOGD(TAG, "header_ret:%d", header_ret);
        status_code = http_client_get_status_code(http_client);
        LOGD(TAG, "status code:%d", status_code);
        err = _http_handle_response_code(http_client, status_code, buffer, buf_size, header_ret);

        if (err != HTTP_CLI_OK) {
            LOGE(TAG, "e handle resp code:%d", err);
            return err;
        }
    } while (process_again(status_code));

    return err;
}

static void _http_cleanup(http_client_handle_t client)
{
    if (client) {
        http_client_cleanup(client);
    }
}

static int _http_event_handler(http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            LOGD(TAG, "HTTP_EVENT_ERROR");
            break;

        case HTTP_EVENT_ON_CONNECTED:
            LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;

        case HTTP_EVENT_HEADER_SENT:
            LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;

        case HTTP_EVENT_ON_HEADER:
            // LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;

        case HTTP_EVENT_ON_DATA:
            // LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;

        case HTTP_EVENT_ON_FINISH:
            LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;

        case HTTP_EVENT_DISCONNECTED:
            LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }

    return 0;
}

static char *hextostr(const uint8_t *source, char *dest, int len)
{
    int  i;
    char tmp[3];

    for (i = 0; i < len; i++) {
        sprintf(tmp, "%02x", (unsigned char)source[i]);
        memcpy(&dest[i * 2], tmp, 2);
    }

    dest[len * 2] = 0;
    return dest;
}

static int check_device_auth_identification(uint8_t *mac, uint32_t vid,
        uint8_t random, uint32_t authcode, char *pcid, uint32_t *short_oob)
{
    int ret = 0, rc;
    char *payload = NULL;
    char getvalue[255];
    cJSON *js = NULL;
    char *buffer = NULL;
    http_errors_t err;
    http_client_config_t config = {0};
    http_client_handle_t client = NULL;

    key_handle     key_addr;
    char           cidr[32 + 1];
    char           authmsg[20];
    char           temp[33 * 2];
    char           timestr[10];
    uint32_t       cid_len;
    uint32_t       t;
    struct timeval t1;
    gettimeofday(&t1, NULL);

    uint8_t oob_hex[16] = {0x00};

    buffer = aos_zalloc(BUFFER_SIZE + 1);

    if (buffer == NULL) {
        ret = -ENOMEM;
        goto out;
    }

    if ((payload = aos_malloc(256)) == NULL) {
        ret = -ENOMEM;
        goto out;
    }

    cidr[32] = 0;
    memset(cidr, 0, sizeof(cidr));
    ret = km_get_key(KEY_ID_CHIPID, &key_addr, &cid_len);

    if (ret != KM_OK) {
        goto out;
    }

    memcpy(cidr, (void *)key_addr, 32);
    stringtohex(cidr, temp, 16);
    cid_len = 16;
    t = t1.tv_sec;
    sprintf(timestr, "%d", t);

    ret = sc_srv_get_authmsg(temp, cid_len, (unsigned char *)timestr, strlen(timestr), authmsg);

    snprintf(payload, 256, "{\"mac\":\"%02x%02x%02x%02x%02x%02x\",\"vid\":\"%d\",\"message\":\"%d\",\"authcode\":\"%u\", \"authMsg\":{\"cid\":\"%s\", \"timestamp\":\"%s\", \"authcode\":\"%s\"}}",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], vid, random, authcode, cidr, timestr, hextostr(authmsg, temp, sizeof(authmsg)));

    LOGD(TAG, "check: %s", payload);

    memset(getvalue, 0, sizeof(getvalue));
    post_verify_msg_url(getvalue, sizeof(getvalue));

    LOGD(TAG, "ota url:%s", getvalue);

    config.method           = HTTP_METHOD_POST;
    config.url              = getvalue;
    config.timeout_ms       = 10000;
    config.buffer_size      = BUFFER_SIZE;
    config.event_handler    = _http_event_handler;
    LOGD(TAG, "http client init start.");
    client = http_client_init(&config);

    if (!client) {
        LOGE(TAG, "Client init e");
        ret = -1;
        goto out;
    }

    LOGD(TAG, "http client init ok.");
    http_client_set_header(client, "Content-Type", "application/json");
    http_client_set_header(client, "Connection", "keep-alive");
    http_client_set_header(client, "Cache-Control", "no-cache");
    err = _http_connect(client, payload, buffer, BUFFER_SIZE);

    if (err != HTTP_CLI_OK) {
        LOGE(TAG, "Client connect e");
        ret = -1;
        goto out;
    }

    int read_len = http_client_read(client, buffer, BUFFER_SIZE);

    if (read_len <= 0) {
        ret = -1;
        goto out;
    }

    buffer[read_len] = 0;
    LOGD(TAG, "resp: %s", buffer);

    js = cJSON_Parse(buffer);

    if (js == NULL) {
        ret = -1;
        LOGW(TAG, "cJSON_Parse failed");
        goto out;
    }

    cJSON *code = cJSON_GetObjectItem(js, "code");

    if (!(code && cJSON_IsNumber(code))) {
        ret = -1;
        LOGW(TAG, "get code failed");
        goto out;
    }

    LOGD(TAG, "code: %d", code->valueint);

    if (code->valueint < 0) {
        ret = -1;
        goto out;
    }

    cJSON *result = cJSON_GetObjectItem(js, "result");

    if (!(result && cJSON_IsObject(result))) {
        LOGW(TAG, "get result failed");
        ret = -1;
        goto out;
    }

    cJSON *identify = cJSON_GetObjectItem(result, "identify");

    if (!(identify && cJSON_IsString(identify))) {
        LOGW(TAG, "get identify failed");
        ret = -1;
        goto out;
    }

    LOGD(TAG, "identify: %s", identify->valuestring);
    /* cid */
    cJSON *cid = cJSON_GetObjectItem(result, "cid");

    if (!(cid && cJSON_IsString(cid))) {
        LOGW(TAG, "get cid failed");
        ret = -1;
        goto out;
    }

    LOGD(TAG, "cid: %s", cid->valuestring);
    strcpy(pcid, cid->valuestring);
    /* oob */
    cJSON *oob = cJSON_GetObjectItem(result, "oob");

    if (!(oob && cJSON_IsString(oob))) {
        LOGW(TAG, "get oob failed");
        ret = -1;
        goto out;
    }

    LOGD(TAG, "oob: %s", oob->valuestring);
    //strcpy(poob, oob->valuestring);
    str2hex(oob_hex, oob->valuestring, 16);
    *short_oob = oob_hex[0] << 24 | oob_hex[1] << 16 | oob_hex[2] << 8 | oob_hex[3];

out:

    if (buffer) {
        aos_free(buffer);
    }

    if (payload) {
        aos_free(payload);
    }

    if (js) {
        cJSON_Delete(js);
    }

    _http_cleanup(client);

    return ret;
}



static int occ_auth_start(uint8_t uuid[16], char cid[33], uint32_t *short_oob)
{
    uint32_t authcode;
    uint16_t vendor_id;
    uint8_t rand;
    uint8_t mac[6];
    int ret = 0;

    memcpy(&authcode, &uuid[0], 4);
    memcpy(mac, &uuid[4], 6);
    rand = uuid[10];
    memcpy(&vendor_id, &uuid[11], 2);

    ret = check_device_auth_identification(mac, vendor_id, rand, authcode, cid, short_oob);

    if (ret) {
        LOGE(TAG, "Send check device auth ident fail %d", ret);
    }

    return ret;
}


static int occ_auth_cmd_get(occ_auth_cmd_message *cmd, unsigned int timeout)
{
    int ret = 0;
    unsigned int read_size = 0;

    if (!cmd || !aos_queue_is_valid(&g_auth_queue)) {
        return -EINVAL;
    }

    ret = aos_queue_recv(&g_auth_queue, timeout, (void *)cmd, &read_size);

    if (ret == 0 && read_size > 0) {
        return 0;
    } else {
        LOGE(TAG, "get queue message fail:%d", ret);
        return -1;
    }

    return 0;
}

static int _mesh_occ_auth_dev_add(uint8_t addr[6], uint8_t addr_type, uint8_t uuid[16], uint8_t try_time)
{
	   int ret = 0;
	   occ_auth_cmd_message cmd = {0x00};
	   cmd.event = OCC_AUTH_START;
	   memcpy(cmd.node.addr, addr, 6);
	   cmd.node.addr_type = addr_type;
	   cmd.node.faild_time = try_time;
	   memcpy(cmd.node.uuid, uuid, 16);
	   ret = aos_queue_send(&g_auth_queue, &cmd, sizeof(occ_auth_cmd_message));
	   if (ret) {
		   LOGE(TAG, "Add auth node faild %d", ret);
		   return ret;
	   }
}

static void occ_auth_task_handler(void *arg)
{
    int ret = 0;
    occ_auth_cmd_message cmd;

    while (1) {
        ret = occ_auth_cmd_get(&cmd, AOS_WAIT_FOREVER);
        if (ret < 0) {
            LOGE(TAG, "Get occ cmd get faild %d", ret);
        }

        LOGD(TAG, "Occ event:%4x", cmd.event);

        switch (cmd.event) {
            case OCC_AUTH_START: {
                occ_auth_data data = {0x00};
                ret = occ_auth_start(cmd.node.uuid, data.CID, &data.short_oob);
                if (ret) {
					cmd.node.faild_time++;
                    LOGE(TAG, "Occ auth faild for device:%s %d %d", bt_hex_real(cmd.node.addr, 6), ret,cmd.node.faild_time);
				    if(cmd.node.faild_time >= MAX_OCC_AUTH_FAIL_TIME) {
                       data.auth_status = MESH_AUTH_FAILD;
					}else {
                       _mesh_occ_auth_dev_add(cmd.node.addr,cmd.node.addr_type,cmd.node.uuid,cmd.node.faild_time);
					   break;
					}
                } else {
                     LOGE(TAG, "Occ auth success for device:%s", bt_hex_real(cmd.node.addr, 6));
                     data.auth_status = MESH_AUTH_SUCCESS;
				}
                if (g_auth_cb) {
                    g_auth_cb(cmd.node.addr, cmd.node.addr_type, cmd.node.uuid, &data);
                }
            }break;
            case OCC_AUTH_CANCEL: {

            } break;

            default:
                break;
        }
    }

}


int mesh_occ_auth_prov_init(occ_auth_cb cb)
{
    int ret = 0;

    static uint8_t init_flag = 0;

    if (init_flag) {
        return -EALREADY;
    }


    ret = aos_queue_new(&g_auth_queue, (void *)g_auth_queue_message, sizeof(g_auth_queue_message), sizeof(occ_auth_cmd_message));

    if (ret) {
        LOGE(TAG, "Occ auth queue create faild");
        return -1;
    }

    ret = krhino_task_create(&g_occ_auth_task, "occ_auth_task", NULL,
                             CONFIG_OCC_AUTH_TASK_PRIO, 1, g_occ_auth_task_stack,                          CONFIG_OCC_AUTH_STACK_SIZE / 4, (task_entry_t)occ_auth_task_handler, 1);

    if (ret < 0) {
        LOGE(TAG, "Occ auth task create faild %d", ret);
        return -1;
    }


    g_auth_cb = cb;
    init_flag = 1;

    return 0;
}

int mesh_occ_auth_prov_dev_add(uint8_t addr[6], uint8_t addr_type, uint8_t uuid[16])
{
    return _mesh_occ_auth_dev_add(addr,addr_type,uuid,0);
}

#else


static int  sc_get_auth_data(message_auth_t *msg, uint32_t *authcode)
{
    key_handle key_addr;
    uint32_t len;
    uint8_t vid[9];

    int ret = km_get_key(KEY_ID_MAC, &key_addr, &len);

    if (ret != KM_OK) {
        return -1;
    }

    memcpy(msg->mac, (uint8_t *)key_addr, 6);

    ret = km_get_key(KEY_ID_CHIPID, &key_addr, &len);

    if (ret != KM_OK) {
        return -1;
    }

    memcpy(vid, (uint8_t *)key_addr + 8, 8);

    vid[8]          = 0;
    uint32_t temp   = strtol((const char *)vid, NULL, 16);
    msg->vendor_id   = (temp & 0xff800) >> 11;
    uint32_t rand_data;
    extern int bt_rand(void *buf, size_t len);
    bt_rand(&rand_data, 4);
    msg->rand        = (uint8_t)(rand_data & 0xff);

    ret = sc_srv_get_message_auth(msg, authcode);

    if (ret) {
        return -1;
    }

    LOGD(TAG, "auth:%06x\r\n", *authcode);
    return 0;
}



int mesh_occ_auth_node_init()
{
    static uint8_t init_flag = 0;

    if (init_flag) {
        return -EALREADY;
    }

    km_init();
    init_flag = 1;
    return 0;
}

int mesh_occ_auth_node_get_uuid(uint8_t uuid[16])
{
    if (!uuid) {
        return -EINVAL;
    }

    int ret = 0;
    message_auth_t msg = {0};
    uint32_t authcode = 0;
    ret = sc_get_auth_data(&msg, &authcode);

    if (ret) {
        LOGE(TAG, "sc get auth faild");
        return -1;
    }

    uuid[0] = authcode & 0x000000ff;
    uuid[1] = (authcode & 0x0000FF00) >> 8;
    uuid[2] = (authcode & 0x00FF0000) >> 16;
    uuid[3] = (authcode & 0xFF000000) >> 24;

    memcpy(uuid + 4, msg.mac, 6); //6B MAC

    uuid[10] =  msg.rand;       // 1B rand
    uuid[11] = msg.vendor_id & 0x00FF;
    uuid[12] = (msg.vendor_id & 0xFF00) >> 8 ;
    LOGD(TAG, "kp uuid get:%s", bt_hex(uuid, 16));

    return 0;
}

int mesh_occ_auth_node_get_oob(uint8_t oob[16])
{
    if (!oob) {
        return -1;
    }

    key_handle key_addr;
    uint32_t len = 0;
    uint32_t oob_len = 0;
    int ret = 0;
    uint8_t Salt[16] = {0};
    ret = km_get_key(KEY_ID_CHIPID, &key_addr, &len);

    if (ret != KM_OK) {
        return -1;
    }

    str2hex(Salt, (char *)key_addr, 16);
    ret = sc_srv_get_auth_key(Salt, 16, oob, &oob_len);

    if (ret) {
        LOGE(TAG, "Get oob key faild %d\r\n", ret);
        return -1;
    }

    LOGD(TAG, "Get oob %s\r\n", bt_hex_real(oob, 16));
    return 0;
}



#endif
