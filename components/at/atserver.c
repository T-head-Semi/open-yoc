/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#define _GNU_SOURCE
#include <aos/aos.h>
#include <yoc/atserver.h>
#include "yoc/at_port.h"
#include "csi_core.h"

#define TAG "atserver"

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


#define CHANNEL_BUF_SIZE 16

#define CHECK_RETURN_VAL(x, ret)                  \
        do {                                        \
            if (!(x)) {                         \
                return ret;                         \
            }                                       \
        } while(0);

#define CHECK_RETURN(x)                  \
        do {                                        \
            if (!(x)) {                         \
                return;                          \
            }                                       \
        } while(0);
typedef struct atcmd_node {
    const atserver_cmd_t   *at;
    int                     len; //cmd total num
    slist_t                 next;
} atcmd_node_t;

typedef enum {
    NORMAL_MODE,
    PASS_THROUGH_MODE
} ATSERVER_MODE;

typedef enum {
    RUNNING,
    STOP,
} ATSERVER_STATUE;

typedef struct {
    ATSERVER_MODE mode;
    char *data;
    int recv_size;
    int len;
    pass_through_cb cb;
} atserver_mode_t;

typedef struct atserver {
    char            *buffer;
    uint16_t         buffer_size;
    uint16_t         recv_size;
    char             channel_buffer[CHANNEL_BUF_SIZE];
    uint8_t          channel_count;
    uint8_t          channel_inc;
    uint8_t          echo_flag;
    char             para_delimiter;
    volatile int     have_channel_event;
    ATSERVER_STATUE  statue;

    int              timeout;
    const char      *output_terminator;
    slist_t          cmd_head;
    uservice_t      *srv;
    void            *dev;
    aos_mutex_t      mutex;
    atserver_mode_t  at_mode;
    at_channel_t    *channel;
} atserver_uservice_t;

extern at_channel_t uart_channel;

typedef enum {
    ATSERVER_SEND_CMD,
    ATSERVER_OUTPUT_CMD,
    ATSERVER_PARA_CMD,
    ATSERVER_TMOUT_CMD,
    ATSERVER_INTERRUPT_CMD,
    ATSERVER_PASS_THROUGH_CMD,
    ATSERVER_STOP_CMD,
    ATSERVER_RESUME_CMD,
    ATSERVER_SHOW_CMD,
    ATSERVER_END_CMD
} ATSERVER_CMD;

#define atmode(at) at.at_mode

static int atserver_process_rpc(void *context, rpc_t *rpc);

static atserver_uservice_t g_atserver;

static int atserver_channel_send(const char *data, size_t size, size_t timeout) 
{
    if (size == 0) {
        return 0;
    }

    int ret = g_atserver.channel->send(g_atserver.dev, (void *)data, size);

    return ret;
}

int atserver_channel_recv(char *data)
{
    if (g_atserver.channel_inc == g_atserver.channel_count) {
        int ret = g_atserver.channel->recv(g_atserver.dev, g_atserver.channel_buffer, CHANNEL_BUF_SIZE, 0);
        if (ret > 0) {
            g_atserver.channel_count = ret;
            g_atserver.channel_inc = 0;
        }
    }

    if (g_atserver.channel_inc < g_atserver.channel_count) {
        *data = g_atserver.channel_buffer[g_atserver.channel_inc++];

        return 1;
    }

    return 0;
}


static void channel_event(int event_id, void *priv)
{
    uint32_t flags = 0;
	int ret = 0;
    if (event_id == AT_CHANNEL_EVENT_READ) {
        if (g_atserver.have_channel_event < 2) {
            ret = uservice_call_async(g_atserver.srv, ATSERVER_INTERRUPT_CMD, NULL, 0);
			if(ret < 0) {
			   return;
			}
	        flags = csi_irq_save();
            g_atserver.have_channel_event++;
			csi_irq_restore(flags);
         }
    }
}

