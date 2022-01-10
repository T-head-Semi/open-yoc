/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <aos/aos.h>

#include <voice_hal.h>


#include "mit_rtos.h"
#include "debug_mode.h"

#define TAG "AUD"

#define MAX_KWS_DATA_LEN    0xC000          // 48 K
#define MAX_VAD_DATA_LEN    80 * 2 * 16     // 80 ms pcm, this has to be longer than 20 ms processing slice
#define MAX_VAD_TAG_LEN     (MAX_VAD_DATA_LEN / 10 / 32)        // vad tag for each 10 ms pcm


static char g_kws_data[MAX_KWS_DATA_LEN] __attribute__((aligned(16)));
static char g_vad_data[MAX_VAD_DATA_LEN] __attribute__((aligned(16)));
static short g_vad_data_tag[MAX_VAD_TAG_LEN];
static int g_vad_data_len;
static int g_vad_data_should_ep=0;

mit_rtos_listener g_nuithings_listener;
MitRtosConfig g_mit_rtos_config;	   //mit_rtos 初始化配置参数
static int wakeup_audio_playing_before, wakeup_audio_playing;
static int wakeup_audio_hw_delay, wakeup_audio_buf_delay;

/* parameters of debug mode*/
unsigned int kws_spot_count_debug = 0;
extern int g_silence_log_level;
extern int g_log_enable;

#define WAV_DATA_LENGTH 320
#define ASR_DATA_LENGTH_DIV 4
short g_nui_buffer_fread[WAV_DATA_LENGTH * 3];                 //读取离线文件中的数据，20ms
short g_nui_buffer_out[WAV_DATA_LENGTH * 2];                   //算法处理结果，单通道/2ch。


static int mit_rtos_data_get_main(void * user_data, char * buffer, int len){
	int ret = len;

	return ret;
}

static int mit_rtos_event_main(void * user_data, mit_rtos_event_t event, int dialog_finish){
	//sl_printf( "mit event( %d)\n", event );
	return 0;
}

static int mit_rtos_data_out(void * user_data,  mit_rtos_event_t event, char * buffer, int len){
	return 0;
}

int csi_kws_open(csi_kws_t *kws, const char *name, csi_kws_event_cb_t cb, void *cb_arg)
{
    kws->name   = (char *)name;
    kws->cb     = cb;
    kws->cb_arg = cb_arg;
    kws->state  = KWS_STATE_STOPPED;

    return 0;
}

int csi_kws_close(csi_kws_t *kws)
{
    return 0;
}

int csi_kws_alg_config(csi_kws_t *kws, void *config, size_t size)
{
    int ret_init = 0;

    kws->config = config;

    ai_alg_param_t *param = (ai_alg_param_t *)config;


    LOGI(TAG, "mit version baolabaola_20210611_114000 bssopt\n");

    g_nuithings_listener.on_event_callback = mit_rtos_event_main;
    g_nuithings_listener.need_data_callback = mit_rtos_data_get_main;
    g_nuithings_listener.put_data_callback = mit_rtos_data_out;
    g_nuithings_listener.user_data = NULL;

    g_mit_rtos_config.listener = &g_nuithings_listener;
    g_mit_rtos_config.alg_type = kMitRtosAlgBSSOpt;//kMitRtosAlgPMWFPG;//kMitRtosAlgBSS;//kMitRtosAlg2MicFloat;
    g_mit_rtos_config.task_enable = 0;
    g_mit_rtos_config.kws_alwayson_enable = 1;
    g_mit_rtos_config.fe_enable = 1;
    g_mit_rtos_config.kws_enable = 1;
    g_mit_rtos_config.vad_enable = param->vadswitch ? 1 : 0;
    g_mit_rtos_config.need_data_after_vad = 1;
    g_mit_rtos_config.wwv_enable = 0;//param->wwv_enable;//enable wwv or not.
    g_mit_rtos_config.vad_endpoint_ignore_enable = param->vadswitch == 2 ? 1 : 0;
    g_mit_rtos_config.voice_filter_type = param->vadfilter;
    g_mit_rtos_config.log_in_asr_enable = 0;
    g_mit_rtos_config.is_interleave = 1;

    g_mit_rtos_config.vad_kws_strategy = param->vadkws_strategy;
    memcpy(&g_mit_rtos_config.kws_thres, &param->vadthresh_kws, sizeof(MitRtosVadThres));
    memcpy(&g_mit_rtos_config.asr_thres, &param->vadthresh_asr, sizeof(MitRtosVadThres));

    g_mit_rtos_config.gain_tune_db = 0; //gain: 0dB
    g_mit_rtos_config.fe_tune.agc_enable = 1; //agc enable
    g_mit_rtos_config.fe_tune.agc_power  = 8;
    g_mit_rtos_config.fe_tune.agc_level  = 4;
    // g_mit_rtos_config.add_kwsdetect_enable = 0;
    g_mit_rtos_config.enable_individual_vad4asr = 1;
    g_mit_rtos_config.vad4asr_enable = 1;
    g_mit_rtos_config.fe_tune.mic_num = 2;
    g_mit_rtos_config.fe_tune.ref_num = 1;
    g_mit_rtos_config.fe_tune.out_num = 2;


    g_mit_rtos_config.vad_endpoint_delay = param->sentence_time_ms;
    g_mit_rtos_config.vad_silencetimeout = param->noack_time_ms;
    g_mit_rtos_config.vad_voicetimeout = param->max_time_ms;


    g_silence_log_level = 4;
    ret_init = mit_rtos_init(&g_mit_rtos_config);
    g_log_enable=0;

    mit_rtos_start();

    //disabled debug info
    // mit_rtos_debug_set_mode(0x00);

    kws->state = KWS_STATE_IDLE;

    return ret_init;
}

