/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/avframe.h"

/**
 * @brief  allocate one frame
 * @return NULL on err
 */
avframe_t *avframe_alloc()
{
    avframe_t *frame = av_zalloc(sizeof(avframe_t));

    return frame;
}

/**
 * @brief  clear nb_samples, sf, linesize, etc. mpool & data point reserved otherwise.
 * @param  [in] frame
 * @return
 */
void avframe_clear(avframe_t *frame)
{
    if (frame) {
        frame->type       = AVMEDIA_TYPE_UNKNOWN;
        frame->sf         = 0;
        frame->nb_samples = 0;
        frame->moffset    = 0;
        memset(frame->linesize, 0, sizeof(frame->linesize));
    }
}

/**
 * @brief  free res of the frame
 * @param  [in] frame
 * @return
 */
void avframe_free(avframe_t **frame)
{
    if (frame && *frame) {
        avframe_t *pframe = *frame;

        if (!pframe->mpool) {
            int i;
            /* FIXME: innner malloc */
            for (i = 0; i < AV_DATA_POINTERS_MAX; i++) {
                void *data = pframe->data[i];

                if (data)
                    av_free(data);
            }
        }
        av_freep((char**)frame);
    }
}

/**
 * @brief  get/alloc data buffer of the frame by sf and nb_samples
 * @param  [in] frame
 * @return 0/-1
 */
int avframe_get_buffer(avframe_t *frame)
{
    int rc = -1;
    int channel, linesize;

    if (!frame) {
        return rc;
    }

    channel = sf_get_channel(frame->sf);
    if (!((channel > 0) && (frame->nb_samples))) {
        return rc;
    }

    rc = sf_get_buffer_size(&linesize, frame->sf, frame->nb_samples);
    if (rc <= 0) {
        return -1;
    }

    if (frame->capsize[0] >= linesize) {
        frame->linesize[0] = linesize;
        /* no need malloc */
        return 0;
    }

    if (frame->mpool && frame->msize >= linesize) {
        frame->data[0]     = frame->mpool;
        frame->linesize[0] = linesize;
        frame->moffset     = linesize;
        return 0;
    } else {
        void *data = av_realloc(frame->data[0], linesize);

        if (data) {
            frame->data[0]     = data;
            frame->linesize[0] = linesize;
            frame->capsize[0]  = linesize;
            return 0;
        }
    }

    return -1;
}

/**
 * @brief  copy the frame filled from one to another
 * @param  [in] from
 * @param  [in] to
 * @return 0/-1
 */
int avframe_copy_from(const avframe_t *from, avframe_t *to)
{
    int rc = -1;
    int f_linesize;

    if (!(from && to && from->data[0] && from->linesize[0] && (from->data[0] != to->data[0]))) {
        return rc;
    }

    f_linesize = from->linesize[0];
    if (to->mpool) {
        if (f_linesize <= to->msize) {
            to->sf          = from->sf;
            to->nb_samples  = from->nb_samples;
            to->data[0]     = to->mpool;
            to->linesize[0] = f_linesize;
            memcpy(to->data[0], from->data[0], f_linesize);
            rc = 0;
        }
    } else {
        //FIXME:
        if (f_linesize > to->capsize[0]) {
            void *data = av_realloc(to->data[0], f_linesize);
            if (!data) {
                return -1;
            }
            to->data[0]    = data;
            to->capsize[0] = f_linesize;
        }

        to->sf          = from->sf;
        to->nb_samples  = from->nb_samples;
        to->linesize[0] = f_linesize;
        memcpy(to->data[0], from->data[0], f_linesize);
        rc = 0;
    }

    return rc;
}

/**
 * @brief  set memory pool of the frame, the data source is from the mem pool
 * @param  [in] frame
 * @param  [in] buf   : ptr of the memory pool
 * @param  [in] size  : size of the memory pool
 * @return 0/-1
 */
int avframe_set_mempool(avframe_t *frame, uint8_t *buf, size_t size)
{
    int rc = -1;

    if (!(frame && buf && size)) {
        return rc;
    }

    frame->mpool = buf;
    frame->msize = size;
    return 0;
}

