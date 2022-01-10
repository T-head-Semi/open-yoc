/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/dict.h"

/**
 * @brief  init the dict
 * @param  [in] d
 * @param  [in] capacity : the number of key-val pair in the dict
 * @return 0/-1
 */
int dict_init(dict_t *d, int capacity)
{
    if (d && (capacity > 0)) {
        keyval_t *c = av_zalloc(sizeof(keyval_t) * capacity);
        CHECK_RET_WITH_RET(c, -1);

        d->keyvals = c;
        d->count   = 0;
        d->alloc   = capacity;
        return 0;
    }

    return -1;
}

/**
 * @brief  increase capacity of the dict
 * @param  [in] d
 * @param  [in] inc : increase size
 * @return 0/-1
 */
int dict_increase(dict_t *d, size_t inc)
{
    size_t capacity;
    size_t align = 7;

    if (!(d && inc)) {
        return -1;
    }

    capacity = (d->count + inc + align) & ~align;
    if (capacity > d->alloc) {
        keyval_t *keyvals;

        keyvals = av_realloc(d->keyvals, sizeof(keyval_t) * capacity);
        CHECK_RET_WITH_RET(keyvals, -1);

        d->alloc   = capacity;
        d->keyvals = keyvals;
    }

    return 0;
}

/**
 * @brief  add one kv pair
 * @param  [in] d
 * @param  [in] key
 * @param  [in] val
 * @return 0/-1
 */
int dict_add(dict_t *d, const char *key, const char *val)
{
    int rc;
    char *k, *v;

    if (!(d && key)) {
        return -1;
    }

    rc = dict_increase(d, 1);
    CHECK_RET_WITH_RET(rc == 0, -1);

    k = (char*)strdup((const char*)key);
    v = strdup(val);
    if (k && v) {
        d->keyvals[d->count].key = k;
        d->keyvals[d->count].val = v;
        d->count++;
        return 0;
    }

    av_free(k);
    av_free(v);

    return -1;
}

/**
 * @brief  add one kv pair(set len of the val)
 * @param  [in] d
 * @param  [in] key
 * @param  [in] val
 * @param  [in] valn : length of the val string
 * @return 0/-1
 */
int dict_addn(dict_t *d, const char *key, const char *val, size_t valn)
{
    int rc;
    char *k, *v;

    if (!(d && key && valn)) {
        return -1;
    }

    rc = dict_increase(d, 1);
    CHECK_RET_WITH_RET(rc == 0, -1);

    k = (char*)strdup((const char*)key);
    v = strndup(val, valn);
    if (k && v) {
        d->keyvals[d->count].key = k;
        d->keyvals[d->count].val = v;
        d->count++;
        return 0;
    }

    av_free(k);
    av_free(v);

    return -1;
}

/**
 * @brief  add one kv pair(the val is integer)
 * @param  [in] d
 * @param  [in] key
 * @param  [in] val
 * @return 0/-1
 */
int dict_add_int(dict_t *d, const char *key, int val)
{
    char buf[sizeof(int) + 1];

    if (!(d && key)) {
        return -1;
    }

    snprintf(buf, sizeof(buf), "%d", val);
    return dict_add(d, key, buf);
}

/**
 * @brief  reduce memory of the dict
 * @param  [in] d
 * @return 0/-1
 */
int dict_shrink(dict_t *d)
{
    if (!d) {
        return -1;
    }

    if (d->alloc > d->count) {
        keyval_t *keyvals = av_realloc(d->keyvals, sizeof(keyval_t) * d->count);
        CHECK_RET_WITH_RET(keyvals, -1);

        d->alloc   = d->count;
        d->keyvals = keyvals;
        return 0;
    }

    return 0;
}

/**
 * @brief  get count of the real kv pair
 * @param  [in] d
 * @return -1 on err
 */
int dict_count(const dict_t *d)
{
    return d ? d->count : -1;
}

/**
 * @brief  uninit the dict
 * @param  [in] d
 * @return 0/-1
 */
int dict_uninit(dict_t *d)
{
    if (d) {
        int i, cnt;
        keyval_t *keyvals = d->keyvals;
        if (keyvals) {
            cnt = d->count;
            for (i = 0; i < cnt; i++) {
                av_free(keyvals[i].key);
                av_free(keyvals[i].val);
            }
            av_free(keyvals);
        }

        memset(d, 0, sizeof(dict_t));

        return 0;
    }

    return -1;
}

/**
 * @brief  get the value by key
 * @param  [in] d
 * @param  [in] key
 * @return NULL on err
 */
const char *dict_get_val(const dict_t *d, const char *key)
{
    int i;

    if (!(d && key)) {
        return NULL;
    }

    for (i = 0; i < d->count; ++i) {
        if (strcasecmp((const char*)d->keyvals[i].key, (const char*)key) == 0) {
            return d->keyvals[i].val;
        }
    }

    return NULL;
}





