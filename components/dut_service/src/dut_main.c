/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdarg.h>

#include <aos/hal/uart.h>
#include <aos/hal/wdg.h>
#include <aos/aos.h>
#include "dut_utility.h"
#include "dut_service.h"

#define   NO_ADD    0x1
#define   ADD       0x2
#define   WHY       0x4
#define   DOU       0x8
#define   FIND      0x10

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define AT_OUTPUT_TERMINATION  "\r\n"
#define DUT_CMD_MAX_ARGS       15
#define DUT_CMD_MAX_ARG_LEN    40

typedef struct cmd_list {
    const dut_at_cmd_t *cmd;
    slist_t            next;
} cmd_list_t;

struct dut_service {
    dut_at_cmd_t   *cmd;
    aos_mutex_t    lock;
    uart_dev_t     uart_dev;

    slist_t        cmd_lists;
};

static struct dut_service dut_svr;
static char argv[DUT_CMD_MAX_ARGS][DUT_CMD_MAX_ARG_LEN] = {0};
static char *argqv[DUT_CMD_MAX_ARGS] = {0};

#define dut_svr_lock()   (aos_mutex_lock(&dut_svr.lock, AOS_WAIT_FOREVER))
#define dut_svr_unlock() (aos_mutex_unlock(&dut_svr.lock))

static int dut_disable_wdt()
{
    wdg_dev_t wdg_dev = {
        0,
        {2000},
        NULL
    };

    int ret = hal_wdg_init(&wdg_dev);
    if (ret != 0) {
        return -1;
    }

    ret = hal_wdg_finalize(&wdg_dev);
    if (ret != 0) {
        return -1;
    }

    return 0;
}

//////////////////////////////////////////////////
/* UART Send */
int dut_sendv(const char *command, va_list args)
{
    int ret = -EINVAL;
    char *send_buf = NULL;

    if (vasprintf(&send_buf, command, args) >= 0) {
        ret = hal_uart_send_poll(&dut_svr.uart_dev, send_buf, strlen(send_buf));

        ret |= hal_uart_send_poll(&dut_svr.uart_dev, AT_OUTPUT_TERMINATION, strlen(AT_OUTPUT_TERMINATION));

        free(send_buf);
    }

    return ret;
}

int dut_at_send(const char *command, ...)
{
    int ret;
    va_list args;

    aos_check_return_einval(command);

    va_start(args, command);

    ret = dut_sendv(command, args);

    va_end(args);

    return ret;
}

//////////////////////////////////////////////////
/* AT Parse */
static int at_cmd_type(char *s)
{
    int flag = 0;

    flag |= NO_ADD;

    for (int i = 0; * (s + i) != '\0'; i++) {
        if (*(s + i) == '+') {
            flag = flag & (~0x1);
            flag |= ADD;    //+ no =
        }

        if (*(s + i) == '=') { //, or not ,
            if (*(s + i + 1) == '?') {
                return  FIND;
            } else if (*(s + i + 1) != '\0') {
                return DOU;
            } else {
                return 0;
            }
        }

        if (*(s + i) == '?') {
            flag = flag & (~0x2); //no +
            flag |= WHY;
        }
    }

    return flag;
}

static void cmd_parse_func(int type, int argc, char **argv)
{
    int err = 0;
    cmd_list_t *node;

    if (argc > DUT_CMD_MAX_ARGS) {
        dut_at_send("+CME ERROR, ARGS OVERFLOW");
        return;
    }
    dut_svr_lock();

    slist_for_each_entry(&dut_svr.cmd_lists, node, cmd_list_t, next) {
        if (argv[1] != NULL && strcmp(argv[1], node->cmd->name) == 0) {
            if (type == DUT_CMD_EXECUTE || type == DUT_CMD_GET) {
                err = node->cmd->function(type, argc - 1, &argv[1]);
                if (err) {
                    dut_at_send("+CME ERROR, %d", err);
                } else {
                    dut_at_send("\r\nOK");
                }
            } else {
                dut_at_send("%s", node->cmd->help);
                dut_at_send("\r\nOK");
            }
            goto cmd_parse_exit;
        }
    }
    dut_at_send("+CME ERROR, NOT FOUND");
cmd_parse_exit:
    dut_svr_unlock();
    return;
}

