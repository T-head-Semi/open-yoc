/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_USE_MINIALSA) || defined(__linux__)
#include <media.h>
#include <uservice/eventid.h>
#include "avformat/avformat_utils.h"
#include <math.h>
#ifndef __linux__
#include <aos/list.h>
#endif
#include "xplayer/xplayer.h"

#define TAG "media"

/** 音频服务
 * type:
 *  0: 音乐播放
 *  1: 提示音播放
 *  2: 对话播放
 *      每一个 type 只有一个播放实例，新的播放会替换旧的播放实例，
 *      提示音的播放会先暂停音乐播放，播放完成之后继续音乐播放
 */
typedef struct {
    int type;
    int vol;
    int step_vol;
    int time;
} vol_param_t;

typedef struct {
    int         type;
    int         resume;
    uint64_t    seek_time;
    char       *url;
} play_param_t;

typedef struct {
    int         type;
    char        *url;
    int         evtid;
    void        *data;
    size_t      size;
} event_param_t;

typedef struct {
    int        type;
    uint64_t   seek_time;
} seek_param_t;

typedef struct {
    int   type;
    float speed;
} speed_param_t;

typedef struct {
    int   type;
    union {
        uint8_t      v_u8;
        uint16_t     v_u16;
        uint32_t     v_u32;
        uint64_t     v_u64;
        int8_t       v_s8;
        int16_t      v_s16;
        int32_t      v_s32;
        int64_t      v_s64;
        float        v_float;
        double       v_double;
        void*        v_point;
    } v;
} media_var_param_t;

typedef enum {
    MEDIA_INIT_CMD,
    MEDIA_PLAY_CMD,
    MEDIA_PAUSE_CMD,
    MEDIA_RESUME_CMD,
    MEDIA_STOP_CMD,
    MEDIA_SEEK_CMD,
    MEDIA_GET_TIME_CMD,
    MEDIA_SET_VOL_CMD,
    MEDIA_SET_MIN_VOL_CMD,
    MEDIA_GET_VOL_CMD,
    MEDIA_ADD_VOL_CMD,
    MEDIA_STEP_VOL_CMD,
    MEDIA_EVT_ERR_CMD,
    MEDIA_EVT_RUN_CMD,
    MEDIA_EVT_END_CMD,
    MEDIA_EVT_UNDER_RUN_CMD,
    MEDIA_EVT_OVER_RUN_CMD,
    MEDIA_CONTINUE_MUSIC_CMD,
    MEDIA_CONFIG_CMD,
    MEDIA_CONFIG_KEY_CMD,
    MEDIA_MUTE_CMD,
    MEDIA_UNMUTE_CMD,
    MEDIA_SET_SPEED_CMD,
    MEDIA_GET_SPEED_CMD,
    MEDIA_GET_MEDIA_INFO_CMD,
#if !CONFIG_AV_AUDIO_ONLY_SUPPORT
    MEDIA_SWITCH_AUDIO_TRACK_CMD,
    MEDIA_SWITCH_SUBTITLE_TRACK_CMD,
    MEDIA_SET_SUBTITLE_URL_CMD,
    MEDIA_SET_SUBTITLE_VISIBLE_CMD,
    MEDIA_SET_VIDEO_VISIBLE_CMD,
    MEDIA_SET_VIDEO_CROP_CMD,
    MEDIA_SET_DISPLAY_WINDOW_CMD,
    MEDIA_SET_FULLSCREEN_CMD,
    MEDIA_SET_DISPLAY_FORMAT_CMD,
    MEDIA_SET_VIDEO_ROTATE_CMD,
#endif
    MEDIA_END_CMD
} MEDIA_CMD;

#define VOLUME_MAX (1000)
#define VOLUME_UNIT (10)
#define VOLUME_MIN (0)

#define MEDIA_MAX_NUM (2)

#define EVENT_STEP_VOL_ID (EVENT_USER + 100)

#define VOL_STEP_TIME (50)
#define VOL_STEP_MAX_TIME (5000)

typedef struct {
    int       type;
    xplayer_t  *hdl;
    char      *url;
    int       speech_pause; //music 被其他打断标志
    int       state;
    int       mute;         //当前是否是mute状态
    int       play_vol;
    int       min_vol;
    int       cur_vol;
    int       exp_vol;
    int       step_vol;
    int       resume;
    aui_player_config_t config;
    media_key_cb_t key_cb;
} media_type_t;

typedef struct {
    uservice_t *      srv;
    media_evt_t       cb;
} media_t;

static media_type_t         media_type[MEDIA_MAX_NUM];
static media_t              g_media;
static eqfp_t * g_eq_segments = NULL;
static int g_eq_segment_count = 0;

static void          m_step_vol(media_type_t *m, int new_vol, int time);
static media_type_t *get_type(int type);
static int           m_vol_set(media_type_t *m, int vol);
#define abs_to_per(abs_vol) (abs_vol / VOLUME_UNIT)
#define per_to_abs(per_vol) (per_vol * VOLUME_UNIT)

static uint8_t g_vol_map[101] = {
    0,
    87,
    88,
    90,
    91,
    92,
    93,
    95,
    96,
    97,
    98,
    99,
    101,
    102,
    103,
    104,
    106,
    107,
    108,
    109,
    110,
    112,
    113,
    114,
    115,
    117,
    118,
    119,
    120,
    121,
    123,
    124,
    125,
    126,
    127,
    129,
    130,
    131,
    132,
    134,
    135,
    136,
    137,
    138,
    140,
    141,
    142,
    143,
    145,
    146,
    147,
    148,
    149,
    151,
    152,
    153,
    154,
    156,
    157,
    158,
    159,
    160,
    162,
    163,
    164,
    165,
    167,
    168,
    169,
    170,
    171,
    173,
    174,
    175,
    176,
    178,
    179,
    180,
    181,
    182,
    184,
    185,
    186,
    187,
    188,
    190,
    191,
    192,
    193,
    195,
    196,
    197,
    198,
    199,
    201,
    202,
    203,
    204,
    206,
    207,
    208
};

static uint8_t *_get_vol_map()
{
    //FIXME: get one valid media-config
    media_type_t *m = get_type(MEDIA_MUSIC);
    aui_player_config_t *config = &m->config;

    return config->vol_map ? config->vol_map : &g_vol_map[0];
}

static void _audio_set_vol(int type, int lvol, int rvol)
{
    int vol = abs_to_per(lvol);
    uint8_t *vol_map = _get_vol_map();
    int vol_idx = vol ? vol_map[vol] : 0;

    //LOGD(TAG, "media(%d) set vol:%d, idx:%d", type, vol, vol_idx);
    if (type != MEDIA_ALL) {
        media_type_t *m = get_type(type);

        if (m->hdl) {
            xplayer_set_vol(m->hdl, vol_idx);
        }
    } else {
        for (int i = 0; i < MEDIA_MAX_NUM; i++) {
            media_type_t *m = get_type(i);

            if (m->hdl) {
                xplayer_set_vol(m->hdl, vol_idx);
            }
        }
    }
}

