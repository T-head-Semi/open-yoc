/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __JS_ENGINE_TYPEDEF_H__
#define __JS_ENGINE_TYPEDEF_H__

#include <tsl_engine/tsl_common.h>

__BEGIN_DECLS__

typedef void (*jse_callback_t)(void *jse, void *arg);

typedef struct {
    jse_callback_t  callback;
    aos_mutex_t     sem;
    void            *param;
} jse_msg_t;


__END_DECLS__

#endif /* __JS_ENGINE_TYPEDEF_H__ */

