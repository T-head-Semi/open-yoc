/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "ulog/ulog.h"
#include <aos/kernel.h>
#include <yoc/fota.h>
#include "http.h"

#define TAG "http"

http_t *http_init(const char *path)
{
    char *ip, *port_str;
    char *end_point;
    char *host = NULL;
    int port;

    // FIXME: only support HTTP now.
    if (path == NULL || !strstr(path, "http://")) {
        return NULL;
    }

    http_t *http = aos_zalloc_check(sizeof(http_t));

    http->url = strdup(path);
    if (http->url == NULL) {
        goto error;
    }

    host = http->url + sizeof("http://") - 1;
    if (strlen(host) == 0) {
        goto error;
    }
    http->port = 80;

    ip = http->url + strlen("http://");

    port_str = strrchr(host, ':');

    if (port_str != NULL) {
        *port_str = 0;
        port_str ++;
        port = strtol(port_str, &end_point, 10);
        http->port = port;
    } else {
        port = 80;
        end_point = strchr(ip, '/');
    }

    if (end_point != NULL) {
        http->path = strdup(end_point);
        LOGD(TAG, "http: path %s", http->path);

        if (http->path == NULL) {
            goto error;
        }
    } else {
        goto error;
    }

    http->buffer = (uint8_t *)aos_malloc(BUFFER_SIZE);
    if (http->buffer == NULL) {
        goto error;
    }
    memset(http->buffer, 0, BUFFER_SIZE);

    *end_point = 0;
    http->host = strdup(ip);
    if (http->host == NULL) {
        goto error;
    }

    LOGD(TAG, "http connect: %s:%d", http->host, port);

    network_init(&http->net);
    if (http->net.net_connect(&http->net, http->host, port, SOCK_STREAM) != 0) {
        goto error;
    }

    return http;

error:
    if (http) {
        if (http->url) aos_free(http->url);
        if (http->path) aos_free(http->path);
        if (http->host) aos_free(http->host);
        if (http->buffer) aos_free(http->buffer);
        aos_free(http);
    }

    return NULL;
}

int http_head_sets(http_t *http, const char *key, const char *value)
{
    int len;
    if (http->buffer == NULL) {
        return -1;
    }

    len = snprintf((char*)http->buffer + http->buffer_offset, BUFFER_SIZE - http->buffer_offset,
            "%s: %s\r\n", key, value);
    if (len <= 0) {
        return -1;
    }

    http->buffer_offset += len;

    return 0;
}

int http_head_seti(http_t *http, const char *key, int value)
{
    int len;
    if (http->buffer == NULL) {
        return -1;
    }

    len = snprintf((char*)http->buffer + http->buffer_offset, BUFFER_SIZE - http->buffer_offset,
            "%s: %d\r\n", key, value);
    if (len <= 0) {
        return -1;
    }

    http->buffer_offset += len;

    return 0;
}

int http_post(http_t *http, char *playload, int timeoutms)
{
    char *temp_buff;
    if ((temp_buff = strdup((char *)http->buffer)) == NULL) {
        return -1;
    }

    // temp_buff = strdup((char *)http->buffer);
    memset(http->buffer, 0, BUFFER_SIZE);

    snprintf((char *)http->buffer, BUFFER_SIZE, "POST %s HTTP/1.1\r\n", http->path);

    strcat((char *)http->buffer, temp_buff);

    strcat((char *)http->buffer, "\r\n");

    strcat((char *)http->buffer, playload);

    // LOGD(TAG, "http post sendbuffer: %s", http->buffer);

    aos_free(temp_buff);

    return http->net.net_write(&http->net, http->buffer, strlen((char*)http->buffer), timeoutms);
}

int http_get(http_t *http, int timeoutms)
{
    char *temp_buff;
    if ((temp_buff = strdup((char *)http->buffer)) == NULL) {
        return -1;
    }

    // temp_buff = strdup((char *)http->buffer);
    memset(http->buffer, 0, BUFFER_SIZE);

    snprintf((char *)http->buffer, BUFFER_SIZE, "GET %s HTTP/1.1\r\n", http->path);

    strcat((char *)http->buffer, temp_buff);

    strcat((char *)http->buffer, "\r\n");

    // LOGD(TAG, "http get sendbuffer: %s", http->buffer);

    aos_free(temp_buff);

    return http->net.net_write(&http->net, http->buffer, strlen((char*)http->buffer), timeoutms);
}

