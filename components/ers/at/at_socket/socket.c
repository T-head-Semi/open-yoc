/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#if defined(CONFIG_SAL) || defined(CONFIG_TCPIP)
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include <aos/debug.h>
#include <uservice/uservice.h>

#include "at_socket.h"
#include "../at_internal.h"

#define TAG "at_socket_cmd"

extern recv_mode_t g_at_recv_mode;

#define MAX_RECV_BUF_LEN (1600)
#define RECV_BUF(conn) (conn->recv_buf + conn->recv_size)
#define RECV_MAX_LEN(conn) (MAX_RECV_BUF_LEN - conn->recv_size)

void *at_socket_recv(at_conn_t *conn, int *size)
{
    ssize_t ret;

    if(conn->recv_size >= MAX_RECV_BUF_LEN) {
        return NULL;
    }

    ret = recv(conn->sockfd, RECV_BUF(conn), RECV_MAX_LEN(conn), MSG_DONTWAIT);

    if (ret <= 0) {
        if(errno != EAGAIN || ret == 0) {
            at_disconnect(conn, 1);
        }
        if (size)
            *size = 0;
        return NULL;
    }

    conn->recv_size += ret;
    if (size)
        *size = ret;

    return conn->recv_buf;
}

static int32_t at_connect_event_hdl(uint32_t id, char *cmd)
{
    if (id < MIN_CIP_ID || id > MAX_CIP_ID) {
        AT_LOGW(TAG, "statu cid=%d", id);

        if (cmd != NULL) {
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
        }

        return AT_OK;
    }

#ifdef CONFIG_LPM_DEEP_SLEEP
    at_protect_delay();
#endif

    if (at_cmd_event_on()) {
        atserver_lock();
        atserver_send("+EVENT=%s,%d\r\n", "CONNECT", id);
        atserver_unlock();
    }

    return AT_OK;
}

static int32_t at_accept_event_hdl(at_conn_t *conn)
{
    int                accept_fd;
    struct sockaddr_in frominet;
    int                fromlen = sizeof(frominet);

    if (conn->type != TYPE_TCP_SERVER) {
        return -EINVAL;
    }

    /* tcp server will accept the conn req */
    accept_fd    = accept(conn->ssockfd, (struct sockaddr *)&frominet, (socklen_t *)&fromlen);
    if(conn->c_cnt > 0) {
        close(accept_fd);
        return -1;
    }
    conn->c_cnt ++;
    conn->sockfd = accept_fd;
    conn->status = STATS_CONNECT;

    LOGD(TAG, "tcp server start");
    // at_event_send(AT_MSG_EVENT, AT_EVENT_SOCK, EVENT_CONN, conn->id, NULL);
    at_connect_event_hdl(conn->id, NULL);

    return 0;
}

static int32_t at_recv_event_hdl(at_conn_t *conn)
{
    int count;

    if (conn->status != STATS_CONNECT) {
        return -EINVAL;
    }

    AT_LOGD(TAG, "recv ev!");

    if (conn_find_by_id(conn->id) == NULL) {
        AT_LOGD(TAG, "conn stop before recv");
        return -EINVAL;
    }

    at_socket_recv(conn, &count);

    if (count > 0) {
        if (at_cmd_event_on() && g_at_recv_mode == PT_MODE) {
            atserver_lock();
            atserver_send("+EVENT=%s,%d,%d,", "RECV", conn->id, count);
            atserver_write(conn->recv_buf, count);
            conn->recv_size = 0;
            atserver_send("\r\n");
            atserver_unlock();
        }

        return 0;
    } else {
        printf("recv fail: %x\r\n", count);
    }

    return -EINVAL;
}

static void event_callback(uint32_t fd, const void *data, void *context)
{
    at_conn_t *conn = (at_conn_t *)context;

    if (conn->type == TYPE_TCP_SERVER && (0x80000000 | conn->ssockfd) == fd) {
        if (at_accept_event_hdl(conn) == 0) {
            // event_unsubscribe_fd(conn->ssockfd, event_callback, conn);
            event_subscribe_fd(conn->sockfd, event_callback, conn);
        } else if (conn->status == STATS_STOP){
            event_unsubscribe_fd(conn->ssockfd, event_callback, conn);
        }
    } else {
        at_recv_event_hdl(conn);
    }
}

void subscribe(at_conn_t *conn)
{
    if (conn->type == TYPE_TCP_SERVER && conn->status == STATS_CONNECTING) {
        event_subscribe_fd(conn->ssockfd, event_callback, conn);
    } else {
        event_subscribe_fd(conn->sockfd, event_callback, conn);
    }
}