__attribute__((unused)) static int is_at_cmd(char *data)
{
    if ((*data == 'A') && (*(data + 1) == 'T')) {
        return 1;
    }
    return 0;
}

static void dut_test_at(char data[])
{
    char *hcc;
    int ustype  = 0;

	memset((void *)argv, 0, sizeof(argv));
	memset((void *)argqv, 0, sizeof(argqv));

	ustype = at_cmd_type(data);
    argv[0][0] = 'A';
    argv[0][1] = 'T';
    argv[0][2] = '\0';

    switch (ustype) {
        case NO_ADD:
            dut_at_send("OK");
            break;

        case ADD://execute
            if (char_cut(argv[1], data, '+', '\0') == NULL) {
                goto dut_main_exit;
            }

            argqv[0] = (char *)(&argv[0]);
            argqv[1] = (char *)(&argv[1]);
            cmd_parse_func(DUT_CMD_EXECUTE, 2, (char **)(&argqv));
            break;

        case WHY://?
            if (char_cut((char *)argv[1], data, '+', '?') == NULL) {
                goto dut_main_exit;
            }

            argqv[0] = (char *)(&argv[0]);
            argqv[1] = (char *)(&argv[1]);
            cmd_parse_func(DUT_CMD_GET, 2, (char **)(&argqv));
            break;

        case DOU:
            if (char_cut((char *)argv[1], data, '+', '=') == NULL) {
                goto dut_main_exit;
            }

            hcc = strchr(data, '=');
            int num = argc_len(hcc);

            if (num == 0) {
                dut_at_send("num:err 0");
                goto dut_main_exit;
            }

            if (num == 1) {
                char_cut((char *)argv[2], hcc, '=', '\0');
                argqv[2] = (char *)(&argv[2]);
            } else {
                char_cut((char *)argv[2], hcc, '=', ',');
                argqv[2] = (char *)(&argv[2]);

                for (int i = 1; i < num; i++) {
                    hcc = strchr(hcc, ',');
                    char_cut((char *)argv[2 + i], hcc, ',', ',');
                    argqv[2 + i] = (char *)(&argv[2 + i]);
                    hcc++;
                }
            }

            argqv[0] = (char *)(&argv[0]);
            argqv[1] = (char *)(&argv[1]);
            cmd_parse_func(DUT_CMD_EXECUTE, num + 2, (char **)(&argqv));
            break;

        case FIND://=  ?
            hcc = strrchr(data, '+');
            hcc++;
            str_chr(argv[1], hcc, '=');
            argqv[0] = (char *)(&argv[0]);
            argqv[1] = (char *)(&argv[1]);
            cmd_parse_func(DUT_CMD_HELP, 2, (char **)(&argqv));
            break;

        default :
            dut_at_send("AT support commands");
            cmd_list_t *node;
            slist_for_each_entry(&dut_svr.cmd_lists, node, cmd_list_t, next) {
                dut_at_send("    AT+%s%s", node->cmd->name, node->cmd->help);
            }
            break;
    }

dut_main_exit:
    return;
}

