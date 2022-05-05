/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#if defined(CONFIG_SAL) || defined(CONFIG_TCPIP)
#if defined (CONFIG_AT_SOCKET_ZTW)
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <aos/aos.h>
#include <yoc_net.h>
#include "at_internal.h"
#include "at_event.h"

static char *const TAG =  "at_sock";

#define S(x) #x
#define STR(x) S(x)

#define ATSOCKET_CTX_MAX_NUM      2
#define ATSOCKET_CTX_MIN_NUM      1
#define ATSOCKET_URL_MAX_LEN      96
#define ATSOCKET_SEND_MAX_LEN     1028
#define ATSOCKET_RECV_MAX_LEN     1028
#define ATSOCKET_SEND_TIMEOUT     2000
#define ATSOCKET_RECV_TIMEOUT     1000
#define ATSOCKET_TASK_PRI         KPRIO_NORMAL_BELOW7
#define ATSOCKET_TASK_STACK_SIZE  768

#define ATSOCKET_NETTYPE_TCP 0
#define ATSOCKET_NETTYPE_UDP 1

enum at_socket_status {
    FREE = 0,
    USED = 1,
};

enum at_socket_nettype {
    TCP = ATSOCKET_NETTYPE_TCP,
    UDP = ATSOCKET_NETTYPE_UDP,
};

enum at_socket_datamode {
    STRING = 1,
    HEXSTR,
    BINARY,
};

struct at_socket_ztw_context {
    int socket_fd;                                        /**< fd for socket */
    enum at_socket_status status;                         /**< ctx status */
    enum at_socket_nettype net_type;                      /**< ctx net type: TCP or UDP */
    struct sockaddr_in    remote_addr;                    /**< remote server address */
    aos_mutex_t mutex;                               /**< mutex for send and recv */
    uint8_t  *recv_buf;                                   /**< recv buf */
    uint8_t  *send_buf;                                   /**< send buf */
    uint32_t send_data_len;                               /**< last send data len */
};

static struct at_socket_ztw_context g_atsock_ztw_ctx[ATSOCKET_CTX_MAX_NUM] = {0}; /**< ctx for each socket_id */
static aos_sem_t g_atsock_ztw_sem = { NULL };                                /**< sem for task exit */
static aos_sem_t g_attask_ztw_sem = { NULL };                                /**< sem for task wake */
static k_task_handle_t g_atsock_task_ztw_handle = NULL;                       /**< task handle for recv and report */
static uint8_t g_atsock_task_ztw_run = 0;                                     /**< task flag for task exit */
static uint8_t g_network_status = 0;

/* g_atsock_datamode cloud set with +ZDTMODE, default to STRING */
static enum at_socket_datamode g_atsock_datamode = STRING;

static struct at_socket_ztw_context *get_atsock_ctx(uint8_t id)
{
    if (id >= ATSOCKET_CTX_MAX_NUM) {
        return NULL;
    } else {
        return &(g_atsock_ztw_ctx[id]);
    }
}

static void free_atsock_ctx(struct at_socket_ztw_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    ctx->status = FREE;

    if (ctx->socket_fd >= 0) {
        aos_mutex_lock(&ctx->mutex, AOS_WAIT_FOREVER);
        close(ctx->socket_fd);
        aos_mutex_unlock(&ctx->mutex);
        ctx->socket_fd = -1;
    }

    if (ctx->recv_buf) {
        aos_free(ctx->recv_buf);
        ctx->recv_buf = NULL;
    }

    if (ctx->send_buf) {
        aos_free(ctx->send_buf);
        ctx->send_buf = NULL;
    }

    if (ctx->mutex.hdl) {
        aos_mutex_free(&ctx->mutex);
        ctx->mutex.hdl = NULL;
    }

}

static int32_t at_sock_task_wait(void)
{
    return aos_sem_wait(&g_attask_ztw_sem, AOS_WAIT_FOREVER);
}

static int32_t at_sock_task_wake(void)
{
    aos_sem_signal(&g_attask_ztw_sem);

    return AT_OK;
}

/* send data to at server */
static void back_at_cmd(char *atcmd)
{
    while (at_event_send(AT_MSG_CMD, AT_EVENT_CMD_BACK, 0, 0, atcmd)) {
        LOGE(TAG, "at_event_send");
        csi_kernel_delay_ms(10);
    }
}

