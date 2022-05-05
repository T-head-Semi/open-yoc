/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../http/http.h"
#include <yoc/fota.h>
#include <yoc/netio.h>
#include <aos/kernel.h>
#include "util/network.h"
#include "ulog/ulog.h"

#define TAG "fota"

static int http_read(netio_t *io, uint8_t *buffer, int length, int timeoutms)
{
    int content_len;
    char *head_end;
    http_t *http = (http_t*)io->private;

    if (io->offset >= io->size) {
        LOGD(TAG, "http_read done: %d %d", io->size, io->offset);
        return 0;
    }

    memset(http->buffer, 0, BUFFER_SIZE);
    http->buffer_offset = 0;
    http_head_sets(http, "Host", http->host);

    int range_end = io->offset + io->block_size - 1;
    char *range;
    range = aos_zalloc(56);
    if (io->size > 0) {
        range_end = range_end < (io->size - 1)? range_end : (io->size - 1);
    }
    snprintf(range, 56, "bytes=%zu-%d", io->offset, range_end);
    http_head_sets(http, "Range", range);
    aos_free(range);
    http_head_sets(http, "Connection", "keep-alive");
    http_head_sets(http, "Cache-Control", "no-cache");

    http_get(http, 10000);

    if ((content_len = http_wait_resp(http, &head_end, 10000)) < 0) {
        LOGE(TAG, "recv failed: %d", content_len);
        return content_len;
    }

    if (content_len != (range_end - io->offset + 1)) {
        LOGE(TAG, "content_len overflow :%d", content_len);
        return -1;
    }
    // LOGD(TAG, "recv sucess: %d", content_len);
    io->offset += content_len;

    memcpy(buffer, head_end, content_len);

    return content_len;
}

static int http_open(netio_t *io, const char *path)
{
    http_t *http;
    if ((http = http_init(path)) == NULL) {
        LOGD(TAG, "e http init");
        return -1;
    }

    io->offset = 0;
    io->block_size = CONFIG_FOTA_BUFFER_SIZE;// 1024
    // io->private = http;

    int content_len;
    char *range;
    char *head_end;
    memset(http->buffer, 0, BUFFER_SIZE);
    http->buffer_offset = 0;
    http_head_sets(http, "Host", http->host);

    int range_end = io->offset + io->block_size - 1;

    range = (void *)aos_zalloc(56);
    if (io->size > 0) {
        range_end = range_end < (io->size - 1)? range_end : (io->size - 1);
    }
    snprintf(range, 56, "bytes=%zu-%d", io->offset, range_end);
    http_head_sets(http, "Range", range);
    aos_free(range);
    http_head_sets(http, "Connection", "keep-alive");
    http_head_sets(http, "Cache-Control", "no-cache");

    http_get(http, 10000);

    if ((content_len = http_wait_resp(http, &head_end, 10000)) < 0) {
        LOGD(TAG, "recv failed: %d", content_len);
        return -1;
    }

    if ((range = strstr((char *)http->buffer, "Content-Range: bytes ")) == NULL) {
        LOGD(TAG, "no Content-Range");
        return -1;
    }

    io->size = atoi(strchr(range, '/') + 1);

    LOGD(TAG, "range_len: %d", io->size);

    io->private = http;

    return 0;
}

static int http_seek(netio_t *io, size_t offset, int whence)
{
    // http_t *http = (http_t*)io->private;

    io->offset = offset;

    return 0;
}

static int http_close(netio_t *io)
{
    http_t *http = (http_t*)io->private;

    return http_deinit(http);
}

const netio_cls_t http_cls = {
    .name = "http",
    .read = http_read,
    .seek = http_seek,
    .open = http_open,
    .close = http_close,
    // .private = http,
    // .getinfo = http_getinfo,
};

int netio_register_http(void)
{
    return netio_register(&http_cls);
}
