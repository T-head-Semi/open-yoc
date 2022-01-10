/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <aos/aos.h>
#include <aos/kernel.h>
#include <csi_core.h>
#include <alsa/pcm.h>
#include <aos/list.h>

#include <voice_def.h>

#define TAG                    "voice"

#ifndef CONFIG_VPCM_BUFFER_FRAME_NUM
#define CONFIG_VPCM_BUFFER_FRAME_NUM    10
#endif
struct __voice_pcm {
    void                    *priv;
    voice_capture_t         *mic;
    voice_capture_t         *ref;
    void                    *data;
    int                      len;
    voice_pcm_send           pcm_send;

    int                      pause;
    int                      paused;

    voice_pcm_evt_t          evt;
    void                    *evt_priv;
};

static aos_pcm_t *_param_init(voice_pcm_t *vpcm, char *name, voice_pcm_param_t *p)
{
    if (name == NULL || p == NULL) {
        return NULL;
    }

    aos_pcm_hw_params_t *params;
    aos_pcm_t *pcm;
    int err, period_frames, buffer_frames;

    aos_pcm_open(&pcm, name, AOS_PCM_STREAM_CAPTURE, 0);

    aos_pcm_hw_params_alloca(&params);
    err = aos_pcm_hw_params_any(pcm, params);

    if (err < 0) {
        LOGD(TAG, "Broken configuration for this PCM: no configurations available");
    }

    err = aos_pcm_hw_params_set_access(pcm, params, p->access == 0 ? AOS_PCM_ACCESS_RW_NONINTERLEAVED :AOS_PCM_ACCESS_RW_INTERLEAVED);

    if (err < 0) {
        LOGD(TAG, "Access type not available");
    }

    err = aos_pcm_hw_params_set_format(pcm, params, p->sample_bits);

    if (err < 0) {
        LOGD(TAG, "Sample format non available");
    }

    err = aos_pcm_hw_params_set_channels(pcm, params, p->channles);

    if (err < 0) {
        LOGD(TAG, "Channels count non available");
    }

    aos_pcm_hw_params_set_rate_near(pcm, params, &p->rate, 0);

    period_frames = p->period_bytes / (p->sample_bits * p->channles / 8);
    aos_pcm_hw_params_set_period_size_near(pcm, params, &period_frames, 0);

    buffer_frames = period_frames * CONFIG_VPCM_BUFFER_FRAME_NUM;
    aos_pcm_hw_params_set_buffer_size_near(pcm, params, &buffer_frames);

    err = aos_pcm_hw_params(pcm, params);

    return pcm;
}

static int pcm_alsa_stop(aos_pcm_t *pcm)
{
    aos_pcm_close(pcm);

    return 0;
}


static int pcm_alsa_start(voice_pcm_t *p)
{
    // voice_t *v = p->priv;
    // v->
    p->mic->hdl = _param_init(p, p->mic->param->pcm_name,  p->mic->param);

    if (p->ref) {
        p->ref->hdl = _param_init(p, p->ref->param->pcm_name,  p->ref->param);
    }

    return 0;
}


// static char test_data[1024*4];
static int pcm_recv(aos_pcm_t *pcm, void *data, int len, int access)
{
    int ret = -1;

    while (1) {
        ret = aos_pcm_wait(pcm, AOS_WAIT_FOREVER);

        if (ret < 0) {
            aos_pcm_recover(pcm, ret, 1);
            continue;
        }

        if (access) {
            // ret = aos_pcm_readi(pcm, (void *)test_data, aos_pcm_bytes_to_frames(pcm, len));
            ret = aos_pcm_readi(pcm, (void *)data, aos_pcm_bytes_to_frames(pcm, len));
        } else {
            // ret = aos_pcm_readn(pcm, (void **)test_data, aos_pcm_bytes_to_frames(pcm, len));
            ret = aos_pcm_readn(pcm, (void **)data, aos_pcm_bytes_to_frames(pcm, len));
        }

        if(ret > 0) {
            break;
        }
    }

    // LOGD(TAG, "captrue over");
    // memcpy((char *)data, test_data, len);

    return aos_pcm_frames_to_bytes(pcm, ret);
}

