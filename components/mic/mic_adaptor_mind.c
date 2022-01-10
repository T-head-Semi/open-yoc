/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdlib.h>
#include <aos/aos.h>
#include <yoc/mic.h>
#include <yoc/mic_port.h>
#include <voice.h>
// #include "mic_internal.h"

static const char *TAG = "mic_voice";

#define MIN(x, y) ((x) > (y) ? (y) : (x))

#define FRAME_SIZE ((16000 / 1000) * (16 / 8) * 20) /* 640 */
typedef struct ai_param {
    int   sample_bits;      /* 采样精度 默认16bit*/
    int   channels;         /* 预留  */
    int   rate;             /* 采样率 默认16K*/
    int   nsmode;           /* 去噪等级参数  0~3 非线性处理等级逐步加强，其他值无非线性处理 */
    int   aecmode;          /* 回音消除等级  0~3 非线性处理等级逐步加强，其他值无非线性处理 */
    int   vadmode;          /* VAD等级 0~3 等级逐步加强 */
    int   sentence_time_ms; /* 有语音断句时间 */
    int   noack_time_ms;    /* 无语音超时时间 */
    int   max_time_ms;      /* 唤醒后总超时时间 */
    void *ext_param1;       /* 预留 */
    void *ext_param2;       /* 预留 */
    int   vadswitch;        /* 0 关闭VAD，1 打开, 2 打开起点关闭尾点 */
    int   vadfilter;        /* VAD过滤器类型， 0 关闭过滤器， 1~3 不同过滤器类型 */
    int   vadkws_strategy;  /* VAD KWS策略 */
    vad_thresh_t vadthresh_kws;    /* KWS VAD阈值 */
    vad_thresh_t vadthresh_asr;    /* ASR VAD阈值 */ 
    int   wwv_enable;        /* 使能算法侧唤醒二次确认功能 */
} ai_alg_param_t;

static ai_alg_param_t ai_alg_param __attribute__ ((aligned(16)));

typedef void (*aui_mic_debug_hook)(mic_event_id_t evt_id, void *data, int len);
typedef struct {
    char    *data;
    int     capacity;
    int     len;
    int     offset;
} data_buffer_t;

static mic_pcm_vad_data_t *mic_vad_data;
static mic_event_t mic_evt;
// static int g_mic_rec_start;
// static void _mic_rec_copy_data(int index, uint8_t *data, int size);
static int mic_adaptor_debug_control(mic_t *mic, int flag);
static aui_mic_debug_hook g_pcm_debug_hook, g_vad_debug_hook;
static data_buffer_t g_kws_buf;
static int g_kws_en = 1;

#define BACLFLOW_LEN 0//(1024 * 300)
typedef struct {
    int len;
    char data[BACLFLOW_LEN];
} backflow_t;

backflow_t backflow_mind[3] = {0};
static void backflow_copy(int id, void *data, int len)
{
    backflow_t *b = &backflow_mind[id];

    if (BACLFLOW_LEN == 0) {
        return;
    }

    if ((b->len + len) < BACLFLOW_LEN) {
        memcpy(b->data + b->len, data, len);
        b->len += len;
    }
}
/*
static void backflow_reset(void)
{
    backflow_mind[0].len = 0;
    backflow_mind[1].len = 0;
    backflow_mind[2].len = 0;
}
*/
static void rec_preprocess(void *data, int *len)
{
    if (BACLFLOW_LEN == 0 && !g_pcm_debug_hook) {
        return;
    }

    static short *input_data = NULL;
    static int input_size = 0;

    if (!input_data || input_size < *len) {
        input_data = (short *)aos_realloc_check(input_data, *len);
        input_size = *len;
    }

    int chan_size = *len / 2 / 5;
    short *chan_data = (short *)data;

    memcpy(input_data, data, *len);

    /* turn 5-channel non-interleaved to interleaved */
    for (int i = 0; i < chan_size; i++) {
        chan_data[i * 5 + 0] = input_data[i];
        chan_data[i * 5 + 1] = input_data[i + chan_size];
        chan_data[i * 5 + 2] = input_data[i + chan_size * 2];
        chan_data[i * 5 + 3] = input_data[i + chan_size * 3];
        chan_data[i * 5 + 4] = input_data[i + chan_size * 4];
    }
}

