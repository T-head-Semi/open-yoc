/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __STASH_REF_H__
#define __STASH_REF_H__

#include <tsl_engine/tsl_common.h>
#include "duktape.h"

__BEGIN_DECLS__

/**
 * @brief  Create a global array refs in the heap stash.
 * @param  [in] ctx
 * @return
 */
void stash_ref_setup(duk_context *ctx);

int stash_ref(duk_context *ctx);

void stash_push_ref(duk_context *ctx, int ref);

void stash_unref(duk_context *ctx, int ref);

__END_DECLS__

#endif /* __STASH_REF_H__ */

