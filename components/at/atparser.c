/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <aos/aos.h>
#include <yoc/atparser.h>
#include "yoc/at_port.h"

#include "internal.h"

#ifdef LF
    #undef LF
    #define LF  10
#else
    #define LF  10
#endif

#ifdef CR
    #undef CR
    #define CR  13
#else
    #define CR  13
#endif


typedef enum {
    ATPARSER_SEND_CMD,
    ATPARSER_RECV_CMD,
    ATPARSER_RECV_STR_CMD,
    ATPARSER_OOB_CREATE_CMD,
    ATPARSER_OOB_DELETE_CMD,
    ATPARSER_TMOUT_CMD,
    ATPARSER_TERMINATOR_CMD,
    ATPARSER_INTERRUPT_CMD,
    ATPARSER_EXIT_CMD,
    ATPARSER_CLR_BUF_CMD,
    ATPARSER_WRITE_CMD,
    ATPARSER_DEBUG_CMD,
    ATPARSER_END_CMD
} ATPARSER_CMD;

typedef enum {
    IDLE_MODE,
    OOB_MODE,
    CMD_MODE,

    END_MODE
} ATPARSER_MODE;

typedef struct oob {
    unsigned char len;
    const char *prefix;
    oob_callback cb;
    void *context;
    slist_t next;
} oob_t;

typedef enum {
    LINE_TYPE,
    STR_TYPE
} RECV_TYPE;

typedef struct {
    const char *response;
    va_list args;
    int type;
} recv_line_t;

typedef struct {
    const void *data;
    int size;
} write_data_t;

struct atparser_uservice {
    //comm begin
    void *dev;
    int timeout;
    const char *output_terminator;
    char dbg_on;
    //comm end
    char *buffer;
    int  recv_size;
    int  buffer_size;
    int  min_size;
    int  max_size;
    int  step_size;
    char *buf_bake;
    volatile int have_uart_event;

    ATPARSER_MODE mode;
    int cmd_mode_flag;
    oob_t *oob;
    slist_t oob_head;
    //uservice begin
    uservice_t *srv;
    rpc_t res_cmd;
    recv_line_t *res_cmd_param;
    rpc_t oob_cmd;
    //uservice end
    aos_mutex_t mutex;
    at_channel_t *channel;
};

extern at_channel_t uart_channel;
#define ATPARSER_LOCK(at) aos_mutex_lock(&(at->mutex), AOS_WAIT_FOREVER)
#define ATPARSER_UNLOCK(at) aos_mutex_unlock(&(at->mutex))

static int atparser_process_rpc(void *context, rpc_t *rpc);
static int atparser_data_push(atparser_uservice_t *at);
static int detch_oob(atparser_uservice_t *at, int *offset);


#define atparser_channel_send(at, data, size, timeout) at->channel->send(at->dev, data, size)
#define atparser_channel_recv(at, data, size, timeout) at->channel->recv(at->dev, data, size, timeout)

static void uart_event(int id, void *priv)
{
    if (id == AT_CHANNEL_EVENT_READ) {
        atparser_uservice_t *at = (atparser_uservice_t *)priv;

        if (at->have_uart_event < 2) {
            at->have_uart_event++;
            atparser_data_push(at);
        }
    }
}

atparser_uservice_t *atparser_init(utask_t *task, const char *name, uart_config_t *config)
{
    atparser_uservice_t *at;

    aos_check_return_val(task, NULL);

    at = aos_zalloc(sizeof(atparser_uservice_t));

    if (at == NULL) {
        return NULL;
    }

    at->buffer_size = BUF_MIN_SIZE;
    at->min_size = BUF_MIN_SIZE;
    at->max_size = BUF_MAX_SIZE;
    at->step_size = BUF_STEP_SIZE;
    at->buffer = aos_malloc_check(BUF_MIN_SIZE);
    at->channel = &uart_channel;

    at->timeout = 8000;
    at->output_terminator = "\r\n";
    slist_init(&at->oob_head);

    at->dev = at->channel->init(name, config);

    if (at->dev == NULL) {
        goto fail_0;
    }

    // uart_set_buffer_size(at->dev, 1024*2);
    // if (config) {
    //     uart_config(at->dev, config);
    // }

    at->srv = uservice_new("atparser", atparser_process_rpc, at);

    if (at->srv == NULL) {
        uart_close(at->dev);
        goto fail_0;
    }

    utask_add(task, at->srv);
    at->channel->set_event(at->dev, uart_event, at);

    aos_mutex_new(&at->mutex);

    return at;

fail_0:
    aos_free(at->buffer);
    aos_free(at);

    return NULL;
}

