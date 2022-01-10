/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/misc.h"
#include "avutil/byte_rw.h"
#include "swresample/aformat_conv.h"

#define TAG                   "afconv"

#define ACONV_FUNC_NAME(dfmt, sfmt) _afconv_ ## sfmt ## _to_ ## dfmt
#define SET_ACONV_FUNC(ac, ofmt, ifmt) _set_aconv_fun(ac, ofmt, ifmt, ACONV_FUNC_NAME(ofmt,  ifmt))

typedef int (*afconv_func_t)(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples);

struct aformat_conv {
    aformat_t                 inf;        ///< input audio format
    aformat_t                 outf;       ///< output audio format
    esf_t                     iesf;
    esf_t                     oesf;
    uint8_t                   iplanar;
    uint8_t                   oplanar;
    uint8_t                   channels;

    afconv_func_t             conv;
    aos_mutex_t               lock;
};

static int _afconv_ESF_S8_to_ESF_S16LE(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    int i, j = 0;
    int16_t sample;
    int16_t *d        = (int16_t*)out[0];
    const int8_t *s   = in[0];
    size_t nb_samples = nb_isamples * ac->channels;

    for (i = 0; i < nb_samples; i++) {
        sample = s[i] << 8;
        d[j++] = sample;
    }

    return nb_isamples;
}

static int _afconv_ESF_U8_to_ESF_S16LE(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    int i, j = 0;
    int16_t sample;
    int16_t *d        = out[0];
    const int8_t *s   = in[0];
    size_t nb_samples = nb_isamples * ac->channels;

    for (i = 0; i < nb_samples; i++) {
        sample  = s[i] << 8;
        sample -= 32768;
        d[j++]  = sample;
    }

    return nb_isamples;
}

static int _afconv_ESF_U16LE_to_ESF_S16LE(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    int i, j = 0;
    int sample;
    int16_t *d        = out[0];
    uint16_t *s       = (uint16_t*)in[0];
    size_t nb_samples = nb_isamples * ac->channels;

    for (i = 0; i < nb_samples; i++) {
        sample  = s[i];
        sample -= 32768;
        d[j++]  = sample;
    }

    return nb_isamples;
}

static int _afconv_ESF_S8P_to_ESF_S16LE(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    //TODO:
    return -1;
}

static int _afconv_ESF_U8P_to_ESF_S16LE(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    //TODO:
    return -1;
}

static int _afconv_ESF_S16LEP_to_ESF_S16LE(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    //TODO:
    return -1;
}

static int _afconv_ESF_U16LEP_to_ESF_S16LE(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    //TODO:
    return -1;
}

static int _afconv_ESF_S24LE_to_ESF_S16LE(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    int i, j = 0;
    int sample;
    int16_t *d        = out[0];
    uint8_t *s        = (uint8_t*)in[0];
    size_t nb_samples = nb_isamples * ac->channels;

    for (i = 0; i < nb_samples; i++) {
        sample  = byte_r24le(s) << 8;
        sample  = sample >> 16;
        d[j++]  = sample;
        s      += 3;
    }

    return nb_isamples;
}

static int _afconv_ESF_S32LE_to_ESF_S16LE(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    int i, j = 0;
    int sample;
    int16_t *d        = out[0];
    int32_t *s        = (int32_t*)in[0];
    size_t nb_samples = nb_isamples * ac->channels;

    for (i = 0; i < nb_samples; i++) {
        sample = s[i] >> 16;
        d[j++] = sample;
    }

    return nb_isamples;
}

static int _afconv_ESF_F32LE_to_ESF_S16LE(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    int i, j = 0;
    int sample;
    int16_t *d        = out[0];
    float *s          = (float*)in[0];
    size_t nb_samples = nb_isamples * ac->channels;

    for (i = 0; i < nb_samples; i++) {
        sample = s[i] * (1 << 15);
        sample = clip_int16(sample);
        d[j++] = sample;
    }

    return nb_isamples;
}

static int _afconv_ESF_F64LE_to_ESF_S16LE(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    int i, j = 0;
    int sample;
    int16_t *d        = out[0];
    double *s         = (double*)in[0];
    size_t nb_samples = nb_isamples * ac->channels;

    for (i = 0; i < nb_samples; i++) {
        sample = s[i] * (1 << 15);
        sample = clip_int16(sample);
        d[j++] = sample;
    }

    return nb_isamples;
}
static esf_t _get_esf_type(const aformat_t *af)
{
    sf_t sf;

    sf = sf_make_signed(af->issign) | sf_make_bigendian(af->be) |
         sf_make_planar(af->planar) | sf_make_bit(af->bits) | sf_make_float(af->isfloat);

    return get_esf_type(sf);
}

