/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_STREAMER_HTTP) && CONFIG_STREAMER_HTTP
#include "stream/stream.h"
#include "avutil/socket_rw.h"
#include "avutil/web.h"

#define TAG                    "s_http"

struct http_priv {
    wsession_t *session;
};

static int _stream_http_open(stream_cls_t *o, int mode)
{
    int rc;
    const char *val;
    wsession_t *session;
    struct http_priv *priv;

    UNUSED(mode);
    priv = av_zalloc(sizeof(struct http_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    session = wsession_create();
    CHECK_RET_TAG_WITH_GOTO(session, err);

    rc = wsession_get(session, o->url, 3);
    if (rc) {
        LOGE(TAG, "wsession_get fail. rc = %d, code = %d, phrase = %s", rc, session->code, (session->phrase ? session->phrase : "NULL"));
        goto err;
    }

    val = dict_get_val(&session->hdrs, "Content-Length");
    CHECK_RET_TAG_WITH_GOTO(val, err);

    priv->session   = session;
    o->size         = atoi(val);
    o->priv         = priv;
    o->enable_cache = 1;

    return 0;
err:
    wsession_destroy(session);
    av_free(priv);
    return -1;
}

static int _stream_http_close(stream_cls_t *o)
{
    struct http_priv *priv = o->priv;
    wsession_t *session = priv->session;

    if (session) {
        wsession_close(session);
        wsession_destroy(session);
    }

    av_free(priv);
    o->priv = NULL;
    return 0;
}

static int _stream_http_read(stream_cls_t *o, uint8_t *buf, size_t count)
{
    int rc;
    struct http_priv *priv = o->priv;
    int min_timeout = 1000, cnt, retry_cnt = 0;

    cnt = o->rcv_timeout / min_timeout;
    cnt = cnt > 0 ? cnt : 1;

retry:
    rc = wsession_read(priv->session, (char*)buf, count, min_timeout);
    if (rc < 0) {
        AV_ERRNO_SET(AV_ERRNO_READ_FAILD);
    } else if (rc == 0) {
        if (!stream_is_interrupt(o) && retry_cnt++ < cnt) {
            goto retry;
        }
    }

    return rc;
}

static int _stream_http_write(stream_cls_t *o, const uint8_t *buf, size_t count)
{
    //TODO
    return -1;
}

static int _stream_http_seek(stream_cls_t *o, int32_t pos)
{
    int rc = -1;
    struct http_priv *priv = o->priv;
    wsession_t *session = priv->session;

    if (session) {
        wsession_close(session);
        rc = wsession_get_range(session, o->url, 3, pos, -1);
        if (rc < 0) {
            LOGE(TAG, "http seek: wsession_get range fail. rc = %d, code = %d, phrase = %s", rc, session->code, (session->phrase ? session->phrase : "NULL"));
        }
    }

    return rc;
}

static int _stream_http_control(stream_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO:
    return -1;
}

const struct stream_ops stream_ops_http = {
    .name            = "http",
    .type            = STREAM_TYPE_HTTP,
    .protocols       = { "http", "https", NULL },

    .open            = _stream_http_open,
    .close           = _stream_http_close,
    .read            = _stream_http_read,
    .write           = _stream_http_write,
    .seek            = _stream_http_seek,
    .control         = _stream_http_control,
};
#endif

