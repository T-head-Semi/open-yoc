/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#if defined(CONFIG_SAL) || defined(CONFIG_TCPIP)
#if defined (CONFIG_AT_SOCKET_HISI)
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <aos/aos.h>
#include <yoc/netdrv.h>
#include "at_internal.h"
#include "at_event.h"

static char *const TAG =  "at_sock";

#define S(x) #x
#define STR(x) S(x)

#define ATSOCKET_CTX_MAX_NUM      2
#define ATSOCKET_CTX_MIN_NUM      1
#define ATSOCKET_URL_MAX_LEN      96
#define ATSOCKET_SEND_MAX_LEN     1028
#define ATSOCKET_RECV_MAX_LEN     516
#define ATSOCKET_SEND_TIMEOUT     2000
#define ATSOCKET_RECV_TIMEOUT     100
#define ATSOCKET_TASK_PRI         KPRIO_NORMAL_BELOW7
#define ATSOCKET_TASK_STACK_SIZE  768

enum at_socket_status {
    FREE = 0,
    USED = 1,
};

enum at_socket_recv_ctrl {
    IGNORED = 0,
    RECEIVE = 1,
};

enum at_socket_send_flag {
    NONE = 0,
    HIGH_PRI = 1,
    SEND_PSM = 2,
    RECV_PSM = 4,
};

struct at_socket_hisi_context {
    int socket_fd;                                        /**< fd for socket */
    enum at_socket_status status;                         /**< ctx status */
    enum at_socket_send_flag send_flag;                   /**< send lpm flag */
    enum at_socket_recv_ctrl recv_ctrl;                   /**< receive control flag */
    struct sockaddr_in    remote_addr;                    /**< remote server address */
    aos_mutex_t mutex;                               /**< mutex for send and recv */
    uint8_t  *recv_buf;                                   /**< recv buf */
    uint32_t  recv_len;                                   /**< recv data len */
    uint32_t  recv_remained_len;                          /**< recv data remained len */
    uint8_t  *send_buf;                                   /**< send buf */
};

static struct at_socket_hisi_context g_atsock_hisi_ctx[ATSOCKET_CTX_MAX_NUM] = {0}; /**< ctx for each socket_id */
static aos_sem_t g_atsock_hisi_sem = { NULL };                                /**< sem for task exit */
static aos_sem_t g_attask_hisi_sem = { NULL };                                /**< sem for task wake */
static aos_task_t g_atsock_hisi_task_handle = { NULL };                       /**< task handle for recv and report */
static uint8_t g_atsock_hisi_task_run = 0;                                     /**< task flag for task exit */
static uint8_t g_network_status = 0;

static struct at_socket_hisi_context *get_atsock_ctx(uint8_t id)
{
    if (id >= ATSOCKET_CTX_MAX_NUM) {
        return NULL;
    } else {
        return &(g_atsock_hisi_ctx[id]);
    }
}

static int32_t at_sock_task_wait(void)
{
    return aos_sem_wait(&g_attask_hisi_sem, AOS_WAIT_FOREVER);
}

static int32_t at_sock_task_wake(void)
{
    aos_sem_signal(&g_attask_hisi_sem);

    return AT_OK;
}

static void free_atsock_ctx(struct at_socket_hisi_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    /* set FERR first to avoid ctx conflict */
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
        aos_msleep(10);
    }
}
#ifdef CONFIG_YOC_LPM
int32_t at_socket_hisi_lpm_hdl()
{
    uint8_t i;
    struct at_socket_hisi_context *ctx;

    for (i = 0; i < ATSOCKET_CTX_MAX_NUM; i ++) {
        ctx = get_atsock_ctx(i);

        if (ctx && ctx->status == USED) {
            free_atsock_ctx(ctx);
            AT_LOGI(TAG, "hisi socket=%d close", i + 1);
        }
    }

    return 0;
}
#endif

