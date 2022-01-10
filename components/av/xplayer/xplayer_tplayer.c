/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 */

#if defined(CONFIG_PLAYER_TPLAYER) && CONFIG_PLAYER_TPLAYER
#include <sys/ioctl.h>
#include <signal.h>
#include "xplayer/xplayer_cls.h"
#include "allwinner/tplayer.h"
#include "allwinner/sunxi_display_v2.h"

#define TAG "player_tplayer"

#if CONFIG_AV_AUDIO_ONLY_SUPPORT
#define PLAY_MODE AUDIO_PLAYER
#else
#define PLAY_MODE CEDARX_PLAYER
#endif

#define SCREEN_0 0

struct player_tplayer_priv {
    TPlayer          *player;
    xwindow_t        *disp_rect;
    int              screen_w;
    int              screen_h;
    int              inited;
    uint8_t          freezed;
};

#define GET_DERIVED_PLAYER_TPLAYER(p) ((struct player_tplayer_priv*)GET_DERIVED_PLAYER(p))

static struct player_tplayer_priv g_priv;

static int _tplayer_event(void *pUserData, int msg, int param0, void *param1)
{
    xplayer_t *xplayer = pUserData;
    uint8_t event = XPLAYER_EVENT_UNKNOWN;
    //LOGD(TAG, "Media(%p) msg:%d\n", xplayer, msg);

    switch (msg) {
    case TPLAYER_NOTIFY_PREPARED:
        break;

    case TPLAYER_NOTIFY_PLAYBACK_COMPLETE:
        event = XPLAYER_EVENT_FINISH;
        break;

    case TPLAYER_NOTIFY_SEEK_COMPLETE:
        break;

    case TPLAYER_NOTIFY_MEDIA_ERROR:
        event = XPLAYER_EVENT_ERROR;
        break;

    case TPLAYER_NOTIFY_NOT_SEEKABLE:
    case TPLAYER_NOTIFY_BUFFER_START:
    case TPLAYER_NOTIFY_BUFFER_END:
    case TPLAYER_NOTIFY_DOWNLOAD_START:
    case TPLAYER_NOTIFY_DOWNLOAD_END:
    case TPLAYER_NOTIFY_DOWNLOAD_ERROR:
    case TPLAYER_NOTIFY_MEDIA_VIDEO_SIZE:
    case TPLAYER_NOTIFY_VIDEO_FRAME:
    case TPLAYER_NOTIFY_AUDIO_FRAME:
    case TPLAYER_NOTIFY_SUBTITLE_FRAME:
    case TPLAYER_NOTYFY_DECODED_VIDEO_SIZE:
        break;

    default:
        break;
    }

    if (event != XPLAYER_EVENT_UNKNOWN && xplayer && xplayer->event_cb) {
        xplayer->event_cb(xplayer->user_data, event, NULL, 0);
    }

    return 0;
}

/* Signal handler */
static void terminate(int sig_no)
{
    printf("Got signal %d, exiting ...\n", sig_no);

    if (g_priv.player != NULL) {
        TPlayerDestroy(g_priv.player);
        g_priv.player = NULL;
        printf("TPlayerDestroy() successfully\n");
    }

    exit(1);
}

static void install_sig_handler(void)
{
    signal(SIGBUS, terminate);
    signal(SIGFPE, terminate);
    signal(SIGHUP, terminate);
    signal(SIGILL, terminate);
    signal(SIGINT, terminate);
    signal(SIGIOT, terminate);
    signal(SIGPIPE, terminate);
    signal(SIGQUIT, terminate);
    signal(SIGSEGV, terminate);
    signal(SIGSYS, terminate);
    signal(SIGTERM, terminate);
    signal(SIGTRAP, terminate);
    signal(SIGUSR1, terminate);
    signal(SIGUSR2, terminate);
}

static int _xplayer_tplayer_init(xplayer_t *xplayer)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);
    unsigned int ioctlParam[4];
    int disp_fd;

    LOGD(TAG, "%s", __FUNCTION__);

    if (g_priv.inited) {
        priv->player = g_priv.player;
        return 0;
    }

    if (!priv->player) {
        priv->player = TPlayerCreate(PLAY_MODE);

        if (!priv->player) {
            LOGE(TAG, "Player init err");
            return -1;
        }
    }

    g_priv.player = priv->player;
    g_priv.inited = 1;
    install_sig_handler();
    TPlayerSetNotifyCallback(priv->player, _tplayer_event, xplayer);

    disp_fd = open("/dev/disp", O_RDWR);

    if (disp_fd > 0) {
        ioctlParam[0] = SCREEN_0;
        ioctlParam[1] = 0;
        priv->screen_w = ioctl(disp_fd, DISP_GET_SCN_WIDTH, ioctlParam);
        priv->screen_h = ioctl(disp_fd, DISP_GET_SCN_HEIGHT, ioctlParam);

        close(disp_fd);
    }

    return 0;
}

static int _xplayer_tplayer_set_config(xplayer_t *xplayer, const xplayer_cnf_t *conf)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    LOGD(TAG, "%s", __FUNCTION__);
    priv->freezed = conf->freezed;
    TPlayerSetHoldLastPicture(priv->player, priv->freezed);
    return 0;
}

