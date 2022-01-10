/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include <stdarg.h>
#include "avutil/named_straightfifo.h"
#include "avutil/named_sfifo_cls.h"

#define lock() aos_mutex_lock(&fifo->lock, AOS_WAIT_FOREVER);
#define unlock() aos_mutex_unlock(&fifo->lock);

#define TAG                    "nsfifo"

/**
 * @brief  open or create a named fifo
 * @param  [in] name
 * @param  [in] mode : O_CREAT/O_RDONLY
 * @param  [in] ...  : if mode == O_CREAT, params size is needed
 * @param              size : size of the straight fifo
 * @return NULL on err
 */
nsfifo_t* nsfifo_open(const char *name, int mode, ...)
{
    int rc;
    va_list ap;
    size_t size = 0;
    nsfifo_t *fifo = NULL;
    struct nsfifo_cls *cls = NULL;

    CHECK_PARAM(name && strlen(name), NULL);
    if (mode & O_CREAT) {
        va_start(ap, mode);
        size = va_arg(ap, size_t);
        va_end(ap);

        if (!size) {
            LOGE(TAG, "param error!");
            return NULL;
        }
    }

    if (strncasecmp(name, "fifo", 4) == 0) {
        extern struct nsfifo_cls nsfifo_cls_fifo;

        cls = &nsfifo_cls_fifo;
        fifo = aos_zalloc(sizeof(nsfifo_t) + cls->priv_size);
#ifdef __linux__
    } else if (strncasecmp(name, "ififo", 4) == 0) {
        extern struct nsfifo_cls nsfifo_cls_ififo;

        cls = &nsfifo_cls_ififo;
        fifo = aos_zalloc(sizeof(nsfifo_t) + cls->priv_size);
#endif
    }

    if (!fifo) {
        LOGE(TAG, "param error, may be oom or not support, name = %s", name);
        return NULL;
    }

    fifo->cls  = cls;
    fifo->size = size;
    rc = fifo->cls->ops->init(fifo, name, mode);
    if (rc) {
        LOGE(TAG, "error. fifo init fail!");
        av_free(fifo);
        return NULL;
    }

    fifo->name = strdup(name);
    aos_mutex_new(&fifo->lock);

    return fifo;
}

/**
 * @brief  get straight read pos
 * @param  [in] fifo
 * @param  [out] pos
 * @param  [in] timeout : ms
 * @return straight read size
 */
int nsfifo_get_rpos(nsfifo_t* fifo, char **pos, uint32_t timeout)
{
    int rc;

    CHECK_PARAM(fifo && pos, -1);
    lock();
    rc = fifo->cls->ops->get_rpos(fifo, pos, timeout);
    unlock();

    return rc;
}

/**
 * @brief  set the read pos after read
 * @param  [in] fifo
 * @param  [in] count
 * @return 0/-1
 */
int nsfifo_set_rpos(nsfifo_t* fifo, size_t count)
{
    int rc;

    CHECK_PARAM(fifo && count, -1);
    lock();
    rc = fifo->cls->ops->set_rpos(fifo, count);
    unlock();

    return rc;
}

/**
 * @brief  get straight write pos
 * @param  [in] fifo
 * @param  [out] pos
 * @param  [in] timeout : ms
 * @return straight write size
 */
int nsfifo_get_wpos(nsfifo_t* fifo, char **pos, uint32_t timeout)
{
    int rc;

    CHECK_PARAM(fifo && pos, -1);
    lock();
    rc = fifo->cls->ops->get_wpos(fifo, pos, timeout);
    unlock();

    return rc;
}

/**
 * @brief  set the write pos after write
 * @param  [in] fifo
 * @param  [in] count
 * @return 0/-1
 */
int nsfifo_set_wpos(nsfifo_t* fifo, size_t count)
{
    int rc;

    CHECK_PARAM(fifo && count, -1);
    lock();
    rc = fifo->cls->ops->set_wpos(fifo, count);
    unlock();

    return rc;
}

/**
 * @brief  set eof to the fifo before destroy, may be block if don't call this fun
 * @param  [in] fifo
 * @param  [in] reof : nsfifo_get_rpos will return immediately when set
 * @param  [in] weof : nsfifo_get_wpos will return immediately when set
 * @return 0/-1
 */
int nsfifo_set_eof(nsfifo_t* fifo, uint8_t reof, uint8_t weof)
{
    int rc;

    CHECK_PARAM(fifo, -1);
    lock();
    rc = fifo->cls->ops->set_eof(fifo, reof, weof);
    unlock();

    return rc;
}

/**
 * @brief  get eof flag from the fifo
 * @param  [in] fifo
 * @param  [out] reof : read eof flag
 * @param  [out] weof : write eof flag
 * @return 0/-1
 */
int nsfifo_get_eof(nsfifo_t* fifo, uint8_t *reof, uint8_t *weof)
{
    int rc;

    CHECK_PARAM(fifo, -1);
    lock();
    rc = fifo->cls->ops->get_eof(fifo, reof, weof);
    unlock();

    return rc;
}

/**
 * @brief  reset the named fifo
 * @param  [in] fifo
 * @return 0/-1
 */
int nsfifo_reset(nsfifo_t *fifo)
{
    int rc;

    CHECK_PARAM(fifo, -1);
    lock();
    rc = fifo->cls->ops->reset(fifo);
    unlock();

    return rc;
}

/**
 * @brief  get valid data len of the named fifo
 * @param  [in] fifo
 * @return -1 on error
 */
int nsfifo_get_len(nsfifo_t *fifo)
{
    int rc;

    CHECK_PARAM(fifo, -1);
    lock();
    rc = fifo->cls->ops->get_len(fifo);
    unlock();

    return rc;
}

/**
 * @brief  close the fifo
 * @param  [in] fifo
 * @return 0/-1
 */
int nsfifo_close(nsfifo_t *fifo)
{
    int rc = 0;

    CHECK_PARAM(fifo, -1);
    fifo->cls->ops->uninit(fifo);
    aos_mutex_free(&fifo->lock);
    av_free(fifo->name);
    av_free(fifo);

    return rc;
}


