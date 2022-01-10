/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <aos/aos.h>
#include <aos/kernel.h>
#include <csi_core.h>

#include <voice.h>
#include <voice_hal.h>
#include <voice_server_dbus.h>


#define voice_ops(v) v->ops

#define TAG "VCAI"


static aos_pcm_t *pcm;
static void capture_init(unsigned int rate, int channel, int format)
{
    aos_pcm_hw_params_t *params;
    int err, period_frames, buffer_frames;
    // int rate = 16000;
    // int channel = 1;
    // int format = 16;
    int period_bytes = 10 * 1024;//channel * rate * format / 8 * 20 / 1000;

    aos_pcm_open(&pcm, "pcmC0", AOS_PCM_STREAM_CAPTURE, 0);

    aos_pcm_hw_params_alloca(&params);
    err = aos_pcm_hw_params_any(pcm, params);

    if (err < 0) {
        LOGD(TAG, "Broken configuration for this PCM: no configurations available");
    }

    err = aos_pcm_hw_params_set_access(pcm, params, AOS_PCM_ACCESS_RW_INTERLEAVED);

    if (err < 0) {
        LOGD(TAG, "Access type not available");
    }

    err = aos_pcm_hw_params_set_format(pcm, params, format);

    if (err < 0) {
        LOGD(TAG, "Sample format non available");
    }

    err = aos_pcm_hw_params_set_channels(pcm, params, channel);

    if (err < 0) {
        LOGD(TAG, "Channels count non available");
    }

    aos_pcm_hw_params_set_rate_near(pcm, params, &rate, 0);

    period_frames = period_bytes / (format * channel / 8);
    aos_pcm_hw_params_set_period_size_near(pcm, params, &period_frames, 0);

    buffer_frames = period_frames * 20;
    aos_pcm_hw_params_set_buffer_size_near(pcm, params, &buffer_frames);

    err = aos_pcm_hw_params(pcm, params);
}

static int capture(void *data, int len)
{
    aos_pcm_readi(pcm, (void *)data, aos_pcm_bytes_to_frames(pcm, len));

    // 第三路模拟参考音，全设置为0
    char *p = (char *)data;
    // memset(p + 640 * 2, 0, 640);
    for(int i = 0; i < len; i += 6) {
        p[4] = 0;
        p[5] = 0;
        p += 6;
    }

    return len;
}


static void voice_entry(void *p);
// static void voice_data_send(void *priv, void *data, int len);


// static void pcm_data_update(voice_t *v, void *data, int len)
// {

//     if (voice_ops(v)->cache_cts > 0) {
//         // if (voice_rb_put(v->voice_rb, data, len) != 0) {
//         //     // LOGE(TAG, "voice rb full");
//         //     return;
//         // }
//     } else {
//         // if (v->pcm_data == NULL) {
//         //     v->pcm_data = aos_malloc_check(len + sizeof(voice_data_t));
//         // }

//         // memcpy(v->pcm_data->data, data, len);
//         // v->pcm_data->len = len;
//     }
    
//     aos_sem_signal(&v->sem);
// }

// static void pcm_backflow_update(voice_t *v, void *data, int len)
// {
    // int temp = *((int*)data);
    // int data_id = ((temp) >> 16) & 0xffff;

    // v->backflow_enable[data_id] = temp & 0xffff;

    // LOGD(TAG, "backflow %s data_id %d\n", v->backflow_enable[data_id] ? "open" : "close", data_id);
// }

int voice_start(voice_t *v)
{
    aos_check_return_einval(v);

    voice_pcm_param_t *p = v->mic_param;
    if (p) {
        p->period_bytes = p->channles_sum * p->rate * p->sample_bits / 8 * v->param.cts_ms / 1000;
    }

    p = v->ref_param;
    if (p) {
        p->period_bytes = p->channles_sum * p->rate * p->sample_bits / 8 * v->param.cts_ms / 1000;
    }

    // v->pcm = pcm_init(voice_data_send, v);

    // pcm_mic_config(v->pcm, v->mic_param);
    // pcm_ref_config(v->pcm, v->ref_param);

    // pcm_start(v->pcm);

    v->task_running = 1;

    unsigned int rate = 16000;
    int channel = 3;
    int format = 16;
    capture_init(rate, channel, format);

#if defined(CONFIG_VOICE_DBUS_SUPPORT) && CONFIG_VOICE_DBUS_SUPPORT
    extern void method_thread_main(void *arg);
    aos_task_new("voice dbus", method_thread_main, v, 10 * 1024);
#endif

    if (voice_ops(v)->start_alg_tsk) {
        voice_ops(v)->start_alg_tsk(v->priv, voice_entry, v);
    } else {
        aos_task_new("voice", voice_entry, v, 50 * 1024);
    }

    return 0;
}

