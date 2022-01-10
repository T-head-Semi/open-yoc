/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/mem_block.h"
#include "swresample/resample.h"
#include "swresample/pcm_convert.h"
#include "swresample/aformat_conv.h"
#include "swresample/swresample.h"

#define TAG                   "swresample"
#define SWR_MB_SIZE_DEFAULT   (4*1024)

/*
 * A->C, A->B->C
 * A is the source format, C is the dst format, B is the resampler
 */
struct swr_context {
    sf_t                      isf;
    sf_t                      osf;
    sf_t                      isf_r;            ///< inner osf as input format for resample
    afconv_t                  *ac;              ///< convert directly or isf_r to osf or ch1<=>ch2 directly
    afconv_t                  *ac_r;            ///< if irate != orate
    resx_t                    *r;               ///< if irate != orate
    uint8_t                   max_sample_size;

    mblock_t                  *mb1;             ///< inner convert buf
    mblock_t                  *mb2;             ///< inner convert buf

    aos_mutex_t               lock;
};

/**
 * @brief  new a swreampler
 * @param  [in] isf : original sample format
 * @param  [in] osf : destination sample format
 * @return NULL on error
 * TODO:   dst only support 16bit/interleave now
 */
swr_t *swr_new(sf_t isf, sf_t osf)
{
    sf_t isf_r = 0, tsf = 0;
    swr_t *s       = NULL;
    resx_t *r  = NULL;
    afconv_t *ac_r = NULL, *ac = NULL;
    mblock_t *mb1  = NULL, *mb2 = NULL;
    aformat_t inf, outf;

    CHECK_PARAM((isf != osf), NULL);
    CHECK_PARAM(sf_get_bit(osf) == 16, NULL);
    CHECK_PARAM(sf_get_planar(osf) == 0, NULL);

    tsf = isf;
    s   = av_zalloc(sizeof(swr_t));
    CHECK_RET_TAG_WITH_RET(s, NULL);
    mb1 = mblock_new(SWR_MB_SIZE_DEFAULT, 0);
    mb2 = mblock_new(SWR_MB_SIZE_DEFAULT, 0);
    CHECK_RET_TAG_WITH_GOTO(mb1 && mb2, err);

    if (sf_get_rate(isf) != sf_get_rate(osf)) {
        //A->B
        //FIXME: performance opt & resampler limit reason, etc
        r = resx_new(sf_get_rate(isf), sf_get_rate(osf), 1, 16);
        CHECK_RET_TAG_WITH_RET(r, NULL);
        isf_r = sf_make_channel(1) | sf_make_rate(sf_get_rate(isf)) | sf_make_bit(16) | sf_make_signed(1);
        /* need judge isf == isf_r */
        if (isf != isf_r) {
            get_aformat_from_sf(isf, &inf);
            get_aformat_from_sf(isf_r, &outf);
            if (memcmp(&inf, &outf, sizeof(aformat_t))) {
                /* audio format convert before resample */
                ac_r = afconv_new(&inf, &outf, sf_get_channel(isf));
                CHECK_RET_TAG_WITH_GOTO(ac_r, err);
            }
        }
        tsf = isf_r;
    }

    //A->C or B->C
    if (tsf != osf) {
        get_aformat_from_sf(tsf, &inf);
        get_aformat_from_sf(osf, &outf);
        if (memcmp(&inf, &outf, sizeof(aformat_t))) {
            /* audio format convert before resample */
            ac = afconv_new(&inf, &outf, sf_get_channel(isf));
            CHECK_RET_TAG_WITH_GOTO(ac, err);
        }
    }

    s->isf             = isf;
    s->osf             = osf;
    s->isf_r           = isf_r;
    s->ac              = ac;
    s->ac_r            = ac_r;
    s->r               = r;
    s->mb1             = mb1;
    s->mb2             = mb2;
    s->max_sample_size = sizeof(int16_t); //support 16bit only now
    aos_mutex_new(&s->lock);

    return s;
err:
    afconv_free(ac);
    afconv_free(ac_r);
    resx_free(r);
    mblock_free(mb1);
    mblock_free(mb2);
    av_free(s);
    return NULL;
}

/**
 * @brief  get the max out samples number per channel
 * @param  [in] s
 * @param  [in] nb_isamples : number of input samples available in one channel
 * @return -1 on error
 */
