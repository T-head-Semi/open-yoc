/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <time.h>

#include "app_main.h"
#include "app_player.h"

#define TAG "app_player"

/*************************************************
 * 本地音播放
 *************************************************/
static local_voice_t local_voices[] = {
    {LOCAL_AUDIO_NET_CFG_CONFIG, "net_cfg_config.mp3"},
    {LOCAL_AUDIO_NET_CFG_CONN, "net_cfg_conn.mp3"},
    {LOCAL_AUDIO_NET_CFG_FAIL, "net_cfg_fail.mp3"},
    {LOCAL_AUDIO_NET_CFG_START, "net_cfg_start.mp3"},
    {LOCAL_AUDIO_NET_CFG_SWITCH, "net_cfg_switch.mp3"},
    {LOCAL_AUDIO_NET_CFG_TIMEOUT, "net_cfg_timeout.mp3"},
    {LOCAL_AUDIO_NET_FAIL, "net_fail.mp3"},
    {LOCAL_AUDIO_NET_SUCC, "net_succ.mp3"},
    {LOCAL_AUDIO_OK, "ok.mp3"},
    {LOCAL_AUDIO_PLAY_ERR, "play_err.mp3"},
    {LOCAL_AUDIO_SORRY, "sorry.mp3"},
    {LOCAL_AUDIO_SORRY2, "sorry2.mp3"},
    {LOCAL_AUDIO_STARTING, "starting.mp3"},
    {LOCAL_AUDIO_BT_CONNECT, "bt_connected.mp3"},
    {LOCAL_AUDIO_BT_DISCONNECT, "bt_disconnected.mp3"},
    {LOCAL_AUDIO_WAKEUP_COMING, "wakeup_coming.mp3"},
    {LOCAL_AUDIO_WAKEUP_GO_AHEAD, "wakeup_go_ahead.mp3"},
    {LOCAL_AUDIO_WAKEUP_HELLO, "wakeup.wav"},
    {LOCAL_AUDIO_WAKEUP_HI, "wakeup_hi.mp3"},
    {LOCAL_AUDIO_WAKEUP_IM_HERE, "wakeup_im_here.mp3"},
};

static int wakeup_audio_stat = 0; //1: 唤醒提示音正在播放
static int local_audio_stat  = 0; //1：本地提示音正在播放

static int _audio_play_(local_voice_tag_t name, int resume)
{
    char local_url[64];

    snprintf(local_url, sizeof(local_url), "file:///mnt/%s", local_voices[name].name);

    local_audio_stat = 1;
    return smtaudio_start(MEDIA_SYSTEM, local_url, 0, resume);
}

int local_audio_play(local_voice_tag_t name)
{
    return _audio_play_(name, 1);
}

int local_wakeup_audio_play(local_voice_tag_t name)
{
    int ret;

    if (name < LOCAL_AUDIO_WAKEUP_COMING || name > LOCAL_AUDIO_WAKEUP_IM_HERE) {
        time_t curtime = time(NULL);
        name           = (curtime % (LOCAL_AUDIO_WAKEUP_IM_HERE - LOCAL_AUDIO_WAKEUP_COMING + 1)) + LOCAL_AUDIO_WAKEUP_IM_HERE;
    }

    //aui_mic_mute(10000);
    ret = _audio_play_(name, 0);
    if (ret == 0) {
        wakeup_audio_stat = 1;
    } else {
        //aui_mic_unmute();
    }

    return ret;
}

void local_audio_finish_check()
{
    if (wakeup_audio_stat) {
        //aui_mic_unmute();
        wakeup_audio_stat = 0;
    }

    local_audio_stat = 0;
}

int local_audio_stat_get()
{
    return local_audio_stat;
}

/*************************************************
 * 播放器初始化
 *************************************************/
static void media_evt(int type, smtaudio_player_evtid_t evt_id)
{
    //LOGD(TAG, "media_evt type %d,evt_id %d", type, evt_id);
    if((type == SMTAUDIO_ONLINE_MUSIC) || (type == SMTAUDIO_LOCAL_PLAY)) {
        switch (evt_id) {
            case SMTAUDIO_PLAYER_EVENT_START:
                event_publish(EVENT_MEDIA_START, NULL);
                break;

            case SMTAUDIO_PLAYER_EVENT_ERROR:
                event_publish(type == SMTAUDIO_LOCAL_PLAY ? EVENT_MEDIA_SYSTEM_ERROR : EVENT_MEDIA_MUSIC_ERROR, NULL);
                if(SMTAUDIO_ONLINE_MUSIC == type) {
                    /* media_system doesn't need to play error audio */
                    LOGE(TAG, "SMTAUDIO_PLAYER_EVENT_ERROR");
                } else {
                    //app_tts_update_running(TTS_STATE_IDLE);
                }
                local_audio_finish_check();
                LOGI(TAG, "audio player exit %d", av_errno_get());
                //app_lpm_update();
                break;

            case SMTAUDIO_PLAYER_EVENT_STOP:
                event_publish(type == SMTAUDIO_LOCAL_PLAY ? EVENT_MEDIA_SYSTEM_FINISH : EVENT_MEDIA_MUSIC_FINISH, NULL);
                if(SMTAUDIO_LOCAL_PLAY == type) {
                    //app_tts_update_running(TTS_STATE_IDLE);
                }
                local_audio_finish_check();
                LOGD(TAG, "audio player exit %d", SMTAUDIO_PLAYER_EVENT_STOP);
                //app_lpm_update();
                break;
            case SMTAUDIO_PLAYER_EVENT_RESUME:
                // event_publish(EVENT_MEDIA_START, NULL);
                LOGD(TAG, "audio player resumed %d", SMTAUDIO_PLAYER_EVENT_RESUME);
                break;
            case SMTAUDIO_PLAYER_EVENT_UNDER_RUN:
                LOGD(TAG, "audio player underrun");
                break;

            case SMTAUDIO_PLAYER_EVENT_OVER_RUN:
                LOGD(TAG, "audio player overrun");
                break;

            default:
                break;
        }
    }
#if defined(AW2013_ENABLED) && AW2013_ENABLED
    if (evt_id == SMTAUDIO_PLAYER_EVENT_START || evt_id == SMTAUDIO_PLAYER_EVENT_STOP || evt_id == SMTAUDIO_PLAYER_EVENT_ERROR) {
        g_aui_player_evtid_t = evt_id;
    }
    if (type == SMTAUDIO_ONLINE_MUSIC || type == SMTAUDIO_LOCAL_PLAY) {
        g_aui_player_type_t = type;
    }
#endif
}