static int _xplayer_tplayer_get_config(xplayer_t *xplayer, xplayer_cnf_t *conf)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    LOGD(TAG, "%s", __FUNCTION__);
    conf->freezed = priv->freezed;

    return 0;
}

static int _xplayer_tplayer_play(xplayer_t *xplayer)
{
    int rc = -1;
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    LOGD(TAG, "%s", __FUNCTION__);

    TPlayerSetRotate(priv->player, XROTATE_DEGREE_90);

    rc = TPlayerSetDataSource(priv->player, xplayer->url, NULL);

    if (rc < 0) {
        LOGE(TAG, "Player set data source failed, ret %d", rc);
        return -1;
    }

    rc = TPlayerPrepare(priv->player);

    if (rc < 0) {
        LOGE(TAG, "Player prepare failed, ret %d", rc);
        return -1;
    }

    rc = TPlayerSeekTo(priv->player, xplayer->start_time);

    if (rc < 0) {
        LOGE(TAG, "Player prepare failed, ret %d", rc);
        return -1;
    }

    rc = TPlayerStart(priv->player);

    if (rc < 0) {
        LOGE(TAG, "Player prepare failed, ret %d", rc);
        return -1;
    }

    return rc;
}

static int _xplayer_tplayer_pause(xplayer_t *xplayer)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    LOGD(TAG, "%s", __FUNCTION__);
    return TPlayerPause(priv->player);
}

static int _xplayer_tplayer_resume(xplayer_t *xplayer)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);
    int msec;
    int ret;

    LOGD(TAG, "%s", __FUNCTION__);

    ret = TPlayerGetCurrentPosition(priv->player, &msec);

    if (ret) {
        return ret;
    }

    ret = TPlayerSeekTo(priv->player, msec);

    if (ret) {
        return ret;
    }

    return TPlayerStart(priv->player);
}

static int _xplayer_tplayer_seek(xplayer_t *xplayer, uint64_t timestamp)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    LOGD(TAG, "%s", __FUNCTION__);
    return TPlayerSeekTo(priv->player, timestamp);
}

static int _xplayer_tplayer_stop(xplayer_t *xplayer)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    LOGD(TAG, "%s", __FUNCTION__);
    return TPlayerStop(priv->player);
}

void _xplayer_tplayer_uninit(xplayer_t *xplayer)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    LOGD(TAG, "%s", __FUNCTION__);

    if (priv->player) {
        TPlayerDestroy(priv->player);
    }

    priv->player = NULL;
    g_priv.inited = 0;
    return;
}

static int _xplayer_tplayer_get_media_info(xplayer_t *xplayer, xmedia_info_t *minfo)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);
    MediaInfo *media_info;

    LOGD(TAG, "%s", __FUNCTION__);
    media_info = TPlayerGetMediaInfo(priv->player);

    minfo->size = media_info->nFileSize;
    minfo->bps = media_info->nBitrate;
    minfo->duration = media_info->nDurationMs;

    return media_info ? 0 : -1;
}

static int _xplayer_tplayer_get_time(xplayer_t *xplayer, xplay_time_t *time)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);
    int duration, curtime, rc;

    LOGD(TAG, "%s", __FUNCTION__);
    rc = TPlayerGetCurrentPosition(priv->player, &curtime);

    if (rc < 0) {
        LOGE(TAG, "Get current time failed, ret %d", rc);
        return -1;
    }

    rc = TPlayerGetDuration(priv->player, &duration);

    if (rc < 0) {
        LOGE(TAG, "Get duration failed, ret %d", rc);
        return -1;
    }

    time->curtime = curtime;
    time->duration = duration;

    return 0;
}

static int _xplayer_tplayer_set_speed(xplayer_t *xplayer, float speed)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);
    TplayerPlaySpeedType speed_type;

    LOGD(TAG, "%s", __FUNCTION__);

    if (speed - (int)speed != 0) {
        return -1;
    }

    switch ((int)speed) {
    case 1:
        speed_type = PLAY_SPEED_1;
        break;

    case 2:
        speed_type = PLAY_SPEED_FAST_FORWARD_2;
        break;

    case 4:
        speed_type = PLAY_SPEED_FAST_FORWARD_4;
        break;

    case 8:
        speed_type = PLAY_SPEED_FAST_FORWARD_8;
        break;

    case 16:
        speed_type = PLAY_SPEED_FAST_FORWARD_16;
        break;

    case -2:
        speed_type = PLAY_SPEED_FAST_BACKWARD_2;
        break;

    case -4:
        speed_type = PLAY_SPEED_FAST_BACKWARD_4;
        break;

    case -8:
        speed_type = PLAY_SPEED_FAST_BACKWARD_8;
        break;

    case -16:
        speed_type = PLAY_SPEED_FAST_BACKWARD_16;
        break;

    default:
        LOGE(TAG, "Speed multiple not available");
        return -1;
    }

    return TPlayerSetSpeed(priv->player, speed_type);
}

