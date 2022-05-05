/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_STREAMER_HLS) && CONFIG_STREAMER_HLS
#include "avutil/misc.h"
#include "avutil/bio.h"
#include "avutil/url_parse.h"
#include "avutil/dync_buf.h"
#include "stream/stream.h"

#ifdef __linux__
#include "aos_port/list.h"
#else
#include <aos/list.h>

#define LIST_TASK_QUIT_EVT     (0x01)
#define LIST_SEG_UPDATE_EVT    (0x02)
#endif

#define TAG                    "s_hls"

#define list_lock()            (aos_mutex_lock(&priv->lock, AOS_WAIT_FOREVER))
#define list_unlock()          (aos_mutex_unlock(&priv->lock))

struct seg_node {
    char                       *url;                        ///< segment play url
    uint32_t                   duration;                    ///< ms
    slist_t                    node;
};

struct seg_info {
    uint8_t                    is_end;                      ///< whether with endlist
    int                        nb_segs;
    int                        sequence;
    uint32_t                   dur_max;                     ///< ms
    uint32_t                   dur_total;                   ///< ms
    slist_t                    list;
};

struct xstm_node {
    //TODO:
    char                       *url;                        ///<  mul-rate play url
    slist_t                    node;
};

struct hls_priv {
    stm_conf_t                 stm_cnf;
    stream_cls_t               *rs;                         ///< real stream
    char                       *seg_url;                    ///< segment url playing current
    char                       *rate_url;                   ///< rate url playing current
    uint8_t                    eof;
    uint8_t                    is_end;                      ///< is_end
    uint8_t                    is_live;                     ///< live or vod

    int                        last_sequence;
    uint32_t                   dur_total;                   ///< ms
    uint32_t                   flush_interval;              ///< may be for live, ms. get new play-list per interval-ms
    slist_t                    seg_plist;                   ///< segment playlist
    slist_t                    master_plist;                ///< master playlist

    aos_task_t                 list_task;
#ifdef __linux__
    pthread_cond_t             cond;
#else
    aos_event_t                evt;
#endif
    aos_mutex_t                lock;                        ///< lock for play list
};

static int _is_interrupt(struct hls_priv *priv)
{
    int rc = 0;

    if (priv) {
        stm_conf_t *stm_cnf = &priv->stm_cnf;

        if (priv->eof || (stm_cnf->irq.handler && stm_cnf->irq.handler(stm_cnf->irq.arg))) {
            LOGI(TAG, "interrupt");
            rc = 1;
        }
    }

    return rc;
}

static void _free_seg_plist(slist_t *list)
{
    slist_t *tmp;
    struct seg_node *seg;

    slist_for_each_entry_safe(list, tmp, seg, struct seg_node, node) {
        slist_del(&seg->node, list);
        av_free(seg->url);
        av_free(seg);
    }
}

static void _free_master_plist(slist_t *list)
{
    slist_t *tmp;
    struct xstm_node *xs;

    slist_for_each_entry_safe(list, tmp, xs, struct xstm_node, node) {
        slist_del(&xs->node, list);
        av_free(xs->url);
        av_free(xs);
    }
}

#if 0
static uint64_t _get_total_duration(slist_t *seg_plist)
{
    uint64_t sum = 0;
    struct seg_node *seg;

    slist_for_each_entry(seg_plist, seg, struct seg_node, node) {
        sum += seg->duration;
    }

    return sum;
}
#endif

