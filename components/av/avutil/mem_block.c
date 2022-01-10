/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/mem_block.h"

#define TAG                   "memblock"

/**
 * @brief  new a mem block
 * @param  [in] size
 * @param  [in] align : address of the data is align when addr_align != 0
 * @return 0/-1
 */
mblock_t* mblock_new(size_t size, size_t align)
{
    size_t sz;
    mblock_t *mb;

    CHECK_PARAM(size, NULL);
    mb = av_zalloc(sizeof(mblock_t));
    CHECK_RET_TAG_WITH_RET(mb, NULL);

    sz = align ? (size + 2 * align) : size;
    mb->data_orip = av_zalloc(sz);
    CHECK_RET_TAG_WITH_GOTO(mb->data_orip, err);

    mb->used  = 0;
    mb->size  = size;
    mb->align = align;
    mb->data  = align ? AV_ALIGN(mb->data_orip, align) : mb->data_orip;

    return mb;
err:
    av_free(mb);
    return NULL;
}

/**
 * @brief  grow the mem block data size
 * @param  [in] mb
 * @param  [in] size
 * @return 0/-1
 */
int mblock_grow(mblock_t *mb, size_t size)
{
    int rc = -1;

    CHECK_PARAM(mb && size, -1);
    if (mb->size < size) {
        size_t sz, align;
        void *data_orip;

        align     = mb->align;
        sz        = align ? (size + 2 * align) : size;
        data_orip = av_realloc(mb->data_orip, sz);
        if (data_orip) {
            mb->size      = size;
            mb->data_orip = data_orip;
            mb->data      = align ? AV_ALIGN(mb->data_orip, align) : mb->data_orip;
            rc            = 0;
        }
    } else {
        rc = 0;
    }

    return rc;
}

/**
 * @brief  free resource of the mem block
 * @param  [in] mb
 * @return
 */
int mblock_free(mblock_t *mb)
{
    CHECK_PARAM(mb, -1);

    mb->used  = 0;
    mb->size  = 0;
    mb->align = 0;
    mb->data  = NULL;
    av_freep((char**)&mb->data_orip);
    av_free(mb);

    return 0;
}


