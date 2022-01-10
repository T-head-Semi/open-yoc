/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <aos/kv.h>

#include <voice.h>
#include <voice_hal.h>
#include <voice_server_dbus.h>
#include <voice_debug.h>

csi_kws_t g_kws;
voice_t  *g_voice; //TODO: optimize me

static csi_pcm_format_t format = {
    .channel_num = 3, //2mic+1ref
    .sample_rate = 16000,
    .sample_bits = 16,
    .period_time = 20
};

static void kws_event_cb(csi_kws_t *kws, csi_kws_event_t event, void *data, size_t size, void *cb_arg)
{
#if defined(CONFIG_VOICE_DBUS_SUPPORT) && CONFIG_VOICE_DBUS_SUPPORT
    voice_t *voice = (voice_t *)cb_arg;

    switch (event) {
        case KWS_EVENT_SESSION_BEGIN:
            voice_set_state(voice, VOICE_STATE_BUSY);
            voice_log(LOG_INFO, "Send KWS_EVENT_SESSION_BEGIN\n");
            voice_dbus_signal_session_begin(voice, data, size);
            break;

        case KWS_EVENT_SESSION_END:
            voice_set_state(voice, VOICE_STATE_IDLE);
            voice_log(LOG_INFO, "Send KWS_EVENT_SESSION_END\n");
            voice_dbus_signal_session_end(voice);
            break;

        case KWS_EVENT_VAD:
            voice_log(LOG_INFO, "Send KWS_EVENT_VAD\n");
            voice_dbus_signal_vad(voice);
            break;

        case KWS_EVENT_SHORTCUT_CMD:
            voice_log(LOG_INFO, "Send KWS_EVENT_SHORTCUT_CMD\n");
            voice_dbus_signal_shortcut_cmd(voice, data, size);
            break;

        case KWS_EVENT_PCM_ORIGINAL:
            break;

        case KWS_EVENT_PCM_ALG:
        case KWS_EVENT_PCM_KWS:
        case KWS_EVENT_AI_ALG:
            break;
    }
#endif
}



#include <k_api.h>

static int create_fe_task(const char *task_name, task_entry_t task_routine, void *arg, int prio);


static int init(void *priv)
{
    csi_kws_t *kws = (csi_kws_t *)priv;
    // aos_event_new(&asr_evt, 0);
	
    voice_log(LOG_INFO, "alg init\n");

	// alg_init(hdl->palg);
    csi_kws_open(kws, "d1rtos", kws_event_cb, kws->priv);
    
    return 0;
}

static int deinit(void *priv)
{
    csi_kws_t *kws = (csi_kws_t *)priv;
    // aos_event_new(&asr_evt, 0);
	
    voice_log(LOG_INFO, "alg deinit\n");

    csi_kws_close(kws);
    
    return 0;
}

static int set_param(void *priv, void *param)
{
    csi_kws_t *kws = (csi_kws_t *)priv;

    ai_alg_param_t *alg_param = (ai_alg_param_t *)param;

    csi_kws_alg_config(kws, alg_param, sizeof(ai_alg_param_t));

    csi_kws_pcm_config(kws, &format);

    return 0;
}


#if defined(CONFIG_VAD_TAG) && CONFIG_VAD_TAG
static mic_vad_buf_t *vad_out_data;
#else
static char vad_out_data[5120];
#endif

static int vad(void *priv, void *mic, void *ref, int ms, void **out, size_t *vad_len)
{
    aos_check_param(out && vad_len);

    static int vad_detected = 0;
    int vad_stat = VOICE_VADSTAT_SILENCE;

    csi_kws_t *kws = (csi_kws_t *)priv;

    *vad_len = 0;
    *out = NULL;

    /* VAD 状态处理 */
    int vadres = csi_kws_get_vad_state(kws);
    switch(vadres) {
        case -1:
            break;
        case 0:
            if (KWS_STATE_WAIT == csi_kws_get_state(kws)) {
                vad_stat = VOICE_VADSTAT_ASR;
                /* 进入下一个状态 */
                csi_kws_set_state(kws, KWS_STATE_ASR);
            }
            vad_detected = 1;
            break;
        case 1:
            vad_detected = 0;
            break;
        case 2:
            vad_detected = 0;
            break;
        default:
            ;
    }

    int vad_data_len;
    int vad_data_finished = 0;
    char *vad_data;
    short *frame_tags;

    int ret = csi_kws_get_tagged_vad_data(&vad_data, &vad_data_len, &frame_tags, &vad_data_finished);
    if (ret == 0) {
#if defined(CONFIG_VAD_TAG) && CONFIG_VAD_TAG
        static short last_tag = 0;    // remember last tag to smooth out 10 ms tag to 20 ms tag

        if (!vad_out_data) {
            vad_out_data = aos_zalloc_check(sizeof(mic_vad_buf_t) + 10 * 20 * 32);
        }

        vad_out_data->len = vad_data_len;

        for (int i = 0; i < vad_data_len / 10 / 32; i++) {

            if (i % 2) {
                vad_out_data->vad_tags[i / 2] = frame_tags[i] + frame_tags[i - 1] + last_tag >= 2 ? 1 : 0;
                last_tag = frame_tags[i];
            }
        }

        memcpy(vad_out_data->data, vad_data, vad_data_len);
        *vad_len = sizeof(mic_vad_buf_t) + vad_data_len;

        vad_stat = VOICE_VAD_PCM_DATA;
#else
        memcpy(vad_out_data, vad_data, vad_data_len);
        *vad_len = vad_data_len;
#endif

        // *out = vad_out_data;
    }
    if (1 == vad_data_finished) {
#ifdef DEBUG_LOG_MX_ENABLE
        voice_log(LOG_INFO, "vad data finished. call yv_vad_send(0)");
#endif
    }

    if (csi_kws_get_state(kws) == KWS_STATE_STOPPED) {
        vad_stat = VOICE_VADSTAT_ASR_FINI;
        /* 进入下一个状态 */
        csi_kws_set_state(kws, KWS_STATE_IDLE);
    }

    return vad_stat != VOICE_VADSTAT_SILENCE ? vad_stat : (vad_detected ? VOICE_VADSTAT_VOICE : VOICE_VADSTAT_SILENCE);
}