static int _get_play_url(struct hls_priv *priv, uint32_t timeout)
{
    int rc;
    char *url = NULL;
    struct seg_node *seg;
    char *seg_url = priv->seg_url;

retry:
    list_lock();
    if (!slist_empty(&priv->seg_plist)) {
        if (seg_url) {
            slist_for_each_entry(&priv->seg_plist, seg, struct seg_node, node) {
                if (strcmp(seg_url, seg->url) == 0) {
                    if (seg->node.next) {
                        seg = aos_container_of(seg->node.next, struct seg_node, node);
                        url = strdup(seg->url);
                    } else {
                        if (priv->is_live) {
#ifdef __linux__
                            {
                                uint64_t nsec;
                                struct timespec ts;
                                struct timeval now;

                                gettimeofday(&now, NULL);
                                nsec       = now.tv_usec * 1000 + (timeout % 1000) * 1000000;
                                ts.tv_nsec = nsec % 1000000000;
                                ts.tv_sec  = now.tv_sec + nsec / 1000000000 + timeout / 1000;
                                rc = pthread_cond_timedwait(&priv->cond, &priv->lock, &ts);
                                list_unlock();
                            }
#else
                            {
                                unsigned int flag;

                                list_unlock();
                                rc = aos_event_get(&priv->evt, LIST_SEG_UPDATE_EVT, AOS_EVENT_OR_CLEAR, &flag, timeout);
                            }
#endif
                            if (rc != 0) {
                                return -1;
                            }
                            goto retry;
                        } else {
                            LOGI(TAG, "may be the last seg-url: %s", seg_url);
                        }
                    }
                    /* get the next url or seg_url is the last one */
                    goto quit;
                }
            }
        }

        seg = aos_container_of(priv->seg_plist.next, struct seg_node, node);
        url = strdup(seg->url);
    }

quit:
    av_freep(&priv->seg_url);
    priv->seg_url = url;
    list_unlock();

    return url ? 0 : -1;
}

static char* _get_real_url(const char *hls_url, const char *seg_url)
{
    char *url, *p;

    if (is_file_url(seg_url) || is_http_url(seg_url) || is_https_url(seg_url)) {
        /* absolute address */
        return strdup(seg_url);
    }

    url = av_zalloc(strlen(hls_url) + strlen(seg_url) + 1);
    if (url) {
        //FIXME:
        if (strncmp(seg_url, "//", 2) == 0) {
            p = strstr(hls_url, "://");
            CHECK_RET_TAG_WITH_GOTO(p, err);
        } else {
            p = strrchr(hls_url, '/');
            CHECK_RET_TAG_WITH_GOTO(p, err);
        }
        strncpy(url, hls_url, p - hls_url + 1);
        strlcpy(url + strlen(url), seg_url, strlen(seg_url));
    }

    return url;
err:
    av_free(url);
    return NULL;
}

static int _parse_master_plist(slist_t *list, const uint8_t *buf)
{
    char ch;
    bio_t bio;
    dync_buf_t dbuf;
    struct xstm_node *xs;
    int rc, eof = 0, xs_next = 0;

    slist_init(list);
    bio_init(&bio, (uint8_t*)buf, strlen((const char*)buf));
    dync_buf_init(&dbuf, 128, 0);

    while (!eof) {
        rc = bio_read(&bio, (uint8_t*)&ch, 1);
        if (rc != 1) {
            eof = 1;
            if (strlen(dbuf.data))
                goto parse;
            else
                break;
        }
        if (ch != '\n') {
            dync_buf_add_char(&dbuf, ch);
            continue;
        }
parse:
        if (strncasecmp(dbuf.data, "#EXT-X-STREAM-INF", 17) == 0) {
            if (!xs_next) {
                //TODO:
                xs_next = 1;
            }
        } else {
            //FIXME:
            if (xs_next && strlen(dbuf.data)) {
                xs           = av_zalloc(sizeof(struct xstm_node));
                xs->url      = strdup(dbuf.data);
                slist_add_tail(&xs->node, list);
                xs_next = 0;
            }
        }

        dync_buf_reset(&dbuf);
    }

    rc = slist_empty(list) ? -1 : 0;
    dync_buf_uninit(&dbuf);

    return rc;
}