static int http_parse_head(http_t *http, char **head)
{
    char *content;
    char *endpoint;
    int content_len;
    char *code_str;
    int code;
    char *end_point;

    *head = strstr((char*)http->buffer, "\r\n\r\n");

    if (*head == NULL) {
        return 0;
    }

    *head += 4;

    // FIXME: server connection close, need reconnect
    if (strstr((char*)http->buffer, "Connection: close")) {
        return -2;
    }

    if ((content = strstr((char*)http->buffer, "Content-Length: ")) == NULL) {
        code_str = strstr((char *)http->buffer, "HTTP/1.1");

        code = atoi(code_str + strlen("HTTP/1.1"));
        if (code != 206 && code != 200) {
            LOGD(TAG, "http code :%d", code);
            return -1;
        }

        content_len = strtol(*head, &end_point, 16);

        if (end_point == NULL) {
            return 0;
        }
    } else {
        // content_len = atoi(content + strlen("Content-Length: "));
        content_len = strtol(content + strlen("Content-Length: "), &endpoint, 10);

        if (endpoint == NULL || *endpoint != '\r') {
            return -1;
        }
    }

    // LOGD(TAG, "http parse head: %s %d", http->buffer, content_len);
    return content_len;
}

static int net_quick_reconnect(http_t *http)
{
    LOGW(TAG, "i need reconnect http");
    http->net.net_disconncet(&http->net);
    if (http->net.net_connect(&http->net, http->host, http->port, SOCK_STREAM) != 0) {
        LOGE(TAG, "reconnect failed!");
        return -1;
    }
    return 0;
}

static int net_read(int fd, unsigned char *buffer, int len, int timeout_ms)
{
    struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 100)) {
        interval.tv_sec = 0;
        interval.tv_usec = 10000;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval,
                   sizeof(struct timeval))) {
        return -1;
    }

    int rc = recv(fd, buffer, len, 0);

    return rc;
}

int http_wait_resp(http_t *http, char **head_end, int timeoutms)
{
    int recv_len = 0;
    int len;
    int content_len;

again:
    recv_len = 0;
    *head_end = NULL;
    content_len = 0;
    memset(http->buffer, 0, BUFFER_SIZE);

    while(recv_len < BUFFER_SIZE) {
        int pre_read = BUFFER_SIZE - recv_len;
        if(pre_read > 512) {
            pre_read = 512;
        }

        len = net_read(http->net.fd, http->buffer + recv_len, pre_read, timeoutms);
        //printf("pre read %d\n", len);

        if (len <= 0) {
            LOGE(TAG, "net read len=%d errno=%d", len, errno);
            return -1;
        }

        recv_len += len;

        if ((content_len = http_parse_head(http, head_end)) == 0) {
            continue;
        }

        if (content_len < 0) {
            if (content_len == -2) {
                if (net_quick_reconnect(http) < 0) {
                    return 0;
                }
                return -2;
            }
            return -1;
        } else {
            break;
        }
    }

    if (content_len <= 0 && !(*head_end)) {
        goto again;
    }

    int head_len = (*head_end - (char *)http->buffer);
    int total_len = content_len + head_len;
    //int left_len = content_len + head_len - len;
    //printf("head len %d, left_len %d, total %d, recv len %d\n", head_len, left_len, total_len, recv_len);

    if ( total_len > BUFFER_SIZE ) {
        LOGE(TAG, "total len %d", total_len);
        return -1;
    }

    // LOGD(TAG, "buffer: %.*s", *head_end - (char*)http_ins->buffer, http_ins->buffer);

    while(recv_len < total_len) {
        if (content_len <= recv_len - (*head_end - (char *)http->buffer)) {
            break;
        }

        len = http->net.net_read(&http->net, http->buffer + recv_len, total_len - recv_len, timeoutms);

        //printf("left read %d\n", len);
        if (len <= 0) {
            LOGE(TAG, "net read len=%d errno=%d", len, errno);
            return -1;
        }

        recv_len += len;
    }

    return content_len;
}

char *http_head_get(http_t *http, char *key, int *length)
{
    char *temp_key;
    char *temp_value;
    char *temp_end;

    if ((temp_key = strstr((char *)http->buffer, key)) == NULL) {
        LOGD(TAG, "no key %s", key);
        return NULL;
    }

    if ((temp_value = strstr(temp_key, ":")) == NULL) {
        LOGD(TAG, "no delimiter");
        return NULL;
    }

    if ((temp_end = strstr(temp_value, "\r\n")) == NULL) {
        LOGD(TAG, "no end");
        return NULL;
    }

    *length = (int)(temp_end - temp_value - 2) >= 0 ? (int)(temp_end - temp_value - 2) : 0;// 1 space

    return temp_value + 1;
}

char *http_read_data(http_t *http)
{
    char *buffer;

    buffer = strstr((char *)http->buffer, "\r\n\r\n");

    if (buffer == NULL) {
        return NULL;
    }

    return buffer + 4;
}

int http_deinit(http_t *http)
{
    if (http) {
        http->net.net_disconncet(&http->net);

        if (http->url) aos_free(http->url);
        if (http->path) aos_free(http->path);
        if (http->host) aos_free(http->host);
        if (http->buffer) aos_free(http->buffer);
        aos_free(http);
    }

    return 0;
}
