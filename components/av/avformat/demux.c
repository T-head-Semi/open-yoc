/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/misc.h"
#include "avutil/url_parse.h"
#include "avutil/path.h"
#include "stream/stream.h"
#include "avformat/avformat_utils.h"
#include "avformat/demux.h"
#include "avformat/avparser.h"

#define TAG                    "demux"

static struct {
    int                    cnt;
    const struct demux_ops *ops[DEMUX_OPS_MAX];
} g_demuxers;

#if 0
static const struct demux_ops* _get_demux_ops_by_type(uint8_t type)
{
    int i;

    for (i = 0; i < g_demuxers.cnt; i++) {
        if (g_demuxers.ops[i]->type == type) {
            return g_demuxers.ops[i];
        }
    }

    LOGE(TAG, "get demux ops err, type = %d\n", type);

    return NULL;
}

static const struct demux_ops* _get_demux_ops_by_ext(const char *ext)
{
    int i, j;

    for (i = 0; i < g_demuxers.cnt; i++) {
        const char *const*exts = g_demuxers.ops[i]->extensions;

        for (j = 0; exts[j]; j++) {
            if (strcasecmp(ext, exts[j]) == 0) {
                return g_demuxers.ops[i];
            }
        }
    }

    LOGE(TAG, "get demux ops err, ext = %s\n", ext);
    return NULL;
}

#endif

/**
 * @brief  regist demux ops
 * @param  [in] ops
 * @return 0/-1
 */
int demux_ops_register(const struct demux_ops *ops)
{
    int i;

    if (ops && (g_demuxers.cnt < DEMUX_OPS_MAX)) {
        for (i = 0; i < g_demuxers.cnt; i++) {
            if (strcmp(ops->name, g_demuxers.ops[i]->name) == 0) {
                /* replicate  */
                break;
            }
        }

        if (i == g_demuxers.cnt) {
            g_demuxers.ops[g_demuxers.cnt] = ops;
            g_demuxers.cnt++;
        }
        return 0;
    }

    LOGE(TAG, "demux ops regist fail\n");
    return -1;
}

const struct demux_ops* _demux_probe_ops(const avprobe_data_t *pd)
{
    int i, j;
    int score, score_max = 0;
    const char *ext;
    const char *const*exts;
    const struct demux_ops *ops = NULL, *ops_ok = NULL;

    ext = get_extension(pd->filename);

    //FIXME
    for (i = 0; i < g_demuxers.cnt; i++) {
        ops  = g_demuxers.ops[i];
        exts = ops->extensions;

        score = ops->read_probe ? ops->read_probe(pd) : 0;
        if (score > score_max) {
            /*  AVPROBE_SCORE_MAX */
            ops_ok    = ops;
            score_max = score;
            if (score == AVPROBE_SCORE_MAX)
                goto find;
        }

        if (strcasecmp(pd->avformat, ops->name) == 0) {
            score = AVPROBE_SCORE_NAME;
            if (score > score_max) {
                ops_ok    = ops;
                score_max = score;
                if (score == AVPROBE_SCORE_MAX)
                    goto find;
            }
        }

        if (ext) {
            for (j = 0; exts[j]; j++) {
                if (strncasecmp(ext, exts[j], strlen(exts[j])) == 0) {
                    score = AVPROBE_SCORE_EXTENSION;
                    if (score > score_max) {
                        ops_ok    = ops;
                        score_max = score;
                        if (score == AVPROBE_SCORE_MAX)
                            goto find;
                    }
                }
            }
        }
    }

find:
    return ops_ok;
}

/**
 * @brief  open/create one demux
 * @param  [in] s : stream
 * @return
 */
