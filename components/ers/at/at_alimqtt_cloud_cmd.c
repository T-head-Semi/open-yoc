/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#if defined(CONFIG_SAL) || defined(CONFIG_TCPIP)
#include <yoc/udata.h>
#include <yoc/iot.h>
#include <yoc/atserver.h>
#include <devices/netdrv.h>

#include "at_internal.h"


static const char *TAG = "alimqtt_at";

typedef enum {
    DISCONNECTED,
    CONNECTED,

    STATUS_ERR
} NET_STATUS;

typedef struct alimqtt_at {
    iot_t *iot_handle;
    iot_channel_t *iot_ch;
    int status;
    aos_mutex_t mutex;
} alimqtt_at_t;

alimqtt_at_t g_alimqtt_at_ctx;

static void channel_lock(void)
{
    if(g_alimqtt_at_ctx.mutex.hdl != NULL)
        aos_mutex_lock(&g_alimqtt_at_ctx.mutex, AOS_WAIT_FOREVER);
}

static void channel_unlock(void)
{
    if(g_alimqtt_at_ctx.mutex.hdl != NULL)
        aos_mutex_unlock(&g_alimqtt_at_ctx.mutex);
}
static void init_channel_udata(uData *udata)
{
    /* init data node */
    yoc_udata_set(udata, value_s("at cloud pub"), value_s("test send"), 0);
}

static void channel_close(iot_channel_t *ch)
{
    iot_channel_close(ch);
}

void alimqtt_evt_send(char *buf)
{
    if (at_cmd_event_on()) {
        atserver_lock();
        atserver_send("+EVENT=%s\r\n", buf);
        atserver_unlock();
    }
}
static void iot_event(uint32_t event_id, const void *param, void *context)
{
    int *status = &g_alimqtt_at_ctx.status;

    switch (event_id) {
        case EVENT_IOT_CONNECT_SUCCESS:
            *status = CONNECTED;
            alimqtt_evt_send("CLOUD,CONNECTED,SUCCESS");
            break;

        case EVENT_IOT_CONNECT_FAILED:
            *status = DISCONNECTED;
            channel_close(g_alimqtt_at_ctx.iot_ch);
            iot_destroy(g_alimqtt_at_ctx.iot_handle);
            g_alimqtt_at_ctx.iot_handle = NULL;
            alimqtt_evt_send("CLOUD,CONNECTED,FAILED");
            break;

        case EVENT_NETMGR_NET_DISCON:
        case EVENT_IOT_DISCONNECTED:
            channel_lock();

            if (*status == CONNECTED) {
                channel_close(g_alimqtt_at_ctx.iot_ch);
                iot_destroy(g_alimqtt_at_ctx.iot_handle);
                g_alimqtt_at_ctx.iot_handle = NULL;
                alimqtt_evt_send("CLOUD,DISCONNECT");
            }

            *status = DISCONNECTED;
            channel_unlock();
            break;

        case EVENT_IOT_PUSH_SUCCESS:
            alimqtt_evt_send("PUB,SUCC");
            break;

        case EVENT_IOT_PUSH_FAILED:
            alimqtt_evt_send("PUB,FAILED");
            break;

        default:
            aos_check_param(NULL);
    };
}

static iot_t *channel_new(void)
{
    iot_alimqtt_config_t cfg = {.server_url_suffix =
                                    "iot-as-mqtt.cn-shanghai.aliyuncs.com:1883"
                               };

    return (iot_new_alimqtt(&cfg));
}

static void channel_open(alimqtt_at_t *ctx)
{
    iot_channel_t *iot_ch = iot_channel_open(ctx->iot_handle, "thing/event/property/post,thing/service/property/set");

    if (iot_ch == NULL) {
        LOGE(TAG, "iot ch open failed");
        return;
    }

    if (ctx->mutex.hdl == NULL) {
        aos_mutex_new(&ctx->mutex);
    }

    /* Subscribe IOT Event */
    event_subscribe(EVENT_IOT_CONNECT_SUCCESS, iot_event, NULL);
    event_subscribe(EVENT_IOT_DISCONNECTED, iot_event, NULL);
    event_subscribe(EVENT_IOT_PUSH_SUCCESS, iot_event, NULL);
    event_subscribe(EVENT_IOT_PUSH_FAILED, iot_event, NULL);
    event_subscribe(EVENT_IOT_CONNECT_FAILED, iot_event, NULL);

    event_subscribe(EVENT_NETMGR_NET_DISCON, iot_event, NULL);

    iot_channel_config(iot_ch, NULL, NULL, NULL);

    /* init data node */
    init_channel_udata(iot_ch->uData);

    ctx->iot_ch = iot_ch;

    iot_channel_start(iot_ch);
}

void at_cmd_alimqtt_conn(char *cmd, int type, char *data)
{
    if (type == READ_CMD) {
        AT_BACK_RET_OK_INT(cmd, g_alimqtt_at_ctx.status);
    } else if (type == EXECUTE_CMD) {
        if (g_alimqtt_at_ctx.status == DISCONNECTED) {
            g_alimqtt_at_ctx.iot_handle = channel_new();

            if (g_alimqtt_at_ctx.iot_handle != NULL) {
                channel_open(&g_alimqtt_at_ctx);
                AT_BACK_OK();
            } else {
                AT_BACK_CME_ERR(AT_ERR);
            }
        } else {
            AT_BACK_CME_ERR(AT_ERR_STATUS);
        }
    }
}

void at_cmd_alimqtt_disconn(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        if (g_alimqtt_at_ctx.status == CONNECTED) {
            channel_close(g_alimqtt_at_ctx.iot_ch);
            iot_destroy(g_alimqtt_at_ctx.iot_handle);
            g_alimqtt_at_ctx.iot_handle = NULL;
            g_alimqtt_at_ctx.status = DISCONNECTED;
            AT_BACK_OK();
            alimqtt_evt_send("CLOUD,DISCONNECT");
        } else {
            AT_BACK_CME_ERR(AT_ERR_STATUS);
        }
    }
}

void at_cmd_alimqtt_pub(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        channel_lock();
        if (g_alimqtt_at_ctx.status == CONNECTED) {
            int data_cnt, len_cnt, len;
            int32_t ret;

            ret = atserver_scanf("%d,%n%*[^\0]%n", &len, &len_cnt, &data_cnt);

            if (ret == 1 && len == (data_cnt - len_cnt) && len <= 128) {
                iot_channel_t *ch = g_alimqtt_at_ctx.iot_ch;

                ret = iot_channel_raw_push(ch, data + len_cnt, len);

                if (ret >= 0) {
                    AT_BACK_OK();
                } else {
                    AT_LOGE(TAG, "pub ret %d", ret);
                    AT_BACK_RET_ERR(cmd, ret);
                }
            } else {
                AT_BACK_CME_ERR(AT_ERR_INVAL);
            }
        } else {
            AT_BACK_CME_ERR(AT_ERR_STATUS);
        }
        channel_unlock();
    }
}
#endif
