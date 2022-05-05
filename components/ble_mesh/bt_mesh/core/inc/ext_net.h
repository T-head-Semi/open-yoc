#ifndef _EXT_NET_H_
#define _EXT_NET_H_


enum {
    EXT_NET_TRANS_COMPLETE,
    EXT_NET_TRANS_FIRST,
    EXT_NET_TRANS_INTERM,
    EXT_NET_TRANS_LAST,
};

int bt_mesh_ext_net_send(struct bt_mesh_net_tx *tx, struct net_buf *buf, uint8_t frag,
                         const struct bt_mesh_send_cb *cb, void *cb_data);


#endif