atparser_uservice_t *atparser_channel_init(utask_t *task, const char *name, void *config, at_channel_t *channel)
{
    atparser_uservice_t *at;

    aos_check_return_val(task && channel, NULL);

    if (channel->init == NULL || channel->recv == NULL || \
        channel->send == NULL || channel->set_event == NULL) {
        return NULL;
    }

    at = aos_zalloc(sizeof(atparser_uservice_t));

    if (at == NULL) {
        return NULL;
    }

    at->buffer_size = BUF_MIN_SIZE;
    at->min_size = BUF_MIN_SIZE;
    at->max_size = BUF_MAX_SIZE;
    at->step_size = BUF_STEP_SIZE;
    at->buffer = aos_malloc_check(BUF_MIN_SIZE);
    at->channel = channel;

    at->timeout = 8000;
    at->output_terminator = "\r\n";
    slist_init(&at->oob_head);

    at->dev = at->channel->init(name, config);

    if (at->dev == NULL) {
        goto fail_0;
    }

    at->srv = uservice_new("atparser", atparser_process_rpc, at);

    if (at->srv == NULL) {
        goto fail_0;
    }

    utask_add(task, at->srv);
    at->channel->set_event(at->dev, uart_event, at);

    aos_mutex_new(&at->mutex);

    return at;

fail_0:
    aos_free(at->buffer);
    aos_free(at);

    return NULL;
}

static int debug_cmd_handle(atparser_uservice_t *at, rpc_t *rpc)
{
    at->dbg_on = rpc_get_int(rpc);
    return 0;
}

void atparser_debug_control(atparser_uservice_t *at, int flag)
{
    rpc_t rpc;

    aos_check_param(at);

    if (rpc_init(&rpc, ATPARSER_DEBUG_CMD, AOS_WAIT_FOREVER) == 0) {
        rpc_put_int(&rpc, flag);
        uservice_call(at->srv, &rpc);
        rpc_deinit(&rpc);
    }
}

static int oob_create_cmd_handle(atparser_uservice_t *at, rpc_t *rpc)
{
    oob_t *oob;
    int ret;
    oob_t *oob1 = (oob_t *)rpc_get_point(rpc);

    slist_for_each_entry(&at->oob_head, oob, oob_t, next) {
        if (strcmp(oob->prefix, oob1->prefix) == 0) {
            ret = -EINVAL;
            goto out;
        }
    }

    oob = aos_malloc(sizeof(struct oob));

    if (oob == NULL) {
        ret = -ENOMEM;
        goto out;
    }

    oob->len = strlen(oob1->prefix);
    oob->prefix = oob1->prefix;
    oob->cb = oob1->cb;
    oob->context = oob1->context;

    slist_add(&oob->next, &at->oob_head);
    ret = 0;

out:
    rpc_put_reset(rpc);
    rpc_put_int(rpc, ret);

    return 0;
}

int atparser_oob_create(atparser_uservice_t *at, const char *prefix, oob_callback cb, void *context)
{
    int ret;
    int rpc_ret;
    oob_t oob;

    aos_check_return_einval(at && prefix && strlen(prefix) > 0 && cb);

    oob.prefix = prefix;
    oob.cb = cb;
    oob.context = context;
    ret = uservice_call_sync(at->srv, ATPARSER_OOB_CREATE_CMD, &oob, &rpc_ret, sizeof(int));

    return ret < 0 ? ret : rpc_ret;
}

static int oob_delete_cmd_handle(atparser_uservice_t *at, rpc_t *rpc)
{
    oob_t *oob;
    char *prefix = (char *)rpc_get_point(rpc);
    int ret = -EINVAL;

    slist_for_each_entry(&at->oob_head, oob, oob_t, next) {
        if (strcmp(oob->prefix, prefix) == 0) {
            slist_del(&oob->next, &at->oob_head);
            aos_free(oob);
            ret = 0;
            break;
        }
    }

    rpc_put_reset(rpc);
    rpc_put_int(rpc, ret);

    return 0;
}