static void _set_aconv_fun(afconv_t *ac, esf_t oesf, esf_t iesf, void *conv)
{
    if ((ac->oesf == oesf) && (ac->iesf == iesf) && conv) {
        ac->conv = conv;
    }
}

static void _set_aconv_generic(afconv_t *ac)
{
    SET_ACONV_FUNC(ac, ESF_S16LE, ESF_S8);
    SET_ACONV_FUNC(ac, ESF_S16LE, ESF_U8);
    SET_ACONV_FUNC(ac, ESF_S16LE, ESF_U16LE);
    SET_ACONV_FUNC(ac, ESF_S16LE, ESF_S8P);
    SET_ACONV_FUNC(ac, ESF_S16LE, ESF_S16LEP);
    SET_ACONV_FUNC(ac, ESF_S16LE, ESF_U8P);
    SET_ACONV_FUNC(ac, ESF_S16LE, ESF_U16LEP);

    SET_ACONV_FUNC(ac, ESF_S16LE, ESF_S24LE);
    SET_ACONV_FUNC(ac, ESF_S16LE, ESF_S32LE);

    SET_ACONV_FUNC(ac, ESF_S16LE, ESF_F32LE);
    SET_ACONV_FUNC(ac, ESF_S16LE, ESF_F64LE);
}

/**
 * @brief  new a audio-converter
 * @param  [in] inf      : original sample format
 * @param  [in] outf     : destination sample format
 * @param  [in] channels
 * @return NULL on error
 */
afconv_t *afconv_new(const aformat_t *inf, const aformat_t *outf, uint8_t channels)
{
    afconv_t *ac = NULL;

    CHECK_PARAM(inf && outf, NULL);
    CHECK_PARAM((inf->be == 0 || inf->be == 1), NULL);
    CHECK_PARAM((inf->issign == 0 || inf->issign == 1), NULL);
    CHECK_PARAM((inf->planar == 0 || inf->planar == 1), NULL);
    CHECK_PARAM((outf->be == 0 || outf->be == 1), NULL);
    CHECK_PARAM((outf->issign == 0 || outf->issign == 1), NULL);
    CHECK_PARAM((outf->planar == 0 || outf->planar == 1), NULL);
    CHECK_PARAM(channels == 1 || channels == 2, NULL);
    if (!memcmp(inf, outf, sizeof(aformat_t))) {
        LOGE(TAG, "in & outf can't same.");
        return NULL;
    }

    ac = av_zalloc(sizeof(afconv_t));
    CHECK_RET_TAG_WITH_GOTO(ac, err);

    ac->iplanar = channels == 1 ? 1 : inf->planar;
    ac->oplanar = channels == 1 ? 1 : outf->planar;
    memcpy(&ac->inf, inf, sizeof(aformat_t));
    memcpy(&ac->outf, outf, sizeof(aformat_t));
    ac->iesf     = _get_esf_type(inf);
    ac->oesf     = _get_esf_type(outf);
    ac->channels = channels;
    _set_aconv_generic(ac);
    if (!ac->conv) {
        LOGE(TAG, "aconv not support . oformat: %s, iformat: %s.", get_esf_name(ac->oesf), get_esf_name(ac->iesf));
        goto err;
    }
    aos_mutex_new(&ac->lock);

    return ac;
err:
    av_free(ac);
    return NULL;
}

/**
 * @brief  convert nb_isamples from src to dst sample format
 * @param  [in] s
 * @param  [in] out
 * @param  [in] nb_osamples : amount of space available for output in samples per channel
 * @param  [in] in
 * @param  [in] nb_isamples : number of input samples available in one channel
 * @return number of samples output per channel, -1 on error
 */
int afconv_convert(afconv_t *ac, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    int rc;

    CHECK_PARAM(ac && out && in && *in && *out && nb_osamples && nb_isamples && (nb_isamples <= nb_osamples), -1);
    aos_mutex_lock(&ac->lock, AOS_WAIT_FOREVER);
    rc = ac->conv(ac, out, nb_osamples, in, nb_isamples);
    aos_mutex_unlock(&ac->lock);

    return rc;
}

/**
 * @brief  free the audio-converter
 * @param  [in] s
 * @return
 */
void afconv_free(afconv_t *ac)
{
    if (ac) {
        aos_mutex_free(&ac->lock);
        av_free(ac);
    }
}