int voice_stop(voice_t *v)
{
    aos_check_return_einval(v);

    v->task_running = 0;

    return 0;
}

int voice_fake_wakeup(voice_t *v)
{
    aos_check_return_einval(v);

    LOGD(TAG, "fake wakeup");
    v->fake_wakeup = 1;

    return 0;
}

int voice_mute(voice_t *v, int mute)
{
    aos_check_return_einval(v);

    LOGD(TAG, "mute voice %s", mute ? "start" : "stop");
    
    if (voice_ops(v)->mute) {
        voice_ops(v)->mute(v->priv, mute);
    }

    return 0;
}

int voice_p2t_switch(voice_t *v, int mode)
{
    aos_check_return_einval(v);

    if (voice_ops(v)->p2t) {
        voice_ops(v)->p2t(v->priv, mode);
    }

    return 0;
}

int voice_add_mic(voice_t *v, voice_pcm_param_t *p)
{
    aos_check_return_einval(v && p);

    if (v->mic_param == NULL) {
        v->mic_param = malloc(sizeof(voice_pcm_param_t));
    }

    memcpy(v->mic_param, p, sizeof(voice_pcm_param_t));

    return 0;
}

int voice_add_ref(voice_t *v, voice_pcm_param_t *p)
{
    aos_check_return_einval(v && p);

    if (v->ref_param == NULL) {
        v->ref_param = malloc(sizeof(voice_pcm_param_t));
    }

    memcpy(v->ref_param, p, sizeof(voice_pcm_param_t));

    return 0;
}

// static void *pcm_ref_get(voice_t *v, int ms)
// {
    // void *ref;
    // voice_pcm_param_t *p = v->mic_param;

    // int offset = p->period_bytes;

    // ref = ((char *)v->pcm_data->data + offset);

    // return ref;
//     return NULL;
// }

// static void *pcm_mic_get(voice_t *v, int ms)
// {
    // void *mic = NULL;

    // if (voice_ops(v)->cache_cts > 0) {
    //     voice_data_t *ele = voice_rb_get(v->voice_rb);
    //     if (ele) {
    //         mic = ele->data;
    //     }
    // } else {
    //     mic = (void **)v->pcm_data->data;
    // }

    // return mic;
//     return NULL;
// }

// static void voice_evt_send(voice_t *v, int evt_id, void *data, int len)
// {
    // voice_ai_msg_send(v, evt_id, data, len, 0);
// }

// static void voice_data_send(void *priv, void *data, int len)
// {
//     voice_t *v = (voice_t *)priv;

//     if (ringbuffer_available_write_space(&v->v_ringbuf) < len) {
//         LOGD(TAG, "voice_data_send write no space %d %d", ringbuffer_available_write_space(&v->v_ringbuf), len);
//     }
//     ringbuffer_write(&v->v_ringbuf, data, len);
// }

// static int voice_wait_pcm(voice_t *v, int ms)
// {
//     return (aos_sem_wait(&v->sem, AOS_WAIT_FOREVER));
// }