static int _parse_seg_plist(struct seg_info *psinfo, const uint8_t *buf)
{
    char ch;
    bio_t bio;
    dync_buf_t dbuf;
    struct seg_node *seg;
    float dur_max = 0, dur = 0;
    int rc, eof = 0, seg_next = 0, ret = -1;

    dync_buf_init(&dbuf, 128, 0);
    bio_init(&bio, (uint8_t*)buf, strlen((const char*)buf));

    while (!eof) {
        rc = bio_read(&bio, (uint8_t*)&ch, 1);
        if (rc != 1) {
            eof = 1;
            if (strlen(dbuf.data))
                goto parse;
            else
                break;
        }
        if (ch != '\n') {
            if (ch != '\r')
                dync_buf_add_char(&dbuf, ch);
            continue;
        }

parse:
        if (strncasecmp(dbuf.data, "#EXT-X-TARGETDURATION", 21) == 0) {
            rc = sscanf(dbuf.data, "%*[^:]:%f", &dur_max);
            CHECK_RET_TAG_WITH_GOTO(rc == 1, err);
            psinfo->dur_max = dur_max * 1000;
        } else if (strncasecmp(dbuf.data, "#EXT-X-MEDIA-SEQUENCE", 21) == 0) {
            rc = sscanf(dbuf.data, "%*[^:]:%d", &psinfo->sequence);
            CHECK_RET_TAG_WITH_GOTO(rc == 1, err);
        } else if (strncasecmp(dbuf.data, "#EXTINF", 7) == 0) {
            if (!seg_next) {
                rc = sscanf(dbuf.data, "%*[^:]:%f", &dur);
                CHECK_RET_TAG_WITH_GOTO(rc == 1, err);
                seg_next = 1;
            }
        } else if (strncasecmp(dbuf.data, "#EXT-X-ENDLIST", 14) == 0) {
            eof           = 1;
            psinfo->is_end = 1;
            break;
        } else {
            //FIXME:
            if (seg_next && strlen(dbuf.data)) {
                seg           = av_zalloc(sizeof(struct seg_node));
                seg->url      = strdup(dbuf.data);
                seg->duration = dur * 1000;  ///< ms
                slist_add_tail(&seg->node, &psinfo->list);

                psinfo->nb_segs++;
                psinfo->dur_total += seg->duration;
                seg_next = 0;
            }
        }

        dync_buf_reset(&dbuf);
    }
    ret = 0;

err:
    if (ret < 0) {
        _free_seg_plist(&psinfo->list);
    }
    dync_buf_uninit(&dbuf);
    return ret;
}

static int _get_play_list(struct hls_priv *priv, const char *url)
{
    stm_conf_t *stm_cnf;
    uint8_t *buf    = NULL;
    stream_cls_t *s = NULL;
    int rc, ret = -1, fsize;
    struct xstm_node *xs;
    struct seg_info sinfo, *psinfo = &sinfo;

    stm_cnf = &priv->stm_cnf;
    memset(psinfo, 0, sizeof(struct seg_info));
retry:
    s = stream_open(url, stm_cnf);
    fsize = stream_get_size(s);
    if (fsize <= 0) {
        LOGE(TAG, "s = %p, fsize = %d, url = %s", s, fsize, url);
        goto err;
    }
    buf = av_zalloc(fsize + 1);
    rc = stream_read(s, buf, fsize);
    if (rc != fsize) {
        LOGE(TAG, "buf = %p, rc = %d, fsize = %d", buf, rc, fsize);
        goto err;
    }

    if (strncasecmp((const char*)buf, "#EXTM3U", 7)) {
        LOGE(TAG, "not m3u");
        goto err;
    }
    if (strstr((const char*)buf, "#EXT-X-STREAM-INF")) {
        if (slist_empty(&priv->master_plist)) {
            rc = _parse_master_plist(&priv->master_plist, buf);
            CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
            //FIXME: get the first rate-url
            xs = aos_container_of(priv->master_plist.next, struct xstm_node, node);
            priv->rate_url = strdup(xs->url);
            url            = priv->rate_url;
            stream_close(s);
            av_freep((char**)&buf);
            goto retry;
        } else {
            LOGE(TAG, "have master play list already");
            goto err;
        }
    }

    rc = _parse_seg_plist(psinfo, buf);
    if (rc != 0) {
        LOGE(TAG, "_parse_seg_plist err, buf = %s.", buf);
        goto err;
    }

    list_lock();
    if (!slist_empty(&psinfo->list) && (priv->last_sequence != psinfo->sequence)) {
        if (slist_empty(&priv->seg_plist)) {
            //FIXME: first judge
            priv->is_live        = !psinfo->is_end;
            priv->flush_interval = psinfo->dur_max;
            priv->flush_interval = priv->flush_interval ? priv->flush_interval : 5000;
        } else {
            /* free old seg list */
            _free_seg_plist(&priv->seg_plist);
        }
        priv->is_end = psinfo->is_end;
        LOGD(TAG, "nb_segs = %d, live = %d, end = %d", psinfo->nb_segs, priv->is_live, psinfo->is_end);
        memcpy(&priv->seg_plist, &psinfo->list, sizeof(slist_t));
#ifdef __linux__
        pthread_cond_signal(&priv->cond);
#else
        aos_event_set(&priv->evt, LIST_SEG_UPDATE_EVT, AOS_EVENT_OR);
#endif
        ret = 0;
    } else {
        ret = 1;
    }
    list_unlock();

err:
    if (ret != 0) {
        _free_seg_plist(&psinfo->list);
    }
    av_free(buf);
    stream_close(s);
    return ret;
}

