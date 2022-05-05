/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_OTA_CLIENT_TRANS_HCI_UART)  && CONFIG_OTA_CLIENT_TRANS_HCI_UART

#include "aos/kernel.h"
#include "ais_ota/ais_ota_client.h"
#include "errno.h"
#include "ulog/ulog.h"
#include "ota_client.h"
#include "ota_trans/ota_client_trans.h"

#define TAG "OTA_TRANS_CLI_HCI_UART"

#define HCI_EXT_H5_SW_UPGRADE                          0xFC20
#define MAX_NOTIFY_LEN                                 255


static ota_client_event_cb g_cb = NULL;



typedef struct {
    uint8_t  ops_status;
    uint8_t  ops;
}  __attribute__((__packed__)) fw_upgrade_cmd_status;

enum {
    CMD_START = 0x01,
    CMD_STOP  = 0x02,
    CMD_DATA  = 0x03,
} cmd_ops;

typedef void (*hci_cb) (uint8_t *data, uint8_t len);

extern void hciExtSwUpgradeRegisterCb(hci_cb cb);
extern void HCI_CommandCompleteEvent1(uint16_t opcode,uint8_t  numParam,uint8_t  *param);


void ais_event_cb(void *event_data,uint32_t len)
{
  if(g_cb) {
     g_cb(OTA_CLIENT_EVENT_WRITE,event_data,len);
  }
}


int ota_client_trans_hci_uart_init(void         (*cb)(uint8_t , void* , uint16_t )      )
{
    int ret = 0;
	hciExtSwUpgradeRegisterCb(ais_event_cb);
	g_cb = cb;
    return 0;
}


int ota_client_trans_hci_uart_disconnect()
{  
    return 0;
}

int ota_client_trans_hci_uart_send(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len, uint8_t ack)
{
    ais_pdu_t msg;
    static uint8_t buffer[MAX_NOTIFY_LEN] = {0};

    fw_upgrade_cmd_status status = {
        .ops = CMD_DATA,
        .ops_status = len,
    };

    LOGD(TAG, "msg_id %02x %02x", msg_id, cmd);

    memset(&msg, 0, sizeof(msg));

    msg.header.cmd = cmd;
    msg.header.msg_id = msg_id;
    msg.header.payload_len = len;

    if (p_msg) {
        memcpy(msg.payload, p_msg, len);
    }

    LOGD(TAG, "len %d: %s", len + 4, bt_hex(&msg, len + 4));

    buffer[0] = status.ops_status;
    buffer[1] = status.ops;
    memcpy(buffer + 2, &msg, len + 4);
    HCI_CommandCompleteEvent1(HCI_EXT_H5_SW_UPGRADE, sizeof(fw_upgrade_cmd_status) +  len + 4, buffer);
}

#endif

