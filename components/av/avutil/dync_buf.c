/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/dync_buf.h"
#include <stdarg.h>

/**
 * @brief  init the dynamic buf
 * @param  [in] buf
 * @param  [in] capacity : initial size of the dbuf
 * @param  [in] max_capacity : space of the dynamic buf will not exceed the value
 *                             0 present no limited
 * @return 0/-1
 */
int dync_buf_init(dync_buf_t *dbuf, size_t capacity, size_t max_capacity)
{
    char *data;

    if (!(dbuf && capacity)) {
        return -1;
    }
    if (!(DYNC_BUF_IS_UNLIMITED(max_capacity) || (capacity <= max_capacity))) {
        return -1;
    }

    data = (char*)av_zalloc(capacity);
    if (data) {
        dbuf->data         = data;
        dbuf->capacity     = capacity;
        dbuf->max_capacity = max_capacity;
        dbuf->len          = 0;
        return 0;
    }

    return -1;
}

/**
 * @brief  increase capacity of the dynamic buf
 * @param  [in] dbuf
 * @param  [in] inc : increase size
 * @return 0/-1
 */
int dync_buf_increase(dync_buf_t *dbuf, size_t inc)
{
    int ret = -1;
    char *data;
    size_t capacity;
    size_t align = 63;

    if (!(dbuf && inc)) {
        return ret;
    }

    capacity = (dbuf->len + inc + 1 + align) & ~align;
    if (capacity > dbuf->capacity) {
        if (!(DYNC_BUF_IS_UNLIMITED(dbuf->max_capacity) || (capacity <= dbuf->max_capacity))) {
            return ret;
        }

        data = av_realloc(dbuf->data, capacity);
        if (data) {
            dbuf->data     = data;
            dbuf->capacity = capacity;
            ret            = 0;
        }
    } else {
        ret = 0;
    }

    return ret;
}

/**
 * @brief  add char to the dbuf
 * @param  [in] dbuf
 * @param  [in] ch
 * @return 0/-1
 */
int dync_buf_add_char(dync_buf_t *dbuf, char ch)
{
    int ret = -1;

    if (!dbuf) {
        return ret;
    }

    ret = dync_buf_increase(dbuf, 1);
    if (ret == 0) {
        dbuf->data[dbuf->len++] = ch;
        dbuf->data[dbuf->len] = 0;
    }

    return ret;
}

/**
 * @brief  add buf to the dbuf
 * @param  [in] dbuf
 * @param  [in] data
 * @param  [in] len
 * @return 0/-1
 */
int dync_buf_add_bytes(dync_buf_t *dbuf, const void *data, size_t len)
{
    int ret = -1;

    if (!(dbuf && data && len)) {
        return ret;
    }

    ret = dync_buf_increase(dbuf, len);
    if (ret == 0) {
        memcpy(dbuf->data + dbuf->len, data, len);
        dbuf->len             += len;
        dbuf->data[dbuf->len]  = 0;
    }

    return ret;
}

/**
 * @brief  add string to the dbuf
 * @param  [in] dbuf
 * @param  [in] str
 * @return 0/-1
 */
int dync_buf_add_string(dync_buf_t *dbuf, const char *str)
{
    int ret = -1;
    int len;

    if (!(dbuf && str)) {
        return -1;
    }

    len = strlen(str);
    ret = dync_buf_increase(dbuf, len);
    if (ret == 0) {
        memcpy(dbuf->data + dbuf->len, str, len);
        dbuf->len             += len;
        dbuf->data[dbuf->len]  = 0;
    }

    return ret;
}

/**
 * @brief  add format data to the dbuf
 * @param  [in] dbuf
 * @param  [in] fmt
 * @return 0/-1
 */
int dync_buf_add_fmt(dync_buf_t *dbuf, const char *fmt, ...)
{
    int ret = -1;
    int len;
    va_list ap;

    if (!(dbuf && fmt)) {
        return ret;
    }

    va_start(ap, fmt);
    len = vsnprintf(dbuf->data + dbuf->len, dbuf->capacity - dbuf->len, fmt, ap);
    va_end(ap);

    if (len > dync_buf_get_wlen(dbuf)) {
        ret = dync_buf_increase(dbuf, len + 1);
        if (ret == 0) {
            va_start(ap, fmt);
            len = vsnprintf(dbuf->data + dbuf->len, dbuf->capacity - dbuf->len, fmt, ap);
            va_end(ap);

            dbuf->len += len;
            ret = 0;
        }
    } else {
        dbuf->len += len;
        ret = 0;
    }

    return ret;
}

/**
 * @brief  reset the dbuf(not release related resource)
 * @param  [in] dbuf
 * @return 0/-1
 */
int dync_buf_reset(dync_buf_t *dbuf)
{
    if (!dbuf) {
        return -1;
    }

    if (dbuf->capacity) {
        memset(dbuf->data, 0, dbuf->capacity);
    }
    dbuf->len = 0;

    return 0;
}

/**
 * @brief  get the availabe write size
 * @param  [in] dbuf
 * @return -1 on err
 */
int dync_buf_get_wlen(dync_buf_t *dbuf)
{
    return dbuf ? (dbuf->capacity - dbuf->len - 1) : -1;
}

/**
 * @brief  get available write pos and size
 * @param  [in] dbuf
 * @param  [in] pos
 * @return availiable write size
 */
int dync_buf_get_wpos(dync_buf_t *dbuf, char **pos)
{
    int ret = -1;

    if (!(dbuf && pos)) {
        return ret;
    }

    *pos = dbuf->data + dbuf->len;
    return dbuf->capacity - dbuf->len - 1;
}

/**
 * @brief  set write size
 * @param  [in] dbuf
 * @param  [in] count
 * @return 0/-1
 */
int dync_buf_set_wpos(dync_buf_t *dbuf, size_t count)
{
    int ret = -1;

    if (!(dbuf && count)) {
        return ret;
    }

    if (dbuf->capacity - dbuf->len - 1 > count) {
        dbuf->len += count;
        ret        = 0;
    }

    return ret;
}

/**
 * @brief  get data start point of the dbuf
 * @param  [in] dbuf
 * @return NULL on err
 */
char* dync_buf_get_data(dync_buf_t *dbuf)
{
    return dbuf ? dbuf->data : NULL;
}

/**
 * @brief  get the actully take size
 * @param  [in] dbuf
 * @return -1 on err
 */
int dync_buf_get_len(dync_buf_t *dbuf)
{
    return dbuf ? dbuf->len : -1;
}

/**
 * @brief  uninit the dbug
 * @param  [in] dbuf
 * @return 0/-1
 */
int dync_buf_uninit(dync_buf_t *dbuf)
{
    if (!dbuf) {
        return -1;
    }

    av_free(dbuf->data);
    memset(dbuf, 0, sizeof(dync_buf_t));

    return 0;
}