void unsubscribe(at_conn_t *conn)
{
    if (conn->type == TYPE_TCP_SERVER && conn->status == STATS_CONNECTING)
        event_unsubscribe_fd(conn->ssockfd, event_callback, conn);
    else
        event_unsubscribe_fd(conn->sockfd, event_callback, conn);
}

static int32_t at_disconnect_event_hdl(uint32_t id)
{
    if (at_cmd_event_on()) {
        atserver_lock();
        atserver_send("+EVENT=%s,%d\r\n", "DISCONNECT", id);
        atserver_unlock();
    }

    return AT_OK;
}

/* ============================================= */
/*    tcp / udp connect / disconnect function    */
/* ============================================= */

static int at_start_tcp_server(at_conn_t *conn, char *cmd)
{
    int                fd;
    int                ret;
    struct sockaddr_in sinme = {'\0'};

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOGE(TAG, "fd=%d %d", fd, errno);
        return -1;
    }

    sinme.sin_port        = htons(conn->lport);
    sinme.sin_family      = AF_INET;
    sinme.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(fd, (const struct sockaddr *)&sinme, sizeof(sinme));

    if (ret != 0) {
        LOGE(TAG, "bind %d", errno);
        close(fd);
        return -1;
    }

    /* backlog will not work since TCP_LISTEN_BACKLOG is 0 */
    ret = listen(fd, 0);

    if (ret != 0) {
        LOGE(TAG, "listen %d", errno);
        close(fd);
        if (ENOTSUP == errno) {
            return AT_ERR_INVAL;
        } else {
            return -1;
        }
    }

    conn->ssockfd = fd;

    AT_BACK_OK();

    return AT_OK;
}

static int at_start_tcp_client(at_conn_t *conn, char *cmd)
{
    int                fd;
    int                ret;
    struct sockaddr_in sinme  = {'\0'};
    struct sockaddr_in sinhim = {'\0'};

    //LOGD(TAG, "1tcp start, %d", conn->id);
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOGE(TAG, "fd=%d %d", fd, errno);
        return -1;
    }

    //LOGD(TAG, "2tcp start, %d", conn->id);
    if (conn->lport != 0) {
        sinme.sin_port   = htons(conn->lport);
        sinme.sin_family = AF_INET;

        ret = bind(fd, (const struct sockaddr *)&sinme, sizeof(sinme));

        if (ret != 0) {
            LOGE(TAG, "bind %d", errno);
            close(fd);
            return AT_ERR;
        }
    }

    sinhim.sin_family      = AF_INET;
    sinhim.sin_addr.s_addr = conn->ip_addr.addr;
    sinhim.sin_port        = htons(conn->rport);

    if (connect(fd, (const struct sockaddr *)&sinhim, sizeof(sinhim)) < 0) {
        LOGE(TAG, "connect %d", errno);
        close(fd);
        return AT_ERR;
    }

    conn->sockfd = fd;
    conn->status = STATS_CONNECT;

    LOGD(TAG, "tcp start, %d", conn->id);
    at_dump_conn_arg(conn);
    AT_BACK_OK();
    at_connect_event_hdl(conn->id, cmd);

    return 0;
}

static int at_start_udp(at_conn_t *conn, char *cmd)
{
    int                fd;
    int                ret;
    struct sockaddr_in sinme  = {'\0'};
    struct sockaddr_in sinhim = {'\0'};

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LOGE(TAG, "fd=%d %d", fd, errno);
        return -1;
    }

    sinme.sin_port        = htons(conn->lport);
    sinme.sin_family      = AF_INET;
    sinme.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(fd, (const struct sockaddr *)&sinme, sizeof(sinme));

    if (ret != 0) {
        LOGE(TAG, "bind %d", errno);
        close(fd);
        return -1;
    }

    sinhim.sin_family      = AF_INET;
    sinhim.sin_addr.s_addr = conn->ip_addr.addr;
    sinhim.sin_port        = htons(conn->rport);

    if (connect(fd, (const struct sockaddr *)&sinhim, sizeof(sinhim)) < 0) {
        LOGE(TAG, "connect %d", errno);
        close(fd);
        return -1;
    }

    conn->sockfd = fd;
    conn->status = STATS_CONNECT;

    LOGD(TAG, "udp start");
    at_dump_conn_arg(conn);
    AT_BACK_OK();
    at_connect_event_hdl(conn->id, cmd);

    return 0;
}

