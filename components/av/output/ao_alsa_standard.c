/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_AV_AO_ALSA_STANDARD) && CONFIG_AV_AO_ALSA_STANDARD
#include "avutil/common.h"
#include "output/ao_cls.h"
#include <alsa/asoundlib.h>

#define TAG                    "ao_alsa_standard"

struct ao_alsa_priv {
    snd_pcm_t           *pcm;
    snd_pcm_hw_params_t *params;
};

static snd_pcm_t *pcm_init(ao_cls_t *o, unsigned int *rate)
{
    snd_pcm_t *pcm = NULL;
    snd_pcm_hw_params_t *params = NULL;
    struct ao_alsa_priv *priv   = o->priv;
    int err;
    unsigned long period_frames, buffer_frames;

    err = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    CHECK_RET_TAG_WITH_RET(err >= 0, NULL);

    snd_pcm_hw_params_malloc(&params);
    err = snd_pcm_hw_params_any(pcm, params);
    if (err < 0) {
        LOGE(TAG, "Broken configuration for this PCM: no configurations available");
        goto out;
    }

    err = snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        LOGE(TAG, "Access type not available");
        goto out;
    }

    err = snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        LOGE(TAG, "Sample format non available");
        goto out;
    }

    err = snd_pcm_hw_params_set_channels(pcm, params, CONFIG_AV_AO_CHANNEL_NUM);
    if (err < 0) {
        LOGE(TAG, "Channels count non available");
        goto out;
    }

    snd_pcm_hw_params_set_rate_near(pcm, params, rate, 0);

    period_frames = ((*rate) / 1000) * o->period_ms;
    snd_pcm_hw_params_set_period_size_near(pcm, params, &period_frames, 0);

    buffer_frames = period_frames * o->period_num;
    snd_pcm_hw_params_set_buffer_size_near(pcm, params, &buffer_frames);

    snd_pcm_hw_params(pcm, params);
    err = snd_pcm_prepare(pcm);
    if (err < 0) {
        LOGE(TAG, "prepare error, err = %d", err);
        goto out;
    }
    priv->params = params;

    return pcm;
out:
    snd_pcm_hw_params_free(params);
    snd_pcm_close(pcm);
    return NULL;
}

static int _ao_alsa_open(ao_cls_t *o, sf_t sf)
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

static int _ao_alsa_start(ao_cls_t *o)
{
    int rc = 0;
    unsigned int rate;
    struct ao_alsa_priv *priv = o->priv;

    if (priv->pcm) {
        //FIXME: may be not support pause low-level for D1-linux
        //snd_pcm_pause(priv->pcm, 0);
        rc = snd_pcm_prepare(priv->pcm);
        if (rc < 0) {
            LOGE(TAG, "prepare error, rc = %d", rc);
            return -1;
        }
    } else {
        rate = sf_get_rate(o->sf);
        priv->pcm = pcm_init(o, &rate);
        if (!priv->pcm) {
            LOGE(TAG, "ao alsa open failed\n");
            return -1;
        }
    }

    return rc;
}

static int _ao_alsa_stop(ao_cls_t *o)
{
    int rc = -1;
    struct ao_alsa_priv *priv = o->priv;

    if (priv->pcm) {
        //FIXME: may be not support pause low-level for D1-linux
        //judge with snd_pcm_hw_params_can_pause
        //snd_pcm_pause(priv->pcm, 1);
        rc = snd_pcm_drop(priv->pcm);
        if (rc < 0) {
            LOGE(TAG, "stop error");
            return -1;
        }
    }
    return rc;
}

static int _ao_alsa_drain(ao_cls_t *o)
{
    struct ao_alsa_priv *priv = o->priv;

    snd_pcm_drain(priv->pcm);
    return 0;
}

static int _ao_alsa_close(ao_cls_t *o)
{
    struct ao_alsa_priv *priv = o->priv;

    if (priv->pcm) {
        snd_pcm_hw_params_free(priv->params);
        snd_pcm_close(priv->pcm);
        priv->pcm = NULL;
    }

    av_free(o->priv);
    return 0;
}

static int _ao_alsa_write(ao_cls_t *o, const uint8_t *buffer, size_t count)
{
    int rc = 0;
    struct ao_alsa_priv *priv = o->priv;

    rc = snd_pcm_writei(priv->pcm, buffer, snd_pcm_bytes_to_frames(priv->pcm, count));
    if (rc < 0) {
        LOGE(TAG, "pcm write error, may be xrun, rc = %d", rc);
        if (rc == -EPIPE) {
            rc = snd_pcm_prepare(priv->pcm);
            if (rc < 0) {
                LOGE(TAG, "prepare error on EPIPE, rc = %d", rc);
                return -1;
            }
        }
    }

    return (rc >= 0)? count : -1;
}

const struct ao_ops ao_ops_alsa_standard = {
    .name          = "alsa_standard",

    .open          = _ao_alsa_open,
    .start         = _ao_alsa_start,
    .stop          = _ao_alsa_stop,
    .drain         = _ao_alsa_drain,
    .close         = _ao_alsa_close,
    .write         = _ao_alsa_write,
};
#endif


