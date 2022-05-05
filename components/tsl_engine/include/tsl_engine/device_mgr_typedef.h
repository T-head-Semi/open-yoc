/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __DEVICE_MGR_TYPEDEF_H__
#define __DEVICE_MGR_TYPEDEF_H__

#include <tsl_engine/device.h>

__BEGIN_DECLS__

typedef struct {
    int event_type;
    //TODO:
} dev_msg_t;

typedef struct {
#define MESSAGE_NUM 10
    uint8_t                   status;
    size_t                    cnt;
    slist_t                   head;
    aos_mutex_t               lock;
    aos_queue_t               queue;
    uint8_t                   qbuf[sizeof(dev_msg_t) * MESSAGE_NUM];
} dev_mgr_t;


__END_DECLS__

#endif /* __DEVICE_MGR_TYPEDEF_H__ */

