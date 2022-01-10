#include <stdio.h>
#include <aos/kernel.h>
#include <stdlib.h>
#include <aos/cli.h>
#include "ulog/ulog.h"
#include "tmall.h"

#include "mit_rtos.h"

#define TAG "mit"

static aos_task_t task_handle;
static int g_mit_running;

typedef enum {
    kMitRtosUserStateKws,
    kMitRtosUserStateAsr,
} MitRtosUserState;

#define WAV_DATA_LENGTH 320
#define ASR_DATA_LENGTH_DIV 4
short g_nui_buffer_fread[WAV_DATA_LENGTH * 3];                 //读取离线文件中的数据，20ms
short g_nui_buffer_out[WAV_DATA_LENGTH * 2];                   //算法处理结果，单通道/2ch。
short g_nui_buffer_asr[WAV_DATA_LENGTH * ASR_DATA_LENGTH_DIV]; //获取语音的缓存，长度随意，最好是20ms的整数倍

mit_rtos_listener g_nuithings_listener;               //定义回调参数
MitRtosConfig g_mit_rtos_config;                      //定义初始化参数
MitRtosAudioResult audio_result_main;                 //定义获取SDK消息事件变量
mit_rtos_voice_data g_voice_data;                     //定义获取数据的变量
MitRtosUserState g_user_state = kMitRtosUserStateKws; //定义app当前状态。唤醒/识别
int asr_data_available = 0;
int nls_should_stop = 1;
int nls_stop_with_cancel = 0;
extern int g_silence_log_level;
extern int g_log_enable;

int mit_rtos_data_get_nui_things(void *user_data, char *buffer, int len)
{
    int ret = len;
    return ret;
}
int mit_rtos_event_nui_things(void *user_data, mit_rtos_event_t event, int dialog_finish)
{
    return 0;
}
int mit_rtos_data_out_nui_things(void *user_data, mit_rtos_event_t event, char *buffer, int len)
{
    return 0;
}

int mit_init(void)
{
    int ret = 0;

    //app启动，执行初始化等操作。
    g_nuithings_listener.on_event_callback = mit_rtos_event_nui_things;
    g_nuithings_listener.need_data_callback = mit_rtos_data_get_nui_things;
    g_nuithings_listener.put_data_callback = mit_rtos_data_out_nui_things;
    g_nuithings_listener.user_data = NULL;

    g_mit_rtos_config.listener = &g_nuithings_listener;
    g_mit_rtos_config.alg_type = kMitRtosAlgBSSOpt;//kMitRtosAlgPMWFPG;//kMitRtosAlgBSS;//kMitRtosAlg2MicFloat;
    g_mit_rtos_config.task_enable = 0;
    g_mit_rtos_config.kws_alwayson_enable = 1;
    g_mit_rtos_config.fe_enable = 1;
    g_mit_rtos_config.kws_enable = 1;
    g_mit_rtos_config.vad_enable = 1;
    g_mit_rtos_config.need_data_after_vad = 1;
    g_mit_rtos_config.wwv_enable = 0;//enable wwv or not.
    g_mit_rtos_config.vad_endpoint_ignore_enable = 0;
    g_mit_rtos_config.voice_filter_type = 0;//vad_filter;
    //g_mit_rtos_config.log_in_asr_enable = 1;
    g_mit_rtos_config.is_interleave = 1;

    g_mit_rtos_config.vad_endpoint_delay = 600;
    g_mit_rtos_config.vad_silencetimeout = 8000;
    g_mit_rtos_config.vad_voicetimeout = 10000;

    g_mit_rtos_config.vad_kws_strategy = 0;
    g_mit_rtos_config.kws_thres.vad_speech_noise_thres_sp = -0.2;
    g_mit_rtos_config.kws_thres.vad_speech_noise_thres_ep = -0.1;

    g_mit_rtos_config.asr_thres.vad_speech_noise_thres_sp = -0.2;
    g_mit_rtos_config.asr_thres.vad_speech_noise_thres_ep = -0.1;

    g_mit_rtos_config.enable_individual_vad4asr  = 1;//enable process of individula_vad for asr function.
    g_mit_rtos_config.vad4asr_enable             = 1;         //是否使能ASR专用VAD子模块
    g_mit_rtos_config.fe_tune.mic_num            = 2;
    g_mit_rtos_config.fe_tune.ref_num            = 1;
    g_mit_rtos_config.fe_tune.out_num            = 2;

    g_mit_rtos_config.gain_tune_db = 0.0f; //gain: 0dB
    g_mit_rtos_config.fe_tune.agc_enable = 1; //agc enable
    //g_mit_rtos_config.fe_tune.agc_gain   = 4.0f;
    g_mit_rtos_config.fe_tune.agc_power  = 8;
    g_mit_rtos_config.fe_tune.agc_level  = 4;
    /*g_mit_rtos_config.wwv_enable = 2;
    if (2==g_mit_rtos_config.wwv_enable) {
      g_mit_rtos_config.local_threshold = 20;
      g_mit_rtos_config.wwv_threshold = 10;
    }*/
    // g_mit_rtos_config.enable_init_more_log = 2;

    g_silence_log_level = 4;
    ret = mit_rtos_init(&g_mit_rtos_config);
    g_log_enable=0;
    printf("call mit_rtos_init() return %d\n", ret);
    ret = mit_rtos_start();
    printf("call mit_rtos_start() return %d\n", ret);
    // mit_rtos_debug_set_mode(0x00);

    return 0;
}

int mit_update(void *data, int len)
{
    //app进入循环，不断读取录音数据进行处理，并检测消息事件

    int length_out = sizeof(g_nui_buffer_out);
    int ret = mit_rtos_update_audio(data, len,
                                    (char *)g_nui_buffer_out, &length_out, &audio_result_main);
    if (ret<=10&&ret>0 && ret&0x01) {
        /* if ret>10 表示是命令词唤醒消息
           else
             ret&0x01 表示唤醒得到唤醒成功消息
             ret&0x02 表示唤醒得到最佳尾点位置信息
             ret&0x03 表示唤醒同时得到唤醒成功消息和最佳尾点位置信息
        */

        if (ret & 1) {
            printf("wakeup success\r\n");
        }
    } else {
        if (ret < 0) {
            LOGE(TAG, "error: mit_rtos_update_audio ret %d", ret);
        }
    }

    return 0;
}

void test_mit(void *priv)
{
    unsigned int rate = 16000;
    int channel = 3;
    int format = 16;

    mit_init();
    int ms = 20;
    int len = ms * (rate / 1000) * format * channel / 8;

    char *data = output_pcm;

    int offset = 0;
    while (g_mit_running) {
        data = output_pcm + offset;
        offset += len;
        if ((offset + len) > output_pcm_len) {
            offset = 0;
            aos_msleep(5000);
        }
        mit_update(data, len);
        aos_msleep(20);
    }
    printf("mit alog end");
}

static void cmd_mit_func(int argc, char **argv)
{
    int item_count = argc;

    if (item_count >= 2) {
        if (strcmp(argv[1], "0") == 0) {
            if (!g_mit_running) {
                if (0 != aos_task_new_ext(&task_handle, "test_mit", test_mit, NULL, 50 * 1024, AOS_DEFAULT_APP_PRI + 3)) {
                    LOGE(TAG, "Create mit task failed.");
                } else {
                    g_mit_running = 1;
                }
            }
        } else if (strcmp(argv[1], "x") == 0) {
            g_mit_running = 0;
        }
    }
}

ALIOS_CLI_CMD_REGISTER(cmd_mit_func, mit, mit cmd);
