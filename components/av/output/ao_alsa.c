/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_AV_AO_ALSA) && CONFIG_AV_AO_ALSA
#include "avutil/common.h"
#include "output/ao_cls.h"
#include <alsa/pcm.h>

#define TAG                    "ao_alsa"

struct ao_alsa_priv {
    aos_pcm_t *pcm;
};

static aos_pcm_t *pcm_init(ao_cls_t *o, unsigned int *rate)
{
    aos_pcm_hw_params_t *params;
    aos_pcm_t *pcm;
    int err, period_frames, buffer_frames;

    aos_pcm_open(&pcm, "pcmP0", AOS_PCM_STREAM_PLAYBACK, 0);

    aos_pcm_hw_params_alloca(&params);
    err = aos_pcm_hw_params_any(pcm, params);

    if (err < 0) {
        LOGE(TAG, "Broken configuration for this PCM: no configurations available");
    }

    err = aos_pcm_hw_params_set_access(pcm, params, AOS_PCM_ACCESS_RW_INTERLEAVED);

    if (err < 0) {
        LOGE(TAG, "Access type not available");
    }

    err = aos_pcm_hw_params_set_format(pcm, params, 16);

    if (err < 0) {
        LOGE(TAG, "Sample format non available");
    }

    err = aos_pcm_hw_params_set_channels(pcm, params, CONFIG_AV_AO_CHANNEL_NUM);

    if (err < 0) {
        LOGE(TAG, "Channels count non available");
    }

    aos_pcm_hw_params_set_rate_near(pcm, params, rate, 0);

    period_frames = ((*rate) / 1000) * o->period_ms;
    aos_pcm_hw_params_set_period_size_near(pcm, params, &period_frames, 0);

    buffer_frames = period_frames * o->period_num;
    aos_pcm_hw_params_set_buffer_size_near(pcm, params, &buffer_frames);


    err = aos_pcm_hw_params(pcm, params);

    return pcm;
}

static int _ao_minialsa_open(ao_cls_t *o, sf_t sf)
{
    unsigned int rate;
    struct ao_alsa_priv *priv = NULL;

    priv = av_zalloc(sizeof(struct ao_alsa_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

    rate = sf_get_rate(sf);
    switch (rate) {
    case 8000 :
    case 16000:
    case 32000:

    case 12000:
    case 24000:
    case 48000:

    case 11025:
    case 22050:
    case 44100:
        break;
    default:
        LOGE(TAG, "rate is not support, rate = %d\n", rate);
        goto err;
    }

    o->sf   = sf;
    o->priv = priv;
    LOGD(TAG, " ao open");
    return 0;
err:
    av_free(priv);

    return -1;
}

static int _ao_minialsa_start(ao_cls_t *o)
{
    unsigned int rate;
    struct ao_alsa_priv *priv = o->priv;

    if (priv->pcm) {
        aos_pcm_pause(priv->pcm, 0);
    } else {
        rate = sf_get_rate(o->sf);
        priv->pcm = pcm_init(o, &rate);
        if (!priv->pcm) {
            LOGE(TAG, "ao alsa open failed\n");
            return -1;
        }
    }

    return 0;
}

static int _ao_minialsa_stop(ao_cls_t *o)
{
    int rc = -1;
    struct ao_alsa_priv *priv = o->priv;

    if (priv->pcm) {
        aos_pcm_pause(priv->pcm, 1);
        rc = 0;
    }
    return rc;
}

static int _ao_minialsa_drain(ao_cls_t *o)
{
    struct ao_alsa_priv *priv = o->priv;

    aos_pcm_drain(priv->pcm);
    return 0;
}

static int _ao_minialsa_close(ao_cls_t *o)
{
    struct ao_alsa_priv *priv = o->priv;

    if (priv->pcm) {
        aos_pcm_close(priv->pcm);
        priv->pcm = NULL;
    }

    av_free(o->priv);
    return 0;
}

static int _ao_minialsa_write(ao_cls_t *o, const uint8_t *buffer, size_t count)
{
    int ret = 0;
    struct ao_alsa_priv *priv = o->priv;

    ret = aos_pcm_writei(priv->pcm, buffer, aos_pcm_bytes_to_frames(priv->pcm, count));

    return (ret >= 0)? count : -1;
}

const struct ao_ops ao_ops_alsa = {
    .name          = "alsa",

    .open          = _ao_minialsa_open,
    .start         = _ao_minialsa_start,
    .stop          = _ao_minialsa_stop,
    .drain         = _ao_minialsa_drain,
    .close         = _ao_minialsa_close,
    .write         = _ao_minialsa_write,
};
#endif