static void player_event(void *user_data, uint8_t event, const void *data, uint32_t len)
{
    int i;
    event_param_t param = {0};
    xplayer_t *player = user_data;
    char *url = (char*)xplayer_get_url(player);

    for (i = 0; i < MEDIA_MAX_NUM; i++) {
        if (media_type[i].hdl == player) {
            break;
        }
    }

    LOGD(TAG, "media(%d) evt:%d\n", i, event);
    if (i != MEDIA_MAX_NUM) {
        param.type  = i;
        param.url   = strdup(url ? url: "default");
        param.evtid = event;
        if (data && len) {
            param.size = len;
            param.data = aos_malloc(len);
            memcpy(param.data, data, len);
        }

        uservice_call_async(g_media.srv, MEDIA_EVT_ERR_CMD + event - 1, &param, sizeof(event_param_t));
    }
}

static media_type_t *get_type(int type)
{
    return (&media_type[type]);
}

static inline void rpc_return_int(rpc_t *rpc, int ret)
{
    rpc_put_reset(rpc);
    rpc_put_int(rpc, ret);
}

static int media_evt_call(int type, const char *url, int evt_id, const void *data, size_t size)
{
    //FIXME:
    if (g_media.cb) {
        g_media.cb(type, url ? url : "default", evt_id, data, size);
    }

    return 0;
}

static int m_stop(media_type_t *m, int need_callback)
{
    if (m->hdl) {
        if (m->state == AUI_PLAYER_PLAYING || m->state == AUI_PLAYER_PAUSED) {
            if (need_callback)
                media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_STOP, NULL, 0);
        }
        xplayer_stop(m->hdl);
        xplayer_free(m->hdl);
        aos_freep(&m->url);
        m->state = AUI_PLAYER_STOP;
        m->hdl   = 0;
    }

    return 0;
}

static int m_pause(media_type_t *m)
{
    if (m->hdl && m->state == AUI_PLAYER_PLAYING) {
        media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_PAUSE, NULL, 0);
        xplayer_pause(m->hdl);
        m->state = AUI_PLAYER_PAUSED;
    }

    return 0;
}

static int m_vol_set(media_type_t *m, int vol)
{
    if ((vol > VOLUME_MAX && m->play_vol == VOLUME_MAX) ||
        (vol < VOLUME_MIN && m->play_vol == VOLUME_MIN)) {
        //LOGD(TAG, "set err->cur:%d  set:%d", abs_to_per(m->play_vol), abs_to_per(vol));
        return -1;
    }

    if (vol > VOLUME_MAX) {
        vol = VOLUME_MAX;
    } else if (vol < VOLUME_MIN) {
        vol = VOLUME_MIN;
    }

    if (abs_to_per(m->play_vol) != abs_to_per(vol)) {
        aui_val_change_t vc;

        vc.val_old = abs_to_per(m->play_vol);
        vc.val_new = abs_to_per(vol);
        media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_VOL_CHANGE, &vc, sizeof(aui_val_change_t));
    }
    m->play_vol = vol;

    LOGD(TAG, "set vol:%d type:%d", abs_to_per(m->play_vol), m->type);
    _audio_set_vol(m->type, m->play_vol, m->play_vol);
    m->cur_vol = m->play_vol;

    return 0;
}

static int m_start(media_type_t *m, const char *url, uint64_t seek_time)
{
    int ret;

    if (m->state == AUI_PLAYER_STOP) {
#if (defined(CONFIG_PLAYER_YOC) && CONFIG_PLAYER_YOC) && (defined(CONFIG_PLAYER_TPLAYER) && CONFIG_PLAYER_TPLAYER)
        m->hdl = xplayer_new(m->type == MEDIA_SYSTEM ? "yoc" : "tplayer");
#else
        /* FIXME: create default xplayer */
        m->hdl = xplayer_new(NULL);
#endif
        if (m->hdl == NULL) {
            LOGE(TAG, "play err(%s)", url);
        } else {
            xplayer_cnf_t conf;

            /* get default params */
            xplayer_get_config(m->hdl, &conf);

            conf.aef_conf            = m->config.aef_conf;
            conf.aef_conf_size       = m->config.aef_conf_size;
            conf.resample_rate       = m->config.resample_rate;
            conf.web_cache_size      = m->config.web_cache_size;
            conf.web_start_threshold = m->config.web_start_threshold;
            conf.snd_period_ms       = m->config.snd_period_ms;
            conf.snd_period_num      = m->config.snd_period_num;
            conf.eq_params           = g_eq_segments;
            conf.eq_segments         = g_eq_segment_count;
            xplayer_set_config(m->hdl, &conf);

            xplayer_set_speed(m->hdl, 1);
            xplayer_set_start_time(m->hdl, seek_time);
            xplayer_set_callback(m->hdl, player_event, m->hdl);
            xplayer_set_url(m->hdl, url);

            if (!m->mute) {
                if (m->play_vol < m->min_vol && m->cur_vol < m->min_vol) {
                    m_vol_set(m, m->min_vol);
                } else {
                    m_vol_set(m, m->play_vol);
                }
            } else {
                _audio_set_vol(m->type, 0, 0);
            }

            ret = xplayer_play(m->hdl);
            if (ret < 0) {
                LOGE(TAG, "player play failed, ret %d", ret);
                m_stop(m, 1);
                return -1;
            }
            m->url   = strdup(url);
            m->state = AUI_PLAYER_PLAYING;
        }
    }

    return m->hdl ? 0 : -1;
}

static int m_continue(media_type_t *m, int flag)
{
    int rc = 0;

    if (m->play_vol < m->min_vol) {
        LOGD(TAG, "set play vol to min_vol %d", m->min_vol / VOLUME_UNIT);
        m->play_vol = m->min_vol;
    }

    //flag == 0 pop mode, flag == 1 continue
    if (m->hdl && m->state == AUI_PLAYER_PAUSED) {
        if (m->mute == 0) {
            if (flag) {
                _audio_set_vol(m->type, m->play_vol, m->play_vol);
            } else {
                _audio_set_vol(m->type, m->cur_vol, m->cur_vol);
            }
        } else {
            _audio_set_vol(m->type, 0, 0);
        }

        media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_RESUME, NULL, 0);
        rc = xplayer_resume(m->hdl);
        m->state = AUI_PLAYER_PLAYING;
    }

    if (flag && m->mute == 1) {
        _audio_set_vol(m->type, m->play_vol, m->play_vol);
        m->cur_vol = m->play_vol;
        m->mute    = 0;
        media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_MUTE, (const void*)&m->mute, sizeof(m->mute));
    }

    return rc;
}