/* at socket task for recv */
static void atsocket_task_entry(void *arg)
{
    int ret;
    struct at_socket_hisi_context *ctx;
    fd_set read_fds;
    int count = 0, i;
    struct timeval interval = {ATSOCKET_RECV_TIMEOUT / 1000, (ATSOCKET_RECV_TIMEOUT % 1000) * 1000};

    while (g_atsock_hisi_task_run) {

        int max_fd = 0;
        int isselect = 0;
        int needsleep = 1;
        FD_ZERO(&read_fds);

        /* FD_SET for used ctx and check recv_remained_len is need read */
        for (i = 0; i < ATSOCKET_CTX_MAX_NUM; i ++) {
            if ((ctx = get_atsock_ctx(i)) && ctx->status == USED) {
                needsleep = 0;

                /* we need to recv another packet, cause user have read all remained data */
                if (ctx->recv_remained_len == 0) {
                    isselect = 1;
                    FD_SET(ctx->socket_fd, &read_fds);

                    if (ctx->socket_fd > max_fd) {
                        max_fd = ctx->socket_fd;
                    }
                }
            }
        }

        /* sleep, if there is no used ctx */
        if (needsleep) {
            at_sock_task_wait();
            //aos_msleep(ATSOCKET_RECV_TIMEOUT);
            continue;
        }

        if (isselect == 1) {
            ret = select(max_fd + 1, &read_fds, NULL, NULL, &interval);

            /* Zero fds ready means we timed out */
            if (ret == 0) {
                continue;
            }

            if (ret < 0) {
                LOGE(TAG, "select %d", errno);
                continue;
            }
        } else {
            /* sleep awhile to yield */
            aos_msleep(ATSOCKET_RECV_TIMEOUT);
            continue;
        }

        /* check FD_ISSET and recv data */
        for (i = 0; i < ATSOCKET_CTX_MAX_NUM; i ++) {
            if ((ctx = get_atsock_ctx(i)) &&
                ctx->status == USED &&
                ctx->recv_remained_len == 0 &&
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
                    continue;
                }

                /* peer closed */
                if (count == 0) {
                    free_atsock_ctx(ctx);
                    continue;
                }

                /* if +NSOCR config recv_ctrl as RECEIVE, report recv data */
                if (ctx->recv_ctrl == RECEIVE) {
                    ctx->recv_len = count;
                    ctx->recv_remained_len = count;
                    back_at_cmd_print(25, "\r\n+NSONMI: %d,%d\r\n", i + 1, count);
                }
            }
        }
    }

    aos_sem_signal(&g_atsock_hisi_sem);
}

