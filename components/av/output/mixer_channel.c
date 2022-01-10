/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "output/mixer.h"

#define TAG    "mixer_cnl"

#define cnl_lock()   (aos_mutex_lock(&cnl->lock, AOS_WAIT_FOREVER))
#define cnl_unlock() (aos_mutex_unlock(&cnl->lock))

/**
 * @brief  create one mixer-channel
 * @param  [in] sf
 * @param  [in] fsize : fifo size. 8 align
 * @return NULL on error
 */
mixer_cnl_t* mixer_cnl_new(sf_t sf, size_t fsize)
{
    mixer_cnl_t *cnl;
    sfifo_t *fifo = NULL;

    CHECK_PARAM(sf && fsize && (fsize % 8 == 0), NULL);
    cnl = av_zalloc(sizeof(mixer_cnl_t));
    CHECK_RET_TAG_WITH_RET(cnl, NULL);
    fifo = sfifo_create(fsize);
    CHECK_RET_TAG_WITH_GOTO(fifo, err);

    cnl->sf     = sf;
    cnl->fifo   = fifo;
    cnl->status = MIXER_CNL_STATUS_RUNING;
    aos_mutex_new(&cnl->lock);

    return cnl;
err:
    av_free(cnl);
    return NULL;
}

/**
 * @brief  write pcm data to the channel
 * @param  [in] cnl
 * @param  [in] buf
 * @param  [in] size
 * @param  [in] timeout : ms. 0 means no block, -1 means wait-forever
 * @return -1 on error
 */
int mixer_cnl_write(mixer_cnl_t *cnl, uint8_t *buf, size_t size, uint32_t timeout)
{
    char *pos;
    sfifo_t *fifo;
    uint8_t reof = 0;
    int rc = 0, wlen, remain = size;

    CHECK_PARAM(cnl && buf && size, -1);
    fifo = cnl->fifo;
    cnl_lock();
    while (rc != size) {
        wlen = sfifo_get_wpos(fifo, &pos, timeout);
        sfifo_get_eof(fifo, &reof, NULL);
        if (wlen <= 0 || reof) {
            LOGD(TAG, "cnl write break. wlen = %d, reof = %d", wlen, reof);
            break;
        }

        wlen = wlen > remain ? remain : wlen;
        memcpy(pos, buf + rc, wlen);
        sfifo_set_wpos(fifo, wlen);
#ifdef __linux__
        pthread_cond_signal(&cnl->mixer->cond);
#else
        aos_event_set(&cnl->mixer->evt, MIXER_READ_EVENT, AOS_EVENT_OR);
#endif
        rc     += wlen;
        remain -= wlen;
    }
    cnl_unlock();

    return rc;
}

/**
 * @brief  get pcm size in the channel, not mix yet
 * @param  [in] cnl
 * @return -1 on error
 */
int mixer_cnl_get_size(mixer_cnl_t *cnl)
{
    int rc;

    CHECK_PARAM(cnl, -1);
    cnl_lock();
    rc = sfifo_get_len(cnl->fifo);
    cnl_unlock();

    return rc;
}

/**
 * @brief  destroy the channel. need dettach from the mixer before destroy
 * @param  [in] cnl
 * @return 0/-1
 */
int mixer_cnl_free(mixer_cnl_t *cnl)
{
    CHECK_PARAM(cnl, -1);
    if (cnl->mixer) {
        LOGE(TAG, "not dettach yet! mixer = %p", cnl->mixer);
        return -1;
    }

    sfifo_destroy(cnl->fifo);
    aos_mutex_free(&cnl->lock);
    av_free(cnl);

    return 0;
}