int atparser_oob_delete(atparser_uservice_t *at, const char *prefix)
{
    int ret;
    int rpc_ret = -1;
    rpc_t rpc;

    aos_check_return_einval(at && prefix);

    ret = rpc_init(&rpc, ATPARSER_OOB_DELETE_CMD, -1);

    if (ret == 0) {
        rpc_put_point(&rpc, prefix);
        ret = uservice_call(at->srv, &rpc);

        if (ret == 0) {
            rpc_ret = rpc_get_int(&rpc);
        }

        rpc_deinit(&rpc);
    }

    return ret < 0 ? ret : rpc_ret;
}

static void set_terminator(atparser_uservice_t *at, const char *terminator)
{
    at->output_terminator = terminator;
}

void atparser_set_terminator(atparser_uservice_t *at, const char *terminator)
{
    aos_check_param(at && terminator);
    uservice_call_sync(at->srv, ATPARSER_TERMINATOR_CMD, (void *)terminator, NULL, 0);
}

static void set_timeout(atparser_uservice_t *at, int timeout)
{
    at->timeout = timeout;
}

void atparser_set_timeout(atparser_uservice_t *at, int timeout)
{
    rpc_t rpc;

    aos_check_param(at);

    if (rpc_init(&rpc, ATPARSER_TMOUT_CMD, AOS_WAIT_FOREVER) == 0) {
        rpc_put_int(&rpc, timeout);
        uservice_call(at->srv, &rpc);
        rpc_deinit(&rpc);
    }
}

void atparser_clr_buf(atparser_uservice_t *at)
{
    rpc_t rpc;

    aos_check_param(at);

    if (rpc_init(&rpc, ATPARSER_CLR_BUF_CMD, AOS_WAIT_FOREVER) == 0) {
        uservice_call(at->srv, &rpc);
        rpc_deinit(&rpc);
    }
}

int atparser_write(atparser_uservice_t *at, const void *data, int size)
{
    write_data_t param;

    aos_check_return_einval(at && data && size);

    param.data = data;
    param.size = size;

    return uservice_call_sync(at->srv, ATPARSER_WRITE_CMD, &param, NULL, 0);
}

void atparser_cmd_exit(atparser_uservice_t *at)
{
    aos_check_param(at);
    uservice_call_sync(at->srv, ATPARSER_EXIT_CMD, NULL, NULL, 0);
}

static int atparser_data_push(atparser_uservice_t *at)
{
    uservice_call_async(at->srv, ATPARSER_INTERRUPT_CMD, NULL, 0);
    return 0;
}

static int write_cmd_handle(atparser_uservice_t *at, rpc_t *rpc)
{
    write_data_t *param = (write_data_t *)rpc_get_point(rpc);

    atparser_channel_send(at, param->data, param->size, at->timeout);

    return 0;
}

static int exit_cmd_handle(atparser_uservice_t *at, rpc_t *rpc)
{
    if (at->mode == CMD_MODE) {
        at->mode = IDLE_MODE;
    }
    at->cmd_mode_flag = 0;

    return 0;
}

static int tmout_cmd_handle(atparser_uservice_t *at, rpc_t *rpc)
{
    set_timeout(at, rpc_get_int(rpc));
    return 0;
}

static int terminator_cmd_handle(atparser_uservice_t *at, rpc_t *rpc)
{
    set_terminator(at, (char *)rpc_get_point(rpc));
    return 0;
}

static void atparser_move(atparser_uservice_t *at, int offset)
{
    memmove(at->buffer, at->buffer + offset, at->recv_size - offset);
    at->recv_size -= offset;
    at->buffer[at->recv_size] = '\0';
}

static char *read_line(atparser_uservice_t *at, int len, size_t *line_size)
{
    char *buffer = at->buffer;
    int i;
    int offset = -1;

    for (i = 0; i < len; i++) {
        if (offset < 0) {
            if (buffer[i] != '\r' && buffer[i] != '\n' && buffer[i] != 0) {
                offset = i;
            }
        } else {
            if (buffer[i] == '\r' || buffer[i] == '\n' || buffer[i] == 0) {
                buffer[i] = '\0';
                *line_size = i - offset + 1;

                return at->buffer + offset;
            }
        }
    }

    return NULL;
}

static void clr_line(atparser_uservice_t *at)
{
    size_t line_size;
    char *line;
    int offset;

    while(1) {
        offset = detch_oob(at, NULL);

        if(offset < 0 ) {
            offset = at->recv_size;
        }

        line = read_line(at, offset, &line_size);
        if(line != NULL) {
            atparser_move(at, line + line_size - at->buffer);
        } else {
            break;
        }
    }
}

