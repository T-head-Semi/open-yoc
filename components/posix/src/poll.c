/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 */

#include <poll.h>
#if defined(CONFIG_AOS_LWIP)
#include <lwip/opt.h>
#endif

#if !defined(CONFIG_AOS_LWIP) || (defined(CONFIG_AOS_LWIP) && !LWIP_SOCKET_POLL)
extern int aos_poll(struct pollfd *fds, int nfds, int timeout);
int poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    return aos_poll(fds, nfds, timeout);
}
#endif