static int _xplayer_tplayer_set_vol(xplayer_t *xplayer, uint8_t vol)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);
    int tplay_volume = vol * 40 / 255;

    LOGD(TAG, "%s", __FUNCTION__);
    return TPlayerSetVolume(priv->player, tplay_volume);
}

static int _xplayer_tplayer_set_mute(xplayer_t *xplayer, uint8_t mute)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    LOGD(TAG, "%s", __FUNCTION__);

    if (mute) {
        return TPlayerSetAudioMute(priv->player, 1);
    } else {
        return TPlayerSetAudioMute(priv->player, 0);
    }
}

static int _xplayer_tplayer_switch_audio_track(xplayer_t *xplayer, uint8_t idx)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    LOGD(TAG, "%s", __FUNCTION__);

    MediaInfo* mi = TPlayerGetMediaInfo(priv->player);

    if (!mi) {
        return -1;
    }

    priv->player->mMediaInfo = mi;

    return TPlayerSwitchAudio(priv->player, idx);
}

static int _xplayer_tplayer_switch_subtitle_track(xplayer_t *xplayer, uint8_t idx)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    LOGD(TAG, "%s", __FUNCTION__);
    return TPlayerSwitchSubtitle(priv->player, idx);
}

static int _xplayer_tplayer_set_subtitle_url(xplayer_t *xplayer, const char *url)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    return TPlayerSetExternalSubUrl(priv->player, url);
}

static int _xplayer_tplayer_set_subtitle_visible(xplayer_t *xplayer, uint8_t visible)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    TPlayerSetSubtitleDisplay(priv->player, visible);

    return 0;
}

static int _xplayer_tplayer_set_video_visible(xplayer_t *xplayer, uint8_t visible)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    TPlayerSetVideoDisplay(priv->player, visible);

    return 0;
}

static int _xplayer_tplayer_set_video_crop(xplayer_t *xplayer, const xwindow_t *win)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    TPlayerSetSrcRect(priv->player, win->x, win->y, win->width, win->height);
    return 0;
}

static int _xplayer_tplayer_set_fullscreen(xplayer_t *xplayer, uint8_t onoff)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);
    xwindow_t *disp_rect = priv->disp_rect;

    if (onoff) {
        if (priv->screen_w && priv->screen_h) {
            TPlayerSetDisplayRect(priv->player, 0, 0, priv->screen_w, priv->screen_h);
            return 0;
        } else {
            LOGE(TAG, "Can not get screen size");
        }
    } else {
        TPlayerSetDisplayRect(priv->player, disp_rect->x, disp_rect->y,
                              disp_rect->width, disp_rect->height);
        return 0;
    }

    return -1;
}

static int _xplayer_tplayer_set_display_window(xplayer_t *xplayer, const xwindow_t *win)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);
    priv->disp_rect = (xwindow_t *)win;

    TPlayerSetDisplayRect(priv->player, win->x, win->y, win->width, win->height);
    return 0;
}

static int _xplayer_tplayer_set_display_format(xplayer_t *xplayer, xdisplay_format_t format)
{
    return -1;
}

static int _xplayer_tplayer_set_video_rotate(xplayer_t *xplayer, xrotate_type_t type)
{
    struct player_tplayer_priv *priv = GET_DERIVED_PLAYER_TPLAYER(xplayer);

    return TPlayerSetRotate(priv->player, type);
}

static const struct xplayer_ops xplayer_ops_tplayer = {
    .init                  = _xplayer_tplayer_init,
    .get_config            = _xplayer_tplayer_get_config,
    .set_config            = _xplayer_tplayer_set_config,
    .play                  = _xplayer_tplayer_play,
    .pause                 = _xplayer_tplayer_pause,
    .resume                = _xplayer_tplayer_resume,
    .seek                  = _xplayer_tplayer_seek,
    .stop                  = _xplayer_tplayer_stop,
    .uninit                = _xplayer_tplayer_uninit,

    .get_media_info        = _xplayer_tplayer_get_media_info,
    .get_time              = _xplayer_tplayer_get_time,
    .set_vol               = _xplayer_tplayer_set_vol,
    .set_speed             = _xplayer_tplayer_set_speed,
    .set_mute              = _xplayer_tplayer_set_mute,
    .switch_audio_track    = _xplayer_tplayer_switch_audio_track,
    .switch_subtitle_track = _xplayer_tplayer_switch_subtitle_track,
    .set_subtitle_url      = _xplayer_tplayer_set_subtitle_url,
    .set_subtitle_visible  = _xplayer_tplayer_set_subtitle_visible,
    .set_video_visible     = _xplayer_tplayer_set_video_visible,
    .set_video_crop        = _xplayer_tplayer_set_video_crop,
    .set_fullscreen        = _xplayer_tplayer_set_fullscreen,
    .set_display_window    = _xplayer_tplayer_set_display_window,
    .set_display_format    = _xplayer_tplayer_set_display_format,
    .set_video_rotate      = _xplayer_tplayer_set_video_rotate,
};

const struct xplayer_cls xplayer_cls_tplayer = {
    .name            = "tplayer",
    .priv_size       = sizeof(struct player_tplayer_priv),
    .ops             = &xplayer_ops_tplayer,
};

#endif