demux_cls_t* demux_open(stream_cls_t *s)
{
    int rc, isize;
    size_t format_size;
    avprobe_data_t pd;
    uint8_t *buf        = NULL;
    demux_cls_t *o      = NULL;
    track_info_t *track = NULL;
    const struct demux_ops *ops;

    CHECK_PARAM(s, NULL);
    buf = av_malloc(CONFIG_AV_PROBE_SIZE_MAX);
    CHECK_RET_TAG_WITH_RET(buf, NULL);
    track = track_info_new();
    track->type = AVMEDIA_TYPE_AUDIO;

    memset(&pd, 0, sizeof(avprobe_data_t));
    format_size = sizeof(pd.avformat);
    rc = stream_read(s, buf, 10);
    CHECK_RET_TAG_WITH_GOTO(rc == 10, err);

    /* skip the id3 info, useless now */
    isize = get_id3v2_size(buf, 10);
    if (isize > 0) {
        LOGD(TAG, "id3v2 size = %d", isize);
        rc = stream_seek(s, isize, SEEK_SET);
        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
        pd.buf_size = stream_read(s, buf, CONFIG_AV_PROBE_SIZE_MAX);
        CHECK_RET_TAG_WITH_GOTO(pd.buf_size > 0, err);
    } else {
        rc = stream_read(s, buf + 10, CONFIG_AV_PROBE_SIZE_MAX - 10);
        CHECK_RET_TAG_WITH_GOTO(rc > 0, err);
        pd.buf_size = rc + 10;
    }
    pd.buf      = buf;
    pd.filename = stream_get_url(s);
    url_get_item_value(stream_get_url(s), "avformat", pd.avformat, sizeof(pd.avformat));

    ops = _demux_probe_ops(&pd);
    if (NULL == ops) {
        LOGE(TAG, "can't find suitable demux type. %s", stream_get_url(s));
        goto err;
    }
    LOGI(TAG, "find a demux, name = %s, url = %s", ops->name, stream_get_url(s));

    stream_seek(s, isize, SEEK_SET);
    o = av_zalloc(sizeof(demux_cls_t));
    CHECK_RET_TAG_WITH_GOTO(o, err);
    avpacket_init(&o->fpkt);
    slist_init(&o->list_free);
    slist_init(&o->list_ready);

    o->s   = s;
    o->ops = ops;
    rc = ops->open(o);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    o->psr = avparser_open(o->ash.id, o->ash.extradata, o->ash.extradata_size);
    o->bps = o->duration > 0 ? stream_get_size(s) * 8 / (o->duration / 1000.0) : 0;

    track->codec_id   = o->ash.id;
    track->codec_name = strdup(get_codec_name(track->codec_id));
    track->duration   = o->duration;
    //FIXME: bps same with media bps
    track->bps        = o->bps;
    track->t.a.sf     = o->ash.sf;
    tracks_info_add(&o->tracks, track);
    aos_mutex_new(&o->lock);
    av_free(buf);

    return o;
err:
    track_info_free(track);
    av_free(buf);
    if (o) {
        avparser_close(o->psr);
        avpacket_free(&o->fpkt);
        av_free(o);
    }
    return NULL;
}

/**
 * @brief  read packet from the demux
 * @param  [in] o
 * @param  [in] pkt
 * @return -1 on err
 */
int demux_read_packet(demux_cls_t *o, avpacket_t *pkt)
{
    int rc = 0, eof;
    slist_t *tmp;
    avparser_t *psr;
    avpacket_t *psr_pkt;

    CHECK_PARAM(o && pkt, -1);
    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    psr      = o->psr;
    pkt->len = 0;

read_try:
    if (slist_empty(&o->list_ready)) {
        if (o->fpkt.len) {
            //FIXME: the first packet already readed
            rc = avpacket_copy(&o->fpkt, pkt);
            CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
            avpacket_free(&o->fpkt);
        } else {
            rc = o->ops->read_packet(o, pkt);
            if (rc < 0) {
                eof = stream_is_eof(o->s);
                rc  = eof ? 0 : rc;
                LOGI(TAG, "read packet may be eof. eof = %d, rc = %d, url = %s", eof, rc, stream_get_url(o->s));
                goto err;
            }
        }

        if (psr && rc >= 0 && pkt->len > 0) {
            uint8_t *obuf;
            size_t osize, isize = pkt->len, ipos = 0, offset = 0;

            while (isize > 0) {
                rc = avparser_parse(psr, pkt->data + offset, isize, &ipos, &obuf, &osize);
                CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

                if (slist_empty(&o->list_free)) {
                    psr_pkt = av_zalloc(sizeof(avpacket_t));
                    rc = avpacket_new(psr_pkt, osize);
                    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
                } else {
                    slist_for_each_entry_safe(&o->list_free, tmp, psr_pkt, avpacket_t, node) {
                        rc = avpacket_grow(psr_pkt, osize);
                        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

                        slist_del(&psr_pkt->node, &o->list_free);
                        break;
                    }
                }
                psr_pkt->len = osize;
                psr_pkt->pts = pkt->pts;
                memcpy(psr_pkt->data, obuf, osize);
                slist_add_tail(&psr_pkt->node, &o->list_ready);

                offset += ipos;
                isize  -= ipos;
            }

            goto read_try;
        }
    } else {
        slist_for_each_entry_safe(&o->list_ready, tmp, psr_pkt, avpacket_t, node) {
            rc = avpacket_grow(pkt, psr_pkt->len);
            CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

            slist_del(&psr_pkt->node, &o->list_ready);
            break;
        }

        pkt->len = psr_pkt->len;
        pkt->pts = psr_pkt->pts;
        memcpy(pkt->data, psr_pkt->data, psr_pkt->len);
        slist_add_tail(&psr_pkt->node, &o->list_free);
    }

    rc = pkt->len;
err:
    aos_mutex_unlock(&o->lock);

    return rc;
}

