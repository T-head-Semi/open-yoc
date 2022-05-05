/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <aos/kv.h>
#include <yoc/mic.h>
#include <ulog/ulog.h>
// #include <avutil/av_errno.h>
#include <uservice/uservice.h>

#include "player/app_player.h"
#include "aui_cloud/app_aui_cloud.h"
#include "app_audio.h"

#define TAG "appaud"

#define SESSION_STATE_IDLE 0
#define SESSION_STATE_START 1
#define SESSION_STATE_WWV 2

/*************************************************
 * 麦克风
 *************************************************/
static unsigned int send_byte = 0;
static int session_state;
static uint32_t g_wakeup_cnt = 0;

/* 接收到 MIC 事件 */
static void mic_evt_cb(int source, mic_event_id_t evt_id, void *data, int size)
{
    int  ret    = 0;
    int  type   = 0;
    char keywords[128];

    switch (evt_id) {
        case MIC_EVENT_PCM_DATA:
        {
            // if (session_state == SESSION_STATE_IDLE)
            //     break;
            // LOGD(TAG, "mic_evt_cb session pcm %d\n", size);

            /* 麦克风数据，推到云端 */
            ret = app_aui_cloud_push_audio(data, size);
            if (ret < 0) {
                /* 数据推送错误 */
                session_state = SESSION_STATE_IDLE;
                LOGE(TAG, "cloud push pcm finish. state %d", session_state);
                //aui_mic_control(MIC_CTRL_STOP_PCM);
                ret = app_aui_cloud_stop(1);
#if 0
                /* 网络检测 */
                if (wifi_internet_is_connected() == 0) {
                    LOGE(TAG, "mic evt ntp not synced");
                    local_audio_play(LOCAL_AUDIO_NET_FAIL);
                } else {
                    if (ret < 0) {
                        local_audio_play(LOCAL_AUDIO_SORRY2);
                    }
                }
#endif
            }
            send_byte += size;
        }
        break;

        case MIC_EVENT_SESSION_START:
            //app_lpm_update();
            app_speaker_mute(0);
            if (SMTAUDIO_STATE_MUTE == smtaudio_get_state()) {
                LOGD(TAG, "Device is mute\n");
                return;
            }
            LOGD(TAG, "WAKEUP id:%d cfd:%d cnt:%u", ((mic_kws_t *)data)->id, type & KWS_ID_WWV_MASK, ++g_wakeup_cnt);
            snprintf(keywords, 128, "{\n\"text\": \"%s\", \n\"tag\": \"%s\", \n\"score\": \"%f\"\n}", 
                     ((mic_kws_t *)data)->word, ((mic_kws_t *)data)->word, (double)((mic_kws_t *)data)->score);
            // if (session_state == SESSION_STATE_START) {
            //     return;
            // }
#if 0
            /* 网络检测 */
            if (wifi_internet_is_connected() == 0) {
                LOGE(TAG, "mic_evt net connect failed");
                aui_mic_control(MIC_CTRL_STOP_PCM);
                app_aui_cloud_stop(1);
                if (wifi_is_pairing())
                    local_audio_play(LOCAL_AUDIO_NET_CFG_CONFIG);
                else
                    local_audio_play(LOCAL_AUDIO_NET_FAIL);
                return;
            }
#endif

            //if (type & KWS_ID_WWV_MASK) {
            //    session_state = SESSION_STATE_WWV;
            //    LOGD(TAG, "wwv process start");
            //} else {
                /* play wakeup voice only when wwv is not needed,
                   otherwise do it in the wwv result callback */
                //if ((type & KWS_ID_P2T_MASK) == 0) {
                    /* no wakeup voice when push to talk */
                    local_wakeup_audio_play(LOCAL_AUDIO_WAKEUP_HELLO);
                //}
            //}
            /* stop first */
            app_aui_cloud_stop(0);
            app_aui_cloud_stop_tts();

            send_byte = 0;
            ret = app_aui_cloud_start(0);
            if (ret != 0) {
                session_state = SESSION_STATE_IDLE;
                LOGE(TAG, "aui cloud pcm start err.");
                return;
            }

            aui_mic_control(MIC_CTRL_START_PCM);

            /* 开始交互 */
            session_state = SESSION_STATE_START;
            break;

        case MIC_EVENT_SESSION_STOP:
            LOGD(TAG, "MIC_EVENT_SESSION_STOP, Send_Bytes = %d bytes", send_byte);

            /* 交互结束 */
            if (session_state != SESSION_STATE_IDLE) {
                app_aui_cloud_stop(0);

                aui_mic_control(MIC_CTRL_STOP_PCM);
                session_state = SESSION_STATE_IDLE;
            }
            break;

        default:;
    }
}

int app_mic_init(void)
{
    int ret;

    mic_voice_register();

    utask_t *task_mic = utask_new("task_mic", 10 * 1024, 20, AOS_DEFAULT_APP_PRI);
    ret               = aui_mic_init(task_mic, mic_evt_cb);

    aui_mic_start();

    return ret;
}
