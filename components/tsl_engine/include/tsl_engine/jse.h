/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __JSE_H__
#define __JSE_H__

#include <tsl_engine/jse_typedef.h>
#include <tsl_engine/file_utils.h>
#include "duktape.h"

__BEGIN_DECLS__

/**
 * @brief  init the js-engine
 * @return 0/-1
 */
int jse_init();

/**
 * @brief  load app-entry & start the jse-loop-task
 * @return 0/-1
 */
int jse_start();

/**
 * @brief  compile javascript code original to bytecode
 * @param  [in] jcode  javascript code
 * @param  [in] jsize  size of the jcode
 * @param  [in] osize  bytecode size
 * attention: return value should be free by the caller
 * @return NULL on error
 */
char* jse_bytecode_compile(const char *jcode, size_t jsize, size_t *osize);

/**
 * @brief  send message to the jse-task and excute it async
 * @param  [in] call callback will be called by jse-task
 * @param  [in] arg args for callback
 * @return 0/-1
 */
int jse_msg_send(jse_callback_t call, void *arg);

/**
 * @brief  send message to the jse-task and excute it
 * @param  [in] call callback will be called by jse-task
 * @param  [in] arg args for callback
 * @return 0/-1
 */
int jse_msg_send_sync(jse_callback_t call, void *arg);

__END_DECLS__

#endif /* __JSE_H__ */