static void _free_psr_packets(demux_cls_t *o)
{
    slist_t *tmp;
    avpacket_t *psr_pkt;

    if (!slist_empty(&o->list_ready)) {
        slist_for_each_entry_safe(&o->list_ready, tmp, psr_pkt, avpacket_t, node) {
            slist_del(&psr_pkt->node, &o->list_ready);
            avpacket_free(psr_pkt);
            av_free(psr_pkt);
        }
    }

    if (!slist_empty(&o->list_free)) {
        slist_for_each_entry_safe(&o->list_free, tmp, psr_pkt, avpacket_t, node) {
            slist_del(&psr_pkt->node, &o->list_free);
            avpacket_free(psr_pkt);
            av_free(psr_pkt);
        }
    }
}

/**
 * @brief  seek the demux
 * @param  [in] o
 * @param  [in] timestamp : seek time(ms)
 * @return 0/-1
 */
int demux_seek(demux_cls_t *o, uint64_t timestamp)
{
    int ret = -1;

    CHECK_PARAM(o && timestamp >= 0, -1);
    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    if (o->time_scale && stream_is_seekable(o->s)) {
        if (timestamp < o->duration) {
            slist_t *tmp;
            avpacket_t *psr_pkt;
            int32_t start_pos = stream_tell(o->s);

            //FIXME: start play time is not zero, release the first packet
            if (o->fpkt.len && timestamp) {
                avpacket_free(&o->fpkt);
            }

            ret = o->ops->seek(o, timestamp / 1000.0 * o->time_scale);
            if (ret < 0) {
                stream_seek(o->s, start_pos, SEEK_SET);
            } else {
                if (!slist_empty(&o->list_ready)) {
                    slist_for_each_entry_safe(&o->list_ready, tmp, psr_pkt, avpacket_t, node) {
                        slist_del(&psr_pkt->node, &o->list_ready);
                        slist_add_tail(&psr_pkt->node, &o->list_free);
                    }
                }
            }
        } else {
            AV_ERRNO_SET(AV_ERRNO_SEEK_FAILD);
        }
    }

    if (ret < 0) {
        LOGE(TAG, "may be not support seek. timestamp = %llums, duration = %llums, url = %s", timestamp, o->duration,
             stream_get_url(o->s));
    }
    aos_mutex_unlock(&o->lock);

    return ret;
}

/**
 * @brief  close/destroy a demux
 * @param  [in] o
 * @return 0/-1
 */
int demux_close(demux_cls_t *o)
{
    int ret = -1;

    CHECK_PARAM(o, -1);
    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    ret = o->ops->close(o);
    aos_mutex_unlock(&o->lock);

    aos_mutex_free(&o->lock);
    _free_psr_packets(o);
    avparser_close(o->psr);
    avpacket_free(&o->fpkt);
    tracks_info_freep(&o->tracks);
    av_free(o->ash.extradata);
    av_free(o);

    return ret;
}

/**
 * @brief  control a demux
 * @param  [in] o
 * @param  [in] cmd : command
 * @param  [in] arg
 * @param  [in/out] arg_size
 * @return
 */
int demux_control(demux_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    int ret = -1;

    CHECK_PARAM(o, -1);
    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    ret = o->ops->control ? o->ops->control(o, cmd, arg, arg_size) : -1;
    if (ret < 0) {
        LOGE(TAG, "control failed");
    }
    aos_mutex_unlock(&o->lock);

    return ret;
}