static void music_stash(void)
{
    media_type_t *music = get_type(MEDIA_MUSIC);

    if (music->state == AUI_PLAYER_PLAYING) {
        music->speech_pause = 1;
    }

    m_step_vol(music, per_to_abs(1), 100);

    m_pause(music);
}

static void music_stash_pop(void)
{
    media_type_t *m = get_type(MEDIA_MUSIC);

    if (m->speech_pause == 1) {
        m->speech_pause = 0;
        m_continue(m, 0);
        m_step_vol(m, m->play_vol, 1000);
    }
}

static void media_type_init()
{
    int vol = per_to_abs(60);

    for (int i = 0; i < MEDIA_MAX_NUM; i++) {
        media_type[i].type     = i;
        media_type[i].state    = AUI_PLAYER_STOP;
        media_type[i].play_vol = vol;
        media_type[i].min_vol  = per_to_abs(10);
        aui_player_config_init(&media_type[i].config);
    }
}

static int _init(media_t *media, rpc_t *rpc)
{
    xplayer_mdl_cnf_t conf;
    g_media.cb = (media_evt_t) rpc_get_point(rpc);;
    xplayer_module_config_init(&conf);
    xplayer_module_init(&conf);
    media_type_init();

    return 0;
}

// 开始播放（停止前一个播放）  "fifo://tts"
static int _play(media_t *media, rpc_t *rpc)
{
    play_param_t *param   = (play_param_t *)rpc_get_buffer(rpc, NULL);
    media_type_t *m       = get_type(param->type);
    int           ret     = -1;
    int           start   = 1;

    LOGD(TAG, "state:%d type:%d\n", m->state, m->type);

    //FIXME:
    media_evt_call(m->type, param->url, AUI_PLAYER_EVENT_START, NULL, 0);

    if (m->type == MEDIA_SYSTEM) {
        m->resume = param->resume;
        music_stash();
        m_stop(get_type(MEDIA_SYSTEM), 1);
    } else {
        media_type_t *m_s = get_type(MEDIA_SYSTEM);

        if (m_s->state == AUI_PLAYER_PLAYING) {
            start = 0;
        } else {
            m_stop(get_type(MEDIA_MUSIC), 1);
        }
    }

    if (start == 1) {
        ret = m_start(m, param->url, param->seek_time);

        if (ret < 0) {
            media_evt_call(m->type, param->url, AUI_PLAYER_EVENT_ERROR, NULL, 0);
        }
    }

    av_free(param->url);
    return 0;
}

// 暂停
static int _pause(media_t *media, rpc_t *rpc)
{
    int           type    = *(int *)rpc_get_point(rpc);
    int           rpc_ret = -1;

    if (type != MEDIA_ALL) {
        media_type_t *m = get_type(type);
        rpc_ret         = m_pause(m);
        m->speech_pause = 0;
    } else {
        for (int i = 0; i < MEDIA_MAX_NUM; i++) {
            media_type_t *m = get_type(i);
            rpc_ret         = m_pause(m);
            m->speech_pause = 0;
        }
    }

    rpc_return_int(rpc, rpc_ret);
    return 0;
}

// 继续
static int _continue(media_t *media, rpc_t *rpc)
{
    int           type    = *(int *)rpc_get_point(rpc);
    media_type_t *m       = get_type(type);
    int           rpc_ret = -1;

    if (m->play_vol < m->min_vol && m->cur_vol < m->min_vol) {
        m_vol_set(m, m->min_vol);
    }

    rpc_ret = m_continue(m, 1);

    rpc_return_int(rpc, rpc_ret);

    return 0;
}

// 停止
static int _stop(media_t *media, rpc_t *rpc)
{
    int type    = *(int *)rpc_get_point(rpc);
    int rpc_ret = -1;

    LOGD(TAG, "media%d stop\n", type);

    if (type != MEDIA_ALL) {
        rpc_ret = m_stop(get_type(type), 1);
    } else {
        for (int i = 0; i < MEDIA_MAX_NUM; i++) {
            rpc_ret = m_stop(get_type(i), 1);
        }
    }

    rpc_return_int(rpc, rpc_ret);

    return 0;
}

static int _mute(media_t *media, rpc_t *rpc)
{
    int           type = rpc_get_int(rpc);
    media_type_t *m;

    LOGD(TAG, "media(%d) mute", type);
    if (type != MEDIA_ALL) {
        m          = get_type(type);
        m->cur_vol = 0;
        if (m->mute != 1) {
            m->mute = 1;
            media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_MUTE, (const void*)&m->mute, sizeof(m->mute));
        }
    } else {
        for (int i = 0; i < MEDIA_MAX_NUM; i++) {
            m          = get_type(i);
            m->cur_vol = 0;
            if (m->mute != 1) {
                m->mute = 1;
                media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_MUTE, (const void*)&m->mute, sizeof(m->mute));
            }
        }
    }

    _audio_set_vol(type, 0, 0);

    return 0;
}

static int _unmute(media_t *media, rpc_t *rpc)
{
    int           type = rpc_get_int(rpc);
    media_type_t *m;

    LOGD(TAG, "media(%d) mute", type);
    if (type != MEDIA_ALL) {
        m          = get_type(type);
        m->cur_vol = m->play_vol;
        if (m->mute != 0) {
            m->mute = 0;
            media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_MUTE, (const void*)&m->mute, sizeof(m->mute));
        }
    } else {
        for (int i = 0; i < MEDIA_MAX_NUM; i++) {
            m          = get_type(i);
            m->cur_vol = m->play_vol;
            if (m->mute != 0) {
                m->mute = 0;
                media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_MUTE, (const void*)&m->mute, sizeof(m->mute));
            }
        }
    }

    _audio_set_vol(type, m->cur_vol, m->cur_vol);

    return 0;
}

static int _resume(media_t *media, rpc_t *rpc)
{
    int           type = rpc_get_int(rpc);
    media_type_t *m    = get_type(type);

    LOGD(TAG, "music resume");

    for (int i = 0; i < MEDIA_MAX_NUM; i++) {
        media_type_t *m_m = get_type(i);

        if (m != m_m && m_m->state == AUI_PLAYER_PLAYING) {
            LOGD(TAG, "music resume err(%d)(%d)", m_m->type, m_m->state);
            return 0;
        }
    }

    music_stash_pop();

    return 0;
}

static int _seek(media_t *media, rpc_t *rpc)
{
    seek_param_t *param = (seek_param_t *)rpc_get_point(rpc);
    media_type_t *m     = get_type(param->type);

    int rpc_ret = xplayer_seek(m->hdl, param->seek_time);

    rpc_return_int(rpc, rpc_ret);

    return 0;
}

static int _set_speed(media_t *media, rpc_t *rpc)
{
    speed_param_t *param = (speed_param_t *)rpc_get_point(rpc);
    media_type_t  *m     = get_type(param->type);

    int rpc_ret = xplayer_set_speed(m->hdl, param->speed);
    rpc_return_int(rpc, rpc_ret);

    return 0;
}

