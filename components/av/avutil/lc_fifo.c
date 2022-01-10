/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/lc_fifo.h"

#define TAG                    "lcfifo"

/**
 * @brief  create circle fifo with lock
 * @param  [in] len : length of the circle fifo
 * @return
 */
lcfifo_t* lcfifo_create(size_t len)
{
    char *buf = NULL;
    lcfifo_t *fifo = NULL;

    if (len == 0) {
        return NULL;
    }

    buf  = (char*)av_zalloc(len);
    fifo =(lcfifo_t*)av_zalloc(sizeof(lcfifo_t));
    if (!(buf && fifo)) {
        LOGE(TAG, "oom");
        goto err;
    }

    if (0 != ringbuffer_create(&fifo->rb, buf, len)) {
        LOGE(TAG, "rb create fail");
        goto err;
    }
    fifo->buf = buf;
    aos_mutex_new(&fifo->lock);

    return fifo;

err:
    av_free(buf);
    av_free(fifo);
    return NULL;
}

/**
 * @brief  destroy the lcfifo
 * @param  [in] fifo
 * @return
 */
int lcfifo_destroy(lcfifo_t* fifo)
{
    if (!fifo) {
        return -1;
    }

    ringbuffer_destroy(&fifo->rb);
    aos_mutex_free(&fifo->lock);
    av_free(fifo->buf);
    av_free(fifo);

    return 0;
}

/**
 * @brief  read
 * @param  [in] fifo
 * @param  [in] data
 * @param  [in] size
 * @return real read size
 */
int lcfifo_read(lcfifo_t* fifo, uint8_t *data, size_t size)
{
    int len;

    if (!(fifo && data && size)) {
        return -1;
    }

    aos_mutex_lock(&fifo->lock, AOS_WAIT_FOREVER);
    len = ringbuffer_read(&fifo->rb, data, size);
    aos_mutex_unlock(&fifo->lock);

    return len;
}

/**
 * @brief  write
 * @param  [in] fifo
 * @param  [in] data
 * @param  [in] size
 * @return real write size
 */
int lcfifo_write(lcfifo_t* fifo, const uint8_t *data, size_t size)
{
    int len;

    if (!(fifo && data && size)) {
        return -1;
    }

    aos_mutex_lock(&fifo->lock, AOS_WAIT_FOREVER);
    len = ringbuffer_write(&fifo->rb, (uint8_t*)data, size);
    aos_mutex_unlock(&fifo->lock);

    return len;
}

/**
 * @brief  is full of the fifo
 * @param  [in] fifo
 * @return
 */
int lcfifo_is_full(lcfifo_t* fifo)
{
    int ret;

    if (!fifo) {
        return -1;
    }

    aos_mutex_lock(&fifo->lock, AOS_WAIT_FOREVER);
    ret = ringbuffer_full(&fifo->rb);
    aos_mutex_unlock(&fifo->lock);

    return ret;
}

/**
 * @brief  is empty of the fifo
 * @param  [in] fifo
 * @return
 */
int lcfifo_is_empty(lcfifo_t* fifo)
{
    int ret;

    if (!fifo) {
        return -1;
    }

    aos_mutex_lock(&fifo->lock, AOS_WAIT_FOREVER);
    ret = ringbuffer_empty(&fifo->rb);
    aos_mutex_unlock(&fifo->lock);

    return ret;
}

/**
 * @brief  get avaliable read len
 * @param  [in] fifo
 * @return
 */
int lcfifo_get_rlen(lcfifo_t* fifo)
{
    int ret;

    if (!fifo) {
        return -1;
    }

    aos_mutex_lock(&fifo->lock, AOS_WAIT_FOREVER);
    ret = ringbuffer_available_read_space(&fifo->rb);
    aos_mutex_unlock(&fifo->lock);

    return ret;
}

/**
 * @brief  get avaliable write len
 * @param  [in] fifo
 * @return
 */
int lcfifo_get_wlen(lcfifo_t* fifo)
{
    int ret;

    if (!fifo) {
        return -1;
    }

    aos_mutex_lock(&fifo->lock, AOS_WAIT_FOREVER);
    ret = ringbuffer_available_write_space(&fifo->rb);
    aos_mutex_unlock(&fifo->lock);

    return ret;
}


