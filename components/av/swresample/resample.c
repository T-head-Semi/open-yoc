/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/common.h"
#include "swresample/resample.h"

#define TAG                   "resample"

static struct resx_ops       *_res_ops;

/**
 * @brief  regist resample ops
 * @param  [in] ops
 * @return 0/-1
 */
int resx_ops_register(const struct resx_ops *ops)
{
    CHECK_PARAM(ops, -1);
    if (_res_ops) {
        LOGE(TAG, "error. ops had regist yet!, name = %s", _res_ops->name);
        return -1;
    }
    _res_ops = (struct resx_ops*)ops;
    LOGD(TAG, "regist resampler, name = %s", _res_ops->name);

    return 0;
}

/**
 * @brief  new a resampler
 * @param  [in] irate    : input sample rate
 * @param  [in] orate    : output sample rate
 * @param  [in] channels : 1/2, mono/stereo
 * @param  [in] bits     : 16 only
 * @return NULL on error
 */
resx_t *resx_new(uint32_t irate, uint32_t orate, uint8_t channels, uint8_t bits)
{
    int rc;
    resx_t *r = NULL;

    CHECK_PARAM((irate != orate), NULL);
    CHECK_PARAM((bits == 16), NULL);
    CHECK_PARAM(((channels == 1) || (channels == 2)), NULL);

    if (!((irate == 8000) || (irate == 16000) || (irate == 32000)
          || (irate == 12000) || (irate == 24000) || (irate == 48000)
          || (irate == 11025) || (irate == 22050) || (irate == 44100))) {
        LOGE(TAG, "irate is not support, irate = %d", irate);
        return NULL;
    }

    if (!((orate == 8000) || (orate == 16000) || (orate == 32000)
          || (orate == 12000) || (orate == 24000) || (orate == 48000)
          || (orate == 11025) || (orate == 22050) || (orate == 44100))) {
        LOGE(TAG, "orate is not support, orate = %d", orate);
        return NULL;
    }

    r = av_zalloc(sizeof(resx_t));
    CHECK_RET_TAG_WITH_RET(r, NULL);

    r->irate    = irate;
    r->orate    = orate;
    r->bits     = bits;
    r->channels = channels;
    r->ops      = _res_ops;
    rc = r->ops->init(r);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    aos_mutex_new(&r->lock);
    return r;
err:
    av_free(r);
    return NULL;
}

/**
 * @brief  get the max out samples number per channel
 * @param  [in] irate
 * @param  [in] orate
 * @param  [in] nb_isamples : number of input samples available in one channel
 * @return -1 on error
 */
int resx_get_osamples_max(uint32_t irate, uint32_t orate, size_t nb_isamples)
{
    int rc;

    //FIXME: guess
    CHECK_PARAM(irate && orate && nb_isamples, -1);
    rc = (1.2 * orate / irate * nb_isamples);
    rc = AV_ALIGN_SIZE(rc + 16, 16);

    return irate == orate ? nb_isamples : rc;
}

/**
 * @brief  convert nb_samples from src to dst sample format
 * @param  [in] r
 * @param  [in] out
 * @param  [in] nb_osamples : amount of space available for output in samples per channels
 * @param  [in] in
 * @param  [in] nb_isamples : number of input samples available in one channels
 * @return number of samples output per channels, -1 on error
 */
int resx_convert(resx_t *r, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    int rc, i;

    CHECK_PARAM(r && out && in && nb_osamples && nb_isamples, -1);
    for (i = 0; i < r->channels; i++) {
        if (!(in[i] && out[i])) {
            LOGE(TAG, "out/in params is err.");
            return -1;
        }
    }

    aos_mutex_lock(&r->lock, AOS_WAIT_FOREVER);
    rc = r->ops->convert(r, out, nb_osamples, in, nb_isamples);
    aos_mutex_unlock(&r->lock);

    return rc;
}

/**
 * @brief  free the resample
 * @param  [in] r
 * @return
 */
void resx_free(resx_t *r)
{
    if (r) {
        r->ops->uninit(r);
        aos_mutex_free(&r->lock);
        av_free(r);
    }
}



