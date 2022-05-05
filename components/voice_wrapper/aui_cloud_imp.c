/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <yoc/aui_cloud.h>
#include <string.h>
#include <aos/aos.h>

#define TAG "CLOUD"

aui_t *aui_cloud_init(aui_config_t *config)
{
    aui_t *aui = aos_zalloc_check(sizeof(aui_t));

    if (config) {
        memcpy(&aui->config, config, sizeof(aui_config_t));
    }
    return aui;
}

int aui_cloud_deinit(aui_t *aui)
{
    aos_check_param(aui);
    
    aos_free(aui);

    return 0;
}

int aui_cloud_start_audio(aui_t *aui, int type)
{
    int ret = -1;

    aos_check_param(aui);

    aui->asr_type = type;
    if (aui->ops.asr) {
        if (aui->ops.asr->start)
            ret = aui->ops.asr->start(aui);
    }
    return ret;
}

/**
    Start to input audio buffer to internal buffer
*/
int aui_cloud_push_audio(aui_t *aui, void *data, size_t size)
{
    int ret = -1;

    aos_check_param(aui);
    aos_check_param(data);
    aos_check_param(size > 0);

    if (aui->ops.asr) {
        if (aui->ops.asr->push_data)
            ret = aui->ops.asr->push_data(aui, data, size);
    }

    return ret;
}

/**
    Finished the buffer input
*/
int aui_cloud_stop_audio(aui_t *aui)
{
    int ret = -1;

    aos_check_param(aui);

    if (aui->ops.asr) {
        if (aui->ops.asr->stop_push_data)
            ret = aui->ops.asr->stop_push_data(aui);
    }

    return ret;
}

int aui_cloud_stop(aui_t *aui)
{
    int ret = 0;
    if (aui->ops.asr) {
        if (aui->ops.asr->stop)
            ret = aui->ops.asr->stop(aui);
    }
    if (aui->ops.nlp) {
        if (aui->ops.nlp->stop)
            ret = aui->ops.nlp->stop(aui);
    }
    if (aui->ops.tts) {
        if (aui->ops.tts->stop)
            ret = aui->ops.tts->stop(aui);
    }
    return ret;
}

int aui_cloud_start_nlp(aui_t *aui)
{
    int ret = -1;

    aos_check_param(aui);

    if (aui->ops.nlp && aui->ops.nlp->start)
        ret = aui->ops.nlp->start(aui);
    
    return ret;
}

int aui_cloud_push_text(aui_t *aui, char *text)
{
    int ret = 0;

    aos_check_param(aui);
    aos_check_param(text);

    if (strncasecmp(text, MUSIC_PREFIX, strlen(MUSIC_PREFIX)) == 0) {
        LOGI(TAG, "get music url start");
    } else {
        LOGI(TAG, "nlp start");
        /** process normal NLP */
        if (aui->ops.nlp && aui->ops.nlp->push_text) {
            ret = aui->ops.nlp->push_text(aui, text);
        }
    }

    return ret;
}

int aui_cloud_stop_nlp(aui_t *aui)
{
    int ret = -1;

    aos_check_param(aui);

    if (aui->ops.nlp && aui->ops.nlp->stop)
        ret = aui->ops.nlp->stop(aui);
    
    return ret;    
}

int aui_cloud_start_tts(aui_t *aui)
{
    int ret = -1;

    aos_check_param(aui);

    if (aui->ops.tts && aui->ops.tts->start)
        ret = aui->ops.tts->start(aui);

    return ret;
}

int aui_cloud_req_tts(aui_t *aui, const char *text)
{
    int ret = -1;
    
    aos_check_param(aui);
    aos_check_param(text);

    if (aui->ops.tts && aui->ops.tts->req_tts) {
        ret = aui->ops.tts->req_tts(aui, text);
    }
    return ret;
}

int aui_cloud_stop_tts(aui_t *aui)
{
    int ret = -1;

    aos_check_param(aui);

    if (aui->ops.tts && aui->ops.tts->stop)
        ret = aui->ops.tts->stop(aui);

    return ret;
}

int aui_cloud_asr_unregister(aui_t *aui)
{
    aos_check_param(aui);

    int ret = 0;

    if (aui->ops.asr && aui->ops.asr->deinit) {
        ret = aui->ops.asr->deinit(aui);
    }

    aui->ops.asr = NULL;

    return ret;
}

int aui_cloud_nlp_unregister(aui_t *aui)
{
    aos_check_param(aui);

    int ret = 0;

    if (aui->ops.nlp && aui->ops.nlp->deinit) {
        ret = aui->ops.nlp->deinit(aui);
    }

    aui->ops.nlp = NULL;

    return ret;
}

int aui_cloud_tts_unregister(aui_t *aui)
{
    aos_check_param(aui);

    int ret = 0;

    if (aui->ops.tts && aui->ops.tts->deinit) {
        ret = aui->ops.tts->deinit(aui);
    }

    aui->ops.tts = NULL;

    return ret;
}

int aui_cloud_asr_register(aui_t *aui, aui_asr_cls_t *ops, aui_asr_cb_t cb, void *priv)
{
    aos_check_param(aui && ops && cb);

    aui->ops.asr = ops;
    aui->cb.asr_cb = cb;
    aui->cb.asr_priv = priv;
    
    if (aui->ops.asr->init) {
        return aui->ops.asr->init(aui);
    } else {
        return 0;
    }
}

int aui_cloud_nlp_register(aui_t *aui, aui_nlp_cls_t *ops, aui_nlp_cb_t cb, void *priv)
{
    aos_check_param(aui && ops && cb);

    aui->ops.nlp = ops;
    aui->cb.nlp_cb = cb;
    aui->cb.nlp_priv = priv;
    
    if (aui->ops.nlp->init) {
        return aui->ops.nlp->init(aui);
    } else {
        return 0;
    }
}

int aui_cloud_tts_register(aui_t *aui, aui_tts_cls_t *ops, aui_tts_cb_t cb, void *priv)
{
    aos_check_param(aui && ops && cb);

    aui->ops.tts = ops;
    aui->cb.tts_cb = cb;
    aui->cb.tts_priv = priv;

    if (aui->ops.tts->init) {
        return aui->ops.tts->init(aui);
    } else {
        return 0;
    }
}