static int _get_speed(media_t *media, rpc_t *rpc)
{
    int type            = *(int *)rpc_get_point(rpc);
    media_type_t *m     = get_type(type);
    int rpc_ret         = -1;

    float speed = 0;

    rpc_ret = xplayer_get_speed(m->hdl, &speed);

    rpc_put_reset(rpc);
    rpc_put_buffer(rpc, &speed, sizeof(speed));

    return 0;
}

static int _get_media_info(media_t *media, rpc_t *rpc)
{
    int rc;
    xmedia_info_t minfo;
    int type        = *(int *)rpc_get_point(rpc);
    media_type_t *m = get_type(type);

    media_info_init(&minfo);
    rc = xplayer_get_media_info(m->hdl, &minfo);
    if (rc != 0) {
        LOGE(TAG, "err, rc = %d, %s", rc, __FUNCTION__);
    }

    rpc_put_reset(rpc);
    rpc_put_buffer(rpc, &minfo, sizeof(minfo));

    return 0;
}

#if !CONFIG_AV_AUDIO_ONLY_SUPPORT
static int _switch_audio_track(media_t *media, rpc_t *rpc)
{
    int rc;
    media_var_param_t *param = (media_var_param_t*)rpc_get_point(rpc);
    media_type_t  *m         = get_type(param->type);

    rc = xplayer_switch_audio_track(m->hdl, param->v.v_u8);
    if (rc != 0) {
        LOGE(TAG, "err, rc = %d, %s", rc, __FUNCTION__);
    }
    rpc_return_int(rpc, rc);

    return 0;
}

static int _switch_subtitle_track(media_t *media, rpc_t *rpc)
{
    int rc;
    media_var_param_t *param = (media_var_param_t*)rpc_get_point(rpc);
    media_type_t  *m         = get_type(param->type);

    rc = xplayer_switch_subtitle_track(m->hdl, param->v.v_u8);
    if (rc != 0) {
        LOGE(TAG, "err, rc = %d, %s", rc, __FUNCTION__);
    }
    rpc_return_int(rpc, rc);

    return 0;
}

static int _set_subtitle_url(media_t *media, rpc_t *rpc)
{
    int rc;
    media_var_param_t *param = (media_var_param_t*)rpc_get_point(rpc);
    media_type_t  *m         = get_type(param->type);

    rc = xplayer_set_subtitle_url(m->hdl, (const char*)param->v.v_point);
    if (rc != 0) {
        LOGE(TAG, "err, rc = %d, %s", rc, __FUNCTION__);
    }
    rpc_return_int(rpc, rc);

    return 0;
}

static int _set_subtitle_visible(media_t *media, rpc_t *rpc)
{
    int rc;
    media_var_param_t *param = (media_var_param_t*)rpc_get_point(rpc);
    media_type_t  *m         = get_type(param->type);

    rc = xplayer_set_subtitle_visible(m->hdl, param->v.v_u8);
    if (rc != 0) {
        LOGE(TAG, "err, rc = %d, %s", rc, __FUNCTION__);
    }
    rpc_return_int(rpc, rc);

    return 0;
}

static int _set_video_visible(media_t *media, rpc_t *rpc)
{
    int rc;
    media_var_param_t *param = (media_var_param_t*)rpc_get_point(rpc);
    media_type_t  *m         = get_type(param->type);

    rc = xplayer_set_video_visible(m->hdl, param->v.v_u8);
    if (rc != 0) {
        LOGE(TAG, "err, rc = %d, %s", rc, __FUNCTION__);
    }
    rpc_return_int(rpc, rc);

    return 0;
}

static int _set_video_crop(media_t *media, rpc_t *rpc)
{
    int rc;
    media_var_param_t *param = (media_var_param_t*)rpc_get_point(rpc);
    media_type_t  *m         = get_type(param->type);

    rc = xplayer_set_video_crop(m->hdl, (const xwindow_t*)param->v.v_point);
    if (rc != 0) {
        LOGE(TAG, "err, rc = %d, %s", rc, __FUNCTION__);
    }
    rpc_return_int(rpc, rc);

    return 0;
}

static int _set_display_window(media_t *media, rpc_t *rpc)
{
    int rc;
    media_var_param_t *param = (media_var_param_t*)rpc_get_point(rpc);
    media_type_t  *m         = get_type(param->type);

    rc = xplayer_set_display_window(m->hdl, (const xwindow_t*)param->v.v_point);
    if (rc != 0) {
        LOGE(TAG, "err, rc = %d, %s", rc, __FUNCTION__);
    }
    rpc_return_int(rpc, rc);

    return 0;
}

static int _set_fullscreen(media_t *media, rpc_t *rpc)
{
    int rc;
    media_var_param_t *param = (media_var_param_t*)rpc_get_point(rpc);
    media_type_t  *m         = get_type(param->type);

    rc = xplayer_set_fullscreen(m->hdl, param->v.v_u8);
    if (rc != 0) {
        LOGE(TAG, "err, rc = %d, %s", rc, __FUNCTION__);
    }
    rpc_return_int(rpc, rc);

    return 0;
}

static int _set_display_format(media_t *media, rpc_t *rpc)
{
    int rc;
    media_var_param_t *param = (media_var_param_t*)rpc_get_point(rpc);
    media_type_t  *m         = get_type(param->type);

    rc = xplayer_set_display_format(m->hdl, (xdisplay_format_t)param->v.v_u32);
    if (rc != 0) {
        LOGE(TAG, "err, rc = %d, %s", rc, __FUNCTION__);
    }
    rpc_return_int(rpc, rc);

    return 0;
}

static int _set_video_rotate(media_t *media, rpc_t *rpc)
{
    int rc;
    media_var_param_t *param = (media_var_param_t*)rpc_get_point(rpc);
    media_type_t  *m         = get_type(param->type);

    rc = xplayer_set_video_rotate(m->hdl, (xrotate_type_t)param->v.v_u32);
    if (rc != 0) {
        LOGE(TAG, "err, rc = %d, %s", rc, __FUNCTION__);
    }
    rpc_return_int(rpc, rc);

    return 0;
}

#endif

static int _get_time(media_t *media, rpc_t *rpc)
{
    int type            = *(int *)rpc_get_point(rpc);
    media_type_t *m     = get_type(type);
    int rpc_ret         = -1;

    play_time_t p = {
        .curtime = -1,
        .duration = -1,
    };

    rpc_ret = xplayer_get_time(m->hdl, &p);

    rpc_put_reset(rpc);
    rpc_put_buffer(rpc, &p, sizeof(p));

    return 0;
}

static int _config(media_t *media, rpc_t *rpc)
{
    aui_player_config_t *config = rpc_get_point(rpc);
    media_type_t *m;

    for (int i = 0; i < MEDIA_MAX_NUM; i++) {
        m = get_type(i);

        memcpy(&m->config, config, sizeof(aui_player_config_t));
    }

    return 0;
}