static stream_cls_t* _get_real_stream(struct hls_priv *priv, const char *hls_url)
{
    int rc;
    int retry_get_cnt = 0;
    int min_timeout = 2000, cnt = 0;
    stm_conf_t *stm_cnf;
    char *url        = NULL;
    stream_cls_t *rs = NULL;

    stm_cnf = &priv->stm_cnf;
    cnt = priv->flush_interval * 3 / min_timeout;
    cnt = cnt > 0 ? cnt + 1 : 1;

retry_get:
    rc = _get_play_url(priv, min_timeout);
    if (rc < 0) {
        if (retry_get_cnt++ > cnt || _is_interrupt(priv)) {
            /* maybe timeout */
            LOGE(TAG, "get play url timeout or interrupt. time out: %d, cnt = %d, retry_get_cnt = %d", min_timeout * cnt,
                 cnt, retry_get_cnt);
            goto err;
        }
        aos_msleep(50);
        goto retry_get;
    } else {
        url = _get_real_url(hls_url, priv->seg_url);
        if (url) {
            int retry_cnt = 0;
retry:
            rs = stream_open(url, stm_cnf);
            if (!rs && retry_cnt++ < 3 && !_is_interrupt(priv)) {
                //FIXME:patch for stream_open may be fail sometimes
                aos_msleep(50);
                goto retry;
            }
            CHECK_RET_TAG_WITH_GOTO(rs, err);
            av_free(url);
        }
    }

    return rs;
err:
    av_free(url);
    return NULL;
}

static void _hls_list_task(void *arg)
{
    int rc;
    char *url;
    stream_cls_t *o       = arg;
    struct hls_priv *priv = o->priv;
    int loop_cnt, cnt = 0, per = 200, retry_cnt;

    loop_cnt = priv->flush_interval / per;
    loop_cnt = loop_cnt ? loop_cnt : 1;
    url      = priv->rate_url ? priv->rate_url : (char*)stream_get_url(o);
    while (!priv->eof && !_is_interrupt(priv)) {
        if (cnt == loop_cnt) {
            cnt       = 0;
            retry_cnt = 0;
retry:
            rc = _get_play_list(priv, url);
            if (rc < 0 && retry_cnt++ < 3 && !_is_interrupt(priv)) {
                //FIXME:patch for stream_open may be fail sometimes
                aos_msleep(50);
                goto retry;
            }
        } else {
            cnt++;
            aos_msleep(per);
        }
    }

#ifndef __linux__
    aos_event_set(&priv->evt, LIST_TASK_QUIT_EVT, AOS_EVENT_OR);
#endif
}

static void _reset_hls_priv(struct hls_priv *priv)
{
    priv->eof           = 0;
    priv->is_live       = 0;
    priv->last_sequence = -1;

    if (priv->rs) {
        stream_close(priv->rs);
        priv->rs = NULL;
    }

    _free_master_plist(&priv->master_plist);
    _free_seg_plist(&priv->seg_plist);
    av_freep(&priv->seg_url);
    av_freep(&priv->rate_url);
}