static void voice_event(void *priv, voice_evt_id_t evt_id, void *data, int len)
{
    char *p = data;
    int data_len = len;

    if (evt_id == VOICE_ASR_EVT) {
        if (g_kws_en == 0) {
            return;
        }
        evt_id = MIC_EVENT_SESSION_START;
    } else if (evt_id == VOICE_DATA_EVT) {
        voice_data_t *vdata = (voice_data_t *)data;

        p = vdata->data;
        data_len = vdata->len;

        if (vdata->type == VOICE_AEC_DATA) {
            evt_id = MIC_EVENT_PCM_DATA;
            rec_preprocess((void *)p, &data_len);

            if (g_pcm_debug_hook) {
                g_pcm_debug_hook(evt_id, p, data_len);
            }
            
            backflow_copy(0, p, data_len);

            // _mic_rec_copy_data(0, (uint8_t *)p, data_len);
            return;
        } else if (vdata->type == VOICE_REF_DATA) {
            backflow_copy(1, p, data_len);
            // _mic_rec_copy_data(1, (uint8_t *)p, data_len);
            return;
        } else if (vdata->type == VOICE_VAD_DATA) {
            // LOGD(TAG, "pcm vad data %d %d", vdata->flag, vdata->len);
            
            if (vdata->flag == VOICE_VADSTAT_ASR_FINI) {
                evt_id = MIC_EVENT_SESSION_STOP;
            } else if (vdata->len > 0) {
                if (vdata->flag == VOICE_VAD_PCM_DATA) {
                    mic_vad_buf_t *vad_buf = (mic_vad_buf_t *)vdata->data;
                    
                    if (!mic_vad_data) {
                        mic_vad_data = aos_malloc_check(sizeof(mic_pcm_vad_data_t) + FRAME_SIZE);
                    }

                    if (vad_buf && vad_buf->len > 0) {
                        int frame_offset = 0;
                        
                        backflow_copy(2, vad_buf->data, vad_buf->len);
                        // _mic_rec_copy_data(2, (uint8_t *)vad_buf->data, vad_buf->len);

                        while (frame_offset < vad_buf->len / FRAME_SIZE) {
                            mic_vad_data->vad_tag = vad_buf->vad_tags[frame_offset];
                            mic_vad_data->len = FRAME_SIZE;
                            memcpy(mic_vad_data->data, vad_buf->data + frame_offset * FRAME_SIZE, FRAME_SIZE);

                            mic_evt(priv, MIC_EVENT_VAD_DATA, mic_vad_data, sizeof(mic_pcm_vad_data_t) + FRAME_SIZE);

                            frame_offset++;
                        }
                    }

                    return;
                } else {
                    if (g_vad_debug_hook) {
                        g_vad_debug_hook(evt_id, p, data_len);
                    }

                    backflow_copy(2, p, data_len);
                    // _mic_rec_copy_data(2, (uint8_t *)p, data_len);

                    evt_id = MIC_EVENT_PCM_DATA;
                }
            } else if (vdata->flag == VOICE_VADSTAT_VOICE) {
                evt_id = MIC_EVENT_VAD;
            } else {
                return;
            }
        } else if (vdata->type == VOICE_KWS_DATA) {
            if (g_kws_en == 0) {
                return;
            }
            LOGD(TAG, "kws data recv len %d", vdata->len);
            evt_id = MIC_EVENT_KWS_DATA;

            if (g_kws_buf.capacity < vdata->len) {
                aos_free(g_kws_buf.data);
                g_kws_buf.data = aos_malloc_check(vdata->len);
                g_kws_buf.capacity = vdata->len;
            }

            memcpy(g_kws_buf.data, vdata->data, vdata->len);
            g_kws_buf.len = vdata->len;
            g_kws_buf.offset = 0;       // for multiple get

            p = g_kws_buf.data;
            data_len = g_kws_buf.len;
        }
    } else if (evt_id == VOICE_SILENCE_EVT) {
        evt_id = MIC_EVENT_SESSION_STOP;
    } else {
        LOGE(TAG, "unkown event id");
        return;
    }

    mic_evt(priv, evt_id, p, data_len);
}

static int mic_adaptor_init(mic_t *mic, mic_event_t event)
{
    mic_evt = event;
    voice_t *v;

    v = voice_init(voice_event, mic);
    mic_set_privdata(v);

    return 0;
}

static int mic_adaptor_deinit(mic_t *mic)
{
    voice_t * v = (voice_t *)mic_get_privdata();

    voice_stop(v);
    voice_deinit(v);

    return 0;
}