int atserver_init(utask_t *task, const char *name, uart_config_t *config)
{
    aos_assert(task);

    memset(&g_atserver, 0, sizeof(atserver_uservice_t));

    g_atserver.channel = &uart_channel;
    //uart init
    g_atserver.dev = g_atserver.channel->init(name, config);

    if (g_atserver.dev == NULL) {
        return -1;
    }

    g_atserver.buffer_size = BUFFER_MIN_SIZE;
    g_atserver.buffer = aos_malloc(BUFFER_MIN_SIZE);
    aos_assert(g_atserver.buffer);

    g_atserver.timeout = 500;
    g_atserver.output_terminator = "\r\n";
    g_atserver.para_delimiter = ',';
    slist_init(&g_atserver.cmd_head);

    //uservice
    g_atserver.srv = uservice_new("atserver", atserver_process_rpc, NULL);
    aos_assert(g_atserver.srv);

    utask_add(task, g_atserver.srv);

    g_atserver.channel->set_event(g_atserver.dev, channel_event, NULL);

    aos_mutex_new(&g_atserver.mutex);

    g_atserver.statue = RUNNING;

    return 0;
}

int atserver_channel_init(utask_t *task, const char *name, void *config, at_channel_t *channel)
{
    aos_assert(task);

    memset(&g_atserver, 0, sizeof(atserver_uservice_t));

    g_atserver.channel = channel;
    //uart init
    g_atserver.dev = g_atserver.channel->init(name, config);

    if (g_atserver.dev == NULL) {
        return -1;
    }

    g_atserver.buffer_size = BUFFER_MIN_SIZE;
    g_atserver.buffer = aos_malloc(BUFFER_MIN_SIZE);
    aos_assert(g_atserver.buffer);

    g_atserver.timeout = 500;
    g_atserver.output_terminator = "\r\n";
    g_atserver.para_delimiter = ',';
    slist_init(&g_atserver.cmd_head);

    //uservice
    g_atserver.srv = uservice_new("atserver", atserver_process_rpc, NULL);
    aos_assert(g_atserver.srv);

    utask_add(task, g_atserver.srv);

    g_atserver.channel->set_event(g_atserver.dev, channel_event, NULL);

    aos_mutex_new(&g_atserver.mutex);

    g_atserver.statue = RUNNING;

    return 0;
}

static int atserver_cmd_check(const char *cmd)
{
    atcmd_node_t *node;

    slist_for_each_entry(&g_atserver.cmd_head, node, atcmd_node_t, next) {
        atserver_cmd_t *atcmd = (atserver_cmd_t *)node->at;

        for (int i = 0; i < node->len; i++) {
            if (strcmp(cmd, atcmd->cmd) == 0) {
                return (-EINVAL);
            }

            atcmd++;
        }
    }
    return 0;
}

static int atserver_cmd_check_group(const atserver_cmd_t cmd[])
{
    atserver_cmd_t *at_cmd = (atserver_cmd_t *)cmd;
    int len = 0;

    while (at_cmd->cmd != NULL) {
        int ret = atserver_cmd_check(at_cmd->cmd);

        if (ret < 0) {
            return ret;
        }

        at_cmd++;
        len++;
    }

    return len;
}

void atserver_set_echo(uint8_t flag)
{
    g_atserver.echo_flag = flag;
}

uint8_t atserver_get_echo(void)
{
    return g_atserver.echo_flag;
}

int atserver_add_command(const atserver_cmd_t cmd[])
{
    int len;

    aos_check_return_einval(cmd);

    len = atserver_cmd_check_group(cmd);

    if (len < 0) {
        return (-EINVAL);
    }

    atcmd_node_t *node = (atcmd_node_t *)aos_malloc(sizeof(atcmd_node_t));
    if (node) {
        node->at = cmd;
        node->len = len;
        slist_add(&node->next, &g_atserver.cmd_head);
        return 0;
    } else {
        return -ENOMEM;
    }
}

int atserver_cmd_link(const atserver_cmd_t *at_cmd)
{
    int ret = -1;

    aos_check_return_einval(at_cmd && at_cmd->cb && at_cmd->cmd && strlen(at_cmd->cmd) > 0);

    ret = atserver_cmd_check(at_cmd->cmd);

    if (ret < 0) {
        return ret;
    }

    atcmd_node_t *node = (atcmd_node_t *)aos_malloc(sizeof(atcmd_node_t));
    if (node) {
        node->at = at_cmd;
        node->len = 1;
        slist_add(&node->next, &g_atserver.cmd_head);

        return 0;
    }

    return -ENOMEM;
}