/* AT+NSOCR=<type>,<protocol>,<listen port>[,<recvice_control>] */
void at_cmd_nsocr(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret, i;
        int err_code = AT_OK;
        int protocol, listen_port, recvive_contrl = 1;
        char type[10];
        int sockfd = -1;
        int socket_id = -1;
        struct at_socket_hisi_context *ctx;
        struct sockaddr_in local = {0};
        struct timeval interval = {ATSOCKET_SEND_TIMEOUT / 1000, (ATSOCKET_SEND_TIMEOUT % 1000) * 1000};
        void *p[] = {&type, &protocol, &listen_port, &recvive_contrl};

        /* return error if task havn't creat */
        if (g_atsock_hisi_task_run == 0) {
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
        ret = at_parse_param_safe("%10s,%4d,%4d,%4d", (char *)data, p);

        if (ret < 0 && (ret != 4 && ret != 3)) {
            LOGE(TAG, "parse");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* check param */
        if ((strcmp(type, "DGRAM") != 0) || (protocol != 17) ||
            (recvive_contrl < IGNORED || recvive_contrl > RECEIVE) ||
            (listen_port <= 0 || listen_port > 65535)) {
            LOGE(TAG, "check");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* get free ctx */
        for (i = 0; i < ATSOCKET_CTX_MAX_NUM; i ++) {
            if ((ctx = get_atsock_ctx(i)) && ctx->status == FREE) {
                socket_id = i;
                break;
            }
        }

        if (socket_id < 0) {
            LOGE(TAG, "used");
            AT_BACK_CME_ERR(AT_ERR_STATUS);
            return;
        }

#if 0
        struct addrinfo *res, *ainfo;
        struct addrinfo hints;
        struct sockaddr_in local = {'\0'};
        /* do dns */
        memset((char *)&hints, 0x00, sizeof(hints));
        hints.ai_socktype = SOCK_RAW;
        hints.ai_family = AF_INET;

        ret = getaddrinfo(url, NULL, &hints, &res);

        if (ret != 0) {
            LOGE(TAG, "dns");
            AT_BACK_RET_ERR(at_get_cmd_name(id), AT_ERR_INVAL);
            return;
        }

        for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
            switch (ainfo->ai_family) {
                case AF_INET:
                    len = ainfo->ai_addrlen;
                    /* record result to ctx */
                    memcpy(&(ctx->remote_addr), ainfo->ai_addr, sizeof(struct sockaddr_in));
                    //ctx->remote_addr = (struct sockaddr_in *)ainfo->ai_addr;
                    break;

                default:
                    ;
            }
        }

        freeaddrinfo(res);
#endif

        /* create socket */
        sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (sockfd < 0) {
            AT_BACK_CME_ERR(AT_ERR_START);
            return;
        }

        /* create socket sucess, record sockfd */
        ctx->socket_fd = sockfd;
        ctx->remote_addr.sin_family = AF_INET;
        ctx->remote_addr.sin_len = sizeof(ctx->remote_addr);

        /* bind local port */
        local.sin_port = htons(listen_port);
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);

        /* set send timeout to avoid send block */
        if (setsockopt(ctx->socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&interval,
                       sizeof(struct timeval))) {
            AT_BACK_CME_ERR(AT_ERR_SEND);
            return;
        }

        if ((ret = bind(ctx->socket_fd, (const struct sockaddr *)&local, sizeof(local))) != 0) {
            err_code = AT_ERR_START;
            goto failed;
        }

        if ((ctx->recv_buf = aos_malloc(ATSOCKET_RECV_MAX_LEN)) == NULL) {
            err_code = AT_ERR_START;
            goto failed;
        }

        if ((ctx->send_buf = aos_malloc(ATSOCKET_SEND_MAX_LEN)) == NULL) {
            err_code = AT_ERR_START;
            goto failed;
        }

#if 0

        if (ringbuffer_create(&ctx->recv_rb, ctx->recv_buf, ATSOCKET_RECV_MAX_LEN)) {
            LOGE(TAG, "rb %d", ret);
            err_code = AT_ERR_START;
            goto failed;
        }

#endif

        /* mutex create, used for socket send and recv */
        aos_mutex_new(&ctx->mutex);

        if (ctx->mutex.hdl == NULL) {
            LOGE(TAG, "mutex %d", ret);
            err_code = AT_ERR_START;
            goto failed;
        }

        /* zipopen sucess, set status to USED */
        ctx->status = USED;
        ctx->recv_ctrl = recvive_contrl;
        at_sock_task_wake();

        /* return socket_idi, +1 for avoid 0 */
        AT_BACK_OK_INT(socket_id  + 1);
        return;

failed:
        free_atsock_ctx(ctx);

        AT_BACK_CME_ERR(err_code);

        return;
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

/* AT+NSOST=<socket_id>,<remote_addr>,<remote_port>,<length>,<data> */
void at_cmd_nsost(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret;
        int count = 0;
        int  data_len = -1, socket_id = -1, remote_port = -1;
        char url[ATSOCKET_URL_MAX_LEN];
        struct at_socket_hisi_context *ctx;
        struct sockaddr_in remote_addr = {0};
        void *p[] = {&socket_id, url, &remote_port, &data_len};

        /* parse param */
        ret = at_parse_param_safe("%4d,%" STR(ATSOCKET_URL_MAX_LEN) "s,%4d,%4d", (char *)data, p);

        if (ret < 0 || ret != 5) {
            LOGE(TAG, "parse");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* check param */
        if ((socket_id < ATSOCKET_CTX_MIN_NUM || socket_id > ATSOCKET_CTX_MAX_NUM) ||
            ((ctx = get_atsock_ctx(socket_id - 1)) && ctx->status == FREE) ||
            (remote_port <= 0 || remote_port > 65535) ||
            (data_len < 1 || data_len > ATSOCKET_SEND_MAX_LEN)) {
            LOGE(TAG, "check");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* convert ip to addr */
        if (inet_pton(AF_INET, url, &remote_addr.sin_addr) != 1) {
            LOGE(TAG, "ip");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        remote_addr.sin_port = ntohs(remote_port);
        remote_addr.sin_family = AF_INET;

        /* get data */
        void *pp[] = {&socket_id, url, &remote_port, &data_len, ctx->send_buf};
        /* parse param */
        ret = at_parse_param_safe("%4d,%" STR(ATSOCKET_URL_MAX_LEN)
                                  "s,%4d,%4d,%" STR(ATSOCKET_SEND_MAX_LEN) "s",
                                  (char *)data, pp);

        if (ret < 0 || ret != 5) {
            LOGE(TAG, "parse");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* string_hex convert */
        if ((data_len * 2 != strlen((char *)ctx->send_buf)) ||
            (data_len != at_hexstr2bytes((char *)ctx->send_buf))) {
            LOGE(TAG, "len");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* send data to server */
        while (count < data_len) {
            aos_mutex_lock(&ctx->mutex, AOS_WAIT_FOREVER);
            ret = sendto(ctx->socket_fd, ctx->send_buf + count,
                         data_len - count, 0,
                         (const struct sockaddr *)&remote_addr,
                         sizeof(remote_addr));
            aos_mutex_unlock(&ctx->mutex);

            if (ret < 0) {
                LOGE(TAG, "send %d", errno);
                AT_BACK_CME_ERR(AT_ERR_SEND);
                return;
            }

            count += ret;
        }

        /* clear send_flag which was set in nsostf */
        ctx->send_flag = NONE;

        AT_BACK_OK_INT2(socket_id, data_len);
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

/* AT+NSOSTF=<socket_id>,<remote_addr>,<remote_port>,<flag>,<length>,<data> */
void at_cmd_nsostf(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret;
        int count = 0;
        int  data_len = -1, socket_id = -1, remote_port = -1;
        char url[ATSOCKET_URL_MAX_LEN], flag[8];
        struct at_socket_hisi_context *ctx;
        struct sockaddr_in remote_addr = {0};
        void *p[] = {&socket_id, url, &remote_port, flag, &data_len};

        /* parse param */
        ret = at_parse_param_safe("%4d,%" STR(ATSOCKET_URL_MAX_LEN) "s,%4d,%8s,%4d", (char *)data, p);

        if (ret < 0 || ret != 6) {
            LOGE(TAG, "parse");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* check param */
        if ((socket_id < ATSOCKET_CTX_MIN_NUM || socket_id > ATSOCKET_CTX_MAX_NUM) ||
            ((ctx = get_atsock_ctx(socket_id - 1)) && ctx->status == FREE) ||
            (remote_port <= 0 || remote_port > 65535) ||
            (data_len < 1 || data_len > ATSOCKET_SEND_MAX_LEN)) {
            LOGE(TAG, "check");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* TODO: support flag 0x200 0x400, 0x200 0x400 is not supported currently */
        if (strcmp(flag, "0") == 0) {
            ctx->send_flag = NONE;
        } else if (strcmp(flag, "0x100") == 0) {
            ctx->send_flag = HIGH_PRI;
        } else if (strcmp(flag, "0x200") == 0) {
            ctx->send_flag = SEND_PSM;
        } else if (strcmp(flag, "0x400") == 0) {
            ctx->send_flag = RECV_PSM;
        } else {
            LOGE(TAG, "flag");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* convert ip to addr */
        if (inet_pton(AF_INET, url, &remote_addr.sin_addr) != 1) {
            LOGE(TAG, "ip");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        remote_addr.sin_port = ntohs(remote_port);
        remote_addr.sin_family = AF_INET;

        /* get data */
        void *pp[] = {&socket_id, url, &remote_port, flag, &data_len, ctx->send_buf};
        /* parse param */
        ret = at_parse_param_safe("%4d,%" STR(ATSOCKET_URL_MAX_LEN)
                                  "s,%4d,%8s,%4d,%" STR(ATSOCKET_SEND_MAX_LEN) "s",
                                  (char *)data, pp);

        if (ret < 0 || ret != 6) {
            LOGE(TAG, "parse");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* string_hex convert */
        if ((data_len * 2 != strlen((char *)ctx->send_buf)) ||
            (data_len != at_hexstr2bytes((char *)ctx->send_buf))) {
            LOGE(TAG, "len");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* send data to server */
        while (count < data_len) {
            aos_mutex_lock(&ctx->mutex, AOS_WAIT_FOREVER);
            ret = sendto(ctx->socket_fd, ctx->send_buf + count,
                         data_len - count, 0,
                         (const struct sockaddr *)&remote_addr, sizeof(remote_addr));
            aos_mutex_unlock(&ctx->mutex);

            if (ret < 0) {
                LOGE(TAG, "send %d", errno);
                AT_BACK_CME_ERR(AT_ERR_SEND);
                return;
            }

            count += ret;
        }

        /* goto psm mode if sendflag is set */
        if (ctx->send_flag == SEND_PSM) {
            at_event_send(AT_MSG_EVENT, AT_EVENT_LPM, 0, 0, NULL);
        }

        AT_BACK_OK_INT2(socket_id, data_len);
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

/* AT+NSORF=<socket_id>,<req_length> */
void at_cmd_nsorf(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret;
        int socket_id = -1, need_len;
        struct at_socket_hisi_context *ctx;
        void *p[] = {&socket_id, &need_len};

        /* parse param */
        ret = at_parse_param_safe("%4d,%4d", (char *)data, p);

        if (ret < 0 || ret != 2) {
            LOGE(TAG, "parse");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* check param */
        if ((socket_id < ATSOCKET_CTX_MIN_NUM || socket_id > ATSOCKET_CTX_MAX_NUM) ||
            (need_len <= 0 || need_len > ATSOCKET_RECV_MAX_LEN)) {
            LOGE(TAG, "check");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* check ctx status */
        if ((ctx = get_atsock_ctx(socket_id - 1)) && ctx->status == FREE) {
            LOGE(TAG, "free");
            AT_BACK_CME_ERR(AT_ERR_STATUS);
            return;
        }

        if (ctx->recv_remained_len <= 0) {
            LOGE(TAG, "len");
            AT_BACK_CME_ERR(AT_ERR_STATUS);
            return;
        }

        /* if we do not have enough needed len, return all remained data */
        if (ctx->recv_remained_len < need_len) {
            need_len = ctx->recv_remained_len;
        }

        /* malloc a buffer for at cmd report */
        char *atcmd = aos_malloc(need_len * 2 + 38);

        if (atcmd == NULL) {
            AT_BACK_CME_ERR(AT_ERR_STATUS);
            return;
        }

        memset(atcmd, 0, need_len * 2 + 38);

        /* report recv data: socket_id,ip,remote_port,read_len,data,remained_len data is hexstr */
        sprintf(atcmd, "%d,", socket_id);

        /* conver ip to string */
        inet_ntop(AF_INET, &ctx->remote_addr.sin_addr, atcmd + strlen(atcmd), 16);

        sprintf(atcmd + strlen(atcmd), ",%d,%d,", ntohs(ctx->remote_addr.sin_port), need_len);

        at_bytes2hexstr(atcmd + strlen(atcmd),
                        ctx->recv_buf + ctx->recv_len - ctx->recv_remained_len, need_len);

        sprintf(atcmd + strlen(atcmd), ",%d\r\n", ctx->recv_remained_len - need_len);

        ctx->recv_remained_len -= need_len;

        /* report recv data */
        AT_BACK_STR(atcmd);

        aos_free(atcmd);

        /* goto psm mode if sendflag is set */
        if (ctx->recv_remained_len == 0 && ctx->send_flag == RECV_PSM) {
            at_event_send(AT_MSG_EVENT, AT_EVENT_LPM, 0, 0, NULL);
        }

        AT_BACK_OK;
        return;
    } else {
        LOGE(TAG, "run %s %d", cmd, type);
    }
}

void at_cmd_nsocl(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int ret;
        int socket_id = -1;
        struct at_socket_hisi_context *ctx;
        void *p[] = {&socket_id};

        /* parse param */
        ret = at_parse_param_safe("%4d", (char *)data, p);

        if (ret < 0 || ret != 1) {
            LOGE(TAG, "parse");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* check param */
        if ((socket_id < ATSOCKET_CTX_MIN_NUM || socket_id > ATSOCKET_CTX_MAX_NUM)) {
            LOGE(TAG, "check");
            AT_BACK_CME_ERR(AT_ERR_INVAL);
            return;
        }

        /* check ctx status */
        if ((ctx = get_atsock_ctx(socket_id - 1)) && ctx->status == FREE) {
            LOGE(TAG, "free");
            AT_BACK_CME_ERR(AT_ERR_STATUS);
            return;
        }

        /* free ctx */
        free_atsock_ctx(ctx);

        AT_BACK_OK;
        return;
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

int at_socket_hisi_init(void)
{
    int ret;

    /* sem create, used for task wake up */
    aos_sem_new(&g_attask_hisi_sem, 0);

    if (g_attask_hisi_sem.hdl == NULL) {
        LOGE(TAG, "sem");
        return -1;
    }

    /* sem create, used for task exit */
    aos_sem_new(&g_atsock_hisi_sem, 0);

    if (g_atsock_hisi_sem.hdl == NULL) {
        LOGE(TAG, "sem");
        return -1;
    }

    g_atsock_hisi_task_run = 1;

    /* create socket task for recv */
    ret = aos_task_new_ext(&g_atsock_hisi_task_handle, "atsocket_task", atsocket_task_entry, NULL,
                         ATSOCKET_TASK_STACK_SIZE, ATSOCKET_TASK_PRI);

    if (ret != 0) {
        LOGE(TAG, "task %d", ret);
        return -1;
    }

    event_subscribe(EVENT_NETMGR_GOT_IP, sub_event_cb, NULL);
    event_subscribe(EVENT_NETMGR_NET_DISCON, sub_event_cb, NULL);

    return 0;
}

int at_socket_hisi_deinit(void)
{
    g_atsock_hisi_task_run = 0;

    /* wait task exit, call task_del to exit task is not a safe way */
    aos_sem_wait(&g_atsock_hisi_sem, AOS_WAIT_FOREVER);

    if (g_atsock_hisi_sem.hdl) {
        aos_sem_free(&g_atsock_hisi_sem);
        g_atsock_hisi_sem.hdl = NULL;
    }

    if (g_attask_hisi_sem.hdl) {
        aos_sem_free(&g_attask_hisi_sem);
        g_attask_hisi_sem.hdl = NULL;
    }

    return 0;
}

#endif /* CONFIG_AT_SOCKET_HISI */
#endif