static int clr_buf_cmd_handle(atparser_uservice_t *at, rpc_t *rpc)
{
    if (at->mode != OOB_MODE) {
        clr_line(at);
    }

    return 0;
}

static int full_buffer(atparser_uservice_t *at)
{
    int recv_size;
    int expected_size;

    expected_size = at->buffer_size - at->recv_size - 1;
    recv_size = atparser_channel_recv(at, at->buffer + at->recv_size, expected_size, 0);
    at->recv_size += recv_size;
    return recv_size;
}

static int detch_oob(atparser_uservice_t *at, int *offset)
{
    // find first oob offset
    oob_t *oob;

    for (int i = 0; i < at->recv_size; i++) {
        // Check for oob data
        slist_for_each_entry(&at->oob_head, oob, oob_t, next) {
            if (((at->recv_size - i) >= oob->len) && (memcmp(oob->prefix, at->buffer + i, oob->len) == 0)) {
                at->oob = oob;

                if (offset != NULL) {
                    *offset = i + oob->len;
                }

                return i;
            }
        }
    }

    return -1;
}

static int analysis_line(atparser_uservice_t *at, const char *response, va_list args, char *line)
{
    char *format = aos_malloc(strlen(response) * 2 + 3);

    if (format == NULL) {
        return -ENOMEM;
    }

    while (response[0]) {
        int i = 0;
        int offset = 0;

        while (response[i]) {
            if (response[i] == '%' && response[i + 1] != '%' && response[i + 1] != '*') {
                format[offset++] = '%';
                format[offset++] = '*';
                i++;
            } else {
                format[offset++] = response[i++];

                if (response[i - 1] == '\n' && !(i >= 3 && response[i - 3] == '[' && response[i - 2] == '^')) {
                    break;
                }
            }
        }

        format[offset++] = '%';
        format[offset++] = 'n';
        format[offset++] = 0;

        int count = -1;
        sscanf(line, format, &count);

        if (count == strlen(line)) {
            vsscanf(line, response, args);
            aos_free(format);
            return 0;
        } else {
            if(at->dbg_on) {
                LOGI("atparser","line(%s), format(%s), count=%d\n", line, format, count);
            }
            aos_free(format);
            return -EINVAL;
        }
    }

    aos_free(format);
    return -EINVAL;
}

int atparser_recv(atparser_uservice_t *at, const char *response, ...)
{
    int ret;
    int rpc_ret;
    recv_line_t param;
    rpc_ret = -1;
    rpc_t rpc;

    param.type = LINE_TYPE;

    aos_check_return_einval(at);

    ret = rpc_init(&rpc, ATPARSER_RECV_CMD, at->timeout);

    if (ret < 0) {
        return rpc_ret;
    }

    va_start(param.args, response);

    param.response = response;

    rpc_put_buffer(&rpc, &param, sizeof(param));
    ret = uservice_call(at->srv, &rpc);

    if (ret < 0) {
        ATPARSER_LOCK(at);
        if (at->res_cmd.srv != 0) {
            rpc_reply(&at->res_cmd);
            memset(&at->res_cmd, 0, sizeof(rpc_t));
        }
        ATPARSER_UNLOCK(at);
    } else {
        rpc_ret = rpc_get_int(&rpc);
    }

    rpc_deinit(&rpc);

    va_end(param.args);

    return rpc_ret;
}

int atparser_recv_str(atparser_uservice_t *at, const char *str)
{
    int ret;
    int rpc_ret;
    recv_line_t param;
    rpc_ret = -1;
    rpc_t rpc;

    aos_check_return_einval(at);

    param.type = STR_TYPE;

    ret = rpc_init(&rpc, ATPARSER_RECV_CMD, at->timeout);

    if (ret < 0) {
        return rpc_ret;
    }

    param.response = str;

    rpc_put_buffer(&rpc, &param, sizeof(param));
    ret = uservice_call(at->srv, &rpc);

    if (ret < 0) {
        ATPARSER_LOCK(at);
        if (at->res_cmd.srv != 0) {
            rpc_reply(&at->res_cmd);
            memset(&at->res_cmd, 0, sizeof(rpc_t));
        }
        ATPARSER_UNLOCK(at);
    } else {
        rpc_ret = rpc_get_int(&rpc);
    }

    rpc_deinit(&rpc);

    return rpc_ret;
}

