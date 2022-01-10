/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "output/mixer.h"

#define TAG    "mixer"

#define mixer_lock()   (aos_mutex_lock(&mixer->lock, AOS_WAIT_FOREVER))
#define mixer_unlock() (aos_mutex_unlock(&mixer->lock))

/**
 * @brief  create one mixer
 * @param  [in] sf
 * @return NULL on error
 */
mixer_t* mixer_new(sf_t sf)
{
    mixer_t *mixer;

    CHECK_PARAM(sf, NULL);
    mixer = av_zalloc(sizeof(mixer_t));
    CHECK_RET_TAG_WITH_RET(mixer, NULL);

    mixer->sf = sf;
    slist_init(&mixer->lists);
#ifdef __linux__
    pthread_cond_init(&mixer->cond, NULL);
#else
    aos_event_new(&mixer->evt, 0);
#endif
    aos_mutex_new(&mixer->lock);

    return mixer;
}

/**
 * @brief  get the number of channels attached
 * @param  [in] mixer
 * @return -1 on error
 */
int mixer_get_nb_cnls(mixer_t *mixer)
{
    int rc = 0;

    CHECK_PARAM(mixer, -1);
    mixer_lock();
    rc = mixer->nb_cnls;
    mixer_unlock();

    return rc;
}

/**
 * @brief  config sf before channels attached
 * @param  [in] mixer
 * @param  [in] sf
 * @return 0/-1
 */
int mixer_set_sf(mixer_t *mixer, sf_t sf)
{
    int rc = -1;

    CHECK_PARAM(mixer && sf, -1);
    mixer_lock();
    if (mixer->nb_cnls) {
        LOGE(TAG, "not dettach all yet! nb_cnls = %u", mixer->nb_cnls);
    } else {
        mixer->sf = sf;
        rc        = 0;
    }
    mixer_unlock();

    return rc;
}

/**
 * @brief  get the sf of mixer
 * @param  [in] mixer
 * @return
 */
sf_t mixer_get_sf(mixer_t *mixer)
{
    sf_t sf;

    CHECK_PARAM(mixer, -1);
    mixer_lock();
    sf = mixer->sf;
    mixer_unlock();

    return sf;
}

/**
 * @brief  read pcm data from the mixer
 * @param  [in] mixer
 * @param  [in] buf
 * @param  [in] size
 * @param  [in] timeout : ms. 0 means no block, -1 means wait-forever
 * @return -1 on error
 */
int mixer_read(mixer_t *mixer, uint8_t *buf, size_t size, uint32_t timeout)
{
    int rc;
    char *pos;
    int64_t sum;
    sfifo_t *fifo;
    mixer_cnl_t *cnl;
    struct mix_buf *mbufs;
    int16_t *s, *d = (int16_t*)buf;
    int rlen, i, count = 0, rmin = size;

    CHECK_PARAM(mixer && buf && size >= sizeof(int16_t), -1);
retry:
    mixer_lock();
    mbufs = mixer->mbufs;
    slist_for_each_entry(&mixer->lists, cnl, mixer_cnl_t, node) {
        if (cnl->status == MIXER_CNL_STATUS_RUNING) {
            fifo = cnl->fifo;
            rlen = sfifo_get_rpos(fifo, &pos, 0);
            if (rlen >= 2) {
                mbufs[count].pos  = pos;
                mbufs[count].fifo = fifo;
                rmin              = rmin > rlen ? rlen : rmin;
                count++;
            }
        }
    }

    if (!count) {
#ifdef __linux__
        {
            uint64_t nsec;
            struct timespec ts;
            struct timeval now;

            gettimeofday(&now, NULL);
            nsec       = now.tv_usec * 1000 + (timeout % 1000) * 1000000;
            ts.tv_nsec = nsec % 1000000000;
            ts.tv_sec  = now.tv_sec + nsec / 1000000000 + timeout / 1000;
            rc = pthread_cond_timedwait(&mixer->cond, &mixer->lock, &ts);
            mixer_unlock();
            if (rc != 0)
            {
                return 0;
            }
            goto retry;
        }
#else
        mixer_unlock();
        {
            unsigned int flag;
            rc = aos_event_get(&mixer->evt, MIXER_READ_EVENT, AOS_EVENT_OR_CLEAR, &flag, timeout);
        }
        if (rc != 0) {
            return 0;
        }
        goto retry;
#endif
    }

    if (count > 1) {
        for (rlen = 0; rlen < rmin / sizeof(int16_t); rlen++) {
            sum = 0;
            for (i = 0; i < count; i++) {
                s = (int16_t*)mbufs[i].pos;
                sum += s[rlen];
            }

            d[rlen] = sum / count;
        }

        for (i = 0; i < count; i++) {
            sfifo_set_rpos(mbufs[i].fifo, rmin);
        }
    } else {
        memcpy(buf, mbufs[0].pos, rmin);
        sfifo_set_rpos(mbufs[0].fifo, rmin);
    }
    mixer_unlock();

    return rmin;
}