void atserver_set_output_terminator(const char *terminator)
{
    uservice_call_sync(g_atserver.srv, ATSERVER_OUTPUT_CMD, terminator ? (void *)terminator : "", NULL, 0);
}

void atserver_set_para_delimiter(char delimiter)
{
    uservice_call_sync(g_atserver.srv, ATSERVER_PARA_CMD, (void *)&delimiter, NULL, 0);
}

void atserver_set_timeout(int timeout)
{
    uservice_call_sync(g_atserver.srv, ATSERVER_TMOUT_CMD, (void *)&timeout, NULL, 0);
}

void atserver_stop(void)
{
    uservice_call_sync(g_atserver.srv, ATSERVER_STOP_CMD, NULL, NULL, 0);
}

void atserver_resume(void)
{
    uservice_call_sync(g_atserver.srv, ATSERVER_RESUME_CMD, NULL, NULL, 0);
}

void atserver_show_command(void)
{
    uservice_call_sync(g_atserver.srv, ATSERVER_SHOW_CMD, NULL, NULL, 0);
}

static void atserver_clear_buffer(void)
{
    if (g_atserver.buffer_size >= BUFFER_MIN_SIZE) {
        aos_free(g_atserver.buffer);

        g_atserver.buffer = aos_malloc(BUFFER_MIN_SIZE);
        g_atserver.buffer_size = g_atserver.buffer ? BUFFER_MIN_SIZE : 0;
    }

    g_atserver.recv_size = 0;
}

static int atserver_get_cmdline(void)
{
    char c = 0;
    int len = 0;
    int flag = 0;

    if (g_atserver.recv_size != 0) {
        flag = 1;
        len = g_atserver.recv_size;
    }

    while (1) {
        int ret = atserver_channel_recv(&c);

        if (ret == 0) {
            if (flag == 1) {
                g_atserver.recv_size = len;
            }

            return 0;
        }

        if (len >= BUFFER_MAX_SIZE - 1) {
            LOGW(TAG, "buffer overflow");

            len = 0; // clear buffer
        }

        g_atserver.buffer[len++] = c;

        if (len >= g_atserver.buffer_size) {
            char *p;
            p = realloc(g_atserver.buffer, g_atserver.buffer_size + BUFFER_STEP_SIZE);

            if (p != NULL) {
                g_atserver.buffer = p;
                g_atserver.buffer_size += BUFFER_STEP_SIZE;
            } else {
                len = 0; // clear buffer
            }
        }

        if (flag == 0 && len > 1) {
            if ((g_atserver.buffer[len - 2] == 'A') && (g_atserver.buffer[len - 1] == 'T')) {
                flag = 1;
                g_atserver.buffer[0] = 'A';
                g_atserver.buffer[1] = 'T';
                len = 2;
            }
        } else if (flag == 1 && (c == LF || c == CR)) {
            g_atserver.buffer[len - 1] = '\0';
            g_atserver.recv_size = len - 1;

            return len - 1;
        }
    }
}

static int get_cmd_len(AT_CMD_TYPE *type)
{
    int i;
    char *str = g_atserver.buffer;

    for (i = 0; i <= strlen(g_atserver.buffer); i++) {
        if (*str == '=') {
            if (*(str + 1) == '?') {
                *type = TEST_CMD;
            } else {
                *type = WRITE_CMD;
            }

            return i;
        } else if (*str == '?') {
            *type = READ_CMD;
            return i;
        } else if (*str == '\0') {
            *type = EXECUTE_CMD;
            return i;
        } else {
            str++;
        }
    }

    return -1;
}

