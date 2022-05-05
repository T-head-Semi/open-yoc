/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <string.h>
#include <aos/list.h>
#include <aos/kernel.h>
#include <yoc/netio.h>
#include <ulog/ulog.h>

#define TAG "fota"
typedef struct fota_netio_list {
    slist_t next;
    const netio_cls_t *cls;
} netio_node_t;

static AOS_SLIST_HEAD(netio_cls_list);

int netio_register(const netio_cls_t *cls)
{
    netio_node_t *node = aos_malloc(sizeof(netio_node_t));

    if (node) {
        node->cls = cls;
        slist_add_tail(&node->next, &netio_cls_list);
        return 0;
    }

    return -1;
}

netio_t *netio_open(const char *path)
{
    netio_t *io = NULL;
    char *delim = strstr(path, "://");
    LOGD(TAG, "path:%s delim:%s\n", path, delim);
    if (delim) {
        int len = delim - path;
        netio_node_t *node;

        if (strstr(path, "https")) {
            len -= 1;
        }

        slist_for_each_entry(&netio_cls_list, node, netio_node_t, next) {
            if (strncmp(node->cls->name, path, len) == 0) {
                if (node->cls->open) {
                    io = aos_zalloc(sizeof(netio_t));
                    io->cls = node->cls;
                    if (io->cls->open(io, path) < 0) {
                        aos_free(io);
                        LOGD(TAG, "open fail\n");
                        return NULL;
                    }
                }
                LOGD(TAG, "open break,%s\n", node->cls->name);
                break;
            }
        }
    }

    return io;
}

int netio_close(netio_t *io)
{
    int ret = -1;
    if (io->cls->close) {
        ret = io->cls->close(io);
        aos_free(io);
    }

    return ret;
}

int netio_read(netio_t *io, uint8_t *buffer, size_t lenght, int timeoutms)
{
    if (io->cls->read)
        return io->cls->read(io, buffer, lenght, timeoutms);

    return -1;
}

int netio_write(netio_t *io, uint8_t *buffer, size_t lenght, int timeoutms)
{
    if (io->cls->write)
        return io->cls->write(io, buffer, lenght, timeoutms);

    return -1;
}

int netio_seek(netio_t *io, size_t offset, int whence)
{
    if (io->cls->seek)
        return io->cls->seek(io, offset, whence);

    return -1;
}