static void voice_entry(void *p)
{
    int ret = 0;
    void *ref = NULL;
    char *kws_data;

    voice_t *v = (voice_t *)p;
    csi_kws_t *kws = (csi_kws_t *)v->priv;

    int len = kws->format->period_time * (kws->format->sample_rate / 1000) *
             kws->format->sample_bits * kws->format->channel_num / 8;

    size_t vad_len, kws_len;
    void *mic = malloc(len);
    char *vad_out = malloc(1280);
    voice_vad_state vad_state;
    static int vad_cnt = 0;



    // if (voice_ops(v)->init)
    //     voice_ops(v)->init(v->kws);


    while (v->task_running) {
        aos_msleep(5);
        capture(mic, len);

            if (voice_ops(v)->vad) {
                vad_state = voice_ops(v)->vad(v->priv, mic, ref, 20, (void **)&vad_out, &vad_len);
                // LOGI(TAG, "ret %d, vad state %d, vad ts %d, sl start %d, now %d, vad cnt %d", ret, vad_state, v->vad_time_stamp, v->silence_start, aos_now_ms(), vad_cnt);
                if (vad_state != VOICE_VADSTAT_SILENCE || vad_len > 0) {
                    int to_send = 1;
                    /* vad end detected, stop the silence timer */
                    if (vad_state == VOICE_VADSTAT_ASR_FINI) {
                        v->silence_start = 0;
                    }

                    /* control the vad voice event freq to around 1/s */
                    if (vad_state == VOICE_VADSTAT_VOICE) {
                        vad_cnt = (vad_cnt + 1) % (1000 / 20);
                        
                        if (vad_cnt != 0 && vad_len <= 0) {
                            to_send = 0;
                        }
                    }

                    if (to_send) {
                        // voice_data_send(v, VOICE_VAD_DATA, vad_out, vad_len, vad_state);
                    }

                } else if (vad_state == VOICE_VADSTAT_SILENCE) {
                    vad_cnt = 0;
                }
                
                if (v->silence_start) {
                    if ((aos_now_ms() - v->vad_time_stamp) > v->silence_tmout) {
                        LOGI(TAG, "voice_entry vad %d", ret);
                        // voice_evt_send(v, VOICE_SILENCE_CMD, NULL, 0);
                    #if defined(CONFIG_VOICE_DBUS_SUPPORT) && CONFIG_VOICE_DBUS_SUPPORT
                        voice_dbus_signal_vad(v);
                    #endif
                        v->vad_time_stamp = 0;
                        v->silence_start = 0;
                    }
                }
            } else {
                ret = -1;
                vad_out = mic;
            }

            if (voice_ops(v)->kws || v->fake_wakeup) {
                ret = 0;
                if (voice_ops(v)->kws) {
                    ret = voice_ops(v)->kws(v->priv, mic, ref, 20, (void **)&vad_out);
                }

                if (ret > 0 || v->fake_wakeup) {
                    v->silence_start = 1;
                    v->vad_time_stamp = aos_now_ms();
                    v->fake_wakeup = 0;
                    LOGI(TAG, "voice_entry kws %d", ret);
                #if defined(CONFIG_VOICE_DBUS_SUPPORT) && CONFIG_VOICE_DBUS_SUPPORT
                    voice_dbus_signal_session_begin(v, &ret, sizeof(ret));
                #endif
                    // voice_evt_send(v, VOICE_KWS_CMD, &ret, sizeof(ret));
                }
            }

            if (voice_ops(v)->get_kws_data && voice_ops(v)->get_kws_data(v->priv, (void **)&kws_data, &kws_len) == 0) {
                // voice_data_send(v, VOICE_KWS_DATA, kws_data, kws_len, 0);
            }

            if (voice_ops(v)->asr && voice_ops(v)->asr(v->priv, (void **)&vad_out, 20) > 0) {
                v->silence_start = 1;
                v->vad_time_stamp = aos_now_ms();
                // voice_evt_send(v, VOICE_KWS_CMD, NULL, 0);
            }


            // if (v->mic_param) {
            //     voice_data_send(v, VOICE_AEC_DATA, vad_out, v->mic_param->period_bytes, 0);
            // }
        // }
    }

    free(mic);

    if (voice_ops(v)->deinit)
        voice_ops(v)->deinit(v->priv);
}

voice_t *voice_init(voice_evt_t cb, void *priv)
{
    int ret;

    if (NULL == priv) {
        return NULL;
    }

    voice_t *v = aos_zalloc_check(sizeof(voice_t));

    extern voice_ops_t *voice_ai_init(voice_t *v);
    voice_ops(v) = voice_ai_init(v);

    // char *v_buf = aos_zalloc_check(10240);
    // ringbuffer_create(&v->v_ringbuf, v_buf, 10240);

    v->priv  = malloc(sizeof(csi_kws_t));

    ((csi_kws_t *)v->priv)->priv = v;

#if defined(CONFIG_VOICE_DBUS_SUPPORT) && CONFIG_VOICE_DBUS_SUPPORT
    voice_dbus_init(v);
#endif

    ret   = aos_sem_new(&v->sem, 0);
    if (ret < 0) {
        aos_free(v);
        return NULL;
    }

    v->silence_tmout = 2000;

    voice_ops(v)->init(v->priv);

    return v;
}

void voice_deinit(voice_t *v)
{
    aos_check_return(v);

    aos_sem_free(&v->sem);

    v->task_running = 0;

    voice_ops(v)->deinit(v->priv);

#if defined(CONFIG_VOICE_DBUS_SUPPORT) && CONFIG_VOICE_DBUS_SUPPORT
    voice_dbus_deinit(v);
#endif

    aos_free(v);
}

int voice_config(voice_t *v, voice_param_t *p)
{
    aos_check_return_einval(v && p);
    memcpy(&v->param, p, sizeof(voice_param_t));

    if (voice_ops(v)->set_param)
        voice_ops(v)->set_param(v->priv, p->ai_param);

    return 0;
}