/**
 * @brief  attach channel to the mixer
 * @param  [in] mixer
 * @param  [in] cnl
 * @return 0/-1
 */
int mixer_attach(mixer_t *mixer, mixer_cnl_t *cnl)
{
    struct mix_buf *mbufs = NULL;

    CHECK_PARAM(mixer && cnl, -1);
    mixer_lock();
    if (mixer->sf != cnl->sf) {
        mixer_unlock();
        LOGE(TAG, "sf not equal. mixer => %s, cnl = %s", sf_get_format_str(mixer->sf), sf_get_format_str(cnl->sf));
        return -1;
    }

    if (mixer->nb_mbufs < mixer->nb_cnls + 1) {
        mixer->nb_mbufs += 4;
        mbufs = (struct mix_buf*)av_realloc(mixer->mbufs, mixer->nb_mbufs * sizeof(struct mix_buf));
        if (!mbufs) {
            LOGE(TAG, "may be oom, %d", mixer->nb_mbufs);
            mixer_unlock();
            return -1;
        }
        mixer->mbufs = mbufs;
    }

    slist_add_tail(&cnl->node, &mixer->lists);
    mixer->nb_cnls++;
    cnl->mixer = mixer;
    mixer_unlock();

    return 0;
}

/**
 * @brief  deattach channel from the mixer
 * @param  [in] mixer
 * @param  [in] cnl
 * @return 0/-1
 */
int mixer_dettach(mixer_t *mixer, mixer_cnl_t *cnl)
{
    CHECK_PARAM(mixer && cnl, -1);
    mixer_lock();
    slist_del(&cnl->node, &mixer->lists);
    mixer->nb_cnls--;
    cnl->mixer = NULL;
    mixer_unlock();

    return 0;
}

/**
 * @brief  set channel status
 * @param  [in] mixer
 * @param  [in] cnl
 * @param  [in] status : MIXER_CNL_STATUS_XX
 * @return -1 on error
 */
int mixer_set_cnl_status(mixer_t *mixer, mixer_cnl_t *cnl, int status)
{
    //FIXME:
    CHECK_PARAM(mixer && cnl, -1);
    mixer_lock();
    cnl->status = status;
    mixer_unlock();

    return 0;
}

/**
 * @brief  destroy the mixer. need dettach all mixer-channel
 * @param  [in] mixer
 * @return 0/-1
 */
int mixer_free(mixer_t *mixer)
{
    CHECK_PARAM(mixer, -1);
    if (mixer->nb_cnls) {
        LOGE(TAG, "not dettach all yet! nb_cnls = %u", mixer->nb_cnls);
        return -1;
    }
#ifdef __linux__
    pthread_cond_destroy(&mixer->cond);
#else
    aos_event_free(&mixer->evt);
#endif
    aos_mutex_free(&mixer->lock);
    av_free(mixer->mbufs);
    av_free(mixer);

    return 0;
}


