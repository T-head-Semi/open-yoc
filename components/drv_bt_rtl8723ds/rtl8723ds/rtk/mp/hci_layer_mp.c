/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include <string.h>

#include <aos/debug.h>
#include <aos/cli.h>
#include <aos/kernel.h>

#include "hci_h5_mp.h"

#define H4_NONE      0x00
#define H4_CMD       0x01
#define H4_ACL_UP    0x02
#define H4_SCO       0x03
#define H4_EVT       0x04
#define H4_ACL_DOWN  0x05

#define H5_ACK_PKT              0x00
#define HCI_COMMAND_PKT         0x01
#define HCI_ACLDATA_PKT         0x02
#define HCI_SCODATA_PKT         0x03
#define HCI_EVENT_PKT           0x04
#define H5_VDRSPEC_PKT          0x0E
#define H5_LINK_CTL_PKT         0x0F

#define HCI_COMMAND_PREAMBLE_SIZE 3

#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}
#define INT8_TO_STREAM(p, u8)    {*(p)++ = (int8_t)(u8);}
#define ARRAY_TO_STREAM(p, a, len) {register int ijk; for (ijk = 0; ijk < len;        ijk++) *(p)++ = (uint8_t) a[ijk];}
#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}

struct bt_hci_evt_hdr {
	uint8_t  evt;
	uint8_t  len;
} __packed;

typedef struct {
    uint8_t*   send_data;
    uint32_t   send_len;
    uint8_t*   resp_data;
    uint32_t   resp_len;
    aos_sem_t* cmd_sem;
    uint16_t   opcode;
    int        status;
}hci_cmd_data_t;

static hci_cmd_data_t cmd_data;

const static h5_mp_t *h5_ctx;

static aos_sem_t cmd_sem;

static bool hci_host_startup_flag;

static hci_cmd_data_t* get_send_cmd(uint16_t opcode)
{
    return &cmd_data;
}

static hci_cmd_data_t* alloc_send_cmd()
{
    if (!aos_sem_is_valid(&cmd_sem)) {
        aos_sem_new(&cmd_sem, 0);
    }
    cmd_data.cmd_sem = &cmd_sem;

    return &cmd_data;
}

static int h5_send( hci_cmd_data_t* send_cmd)
{
    if (send_cmd->opcode == HCI_VSC_H5_INIT) {
        h5_ctx->h5_send_sync_cmd(HCI_VSC_H5_INIT, NULL, 0);
        return 0;
    }

    h5_ctx->h5_send_cmd(HCI_COMMAND_PKT, send_cmd->send_data, send_cmd->send_len);

    return 0;
}

int h5_mp_send_cmd(uint16_t opcode, uint8_t *data, uint32_t len, uint8_t *resp_data, uint32_t *resp_len)
{
    int err = 0;
    uint8_t *command = calloc(1, HCI_COMMAND_PREAMBLE_SIZE + len);
    hci_cmd_data_t *send_cmd;

    if (command == NULL) {
        printf("malloc error\n");
        return -1;
    }

    uint8_t *stream = command;
    UINT16_TO_STREAM(stream, opcode);
    UINT8_TO_STREAM(stream, len);
    ARRAY_TO_STREAM(stream, data, len);
    send_cmd = alloc_send_cmd();

    send_cmd->send_data = command;
    send_cmd->send_len  = len + HCI_COMMAND_PREAMBLE_SIZE;
    send_cmd->opcode    = opcode;
    send_cmd->resp_data = resp_data;

    if (resp_len != NULL) {
        send_cmd->resp_len  = *resp_len;
    }

    h5_send(send_cmd);

    int ret = aos_sem_wait(send_cmd->cmd_sem, AOS_WAIT_FOREVER);

    aos_check(!ret, EIO);

    if (resp_len != NULL) {
        *resp_len = send_cmd->resp_len;
    }

    free(command);
    return err;
}

