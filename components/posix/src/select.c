/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 */

#include <sys/select.h>
#if defined(CONFIG_AOS_LWIP)
#include <lwip/opt.h>
#endif

#if !defined(CONFIG_AOS_LWIP) || (defined(CONFIG_AOS_LWIP) && !LWIP_SOCKET_SELECT)
extern int aos_select(int maxfd, fd_set *readset, fd_set *writeset, fd_set *exceptset,
                struct timeval *timeout);

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds,
                         struct timeval *timeout)
{
    return aos_select(nfds, readfds, writefds, errorfds, timeout);
}
#endif