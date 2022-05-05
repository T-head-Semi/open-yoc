#include <devices/uart.h>
#include <ulog/ulog.h>
#include <uservice/eventid.h>
#include <uservice/uservice.h>
#include <yoc/atparser.h>
#include <aos/cli.h>

#include "app_main.h"

#define TAG "at_parser_init"

static atparser_uservice_t *g_atparser_uservice_t;
static aos_mutex_t g_cmd_mutex;



int at_demo(void)
{
    int ret = -1;
    int count = 3;

    aos_mutex_lock(&g_cmd_mutex, AOS_WAIT_FOREVER);

    atparser_clr_buf(g_atparser_uservice_t);

    if (atparser_send(g_atparser_uservice_t, "AT") == 0) {
        while (count --) {
            if (atparser_recv(g_atparser_uservice_t, "OK") == 0) {
                LOGD(TAG, "at_demo success");
                ret = 0;
                break;
            }
        }
    }

    atparser_cmd_exit(g_atparser_uservice_t);

    aos_mutex_unlock(&g_cmd_mutex);

    return ret;
}

int at_systime_demo(void)
{
    int ret = -1;
    int count = 3;
    int tv_sec = 0;
    int tv_ms = 0;

    aos_mutex_lock(&g_cmd_mutex, AOS_WAIT_FOREVER);

    atparser_clr_buf(g_atparser_uservice_t);

    if (atparser_send(g_atparser_uservice_t, "AT+SYSTIME") == 0) {
        while (count --) {
            if (atparser_recv(g_atparser_uservice_t, "+SYSTIME:%d.%d", &tv_sec, &tv_ms) == 0) {
                LOGD(TAG, "at_systime_demo: time %d.%d", tv_sec, tv_ms);
                ret = 0;
                break;
            }
        }
    }

    atparser_cmd_exit(g_atparser_uservice_t);

    aos_mutex_unlock(&g_cmd_mutex);

    return ret;
}


typedef enum {
    NET_TYPE_NULL = 0,
    NET_TYPE_TCP_SERVER,
    NET_TYPE_TCP_CLIENT,
    NET_TYPE_UDP_UNICAST,
    NET_TYPE_MAX
} net_conn_e;

int at_connect_remote(int id, net_conn_e type, char *srvname, uint16_t port)
{
    int ret = -1;
    int ret_id;


    aos_mutex_lock(&g_cmd_mutex, AOS_WAIT_FOREVER);

    atparser_clr_buf(g_atparser_uservice_t);

    switch (type) {
        case NET_TYPE_TCP_SERVER:
            /* TCP Server can NOT ignore lport */
            break;

        case NET_TYPE_UDP_UNICAST:
        case NET_TYPE_TCP_CLIENT:
            ret = atparser_send(g_atparser_uservice_t, "AT+CIPSTART=%d,\"%s\",\"%s\",%d", id, type == NET_TYPE_TCP_CLIENT ? "TCP" : "UDP", srvname, port);
            break;

        default:
            LOGE(TAG, "type=%d err!", type);
            return -1;

    }

    if (ret == 0) {
        // ret = -1;

        if ((atparser_recv(g_atparser_uservice_t, "%d,CONNECT\n", &ret_id) == 0) \
            && (atparser_recv(g_atparser_uservice_t, "OK\n") == 0)) {
            // if (ret_id == id) {
            //     ret = 0;
            // }
        } else {
            LOGE(TAG, "connect fail!");
        }
    }

    atparser_cmd_exit(g_atparser_uservice_t);

    aos_mutex_unlock(&g_cmd_mutex);

    return ret;
}

static void cmd_atcmd_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if(strcmp(argv[1], "AT+CIPSTART") == 0) {
        at_connect_remote(0, NET_TYPE_TCP_CLIENT, "192.168.0.78", 1883);
    } else if (strcmp(argv[1], "AT")== 0) {
        if(at_demo() != 0) {
            LOGE(TAG, "at fail");
        }
    } else if (strcmp(argv[1], "AT+SYSTIME") == 0) {
        if(at_systime_demo() != 0) {
            LOGE(TAG, "at fail");
        }
    }

}

void cli_reg_cmd_atcmd_demo(void)
{
    static const struct cli_command cmd_info =
    {
        "atcmd",
        "atcmd at+cmd=",
        cmd_atcmd_func
    };

    aos_cli_register_command(&cmd_info);
}


//发送at消息后收到对方的at回复消息，解析该回复消息
int app_at_parser_init(utask_t *task, const char *device_name)
{
    uart_config_t config;

    if (task == NULL) {
        task = utask_new("at_parser", 2 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI + 4);
    }

    if (task == NULL) {
        return -1;
    }

    uart_config_default(&config);

    g_atparser_uservice_t = atparser_init(task, device_name, &config);

    if (g_atparser_uservice_t == NULL) {
        LOGI(TAG, "atparser_init FAIL");
        return -1;
    }

    aos_mutex_new(&g_cmd_mutex);

    if (g_cmd_mutex.hdl == NULL) {
        LOGI(TAG, "atparser_init mutex FAIL");
        return -1;
    }

    // atparser_oob_create(g_atparser_uservice_t, "5,CLOSED", _closed_handler, (void *)5);
    // atparser_oob_create(g_atparser_uservice_t, "+PDP DEACT", _disconnect_handler, NULL);
    // atparser_oob_create(g_atparser_uservice_t, "+CPIN: NOT READY", _disconnect_handler, NULL);
    // atparser_oob_create(g_atparser_uservice_t, "+RECEIVE:", _recv_data_handler, NULL);
    // atparser_oob_create(g_atparser_uservice_t, "+CREG: 1", _creg_handler, (void *)1);

    return 0;
}
