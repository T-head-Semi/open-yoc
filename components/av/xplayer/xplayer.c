/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#include "xplayer/xplayer_cls.h"
#include "xplayer/xplayer.h"
#include "xplayer/xplayer_all.h"

#define TAG                    "xplayer"

#define xplayer_lock()         (aos_mutex_lock(&xplayer->lock, AOS_WAIT_FOREVER))
#define xplayer_unlock()       (aos_mutex_unlock(&xplayer->lock))

static struct xplayer_global {
    //TODO:
    int                          init;
    aos_mutex_t                  lock;
    slist_t                      head;       ///< for xplayer class list
} g_xplayer_global;

#define get_xplayer_cls_list()   (&g_xplayer_global.head)

/**
 * @brief  regist xplayer class
 * @param  [in] ops
 * @return 0/-1
 */
int xplayer_cls_register(const xplayer_cls_t *cls)
{
    struct xplayer_cls *tcls;

    CHECK_PARAM(cls && cls->name, -1);
    if (!slist_empty(get_xplayer_cls_list())) {
        slist_for_each_entry(get_xplayer_cls_list(), tcls,  struct xplayer_cls, node) {
            if (!strcmp(tcls->name, cls->name)) {
                LOGE(TAG, "error. cls had regist yet!, name = %s", cls->name);
                return -1;
            }
        }
    }

    tcls = av_malloc(sizeof(struct xplayer_cls));
    memcpy(tcls, cls, sizeof(struct xplayer_cls));
    slist_add_tail(&tcls->node, get_xplayer_cls_list());
    LOGD(TAG, "regist one xplayer class, name = %s", cls->name);

    return 0;
}

static struct xplayer_cls* _get_xplayer_cls(const char *name)
{
    struct xplayer_cls *tcls;

    if (!slist_empty(get_xplayer_cls_list())) {
        slist_for_each_entry(get_xplayer_cls_list(), tcls,  struct xplayer_cls, node) {
            if (name) {
                if (strcmp(tcls->name, name) == 0) {
                    return tcls;
                }
            } else {
                return tcls;
            }
        }
    }

    return NULL;
}


/**
* @brief  init config param for xplayer_module_config
* @param  [in] conf
* @return 0/-1
*/
int xplayer_module_config_init(xplayer_mdl_cnf_t *conf)
{
    CHECK_PARAM(conf, -1);
    //TODO:
    memset(conf, 0, sizeof(xplayer_mdl_cnf_t));

    return 0;
}

/**
 * @brief  init xplayer module default(init once)
 * @param  [in] params : module config params
 * @return 0/-1
 */
int xplayer_module_init(const xplayer_mdl_cnf_t *conf)
{
    if (!g_xplayer_global.init) {
        slist_init(&g_xplayer_global.head);
        xplayer_register();
        aos_mutex_new(&g_xplayer_global.lock);
        g_xplayer_global.init = 1;
    }

    return 0;
}

/**
 * @brief  new one xplayer obj by the name. create default player if name is NULL
 * @param  [in] name : player class by the name, may be NULL
 * @return NULL on error
 */
xplayer_t* xplayer_new(const char *name)
{
    int rc = -1;
    xplayer_t *xplayer;
    struct xplayer_cls *cls;

    cls = _get_xplayer_cls(name);
    if (!cls) {
        LOGE(TAG, "error. xplayer cls had not regist yet!, name = %s", name ? name : "default");
        return NULL;
    }
    xplayer = aos_zalloc(sizeof(xplayer_t) + cls->priv_size);
    xplayer->vol    = 180;
    xplayer->speed  = 1.0;
    xplayer->cls    = cls;
    xplayer->status = XPLAYER_STATUS_STOPED;

    rc = xplayer->cls->ops->init(xplayer);
    if (rc) {
        LOGE(TAG, "error. xplayer init fail!");
        av_free(xplayer);
        return NULL;
    }

    aos_mutex_new(&xplayer->lock);

    return xplayer;
}

/**
 * @brief  get config param from the xplayer
 * @param  [in] xplayer
 * @param  [in/out] conf
 * @return 0/-1
 */