static int hci_h5_event_recv(uint8_t *data, uint16_t data_len)
{
    int err = 0;;
    uint8_t *pdata = data;
    int32_t len = data_len;
    struct bt_hci_evt_hdr hdr;
    hci_cmd_data_t *send_cmd;

    if (pdata == NULL || len == 0) {
        return -1;
    }

    if (len < 3) {
        return -1;
    }

    hdr.evt = *pdata++;
    hdr.len = *pdata++;

    if (len < hdr.len + 2) {
        return -1;
    }

    if (hdr.evt == HCI_COMMAND_COMPLETE_EVT ||
        hdr.evt  == HCI_COMMAND_STATUS_EVT) {
        uint16_t opcode;
        STREAM_TO_UINT16(opcode, pdata);

        send_cmd = get_send_cmd(opcode);

        if (send_cmd == NULL) {
            err = -1;
            goto end;
        }

        if (send_cmd->resp_data == NULL) {
            err = 0;
            goto end;
        }

        if (send_cmd->resp_len < data_len) {
            err = -1;
            goto end;
        }


        if (!cmd_data.status) {
            memcpy(send_cmd->resp_data, data + 5, hdr.len - 3);

            send_cmd->resp_len = hdr.len - 3;
        } else {
            memcpy(send_cmd->resp_data, data, hdr.len + 2);

            send_cmd->resp_len = hdr.len + 2;
        }
    } 

end:
    aos_sem_signal(send_cmd->cmd_sem);

    return err;
}

static void packet_recv_cb(hci_data_type_t type, uint8_t *data, uint32_t len)
{
    switch (type) {
        case DATA_TYPE_ACL:
            break;
        case  DATA_TYPE_EVENT:
            hci_h5_event_recv(data, len);
            break;
        default:
            break;
    }

    return;
}

// Module lifecycle functions
static void hci_bt_mp_start(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (hci_host_startup_flag == true) {
        printf("alreay start\n");
        return;
    }

    h5_ctx = get_h5_mp_interface();

    h5_ctx->h5_int_init(packet_recv_cb);

    hci_host_startup_flag = true;

    printf("start success\n");

    return;
}

static void hci_bt_mp_end(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (hci_host_startup_flag == false) {
        printf("alreay stop\n");
        return;
    }

    h5_ctx->h5_int_cleanup();

    hci_host_startup_flag = false;

    printf("end success\n");
}

static uint8_t resp[100];
static void hci_bt_mp_test(char *wbuf, int wbuf_len, int argc, char **argv)
{
    uint16_t opcode;
    uint32_t parameter_size;
    uint8_t param[32];
    int i;
    char *endptr;

    if (argc < 3) {
        printf("too few argc\n");
        return;
    }

    opcode =  strtol(argv[1], &endptr, 16);

    if (endptr == NULL) {
        printf("arg error\n");
        return;
    }

    parameter_size = strtol(argv[2], &endptr, 16);

    if (endptr == NULL) {
        printf("arg error\n");
        return;
    }

    if (argc != parameter_size + 3 || parameter_size > 32) {
        printf("too few argc\n");
        return;
    }

    for (i = 0 ; i < parameter_size; i ++) {
        param[i] = strtol(argv[3 + i], &endptr, 16);

        if (endptr == NULL) {
            printf("arg error\n");
            return;
        }
    }

    uint32_t resp_len = 100;

    cmd_data.status = 1;

    h5_mp_send_cmd(opcode, param, parameter_size, resp, &resp_len);
#if 1
    printf("%02X ", resp_len);
     for (i = 0 ; i < resp_len; i ++) {

        printf("%02X ", resp[i]);
    }
#else
    printf("success:%d ", resp[5]);
    printf("opcode: %04x ", *(uint16_t*)(resp + 3));
    printf("length: %d ", resp_len - 6);
    printf("param: ");

    for (i = 6 ; i < resp_len; i ++) {

        printf("%02X ", resp[i]);
    }
#endif

    printf("\n");

    cmd_data.status = 0;

    return;
}

void cli_reg_cmd_bt_test_start(void)
{
    static const struct cli_command cmd_info = {
        "bt_mp_start",
        "bt_mp_start",
         hci_bt_mp_start};

    aos_cli_register_command(&cmd_info);
}

void cli_reg_cmd_bt_test_end(void)
{
    static const struct cli_command cmd_info = {
        "bt_mp_end",
        "bt_mp_end",
         hci_bt_mp_end};

    aos_cli_register_command(&cmd_info);
}

void cli_reg_cmd_bt_test(void)
{
    static const struct cli_command cmd_info = {
        "bt_mp_test",
        "bt_mp_test",
         hci_bt_mp_test};

    aos_cli_register_command(&cmd_info);
}
