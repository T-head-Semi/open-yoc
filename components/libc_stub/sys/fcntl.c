/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(CONFIG_TCPIP) || defined(CONFIG_SAL)
#include "lwip/sys.h"
#include "lwip/sockets.h"
#else
#undef MEMP_NUM_NETCONN
#define MEMP_NUM_NETCONN 0
#endif

int fcntl(int fd, int cmd, ...)
{
    int ret = 0;
    va_list ap;
    va_start(ap, cmd);

#if MEMP_NUM_NETCONN

    if ((fd >= LWIP_SOCKET_OFFSET)
        && (fd < (LWIP_SOCKET_OFFSET + MEMP_NUM_NETCONN))) {
        #if defined(CONFIG_SAL)
        extern int sal_fcntl(int s, int cmd, ...);
        ret = sal_fcntl(fd, cmd, va_arg(ap, int));
        #else
        ret = lwip_fcntl(fd, cmd, va_arg(ap, int));
        #endif
    }

#endif
    va_end(ap);

    return ret;
}