int mic_adaptor_set_hook(aui_mic_debug_hook pcm_hook, aui_mic_debug_hook vad_hook)
{
    g_pcm_debug_hook = pcm_hook;
    g_vad_debug_hook = vad_hook;

    return 0;
}

// static int mic_adaptor_kws_control(mic_t *mic, int flag)
// {
//     g_kws_en = flag;
//     return 0;
// }

static int mic_adaptor_pcm_data_control(mic_t *mic, int flag, int enable)
{
    voice_t * v = (voice_t *)mic_get_privdata();

    // if (enable == 1) {
    //     backflow_reset();
    // }

	/* when mic hook set, don't allow disabling mic data flow */
    if (!g_pcm_debug_hook || enable) {
        voice_backflow_control(v, VOICE_AEC_DATA, enable == 0? 0 : 1);
    }		    
    // voice_backflow_control(v, VOICE_AEC_DATA, flag == 0? 0 : 1);
    // voice_backflow_control(v, VOICE_REF_DATA, flag == 0? 0 : 1);
    // voice_backflow_control(v, flag, enable == 0 ? 0 : 1);
    // voice_backflow_control(v, VOICE_VAD_DATA, flag == 0? 0 : 1);

    return 0;
}

// static int mic_adaptor_pcm_aec_control(mic_t *mic, int flag)
// {
//     // voice_t * v = (voice_t *)mic_get_privdata();

//     return 0;
// }

static int mic_adaptor_set_param(mic_t *mic, mic_param_t *param)
{
    voice_adpator_param_t *p = (voice_adpator_param_t*)param->priv;
    voice_t               *v = (voice_t *)mic_get_privdata();
    voice_pcm_param_t      pcm_p;
    voice_param_t v_p;

    pcm_p.access        = 0;
    pcm_p.channles      = param->channels;
    pcm_p.channles_sum  = param->channels;
    pcm_p.rate          = param->rate;
    pcm_p.sample_bits   = param->sample_bits;
    pcm_p.pcm_name      = p->pcm;
    voice_add_mic(v, &pcm_p);

    // pcm_p.pcm_name      = "pcmC2";
    // voice_add_ref(v, &pcm_p);

    v_p.cts_ms          = p->cts_ms;
    v_p.ipc_mode        = p->ipc_mode;
    ai_alg_param.sample_bits    = param->sample_bits;
    ai_alg_param.channels       = param->channels;
    ai_alg_param.rate           = param->rate;
    ai_alg_param.sentence_time_ms = param->sentence_time_ms;
    ai_alg_param.noack_time_ms = param->noack_time_ms;
    ai_alg_param.max_time_ms    = param->max_time_ms;

    /* if params set to null, don't run the algorithm */
    if (!ai_alg_param.sentence_time_ms &&
        !ai_alg_param.noack_time_ms &&
        !ai_alg_param.max_time_ms) {
        v_p.ai_param        = NULL;
        v_p.ai_param_len    = 0;
    } else {
        v_p.ai_param        = &ai_alg_param;
        v_p.ai_param_len    = sizeof(ai_alg_param);
    }
    voice_config(v, &v_p);

    return 0;
}

// static int mic_adaptor_get_kw(mic_t *mic, int kwid, char **kw)
// {
//     aos_check_param(kw);

//     voice_t * v = (voice_t *)mic_get_privdata();

//     voice_get_kw(v, kwid, kw);
//     return 0;
// }

static int mic_adaptor_start(mic_t *mic)
{
    voice_t * v = (voice_t *)mic_get_privdata();

    voice_start(v);
    voice_backflow_control(v, VOICE_VAD_DATA, 1);
    voice_backflow_control(v, VOICE_KWS_DATA, 1);

    return 0;
}

// static void _mic_rec_copy_data(int index, uint8_t *data, int size)
// {
//     if (g_mic_rec_start) {
//         mic_rec_copy_data(index, data, size);
//     }
// }

// static void mic_adaptor_rec_start(mic_t *mic, const char *url, const char *save_name)
// {
//     if (g_mic_rec_start) {
//         LOGW(TAG, "mic rec start yet, please stop before starting");
//         return;
//     }
//     mic_rec_start(url, save_name);
//     mic_adaptor_pcm_data_control(mic, 1);
//     g_mic_rec_start = 1;
// }