static int _stream_hls_open(stream_cls_t *o, int mode)
{
    int rc;
    stm_conf_t *stm_cnf;
    stream_cls_t *rs       = NULL;
    struct hls_priv *priv = NULL;

    UNUSED(mode);
    priv = av_zalloc(sizeof(struct hls_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);
    priv->last_sequence = -1;
    o->priv = priv;
    stm_cnf = &priv->stm_cnf;

#ifdef __linux__
    pthread_cond_init(&priv->cond, NULL);
#else
    aos_event_new(&priv->evt, 0);
#endif
    aos_mutex_new(&priv->lock);
    stream_conf_init(stm_cnf);
    stm_cnf->cache_size  = 0;
    stm_cnf->need_parse  = 0;
    stm_cnf->rcv_timeout = o->rcv_timeout;
    memcpy(&stm_cnf->irq, &o->irq, sizeof(irq_av_t));

    rc = _get_play_list(priv, o->url);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    rs = _get_real_stream(priv, stream_get_url(o));
    CHECK_RET_TAG_WITH_GOTO(rs, err);
    if (priv->is_live) {
        aos_task_new_ext(&priv->list_task, "_hls_list_task", _hls_list_task, (void *)o, 4 * 1024, AOS_DEFAULT_APP_PRI - 2);
    }

    priv->rs        = rs;
    o->live         = priv->is_live;
    o->enable_cache = is_need_cache(o->url);

    return 0;
err:
    stream_close(rs);
    if (priv) {
        _reset_hls_priv(priv);
        aos_mutex_free(&priv->lock);
#ifdef __linux__
        pthread_cond_destroy(&priv->cond);
#else
        aos_event_free(&priv->evt);
#endif
        av_free(priv);
    }
    o->priv = NULL;
    return -1;
}

static int _stream_hls_close(stream_cls_t *o)
{
    struct hls_priv *priv = o->priv;

    priv->eof = 1;
    if (priv->is_live) {
#ifdef __linux__
        if (priv->list_task) {
            pthread_join(priv->list_task, NULL);
            priv->list_task = 0;
        }
#else
        {
            unsigned int flag;

            aos_event_get(&priv->evt, LIST_TASK_QUIT_EVT, AOS_EVENT_OR_CLEAR, &flag, AOS_WAIT_FOREVER);
        }
#endif
    }

    _reset_hls_priv(priv);
    aos_mutex_free(&priv->lock);
#ifdef __linux__
    pthread_cond_destroy(&priv->cond);
#else
    aos_event_free(&priv->evt);
#endif
    av_free(priv);
    o->priv = NULL;
    return 0;
}

static int _stream_hls_read(stream_cls_t *o, uint8_t *buf, size_t count)
{
    int rc;
    char *url;
    struct hls_priv *priv = o->priv;

    url = priv->rate_url ? priv->rate_url : (char*)stream_get_url(o);
    rc  = stream_read(priv->rs, buf, count);
    if (rc <= 0) {
        if (!stream_is_eof(priv->rs)) {
            //FIXME: read next segment. current stream may be pause a long time or expired/delete.
            LOGD(TAG, "close cur, read next, rc = %d, url = %s", rc, stream_get_url(priv->rs));
        }
        stream_close(priv->rs);

        priv->rs = _get_real_stream(priv, url);
        if (priv->rs) {
            rc = stream_read(priv->rs, buf, count);
        }
    }

    return rc;
}

static int _stream_hls_write(stream_cls_t *o, const uint8_t *buf, size_t count)
{
    //TODO
    return -1;
}

static int _stream_hls_seek(stream_cls_t *o, int32_t pos)
{
    //TODO
    return -1;
}

static int _stream_hls_control(stream_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    //TODO:
    return -1;
}

//FIXME: rate-single stream support only
const struct stream_ops stream_ops_hls = {
    .name            = "hls",
    .type            = STREAM_TYPE_HLS,
    .protocols       = { NULL },

    .open            = _stream_hls_open,
    .close           = _stream_hls_close,
    .read            = _stream_hls_read,
    .write           = _stream_hls_write,
    .seek            = _stream_hls_seek,
    .control         = _stream_hls_control,
};
#endif

