/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/named_fifo.h"
#include <aos/list.h>

struct named_fifo {
    char               name[32];
    int                ref;
    lcfifo_t           *lcfifo;
    int                write_eof;
    int                read_eof;
    aos_mutex_t        lock;
    dlist_t            node;
};

static struct {
    int                init;
    dlist_t            head;
    aos_mutex_t        lock;
} g_fifo_list;

#define lock() aos_mutex_lock(&fifo->lock, AOS_WAIT_FOREVER);
#define unlock() aos_mutex_unlock(&fifo->lock);

#define TAG                    "namedfifo"

#if 0
static named_fifo_t* _get_named_fifo(const char *name)
{
    int find = 0;
    named_fifo_t *fifo = NULL;
    dlist_t *node = &g_fifo_list.head;

    aos_mutex_lock(&g_fifo_list.lock, AOS_WAIT_FOREVER);
    for (node = node->next; node != &named_fifo_list; node = node->next) {
        fifo = list_entry(node, named_fifo_t, node);
        if (!strcmp(fifo->name, name)) {
            find = 1;
            break;
        }
    }
    aos_mutex_unlock(&g_fifo_list.lock);

    return find ? fifo : NULL;
}

static int _add_named_fifo(const named_fifo_t *fifo)
{
    int find = 0;
    named_fifo_t *fifo = NULL;
    dlist_t *node = &named_fifo_list;

    aos_mutex_lock(&g_fifo_list.lock, AOS_WAIT_FOREVER);
    for (node = node->next; node != &named_fifo_list; node = node->next) {
        fifo = list_entry(node, named_fifo_t, node);
        if (!strcmp(fifo->name, name)) {
            find = 1;
            break;
        }
    }

    if (!find) {
        dlist_add_tail(&fifo->node, &g_fifo_list.head);
    }
    aos_mutex_unlock(&g_fifo_list.lock);

    return find ? 0 : -1;
}
#endif

/**
 * @brief  open or create the fifo
 * @param  [in] name : the name of fifo
 * @param  [in] fifo_size : default len is NAMED_FIFO_SIZE_DEFAULT when fifo_size = 0
 * @return
 */
named_fifo_t *named_fifo_open(const char *name, size_t fifo_size)
{
    named_fifo_t *fifo = NULL;
    lcfifo_t    *lcfifo = NULL;
    dlist_t *node = &g_fifo_list.head;

    if (!(name && (strlen(name) < NAMED_FIFO_NAME_SIZE_MAX))) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return NULL;
    }

    if (!g_fifo_list.init) {
        dlist_init(&g_fifo_list.head);
        aos_mutex_new(&g_fifo_list.lock);
        g_fifo_list.init = 1;
    }

    aos_mutex_lock(&g_fifo_list.lock, AOS_WAIT_FOREVER);
    for (node = node->next; node != &g_fifo_list.head; node = node->next) {
        fifo = list_entry(node, named_fifo_t, node);
        if (!strcmp(fifo->name, name)) {
            lock();
            fifo->ref++;
            unlock();
            aos_mutex_unlock(&g_fifo_list.lock);

            return fifo;
        }
    }

    fifo_size = (0 == fifo_size) ? NAMED_FIFO_SIZE_DEFAULT : fifo_size;
    lcfifo = lcfifo_create(fifo_size);
    if (!lcfifo) {
        LOGE(TAG, "oom1");
        return NULL;
    }

    fifo =(named_fifo_t*)av_zalloc(sizeof(named_fifo_t));
    if (!fifo) {
        LOGE(TAG, "oom2");
        goto err;
    }

    fifo->ref        = 1;
    fifo->lcfifo     = lcfifo;
    aos_mutex_new(&fifo->lock);
    snprintf(fifo->name, sizeof(fifo->name), "%s", name);

    dlist_add_tail(&fifo->node, &g_fifo_list.head);
    aos_mutex_unlock(&g_fifo_list.lock);

    return fifo;