static int _key(media_t *media, rpc_t *rpc)
{
    media_key_cb_t *cb = rpc_get_point(rpc);
    media_type_t   *m;

    for (int i = 0; i < MEDIA_MAX_NUM; i++) {
        m = get_type(i);

        m->key_cb = *cb;
    }

    return 0;
}

// 增加增量设置， 正数加大，负数减小
static int _vol_add(media_t *media, rpc_t *rpc)
{
    vol_param_t * param = (vol_param_t *)rpc_get_point(rpc);
    media_type_t *m;
    int           volume;
    int           add_vol = per_to_abs(param->vol);
    int           rpc_ret = -1;

    if (param->type != MEDIA_ALL) {
        m       = get_type(param->type);
        volume  = m->play_vol + add_vol;
        rpc_ret = m_vol_set(m, volume);
        if (m->mute != 0) {
            m->mute    = 0;
            media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_MUTE, (const void*)&m->mute, sizeof(m->mute));
        }
    } else {
        for (int i = 0; i < MEDIA_MAX_NUM; i++) {
            m       = get_type(i);
            volume  = m->play_vol + add_vol;
            rpc_ret = m_vol_set(m, volume);
            if (m->mute != 0) {
                m->mute = 0;
                media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_MUTE, (const void*)&m->mute, sizeof(m->mute));
            }
        }
    }

    rpc_return_int(rpc, rpc_ret);

    return 0;
}

// 设置绝对音量
static int _vol_set(media_t *media, rpc_t *rpc)
{
    vol_param_t * param   = (vol_param_t *)rpc_get_point(rpc);
    int           rpc_ret = -1;
    int           volume  = per_to_abs(param->vol);
    media_type_t *m;

    if (param->type != MEDIA_ALL) {
        m       = get_type(param->type);
        rpc_ret = m_vol_set(m, volume);
        if (m->mute != 0) {
            m->mute = 0;
            media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_MUTE, (const void*)&m->mute, sizeof(m->mute));
        }
    } else {
        for (int i = 0; i < MEDIA_MAX_NUM; i++) {
            m = get_type(i);

            rpc_ret = m_vol_set(m, volume);
            if (m->mute != 0) {
                m->mute = 0;
                media_evt_call(m->type, m->url, AUI_PLAYER_EVENT_MUTE, (const void*)&m->mute, sizeof(m->mute));
            }
        }
    }

    rpc_return_int(rpc, rpc_ret);

    return 0;
}

static int _set_min_vol(media_t *media, rpc_t *rpc)
{
    vol_param_t * param  = (vol_param_t *)rpc_get_point(rpc);
    int           volume = per_to_abs(param->vol);
    media_type_t *m;

    if (param->type != MEDIA_ALL) {
        m          = get_type(param->type);
        m->min_vol = volume;

        if (m->play_vol < m->min_vol) {
            m->play_vol = m->min_vol;
        }
    } else {
        for (int i = 0; i < MEDIA_MAX_NUM; i++) {
            m = get_type(i);
            m->min_vol = volume;

            if (m->play_vol < m->min_vol) {
                m->play_vol = m->min_vol;
            }
        }
    }

    rpc_return_int(rpc, 0);

    return 0;
}

static int _vol_get(media_t *media, rpc_t *rpc)
{
    int           type = *(int *)rpc_get_point(rpc);
    media_type_t *m    = get_type(type);

    rpc_return_int(rpc, abs_to_per(m->play_vol));

    return 0;
}

static void _evt_hdl(rpc_t *rpc, int evt_id)
{
    event_param_t *param = (event_param_t *)rpc_get_buffer(rpc, NULL);
    int           type   = param->type;
    media_type_t *m      = (media_type_t *)get_type(type);

    m_stop(m, 0);
    if (type == MEDIA_SYSTEM && m->resume == 1) {
        music_stash_pop();
        m->resume = 0;
    }

    media_evt_call(m->type, param->url, evt_id, NULL, 0);
    aos_freep(&param->url);
    aos_freep((char**)&param->data);
}

static int _evt_err(media_t *media, rpc_t *rpc)
{
    //播放异常
    _evt_hdl(rpc, AUI_PLAYER_EVENT_ERROR);
    return 0;
}

static int _evt_ok(media_t *media, rpc_t *rpc)
{
    //播放完成
    _evt_hdl(rpc, AUI_PLAYER_EVENT_FINISH);
    return 0;
}

static int _evt_under_run(media_t *media, rpc_t *rpc)
{
    event_param_t *param = (event_param_t *)rpc_get_buffer(rpc, NULL);
    int           type   = param->type;
    media_type_t *m      = (media_type_t *)get_type(type);

    media_evt_call(m->type, param->url, AUI_PLAYER_EVENT_UNDER_RUN, NULL, 0);
    aos_freep(&param->url);
    aos_freep((char**)&param->data);

    return 0;
}

static int _evt_over_run(media_t *media, rpc_t *rpc)
{
    event_param_t *param = (event_param_t *)rpc_get_buffer(rpc, NULL);
    int           type   = param->type;
    media_type_t *m      = (media_type_t *)get_type(type);

    media_evt_call(m->type, param->url, AUI_PLAYER_EVENT_OVER_RUN, NULL, 0);
    aos_freep(&param->url);
    aos_freep((char**)&param->data);

    return 0;
}

static int _evt_run(media_t *media, rpc_t *rpc)
{
    event_param_t *param = (event_param_t *)rpc_get_buffer(rpc, NULL);
    //FIXME: play start event upload before

    aos_freep(&param->url);
    aos_freep((char**)&param->data);

    return 0;
}

static void _step_vol_handle(int type)
{
    media_type_t *m = get_type(type);

    if (m->cur_vol == m->exp_vol) {
        return;
    }

    m->cur_vol -= m->step_vol;

    if (!m->mute)
        _audio_set_vol(type, m->cur_vol, m->cur_vol);

    if ((m->cur_vol - m->exp_vol) / m->step_vol == 0) {
        m->step_vol = m->cur_vol - m->exp_vol;
    }
}

static int _get_step_vol(int old, int new, int ms)
{
    ms          = ms > VOL_STEP_MAX_TIME ? VOL_STEP_MAX_TIME : ms;
    int i       = ms / VOL_STEP_TIME;
    int vol_gap = old - new;
    int flag    = vol_gap > 0 ? 1 : -1; //+:1 -:0

    vol_gap = vol_gap > 0 ? vol_gap : -vol_gap;

    if (i <= 1 || vol_gap < i) {
        return vol_gap * flag;
    } else {
        return (flag * (vol_gap + i / 2) / i);
    }
}

static void _step_vol_sleep(int t1, int t2)
{
    if (t1 > t2) {
        if ((t1 - t2) < VOL_STEP_TIME) {
            aos_msleep(VOL_STEP_TIME - (t1 - t2));
        }
    } else {
        aos_msleep(VOL_STEP_TIME + (t2 - t1));
    }
}

