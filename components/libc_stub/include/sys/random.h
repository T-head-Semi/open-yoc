/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 */

#ifndef __SYS_RANDOM__
#define __SYS_RANDOM__

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

ssize_t getrandom(void *buf, size_t buflen, unsigned int flags);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__SYS_RANDOM__