err:
    lcfifo_destroy(lcfifo);

    return NULL;
}

/**
 * @brief  write data
 * @param  [in] fifo
 * @param  [in] data
 * @param  [in] size
 * @return real size or -1 when err
 */
int named_fifo_write(named_fifo_t *fifo, const uint8_t *data, size_t size)
{
    int ret = 0;

    if (!(fifo && data && size)) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return -1;
    }
    lock();
    ret = lcfifo_write(fifo->lcfifo, data, size);
    unlock();

    return ret;
}

/**
 * @brief  read data
 * @param  [in] fifo
 * @param  [in] data
 * @param  [in] size
 * @return real size or -1 when err
 */
int named_fifo_read(named_fifo_t *fifo, uint8_t *data, size_t size)
{
    int ret = 0;

    if (!(fifo && data && size)) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return -1;
    }
    lock();
    ret = lcfifo_read(fifo->lcfifo, data, size);
    unlock();

    return ret;
}

/**
 * @brief  set read finish
 * @param  [in] fifo
 * @param  [in] eof
 * @return  0/-1
 */
int named_fifo_set_reof(named_fifo_t *fifo, int eof)
{
    int ret = 0;

    if (!fifo) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return -1;
    }
    lock();
    fifo->read_eof = eof;
    unlock();

    return ret;
}

/**
 * @brief  set write finish
 * @param  [in] fifo
 * @param  [in] eof
 * @return  0/-1
 */
int named_fifo_set_weof(named_fifo_t *fifo, int eof)
{
    int ret = 0;

    if (!fifo) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return -1;
    }
    lock();
    fifo->write_eof = eof;
    unlock();

    return ret;
}

/**
 * @brief  get availabe read len
 * @param  [in] fifo
 * @return
 */
int named_fifo_get_rlen(named_fifo_t *fifo)
{
    int ret = 0;

    if (!fifo) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return -1;
    }
    lock();
    ret = lcfifo_get_rlen(fifo->lcfifo);
    unlock();

    return ret;
}

/**
 * @brief  get availabe write len
 * @param  [in] fifo
 * @return
 */
int named_fifo_get_wlen(named_fifo_t *fifo)
{
    int ret = 0;

    if (!fifo) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return -1;
    }
    lock();
    ret = lcfifo_get_wlen(fifo->lcfifo);
    unlock();

    return ret;
}

/**
 * @brief  is the read finish
 * @param  [in] fifo
 * @return
 */
int named_fifo_is_reof(named_fifo_t *fifo)
{
    int ret = 0;

    if (!fifo) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return -1;
    }
    lock();
    ret = fifo->read_eof;
    unlock();

    return ret;
}

/**
 * @brief  is the write finish
 * @param  [in] fifo
 * @return
 */
int named_fifo_is_weof(named_fifo_t *fifo)
{
    int ret = 0;

    if (!fifo) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return -1;
    }
    lock();
    ret = fifo->write_eof;
    unlock();

    return ret;
}

/**
 * @brief  close the fifo
 * @param  [in] fifo
 * @return 0/-1
 */
int named_fifo_close(named_fifo_t *fifo)
{
    int ret = 0;

    if (!fifo) {
        LOGE(TAG, "param err, %s", __FUNCTION__);
        return -1;
    }

    aos_mutex_lock(&g_fifo_list.lock, AOS_WAIT_FOREVER);
    lock();
    fifo->ref--;
    if (0 == fifo->ref) {
        dlist_t *item, *tmp;
        lcfifo_destroy(fifo->lcfifo);
        unlock();

        dlist_for_each_safe(item, tmp, &g_fifo_list.head) {
            if (&fifo->node == item) {
                dlist_del(item);
                break;
            }
        }
        aos_mutex_unlock(&g_fifo_list.lock);

        aos_mutex_free(&fifo->lock);
        av_free(fifo);
        return ret;
    }
    unlock();
    aos_mutex_unlock(&g_fifo_list.lock);

    return ret;
}


