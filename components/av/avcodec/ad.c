/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/misc.h"
#include "avcodec/ad.h"

#define TAG    "ad"

static struct {
    int                 cnt;
    const struct ad_ops *ops[AD_OPS_MAX];
} g_aders;

#if 0
static const struct ad_ops* _get_ad_ops_by_name(const char* name)
{
    int i;

    for (i = 0; i < g_aders.cnt; i++) {
        if (strcmp(g_aders.ops[i]->name, name) == 0) {
            return g_aders.ops[i];
        }
    }

    LOGE(TAG, "get ad ops err, name = %s\n", name);

    return NULL;
}
#endif

static const struct ad_ops* _get_ad_ops_by_id(avcodec_id_t id)
{
    int i;

    for (i = 0; i < g_aders.cnt; i++) {
        if (g_aders.ops[i]->id & id) {
            return g_aders.ops[i];
        }
    }

    LOGE(TAG, "get ad ops err, id = %u", id);

    return NULL;
}

/**
 * @brief  regist ad ops
 * @param  [in] ops
 * @return 0/-1
 */
int ad_ops_register(const struct ad_ops *ops)
{
    int i;

    if (ops && (g_aders.cnt < AD_OPS_MAX)) {
        for (i = 0; i < g_aders.cnt; i++) {
            if (strcmp(ops->name, g_aders.ops[i]->name) == 0) {
                /* replicate  */
                break;
            }
        }

        if (i == g_aders.cnt) {
            g_aders.ops[g_aders.cnt] = ops;
            g_aders.cnt++;
        }
        return 0;
    }

    LOGE(TAG, "ad ops regist fail");
    return -1;
}

/**
* @brief  init the ad config param
* @param  [in] ad_cnf
* @return 0/-1
*/
int ad_conf_init(ad_conf_t *ad_cnf)
{
    //TODO:
    CHECK_PARAM(ad_cnf, -1);
    memset(ad_cnf, 0, sizeof(ad_conf_t));
    return 0;
}

/**
 * @brief  open/create one audio decoder
 * @param  [in] id
 * @param  [in] ad_cnf
 * @return NULL on err
 */
ad_cls_t* ad_open(avcodec_id_t id, const ad_conf_t *ad_cnf)
{
    int ret;
    ad_cls_t *o = NULL;
    uint8_t  *data = NULL;

    CHECK_PARAM(id && ad_cnf, NULL);
    o = av_zalloc(sizeof(ad_cls_t));
    CHECK_RET_TAG_WITH_GOTO(o, err);

    o->ops = _get_ad_ops_by_id(id);
    if (!o->ops) {
        LOGE(TAG, "can't find suitable decode type. id = %d, name = %s", id, get_codec_name(id));
        goto err;
    }

    if (ad_cnf->extradata_size) {
        data = av_zalloc(ad_cnf->extradata_size);
        CHECK_RET_TAG_WITH_GOTO(data, err);
        memcpy(data, ad_cnf->extradata, ad_cnf->extradata_size);
        o->ash.extradata = data;
        o->ash.extradata_size = ad_cnf->extradata_size;
    }
    o->ash.id          = id;
    o->ash.sf          = ad_cnf->sf;
    o->ash.block_align = ad_cnf->block_align;
    o->ash.bps         = ad_cnf->bps;
    LOGI(TAG, "find a decode, name = %s, id = %d", o->ops->name, id);

    ret = o->ops->open ? o->ops->open(o) : -1;
    CHECK_RET_TAG_WITH_GOTO(ret != -1, err);
    aos_mutex_new(&o->lock);

    return o;
err:
    av_free(data);
    av_free(o);
    return NULL;
}

/**
 * @brief  decode one audio frame
 * @param  [in] o
 * @param  [out] frame : frame of pcm
 * @param  [out] got_frame : whether decode one frame
 * @param  [in]  pkt : data of encoded audio
 * @return -1 when err happens, otherwise the number of bytes consumed from the
 *         input avpacket is returned
 */
int ad_decode(ad_cls_t *o, avframe_t *frame, int *got_frame, const avpacket_t *pkt)
{
    int ret = -1;
    int frame_size;

    if (!(o && frame && got_frame && pkt && pkt->data && pkt->size)) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return -1;
    }

    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    *got_frame = 0;
    ret = o->ops->decode(o, frame, got_frame, pkt);
    if (ret <= 0) {
        LOGD(TAG, "decode fail, ret = %d", ret);
        goto quit;
    }

    if (*got_frame) {
        frame_size = sf_get_frame_size(o->ash.sf);
        if ((frame->linesize[0] % frame_size) != 0) {
            LOGE(TAG, "read err. ret = %d, frame size = %d", ret, frame_size);
            ret = -1;
            goto quit;
        }
    }

quit:
    aos_mutex_unlock(&o->lock);
    return ret;
}

/**
 * @brief  control a audio decoder
 * @param  [in] o
 * @param  [in] cmd : command
 * @param  [in] arg
 * @param  [in/out] arg_size
 * @return 0/-1
 */
int ad_control(ad_cls_t *o, int cmd, void *arg, size_t *arg_size)
{
    int ret = -1;

    CHECK_PARAM(o, -1);
    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    ret = o->ops->control ? o->ops->control(o, cmd, arg, arg_size) : -1;
    aos_mutex_unlock(&o->lock);

    return ret;
}

/**
 * @brief  reset the audio decoder
 * @param  [in] o
 * @return 0/-1
 */
int ad_reset(ad_cls_t *o)
{
    int ret = -1;

    CHECK_PARAM(o, -1);
    aos_mutex_lock(&o->lock, AOS_WAIT_FOREVER);
    ret = o->ops->reset ? o->ops->reset(o) : 0;
    aos_mutex_unlock(&o->lock);

    return ret;
}

/**
 * @brief  close/destroy audio decoder
 * @param  [in] o
 * @return 0/-1
 */
int ad_close(ad_cls_t *o)
{
    int ret = -1;

    CHECK_PARAM(o, -1);
    ret = o->ops->close(o);

    aos_mutex_free(&o->lock);
    av_free(o->ash.extradata);
    av_free(o);

    return ret;
}