static int send_cmd_handle(atparser_uservice_t *at, rpc_t *rpc)
{
    char *param = (char *)rpc_get_point(rpc);

    if (param) {
        atparser_channel_send(at, param, strlen(param), at->timeout);
        atparser_channel_send(at, at->output_terminator, strlen(at->output_terminator), at->timeout);
        at->mode = CMD_MODE;
        at->cmd_mode_flag = 1;
    }

    return 0;
}

int atparser_send(atparser_uservice_t *at, const char *command, ...)
{
    int ret;
    int count = 0;
    int max = at->timeout / 10;
    va_list args;
    char *send_buf;

    aos_check_return_einval(at);

    va_start(args, command);

    // Create and send command
    if (vasprintf(&send_buf, command, args) < 0) {
        va_end(args);
        return -EINVAL;
    }

    //  strcat(send_buf, at->output_terminator);
    va_end(args);

    while (at->mode != IDLE_MODE) {
        aos_msleep(10);
        count++;

        if (count > max) {
            aos_free(send_buf);
            return -1;
        }
    }

    ret = uservice_call_sync(at->srv, ATPARSER_SEND_CMD, send_buf, NULL, 0);

    aos_free(send_buf);
    return ret;
}

static int oob_process(atparser_uservice_t *at, oob_data_t *oob_data)
{
    oob_data->used_len = 0;
    oob_data->buf = at->buffer;
    oob_data->len = at->recv_size;

    int ret = at->oob->cb(at, at->oob->context, oob_data);

    atparser_move(at, oob_data->used_len);

    return ret;
}

static int idle_process(atparser_uservice_t *at, rpc_t *rpc)
{
    int offset;
    int ret = -1;

    ret = detch_oob(at, &offset);

    if (ret >= 0) {
        atparser_move(at, offset);
    }

    return (ret >= 0) ? 0 : -1;
}

static int cmd_line_process(atparser_uservice_t *at, recv_line_t *param, int offset)
{
    char *line;
    size_t line_size;

    if (offset < 0) { // no found oob
        line = read_line(at, at->recv_size, &line_size);

        if (line == NULL) {
            return -1;
        }
    } else if (offset > 0) { // found oob
        line = read_line(at, offset, &line_size);

        if (line == NULL) {
            return 0;
        }
    } else { // first byte is oobpa
        return 0;
    }

    int ret;

    ret = analysis_line(at, param->response, param->args, line);
    atparser_move(at, line + line_size - at->buffer);

    rpc_put_reset(&at->res_cmd);
    rpc_put_int(&at->res_cmd, ret);
    rpc_reply(&at->res_cmd);
    at->res_cmd.srv = NULL;

    return -2;
}

static int analysis_str(atparser_uservice_t *at, const char *str, int len)
{
    int ret = 0;
    char *buf = strstr(at->buffer, str);

    if(buf != NULL) {
        //\r\n+str   is ok
        //data1+str  is err
        int p_len = (int)(buf - at->buffer);

        for(int i = 0; i < p_len; i++) {
            if(at->buffer[i] != '\r' && at->buffer[i] != '\n') {
                ret = -1;
                return ret;
            }
        }
        atparser_move(at, len + p_len);
    } else {
        if(at->dbg_on) {
            LOGE("atparser", "str(%d):%s", at->recv_size, at->buffer);
            // asm("bkpt");
        }
        ret = -1;
    }

    return ret;
}

static int get_str_len(atparser_uservice_t *at, int len)
{
    int i;

    for(i = 0; i < len; i++) {
        if(at->buffer[i] != '\r' && at->buffer[i] != '\n') {
           break;
        }
    }

    return(len - i);
}

static int cmd_str_process(atparser_uservice_t *at, recv_line_t *param, int offset)
{
    int size;
    int len = strlen(param->response);

    if(offset < 0) {
        size = get_str_len(at, at->recv_size);

        if(size < len) {
            return -1;
        }
    } else if (offset > 0) {
        size = get_str_len(at, offset);

        if(size < len) {
            return 0;
        }
    } else {
        return 0;
    }

    int ret = analysis_str(at, param->response, len);

    rpc_put_reset(&at->res_cmd);
    rpc_put_int(&at->res_cmd, ret);
    rpc_reply(&at->res_cmd);
    at->res_cmd.srv = NULL;

    return -2;
}