int csi_kws_pcm_config(csi_kws_t *kws, csi_pcm_format_t *format)
{
    kws->format = format;

    return 0;
}

static int g_mitalg_mode = 0;

int csi_kws_start(csi_kws_t *kws)
{
    g_mitalg_mode = 1;

    return 0;
    // return mit_rtos_start();
}

int csi_kws_stop(csi_kws_t *kws)
{
    g_mitalg_mode = 0;

    return 0;
    // return mit_rtos_stop();
}

csi_kws_state_t csi_kws_get_state(csi_kws_t *kws)
{
    return kws->state;
}

void csi_kws_set_state(csi_kws_t *kws, csi_kws_state_t state)
{
    kws->state = state;
}


static mit_rtos_voice_data g_voice_data;
static MitRtosAudioResult audio_result_main;
static int should_check_asr_data_finish = 0;

static void mit_status_update(csi_kws_t *kws)
{
    int ret = -1;
    int asr_data_is_null = 0;//the asr_data is null of this get

    int au_status = csi_kws_get_state(kws);
    if (au_status == KWS_STATE_IDLE || au_status == KWS_STATE_STOPPED) {
        return;
    }

    g_voice_data.mode        = kMitRtosVoiceModeAsr;
    g_voice_data.available   = kVoiceDataInvalid;
    g_voice_data.data        = g_vad_data;
    g_voice_data.data_length = MAX_VAD_DATA_LEN;
    g_voice_data.data_framestate             = g_vad_data_tag;
    g_voice_data.data_framestate_length_byte = sizeof(g_vad_data_tag);

    ret = mit_rtos_get_voice(&g_voice_data);
    if (0 == ret) {
        g_vad_data_len = g_voice_data.data_length;
        if (g_vad_data_len <= 0) {
            asr_data_is_null = 1;
            if (1 == should_check_asr_data_finish) {
                should_check_asr_data_finish = 0;
                asr_data_is_null = 0;

                g_vad_data_should_ep = 1;
                if (au_status != KWS_STATE_BUSY) {
                    csi_kws_set_state(kws, KWS_STATE_STOPPED);
                }
            }
        } else {
            if (g_voice_data.data_framestate_length_byte / 2 != g_vad_data_len / 10 / 32) {
                LOGE(TAG, "frame tag len error(%d VS %d)",g_voice_data.data_framestate_length_byte / 2, g_vad_data_len / 10 / 32);
            }
        }
    } else {
        LOGE(TAG, "ASR data ret %d",ret);
    }

    if (audio_result_main.vad_state >= 0) {
        switch (audio_result_main.vad_state) {
        case kMitRotsVadResultStartPoint:
            //do nothing
            break;
        case kMitRotsVadResultEndPoint:
            //nls end
            if (1 == asr_data_is_null) {
                // endpoint and asr data is null. so stop this dialog
                g_vad_data_should_ep = 1;
                if (au_status != KWS_STATE_BUSY) {
                    csi_kws_set_state(kws, KWS_STATE_STOPPED);
                }
            } else {
                //start the check of data_end
                should_check_asr_data_finish = 1;
            }
            break;
        case kMitRotsVadResultTimeout:
            //nls cancel with err
            if (au_status != KWS_STATE_BUSY) {
                csi_kws_set_state(kws, KWS_STATE_STOPPED);
            }
            break;
        default:
            LOGE(TAG, "user vad_state(%d) error", audio_result_main.vad_state);
            break;
        }
    }
}

