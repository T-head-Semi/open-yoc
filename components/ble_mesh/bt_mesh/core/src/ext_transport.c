
#if 0
#include <ble_os.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <misc/util.h>
#include <misc/byteorder.h>

#include <net/buf.h>

#include <bluetooth/hci.h>
#include <api/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_TRANS)
#include "common/log.h"

#include "host/testing.h"

#include "crypto.h"
#include "adv.h"
#include "mesh.h"
#include "net.h"
#include "lpn.h"
#include "friend.h"
#include "access.h"
#include "foundation.h"
#include "settings.h"
#include "ble_transport.h"

#ifdef CONFIG_BT_MESH_PROVISIONER
#include "provisioner_prov.h"
#include "provisioner_main.h"
#include "provisioner_proxy.h"
#endif

#ifdef CONFIG_BT_MESH_EVENT_CALLBACK
#include "mesh_event_port.h"
#endif


#define AID_MASK                    ((u8_t)(BIT_MASK(6)))
#define SEG(data)                   ((data)[0] >> 7)
#define AKF(data)                   (((data)[0] >> 6) & 0x01)
#define AID(data)                   ((data)[0] & AID_MASK)
#define ASZMIC(data)                (((data)[1] >> 7) & 1)
#define APP_MIC_LEN(aszmic)         ((aszmic) ? 8 : 4)




int ext_sdu_recv(struct bt_mesh_net_rx *rx, u32_t seq, u8_t hdr,
                 u8_t aszmic, struct net_buf_simple *buf)
{

    NET_BUF_SIMPLE_DEFINE(sdu, CONFIG_BT_MESH_RX_EXT_SDU_MAX - 4);

    u8_t *ad;
    u16_t i;
    int err;

    BT_DBG("ASZMIC %u AKF %u AID 0x%02x", aszmic, AKF(&hdr), AID(&hdr));
    BT_DBG("ext len %u: %s", buf->len, bt_hex(buf->data, buf->len));

    if (buf->len < 1 + APP_MIC_LEN(aszmic)) {
        BT_ERR("Too short SDU + MIC");
        return -EINVAL;
    }

#if 0
    if (IS_ENABLED(CONFIG_BT_MESH_FRIEND) && !rx->local_match) {
        BT_DBG("Ignoring PDU for LPN 0x%04x of this Friend",
               rx->ctx.recv_dst);
        return 0;
    }
#endif

    if (BT_MESH_ADDR_IS_VIRTUAL(rx->ctx.recv_dst)) {
        ad = bt_mesh_label_uuid_get(rx->ctx.recv_dst);
    } else {
        ad = NULL;
    }

    /* Adjust the length to not contain the MIC at the end */
    buf->len -= APP_MIC_LEN(aszmic);

    if (!AKF(&hdr)) {
        const u8_t *dev_key = NULL;
#ifdef CONFIG_BT_MESH_PROVISIONER
        dev_key = provisioner_get_device_key(rx->ctx.addr);
        if (!dev_key) {
            dev_key = bt_mesh.dev_key;
        }
#else
        dev_key = bt_mesh.dev_key;
#endif
        if (!dev_key) {
            BT_DBG("%s: get NULL dev_key", __func__);
            return -EINVAL;
        }
        err = bt_mesh_app_decrypt(dev_key, true, aszmic, buf,
                                  &sdu, ad, rx->ctx.addr,
                                  rx->ctx.recv_dst, seq,
                                  BT_MESH_NET_IVI_RX(rx));
        if (err) {
            BT_ERR("Unable to decrypt with DevKey %d", err);
            return -EINVAL;
        }

        rx->ctx.app_idx = BT_MESH_KEY_DEV;
        bt_mesh_model_recv(rx, &sdu);
        return 0;
    }

    u32_t array_size = 0;
    array_size = ARRAY_SIZE(bt_mesh.app_keys);

    for (i = 0; i < array_size; i++) {
        struct bt_mesh_app_key *key;
        struct bt_mesh_app_keys *keys;

        key = &bt_mesh.app_keys[i];

        /* Check that this AppKey matches received net_idx */
        if (key->net_idx != rx->sub->net_idx) {
            continue;
        }

        if (rx->new_key && key->updated) {
            keys = &key->keys[1];
        } else {
            keys = &key->keys[0];
        }

        /* Check that the AppKey ID matches */
        if (AID(&hdr) != keys->id) {
            continue;
        }

        net_buf_simple_reset(&sdu);
        err = bt_mesh_app_decrypt(keys->val, false, aszmic, buf,
                                  &sdu, ad, rx->ctx.addr,
                                  rx->ctx.recv_dst, seq,
                                  BT_MESH_NET_IVI_RX(rx));
        if (err) {
            BT_WARN("Unable to decrypt with AppKey 0x%03x",
                    key->app_idx);
            continue;

        }

        rx->ctx.app_idx = key->app_idx;

        bt_mesh_model_recv(rx, &sdu);
        return 0;
    }

    BT_WARN("No matching AppKey");

    return -EINVAL;
}
#endif