static void pa_check_event(uint32_t event_id, const void *data, void *context);
static aos_mutex_t pa_lock = { NULL };
int app_player_init(void)
{
    unsigned char *eqcfg = NULL;
    unsigned int eqcfg_size = 0;

    aos_mutex_new(&pa_lock);

#if 0
    int eq_en = 1;
    aos_kv_getint("eq_en", &eq_en);
    LOGD(TAG, "EQ enable %d", eq_en);

    if (eq_en) {
        board_audio_get_eqconfig(&eqcfg, &eqcfg_size, eq_en);
    }
#endif
    smtaudio_init(media_evt);
#ifndef CONFIG_AEFXER_SONA_V1
    smtaudio_register_local_play(0, (uint8_t *)eqcfg, eqcfg_size, 1.0f, 0);
    smtaudio_register_online_music(0, (uint8_t *)eqcfg, eqcfg_size, 1.0f, 0);
#else
    smtaudio_register_local_play(0, (uint8_t *)eqcfg, eqcfg_size, 1.0f, 44100);
    smtaudio_register_online_music(0, (uint8_t *)eqcfg, eqcfg_size, 1.0f, 44100);
#endif

    /* 配置音量曲线 */
#ifdef VOLUME_MARK_POINTS
    int mark_point[] = VOLUME_MARK_POINTS;
    extern void aui_vol_set_mark(int *mark_point, int count);
    aui_vol_set_mark(mark_point, sizeof(mark_point) / sizeof(int));
#endif

    event_subscribe(EVENT_PA_CHECK, pa_check_event, NULL);
    event_publish_delay(EVENT_PA_CHECK, NULL, 1000);

    extern void cli_reg_cmd_player(void);
    cli_reg_cmd_player();

    return 0;
}


/*************************************************
 * PA控制-延时关闭
 *************************************************/
static volatile long long g_pa_delay_mute = 0;
static void pa_check_event(uint32_t event_id, const void *data, void *context)
{
    //printf("pa check %d\n", g_pa_delay_mute);
    if (event_id == EVENT_PA_CHECK) {
        aos_mutex_lock(&pa_lock, AOS_WAIT_FOREVER);
        if (g_pa_delay_mute) {
            long long now = aos_now_ms();
            if (now - g_pa_delay_mute > 3000) {
                app_speaker_mute(1);
                g_pa_delay_mute = 0;
            }
        }
        aos_mutex_unlock(&pa_lock);
    }
    event_publish_delay(EVENT_PA_CHECK, NULL, 1000);
}

void ao_event_hook(int ao_evt)
{
    if (pa_lock.hdl == NULL) {
        return;
    }

    switch(ao_evt) {
        case 0:/* 结束播放 */
            /* 延时关闭 */
            aos_mutex_lock(&pa_lock, AOS_WAIT_FOREVER);
            if (g_pa_delay_mute == 0) {
                g_pa_delay_mute = aos_now_ms();
            }
            aos_mutex_unlock(&pa_lock);
            //app_speaker_mute(1);
            break;
        case 1:/* 开始播放 */
            //printf("pa update\n");
            aos_mutex_lock(&pa_lock, AOS_WAIT_FOREVER);
            g_pa_delay_mute = 0;
            aos_mutex_unlock(&pa_lock);
            app_speaker_mute(0);
            break;
        default:
            ;
    }
    return;
}

#include "drv_amp.h"
void app_speaker_init(void)
{
    //amplifier_init(AMP_TYPE_CS8122S, PIN_PA_MUTE, -1, AMP_MODE_DEF);
}

void app_speaker_mute(int mute)
{
    if(smtaudio_get_state() == SMTAUDIO_STATE_MUTE)
        mute = 1;

    LOGD(TAG, "%s %d", __func__, mute);
    amplifier_onoff(mute ? 0 : 1);
}