/* send data to at server */
static void back_at_cmd_print(uint32_t len, const char *fmt, ...)
{
    va_list args;

    char *atcmd = aos_malloc(len);

    if (atcmd == NULL) {
        return;
    }

    va_start(args, fmt);
    vsprintf(atcmd, fmt, args);
    va_end(args);

    while (at_event_send(AT_MSG_CMD, AT_EVENT_CMD_BACK, 0, 0, atcmd)) {
        LOGE(TAG, "at_event_send");
        csi_kernel_delay_ms(10);
    }
}

#ifdef CONFIG_YOC_LPM
int32_t at_socket_zte_lpm_hdl()
{
    uint8_t i;
    char ret_cmd[20];
    struct at_socket_ztw_context *ctx;

    for (i = 0; i < ATSOCKET_CTX_MAX_NUM; i++) {
        ctx = get_atsock_ctx(i);

        if (ctx && ctx->status == USED) {
            free_atsock_ctx(ctx);
            snprintf(ret_cmd, 20, "\r\n+ZIPSTAT:%d,0\r\n", i + 1);
            AT_BACK_STR(ret_cmd);
            AT_LOGI(TAG, "zte socket=%d close", i + 1);
        }
    }

    return 0;
}
#endif


/* at socket task, every socket has a task for recv */
static void atsocket_task_entry(void *arg)
{
    int ret;
    struct at_socket_ztw_context *ctx;
    fd_set read_fds;
    int count = 0, i;
    struct timeval interval = {ATSOCKET_RECV_TIMEOUT / 1000, (ATSOCKET_RECV_TIMEOUT % 1000) * 1000};

    while (g_atsock_task_ztw_run) {

        int max_fd = 0;
        int isselect = 0;
        FD_ZERO(&read_fds);

        /* FD_SET for used ctx */
        for (i = 0; i < ATSOCKET_CTX_MAX_NUM; i ++) {
            if ((ctx = get_atsock_ctx(i)) && ctx->status == USED) {
                isselect = 1;
                FD_SET(ctx->socket_fd, &read_fds);

                if (ctx->socket_fd > max_fd) {
                    max_fd = ctx->socket_fd;
                }
            }
        }

        if (isselect == 0) {
            at_sock_task_wait();
            //csi_kernel_delay(csi_kernel_ms2tick(ATSOCKET_RECV_TIMEOUT));
            continue;
        }

        ret = select(max_fd + 1, &read_fds, NULL, NULL, &interval);

        /* Zero fds ready means we timed out */
        if (ret == 0) {
            continue;
        }

        if (ret < 0) {
            LOGE(TAG, "select %d", errno);
            continue;
        }

        /* check FD_ISSET and recv data */
        for (i = 0; i < ATSOCKET_CTX_MAX_NUM; i ++) {
            if ((ctx = get_atsock_ctx(i)) &&
                ctx->status == USED &&
                FD_ISSET(ctx->socket_fd, &read_fds)) {

                /* recv data, use mutex to avoid conflict with send */
                aos_mutex_lock(&ctx->mutex, AOS_WAIT_FOREVER);
                count = recvfrom(ctx->socket_fd, ctx->recv_buf,
                                 ATSOCKET_RECV_MAX_LEN, 0,
                                 (struct sockaddr *) & (ctx->remote_addr),
                                 (socklen_t *) & (ctx->remote_addr.sin_len));
                aos_mutex_unlock(&ctx->mutex);

                if (count < 0) {
                    LOGE(TAG, "recv %d", errno);

                    /* report socket_id closed and free ctx, if recv occur any error */
                    back_at_cmd_print(20, "\r\n+ZIPSTAT:%d,0\r\n", i + 1);

                    free_atsock_ctx(ctx);
                    continue;
                }

                if (count == 0) {
                    /* report socket_id closed and free ctx, if server close socket */
                    back_at_cmd_print(20, "\r\n+ZIPSTAT:%d,0\r\n", i + 1);

                    free_atsock_ctx(ctx);
                    continue;
                }

                char *atcmd = aos_malloc(count * 2 + 50);

                /* if malloc failed, we should drop the packet and go on */
                if (atcmd == NULL) {
                    continue;
                }

                memset(atcmd, 0, count * 2 + 50);

                /* report recv data: +ZIPRECV:socket_id,ip,remote_port,len,data, data is hexstr */
                sprintf(atcmd, "\r\n+ZIPRECV:%d,", i + 1);

                /* conver ip to string */
                inet_ntop(ctx->remote_addr.sin_family, &ctx->remote_addr.sin_addr, atcmd + strlen(atcmd), 16);

                sprintf(atcmd + strlen(atcmd), ",%d,%d,", ntohs(ctx->remote_addr.sin_port), count);

                at_bytes2hexstr(atcmd + strlen(atcmd), ctx->recv_buf, count);

                strcat(atcmd, "\r\n");

                /* report recv data */
                back_at_cmd(atcmd);
            }
        }
    }

    aos_sem_signal(&g_atsock_ztw_sem);
}