static int get_para_num(void)
{
    int argc = 0;
    char *p;
    char para_delimiter = g_atserver.para_delimiter;

    p = strstr(g_atserver.buffer, "=");

    if (p != NULL) {
        if (*(p + 1) != '\0') {
            argc = 1;

            while (*p) {
                if (*p == para_delimiter) {
                    argc++;
                }

                p++;
            }
        } else {
            return -1;
        }
    }

    return argc;
}

static int para_cut(char *buf, char **argv)
{
    int argc = 0;
    char *p;
    int i;
    char para_delimiter = g_atserver.para_delimiter;

    if (buf == NULL) {
        return argc;
    }

    argv[argc++] = buf;

    while (*buf != '\0') {
        if (*buf == para_delimiter) {
            *buf = '\0';

            if ((*(buf + 1) == '\0') || (*(buf + 1) == para_delimiter)) {
                argv[argc++] = NULL;
            } else {
                argv[argc++] = buf + 1;
            }
        }

        buf++;
    } //cut para

    for (i = 0; i < argc; i++) {
        p = argv[i];

        while (*p != '\0') {
            if (*p != ' ') {
                break;
            } else {
                p++;
            }
        }

        argv[i] = p;
    } //remove head space

    for (i = 0; i < argc; i++) {
        p = argv[i] + strlen(argv[i]) - 1;

        while (*p != '\0') {
            if (*p == ' ') {
                //*p = '\0';
                p--;
            } else {
                p[1] = '\0';
                break;
            }
        }
    } //remove tail space

    return argc;
}

static int cmd_process(void)
{
    int len;
    AT_CMD_TYPE type = INVAILD_CMD;
    atcmd_node_t *node;

    len = get_cmd_len(&type);

    if (len < 0) {
        return -1;
    }

    slist_for_each_entry(&g_atserver.cmd_head, node, atcmd_node_t, next) {
        atserver_cmd_t *atcmd = (atserver_cmd_t *)node->at;

        for (int i = 0; i < node->len; i++) {
            if ((strncmp(g_atserver.buffer, atcmd->cmd, len) == 0) && (len == strlen(atcmd->cmd))) {
                if (atcmd->cb != NULL) {
                    char *param = NULL;

                    if (type == WRITE_CMD) {
                        param = strstr(g_atserver.buffer, "=");
                        param++;
                    }
                    atserver_lock();
                    atcmd->cb((char *)atcmd->cmd, type, param);
                    atserver_unlock();
                    return 0;
                }
            }

            atcmd++;
        }
    }

    return -1;
}

static int get_next_line(void)
{
    char *p_pre = g_atserver.buffer;
    char *p = p_pre;

    if (strlen(g_atserver.buffer) >= 2) {
        if (p_pre) {
            p = strstr(p_pre + 2, "AT");
        }

        if (p != NULL && p != g_atserver.buffer) {
            int len = strlen(p);

            memmove(g_atserver.buffer, p, len);
            g_atserver.buffer[len] = '\0';
            return 0;
        }
    }

    return -1;
}

static int pass_through(void)
{
    if (atmode(g_atserver).mode == PASS_THROUGH_MODE) {
        int ret, recv_size, offset;

        recv_size = atmode(g_atserver).len - atmode(g_atserver).recv_size;
        offset = atmode(g_atserver).recv_size;
        ret = g_atserver.channel->recv(g_atserver.dev, atmode(g_atserver).data + offset, recv_size, 0);
        atmode(g_atserver).recv_size += ret;

        if (atmode(g_atserver).recv_size >= atmode(g_atserver).len) {

            char *data = atmode(g_atserver).data;
            int len = atmode(g_atserver).len;

            atmode(g_atserver).cb(data, len);
            aos_free(atmode(g_atserver).data);
            memset(&g_atserver.at_mode, 0x00, sizeof(atserver_mode_t));

            return 0;
        }

        return -1;
    }

    return 0;
}