int csi_kws_write_pcm(csi_kws_t *kws, void *pcm_in_data, size_t pcm_in_len)
{
    int ret = -1;

    int fe_buffer_out_length = 0;

    int input_data_len = pcm_in_len * sizeof(short) * kws->format->channel_num;
    // int output_data_len;

    if (g_mitalg_mode == 0) {
        return 0;
    }

    // if (g_mit_rtos_config.alg_type == kMitRtosAlgBSSOpt) {
    //     output_data_len = pcm_in_len * sizeof(short) * 2;//sample_count * sizeof(short) * 2;
    // } else {
    //     output_data_len = pcm_in_len * sizeof(short);//sample_count * sizeof(short);
    // }

    fe_buffer_out_length = sizeof(g_nui_buffer_out);

    if (wakeup_audio_playing_before != wakeup_audio_playing) {
        mit_rtos_set_playstate(wakeup_audio_playing ? kMitRtosPlayOn : kMitRtosPlayOff, wakeup_audio_hw_delay, wakeup_audio_buf_delay);
        LOGD(TAG, "audio stat %d", wakeup_audio_playing);
        wakeup_audio_playing_before = wakeup_audio_playing;
    }

    ret = mit_rtos_update_audio((char *)pcm_in_data, input_data_len, (char *)g_nui_buffer_out, &fe_buffer_out_length, &audio_result_main);

    if (ret > 10 || ret & 0x01) {
        csi_kws_set_state(kws, KWS_STATE_BUSY);

        kws_spot_count_debug ++;
        LOGI(TAG, "kws is %s, kws_spot_count=%u", audio_result_main.data_kws[0].word, kws_spot_count_debug);
        if (audio_result_main.data_kws_count > 0 && audio_result_main.data_kws[0].do_wwv) {
            ret |= KWS_ID_WWV_MASK;
            LOGD(TAG, "do wwv");
        }
    }

    mit_status_update(kws);
    /* 更新状态 */

    return ret;
}


void csi_kws_set_wku_voice_state(int stat, int buf_delay, int hw_delay)
{
    wakeup_audio_playing   = stat;
    wakeup_audio_hw_delay  = hw_delay;
    wakeup_audio_buf_delay = buf_delay;
}


csi_vad_state_t csi_kws_get_vad_state(csi_kws_t *kws)
{
    /* 适配接口定义与该枚举一致 */
    return audio_result_main.vad_state;
}


int csi_kws_get_tagged_vad_data(char **vad_data, int *data_len, short **frame_tag, int *finished)
{
    *data_len = g_vad_data_len;
    g_vad_data_len = 0;

    *vad_data  = g_vad_data;
    *frame_tag = g_vad_data_tag;

    *finished = g_vad_data_should_ep;
    if (1 == g_vad_data_should_ep) {
        g_vad_data_should_ep = 0;
        LOGD(TAG, "audio_get_vad_data all finished");
    }

    return *data_len == 0 ? -1 : 0;
}


int csi_kws_get_kws_data(char **kws_data, int *data_len)
{
    g_voice_data.mode        = kMitRtosVoiceModeKws;
    g_voice_data.available   = kVoiceDataInvalid;
    g_voice_data.data        = g_kws_data;
    g_voice_data.data_end    = 0;
    g_voice_data.data_length = MAX_KWS_DATA_LEN;

    int ret = mit_rtos_get_voice(&g_voice_data);
    if (ret == 0 && g_voice_data.data_length > 0) {
        *data_len = g_voice_data.data_length;
        *kws_data = g_kws_data;
    }

    return *data_len == 0 ? -1 : 0;
}


int csi_kws_send_cmd(csi_kws_t *kws, int cmd, void *data, size_t size, int wait)
{
    int mode = *(int *)data;

    if (cmd == ALG_CMD_DEBUG_MODE) {
        mit_rtos_debug_set_mode(mode);
    } else if (cmd == ALG_CMD_ASR_MODE) {
        LOGD(TAG, "set asr mode %d", mode);
        mit_rtos_stop();
        mit_rtos_start_mode(mode);
    }

    return 0;
}