static void m_step_vol(media_type_t *m, int new_vol, int time)
{
    if (m->hdl && m->state == AUI_PLAYER_PLAYING) {
        int       i, j;
        long long old_t, new_t, off_t;

        m->exp_vol  = new_vol;
        m->step_vol = _get_step_vol(m->cur_vol, new_vol, time);

        i = time / VOL_STEP_TIME;
        j = 0;
        LOGD(TAG, "media vol start(%d)(%d)(%d) :%d", m->cur_vol / 1, i, m->step_vol, aos_now_ms());
        old_t = aos_now_ms();
        off_t = 0;

        while (i--) {
            _step_vol_handle(m->type);
            //printf("---%d---\n", m->cur_vol);
            _step_vol_sleep(off_t, VOL_STEP_TIME * j);
            new_t = aos_now_ms();
            off_t = new_t - old_t;
            j++;
        }

        _step_vol_handle(m->type);
        LOGD(TAG, "media vol end(%d) :%d", m->cur_vol / 1, aos_now_ms());
    }
}
// 音量在指定的时间内逐渐调整到指定音量
static int _vol_gradual(media_t *media, rpc_t *rpc)
{
    vol_param_t * param = (vol_param_t *)rpc_get_buffer(rpc, NULL);
    media_type_t *m     = get_type(param->type);

    m_step_vol(m, param->vol, param->time);

    m->play_vol = param->vol;

    return 0;
}

int aui_player_play(int type, const char *url, int resume)
{
    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM && url && strlen(url) > 0);
    int          ret = -1;
    play_param_t param;

    param.type = type;
    param.url  = strdup(url);
    param.resume = resume;
    param.seek_time = 0;

    ret = uservice_call_async(g_media.srv, MEDIA_PLAY_CMD, &param, sizeof(play_param_t));
    return ret;
}

int aui_player_seek_play(int type, const char *url, uint64_t seek_time, int resume)
{
    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM && url && strlen(url) > 0);
    int          ret = -1;
    play_param_t param;

    param.type = type;
    param.url  = strdup(url);
    param.resume = resume;
    param.seek_time = seek_time;


    ret = uservice_call_async(g_media.srv, MEDIA_PLAY_CMD, &param, sizeof(play_param_t));
    return ret;
}

int aui_player_pause(int type)
{
    aos_check_return_einval((type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM) || type == MEDIA_ALL);
    int ret = -1;
    int rpc_ret;

    ret =
        uservice_call_sync(g_media.srv, MEDIA_PAUSE_CMD, (void *)&type, &rpc_ret, sizeof(rpc_ret));
    return ret < 0 ? ret : rpc_ret;
}

int aui_player_resume(int type)
{
    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    int ret = -1;
    int rpc_ret;
    ret =
        uservice_call_sync(g_media.srv, MEDIA_RESUME_CMD, (void *)&type, &rpc_ret, sizeof(rpc_ret));
    return ret < 0 ? ret : rpc_ret;
}

int aui_player_stop(int type)
{
    aos_check_return_einval((type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM) || type == MEDIA_ALL);
    int ret = -1;
    int rpc_ret;
    ret = uservice_call_sync(g_media.srv, MEDIA_STOP_CMD, (void *)&type, &rpc_ret, sizeof(rpc_ret));
    return ret < 0 ? ret : rpc_ret;
}

int aui_player_mute(int type)
{
    aos_check_return_einval((type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM) || type == MEDIA_ALL);
    int ret = -1;

    ret = uservice_call_async(g_media.srv, MEDIA_MUTE_CMD, &type, sizeof(int));
    return ret;
}

int aui_player_unmute(int type)
{
    aos_check_return_einval((type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM) || type == MEDIA_ALL);
    int ret = -1;

    ret = uservice_call_async(g_media.srv, MEDIA_UNMUTE_CMD, &type, sizeof(int));
    return ret;
}

int aui_player_seek(int type, uint64_t seek_time)
{
    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    int ret = -1;
    int rpc_ret;
    seek_param_t param;

    param.type      = type;
    param.seek_time = seek_time;

    ret = uservice_call_sync(g_media.srv, MEDIA_SEEK_CMD, (void *)&param, &rpc_ret, sizeof(rpc_ret));
    return ret < 0 ? ret : rpc_ret;
}

int aui_player_get_time(int type, aui_play_time_t *t)
{
    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    int ret = -1;

    ret = uservice_call_sync(g_media.srv, MEDIA_GET_TIME_CMD, (void *)&type, t, sizeof(aui_play_time_t));
    return ret;
}
/*
   int media_event(int type, int state)
   {
   aos_check_return_einval((type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM) || type == MEDIA_ALL);
   int ret = -1;
// ret = uservice_call_sync(g_media.srv, MEDIA_, &type, NULL, 0);
return ret;
}
*/

int aui_player_vol_adjust(int type, int inc_volume)
{
    aos_check_return_einval((type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM) || type == MEDIA_ALL);
    int         ret = -1;
    vol_param_t param;
    int         rpc_ret;

    param.type = type;
    param.vol  = inc_volume;
    ret = uservice_call_sync(g_media.srv, MEDIA_ADD_VOL_CMD, &param, &rpc_ret, sizeof(rpc_ret));
    return ret < 0 ? ret : rpc_ret;
}

int aui_player_vol_set(int type, int volume)
{
    aos_check_return_einval((type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM) || type == MEDIA_ALL);

    if (volume < VOLUME_MIN) {
        volume = VOLUME_MIN;
    } else if(volume > (VOLUME_MAX / VOLUME_UNIT)) {
        volume = (VOLUME_MAX / VOLUME_UNIT);
    }

    int         ret = -1;
    vol_param_t param;
    int         rpc_ret;

    param.type = type;
    param.vol  = volume;

    ret = uservice_call_sync(g_media.srv, MEDIA_SET_VOL_CMD, &param, &rpc_ret, sizeof(rpc_ret));
    return ret < 0 ? ret : rpc_ret;
}

int aui_player_set_minvol(int type, int volume)
{
    aos_check_return_einval((type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM) || type == MEDIA_ALL);

    if (volume < VOLUME_MIN || volume > (VOLUME_MAX / VOLUME_UNIT)) {
        return -1;
    }

    int         ret = -1;
    vol_param_t param;
    int         rpc_ret;

    param.type = type;
    param.vol  = volume;

    ret = uservice_call_sync(g_media.srv, MEDIA_SET_MIN_VOL_CMD, &param, &rpc_ret, sizeof(rpc_ret));
    return ret < 0 ? ret : rpc_ret;
}

int aui_player_vol_get(int type)
{
    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    int ret = -1;
    int vol = -1;

    ret = uservice_call_sync(g_media.srv, MEDIA_GET_VOL_CMD, &type, &vol, sizeof(int));
    return ret < 0 ? ret : vol;
}