/* AT+ZIPOPEN=? */
/* AT+ZIPOPEN? */
/* AT+ZIPOPEN=<socket_id>,<net_type>,<remote_url>,<remote_port>[,local_port] */
void at_cmd_zip_open(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_RET_OK_CMD_HEAD(cmd);
        AT_BACK_STR("(" STR(ATSOCKET_CTX_MIN_NUM) "-" STR(ATSOCKET_CTX_MAX_NUM) "),"
                    "(" STR(ATSOCKET_NETTYPE_TCP) "-" STR(ATSOCKET_NETTYPE_UDP) "),"
                    "(1-65535)\r\n");
        AT_BACK_OK;
    } else if (type == READ_CMD) {
        int i;
        struct at_socket_ztw_context *ctx;
        char ip[16];
        char ret_cmd[64];


        for (i = 0; i < ATSOCKET_CTX_MAX_NUM; i ++) {
            ctx = get_atsock_ctx(i);

            if (ctx && ctx->status == USED) {
                /* conver ip to string */
                inet_ntop(ctx->remote_addr.sin_family, &ctx->remote_addr.sin_addr, ip, 16);

                snprintf(ret_cmd, 84, "%d,%d,%s,%d\r\n", i + 1, ctx->net_type, ip, ntohs(ctx->remote_addr.sin_port));

                AT_BACK_RET_OK_HEAD(cmd, ret_cmd);
            }
        }

        AT_BACK_OK;
    } else if (type == WRITE_CMD) {
        int ret;
        char ret_cmd[20];
        int err_code = AT_OK;
        int socket_id = -1, net_type = -1, remote_port = -1, local_port = -1;
        char url[ATSOCKET_URL_MAX_LEN];
        int sockfd = -1;
        struct at_socket_ztw_context *ctx;
        struct addrinfo *res, *ainfo;
        struct addrinfo hints;
        struct sockaddr_in local = {'\0'};
        struct timeval interval = {ATSOCKET_SEND_TIMEOUT / 1000, (ATSOCKET_SEND_TIMEOUT % 1000) * 1000};
        void *p[] = {&socket_id, &net_type, url, &remote_port, &local_port};

        /* return error if task havn't creat */
        if (g_atsock_task_ztw_run == 0) {
            LOGE(TAG, "init");
            AT_BACK_RET_ERR(cmd, AT_ERR);
            return;
        }

        /* return error if net is link down */
        if (g_network_status == false) {
            LOGE(TAG, "net");
            AT_BACK_RET_ERR(cmd, AT_ERR);
            return;
        }

        /* parse param */
        ret = at_parse_param_safe("%4d,%4d,%" STR(ATSOCKET_URL_MAX_LEN) "s,%4d,%4d", (char *)data, p);

        if (ret < 0 || (ret != 4 && ret != 5)) {
            LOGE(TAG, "parse");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        /* check param */
        if ((socket_id < ATSOCKET_CTX_MIN_NUM || socket_id > ATSOCKET_CTX_MAX_NUM) ||
            (net_type != ATSOCKET_NETTYPE_UDP && net_type != ATSOCKET_NETTYPE_TCP) ||
            (remote_port <= 0 || remote_port > 65535)) {
            LOGE(TAG, "check");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        if (local_port != -1 && ((local_port <= 0 || local_port > 65535))) {
            LOGE(TAG, "port");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        /* check ctx status */
        if ((ctx = get_atsock_ctx(socket_id - 1)) && ctx->status == USED) {
            LOGE(TAG, "used");
            AT_BACK_RET_ERR(cmd, AT_ERR_STATUS);
            return;
        }

        /* do dns */
        memset((char *)&hints, 0x00, sizeof(hints));
        hints.ai_socktype = SOCK_RAW;
        hints.ai_family = AF_INET;

        ret = getaddrinfo(url, NULL, &hints, &res);

        if (ret != 0) {
            LOGE(TAG, "dns");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
            switch (ainfo->ai_family) {
                case AF_INET:
                    //len = ainfo->ai_addrlen;
                    /* record result to ctx */
                    memcpy(&(ctx->remote_addr), ainfo->ai_addr, sizeof(struct sockaddr_in));
                    /* set a flag */
                    ret = 1;
                    //ctx->remote_addr = (struct sockaddr_in *)ainfo->ai_addr;
                    break;

                default:
                    ;
            }
        }

        freeaddrinfo(res);

        if (ret != 1) {
            LOGE(TAG, "ip");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        /* create socket */
        ctx->net_type = net_type;

        if (ctx->net_type == TCP) {
            sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        } else if (ctx->net_type == UDP) {
            sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        }

        if (sockfd < 0) {
            LOGE(TAG, "socket");
            AT_BACK_RET_ERR(cmd, AT_ERR_START);
            return;
        }

        /* create socket sucess, record sockfd */
        ctx->socket_fd = sockfd;

        /* bind local port */
        local.sin_port = htons(local_port);
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);

        /* set send timeout to avoid send block */
        if (setsockopt(ctx->socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&interval,
                       sizeof(struct timeval))) {
            AT_BACK_RET_ERR(cmd, AT_ERR_SEND);
            return;
        }

        if (local_port > 0) {
            /* NOTICE: there is TIME_WAIT between close and next bind for same port, if in tcp mode */
            if ((ret = bind(ctx->socket_fd, (const struct sockaddr *)&local, sizeof(local))) != 0) {
                LOGE(TAG, "bind %d %d", ret, errno);
                err_code = AT_ERR_START;
                goto failed;
            }
        }

        /* connect to server */
        ctx->remote_addr.sin_port = htons(remote_port);

        if ((ret = connect(ctx->socket_fd, (struct sockaddr *) & (ctx->remote_addr), sizeof(ctx->remote_addr))) != 0) {
            LOGE(TAG, "connect %d %d", ret, errno);
            err_code = AT_ERR_START;
            goto failed;
        }

        if ((ctx->recv_buf = aos_malloc(ATSOCKET_RECV_MAX_LEN)) == NULL) {
            LOGE(TAG, "malloc %d", ret);
            err_code = AT_ERR_START;
            goto failed;
        }

        if ((ctx->send_buf = aos_malloc(ATSOCKET_SEND_MAX_LEN)) == NULL) {
            LOGE(TAG, "malloc %d", ret);
            err_code = AT_ERR_START;
            goto failed;
        }

        /* mutex create, used for socket send and recv */
        aos_mutex_new(&ctx->mutex);

        if (ctx->mutex.hdl == NULL) {
            LOGE(TAG, "mutex %d", ret);
            err_code = AT_ERR_START;
            goto failed;
        }

        /* zipopen sucess, set status to USED */
        ctx->status = USED;
        at_sock_task_wake();

        AT_BACK_OK;

        /* report stat */
        snprintf(ret_cmd, 20, "\r\n+ZIPSTAT:%d,1\r\n", socket_id);
        AT_BACK_STR(ret_cmd);
        return;

failed:
        free_atsock_ctx(ctx);

        AT_BACK_RET_ERR(cmd, err_code);

        return;
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

/* AT+ZIPSEND? */
/* AT+ZIPSEND=<socket_id>,<data_len>[,data] */
void at_cmd_zip_send(char *cmd, int type, char *data)
{
    if (type == READ_CMD) {
        int i;
        struct at_socket_ztw_context *ctx;
        char ret_cmd[28];

        for (i = 0; i < ATSOCKET_CTX_MAX_NUM; i ++) {
            ctx = get_atsock_ctx(i);

            if (ctx && ctx->status == USED) {

                snprintf(ret_cmd, 28, "%d,%d\r\n", i + 1, ctx->send_data_len);

                AT_BACK_RET_OK_HEAD(cmd, ret_cmd);
            }
        }

        AT_BACK_OK;
    } else if (type == WRITE_CMD) {
        int ret;
        int  data_len = -1, socket_id = -1;
        int count = 0;
        struct at_socket_ztw_context *ctx;
        void *p[] = {&socket_id, &data_len};

        /* parse param */
        ret = at_parse_param_safe("%4d,%4d", (char *)data, p);

        if (ret < 0 || (ret != 3 && ret != 2)) {
            LOGE(TAG, "parse");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        /* check param */
        if ((socket_id < ATSOCKET_CTX_MIN_NUM || socket_id > ATSOCKET_CTX_MAX_NUM) ||
            (data_len < 1 || data_len > ATSOCKET_SEND_MAX_LEN)) {
            LOGE(TAG, "check");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        /* check ctx status */
        if ((ctx = get_atsock_ctx(socket_id - 1)) && ctx->status == FREE) {
            LOGE(TAG, "no open");
            AT_BACK_RET_ERR(cmd, AT_ERR_STATUS);
            return;
        }

        /* send in diff mode with g_atsock_datamode */
        if (g_atsock_datamode == HEXSTR) {
            /* get data */
            void *pp[] = {&socket_id, &data_len, ctx->send_buf};
            ret = at_parse_param_safe("%4d,%4d,%" STR(ATSOCKET_SEND_MAX_LEN) "s", (char *)data, pp);

            if (ret < 0 || ret != 3) {
                LOGE(TAG, "parse");
                AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
                return;
            }

            /* string_hex convert */
            if ((data_len * 2 != strlen((char *)ctx->send_buf)) ||
                (data_len != at_hexstr2bytes((char *)ctx->send_buf))) {
                LOGE(TAG, "len");
                AT_BACK_CME_ERR(AT_ERR_INVAL);
                return;
            }
        } else if (g_atsock_datamode == STRING) {
            /* get data */
            void *pp[] = {&socket_id, &data_len, ctx->send_buf};
            ret = at_parse_param_safe("%4d,%4d,%" STR(ATSOCKET_SEND_MAX_LEN) "s", (char *)data, pp);

            if (ret < 0 || ret != 3) {
                LOGE(TAG, "parse");
                AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
                return;
            }

            /* len check */
            if (data_len != strlen((char *)(ctx->send_buf))) {
                LOGE(TAG, "len");
                AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
                return;
            }
        } else if (g_atsock_datamode == BINARY) {
            /* TODO: support BINARY mode */
            LOGE(TAG, "mode");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        } else {
            LOGE(TAG, "mode");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        /* send data to server */
        while (count < data_len) {
            aos_mutex_lock(&ctx->mutex, AOS_WAIT_FOREVER);
            ret = send(ctx->socket_fd, ctx->send_buf + count, data_len - count, 0);
            aos_mutex_unlock(&ctx->mutex);

            if (ret < 0) {
                LOGE(TAG, "send %d", errno);
                AT_BACK_RET_ERR(cmd, AT_ERR_SEND);
                return;
            }

            count += ret;
        }

        ctx->send_data_len = data_len;

        AT_BACK_RET_OK_INT2(cmd, socket_id, data_len);
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

/* AT+ZIPCLOSE=? */
/* AT+ZIPCLOSE? */
/* AT+ZIPCLOSE=scket_id */
void at_cmd_zip_close(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_RET_OK_CMD_HEAD(cmd);
        AT_BACK_STR("(" STR(ATSOCKET_CTX_MIN_NUM) "-" STR(ATSOCKET_CTX_MAX_NUM) ")\r\n");
        AT_BACK_OK;
    } else if (type == READ_CMD) {
        int i;
        struct at_socket_ztw_context *ctx;
        char ret_cmd[20];

        AT_BACK_RET_OK_CMD_HEAD(cmd);

        for (i = 0; i < ATSOCKET_CTX_MAX_NUM; i ++) {
            ctx = get_atsock_ctx(i);

            snprintf(ret_cmd, 20, "(%d,%d)", i + 1, ctx->status);

            AT_BACK_STR(ret_cmd);

            if (i == ATSOCKET_CTX_MAX_NUM - 1) {
                AT_BACK_STR("\r\n");
            } else {
                AT_BACK_STR(",");
            }
        }

        AT_BACK_OK;
    } else if (type == WRITE_CMD) {
        int ret;
        int socket_id = -1;
        struct at_socket_ztw_context *ctx;
        void *p[] = {&socket_id};
        char ret_cmd[20];

        /* parse param */
        ret = at_parse_param_safe("%4d", (char *)data, p);

        if (ret < 0 || ret != 1) {
            LOGE(TAG, "parse");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        /* check param */
        if ((socket_id < ATSOCKET_CTX_MIN_NUM || socket_id > ATSOCKET_CTX_MAX_NUM)) {
            LOGE(TAG, "check");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        /* check ctx status */
        if ((ctx = get_atsock_ctx(socket_id - 1)) && ctx->status == FREE) {
            LOGE(TAG, "free");
            AT_BACK_RET_ERR(cmd, AT_ERR_STATUS);
            return;
        }

        /* report ok first */
        AT_BACK_OK;

        /* free ctx */
        free_atsock_ctx(ctx);

        /* report stat */
        snprintf(ret_cmd, 20, "\r\n+ZIPSTAT:%d,0\r\n", socket_id);
        AT_BACK_STR(ret_cmd);

        return;

    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }

}

/* AT+ZIPSTAT=? */
/* AT+ZIPSTAT=socket_id */
void at_cmd_zip_stat(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_RET_OK_CMD_HEAD(cmd);
        AT_BACK_STR("(0-4)\r\n");
        AT_BACK_OK;
    } else if (type == WRITE_CMD) {
        int ret;
        int socket_id = -1;
        struct at_socket_ztw_context *ctx;
        char ret_cmd[20];
        void *p[] = {&socket_id};

        /* parse param */
        ret = at_parse_param_safe("%4d", (char *)data, p);

        if (ret < 0 || ret != 1) {
            LOGE(TAG, "parse");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        /* check param */
        if ((socket_id < ATSOCKET_CTX_MIN_NUM || socket_id > ATSOCKET_CTX_MAX_NUM)) {
            LOGE(TAG, "check");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        ctx = get_atsock_ctx(socket_id - 1);

        AT_BACK_RET_OK_CMD_HEAD(cmd);

        snprintf(ret_cmd, 20, "(%d,%d)\r\n", socket_id, ctx->status);

        AT_BACK_STR(ret_cmd);

        AT_BACK_OK;
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

/* AT+ZDTMODE=? */
/* AT+ZDTMODE? */
/* AT+ZDTMODE=mode */
void at_cmd_zdt_mode(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_RET_OK_CMD_HEAD(cmd);
        AT_BACK_STR("(1-3)\r\n");
        AT_BACK_OK;
    } else if (type == READ_CMD) {
        AT_BACK_RET_OK_INT(cmd, g_atsock_datamode);
    } else if (type == WRITE_CMD) {
        int ret;
        int mode = -1;
        void *p[] = {&mode};

        /* parse param */
        ret = at_parse_param_safe("%4d", (char *)data, p);

        if (ret < 0 || ret != 1) {
            LOGE(TAG, "parse");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        /* check param */
        if ((mode < STRING || mode > BINARY)) {
            LOGE(TAG, "parse");
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
            return;
        }

        /* data send mode */
        g_atsock_datamode = mode;

        AT_BACK_RET_OK_INT(cmd, mode);
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

static void sub_event_cb(uint32_t event_id, const void *param, void *context)
{
    if (event_id == EVENT_NETMGR_GOT_IP) {
        g_network_status = 1;
    } else if (event_id == EVENT_NETMGR_NET_DISCON) {
        g_network_status = 0;
    }
}

int at_socket_ztw_init(void)
{
    int ret;

    /* sem create, used for task wake up */
    aos_sem_new(&g_attask_ztw_sem, 0);

    if (g_attask_ztw_sem.hdl == NULL) {
        LOGE(TAG, "sem");
        return -1;
    }


    /* sem create, used for task exit */
    aos_sem_new(&g_atsock_ztw_sem, 0);

    if (g_atsock_ztw_sem.hdl == NULL) {
        LOGE(TAG, "sem");
        return -1;
    }

    g_atsock_task_ztw_run = 1;

    /* create socket task for recv */
    ret = csi_kernel_task_new(atsocket_task_entry, "atsocket_task", NULL,
                              ATSOCKET_TASK_PRI, 0, NULL,
                              ATSOCKET_TASK_STACK_SIZE,
                              &(g_atsock_task_ztw_handle));

    if (ret != 0) {
        LOGE(TAG, "task %d", ret);
        return -1;
    }

    event_subscribe(EVENT_NETMGR_GOT_IP, sub_event_cb, NULL);
    event_subscribe(EVENT_NETMGR_NET_DISCON, sub_event_cb, NULL);

    return 0;
}

int at_socket_ztw_deinit(void)
{
    g_atsock_task_ztw_run = 0;

    /* wait task exit, call task_del to exit task is not a safe way */
    aos_sem_wait(&g_atsock_ztw_sem, AOS_WAIT_FOREVER);

    if (g_atsock_ztw_sem.hdl) {
        aos_sem_free(&g_atsock_ztw_sem);
        g_atsock_ztw_sem.hdl = NULL;
    }

    if (g_attask_ztw_sem.hdl) {
        aos_sem_free(&g_attask_ztw_sem);
        g_attask_ztw_sem.hdl = NULL;
    }

    return 0;
}

#endif /* CONFIG_AT_SOCKET_ZTW */
#endif
