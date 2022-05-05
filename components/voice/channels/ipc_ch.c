/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <aos/kernel.h>

#include <stdio.h>
#include <stdlib.h>
#include <aos/aos.h>
#include <aos/kernel.h>
#include <ipc.h>
#include <csi_core.h>

#include <voice_def.h>

extern uint32_t soc_get_cpu_id(void);
extern int32_t drv_get_cpu_id(void);
message_t g_msg_cp;

static void _ipc_process(ipc_t *ipc, message_t *ipc_msg, void *priv)
{
    voice_ch_t *ch = (voice_ch_t *)priv;
    voice_msg_t msg = {0};

    msg.command  = ipc_msg->command;
    msg.req_data = ipc_msg->req_data;
    msg.req_len  = ipc_msg->req_len;
    msg.flag     = ipc_msg->flag;

    if (ch->cb)
        ch->cb(ch->priv, &msg);

    if (ipc_msg->flag & MESSAGE_SYNC) {
        ipc_message_ack(ipc, ipc_msg, AOS_WAIT_FOREVER);
    }
}

static int ipc_msg_send(voice_ch_t *ch, voice_msg_t *msg)
{
    message_t ipc_msg;
    memset(&ipc_msg, 0, sizeof(message_t));
    ipc_t *ipc = (ipc_t*)ch->hdl;

    ipc_msg.service_id = VOICE_IPC_ID;
    ipc_msg.command    = msg->command;
    ipc_msg.req_data   = msg->req_data;
    ipc_msg.req_len    = msg->req_len;
    ipc_msg.flag       = msg->flag;

    ipc_message_send(ipc, &ipc_msg, AOS_WAIT_FOREVER);

    return 0;
}

static voice_ch_t *ipc_init(voice_msg_evt_t cb, void *priv)
{
    voice_ch_t *ch = aos_malloc_check( sizeof(voice_ch_t));
#ifdef CONFIG_CSI_V2
    int cpu_id = soc_get_cpu_id();
#else
    int cpu_id = drv_get_cpu_id();
#endif
    int dec_cpu_id;

    dec_cpu_id = cpu_id == 0? 2 : 0;
    ch->hdl = (int)ipc_get(dec_cpu_id);

    ipc_add_service((ipc_t *)ch->hdl, VOICE_IPC_ID, _ipc_process, ch);

    ch->cb = cb;
    ch->priv = priv;

    return ch;
}

static voice_ch_io_t ipc_ch = {
    .init     = ipc_init,
    .msg_send = ipc_msg_send,
};

voice_ch_io_t *ipc_ch_get(void)
{
    return &ipc_ch;
}