int at_connect_start(at_conn_t *at_conn, char *cmd)
{
    int ret = -1;

    /* start tcp / udp */
    switch (at_conn->type) {
        case TYPE_TCP_SERVER:
            ret = at_start_tcp_server(at_conn, cmd);
            break;

        case TYPE_TCP_CLIENT:
            ret = at_start_tcp_client(at_conn, cmd);
            break;

        case TYPE_UDP_UNICAST:
            ret = at_start_udp(at_conn, cmd);
            break;

        default:
            LOGE(TAG, "type=%d", at_conn->type);
            break;
    }

    printf("start ret %d s:%d ss:%d type: %d status: %d\r\n",
           ret, at_conn->sockfd, at_conn->ssockfd, at_conn->type, at_conn->status);//debug

    if (ret == AT_OK) {
        add_conn(at_conn);
        subscribe(at_conn);
    }

    return ret;
}

/**
  * @brief     disconnect one connection
  *
  * @param[in]  conn connection to be disconnected
  * @param[in]  keep_server flg to specify if tcp server socket should
  *             be kept or not
  * @return     0: succeed, -1: failed
  *
  */
int at_disconnect(at_conn_t *conn, int keep_server)
{
    aos_check_param(conn);
    int temp_id = 0;

    /* check if already disconnect */
    if (conn->status == STATS_STOP) {
        return -EINVAL;
    }

    temp_id = conn->id;

    if (!keep_server) {
        conn->status = STATS_STOP;
    } else if (conn->type == TYPE_TCP_SERVER && conn->ssockfd == 0) {
        AT_LOGE(TAG, "sock close");
        return -1;
    }

    if (conn->sockfd) {
        event_unsubscribe_fd(conn->sockfd, event_callback, conn);
        close(conn->sockfd);
        conn->sockfd = 0;
        if (conn->c_cnt > 0) {
            conn->c_cnt --;
        }
    }

    if (conn->type == TYPE_TCP_SERVER) {
        /* if keep server, server will NOT be stop, and can accept new
        * connection */
        if (keep_server) {
            event_subscribe_fd(conn->ssockfd, event_callback, conn);
            conn->status = STATS_CONNECTING;
        } else {
            if (conn->ssockfd) {
                close(conn->ssockfd);
                if (conn->c_cnt > 0) {
                    conn->c_cnt --;
                }
            }
            event_unsubscribe_fd(conn->ssockfd, event_callback, conn);
            remove_conn(conn);
        }
    } else {
        remove_conn(conn);
    }

    at_disconnect_event_hdl(temp_id);

    return 0;
}

static AOS_SLIST_HEAD(conn_lists);

at_conn_t *conn_find_by_id(int id)
{
    at_conn_t *conn;
    slist_for_each_entry(&conn_lists, conn, at_conn_t, next) {
        if (conn->id == id) {
            return conn;
        }
    }

    return NULL;
}
at_conn_t *conn_find_by_fd(int fd)
{
    at_conn_t *conn;
    slist_for_each_entry(&conn_lists, conn, at_conn_t, next) {
        if (conn->type == TYPE_TCP_SERVER && conn->ssockfd == fd)
        {
            return conn;
        }
        else if (conn->sockfd == fd)
            return conn;
    }

    return NULL;
}

static void create_recv_buf(at_conn_t *at_conn)
{
    at_conn->recv_buf = (char *)aos_malloc_check(MAX_RECV_BUF_LEN);
}

static void destroy_recv_buf(at_conn_t *at_conn)
{
    aos_free(at_conn->recv_buf);
    at_conn->recv_size = 0;
}

void add_conn(at_conn_t *at_conn)
{
    create_recv_buf(at_conn);
    slist_add_tail(&at_conn->next, &conn_lists);
}

void remove_conn(at_conn_t *at_conn)
{
    destroy_recv_buf(at_conn);
    slist_del(&at_conn->next, &conn_lists);
    aos_free(at_conn);
}

int at_disconnect_all()
{
    int        ret = 0;
    at_conn_t *conn;
    slist_t   *tmp;

    slist_for_each_entry_safe(&conn_lists, tmp, conn, at_conn_t, next) {
        ret |= at_disconnect(conn, 1);
    }

    return ret;
}

int at_count_conn()
{
    int        ret = 0;
    at_conn_t *conn;
    slist_t   *tmp;

    slist_for_each_entry_safe(&conn_lists, tmp, conn, at_conn_t, next) {
        ret++;
    }

    return ret;
}
#endif
