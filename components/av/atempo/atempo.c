/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "atempo/atempo.h"

#define TAG                   "atempo"

static struct atempo_ops      *_atempo_ops;

/**
 * @brief  regist atempo ops
 * @param  [in] ops
 * @return 0/-1
 */
int atempo_ops_register(const struct atempo_ops *ops)
{
    CHECK_PARAM(ops, -1);
    if (_atempo_ops) {
        LOGE(TAG, "error. ops had regist yet!, name = %s", _atempo_ops->name);
        return -1;
    }
    _atempo_ops = (struct atempo_ops*)ops;
    LOGD(TAG, "regist affecter, name = %s", _atempo_ops->name);

    return 0;
}

/**
 * @brief  alloc a audio atempoer
 * @param  [in] rate      : audio freq
 * @return NULL on error
 */
atempo_t* atempo_new(uint32_t rate)
{
    int rc;
    atempo_t *atempo;

    CHECK_PARAM(rate, NULL);
    if (!_atempo_ops) {
        LOGE(TAG, "error. ops has not regist yet!s");
        return NULL;
    }

    atempo = av_zalloc(sizeof(atempo_t));
    atempo->rate  = rate;
    atempo->speed = 1;

    rc = _atempo_ops->init(atempo);
    CHECK_RET_TAG_WITH_GOTO(rc == 0, err);
    atempo->ops = _atempo_ops;
    aos_mutex_new(&atempo->lock);

    return atempo;
err:
    av_free(atempo);
    return NULL;
}

/**
 * @brief  write samples to atempoer
 * @param  [in] atempo
 * @param  [in] in
 * @param  [in] isamples
 * @return 0/-1
 */
int atempo_write(atempo_t *atempo, const int16_t *in, size_t isamples)
{
    int rc;

    CHECK_PARAM(atempo && in && isamples, -1);
    aos_mutex_lock(&atempo->lock, AOS_WAIT_FOREVER);
    rc = atempo->ops->write(atempo, in, isamples);
    aos_mutex_unlock(&atempo->lock);

    return rc;
}

/**
 * @brief  read samples from atempoer
 * @param  [in] atempo
 * @param  [in] out
 * @param  [in] osamples
 * @return num of read samples, -1 on error
 */
int atempo_read(atempo_t *atempo, int16_t *out, size_t osamples)
{
    int rc;

    CHECK_PARAM(atempo && out && osamples, -1);
    aos_mutex_lock(&atempo->lock, AOS_WAIT_FOREVER);
    rc = atempo->ops->read(atempo, out, osamples);
    aos_mutex_unlock(&atempo->lock);

    return rc;
}

/**
 * @brief  set speed of the audio atempoer
 * @param  [in] atempo
 * @param  [in] speed
 * @return 0/-1
 */
int atempo_set_speed(atempo_t *atempo, float speed)
{
    int rc = 0;

    CHECK_PARAM(atempo && speed > 0, -1);
    aos_mutex_lock(&atempo->lock, AOS_WAIT_FOREVER);
    if (atempo->speed != speed) {
        rc = atempo->ops->set_speed(atempo, speed);
    }
    aos_mutex_unlock(&atempo->lock);

    return rc;
}

/**
 * @brief  flush the audio atempoer
 * @param  [in] atempo
 * @return 0/-1
 */
int atempo_flush(atempo_t *atempo)
{
    int rc;

    CHECK_PARAM(atempo, -1);
    aos_mutex_lock(&atempo->lock, AOS_WAIT_FOREVER);
    rc = atempo->ops->flush(atempo);
    aos_mutex_unlock(&atempo->lock);

    return rc;
}

/**
 * @brief  free the audio atempoer
 * @param  [in] atempo
 * @return 0/-1
 */
int atempo_free(atempo_t *atempo)
{
    CHECK_PARAM(atempo, -1);
    atempo->ops->uninit(atempo);
    aos_mutex_free(&atempo->lock);
    av_free(atempo);

    return 0;
}


