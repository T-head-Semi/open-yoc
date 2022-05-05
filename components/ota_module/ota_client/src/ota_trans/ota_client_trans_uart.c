/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_OTA_CLIENT_TRANS_UART)  && CONFIG_OTA_CLIENT_TRANS_UART
#include "ota_server.h"
#include "ais_ota/ais_ota_server.h"
#include "aos/kernel.h"
#include "ota_trans/ota_client_trans_uart.h"
#include "ota_trans/ota_client_trans.h"
#include "errno.h"
#include "ulog/ulog.h"
#include <yoc/atserver.h>

#define TAG "OTA_TRANS_CLI_UART"

#define DEF_MTU_SIZE 200

static ota_client_event_cb g_ota_client_cb = NULL;

#define DEF_UART_CHANN_CHECK_TIMEOUT 1000

typedef struct {
    uint8_t chan_status;
    upgrade_device *device;
} trans_uart_ctx;

trans_uart_ctx g_uart_ctx;

enum {
    TRANS_IDLE  = 0x00,
    TRANS_START = 0x01,
    TRANS_BUSY  = 0X02,
} trans_status;


typedef struct {
    char *buff;
    int   len;
} ais_ota_msg_t;

void ota_event_input_cb(int event, uint8_t *event_data, int len)
{
    if(g_ota_client_cb) {
        g_ota_client_cb(event,event_data,len);
    }
}

extern void ota_event_register_cb(ota_event_cb cb);
int ota_client_trans_uart_init(ota_client_event_cb cb)
{
    g_ota_client_cb = cb;
    ota_event_register_cb(ota_event_input_cb);
    return 0;
}

int ota_client_trans_uart_prepare(slist_t *dev_list)
{
    if (!dev_list) {
        return -EINVAL;
    }

    g_uart_ctx.device = NULL;
    g_uart_ctx.device = dev_list_dev_get(dev_list);

    if (!g_uart_ctx.device) {
        LOGE(TAG, "no ota dev found using channel uart");
        return -1;
    }
    return 0;
}

int ota_client_trans_uart_unregister()
{
    ota_client_trans_uart_disconnect();
    memset(&g_uart_ctx, 0x00, sizeof(trans_uart_ctx));
    return 0;
}

int ota_client_trans_uart_reset()
{
    return 0;
}

int ota_client_trans_uart_disconnect()
{
    return 0;
}

int ota_client_trans_uart_send(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len, uint8_t ack)
{
    ais_pdu_t msg;
    char tmp_buff[20];

    LOGD(TAG, "ota_ais_notify msg_id %02x %02x", msg_id, cmd);

    memset(&msg, 0, sizeof(msg));

    msg.header.enc = 1;
    msg.header.cmd = cmd;
    msg.header.msg_id = msg_id;
    msg.header.payload_len = len;

    if (p_msg) {
        memcpy(msg.payload, p_msg, len);
    }
    //串口发送
    atserver_lock();
    sprintf(tmp_buff,"+AISOTA=%d,", len + sizeof(ais_header_t));
    atserver_write(tmp_buff, strlen(tmp_buff));
    atserver_write(&msg, len + sizeof(ais_header_t));
    atserver_unlock();
    return 0;
}

void ota_client_trans_start(void)
{
    g_uart_ctx.chan_status = TRANS_START;
    int mtu                = DEF_MTU_SIZE;
    if (g_ota_client_cb) {
        g_ota_client_cb(AIS_OTA_SERVER_STATE_CHAN_READY, &mtu,1);
        g_ota_client_cb(AIS_OTA_SERVER_STATE_ONGOING, &g_uart_ctx.device->device.addr,sizeof(g_uart_ctx.device->device.addr));
    }
}

#endif

