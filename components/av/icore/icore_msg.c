/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/common.h"
#include "icore/icore_msg.h"

/**
 * @brief  new one icore message by msdid
 * @param  [in] id
 * @param  [in] size : size of the msg actualy
 * @return NULL on error
 */
icore_msg_t *icore_msg_new(msgid_t id, size_t size)
{
    char *orip;
    icore_msg_t *msg = NULL;

    orip = av_zalloc(ICORE_ALIGN_BUFZ(size + ICORE_MSG_SIZE));
    if (orip) {
        msg           = ICORE_ALIGN(orip);
        msg->orip     = orip;
        msg->id       = id;
        msg->size     = size;
        msg->ret.code = -1;
    }

    return msg;
}

/**
 * @brief  free res by icore_msg_new
 * @param  [in] msg
 * @return
 */
void icore_msg_free(icore_msg_t *msg)
{
    if (msg && msg->orip) {
        av_free(msg->orip);
    }
}




