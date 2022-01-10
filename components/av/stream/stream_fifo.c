/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_STREAMER_FIFO) && CONFIG_STREAMER_FIFO
#include "stream/stream_cls.h"
#include "avutil/named_straightfifo.h"
#include "avutil/url_parse.h"

#define TAG                    "s_fifo"

struct sfifo_priv {
    nsfifo_t          *fifo;
};

/* url example: fifo://tts/1?avformat=rawaudio&avcodec=pcm_s16le&channel=2&rate=44100&size=1024 */
static int _stream_fifo_open(stream_cls_t *o, int mode)
{
    int size;
    nsfifo_t *fifo          = NULL;
    struct sfifo_priv *priv = NULL;

    UNUSED(mode);
    priv = av_zalloc(sizeof(struct sfifo_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    fifo = nsfifo_open(o->url, O_RDONLY);
    if (NULL == fifo) {
        LOGE(TAG, "named fifo open fail: %s", o->url);
        goto err;
    }

    url_get_item_value_int(o->url, "size", &size);

    priv->fifo = fifo;
    o->priv    = priv;
    o->size    = size;
    return 0;
err:
    if (fifo)
        nsfifo_close(fifo);
    av_free(priv);

    return -1;
}

static int _stream_fifo_close(stream_cls_t *o)
{
    struct sfifo_priv *priv = o->priv;
    nsfifo_t *fifo = priv->fifo;

    nsfifo_set_eof(fifo, 1, 0);
    nsfifo_close(fifo);

    av_free(priv);
    o->priv = NULL;

    return 0;
}

static int _stream_fifo_read(stream_cls_t *o, uint8_t *buf, size_t count)
{
    int len = 0, rlen;
    int left = count;
    char *pos;
    uint8_t weof = 0;
    struct sfifo_priv *priv = o->priv;
    nsfifo_t *fifo          = priv->fifo;

    while (left > 0) {
        rlen = nsfifo_get_rpos(fifo, &pos, (o->rcv_timeout == AOS_WAIT_FOREVER) ? 2 * 1000 : o->rcv_timeout);
        if (o->irq.handler && o->irq.handler(o->irq.arg)) {
            LOGE(TAG, "interrupt quit");
            break;
        }
        if (rlen <= 0) {
            nsfifo_get_eof(fifo, NULL, &weof);
            if (weof)
                break;

            if (o->rcv_timeout == AOS_WAIT_FOREVER)
                continue;
            else
                break;
        }
        rlen = rlen < left ? rlen : left;
        memcpy(buf + len, pos, rlen);
        len += rlen;
        left -= rlen;
        nsfifo_set_rpos(fifo, rlen);
    }

    return len;
}

static int _stream_fifo_write(stream_cls_t *o, const uint8_t *buf, size_t count)
{
    //TODO
    return -1;
}

#if 0
static int _stream_fifo_seek(stream_cls_t *o, int32_t pos)
{
    //TODO
    return -1;
}
#endif

static int _stream_fifo_control(stream_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO
    return -1;
}

const struct stream_ops stream_ops_fifo = {
    .name            = "fifo",
    .type            = STREAM_TYPE_FIFO,
    .protocols       = { "fifo", "ififo", NULL },

    .open            = _stream_fifo_open,
    .close           = _stream_fifo_close,
    .read            = _stream_fifo_read,
    .write           = _stream_fifo_write,
    //.seek            = _stream_fifo_seek,
    .control         = _stream_fifo_control,
};
#endif

