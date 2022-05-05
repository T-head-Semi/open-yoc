/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#if defined(CONFIG_SAL) || defined(CONFIG_TCPIP)
#include <stdint.h>
#include <stdio.h>

#include <aos/debug.h>

#include "at_socket.h"
#include "../at_internal.h"

#define TAG "soket_cmd"

static int g_at_cip_id = 0;

recv_mode_t g_at_recv_mode = PT_MODE;

/* ============================================= */
/*    Socket AT CMD Implemention                 */
/* ============================================= */
void at_cmd_cip_recv(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        int err = AT_ERR_INVAL;

        if (g_at_cip_id >= MIN_CIP_ID && g_at_cip_id <= MAX_CIP_ID) {
            at_conn_t *conn = conn_find_by_id(g_at_cip_id);

            if (conn != NULL && g_at_recv_mode == CMD_MODE) {
                if (conn->status == STATS_CONNECT) {
                    if (conn->recv_size > 0) {
                        atserver_send(CRLF"%s:%d,%d,", cmd + 2, conn->id, conn->recv_size);
                        atserver_write(conn->recv_buf, conn->recv_size);
                        conn->recv_size = 0;
                        atserver_send(CRLF_OK_CRLF);
                        return;
                    }
                }
            }
            err = (conn == NULL || g_at_recv_mode != CMD_MODE)?AT_ERR_STATUS:AT_ERR_RECV;
        }

        AT_BACK_RET_ERR(cmd, err);
    }
}

void at_cmd_cip_start(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int        ret = AT_ERR_INVAL;
        at_conn_t *at_conn;

        /* parse the start arg */
        at_conn = at_parse_start_arg((char *)data);

        if ((at_conn != NULL) && (conn_find_by_id(at_conn->id) == NULL)) {
            /* set the status to connecting, so the next req will refuse */
            at_conn->status = STATS_CONNECTING;

            at_dump_conn_arg(at_conn);

            ret = at_connect_start(at_conn, cmd);

            if (ret == AT_OK) {
                g_at_cip_id = at_conn->id;
                LOGI(TAG, "start id %d \r\n", at_conn->id);
                return;
            } else if (ret == AT_ERR_INVAL) {
                aos_free(at_conn);
                AT_BACK_RET_ERR(cmd, ret);
                return;
            }
        }

        if(at_conn != NULL ) {
            ret = AT_ERR_STATUS;
            aos_free(at_conn);
        }

        AT_BACK_RET_ERR(cmd, ret);
    }
}
void at_cmd_cip_stop(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int cid, count;
        atserver_scanf("%*[^\0]%n", &count);
        if (count == 1 && atserver_scanf("%d", &cid) == 1) {
            if ((cid >= MIN_CIP_ID) && (cid <= MAX_CIP_ID + 1)) {
                if (cid <= MAX_CIP_ID) {
                    at_conn_t *at_conn = conn_find_by_id(cid);

                    if (at_conn == NULL) {
                        AT_BACK_RET_ERR(cmd, AT_ERR_STATUS);
                        return;
                    }
                    if(at_disconnect(at_conn, 1) == 0) {
                        AT_BACK_OK();
                        LOGI(TAG, "stop id %d \r\n", at_conn->id);
                    } else {
                        AT_BACK_RET_ERR(cmd, AT_ERR_STATUS);
                    }
                    return;
                } else {
                    at_conn_t *at_conn = conn_find_by_id(cid);
                    if (at_conn->type == TYPE_TCP_SERVER) {
                        at_disconnect(at_conn, 1);
                        AT_BACK_OK();
                        return;
                    }
                    if (at_count_conn() == 0) {
                        AT_BACK_RET_ERR(cmd, AT_ERR_STATUS);
                        return;
                    }

                    AT_BACK_OK();
                    at_disconnect_all();
                    return;
                }
            }
        }

        AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
    }
}

void at_cmd_cip_recv_cfg(char *cmd, int type, char *data)
{
    if (type == READ_CMD) {
        atserver_send(CRLF"%s:%d"CRLF_OK_CRLF, cmd + 2, g_at_recv_mode);
    } else if (type == WRITE_CMD) {
        if (strlen(data) == 1) {
            if (data[0] == '0' || data[0] == '1') {
                g_at_recv_mode = (data[0] == '0') ? PT_MODE : CMD_MODE;
                at_conn_t *conn = conn_find_by_id(g_at_cip_id);
                if( conn != NULL) {
                    if(g_at_recv_mode == CMD_MODE) {
                        conn->recv_size = 0;
                    }
                }
                AT_BACK_OK();
                return;
            }
        }

        AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
    }
}

