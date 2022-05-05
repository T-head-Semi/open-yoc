/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
#define BT_MESH_ADV_DATA_SIZE 255
#else
/* Maximum advertising data payload for a single data type */
#define BT_MESH_ADV_DATA_SIZE 29
#endif
/* The user data is a pointer (4 bytes) to struct bt_mesh_adv */
#define BT_MESH_ADV_USER_DATA_SIZE 4

#define BT_MESH_ADV(buf) (*(struct bt_mesh_adv **)net_buf_user_data(buf))

enum bt_mesh_adv_type {
	BT_MESH_ADV_PROV,
	BT_MESH_ADV_DATA,
	BT_MESH_ADV_BEACON,
	BT_MESH_ADV_URI,
};

typedef void (*bt_mesh_adv_func_t)(struct net_buf *buf, u16_t duration,
				   int err, void *user_data);

struct bt_mesh_adv {
	const struct bt_mesh_send_cb *cb;
	void *cb_data;
#if  defined(CONFIG_MESH_LPM) && defined(CONFIG_BT_MESH_PROVISIONER)
	u8_t      type:2,
		      busy:1,
		      lpm_flag:1,
			  trans:4;
#else
    u8_t      type:2,
		      busy:1,
			  trans:5;
#endif
	u8_t      xmit;
#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
    u8_t      sid;
#endif
	#ifdef GENIE_ULTRA_PROV
    u8_t tiny_adv;
	#endif
	union {
		/* Address, used e.g. for Friend Queue messages */
		u16_t addr;

		/* For transport layer segment sending */
		struct {
			u8_t attempts;
		} seg;
	};
};

typedef struct bt_mesh_adv *(*bt_mesh_adv_alloc_t)(int id);

typedef void (*vendor_beacon_cb)(const bt_addr_le_t *addr, s8_t rssi,u8_t adv_type,void *user_data, uint16_t len);

/* xmit_count: Number of retransmissions, i.e. 0 == 1 transmission */
struct net_buf *bt_mesh_adv_create_with_net_if(enum bt_mesh_adv_type type, u8_t xmit,u8_t trans, bool sid_change_flag,
				   s32_t timeout);
#if defined(CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT) && CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT > 0
/* xmit_count: Number of retransmissions, i.e. 0 == 1 transmission */
struct net_buf *bt_mesh_relay_adv_create_with_net_if(enum bt_mesh_adv_type type, u8_t xmit,u8_t trans, bool sid_change_flag,
				   s32_t timeout);
#endif

#if defined(CONFIG_BT_MESH_EXT_ADV) && CONFIG_BT_MESH_EXT_ADV > 0
#define  bt_mesh_ext_adv_create(type, xmit, trans, sid_change_flag, timeout)  bt_mesh_adv_create_with_net_if(type, xmit, trans, sid_change_flag, timeout);
#endif

#define  bt_mesh_adv_create(type, xmit, timeout)  bt_mesh_adv_create_with_net_if(type, xmit, NET_TRANS_LEGACY, 0, timeout);

#if defined(CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT) && CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT > 0
#if defined(CONFIG_BT_MESH_EXT_ADV) && CONFIG_BT_MESH_EXT_ADV > 0
#define  bt_mesh_ext_relay_adv_create(type, xmit, trans, sid_change_flag, timeout)  bt_mesh_relay_adv_create_with_net_if(type, xmit, trans, sid_change_flag, timeout);
#endif
#define  bt_mesh_relay_adv_create(type, xmit, timeout)  bt_mesh_relay_adv_create_with_net_if(type, xmit, NET_TRANS_LEGACY, 0, timeout);
#endif

struct net_buf *bt_mesh_adv_create_from_pool(struct net_buf_pool *pool,
					     bt_mesh_adv_alloc_t get_id,
					     enum bt_mesh_adv_type type,
					     u8_t xmit, u8_t trans, bool sid_change_flag, s32_t timeout);

void bt_mesh_adv_send(struct net_buf *buf, const struct bt_mesh_send_cb *cb,
		      void *cb_data);

void bt_mesh_adv_update(void);

void bt_mesh_adv_init(void);

int bt_mesh_scan_enable(void);

int bt_mesh_scan_disable(void);

int bt_mesh_adv_enable(const struct bt_le_adv_param *param,
			const struct bt_data *ad, size_t ad_len,
			const struct bt_data *sd, size_t sd_len);
int bt_mesh_adv_disable();
int bt_mesh_adv_scan_update();
int bt_mesh_scan_enable(void);
int bt_mesh_adv_vnd_scan_register(vendor_beacon_cb);
int bt_mesh_adv_free_get(uint8_t *count, uint16_t *size);
int bt_mesh_adv_busy_get(uint8_t *count, uint16_t *size);

#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
int bt_mesh_fsm_refresh_state();
int bt_mesh_fsm_adv_send();
int bt_mesh_fsm_init();
int bt_mesh_fsm_adv_stop();
int bt_mesh_fsm_scan_start(void);
int bt_mesh_fsm_scan_stop(void);
#endif