static int interrupt_cmd_handle(void *context, rpc_t *rpc)
{
    int ret = -1;
	uint32_t flags;

	if(g_atserver.statue != RUNNING) {
		flags = csi_irq_save();
		g_atserver.have_channel_event--;
		csi_irq_restore(flags);
		return 0;
	}

    if (pass_through() < 0) {
		flags = csi_irq_save();
        g_atserver.have_channel_event--;
        csi_irq_restore(flags);
        return 0;
    }

    while(1) {
        ret = atserver_get_cmdline();

        if (ret > 0) {
            if(g_atserver.echo_flag) {
                atserver_send("%s\r\n",g_atserver.buffer);
            }
            while (ret >= 0) {
                ret = cmd_process();

                if (ret < 0) {
                    ret = get_next_line();
                } else {
                    break;
                }
            }

            if (ret < 0) {
                const char *err = "\r\nERROR\r\n";
                atserver_write(err, strlen(err));
            }

            atserver_clear_buffer();
        }
        else {
            break;
        }
    }
	flags = csi_irq_save();
	g_atserver.have_channel_event --;
	csi_irq_restore(flags);

    return 0;
}

int atserver_scanf(const char *format, ...)
{
    va_list args;
    int argc;
    char **argv;
    int ret = -1;

    aos_check_return_einval(format);
    va_start(args, format);

    argc = get_para_num();

    if (argc > 0) {
        char *copy = strdup(g_atserver.buffer);
        if (copy == NULL) {
            va_end(args);
            return -ENOMEM;
        }

        argv = aos_malloc(sizeof(char *) * argc);
        if (argv == NULL) {
            aos_free(copy);
            va_end(args);
            return -ENOMEM;
        }

        char *param = NULL;

        param = strstr(copy, "=");
        param++;
        argc = para_cut(param, argv);

        int len = 0;
        int offset = 0;

        if (argv[0] != NULL) {
            len = strlen(argv[0]);
            memmove(copy,argv[0],len);
            copy[len] = 0;
            offset += len;
        }

        for (int i = 1; i < argc; i++) {
            copy[offset] = g_atserver.para_delimiter;
            offset++;
            len = 0;

            if (argv[i] != NULL) {
                len = strlen(argv[i]);
                memmove(copy + offset, argv[i],len);
                offset += len;
                copy[offset] = 0;
            }
        }

       // offset += len;
        copy[offset] = '\0';
        ret = vsscanf(copy, format, args);

        aos_free(argv);
        aos_free(copy);
    }

    va_end(args);

    if (ret < 0) {
        return -EINVAL;
    } else {
        return ret;
    }
}

int atserver_write(const void *data, int size)
{
    int ret = -1;

    aos_check_return_einval(data && size > 0);
    CHECK_RETURN_VAL(g_atserver.statue == RUNNING, -1);

    ret = atserver_channel_send(data, size, g_atserver.timeout);

    return ret;
}

int atserver_sendv(const char *command, va_list args)
{
    int ret = -EINVAL;
    char *send_buf = NULL;

    aos_check_return_einval(command);
    CHECK_RETURN_VAL(g_atserver.statue == RUNNING, -1);

    if (vasprintf(&send_buf, command, args) >= 0) {
        ret = atserver_channel_send(send_buf, strlen(send_buf), g_atserver.timeout);

        if (ret == 0 && g_atserver.output_terminator != NULL) {
            ret = atserver_channel_send(g_atserver.output_terminator, strlen(g_atserver.output_terminator), g_atserver.timeout);
        }

        aos_free(send_buf);
    }

    return ret;
}

int atserver_send(const char *command, ...)
{
    int ret;
    va_list args;

    aos_check_return_einval(command);
    CHECK_RETURN_VAL(g_atserver.statue == RUNNING, -1);

    va_start(args, command);

    ret = atserver_sendv(command, args);

    va_end(args);

    return ret;
}

int atserver_set_uartbaud(int baud)
{
    CHECK_RETURN_VAL(g_atserver.statue == RUNNING, -1);
    uart_config_t config;
    if (g_atserver.dev == NULL) {
        return -1;
    }
    memset(&config, 0, sizeof(config));
    uart_config_default(&config);
    config.baud_rate = baud;
    if (uart_config(g_atserver.dev, &config) < 0) {
        return -1;
    }
    return 0;
}

