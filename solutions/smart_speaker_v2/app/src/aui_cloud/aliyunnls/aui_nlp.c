/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <sys/time.h>

#include <aos/kv.h>
#include <devices/wifi.h>
#include <avutil/named_straightfifo.h>
#include <cJSON.h>
#include <yoc/aui_cloud.h>

#include "player/app_player.h"
#include "aui_nlp.h"

#define TAG "auinlp"

/* wwv event */
#define EVENT_WWV_CONFIRMED             (1 << 0)
#define EVENT_WWV_REJECTED              (1 << 1)

/* ai engine */
static aui_t *     g_aui_handler;
static aos_event_t event_wwv_result;
static int         g_initialized = 0;

int json_string_eq(cJSON *js, const char *str)
{
    if (cJSON_IsString(js)) {
        if (strcmp(js->valuestring, str) == 0) {
            return 1;
        }
    }
    return 0;
}

static void asr_handle(void *data, int len, void *priv)
{
    aui_kws_t *kws_info = (aui_kws_t *)data;

    if (kws_info->asr_type) {
        LOGD(TAG, "%s wwv_state %d\n", __func__, ((int32_t *)kws_info->data)[0]);
        if (((int32_t *)kws_info->data)[0] == EVENT_WWV_REJECTED) {
            aos_event_set(&event_wwv_result, EVENT_WWV_REJECTED, AOS_EVENT_OR);
        } else if (((int32_t *)kws_info->data)[0] == EVENT_WWV_CONFIRMED) {
            aos_event_set(&event_wwv_result, EVENT_WWV_CONFIRMED, AOS_EVENT_OR);
        }
    } else {
        LOGD(TAG, "%s %s\n", __func__, kws_info->data);
        cJSON *js = cJSON_Parse(kws_info->data);

        int ret = aui_nlp_proc_mit(js, kws_info->data);
        if (ret < 0) {
            local_audio_play(LOCAL_AUDIO_SORRY2); /* 不懂 */
        }

        cJSON_Delete(js);
    }
}


static void tts_handle(void *data, int data_len, void *priv)
{
    smtaudio_stop(MEDIA_SYSTEM);
    smtaudio_start(MEDIA_SYSTEM, data, 0, 1);

    return;
}

/* ai engine init */
int aui_nlp_init(void)
{
    aui_config_t cfg;
    cfg.per             = "aixia";
    cfg.vol             = 100;      /* 音量 0~100 */
    cfg.spd             = 0;        /* -500 ~ 500*/
    cfg.pit             = 0;        /* 音调*/
    cfg.asr_fmt         = 2;        /* 编码格式，1：PCM 2：MP3 */
    cfg.tts_fmt         = 2;        /* 编码格式，1：PCM 2：MP3 */
    cfg.srate           = 16000;    /* 采样率，16000 */
    cfg.tts_cache_path  = NULL;     /* TTS内部缓存路径，NULL：关闭缓存功能 */
    cfg.cloud_vad       = 1;        /* 云端VAD功能使能， 0：关闭；1：打开 */

    g_aui_handler = aui_cloud_init(&cfg);

    aui_asr_register(g_aui_handler, asr_handle, g_aui_handler);
    aui_tts_register(g_aui_handler, tts_handle, g_aui_handler);
    aos_event_new(&event_wwv_result, 0);
    g_initialized = 1;
    
    return 0;
}

int app_aui_get_wwv_result(unsigned int timeout)
{
    unsigned int flags = 0;

    aos_event_get(&event_wwv_result, EVENT_WWV_CONFIRMED | EVENT_WWV_REJECTED, AOS_EVENT_OR_CLEAR, &flags, timeout);
    if (flags & EVENT_WWV_CONFIRMED) {
        return 0;
    }

    return -1;
}

int app_aui_cloud_push_audio(void *data, size_t size)
{
    if (g_initialized != 1) {
        return 0;
    }
    return aui_cloud_push_audio(g_aui_handler, data, size);
}

int app_aui_cloud_stop(int force_stop)
{
    if (g_initialized != 1) {
        return 0;
    }
    if (force_stop) {
        return aui_cloud_stop(g_aui_handler);
    }

    return aui_cloud_stop_audio(g_aui_handler);
}

int app_aui_cloud_start(int do_wwv)
{
    if (g_initialized != 1) {
        return 0;
    }
    aos_event_set(&event_wwv_result, 0, AOS_EVENT_AND);
    return aui_cloud_start_audio(g_aui_handler, do_wwv);
}

int app_aui_cloud_start_tts()
{
    return aui_cloud_start_tts(g_aui_handler);
}

int app_aui_push_wwv_data(void *data, size_t len)
{
    if (g_initialized != 1) {
        return 0;
    }
    aui_cloud_start_audio(g_aui_handler, 1);
    aui_cloud_push_audio(g_aui_handler, data, len);
    return 0;
}

int app_aui_cloud_stop_tts()
{
    return aui_cloud_stop_tts(g_aui_handler);
}

int app_aui_cloud_tts_run(const char *text, int wait_last)
{
    return aui_cloud_req_tts(g_aui_handler, text);
}

int app_aui_cloud_push_text(char *text)
{
    return aui_cloud_push_text(g_aui_handler, text);
}
