/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avfilter/avfilter.h"

#define TAG                   "filter"

/**
 * @brief  init inner resource of the avfilter
 * @param  [in] inst_name : the name of avfilter_t instance
 * @param  [in] avf
 * @return 0/-1
 */
int avf_init(const char *inst_name, avfilter_t *avf)
{
    char *iname      = NULL;
    avframe_t *frame = NULL;

    CHECK_PARAM(inst_name && avf, -1);
    iname = strdup(inst_name);
    CHECK_RET_TAG_WITH_GOTO(iname, err);
    frame = avframe_alloc();
    CHECK_RET_TAG_WITH_GOTO(frame, err);

    avf->oframe    = frame;
    avf->inst_name = iname;
    aos_mutex_new(&avf->lock);

    return 0;
err:
    av_free(iname);
    avframe_free(&frame);
    return -1;
}

/**
 * @brief  bypass enable
 * @param  [in] avf
 * @param  [in] bypass : 0 or 1
 * @return 0/-1
 */
int avf_set_bypass(avfilter_t *avf, int bypass)
{
    CHECK_PARAM(avf, -1);
    aos_mutex_lock(&avf->lock, AOS_WAIT_FOREVER);
    avf->bypass = bypass;
    aos_mutex_unlock(&avf->lock);

    return 0;
}

/**
 * @brief  control the filter
 * @param  [in] avf
 * @param  [in] cmd          : command id of the filter
 * @param  [in] arg
 * @param  [in/out] arg_size
 * @return 0/-1
 */
int avf_control(avfilter_t *avf, int cmd, void *arg, size_t *arg_size)
{
    int rc = -1;

    CHECK_PARAM(avf && arg && arg_size && cmd, -1);
    aos_mutex_lock(&avf->lock, AOS_WAIT_FOREVER);
    rc = avf->ops->control ? avf->ops->control(avf, cmd, arg, arg_size) : -1;
    aos_mutex_unlock(&avf->lock);

    return rc;
}

/**
 * @brief  link the two filters, to make a filter-chain
 * @param  [in] src : source filter
 * @param  [in] dst : the next/sink filter
 * @return 0/-1
 */
int avf_link(avfilter_t *src, avfilter_t *dst)
{
    CHECK_PARAM(src && dst, -1);
    src->next = dst;
    dst->prev = src;

    return 0;
}

/**
 * @brief  link the tail to head filter
 * @param  [in] head : head of the filter chain
 * @param  [in] tail
 * @return 0/-1
 */
int avf_link_tail(avfilter_t *head, avfilter_t *tail)
{
    avfilter_t *p;

    CHECK_PARAM(head && tail, -1);
    p = head;
    while (p->next) {
        p = p->next;
    }
    p->next    = tail;
    tail->prev = p;

    return 0;
}

/**
 * @brief  filter frame over the avfilter or filter-chain(if had linked)
 * @param  [in] avf : the avfilter
 * @param  [in] in
 * @param  [in] out
 * @return number of samples output per channel, -1 on error
 */
int avf_filter_frame(avfilter_t *avf, const avframe_t *in, avframe_t *out)
{
    int rc = -1;
    avframe_t *oframe;

    CHECK_PARAM(avf && in && out, -1);
    aos_mutex_lock(&avf->lock, AOS_WAIT_FOREVER);
    oframe = AVF_IS_SINK(avf) ? out : avf->oframe;
    avframe_clear(oframe);

    if (AVF_IS_SINK(avf)) {
        if (avf->bypass) {
            rc = avframe_copy_from(in, oframe);
            if (rc < 0) {
                LOGE(TAG, "may be oom, rc = %d, filter name = %s", rc, avf->inst_name);
                goto quit;
            }
            rc = oframe->nb_samples;
        } else {
            rc = avf->ops->filter_frame(avf, in, oframe);
        }
    } else {
        if (avf->bypass) {
            rc = avf_filter_frame(avf->next, in, out);
        } else {
            rc = avf->ops->filter_frame(avf, in, oframe);
            rc = rc > 0 ? avf_filter_frame(avf->next, (const avframe_t*)oframe, out) : rc;
        }
    }
quit:
    aos_mutex_unlock(&avf->lock);

    return rc;
}

/**
* @brief  free inner resource of the avfilter
* @param  [in] avf
* @return 0/-1
*/
int avf_uninit(avfilter_t *avf)
{
    int rc = -1;

    CHECK_PARAM(avf, -1);
    avframe_free(&avf->oframe);
    aos_mutex_free(&avf->lock);
    av_free(avf->inst_name);

    return rc;
}

/**
* @brief  close & free the avfilter
* @param  [in] avf
* @return 0/-1
*/
int avf_close(avfilter_t *avf)
{
    int rc = -1;

    CHECK_PARAM(avf, -1);
    aos_mutex_lock(&avf->lock, AOS_WAIT_FOREVER);
    rc = avf->ops->uninit ? avf->ops->uninit(avf) : 0;
    aos_mutex_unlock(&avf->lock);

    avf_uninit(avf);
    av_free(avf);

    return rc;
}

/**
* @brief  close the filter-chain
* @param  [in] avf : header avfilter of the filter-chain
* @return 0/-1
*/
int avf_chain_close(avfilter_t *avf)
{
    int rc = -1;
    avfilter_t *next, *filter;

    CHECK_PARAM(avf, -1);
    if (!AVF_IS_SRC(avf)) {
        LOGE(TAG, "the head avfilter-chain is not a source filter !");
        return -1;
    }

    filter = avf;
    while (filter) {
        next    = filter->next;
        rc     |= avf_close(filter);
        filter  = next;
    }

    return rc;
}