// static void mic_adaptor_rec_stop(mic_t *mic)
// {
//     if (g_mic_rec_start) {
//         g_mic_rec_start = 0;
//         mic_adaptor_pcm_data_control(mic, 0);
//         mic_rec_stop();
//     }
// }

static int mic_adaptor_stop(mic_t *mic)
{
    voice_t * v = (voice_t *)mic_get_privdata();

    voice_stop(v);
    return 0;
}

static int mic_adaptor_audio_control(mic_t *mic, int enable, int timeout)
{
    voice_t * v = (voice_t *)mic_get_privdata();

    if (enable) {
        voice_unmute(v);
    } else {
        voice_mute(v, timeout);
    }

    return 0;
}

static int mic_adaptor_ai_ctl(mic_t *mic, int cmd, void *param)
{
    if (cmd == MIC_CTRL_SET_VAD_PARAM) {
        mic_vad_param_t *p = (mic_vad_param_t *)param;

        ai_alg_param.vadswitch                   = p->vadswitch;
        ai_alg_param.vadmode                     = p->vadmode;
        ai_alg_param.vadfilter                   = p->vadfilter;
        ai_alg_param.vadkws_strategy             = p->vadkws_strategy;
        ai_alg_param.vadthresh_kws.vad_thresh_ep = p->vadthresh_kws.vad_thresh_ep;
        ai_alg_param.vadthresh_kws.vad_thresh_sp = p->vadthresh_kws.vad_thresh_sp;
        ai_alg_param.vadthresh_asr.vad_thresh_ep = p->vadthresh_asr.vad_thresh_ep;
        ai_alg_param.vadthresh_asr.vad_thresh_sp = p->vadthresh_asr.vad_thresh_sp;
    } else if (cmd == MIC_CTRL_SET_AEC_PARAM) {
        mic_aec_param_t *p = (mic_aec_param_t *)param;

        ai_alg_param.nsmode  = p->nsmode;
        ai_alg_param.aecmode = p->aecmode;
    } 

    return 0;
}

/* update the wakeup voice sate to ai side to allow voice cut */
static int mic_adaptor_fake_wakeup(mic_t *mic, int en, int flag)
{
    voice_t * v = (voice_t *)mic_get_privdata();

    voice_push2talk(v, VOICE_P2T_MODE_VAD);

    return 0;
}

static int mic_adaptor_debug_control(mic_t *mic, int flag)
{
    // int id1 = BACKFLOW_MIC_DATA | BACKFLOW_REF_DATA | BACKFLOW_AEC_DATA;
    // int id2 = BACKFLOW_VAD_DATA;

    // yv_debug_mode = flag;

    // if (yv_debug_mode == 2 || yv_debug_mode == 1) {
    //     backflow_control_rec(mic, id1, 1);
    //     backflow_control_rec(mic, id2, 1);
    // } else if (yv_debug_mode == 0) {
    //     backflow_control_rec(mic, id1, 0);
    //     backflow_control_rec(mic, id2, 0);
    // }

    return 0;
}

// static int mic_adaptor_lpm_data(mic_t *mic, void *mic_data, int len)
// {
//     voice_t              * v = (voice_t *)mic_get_privdata();

//     voice_lpm_data(v, mic_data, len);

//     return 0;
// }

static mic_ops_t voice_ops = {
    .init = mic_adaptor_init,
    .deinit = mic_adaptor_deinit,
    // .kws_control = mic_adaptor_kws_control,
    .ai_ctl = mic_adaptor_ai_ctl,
    .start = mic_adaptor_start,
    .stop = mic_adaptor_stop,
    .pcm_data_control = mic_adaptor_pcm_data_control,
    // .pcm_aec_control = mic_adaptor_pcm_aec_control,
    .set_param = mic_adaptor_set_param,
    .audio_control = mic_adaptor_audio_control,
    // .pcm_get_param = mic_adaptor_get_param,
    // .mic_rec_start = mic_adaptor_rec_start,
    // .mic_rec_stop = mic_adaptor_rec_stop,
    // .wakup_audio_stat = mic_adaptor_wkp_voice_state,
    .kws_wakeup = mic_adaptor_fake_wakeup,
    .debug_control    = mic_adaptor_debug_control,
    // .set_dbghook = mic_adaptor_set_hook,
    // .wwv_get_data = mic_adaptor_get_wwv_data,
    // .get_kw = mic_adaptor_get_kw,
    // .lpm_data = mic_adaptor_lpm_data
};

void mic_mind_register(void)
{
    mic_ops_register(&voice_ops);
}
