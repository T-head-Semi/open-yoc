/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_AEFXER_SONA) && CONFIG_AEFXER_SONA

#include "avutil/av_typedef.h"
#include "aef/aef_cls.h"
#include "aef/audioaef.h"

#define TAG                   "aef_sona"
struct aef_sona_priv {
    void                      *hdl;
};

#define AEF_INIT_ONCE 1

#if AEF_INIT_ONCE
static void * g_aef_sona_hdl = NULL;
#endif

#if CONFIG_AV_AEF_DEBUG
static void *aef_ins;
static void *rpc_ins;

int aef_debug_init(int fs, uint8_t *conf, size_t conf_size)
{
    if (!aef_ins) {
        aef_ins = audioaef_create_fromBuffer(fs, fs, conf, conf_size, 16, POST_PROC_SAMPLES_MAX);
        CHECK_RET_TAG_WITH_RET(aef_ins, -1);
    }

    if (!rpc_ins) {
        rpc_ins = audioRPC_init(aef_ins);
        CHECK_RET_TAG_WITH_RET(rpc_ins, -1);
    }

    return 0;
}

void aef_debug_uninit()
{
    if (aef_ins) {
        audioaef_destroy(aef_ins);
        aef_ins = NULL;
    }
    if (rpc_ins) {
        audioRPC_close(rpc_ins);
        rpc_ins = NULL;
    }
}

#endif

#define EQ_CLEAN_FRAME_SIZE 512
static short g_zero_pcm_in[EQ_CLEAN_FRAME_SIZE] = {0};
static short g_zero_pcm_out[EQ_CLEAN_FRAME_SIZE] = {0};
static int _aef_sona_init(aefx_t *aef)
{
    void *hdl = NULL;
    struct aef_sona_priv *priv = NULL;

    priv = av_zalloc(sizeof(struct aef_sona_priv));
    CHECK_RET_TAG_WITH_RET(priv, -1);

#if CONFIG_AV_AEF_DEBUG
    priv->hdl = aef_ins;
    audioaef_set_fs(aef_ins, aef->rate, aef->rate);
#else
#if AEF_INIT_ONCE
    if (g_aef_sona_hdl == NULL) {
        hdl = audioaef_create_fromBuffer(aef->rate, aef->rate, (char*)aef->conf, aef->conf_size, sizeof(int16_t) * 8, aef->nsamples_max);
        g_aef_sona_hdl = hdl;
        for(int i = 0; i < 512; i++) {
            g_zero_pcm_in[i] = rand() % 5;
        }
    } else {
        hdl = g_aef_sona_hdl;
        audioaef_set_fs(hdl, aef->rate, aef->rate);
    }
#else
    hdl = audioaef_create_fromBuffer(aef->rate, aef->rate, (char*)aef->conf, aef->conf_size, sizeof(int16_t) * 8, aef->nsamples_max);
#endif

    /* clear history */
    for(int i = 0; i < 8; i++) {
        audioaef_process(hdl, (char*)g_zero_pcm_in, EQ_CLEAN_FRAME_SIZE, 1, sizeof(int16_t) * 8, (char*)g_zero_pcm_out, 1);
    }

    CHECK_RET_TAG_WITH_GOTO(hdl, err);
    priv->hdl = hdl;
#endif
    aef->priv = priv;
    return 0;
err:
    av_free(priv);
    return -1;
}

static int _aef_sona_process(aefx_t *aef, const int16_t *in, int16_t *out, size_t nb_samples)
{
    int rc;
    size_t size;
    struct aef_sona_priv *priv = aef->priv;

    size = nb_samples * sizeof(int16_t);

    rc = audioaef_process(priv->hdl, (char*)in, size, 1, sizeof(int16_t) * 8, (char*)out, 1);

    CHECK_RET_TAG_WITH_RET(rc == size, -1);

    return 0;
}

static int _aef_sona_uninit(aefx_t *aef)
{
    struct aef_sona_priv *priv = aef->priv;

#if !(CONFIG_AV_AEF_DEBUG)
#if !(AEF_INIT_ONCE)
    audioaef_destroy(priv->hdl);
#endif
#endif
    av_free(priv);
    aef->priv = NULL;
    return 0;
}

const struct aefx_ops aefx_ops_sona = {
    .name            = "aef_sona",

    .init            = _aef_sona_init,
    .process         = _aef_sona_process,
    .uninit          = _aef_sona_uninit,
};

#endif

