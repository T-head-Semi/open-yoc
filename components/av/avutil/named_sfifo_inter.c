/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#ifdef __linux__
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "avutil/url_encode.h"
#include "avutil/named_sfifo_cls.h"

struct nsfifo_inter {
    size_t               len;
    size_t               size;
    uint32_t             ridx;
    uint32_t             widx;
    uint8_t              reof;
    uint8_t              weof;

    pthread_cond_t       wcond;
    pthread_cond_t       rcond;
    pthread_mutex_t      lock;

    int                  ref;
};

struct nsfifo_inter_priv {
    int                  fd;
    uint8_t              *buf;
    char                 *b64_name;
    size_t               share_size;
    struct nsfifo_inter  *fifo_inter;
};

#define TAG                    "nsfifo_inter"

#define lock()   aos_mutex_lock(&fifo_inter->lock, AOS_WAIT_FOREVER);
#define unlock() aos_mutex_unlock(&fifo_inter->lock);

#define is_full(fifo) (fifo_inter->len == fifo_inter->size)
#define is_empty(fifo) (fifo_inter->len == 0)

#define GET_DERIVED_NSFIFO_INTER(p) ((struct nsfifo_inter_priv*)GET_DERIVED_NSFIFO(p))

static int _nsfifo_inter_init(nsfifo_t *fifo, const char *name, int mode)
{
    int rc, fd = -1;
    size_t share_size;
    char *b64_name = NULL;
    size_t src_size = strlen(name);
    size_t dst_size = 2 * src_size + 16;
    struct nsfifo_inter *fifo_inter = NULL;
    struct nsfifo_inter_priv *priv = GET_DERIVED_NSFIFO_INTER(fifo);

    b64_name =(char*)av_zalloc(dst_size);
    rc = url_encode(name, src_size, b64_name, dst_size);
    CHECK_RET_TAG_WITH_GOTO(rc > 0, err);

    if (mode & O_CREAT) {
        pthread_condattr_t cattr;
        pthread_mutexattr_t attr;

        pthread_condattr_init(&cattr);
        pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

        share_size = sizeof(struct nsfifo_inter) + fifo->size;
        /* unlink before */
        shm_unlink(b64_name);
        fd = shm_open(b64_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        CHECK_RET_TAG_WITH_GOTO(fd >= 0, err);

        rc = ftruncate(fd, share_size);
        CHECK_RET_TAG_WITH_GOTO(rc == 0, err);

        fifo_inter = (struct nsfifo_inter*)mmap(NULL, share_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        CHECK_RET_TAG_WITH_GOTO(fifo_inter != MAP_FAILED, err);
        memset(fifo_inter, 0, sizeof(struct nsfifo_inter));
        fifo_inter->size = fifo->size;

        pthread_mutex_init(&fifo_inter->lock, &attr);
        pthread_cond_init(&fifo_inter->wcond, &cattr);
        pthread_cond_init(&fifo_inter->rcond, &cattr);
    } else {
        /* read only */
        fd = shm_open(b64_name, O_RDWR, S_IRUSR | S_IWUSR);
        CHECK_RET_TAG_WITH_GOTO(fd >= 0, err);

        fifo_inter = (struct nsfifo_inter*)mmap(NULL, sizeof(struct nsfifo_inter), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        CHECK_RET_TAG_WITH_GOTO(fifo_inter != MAP_FAILED, err);
        if (!(fifo_inter->size > 0 && fifo_inter->ref > 0)) {
            LOGE(TAG, "share mem error, size = %u, ref = %d", fifo_inter->size, fifo_inter->ref);
            goto err;
        }
        fifo->size = fifo_inter->size;
        share_size = sizeof(struct nsfifo_inter) + fifo->size;
        munmap(fifo_inter, sizeof(struct nsfifo_inter));

        /* map again */
        fifo_inter = (struct nsfifo_inter*)mmap(NULL, share_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        CHECK_RET_TAG_WITH_GOTO(fifo_inter != MAP_FAILED, err);
    }
    fifo_inter->ref++;
    msync((void*)fifo_inter, sizeof(struct nsfifo_inter), MS_SYNC);

    priv->fd         = fd;
    priv->b64_name   = b64_name;
    priv->fifo_inter = fifo_inter;
    priv->share_size = share_size;
    priv->buf        = (uint8_t*)fifo_inter + sizeof(struct nsfifo_inter);

    return 0;
err:
    av_free(b64_name);
    if (fd >= 0)
        close(fd);
    return -1;
}

static int _nsfifo_inter_get_rpos(nsfifo_t *fifo, char **pos, uint32_t timeout)
{
    int rc = 0;
    struct nsfifo_inter_priv *priv  = GET_DERIVED_NSFIFO_INTER(fifo);
    struct nsfifo_inter *fifo_inter = priv->fifo_inter;

    lock();
    if (is_empty(fifo_inter) && (!fifo_inter->weof)) {
        uint64_t nsec;
        struct timespec ts;
        struct timeval now;

        gettimeofday(&now, NULL);
        nsec       = now.tv_usec * 1000 + (timeout % 1000) * 1000000;
        ts.tv_nsec = nsec % 1000000000;
        ts.tv_sec  = now.tv_sec + nsec / 1000000000 + timeout / 1000;
        rc = pthread_cond_timedwait(&fifo_inter->rcond, &fifo_inter->lock, &ts);
        if (rc != 0) {
            unlock();
            /* maybe timeout */
            return -1;
        }
    }

    /* may be reof set, is_empty again */
    if (!is_empty(fifo_inter)) {
        if (fifo_inter->widx > fifo_inter->ridx) {
            rc   = fifo_inter->widx - fifo_inter->ridx;
            *pos = (char*)priv->buf + fifo_inter->ridx;
        } else {
            rc   = fifo_inter->size - fifo_inter->ridx;
            *pos = (char*)priv->buf + fifo_inter->ridx;
        }
    } else {
        //LOGD(TAG, "get rpos. weof = %d, reof = %d size = %u, widx = %d, ridx = %d, len = %d",
        //    fifo_inter->weof, fifo_inter->reof, fifo_inter->size, fifo_inter->widx, fifo_inter->ridx, fifo_inter->len);
    }
    unlock();

    return rc;
}

static int _nsfifo_inter_set_rpos(nsfifo_t* fifo, size_t count)
{
    int rc = -1;
    struct nsfifo_inter_priv *priv  = GET_DERIVED_NSFIFO_INTER(fifo);
    struct nsfifo_inter *fifo_inter = priv->fifo_inter;

    lock();
    if (fifo_inter->ridx + count <= fifo_inter->size) {
        fifo_inter->len  -= count;
        fifo_inter->ridx += count;
        fifo_inter->ridx %= fifo_inter->size;
        pthread_cond_signal(&fifo_inter->wcond);
        msync((void*)fifo_inter, sizeof(struct nsfifo_inter), MS_SYNC);
        rc = 0;
    } else {
        //LOGE(TAG, "set rpos err. count = %u, size = %u, widx = %d, ridx = %d, len = %d",
        //     count, fifo_inter->size, fifo_inter->widx, fifo_inter->ridx, fifo_inter->len);
    }
    unlock();

    return rc;

}

static int _nsfifo_inter_get_wpos(nsfifo_t* fifo, char **pos, uint32_t timeout)
{
    int rc = 0;
    struct nsfifo_inter_priv *priv  = GET_DERIVED_NSFIFO_INTER(fifo);
    struct nsfifo_inter *fifo_inter = priv->fifo_inter;

    lock();
    if (is_full(fifo_inter) && (!fifo_inter->reof)) {
        uint64_t nsec;
        struct timespec ts;
        struct timeval now;

        gettimeofday(&now, NULL);
        nsec       = now.tv_usec * 1000 + (timeout % 1000) * 1000000;
        ts.tv_nsec = nsec % 1000000000;
        ts.tv_sec  = now.tv_sec + nsec / 1000000000 + timeout / 1000;
        rc = pthread_cond_timedwait(&fifo_inter->wcond, &fifo_inter->lock, &ts);
        if (rc != 0) {
            unlock();
            /* maybe timeout */
            return -1;
        }
    }

    /* may be weof set, is_full again */
    if (!is_full(fifo_inter)) {
        if (fifo_inter->widx >= fifo_inter->ridx) {
            rc   = fifo_inter->size - fifo_inter->widx;
            *pos = (char*)priv->buf + fifo_inter->widx;
        } else {
            rc   = fifo_inter->ridx - fifo_inter->widx;
            *pos = (char*)priv->buf + fifo_inter->widx;
        }
    } else {
        //LOGD(TAG, "get wpos. weof = %d, reof = %d size = %u, widx = %d, ridx = %d, len = %d",
        //     fifo_inter->weof, fifo_inter->reof, fifo_inter->size, fifo_inter->widx, fifo_inter->ridx, fifo_inter->len);
    }
    unlock();

    return rc;
}

static int _nsfifo_inter_set_wpos(nsfifo_t* fifo, size_t count)
{
    int rc = -1;
    struct nsfifo_inter_priv *priv  = GET_DERIVED_NSFIFO_INTER(fifo);
    struct nsfifo_inter *fifo_inter = priv->fifo_inter;

    lock();
    if (fifo_inter->widx + count <= fifo_inter->size) {
        fifo_inter->len  += count;
        fifo_inter->widx += count;
        fifo_inter->widx %= fifo_inter->size;
        pthread_cond_signal(&fifo_inter->rcond);
        msync((void*)fifo_inter, sizeof(struct nsfifo_inter), MS_SYNC);
        rc = 0;
    } else {
        //LOGE(TAG, "set wpos err. count = %u, size = %u, widx = %d, ridx = %d, len = %d",
        //     count, fifo_inter->size, fifo_inter->widx, fifo_inter->ridx, fifo_inter->len);
    }
    unlock();

    return rc;
}

static int _nsfifo_inter_set_eof(nsfifo_t* fifo, uint8_t reof, uint8_t weof)
{
    int rc = 0;
    struct nsfifo_inter_priv *priv  = GET_DERIVED_NSFIFO_INTER(fifo);
    struct nsfifo_inter *fifo_inter = priv->fifo_inter;

    lock();
    if (reof) {
        fifo_inter->reof = reof;
        pthread_cond_signal(&fifo_inter->wcond);
    }

    if (weof) {
        fifo_inter->weof = weof;
        pthread_cond_signal(&fifo_inter->rcond);
    }
    msync((void*)fifo_inter, sizeof(struct nsfifo_inter), MS_SYNC);
    unlock();

    return rc;
}

static int _nsfifo_inter_get_eof(nsfifo_t* fifo, uint8_t *reof, uint8_t *weof)
{
    int rc = 0;
    struct nsfifo_inter_priv *priv  = GET_DERIVED_NSFIFO_INTER(fifo);
    struct nsfifo_inter *fifo_inter = priv->fifo_inter;

    lock();
    if (reof)
        *reof = fifo_inter->reof;

    if (weof)
        *weof = fifo_inter->weof;
    unlock();

    return rc;
}

static int _nsfifo_inter_reset(nsfifo_t *fifo)
{
    int rc = 0;
    struct nsfifo_inter_priv *priv  = GET_DERIVED_NSFIFO_INTER(fifo);
    struct nsfifo_inter *fifo_inter = priv->fifo_inter;

    lock();
    fifo_inter->len  = 0;
    fifo_inter->ridx = 0;
    fifo_inter->widx = 0;
    fifo_inter->reof = 0;
    fifo_inter->weof = 0;
    msync((void*)fifo_inter, sizeof(struct nsfifo_inter), MS_SYNC);
    unlock();

    return rc;
}

static int _nsfifo_inter_get_len(nsfifo_t *fifo)
{
    int rc = 0;
    struct nsfifo_inter_priv *priv  = GET_DERIVED_NSFIFO_INTER(fifo);
    struct nsfifo_inter *fifo_inter = priv->fifo_inter;

    lock();
    rc = fifo_inter->len;
    unlock();

    return rc;
}

static void _nsfifo_inter_uninit(nsfifo_t *fifo)
{
    struct nsfifo_inter_priv *priv  = GET_DERIVED_NSFIFO_INTER(fifo);
    struct nsfifo_inter *fifo_inter = priv->fifo_inter;

    lock();
    fifo_inter->ref--;
    if (0 == fifo_inter->ref) {
        unlock();

        pthread_mutex_destroy(&fifo_inter->lock);
        pthread_cond_destroy(&fifo_inter->wcond);
        pthread_cond_destroy(&fifo_inter->rcond);

        munmap(fifo_inter, priv->share_size);
        shm_unlink(priv->b64_name);
    } else {
        msync((void*)fifo_inter, sizeof(struct nsfifo_inter), MS_SYNC);
        unlock();
        munmap(fifo_inter, priv->share_size);
    }

    av_free(priv->b64_name);
    close(priv->fd);
}

static const struct nsfifo_ops nsfifo_ops_ififo = {
    .init            = _nsfifo_inter_init,
    .get_rpos        = _nsfifo_inter_get_rpos,
    .set_rpos        = _nsfifo_inter_set_rpos,
    .get_wpos        = _nsfifo_inter_get_wpos,
    .set_wpos        = _nsfifo_inter_set_wpos,
    .get_eof         = _nsfifo_inter_get_eof,
    .set_eof         = _nsfifo_inter_set_eof,
    .reset           = _nsfifo_inter_reset,
    .get_len         = _nsfifo_inter_get_len,
    .uninit          = _nsfifo_inter_uninit,
};

const struct nsfifo_cls nsfifo_cls_ififo = {
    .name            = "ififo",
    .priv_size       = sizeof(struct nsfifo_inter_priv),
    .ops             = &nsfifo_ops_ififo,
};
#endif



