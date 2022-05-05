/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <errno.h>
#include <stdio.h>
#include <yoc/fota.h>
#include <yoc/netio.h>
#include <aos/kernel.h>
#include <aos/kv.h>
#include <aos/version.h>
#include <ulog/ulog.h>
#include <yoc/sysinfo.h>

#define COP_IMG_URL "cop_img_url"
#define COP_VERSION "cop_version"
#define TO_URL      "flash://misc"

#define TAG "fotacop"

static void cop_res_release(fota_info_t *info)
{
    if (info) {
        if (info->fota_url) {
            aos_free(info->fota_url);
            info->fota_url = NULL;
        }
        if (info->local_changelog) {
            aos_free(info->local_changelog);
            info->local_changelog = NULL;
        }
        if (info->changelog) {
            aos_free(info->changelog);
            info->changelog = NULL;
        }
        if (info->cur_version) {
            aos_free(info->cur_version);
            info->cur_version = NULL;
        }
        if (info->new_version) {
            aos_free(info->new_version);
            info->new_version = NULL;
        }
    }
}

#if CONFIG_FOTA_USE_HTTPC == 1
#include <http_client.h>
#include <cJSON.h>
static int cop_get_ota_url(char *ota_url, int len)
{
    int ret = -1;

    ret = aos_kv_getstring("otaurl", ota_url, len);
    if (ret < 0) {
        strcpy(ota_url, "http://occ.t-head.cn/api/image/ota/pull");
    }

    return ret;
}

