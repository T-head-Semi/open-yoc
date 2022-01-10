/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef _VOICE_DEBUG_H_
#define _VOICE_DEBUG_H_

#include <stdio.h>
#include <errno.h>
#include <ulog/ulog.h>

/*
 * priority, defined in syslog.h
 */
#define LOG_EMERG       0       // system is unusable
#define LOG_ALERT       1       // action must be taken immediately
#define LOG_CRIT        2       // critical conditions
#define LOG_ERR         3       // error conditions
#define LOG_WARNING     4       // warning conditions
#define LOG_NOTICE      5       // normal but significant condition
#define LOG_INFO        6       // informational
#define LOG_DEBUG       7       // debug-level messages

void voice_log(int priority, const char *format, ...);

#define voice_log(priority, ...) ulog(priority, "Voice", __func__, __LINE__, __VA_ARGS__)

#define voice_check(X, errno)                                                                     \
    do {                                                                                          \
        if (!(X)) {                                                                               \
            voice_log(LOG_EMERG, "func: %s, errno: %d\n", __func__, errno);                       \
            abort();                                                                              \
        }                                                                                         \
    } while (0)

#define voice_check_param(X) voice_check(X, EINVAL)

#endif /* _VOICE_DEBUG_H_ */
