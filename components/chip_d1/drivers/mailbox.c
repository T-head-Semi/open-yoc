#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <soc.h>
#include <drv/mbox.h>
#include <drv/irq.h>
#include <drv/porting.h>
#include <drv/ringbuf.h>

#include <sunxi_hal_pwm.h>
#include <hal_msgbox.h>
#include "platform-msgbox.h"

#define MBOX_RINGBUF_LEN (32)

typedef struct {
    struct msg_endpoint edp;
    csi_mbox_t *mbox;
    char recv_buf[MBOX_RINGBUF_LEN];
    csi_ringbuf_t ringbuf;
} d1_mbox_t;

static d1_mbox_t g_d1_mbox;

static void msgbox_rec(uint32_t d, void *data)
{
    csi_mbox_t *mbox = g_d1_mbox.mbox;

    if(mbox->callback) {
        csi_ringbuf_in(&g_d1_mbox.ringbuf, &d, sizeof(uint32_t));
        mbox->callback(mbox, MBOX_EVENT_RECEIVED, 0, 1, mbox->arg);
    }
}

static void msgbox_tx_done(void *d)
{
    csi_mbox_t *mbox = g_d1_mbox.mbox;

    if(mbox->callback) {
        mbox->callback(mbox, MBOX_EVENT_SEND_COMPLETE, 0, 0, mbox->arg);
    }
}

csi_error_t csi_mbox_init(csi_mbox_t *mbox, uint32_t idx)
{
    CSI_PARAM_CHK(mbox, CSI_ERROR);

    g_d1_mbox.mbox = mbox;
    csi_ringbuf_reset(&g_d1_mbox.ringbuf);
    g_d1_mbox.ringbuf.buffer = g_d1_mbox.recv_buf;
    g_d1_mbox.ringbuf.size   = MBOX_RINGBUF_LEN;
    hal_msgbox_init();
    return 0;
}

void csi_mbox_uninit(csi_mbox_t *mbox)
{
    CSI_PARAM_CHK(mbox, CSI_ERROR);
    struct msg_endpoint *medp = &g_d1_mbox.edp;

    mbox->callback = NULL;
    mbox->arg      = NULL;
    medp->rec      = NULL;
    medp->tx_done  = NULL;
}

int32_t csi_mbox_send(csi_mbox_t *mbox, uint32_t channel_id, const void *data, uint32_t size)
{
    CSI_PARAM_CHK(mbox, CSI_ERROR);
    struct msg_endpoint *medp = &g_d1_mbox.edp;

    if (!medp->rec && !medp->tx_done) {
        medp->rec       = msgbox_rec;
        medp->tx_done   = msgbox_tx_done;
        hal_msgbox_alloc_channel(medp, 0, 1, 1);
    }

    hal_msgbox_channel_send(medp, (u8 *)data, size);

    return 0;
}

int32_t csi_mbox_receive(csi_mbox_t *mbox, uint32_t channel_id, void *data, uint32_t size)
{
    CSI_PARAM_CHK(mbox, CSI_ERROR);

    int ringbuf_len = csi_ringbuf_len(&g_d1_mbox.ringbuf);

    return csi_ringbuf_out(&g_d1_mbox.ringbuf, data, (ringbuf_len < size)? ringbuf_len : size);
}

csi_error_t csi_mbox_attach_callback(csi_mbox_t *mbox, void *callback, void *arg)
{
    CSI_PARAM_CHK(mbox, CSI_ERROR);

    mbox->callback = callback;
    mbox->arg      = arg;

    return 0;
}

void csi_mbox_detach_callback(csi_mbox_t *mbox)
{
    CSI_PARAM_CHK(mbox, CSI_ERROR);

    mbox->callback = NULL;
    mbox->arg      = NULL;
}