void atserver_lock(void)
{
    CHECK_RETURN(g_atserver.statue == RUNNING);
    aos_mutex_lock(&(g_atserver.mutex), AOS_WAIT_FOREVER);
}

void atserver_unlock(void)
{
    CHECK_RETURN(g_atserver.statue == RUNNING);
    aos_mutex_unlock(&(g_atserver.mutex));
}

int atserver_pass_through(int len, pass_through_cb cb)
{
    aos_check_return_einval(len && cb);
    CHECK_RETURN_VAL(g_atserver.statue == RUNNING, -1);

    atserver_mode_t atmode;

    atmode.mode = PASS_THROUGH_MODE;
    atmode.len  = len;
    atmode.cb   = cb;

    uservice_call_async(g_atserver.srv, ATSERVER_PASS_THROUGH_CMD, (void *)&atmode, sizeof(atserver_mode_t));

    return 0;
}

static int pass_through_handle(void *context, rpc_t *rpc)
{
    int len;
    atserver_mode_t *p = (atserver_mode_t *)rpc_get_buffer(rpc, &len);

    atmode(g_atserver).mode = p->mode;
    atmode(g_atserver).len = p->len;
    atmode(g_atserver).cb = p->cb;

    if (atmode(g_atserver).data != NULL) {
        aos_free(atmode(g_atserver).data);
    }
    atmode(g_atserver).data = aos_malloc_check(p->len);

    return 0;
}

static int send_cmd_handle(void *context, rpc_t *rpc)
{
    char *param = (char *)rpc_get_point(rpc);

    atserver_channel_send(param, strlen(param), g_atserver.timeout);
    return 0;
}

static int output_cmd_handle(void *context, rpc_t *rpc)
{
    g_atserver.output_terminator = (char *)rpc_get_point(rpc);
    return 0;
}

static int para_cmd_handle(void *context, rpc_t *rpc)
{
    g_atserver.para_delimiter = *(char *)rpc_get_point(rpc);
    return 0;
}

static int tmout_cmd_handle(void *context, rpc_t *rpc)
{
    g_atserver.timeout = rpc_get_int(rpc);
    return 0;
}

static int stop_cmd_handle(void *context, rpc_t *rpc)
{
    g_atserver.statue = STOP;
    atserver_clear_buffer();
    g_atserver.channel->set_event(g_atserver.dev, NULL, NULL);

    return 0;
}

static int resume_cmd_handle(void *context, rpc_t *rpc)
{
    g_atserver.statue = RUNNING;
    g_atserver.channel->set_event(g_atserver.dev, channel_event, NULL);

    return 0;
}

static int show_cmd_handle(void *context, rpc_t *rpc)
{
    atcmd_node_t *node;

    slist_for_each_entry(&g_atserver.cmd_head, node, atcmd_node_t, next) {
        atserver_cmd_t *atcmd = (atserver_cmd_t *)node->at;

        for (int i = 0; i < node->len; i++) {
            
            atserver_channel_send(atcmd->cmd+3, strlen(atcmd->cmd) - 3, g_atserver.timeout);
            atserver_channel_send(";\r\n", 3, g_atserver.timeout);

            atcmd++;
        }
    }
    return 0;
}

static const rpc_process_t c_atserver_cmd_cb_table[] = {
    {ATSERVER_SEND_CMD,         send_cmd_handle},
    {ATSERVER_OUTPUT_CMD,       output_cmd_handle},
    {ATSERVER_PARA_CMD,         para_cmd_handle},
    {ATSERVER_TMOUT_CMD,        tmout_cmd_handle},
    {ATSERVER_INTERRUPT_CMD,    interrupt_cmd_handle},
    {ATSERVER_PASS_THROUGH_CMD, pass_through_handle},
    {ATSERVER_STOP_CMD,         stop_cmd_handle},
    {ATSERVER_RESUME_CMD,       resume_cmd_handle},
    {ATSERVER_SHOW_CMD,         show_cmd_handle},

    {ATSERVER_END_CMD,           NULL},
};

static int atserver_process_rpc(void *context, rpc_t *rpc)
{
    return uservice_process(context, rpc, c_atserver_cmd_cb_table);
}