//////////////////////////////////////////////////
/* DUT Task */
static char msg_recv[CONFIG_DUT_UART_BUF_SIZE] = {0};
static uint32_t recv_size = 0;
void dut_task_entry(void)
{
    int ret = -1;
    char input_c;
    uint32_t tmp_len = 0;

    while (1) {
        tmp_len = 0;
        ret = hal_uart_recv_II(&dut_svr.uart_dev, &input_c, 1, &tmp_len, HAL_WAIT_FOREVER);
        if (ret != 0) {
            /* read err, clear buf */
            memset(msg_recv, 0, sizeof(msg_recv));
            recv_size = 0;
            continue;
        }
        msg_recv[recv_size++] = (unsigned char)input_c;
        if (recv_size >= CONFIG_DUT_UART_BUF_SIZE) {
            /* over flow, clear buf */
            memset(msg_recv, 0, sizeof(msg_recv));
            recv_size = 0;
            continue;
        }

        if ((msg_recv[recv_size - 1] == '\r' && msg_recv[recv_size - 2] == '\n') ||
            (msg_recv[recv_size - 1] == '\n' && msg_recv[recv_size - 2] == '\r')) {
            msg_recv[recv_size - 2] = '\0';
        } else if ((msg_recv[recv_size - 1] == '\n') || (msg_recv[recv_size - 1] == '\r')) {
            msg_recv[recv_size - 1] = '\0';
        } else {
            continue;
        }

        dut_test_at(msg_recv);
        recv_size = 0;
    }
}

int dut_service_init(dut_service_cfg_t *config)
{
    int ret;

    if (config == NULL) {
        return -1;
    }

    memset(&dut_svr, 0, sizeof(dut_svr));

    /* close wdt */
    dut_disable_wdt();

    aos_mutex_new(&dut_svr.lock);

    /* config uart pin function */
    dut_svr.uart_dev.port =  config->uart_idx;
    dut_svr.uart_dev.config.baud_rate = config->uart_baud;
    dut_svr.uart_dev.config.data_width = DATA_WIDTH_8BIT;
    dut_svr.uart_dev.config.parity = NO_PARITY;
    dut_svr.uart_dev.config.stop_bits = STOP_BITS_1;

    /* config uart for AT cmd input */
    ret = hal_uart_init(&dut_svr.uart_dev);

    if (ret) {
        printf("hal_uart_init error\r\n");
        return -1;
    }

    return 0;
}

int dut_service_cmd_reg(const dut_at_cmd_t *info)
{
    if (NULL == info || info->name == NULL || info->function == NULL) {
        return -EINVAL;
    }
    dut_svr_lock();
    cmd_list_t *temp_cmd_node;
    slist_for_each_entry(&dut_svr.cmd_lists, temp_cmd_node, cmd_list_t, next) {
        if (strcmp(temp_cmd_node->cmd->name, info->name) == 0) {
            goto err;
        }
    }

    cmd_list_t *cmd_node = (cmd_list_t *)aos_malloc(sizeof(cmd_list_t));

    if (cmd_node == NULL) {
        goto err;
    }

    cmd_node->cmd = info;
    slist_add_tail(&cmd_node->next, &dut_svr.cmd_lists);

    dut_svr_unlock();
    return 0;
err:
    dut_svr_unlock();
    return -1;

}

int dut_service_cmd_unreg(const dut_at_cmd_t *info)
{
    if (NULL == info) {
        return -EINVAL;
    }

    if (info->name && info->function) {
        cmd_list_t *cmd_node;

        dut_svr_lock();
        slist_for_each_entry(&dut_svr.cmd_lists, cmd_node, cmd_list_t, next) {
            if (cmd_node->cmd == info) {
                slist_del(&cmd_node->next, &dut_svr.cmd_lists);
                aos_free(cmd_node);
                break;
            }
        }
        dut_svr_unlock();

        return 0;
    } else {
        return -EINVAL;
    }

    return -1;
}

int dut_service_cmds_reg(const dut_at_cmd_t commands[], int num_commands)
{
    int i, ret;
    aos_check_return_einval(commands);

    for (i = 0; i < num_commands; i++) {
        ret = dut_service_cmd_reg(&commands[i]);
        if (0 != ret) {
            return ret;
        }
    }

    return 0;
}

int dut_service_cmds_unreg(const dut_at_cmd_t commands[], int num_commands)
{
    int i, ret;

    aos_check_return_einval(commands);

    for (i = 0; i < num_commands; i++) {
        ret = dut_service_cmd_unreg(&commands[i]);
        if (0 != ret) {
            return ret;
        }
    }

    return 0;
}

