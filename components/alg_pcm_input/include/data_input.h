/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef __DATA_INPUT_H__
#define __DATA_INPUT_H__

#include <aos/aos.h>

#ifdef __cplusplus
extern "C" {
#endif

int pcm_hook_call(void *data, int len);

#ifdef __cplusplus
}
#endif

#endif