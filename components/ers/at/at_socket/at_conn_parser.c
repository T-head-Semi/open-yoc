/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#if defined(CONFIG_SAL) || defined(CONFIG_TCPIP)
#include <errno.h>
#include <aos/debug.h>
#include <ctype.h>
#include <string.h>

#include "../at_internal.h"
#include "at_socket.h"

#define TAG "at_socket_conn"

#define AT_MAX_ARG_CNT (5)
typedef struct {
    at_conn_type_t conn_type;
    int8_t         min_len;
    int8_t         max_len;
    /* id, type, ip, rport, lport */
    struct {
        int must; /* cmd is optional or not */
        int (*at_parse_arg)(at_conn_t *conn, char *data);
    } parse_arg[AT_MAX_ARG_CNT];
} at_parse_func_t;

static char *g_at_conn_type[] = {
    "null",
    "tcp_server",
    "tcp_client",
    "udp_unicast",
};

const char *at_conn_type_str(at_conn_type_t type)
{
    return g_at_conn_type[type];
}


static int at_parse_id(at_conn_t *conn, char *data);
static int at_parse_type(at_conn_t *conn, char *data);
static int at_parse_ip(at_conn_t *conn, char *data);
static int at_parse_rport(at_conn_t *conn, char *data);
static int at_parse_lport(at_conn_t *conn, char *data);

static at_parse_func_t g_at_parse_func[] = {
    {TYPE_NULL, 2, 5, {{1, at_parse_id}, {1, at_parse_type}}},
    {TYPE_TCP_SERVER, 3, 3, {{1, at_parse_id}, {1, at_parse_type}, {1, at_parse_lport}}},
    {TYPE_TCP_CLIENT,4,5,{{1, at_parse_id},{1, at_parse_type},{1, at_parse_ip},{1, at_parse_rport},{0, at_parse_lport}}},
    {TYPE_UDP_UNICAST,5,5,{{1, at_parse_id},{1, at_parse_type},{1, at_parse_ip},{1, at_parse_rport},{1, at_parse_lport}}},
};

extern int strsplit(char **array, size_t count, char *data, const char *delim);
/**
  \brief       parse id in start cmd
  \return      char num with comma if success or -1 if fail
*/
static int at_parse_id(at_conn_t *conn, char *data)
{
    if(strlen(data) != 1 || isdigit((int)(*data)) == 0) {
        LOGW(TAG, "id:%s",data);
        return -1;
    }
    uint8_t cid = atoi(data);

    if (cid < MIN_CIP_ID || cid > MAX_CIP_ID) {
        AT_LOGW(TAG, "id=%d", cid);
        return -1;
    }

    conn->id = cid;

    return 0;
}

static int at_parse_type(at_conn_t *conn, char *data)
{
    for (int i = 1; i < ARRAY_SIZE(g_at_conn_type); i++) {
        if (strcmp(g_at_conn_type[i], data) == 0) {
            conn->type = i;

            return 0;
        }
    }

    return -1;
}

static int at_parse_ip(at_conn_t *conn, char *data)
{
    uint32_t u_ipaddr;

    u_ipaddr = inet_addr((const char *)data);

    if (u_ipaddr == IPADDR_NONE) {
        struct hostent *host = gethostbyname((const char *)data);

        if (host == NULL) {
            AT_LOGE(TAG, "parse ip err!");
            return -1;
        }

        u_ipaddr = *(uint32_t *)(host->h_addr_list[0]);
    }

    conn->ip_addr.addr = u_ipaddr;

    return 0;
}

static int at_parse_lport(at_conn_t *conn, char *data)
{
    int lport = atoi(data);

    if (lport > 0 && lport <= 65535) {
        conn->lport = lport;
        return 0;
    }

    AT_LOGW(TAG, "lport=%d", lport);
    return -1;
}

static int at_parse_rport(at_conn_t *conn, char *data)
{
    int rport = atoi(data);

    if (rport > 0 && rport <= 65535) {
        conn->rport = rport;
        return 0;
    }

    AT_LOGW(TAG, "rport=%d", rport);
    return -1;
}

at_conn_t *at_parse_start_arg(char *data)
{
    char *at_data[AT_MAX_ARG_CNT];

    at_conn_t *conn = (at_conn_t *)aos_zalloc(sizeof(at_conn_t));
    if (conn == NULL)
        return NULL;

    int arg_cnt = strsplit(at_data, AT_MAX_ARG_CNT, data, ",");

    for (int i = 0; i < arg_cnt; i++) {
        at_parse_func_t parse_func = g_at_parse_func[conn->type];

        if (arg_cnt < parse_func.min_len || arg_cnt > parse_func.max_len) {
            AT_LOGW(TAG, "type=%d arg cnt=%d", conn->type, arg_cnt);
            goto fail;
        }

        /* if optional and data not set, ignore it */
        if (!parse_func.parse_arg[i].must && strlen(at_data[i]) == 0) {
            continue;
        }

        if (parse_func.parse_arg[i].at_parse_arg(conn, at_data[i]) == -1) {
            AT_LOGW(TAG, "type=%d arg=%d fail", conn->type, i);
            goto fail;
        }
    }

    return conn;

fail:
    aos_free(conn);

    return NULL;
}

void at_dump_conn_arg(at_conn_t *conn)
{
    AT_LOGD(TAG, "conn info: id=%d, socketfd=%d", conn->id, conn->sockfd);
    AT_LOGD(TAG, "           target ip_add=%d.%d.%d.%d", ((conn->ip_addr.addr) & 0xff),
            ((conn->ip_addr.addr >> 8) & 0xff), ((conn->ip_addr.addr >> 16) & 0xff),
            ((conn->ip_addr.addr >> 24) & 0xff));
    AT_LOGD(TAG, "           rport=%d, lport=%d", conn->rport, conn->lport);
    AT_LOGD(TAG, "           type=%d, status=%d", conn->type, conn->status);
}
#endif