void at_cmd_cip_id(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        atserver_send(CRLF"%s:%d-%d"CRLF_OK_CRLF, cmd + 2, MIN_CIP_ID, MAX_CIP_ID);
    } else if (type == READ_CMD) {
        if (g_at_cip_id >= MIN_CIP_ID && g_at_cip_id <= MAX_CIP_ID) {
            atserver_send(CRLF"%s:%d"CRLF_OK_CRLF, cmd + 2,g_at_cip_id);
        } else {
            AT_BACK_RET_ERR(cmd, AT_ERR_INVAL);
        }
    } else if (type == WRITE_CMD) {
        int cid = 0;
        int ret = AT_ERR_INVAL;
        int count = 1;

        if (atserver_scanf("%d%*[^\0]%n", &cid, &count) == 1) {
            if ((count == 1) && (cid >= MIN_CIP_ID) && (cid <= MAX_CIP_ID)) {
                at_conn_t *conn = conn_find_by_id(cid);

                if (conn != NULL || conn->status == STATS_CONNECT) {
                    g_at_cip_id = cid;
                    AT_BACK_OK();
                    return;
                }

                ret = AT_ERR_STATUS;
            }
        }

        AT_BACK_RET_ERR(cmd, ret);
    }
}

void at_cmd_cip_status(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        if (g_at_cip_id >= MIN_CIP_ID && g_at_cip_id <= MAX_CIP_ID) {
            at_conn_t * conn = conn_find_by_id(g_at_cip_id);

            if (conn != NULL) {
                const char *c_at_conn_status[] = {
                    "stop",
                    "start",
                    "connect",
                };
                atserver_send(CRLF"%s:%s,%s"CRLF_OK_CRLF, cmd + 2, at_conn_type_str(conn->type), c_at_conn_status[conn->status]);
                return;
            }
        }

        AT_BACK_RET_ERR(cmd,  AT_ERR_STATUS);
    }
}

static void at_socket_send(char *cmd, at_conn_t *conn, uint8_t *data, uint16_t len)
{
    int                ret;
    struct sockaddr_in sinhim    = {'\0'};
    struct sockaddr   *dest_addr = NULL;
    socklen_t          addrlen   = 0;

    if (conn->type == TYPE_UDP_UNICAST) {
        sinhim.sin_family      = AF_INET;
        sinhim.sin_addr.s_addr = conn->ip_addr.addr;
        sinhim.sin_port        = htons(conn->rport);
        dest_addr              = (struct sockaddr *)&sinhim;
        addrlen                = sizeof(struct sockaddr);
    }

    ret = sendto(conn->sockfd, data, len, MSG_DONTWAIT, dest_addr, addrlen);
    printf("send data:%s, len: %x, fd: %x\r\n", data, len, conn->sockfd);

    if (ret == -1) {
        LOGE(TAG, "sendto %d", errno);
        AT_BACK_RET_ERR(cmd, AT_ERR_SEND);
        return;
    }

    AT_BACK_RET_OK_INT(cmd, ret);
    return;
}


// CIP_SEND=4,AABB\r\n -> "AABB"
// CIP_SEND=2,AABB\r\n -> 0xAABB
void at_cmd_cip_send(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        int err = AT_ERR_INVAL;

        if (g_at_cip_id >= MIN_CIP_ID && g_at_cip_id <= MAX_CIP_ID) {
            at_conn_t *conn = conn_find_by_id(g_at_cip_id);

            if (conn->status == STATS_CONNECT) {
                int len;
                int data_cnt,len_cnt;

                atserver_scanf("%d,%n%*[^\0]%n", &len, &len_cnt, &data_cnt);
                data_cnt -= len_cnt;
                if (len <= 128 && data_cnt <= 128 && data_cnt == len) {
                    printf("len: %d, data: %s\r\n", len, data + len_cnt);
                    at_socket_send(cmd, conn, (uint8_t *)data + len_cnt, len);
                    return;
                }
            }

            err = (conn->status == STATS_CONNECT) ? AT_ERR_SEND : AT_ERR_STATUS;
        }

        AT_BACK_RET_ERR(cmd, err);
    }
}

#ifdef CONFIG_YOC_LPM
extern void delayed_action_sleep(void *arg);

void at_setup_cmd_cip_sendpsm(uint8_t id, uint16_t len, uint8_t *data)
{
    at_conn_t *conn;

    if (g_at_cip_id < MIN_CIP_ID || g_at_cip_id > MAX_CIP_ID) {
        LOGW(TAG, "send id=%d", g_at_cip_id);
        AT_BACK_RET_ERR(at_get_cmd_name(id), AT_ERR_INVAL);
        return;
    }

    conn = conn_find_by_id(g_at_cip_id);

    if (conn->status != STATS_CONNECT) {
        LOGW(TAG, "send tat=%d", conn->status);
        AT_BACK_RET_ERR(at_get_cmd_name(id), AT_ERR_STATUS);
        return;
    }

    at_socket_send(id, conn, data, len);
    return;
}
#endif
#endif