/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <yoc/fota.h>
#if CONFIG_FOTA_USE_HTTPC == 1
#include <yoc/netio.h>
#include <ulog/ulog.h>
#include <aos/kernel.h>
#include <errno.h>
#include <http_client.h>
#include "util/network.h"

#define TAG "fota-httpc"

typedef struct {
    http_client_handle_t http_client;
    const char *cert;
    const char *path;
} httpc_priv_t;

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

static http_errors_t _http_connect(http_client_handle_t http_client, char *buffer, int buf_size)
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
        err = http_client_open(http_client, 0);
        if (err != HTTP_CLI_OK) {
            LOGE(TAG, "Failed to open HTTP connection");
            return err;
        }
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
    LOGD(TAG, "httpc cleanup...");
    if (client) {
        http_client_cleanup(client);
    }
}

static int http_read(netio_t *io, uint8_t *buffer, int length, int timeoutms)
{
#define RANGE_BUF_SIZE 56
    int read_len;
    long long time1ms;
    httpc_priv_t *priv = (httpc_priv_t *)io->private;
    http_client_handle_t client = priv->http_client;

    if (client == NULL) {
        int ret = 0;
        int statuscode;
        char *range = NULL;
        char *buffer = NULL;
        http_errors_t err;
        http_client_config_t config = {0};

        config.method = HTTP_METHOD_GET;
        config.url = priv->path;
        config.timeout_ms = timeoutms;
        config.buffer_size = BUFFER_SIZE;
        config.cert_pem = priv->cert;
        config.event_handler = _http_event_handler;
        client = http_client_init(&config);
        if (!client) {
            LOGE(TAG, "Client init e");
            ret = -1;
            goto exit;
        }
        LOGD(TAG, "http client init ok.[%s]", config.url);
        LOGD(TAG, "http read connecting........");
        buffer = aos_zalloc(BUFFER_SIZE + 1);
        if (!buffer) {
            LOGE(TAG, "http open nomem.");
            ret = -ENOMEM;
            goto exit;
        }
        range = (void *)aos_zalloc(RANGE_BUF_SIZE);
        if (!range) {
            LOGE(TAG, "range malloc e");
            ret = -ENOMEM;
            goto exit;
        }
        snprintf(range, RANGE_BUF_SIZE, "bytes=%zu-", io->offset);
        LOGD(TAG, "range:%s", range);
        err = http_client_set_header(client, "Range", range);
        if (err != HTTP_CLI_OK) {
            LOGE(TAG, "set header Range e");
            ret = -1;
            goto exit;
        }
        err = http_client_set_header(client, "Connection", "keep-alive");
        if (err != HTTP_CLI_OK) {
            LOGE(TAG, "set header Connection e");
            ret = -1;
            goto exit;
        }
        err = http_client_set_header(client, "Cache-Control", "no-cache");
        if (err != HTTP_CLI_OK) {
            LOGE(TAG, "set header Cache-Control e");
            ret = -1;
            goto exit;
        }

        err = _http_connect(client, buffer, BUFFER_SIZE);
        if (err != HTTP_CLI_OK) {
            LOGE(TAG, "Client connect e");
            ret = -1;
            goto exit;
        }
        statuscode = http_client_get_status_code(client);
        if (statuscode != 206) {
            LOGE(TAG, "not 206 Partial Content");
            ret = -1;
            goto exit;
        }
        io->size = http_client_get_content_length(client);
        io->size += io->offset;
        LOGD(TAG, "range_len: %d", io->size);
        priv->http_client = client;
exit:
        if (buffer) aos_free(buffer);
        if (range) aos_free(range);
        if (ret != 0) {
            _http_cleanup(client);
            return ret;
        }
    }
    if (io->offset >= io->size) {
        LOGW(TAG, "http_read done: offset:%d tsize:%d", io->offset, io->size);
        return 0;
    }

    time1ms = aos_now_ms();
    read_len = 0;
    while (read_len < length) {
        if (aos_now_ms() - time1ms > timeoutms) {
            break;
        }
        int data_read = http_client_read(client, (char *)buffer + read_len, length - read_len);
        if (data_read < 0) {
            LOGW(TAG, "http client read error:%d, errno:%d", data_read, errno);
            return data_read;
        } else if (data_read == 0) {
            LOGD(TAG, "http client read 0 size");
            break;
        }
        read_len += data_read;
    }
    io->offset += read_len;
    return read_len;
}

static int http_open(netio_t *io, const char *path)
{
    const char *cert;

    LOGD(TAG, "http open:%s", path);
    cert = (const char *)io->cls->private;

    io->size = 0;
    io->offset = 0;
    io->block_size = CONFIG_FOTA_BUFFER_SIZE;

    httpc_priv_t *priv = aos_zalloc_check(sizeof(httpc_priv_t));
    if (!priv) {
        LOGE(TAG, "http open nomem e");
        return -1;
    }
    priv->http_client = NULL;
    priv->path = path;
    priv->cert = cert;
    io->private = priv;
#if 0 // just for test, mem leak...
    char *tempbuffer = (char *)aos_malloc(400);
    if (tempbuffer == NULL) {
        LOGE(TAG, "malloc tempbuffer failed");
        return -1;
    }
    memset(tempbuffer, 0, 400);
    if (strstr(priv->path, "https://")) {
        snprintf(tempbuffer, 400, "http://%s", priv->path + strlen("https://"));
    }
    priv->path = tempbuffer;
#endif
#if 0 //just for test redirection
    priv->path = "http://192.168.1.102:8889";
#endif
    return 0;
}

static int http_seek(netio_t *io, size_t offset, int whence)
{
    io->offset = offset;
    return 0;
}

static int http_close(netio_t *io)
{
    httpc_priv_t *priv = (httpc_priv_t *)io->private;
    http_client_handle_t client = priv->http_client;
    _http_cleanup(client);
    aos_free(priv);
    return 0;
}

netio_cls_t httpc_cls = {
    .name = "http",
    .read = http_read,
    .seek = http_seek,
    .open = http_open,
    .close = http_close,
};

int netio_register_httpc(const char *cert)
{
    httpc_cls.private = (void *)cert;
    return netio_register(&httpc_cls);
}
#endif