static void pcm_buffer_init(voice_pcm_t *pcm)
{
    int mic_len = 0;
    int ref_len = 0;

    if (pcm == NULL || (pcm->mic == NULL && pcm->ref == NULL)) {
        return;
    }

    if (pcm->mic) {
        voice_pcm_param_t *p = pcm->mic->param;
        mic_len = p->period_bytes;
    }

    if (pcm->ref) {
        voice_pcm_param_t *p =pcm->ref->param;
        ref_len = p->period_bytes;
    }

    pcm->data = voice_malloc(mic_len + ref_len);
    pcm->len = mic_len + ref_len;
    pcm->mic->len = mic_len;
    pcm->mic->data = pcm->data;

    if (pcm && pcm->ref && pcm->ref->param) {
        pcm->ref->data = (char *)pcm->data + mic_len;
        pcm->ref->len = ref_len;
    }
}

static void pcm_buffer_deinit(voice_pcm_t *p)
{
    aos_free(p->mic->param);
    aos_free(p->ref->param);
    aos_free(p->mic);
    aos_free(p->ref);
    voice_free(p->data);
}

static void pcm_entry(void *priv)
{
    voice_pcm_t *p = (voice_pcm_t *)priv;
    int ret = -1;
    voice_capture_t *capture;
    int len;

    p->mic->len  = p->mic->param->period_bytes;
    len = p->mic->len;

    if (p->ref) {
        p->ref->len  = p->ref->param->period_bytes;
        len += p->ref->len;
    }
    
    p->data = malloc(len);
    p->mic->data = p->data;

    if (p->ref) {
        p->ref->data = p->data + p->mic->len;
    }


    pcm_alsa_start(p);

    while (1) {
        while (p->pause) {
            p->paused = 1;

            aos_msleep(20);
        }
        p->paused = 0;

        capture = p->mic;
        ret = pcm_recv(capture->hdl, capture->data, capture->len,capture->param->access);
        capture = p->ref;
        if (capture) {
            ret = pcm_recv(capture->hdl, capture->data, capture->len,capture->param->access);
        }

        if (ret >= 0) {
            if (p->evt) {
                p->evt(p->priv, PCM_PRE_SEND, p->data, p->len);
            }

            p->pcm_send(p->priv, p->data, p->len);

            if (p->evt) {
                p->evt(p->priv, PCM_POST_SEND, p->data, p->len);
            }
        }
    }
}

voice_pcm_t *pcm_init(voice_pcm_send send, void *priv)
{
    voice_pcm_t *p = aos_zalloc_check(sizeof(voice_pcm_t));

    p->pcm_send = send;
    p->priv     = priv;

    return p;
}

void pcm_deinit(voice_pcm_t *p)
{
    pcm_buffer_deinit(p);
}

int pcm_pause(voice_pcm_t *p)
{
    p->pause    = 1;

    while (!p->paused) {
        aos_msleep(10);
    }

    return 0;
}

int pcm_resume(voice_pcm_t *p)
{
    p->pause   = 0;
    return 0;
}

int pcm_evt_register(voice_pcm_t *p, voice_pcm_evt_t evt, void *priv)
{
    p->priv = priv;
    p->evt  = evt;
    return 0;
}

void pcm_mic_config(voice_pcm_t *p, voice_pcm_param_t *param)
{
    voice_capture_t *capture = p->mic;

    if (param == NULL) {
        return;
    }

    if (capture == NULL) {
        capture = aos_malloc_check(sizeof(voice_capture_t));
        capture->param = aos_malloc_check(sizeof(voice_pcm_param_t));
        p->mic = capture;
    }

    memcpy(capture->param, param, sizeof(voice_pcm_param_t));
}

void pcm_ref_config(voice_pcm_t *p, voice_pcm_param_t *param)
{
    voice_capture_t *capture = p->ref;

    if (param == NULL) {
        return;
    }

    if (capture == NULL) {
        capture = aos_malloc_check(sizeof(voice_capture_t));
        capture->param = aos_malloc_check(sizeof(voice_pcm_param_t));
        p->ref = capture;
    }

    memcpy(capture->param, param, sizeof(voice_pcm_param_t));
}

int pcm_start(voice_pcm_t *p)
{
    aos_task_t task;
    int ret = aos_task_new_ext(&task, "vpcm", pcm_entry, p, 2 * 1024, AOS_DEFAULT_APP_PRI - 4);

    if (ret < 0) {
        return -1;
    }

    return 0;
}

int pcm_stop(voice_pcm_t *p)
{
    if (p->mic->hdl) {
        pcm_alsa_stop(p->mic->hdl);
    }

    if (p->ref->hdl) {
        pcm_alsa_stop(p->ref->hdl);
    }

    // aos_task_new
    return 0;
}

