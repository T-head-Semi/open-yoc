/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/avpacket.h"

/**
 * @brief  init the packet
 * @param  [in] pkt
 * @return 0/-1
 */
int avpacket_init(avpacket_t *pkt)
{
    CHECK_PARAM(pkt, -1);
    memset(pkt, 0, sizeof(avpacket_t));

    return 0;
}

/**
 * @brief  malloc resource of the packet
 * @param  [in] pkt
 * @param  [in] size
 * @return 0/-1
 */
int avpacket_new(avpacket_t *pkt, size_t size)
{
    int rc = -1;

    CHECK_PARAM(pkt && size, -1);
    memset(pkt, 0, sizeof(avpacket_t));
    pkt->data = av_zalloc(size);
    if (pkt->data) {
        pkt->size = size;
        rc        = 0;
    }

    return rc;
}

/**
 * @brief  grow the packet data size
 * @param  [in] pkt
 * @param  [in] size
 * @return 0/-1
 */
int avpacket_grow(avpacket_t *pkt, size_t size)
{
    int rc = -1;

    CHECK_PARAM(pkt && size, -1);
    if (pkt->size < size) {
        void *data = av_realloc(pkt->data, size);
        if (data) {
            pkt->data = data;
            pkt->size = size;
            rc        = 0;
        }
    } else {
        rc = 0;
    }

    return rc;
}

/**
 * @brief  copy the src pkt to dsp pkt
 * @param  [in] spkt : src pkt
 * @param  [in] dpkt : dst pkt
 * @return 0/-1
 */
int avpacket_copy(const avpacket_t *spkt, avpacket_t *dpkt)
{
    int rc;

    CHECK_PARAM(spkt && spkt->len && spkt->data && dpkt, -1);
    rc = avpacket_grow(dpkt, spkt->len);
    if (rc == 0) {
        dpkt->len = spkt->len;
        dpkt->pts = spkt->pts;
        memcpy(dpkt->data, spkt->data, spkt->len);
    }

    return rc;
}

/**
 * @brief  free resource of the packet
 * @param  [in] pkt
 * @return
 */
int avpacket_free(avpacket_t *pkt)
{
    CHECK_PARAM(pkt, -1);
    pkt->len  = 0;
    pkt->size = 0;
    av_freep((char**)&pkt->data);
    return 0;
}