int swr_get_osamples_max(swr_t *s, size_t nb_isamples)
{
    CHECK_PARAM(s && nb_isamples, -1);
    return resx_get_osamples_max(sf_get_rate(s->isf), sf_get_rate(s->osf), nb_isamples);
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
int swr_convert(swr_t *s, void **out, size_t nb_osamples, const void **in, size_t nb_isamples)
{
    //TODO: (dst only support 16bit/!planar now)
    int rc;
    sf_t tsf;
    size_t swr_size = 0, osamples;
    void **in_r, **out_r;

    CHECK_PARAM(s && out && nb_osamples && in && nb_isamples, -1);

    aos_mutex_lock(&s->lock, AOS_WAIT_FOREVER);
    osamples = resx_get_osamples_max(sf_get_rate(s->isf), sf_get_rate(s->osf), nb_isamples);
    swr_size = 2 * osamples * s->max_sample_size;
    rc = mblock_grow(s->mb1, swr_size);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    rc = mblock_grow(s->mb2, swr_size);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

    in_r  = (void**)in;
    out_r = out;
    tsf   = s->isf;
    if (s->r) {
        /* A->B */
        if (s->isf != s->isf_r) {
            if (s->ac_r) {
                rc = afconv_convert(s->ac_r, (void**)&s->mb1->data, nb_osamples, (const void**)in_r, nb_isamples);
                CHECK_RET_TAG_WITH_GOTO(rc > 0, err);
                in_r = &s->mb1->data;
            }

            /* FIXME: max_ch to ch1 */
            if (sf_get_channel(s->isf) > 1) {
                s16_ch2_to_s16_ch1((void*)s->mb2->data, (const void*)in_r[0], nb_isamples);
                /* use mb2, mb1 not used */
                in_r = &s->mb2->data;
            }
        }
        /* 16bit/mono */
        out_r = (in_r == &s->mb1->data) ? &s->mb2->data : &s->mb1->data;
        rc = resx_convert(s->r, (void**)out_r, nb_osamples, (const void**)in_r, nb_isamples);
        CHECK_RET_TAG_WITH_GOTO(rc >= 0, err);
        if (rc == 0) {
            //FIXME: patch for soxr
            LOGI(TAG, "need more samples, rc = %d", rc);
            aos_mutex_unlock(&s->lock);
            return rc;
        }
        tsf         = s->isf_r;
        in_r        = out_r;
        nb_isamples = rc;

        /* B->C: 16bit/mono => osf */
        if (s->ac) {
            out_r = (in_r == &s->mb1->data) ? &s->mb2->data : &s->mb1->data;
            rc = afconv_convert(s->ac, (void**)out_r, nb_osamples, (const void**)in_r, nb_isamples);
            CHECK_RET_TAG_WITH_GOTO(rc > 0, err);
            in_r = out_r;
        }

        /* FIXME: ch1 to ch2 */
        if (sf_get_channel(s->osf) > 1) {
            s16_ch1_to_s16_ch2((void*)out[0], (const void*)in_r[0], nb_isamples);
        } else {
            /* copy inner result to out */
            memcpy(out[0], out_r[0], sizeof(int16_t) * nb_isamples);
        }
    } else {
        /* A->C */
        if (s->ac) {
            out_r = (in_r == &s->mb1->data) ? &s->mb2->data : &s->mb1->data;
            rc = afconv_convert(s->ac, (void**)out_r, nb_osamples, (const void**)in_r, nb_isamples);
            CHECK_RET_TAG_WITH_GOTO(rc > 0, err);
            in_r = out_r;
        }

        /* FIXME: ch1 to ch2 */
        //TODO:   dst only support 16bit/interleave now
        if (sf_get_channel(s->osf) == sf_get_channel(tsf)) {
            /* copy inner result to out */
            memcpy(out[0], out_r[0], sizeof(int16_t) * nb_isamples * sf_get_channel(s->osf));
        } else {
            if (sf_get_channel(s->osf) == 1)
                s16_ch2_to_s16_ch1((void*)out[0], (const void*)in_r[0], nb_isamples);
            else
                s16_ch1_to_s16_ch2((void*)out[0], (const void*)in_r[0], nb_isamples);
        }
        rc = nb_isamples;
    }
    aos_mutex_unlock(&s->lock);

    return rc;
err:
    aos_mutex_unlock(&s->lock);
    return -1;
}

/**
 * @brief  convert a audio avframe from src to dst sample format
 * @param  [in] s
 * @param  [in] iframe
 * @param  [in] oframe
 * @return number of samples output per channel, -1 on error
 */
int swr_convert_frame(swr_t *s, const avframe_t *iframe, avframe_t *oframe)
{
    int rc;

    CHECK_PARAM(s && iframe && oframe && iframe->nb_samples && (iframe->sf == s->isf), -1);
    oframe->sf         = s->osf;
    oframe->nb_samples = swr_get_osamples_max(s, iframe->nb_samples);
    rc = avframe_get_buffer(oframe);
    if (rc < 0) {
        LOGE(TAG, "may be oom, rc = %d", rc);
        return -1;
    }

    rc = swr_convert(s, (void**)&oframe->data, oframe->nb_samples, (const void**)&iframe->data, iframe->nb_samples);
    if (rc >= 0) {
        //FIXME: interleaved output only
        oframe->nb_samples  = rc;
        oframe->linesize[0] = rc * sf_get_frame_size(oframe->sf);
    }

    return rc;
}

/**
 * @brief  free the swreasampler
 * @param  [in] s
 * @return
 */
void swr_free(swr_t *s)
{
    if (s) {
        mblock_free(s->mb1);
        mblock_free(s->mb2);
        afconv_free(s->ac);
        afconv_free(s->ac_r);
        resx_free(s->r);
        aos_mutex_free(&s->lock);
        av_free(s);
    }
}