int aui_player_config_init(aui_player_config_t *config)
{
    aos_check_return_einval(config);
    memset(config, 0, sizeof(aui_player_config_t));

    config->web_cache_size      = CONFIG_AV_STREAM_CACHE_SIZE_DEFAULT;
    config->web_start_threshold = CONFIG_AV_STREAM_CACHE_THRESHOLD_DEFAULT;
    config->snd_period_ms       = AO_ONE_PERIOD_MS;
    config->snd_period_num      = AO_TOTAL_PERIOD_NUM;
    config->db_min              = VOL_SCALE_DB_MIN;
    config->db_max              = VOL_SCALE_DB_MAX;

    return 0;
}

int aui_player_config(const aui_player_config_t *config)
{
    int ret = -1;

    CHECK_PARAM(config && config->db_min < config->db_max, -1);
    ret = uservice_call_sync(g_media.srv, MEDIA_CONFIG_CMD, (void*)config, NULL, 0);
    return ret;
}

int aui_player_key_config(media_key_cb_t cb)
{
    aos_check_return_einval(cb);
    int ret = -1;

    ret = uservice_call_sync(g_media.srv, MEDIA_CONFIG_KEY_CMD, &cb, NULL, 0);
    return ret;
}

int aui_player_vol_gradual(int type, int new_volume, int ms)
{
    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);

    if (ms > 5000) {
        ms = 5000;
    }

    if (new_volume < VOLUME_MIN || new_volume > (VOLUME_MAX / VOLUME_UNIT)) {
        return -1;
    }

    int         ret = -1;
    vol_param_t param;

    param.type = type;
    param.vol  = per_to_abs(new_volume);
    // param.step_vol = step_vol;
    param.time = ms;

    ret = uservice_call_async(g_media.srv, MEDIA_STEP_VOL_CMD, &param, sizeof(vol_param_t));
    return ret;
}

aui_player_state_t aui_player_get_state(int type)
{
    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    int state = AUI_PLAYER_UNKNOWN;

    if (g_media.srv == NULL) {
        return state;
    }

    if (media_type[0].state ==  AUI_PLAYER_UNKNOWN || media_type[1].state ==  AUI_PLAYER_UNKNOWN) {
        return state;
    }

    media_type_t *m = get_type(type);

    return m->speech_pause == 0 ? m->state : AUI_PLAYER_PLAYING;
}

int aui_player_resume_music(void)
{
    int type = MEDIA_MUSIC;
    int ret  = -1;

    ret = uservice_call_async(g_media.srv, MEDIA_CONTINUE_MUSIC_CMD, &type, sizeof(int));
    return ret;
}

int aui_player_get_speed(int type, float *speed)
{
    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);

    int ret = uservice_call_sync(g_media.srv, MEDIA_GET_SPEED_CMD, &type, speed, sizeof(float));
    return ret;
}

int aui_player_set_speed(int type, float speed)
{
    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    speed_param_t param;

    int ret   = -1;
    int rpc_ret;

    param.type  = type;
    param.speed = speed;

    ret = uservice_call_sync(g_media.srv, MEDIA_SET_SPEED_CMD, (void *)&param, &rpc_ret, sizeof(rpc_ret));
    return ret < 0 ? ret : rpc_ret;
}

int aui_player_eq_config(eqfp_t *eq_segments, int count)
{
    if (eq_segments == NULL || count <= 0) {
        av_free(g_eq_segments);
        g_eq_segments = NULL;
        g_eq_segment_count = 0;
        return 0;
    }

    av_free(g_eq_segments);
    g_eq_segments = av_malloc(sizeof(eqfp_t) * count);
    g_eq_segment_count = count;

    memcpy(g_eq_segments, eq_segments, sizeof(eqfp_t) * count);

    return 0;
}

/**
 * @brief  get media info of the type
 * @param  [in] type
 * @param  [in/out] minfo
 * @return 0/-1
 */
int aui_player_get_media_info(int type, xmedia_info_t *minfo)
{
    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM && minfo);

    int rc = uservice_call_sync(g_media.srv, MEDIA_GET_MEDIA_INFO_CMD, &type, minfo, sizeof(xmedia_info_t));
    return rc;
}

#if !CONFIG_AV_AUDIO_ONLY_SUPPORT
/**
 * @brief  switch audio track of the type
 * @param  [in] type
 * @param  [in] idx : audio index
 * @return 0/-1
 */
int aui_player_switch_audio_track(int type, uint8_t idx)
{
    int rc, ret = -1;
    media_var_param_t param;

    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    param.type   = type;
    param.v.v_u8 = idx;

    rc = uservice_call_sync(g_media.srv, MEDIA_SWITCH_AUDIO_TRACK_CMD, (void *)&param, &ret, sizeof(ret));
    return rc < 0 ? -1 : ret;
}

/**
 * @brief  switch subtitle track of the type
 * @param  [in] type
 * @param  [in] idx : subtitle index
 * @return 0/-1
 */
int aui_player_switch_subtitle_track(int type, uint8_t idx)
{
    int rc, ret = -1;
    media_var_param_t param;

    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    param.type   = type;
    param.v.v_u8 = idx;

    rc = uservice_call_sync(g_media.srv, MEDIA_SWITCH_SUBTITLE_TRACK_CMD, (void *)&param, &ret, sizeof(ret));
    return rc < 0 ? -1 : ret;
}

/**
 * @brief  set extern subtitle of the type
 * @param  [in] type
 * @param  [in] url
 * @return 0/-1
 */
int aui_player_set_subtitle_url(int type, const char *url)
{
    int rc, ret = -1;
    media_var_param_t param;

    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM && url);
    param.type      = type;
    param.v.v_point = (void*)url;

    rc = uservice_call_sync(g_media.srv, MEDIA_SET_SUBTITLE_URL_CMD, (void *)&param, &ret, sizeof(ret));
    return rc < 0 ? -1 : ret;
}

/**
 * @brief  hide/show subtitle of the type
 * @param  [in] type
 * @param  [in] visible
 * @return 0/-1
 */
int aui_player_set_subtitle_visible(int type, uint8_t visible)
{
    int rc, ret = -1;
    media_var_param_t param;

    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    param.type   = type;
    param.v.v_u8 = visible;

    rc = uservice_call_sync(g_media.srv, MEDIA_SET_SUBTITLE_VISIBLE_CMD, (void *)&param, &ret, sizeof(ret));
    return rc < 0 ? -1 : ret;
}

/**
 * @brief  hide/show video of the type
 * @param  [in] type
 * @param  [in] visible
 * @return 0/-1
 */
int aui_player_set_video_visible(int type, uint8_t visible)
{
    int rc, ret = -1;
    media_var_param_t param;

    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    param.type   = type;
    param.v.v_u8 = visible;

    rc = uservice_call_sync(g_media.srv, MEDIA_SET_VIDEO_VISIBLE_CMD, (void *)&param, &ret, sizeof(ret));
    return rc < 0 ? -1 : ret;
}