int xplayer_get_config(xplayer_t *xplayer, xplayer_cnf_t *conf)
{
    int rc = -1;

    CHECK_PARAM(xplayer && conf, -1);
    xplayer_lock();
    //LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    memset(conf, 0, sizeof(xplayer_cnf_t));
    rc = xplayer->cls->ops->get_config ? xplayer->cls->ops->get_config(xplayer, conf) : rc;
    //LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  set config param for the xplayer before play
 * @param  [in] xplayer
 * @param  [in] conf
 * @return 0/-1
 */
int xplayer_set_config(xplayer_t *xplayer, const xplayer_cnf_t *conf)
{
    int rc = -1;
    CHECK_PARAM(xplayer && conf, -1);
    xplayer_lock();
    //LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_config ? xplayer->cls->ops->set_config(xplayer, conf) : rc;
    //LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  set event callback before play
 * @param  [in] xplayer
 * @param  [in] event_cb : callback of the xplayer event
 * @param  [in] user_data : user data for event_cb
 * @return 0/-1
 */
int xplayer_set_callback(xplayer_t *xplayer, xplayer_eventcb_t event_cb, const void *user_data)
{
    CHECK_PARAM(xplayer && event_cb, -1);
    xplayer_lock();
    //LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer->event_cb  = event_cb;
    xplayer->user_data = (void*)user_data;
    //LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return 0;
}

/**
 * @brief  set play source url before play
 * @param  [in] xplayer
 * @param  [in] url : example: http://ip:port/xx.mp3
 * @return 0/-1
 */
int xplayer_set_url(xplayer_t *xplayer, const char *url)
{
    CHECK_PARAM(xplayer && url, -1);
    xplayer_lock();
    //LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    av_freep(&xplayer->url);
    xplayer->url = strdup(url);
    //LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return 0;
}

/**
 * @brief  xplayer play interface
 * @param  [in] xplayer
 * @return 0/-1
 */
int xplayer_play(xplayer_t *xplayer)
{
    int rc;

    CHECK_PARAM(xplayer && xplayer->url, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->play(xplayer);
    if (rc) {
        LOGE(TAG, "play error, rc = %d, url = %s", rc, xplayer->url);
    } else {
        xplayer->status = XPLAYER_STATUS_PLAYING;
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  pause the xplayer
 * @param  [in] xplayer
 * @return 0/-1
 */
int xplayer_pause(xplayer_t *xplayer)
{
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    if (xplayer->status == XPLAYER_STATUS_PLAYING) {
        rc = xplayer->cls->ops->pause(xplayer);
        xplayer->status = rc == 0 ? XPLAYER_STATUS_PAUSED : xplayer->status;
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  resume the xplayer
 * @param  [in] xplayer
 * @return 0/-1
 */
int xplayer_resume(xplayer_t *xplayer)
{
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    if (xplayer->status == XPLAYER_STATUS_PAUSED) {
        rc = xplayer->cls->ops->resume(xplayer);
        xplayer->status = rc == 0 ? XPLAYER_STATUS_PLAYING : xplayer->status;
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  stop the xplayer
 * @param  [in] xplayer
 * @return 0/-1
 */
int xplayer_stop(xplayer_t *xplayer)
{
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    if (xplayer->status != XPLAYER_STATUS_STOPED) {
        rc = xplayer->cls->ops->stop(xplayer);
        xplayer->status = XPLAYER_STATUS_STOPED;
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  free/destroy the xplayer obj
 * @param  [in] xplayer
 * @return 0/-1
 */
int xplayer_free(xplayer_t *xplayer)
{
    CHECK_PARAM(xplayer, -1);
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_stop(xplayer);
    xplayer->cls->ops->uninit(xplayer);
    aos_mutex_free(&xplayer->lock);
    av_freep(&xplayer->url);
    av_free(xplayer);
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);

    return 0;
}

/**
 * @brief  seek to the time
 * @param  [in] xplayer
 * @param  [in] timestamp : seek time
 * @return 0/-1
 */
int xplayer_seek(xplayer_t *xplayer, uint64_t timestamp)
{
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    if (xplayer->status != XPLAYER_STATUS_STOPED) {
        rc = xplayer->cls->ops->seek ? xplayer->cls->ops->seek(xplayer, timestamp) : rc;
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  set start play time before play
 * @param  [in] xplayer
 * @param  [in] start_time : start play time
 * @return 0/-1
 */
int xplayer_set_start_time(xplayer_t *xplayer, uint64_t start_time)
{
    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    //LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer->start_time = start_time;
    //LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return 0;
}

/**
 * @brief  get play time(current time & duration, ms)
 * @param  [in] xplayer
 * @param  [in/out] time
 * @return 0/-1
 */
int xplayer_get_time(xplayer_t *xplayer, xplay_time_t *time)
{
    int rc = -1;

    CHECK_PARAM(xplayer && time, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->get_time ? xplayer->cls->ops->get_time(xplayer, time) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  get media info
 * @param  [in] xplayer
 * @param  [in/out] minfo : need free by the caller(use media_info_uninit function)
 * @return 0/-1
 */
int xplayer_get_media_info(xplayer_t *xplayer, xmedia_info_t *minfo)
{
    int rc = -1;

    CHECK_PARAM(xplayer && minfo, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->get_media_info ? xplayer->cls->ops->get_media_info(xplayer, minfo) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  get soft vol index of the xplayer
 * @param  [in] xplayer
 * @param  [in/out] vol : vol scale index(0~255)
 * @return 0/-1
 */
int xplayer_get_vol(xplayer_t *xplayer, uint8_t *vol)
{
    CHECK_PARAM(xplayer && vol, -1);
    *vol = xplayer->vol;
    return 0;
}

/**
 * @brief  set soft vol index of the xplayer
 * @param  [in] xplayer
 * @param  [in/out] vol : vol scale index(0~255)
 * @return 0/-1
 */
int xplayer_set_vol(xplayer_t *xplayer, uint8_t vol)
{
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_vol ? xplayer->cls->ops->set_vol(xplayer, vol) : rc;
    if (rc == 0) {
        xplayer->vol = vol;
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  get play speed of the xplayer
 * @param  [in] xplayer
 * @param  [out] speed
 * @return 0/-1
 */
int xplayer_get_speed(xplayer_t *xplayer, float *speed)
{
    CHECK_PARAM(xplayer && speed, -1);
    *speed = xplayer->speed;
    return 0;
}

/**
 * @brief  set play speed of the xplayer
 * @param  [in] xplayer
 * @param  [in] speed : 0.5 ~ 2.0 for audio, etc
 * @return 0/-1
 */
int xplayer_set_speed(xplayer_t *xplayer, float speed)
{
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_speed ? xplayer->cls->ops->set_speed(xplayer, speed) : rc;
    if (rc == 0) {
        xplayer->speed = speed;
    }
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  mute/unmute the xplayer
 * @param  [in] xplayer
 * @param  [in] mute : mute flag
 * @return 0/-1
 */
int xplayer_set_mute(xplayer_t *xplayer, uint8_t mute)
{
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_mute ? xplayer->cls->ops->set_mute(xplayer, mute) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
}

/**
 * @brief  get play source url
 * @param  [in] xplayer
 * @return NULL on error
 */
const char* xplayer_get_url(xplayer_t *xplayer)
{
    return xplayer ? xplayer->url : NULL;
}

/**
 * @brief  switch audio track of the player
 * @param  [in] xplayer
 * @param  [in] idx : index of audios
 * @return 0/-1
 */
int xplayer_switch_audio_track(xplayer_t *xplayer, uint8_t idx)
{
#if CONFIG_AV_AUDIO_ONLY_SUPPORT
    LOGD(TAG, "%s, %d. audio only support!", __FUNCTION__, __LINE__);
    return -1;
#else
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->switch_audio_track ? xplayer->cls->ops->switch_audio_track(xplayer, idx) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
#endif
}

/**
 * @brief  switch subtitle track of the player
 * @param  [in] xplayer
 * @param  [in] idx : index of subtitles
 * @return 0/-1
 */
int xplayer_switch_subtitle_track(xplayer_t *xplayer, uint8_t idx)
{
#if CONFIG_AV_AUDIO_ONLY_SUPPORT
    LOGD(TAG, "%s, %d. audio only support!", __FUNCTION__, __LINE__);
    return -1;
#else
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->switch_subtitle_track ? xplayer->cls->ops->switch_subtitle_track(xplayer, idx) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
#endif
}

/**
 * @brief  set external subtitle url of the player
 * @param  [in] xplayer
 * @param  [in] url : url of the external subtitle
 * @return 0/-1
 */
int xplayer_set_subtitle_url(xplayer_t *xplayer, const char *url)
{
#if CONFIG_AV_AUDIO_ONLY_SUPPORT
    LOGD(TAG, "%s, %d. audio only support!", __FUNCTION__, __LINE__);
    return -1;
#else
    int rc = -1;

    CHECK_PARAM(xplayer && url, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_subtitle_url ? xplayer->cls->ops->set_subtitle_url(xplayer, url) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
#endif
}

/**
 * @brief  show/hide subtitle of the xplayer
 * @param  [in] xplayer
 * @param  [in] visible : show/hide
 * @return 0/-1
 */
int xplayer_set_subtitle_visible(xplayer_t *xplayer, uint8_t visible)
{
#if CONFIG_AV_AUDIO_ONLY_SUPPORT
    LOGD(TAG, "%s, %d. audio only support!", __FUNCTION__, __LINE__);
    return -1;
#else
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_subtitle_visible ? xplayer->cls->ops->set_subtitle_visible(xplayer, visible) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
#endif
}

/**
 * @brief  show/hide video of the xplayer
 * @param  [in] xplayer
 * @param  [in] visible : show/hide
 * @return 0/-1
 */
int xplayer_set_video_visible(xplayer_t *xplayer, uint8_t visible)
{
#if CONFIG_AV_AUDIO_ONLY_SUPPORT
    LOGD(TAG, "%s, %d. audio only support!", __FUNCTION__, __LINE__);
    return -1;
#else
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_video_visible ? xplayer->cls->ops->set_video_visible(xplayer, visible) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
#endif
}

/**
 * @brief  crop video of the xplayer
 * @param  [in] xplayer
 * @param  [in] win : crop window
 * @return 0/-1
 */
int xplayer_set_video_crop(xplayer_t *xplayer, const xwindow_t *win)
{
#if CONFIG_AV_AUDIO_ONLY_SUPPORT
    LOGD(TAG, "%s, %d. audio only support!", __FUNCTION__, __LINE__);
    return -1;
#else
    int rc = -1;

    CHECK_PARAM(xplayer && win, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_video_crop ? xplayer->cls->ops->set_video_crop(xplayer, win) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
#endif
}

/**
 * @brief  set display window of the xplayer
 * @param  [in] xplayer
 * @param  [in] win : display window
 * @return 0/-1
 */
int xplayer_set_display_window(xplayer_t *xplayer, const xwindow_t *win)
{
#if CONFIG_AV_AUDIO_ONLY_SUPPORT
    LOGD(TAG, "%s, %d. audio only support!", __FUNCTION__, __LINE__);
    return -1;
#else
    int rc = -1;

    CHECK_PARAM(xplayer && win, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_display_window ? xplayer->cls->ops->set_display_window(xplayer, win) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
#endif
}

/**
 * @brief  enable/disable fullscreen for the xplayer
 * @param  [in] xplayer
 * @param  [in] onoff
 * @return 0/-1
 */
int xplayer_set_fullscreen(xplayer_t *xplayer, uint8_t onoff)
{
#if CONFIG_AV_AUDIO_ONLY_SUPPORT
    LOGD(TAG, "%s, %d. audio only support!", __FUNCTION__, __LINE__);
    return -1;
#else
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_fullscreen ? xplayer->cls->ops->set_fullscreen(xplayer, onoff) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
#endif
}

/**
 * @brief  set display format for the xplayer
 * @param  [in] xplayer
 * @param  [in] format
 * @return 0/-1
 */
int xplayer_set_display_format(xplayer_t *xplayer, xdisplay_format_t format)
{
#if CONFIG_AV_AUDIO_ONLY_SUPPORT
    LOGD(TAG, "%s, %d. audio only support!", __FUNCTION__, __LINE__);
    return -1;
#else
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_display_format ? xplayer->cls->ops->set_display_format(xplayer, format) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
#endif
}

/**
 * @brief  rotate video of the xplayer
 * @param  [in] xplayer
 * @param  [in] type : rotate type
 * @return 0/-1
 */
int xplayer_set_video_rotate(xplayer_t *xplayer, xrotate_type_t type)
{
#if CONFIG_AV_AUDIO_ONLY_SUPPORT
    LOGD(TAG, "%s, %d. audio only support!", __FUNCTION__, __LINE__);
    return -1;
#else
    int rc = -1;

    CHECK_PARAM(xplayer, -1);
    xplayer_lock();
    LOGD(TAG, "%s, %d enter. player = %p", __FUNCTION__, __LINE__, xplayer);
    rc = xplayer->cls->ops->set_video_rotate ? xplayer->cls->ops->set_video_rotate(xplayer, type) : rc;
    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, xplayer);
    xplayer_unlock();

    return rc;
#endif
}