static int kws(void *priv, void *mic, void *ref, int ms, void **out)
{
    aos_check_param(mic);

    csi_kws_t *kws = (csi_kws_t *)priv;

    int kwid = csi_kws_write_pcm(kws, mic, ms * 16);

    /* 强制唤醒 */
    // if (hdl->p2t_cmd == AI_P2T_CMD_START) {
        
    //     kwid = KWS_ID_P2T_MASK | 0x0001;
    //     csi_kws_send_cmd(kws, ALG_CMD_ASR_MODE, );
	// 	// audio_set_asr_mode(kMitRotsModeVadKws);
    //     csi_kws_set_state(kws, KWS_STATE_BUSY);
    //     // audio_set_status(AU_STATUS_WAKE);
    //     hdl->p2t_cmd = AI_P2T_CMD_NONE;
    // } else if (hdl->p2t_cmd == AI_P2T_CMD_STOP) {
    //     csi_kws_set_state(kws,KWS_STATE_STOPPED);
    //     // audio_set_status(AU_STATUS_FINI);
    //     hdl->p2t_cmd = AI_P2T_CMD_NONE;
    // }

    /* 状态处理 */
    int ret = csi_kws_get_state(kws);
    switch (ret) {
    case KWS_STATE_IDLE:
        break;
    case KWS_STATE_BUSY:
        /* 进入下一个状态 */
        csi_kws_set_state(kws, KWS_STATE_WAIT);
        break;
    case KWS_STATE_WAIT:
        break;
    case KWS_STATE_ASR:
        break;
    case KWS_STATE_STOPPED:
        /* 结束处理 */
        /* has to revert the asr to default state after finishing dialog */
        // if (hdl->p2t_mode == VOICE_P2T_MODE_VAD) {
        //     hdl->p2t_mode = VOICE_P2T_MODE_STOP;
        //     csi_kws_send_cmd();
        //     // audio_set_asr_mode(kMitRotsModeKws);
        // }

        break;
    default:
        ;
    }

    if (kwid > 0) {
        voice_log(LOG_INFO, "kwid is %d\n", kwid);
    }

    /* out is the backflow data for aec */
    // *out = mic;

    return kwid & 1 ? kwid : 0;
}

// static int get_kws_data(void *priv, void **data, size_t *data_len)
// {
//     aos_check_param(data);

//     return csi_kws_get_kws_data((char **)data, (int *)data_len);
// }

// static int push2talk(void *priv, int mode)
// {
    // ai_hdl_t *hdl = (ai_hdl_t *)priv;

    // if (mode == VOICE_P2T_MODE_STOP) {
    //     /*p2t close*/
    // 	if (hdl->p2t_mode == VOICE_P2T_MODE_VAD || hdl->p2t_mode == VOICE_P2T_MODE_FORCE) {
    // 		hdl->p2t_cmd = AI_P2T_CMD_STOP;
    // 	}
    // } else if (mode == VOICE_P2T_MODE_VAD || mode == VOICE_P2T_MODE_FORCE) {
    //     hdl->p2t_cmd = AI_P2T_CMD_START;
    // 	hdl->p2t_mode = mode;
    // }

//     return 0;
// }

// static int mute(void *priv, int mute_flag)
// {
//     csi_kws_set_wku_voice_state(mute_flag, 20, 200);

//     return 0;
// }

static int start_alg_tsk(void *priv, void (*fn)(void *), void *arg)
{
    return create_fe_task("alg", fn, arg, AOS_DEFAULT_APP_PRI - 4);
}

static voice_ops_t voice_ops = {
    .cache_cts = 50,
    .init   = init,
    .deinit = deinit,
    .start_alg_tsk = start_alg_tsk,
    .set_param = set_param,
    // .mute = mute,
    .kws    = kws,
    .vad    = vad,
    // .p2t    = push2talk,
    // .get_kws_data    = get_kws_data,
};


#define FE_TASK_STACK_SIZE (50 *1024 / 4)
static cpu_stack_t g_fe_task_stack[FE_TASK_STACK_SIZE];// __attribute__((section(".fe_stack")));
static ktask_t     g_fe_task_handle;

static int create_fe_task(const char *task_name, task_entry_t task_routine, void *arg, int prio)
{
    return krhino_task_create(&g_fe_task_handle, task_name, arg,
                        prio, 0u, g_fe_task_stack,
                        FE_TASK_STACK_SIZE, task_routine, 1u);
}


voice_ops_t *voice_ai_init(voice_t *v)
{
    g_voice = v;
    voice_log(LOG_INFO, "alg voice_ai_init\n");

    return &voice_ops;
}