/**
 * @brief  crop video of the type
 * @param  [in] type
 * @param  [in] win
 * @return 0/-1
 */
int aui_player_set_video_crop(int type, const xwindow_t *win)
{
    int rc, ret = -1;
    media_var_param_t param;

    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM && win);
    param.type      = type;
    param.v.v_point = (void*)win;

    rc = uservice_call_sync(g_media.srv, MEDIA_SET_VIDEO_CROP_CMD, (void *)&param, &ret, sizeof(ret));
    return rc < 0 ? -1 : ret;
}

/**
 * @brief  set display window of the type
 * @param  [in] type
 * @param  [in] win
 * @return 0/-1
 */
int aui_player_set_display_window(int type, const xwindow_t *win)
{
    int rc, ret = -1;
    media_var_param_t param;

    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM && win);
    param.type      = type;
    param.v.v_point = (void*)win;

    rc = uservice_call_sync(g_media.srv, MEDIA_SET_DISPLAY_WINDOW_CMD, (void *)&param, &ret, sizeof(ret));
    return rc < 0 ? -1 : ret;
}

/**
 * @brief  on/off fullscreen of the type
 * @param  [in] type
 * @param  [in] onoff
 * @return 0/-1
 */
int aui_player_set_fullscreen(int type, uint8_t onoff)
{
    int rc, ret = -1;
    media_var_param_t param;

    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    param.type   = type;
    param.v.v_u8 = onoff;

    rc = uservice_call_sync(g_media.srv, MEDIA_SET_FULLSCREEN_CMD, (void *)&param, &ret, sizeof(ret));
    return rc < 0 ? -1 : ret;
}

/**
 * @brief  set display format of the type
 * @param  [in] type
 * @param  [in] format
 * @return 0/-1
 */
int aui_player_set_display_format(int type, xdisplay_format_t format)
{
    int rc, ret = -1;
    media_var_param_t param;

    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    param.type    = type;
    param.v.v_u32 = (uint32_t)format;

    rc = uservice_call_sync(g_media.srv, MEDIA_SET_DISPLAY_FORMAT_CMD, (void *)&param, &ret, sizeof(ret));
    return rc < 0 ? -1 : ret;
}

/**
 * @brief  rotate video of the type
 * @param  [in] type
 * @param  [in] rotate_type
 * @return 0/-1
 */
int aui_player_set_video_rotate(int type, xrotate_type_t rotate_type)
{
    int rc, ret = -1;
    media_var_param_t param;

    aos_check_return_einval(type >= MEDIA_MUSIC && type <= MEDIA_SYSTEM);
    param.type    = type;
    param.v.v_u32 = (uint32_t)rotate_type;

    rc = uservice_call_sync(g_media.srv, MEDIA_SET_VIDEO_ROTATE_CMD, (void *)&param, &ret, sizeof(ret));
    return rc < 0 ? -1 : ret;
}

#endif

static const rpc_process_t c_media_cmd_cb_table[] = {
    { MEDIA_INIT_CMD,                  (process_t)_init },
    { MEDIA_PLAY_CMD,                  (process_t)_play },
    { MEDIA_PAUSE_CMD,                 (process_t)_pause },
    { MEDIA_RESUME_CMD,                (process_t)_continue },
    { MEDIA_STOP_CMD,                  (process_t)_stop },
    { MEDIA_SEEK_CMD,                  (process_t)_seek },
    { MEDIA_SET_VOL_CMD,               (process_t)_vol_set },
    { MEDIA_SET_MIN_VOL_CMD,           (process_t)_set_min_vol },
    { MEDIA_GET_VOL_CMD,               (process_t)_vol_get },
    { MEDIA_ADD_VOL_CMD,               (process_t)_vol_add },
    { MEDIA_STEP_VOL_CMD,              (process_t)_vol_gradual },
    { MEDIA_EVT_ERR_CMD,               (process_t)_evt_err },
    { MEDIA_EVT_RUN_CMD,               (process_t)_evt_run },
    { MEDIA_EVT_END_CMD,               (process_t)_evt_ok },
    { MEDIA_EVT_UNDER_RUN_CMD,         (process_t)_evt_under_run },
    { MEDIA_EVT_OVER_RUN_CMD,          (process_t)_evt_over_run },
    { MEDIA_MUTE_CMD,                  (process_t)_mute },
    { MEDIA_UNMUTE_CMD,                (process_t)_unmute },
    { MEDIA_CONTINUE_MUSIC_CMD,        (process_t)_resume },
    { MEDIA_CONFIG_CMD,                (process_t)_config },
    { MEDIA_CONFIG_KEY_CMD,            (process_t)_key },
    { MEDIA_GET_TIME_CMD,              (process_t)_get_time },
    { MEDIA_SET_SPEED_CMD,             (process_t)_set_speed },
    { MEDIA_GET_SPEED_CMD,             (process_t)_get_speed },
    { MEDIA_GET_MEDIA_INFO_CMD,        (process_t)_get_media_info },
#if !CONFIG_AV_AUDIO_ONLY_SUPPORT
    { MEDIA_SWITCH_AUDIO_TRACK_CMD,    (process_t)_switch_audio_track },
    { MEDIA_SWITCH_SUBTITLE_TRACK_CMD, (process_t)_switch_subtitle_track },
    { MEDIA_SET_SUBTITLE_URL_CMD,      (process_t)_set_subtitle_url },
    { MEDIA_SET_SUBTITLE_VISIBLE_CMD,  (process_t)_set_subtitle_visible },
    { MEDIA_SET_VIDEO_VISIBLE_CMD,     (process_t)_set_video_visible },
    { MEDIA_SET_VIDEO_CROP_CMD,        (process_t)_set_video_crop },
    { MEDIA_SET_DISPLAY_WINDOW_CMD,    (process_t)_set_display_window },
    { MEDIA_SET_FULLSCREEN_CMD,        (process_t)_set_fullscreen },
    { MEDIA_SET_DISPLAY_FORMAT_CMD,    (process_t)_set_display_format },
    { MEDIA_SET_VIDEO_ROTATE_CMD,      (process_t)_set_video_rotate },
#endif
    { MEDIA_END_CMD, (process_t)NULL },
};

int media_process_rpc(void *context, rpc_t *rpc)
{
    return uservice_process(context, rpc, c_media_cmd_cb_table);
}

int aui_player_init(utask_t *task, media_evt_t evt_cb)
{
    aos_check_return_einval(task);

    if (g_media.srv) {
        return -1;
    }

    g_media.srv = uservice_new("media", media_process_rpc, &g_media);

    aos_check_return_enomem(g_media.srv);
    utask_add(task, g_media.srv);
    uservice_call_async(g_media.srv, MEDIA_INIT_CMD, &evt_cb, sizeof(&evt_cb));

    return 0;
}
#endif