static int _http_event_handler(http_client_event_t *evt)
{
    switch(evt->event_id) {
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
    } else if(status_code == HttpStatus_NotFound || status_code == HttpStatus_Forbidden) {
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

static int cop_version_check(fota_info_t *info) {
    int ret = 0, rc;
    char *payload = NULL;
    char getvalue[64];
    cJSON *js = NULL;
    char *buffer = NULL;
    http_errors_t err;
    http_client_config_t config = {0};
    http_client_handle_t client = NULL;

    buffer = aos_zalloc(BUFFER_SIZE + 1);
    if (buffer == NULL) {
        ret = -ENOMEM;
        goto out;
    }
    if ((payload = aos_malloc(156)) == NULL) {
        ret = -ENOMEM;
        goto out;
    }
#if 1
    char * dev_id = aos_get_device_id();
    char * pro_model = (char *)aos_get_product_model();
    char * app_ver = aos_get_app_version();
    snprintf(payload, 156, "{\"cid\":\"%s\",\"model\":\"%s\",\"version\":\"%s\"}",
             dev_id ? dev_id : "null", pro_model ? pro_model : "null", app_ver ? app_ver : "null");
#else
    //snprintf(payload, 100, "{\"cid\":\"%s\",\"model\":\"%s\",\"version\":\"%s\"}",
    //         "00A2C6FB32241D9423F5AF00", "hlb_test", "1.0.0-20181125.1321-R-hlb_tes");
    // FIXME: for test.
    snprintf(payload, 156, "{\"cid\":\"%s\",\"model\":\"%s\",\"version\":\"%s\"}",
            "e6d0d5480440000008376eb1e834a765", "pangu", "1.1.1-20201027.2235-R-pangu");
#endif
    LOGD(TAG, "check: %s", payload);

    memset(getvalue, 0, sizeof(getvalue));
    cop_get_ota_url(getvalue, sizeof(getvalue));
    LOGD(TAG, "ota url:%s", getvalue);

    config.method = HTTP_METHOD_POST;
    config.url = getvalue;
    config.timeout_ms = 10000;
    config.buffer_size = BUFFER_SIZE;
    config.event_handler = _http_event_handler;
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

    cJSON *version = cJSON_GetObjectItem(result, "version");
    if (!(version && cJSON_IsString(version))) {
        LOGW(TAG, "get version failed");
        ret = -1;
        goto out;
    }
    LOGD(TAG, "version: %s", version->valuestring);
    aos_kv_setstring(COP_VERSION, version->valuestring);

    cJSON *url = cJSON_GetObjectItem(result, "url");
    if (!(url && cJSON_IsString(url))) {
        ret = -1;
        LOGW(TAG, "get url failed");
        goto out;
    }
    LOGD(TAG, "url: %s", url->valuestring);

    char *urlbuf = aos_malloc(156);
    if (urlbuf == NULL) {
        ret = -1;
        goto out;
    }
    rc = aos_kv_getstring(COP_IMG_URL, urlbuf, 156);

    if (rc <= 0) {
        aos_kv_setstring(COP_IMG_URL, url->valuestring);
    } else {
        if (strcmp(url->valuestring, urlbuf) == 0) {
            aos_kv_getint("fota_offset", &rc);
            LOGI(TAG, "continue fota :%d", rc);
        } else {
            aos_kv_setstring(COP_IMG_URL, url->valuestring);
            aos_kv_setint("fota_offset", 0);
            LOGI(TAG, "restart fota");
        }
    }
    aos_free(urlbuf);

    if (info->fota_url) {
        aos_free(info->fota_url);
        info->fota_url = NULL;
    }
    info->fota_url = strdup(url->valuestring);
    LOGD(TAG, "get url: %s", info->fota_url);
out:
    if (buffer) aos_free(buffer);
    if (payload) aos_free(payload);
    if (js) cJSON_Delete(js);
    _http_cleanup(client);
    return ret;
}

#else

#include "../http/http.h"
#include "util/network.h"
char *json_getvalue(char *body, char *key, int *len)
{
    char key_buf[16];
    char *start_ptr, *end_ptr;

    snprintf(key_buf, 16, "\"%s\":", key);

    start_ptr = strstr(body, key_buf);

    if (start_ptr == NULL || body == start_ptr) {
        LOGD(TAG, "key");
        return NULL;
    }

    start_ptr += strlen(key_buf);

    while ((*start_ptr == ' ') || (*start_ptr == '\"') || (*start_ptr == 0)) {
        start_ptr++;
    }

    end_ptr = start_ptr;

    while (((*end_ptr != ',') && (*end_ptr != '\"')) && (*end_ptr != 0)) {
        end_ptr++;
    }

    if (start_ptr == end_ptr) {
        return NULL;
    }

    if (*end_ptr == '\"') {
        end_ptr --;
    }

    *len = end_ptr - start_ptr + 1;

    return start_ptr;
}

static int cop_get_ota_url(char *ota_url, int len)
{
    int ret = -1;

    ret = aos_kv_getstring("otaurl", ota_url, len);
    if (ret < 0) {
        strcpy(ota_url, "http://occ.t-head.cn/api/image/ota/pull");
    }

    return ret;
}

static int cop_version_check(fota_info_t *info) {
    http_t *http;
    char *payload;
    char *body;
    int value_len;
    char *cptr;
    char getvalue[64];
    int rc;

    if ((payload = aos_malloc(156)) == NULL) {
        return -1;
    }

    snprintf(payload, 156, "{\"cid\":\"%s\",\"model\":\"%s\",\"version\":\"%s\"}",
             aos_get_device_id(), aos_get_product_model(), aos_get_app_version());
    //snprintf(payload, 100, "{\"cid\":\"%s\",\"model\":\"%s\",\"version\":\"%s\"}",
    //         "00A2C6FB32241D9423F5AF00", "hlb_test", "1.0.0-20181125.1321-R-hlb_tes");

    LOGD(TAG, "check: %s", payload);

    memset(getvalue, 0, sizeof(getvalue));
    cop_get_ota_url(getvalue, sizeof(getvalue));

    if ((http = http_init(getvalue)) == NULL) {
        LOGD(TAG, "e cop http init");
        aos_free(payload);
        return -1;
    }
    memset(http->buffer, 0, BUFFER_SIZE);
    http->buffer_offset = 0;
    http_head_sets(http, "Host", http->host);
    http_head_seti(http, "Content-Length", strlen(payload));
    http_head_sets(http, "Connection", "keep-alive");
    http_head_sets(http, "Content-Type", "application/json");
    http_head_sets(http, "Cache-Control", "no-cache");

    http_post(http, payload, 10000);

    if (http_wait_resp(http, &body, 10000) < 0) {
        http_deinit(http);
        aos_free(payload);
        LOGD(TAG, "recv failed");
        return -1;
    }

    aos_free(payload);
    LOGD(TAG, "resp body: %s", body);
    cptr = json_getvalue(body, "code", &value_len);

    if ((cptr == NULL) || (atoi(cptr) < 0)) {
        LOGW(TAG, "rsp format");
        http_deinit(http);
        return -1;
    }

    cptr = json_getvalue(body, "version", &value_len);
    if (cptr == NULL) {
        LOGD(TAG, "rsp format %s", "version");
        http_deinit(http);
        return -1;
    }
    char *ver = aos_malloc(value_len + 1);
    if (!ver) {
        return -1;
    }
    memcpy(ver, cptr, value_len);
    ver[value_len] = 0;
    LOGD(TAG, "%s: %s", COP_VERSION, ver);
    aos_kv_setstring(COP_VERSION, ver);
    aos_free(ver);

    cptr = json_getvalue(body, "url", &value_len);

    if (cptr == NULL) {
        LOGD(TAG, "rsp format %s", "url");
        http_deinit(http);
        return -1;
    }

    if (http->url) {
        aos_free(http->url);
        http->url = NULL;
    }
    http->url = calloc(value_len + 1, 1);

    if (http->url == NULL) {
        LOGD(TAG, "realloc failed");
        http_deinit(http);
        return -1;
    }

    if (strstr(cptr, "https://")) {
        snprintf(http->url, value_len, "http://%s", cptr + strlen("https://"));
    } else {
        memcpy(http->url, cptr, value_len);
    }

    char *buffer = aos_malloc(156);

    if (buffer == NULL) {
        http_deinit(http);
        return -1;
    }

    rc = aos_kv_getstring(COP_IMG_URL, buffer, 156);

    if (rc <= 0) {
        aos_kv_setstring(COP_IMG_URL, http->url);
    } else {
        if (strcmp(http->url, buffer) == 0) {
            aos_kv_getint("fota_offset", &rc);
            LOGI(TAG, "continue fota :%d", rc);
        } else {
            aos_kv_setstring(COP_IMG_URL, http->url);
            aos_kv_setint("fota_offset", 0);
            LOGI(TAG, "restart fota");
        }
    }
    aos_free(buffer);
    if (info->fota_url) {
        aos_free(info->fota_url);
        info->fota_url = NULL;
    }
    if (info->changelog) {
        aos_free(info->changelog);
        info->changelog = NULL;
    }
    if (info->new_version) {
        aos_free(info->new_version);
        info->new_version = NULL;
    }
    info->fota_url = strdup(http->url);
    LOGD(TAG, "get url: %s", info->fota_url);
    http_deinit(http);
    return 0;
}
#endif /* CONFIG_FOTA_USE_HTTPC */

static int finish(fota_info_t *info)
{
    cop_res_release(info);
    return 0;
}

static int fail(fota_info_t *info)
{
    cop_res_release(info);
    return 0;
}

const fota_cls_t fota_cop_cls = {
    "cop",
    NULL,
    cop_version_check,
    finish,
    fail,
    NULL
};

int fota_register_cop(void)
{
    return fota_register(&fota_cop_cls);
}