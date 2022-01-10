/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <aos/kv.h>
#include <yoc/mic.h>
#include <ulog/ulog.h>
// #include <avutil/av_errno.h>
#include <uservice/uservice.h>
// #include <board.h>
// #include "app_main.h"

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
    // int        ret = 0;
    int        type   = 0;

    // if (g_fct_mode) {
    //     return;
    // }

    switch (evt_id) {
        case MIC_EVENT_PCM_DATA:
        case MIC_EVENT_VAD_DATA:
            if (session_state == SESSION_STATE_IDLE)
                break;

            mic_pcm_vad_data_t *pcm_vad_data = data;
            // void *pcm_data = data;
            size_t pcm_len = (size_t)size;

            if (evt_id == MIC_EVENT_VAD_DATA) {
                // pcm_data = pcm_vad_data->data;
                pcm_len = pcm_vad_data->len; 
            }

            // if (vad_dump_len + pcm_len < 32 * 10000) {
            //     memcpy(vad_data_dump + vad_dump_len, pcm_data, pcm_len);
            //     vad_dump_len += pcm_len;
            // }

            // printf("pcm %d %d\n", pcm_len, pcm_vad_data->vad_tag);

            /* 麦克风数据，推到云端 */
            // ret = app_aui_cloud_push_audio(pcm_data, pcm_len);
            // if (ret < 0) {
            //     /* 数据推送错误 */
            //     session_state = SESSION_STATE_IDLE;
            //     LOGE(TAG, "cloud push pcm finish");
            //     aui_mic_control(MIC_CTRL_STOP_PCM);
            //     ret = app_aui_cloud_stop(1);
            //     if (wifi_internet_is_connected() == 0) {
            //         LOGE(TAG, "mic evt ntp not synced");
            //         local_audio_play(LOCAL_AUDIO_NET_FAIL);
            //     } else {
            //         if (ret < 0) {
            //             local_audio_play(LOCAL_AUDIO_SORRY2);
            //         }
            //     }
            // }
			send_byte += pcm_len;
            break;
        case MIC_EVENT_VAD:
            // if (SMTAUDIO_STATE_MUTE != smtaudio_get_state()) {
            //     app_lpm_update();
            // }
            break;
        case MIC_EVENT_SESSION_START:
            // app_lpm_update();
            // app_speaker_mute(0);
            // if (SMTAUDIO_STATE_MUTE == smtaudio_get_state()) {
            //     LOGD(TAG, "Device is mute\n");
            //     return;
            // }
            type = ((mic_kws_t *)data)->id;
            LOGD(TAG, "WAKEUP id:%d cfd:%d cnt:%u", type & KWS_ID_MASK, type & KWS_ID_WWV_MASK, ++g_wakeup_cnt);

            /* 网络检测 */
            // if (wifi_internet_is_connected() == 0) {
            //     LOGE(TAG, "mic_evt net connect failed");
            //     aui_mic_control(MIC_CTRL_STOP_PCM);
            //     app_aui_cloud_stop(1);
            //     if (wifi_is_pairing())
            //         local_audio_play(LOCAL_AUDIO_NET_CFG_CONFIG);
            //     else
            //         local_audio_play(LOCAL_AUDIO_NET_FAIL);
            //     return;
            // }

            /* 开始交互 */
            session_state = SESSION_STATE_START;
            // callback(kws, KWS_EVENT_SESSION_BEGIN, keywords, sizeof(keywords), kws->cb_arg);

            if (type & KWS_ID_WWV_MASK) {
                session_state = SESSION_STATE_WWV;
                LOGD(TAG, "wwv process start");
            } else {
                /* play wakeup voice only when wwv is not needed,
                   otherwise do it in the wwv result callback */
                if ((type & KWS_ID_P2T_MASK) == 0) {
                    /* no wakeup voice when push to talk */
                    // local_wakeup_audio_play(LOCAL_AUDIO_WAKEUP_HELLO);
                }

            }
			send_byte = 0;
            // ret = app_aui_cloud_start(0);
            // if (ret != 0) {
            //     session_state = SESSION_STATE_IDLE;
            //     return;
            // }

            aui_mic_control(MIC_CTRL_START_PCM);

            break;
        case MIC_EVENT_SESSION_STOP:
            LOGD(TAG, "MIC_EVENT_SESSION_STOP, Send_Bytes = %d bytes", send_byte);
                        // callback(kws, KWS_EVENT_SESSION_BEGIN, keywords, sizeof(keywords), kws->cb_arg);

            // app_lpm_update();
            /* 交互结束 */
            if (session_state != SESSION_STATE_IDLE) {
                // ret = 0;

                // if (ret == 0) {
                //     app_aui_cloud_stop(0);
                // } else {
                //     /* wwv rejected or timeout */
                //     app_aui_cloud_stop(1);
                // }

                aui_mic_control(MIC_CTRL_STOP_PCM);
                session_state = SESSION_STATE_IDLE;
            }

            break;
        case MIC_EVENT_KWS_DATA:
            // LOGD(TAG, "get kws data %d", size);
            if (session_state == SESSION_STATE_WWV) {
                LOGD(TAG, "push wwv data %p %d", data, size);
                // app_aui_push_wwv_data(data, size);
            }

            break;
        default:;
    }
}

// extern void pcm_rec_mode_set(int mode);
int app_mic_init(void)
{
    int ret;
    int record_mode = 0;
    static voice_adpator_param_t voice_param;

    mic_param_t param;
    mic_vad_param_t vad_param;
    mic_aec_param_t aec_param;

    memset(&param, 0, sizeof(param));
    memset(&vad_param, 0, sizeof(vad_param));
    memset(&aec_param, 0, sizeof(aec_param));

    //0:normal, 1:rec mode, 2:disable
    aos_kv_getint("rec_mode", &record_mode);
    if (record_mode == 1) {
        LOGI(TAG, "record mode, no local algorithm");
    }
    // pcm_rec_mode_set(record_mode);

    mic_mind_register();

    utask_t *task_mic = utask_new("task_mic", 8 * 1024, 20, AOS_DEFAULT_APP_PRI);
    ret               = aui_mic_init(task_mic, mic_evt_cb);

    param.channels          = 3;     /* 算法声音道数 */
    param.sample_bits       = 16;    /* 比特数 */
    param.rate              = 16000; /* 采样率 */
    param.sentence_time_ms  = 600;    /* VAD尾点闭麦参数 */
    param.noack_time_ms     = 8000;
    param.max_time_ms       = 10000;
    aec_param.nsmode        = 0;      /* 无非线性处理 */
    aec_param.aecmode       = 0;      /* 无非线性处理 */
    vad_param.vadmode          = 0;  /* 使能VAD */
    vad_param.vadswitch        = 1;
    vad_param.vadfilter        = 2;
    vad_param.vadkws_strategy  = 0;
    vad_param.vadthresh_kws.vad_thresh_sp = -0.2;
    vad_param.vadthresh_kws.vad_thresh_ep = 0.1;
    vad_param.vadthresh_asr.vad_thresh_sp = -0.2;
    vad_param.vadthresh_asr.vad_thresh_ep = 0.1;
    // param.wwv_enable        = 0; /* 使能唤醒二次确认 */

    voice_param.pcm         = "pcmC0";
    voice_param.cts_ms      = 20;
    voice_param.ipc_mode    = 0;
    param.priv              = &voice_param;

    aui_mic_set_vad_param(&vad_param);
    aui_mic_set_aec_param(&aec_param);

    aui_mic_set_param(&param);

    aui_mic_start();

    return ret;
}
