/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_STREAMER_FILE) && CONFIG_STREAMER_FILE
#include "stream/stream_cls.h"
#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include "vfs.h"
#endif

#define TAG "s_file"

struct file_priv {
    int        fd;
    char       *path;
};

static char* _get_file_name(const char *url)
{
    int len, pre_len;
    char *p;
    char *path = NULL;

    pre_len = strlen("file://");
    p = strstr(url, "?");
    if (p) {
        len = p - url;
        if (len > pre_len) {
            path = strndup(url + pre_len, len - pre_len);
        }
    } else {
        path = strdup(url + pre_len);
    }

    return path;
}

/**
 * @brief
 * @param  [in] o
 * @param  [in] mode
 * url example: file://res/48k.m4a
 *              file://res/hello.raw?avformat=rawaudio&avcodec=pcm_s16le&channel=2&rate=44100
 * @return
 */
static int _stream_file_open(stream_cls_t *o, int mode)
{
    int ret = -1, fd;
#ifdef __linux__
    struct stat st;
#else
    struct aos_stat st;
#endif
    char *path = NULL;
    struct file_priv *priv = NULL;

    UNUSED(mode);
    priv = av_zalloc(sizeof(struct file_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    path = _get_file_name(o->url);
    CHECK_RET_TAG_WITH_GOTO(path, err);

    ret = aos_stat(path, &st);
    CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

    fd = aos_open(path, O_RDONLY);
    CHECK_RET_TAG_WITH_GOTO(fd > 0, err);

    priv->fd   = fd;
    priv->path = path;
    o->size    = st.st_size;
    o->priv    = priv;

    return 0;
err:
    LOGE(TAG, "open fail. ret = %d, url = %s", ret, o->url);
    av_free(path);
    av_free(priv);
    return -1;
}

static int _stream_file_close(stream_cls_t *o)
{
    struct file_priv *priv = o->priv;

    aos_close(priv->fd);
    av_free(priv->path);
    av_free(priv);
    o->priv = NULL;

    return 0;
}

static int _stream_file_read(stream_cls_t *o, uint8_t *buf, size_t count)
{
    struct file_priv *priv = o->priv;

    return  aos_read(priv->fd, buf, count);
}

static int _stream_file_write(stream_cls_t *o, const uint8_t *buf, size_t count)
{
    //TODO
    return -1;
}

static int _stream_file_seek(stream_cls_t *o, int32_t pos)
{
    struct file_priv *priv = o->priv;

    if (!((pos >= 0) && (pos <= o->size))) {
        return -1;
    }

    return aos_lseek(priv->fd, pos, 0);
}

static int _stream_file_control(stream_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO:
    return -1;
}

const struct stream_ops stream_ops_file = {
    .name            = "file",
    .type            = STREAM_TYPE_FILE,
    .protocols       = { "file", NULL },

    .open            = _stream_file_open,
    .close           = _stream_file_close,
    .read            = _stream_file_read,
    .write           = _stream_file_write,
    .seek            = _stream_file_seek,
    .control         = _stream_file_control,
};
#endif