static int cmd_process(atparser_uservice_t *at, rpc_t *rpc)
{
    int offset = detch_oob(at, NULL);

    if(offset >= 0 && at->res_cmd.srv == NULL) {
        for (int i = 0; i < offset; i++) {
            if(at->buffer[i] != '\r' && at->buffer[i] != '\n') {
                return -3;
            }
        }
        return 0;
    }

    if (at->mode == CMD_MODE && at->res_cmd.srv != NULL) {
        int ret;
        int size;

        rpc_get_reset(&at->res_cmd);
        recv_line_t *param = (recv_line_t *)rpc_get_buffer(&at->res_cmd, &size);
        aos_check_param(size == sizeof(recv_line_t));

        if(param->type == LINE_TYPE) {
            ret = cmd_line_process(at, param, offset);
        } else {
            ret = cmd_str_process(at, param, offset);
        }

        return ret;
    }

    return -1;
}

static int recv_cmd_handle(atparser_uservice_t *at, rpc_t *rpc)
{
    memcpy(&at->res_cmd, rpc, sizeof(at->res_cmd));
    ATPARSER_LOCK(at);
    cmd_process(at, NULL);
    ATPARSER_UNLOCK(at);

    return -1;
}

static oob_data_t *oob_data_new(void)
{
    oob_data_t *p = aos_zalloc(sizeof(oob_data_t));

    if (p != NULL) {
        p->time_stamp = aos_now_ms();
    }

    return p;
}

static void oob_data_destroy(oob_data_t **p)
{
    aos_free(*p);
    *p = NULL;
}

static int interrupt_cmd_handle(atparser_uservice_t *at, rpc_t *rpc)
{
    int len = 0;
    int change = 0;
    static oob_data_t *oob_data;

    while (1) {
        len = full_buffer(at);

        if ((len <= 0) && (change == 0) && at->mode != OOB_MODE) {
            break;
        }

        at->buffer[at->recv_size] = 0;

        switch (at->mode) {
            case OOB_MODE:
                change = 0;

                if (oob_data == NULL) {
                    oob_data = oob_data_new();
                }

                if (oob_process(at, oob_data) == 0) {
                    if (at->cmd_mode_flag == 0) {
                        at->mode = IDLE_MODE;
                    } else {
                        at->mode = CMD_MODE;
                    }

                    oob_data_destroy(&oob_data);
                    change = 1;
                }

                break;

            case IDLE_MODE:
                change = 0;

                if (idle_process(at, rpc) == 0) {
                    at->mode = OOB_MODE;
                    change = 1;
                }

                break;

            case CMD_MODE:
                change = 0;

                ATPARSER_LOCK(at);
                if (cmd_process(at, rpc) == 0) {
                    at->mode = IDLE_MODE;
                    change = 1;
                }
                ATPARSER_UNLOCK(at);

                break;

            default:
                break;
        }
    }

    at->have_uart_event--;
    return -1;
}

static const rpc_process_t c_atparser_cmd_cb_table[] = {
    {ATPARSER_SEND_CMD,             (process_t)send_cmd_handle},
    {ATPARSER_EXIT_CMD,             (process_t)exit_cmd_handle},
    {ATPARSER_OOB_CREATE_CMD,       (process_t)oob_create_cmd_handle},
    {ATPARSER_OOB_DELETE_CMD,       (process_t)oob_delete_cmd_handle},
    {ATPARSER_RECV_CMD,             (process_t)recv_cmd_handle},
    {ATPARSER_TMOUT_CMD,            (process_t)tmout_cmd_handle},
    {ATPARSER_TERMINATOR_CMD,       (process_t)terminator_cmd_handle},
    {ATPARSER_INTERRUPT_CMD,        (process_t)interrupt_cmd_handle},
    {ATPARSER_CLR_BUF_CMD,          (process_t)clr_buf_cmd_handle},
    {ATPARSER_WRITE_CMD,            (process_t)write_cmd_handle},
    {ATPARSER_DEBUG_CMD,            (process_t)debug_cmd_handle},

    {ATPARSER_END_CMD, NULL},
};

static int atparser_process_rpc(void *context, rpc_t *rpc)
{
    return uservice_process(context, rpc, c_atparser_cmd_cb_table);
}
