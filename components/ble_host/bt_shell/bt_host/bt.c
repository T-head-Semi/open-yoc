/** @file
 * @brief Bluetooth shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <aos/ble.h>
#include <aos/cli.h>

#include <misc/byteorder.h>
#include <ble_os.h>
#include <ble_types/types.h>

#include <settings/settings.h>
#include <yoc/partition.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <common/log.h>
#include <host/conn_internal.h>
#include "host/hci_core.h"

/** @brief Callback called when command is entered.
 *
 *  @param argc Number of parameters passed.
 *  @param argv Array of option strings. First option is always command name.
 *
 * @return 0 in case of success or negative value in case of error.
 */
typedef int (*shell_cmd_function_t)(int argc, char *argv[]);
// typedef int partition_t;
// static partition_t handle = -1;
// static hal_logic_partition_t *lp;


struct shell_cmd {
    const char *cmd_name;
    shell_cmd_function_t cb;
    const char *help;
    const char *desc;
};

#include "bt.h"
#include "gatt.h"
#include "ll.h"

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN     (sizeof(DEVICE_NAME) - 1)
#define CREDITS         15
#define DATA_MTU        (23 * CREDITS)
#define DATA_BREDR_MTU      48

static u8_t selected_id = BT_ID_DEFAULT;

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#if defined(CONFIG_BT_CONN)
struct bt_conn *default_conn;
int16_t g_bt_conn_handle = -1;
int16_t g_security_level = 0;
uint8_t ble_init_flag = 0;

typedef struct {
    dev_addr_t addr;
    uint8_t set_flag;
} wl_addr;

#define MAX_WL_SZIE 10
wl_addr wl_list[MAX_WL_SZIE]= {0};

/* Multiply bt 1.25 to get MS */
#define BT_INTERVAL_TO_MS(interval) ((interval) * 5 / 4)

/* Connection context for BR/EDR legacy pairing in sec mode 3 */
static int16_t g_pairing_handle = -1;
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_EXT_ADV)
uint8_t selected_adv;
struct bt_le_ext_adv *adv_sets[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
#endif /* CONFIG_BT_EXT_ADV */

static void device_find(ble_event_en event, void *event_data);
#if defined(CONFIG_BT_SMP)
static void smp_event(ble_event_en event, void *event_data);
#endif
static void conn_param_req(ble_event_en event, void *event_data);
static void conn_param_update(ble_event_en event, void *event_data);
struct bt_le_oob oob_local;

#define L2CAP_DYM_CHANNEL_NUM 2

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
NET_BUF_POOL_DEFINE(data_tx_pool, L2CAP_DYM_CHANNEL_NUM, DATA_MTU, BT_BUF_USER_DATA_MIN, NULL);
NET_BUF_POOL_DEFINE(data_rx_pool, L2CAP_DYM_CHANNEL_NUM, DATA_MTU, BT_BUF_USER_DATA_MIN, NULL);
#endif

#if defined(CONFIG_BT_BREDR)
NET_BUF_POOL_DEFINE(data_bredr_pool, 1, DATA_BREDR_MTU, BT_BUF_USER_DATA_MIN,
                    NULL);

#define SDP_CLIENT_USER_BUF_LEN     512
NET_BUF_POOL_DEFINE(sdp_client_pool, CONFIG_BT_MAX_CONN,
                    SDP_CLIENT_USER_BUF_LEN, BT_BUF_USER_DATA_MIN, NULL);
#endif /* CONFIG_BT_BREDR */

#if defined(CONFIG_BT_RFCOMM)

static struct bt_sdp_attribute spp_attrs[] = {
    BT_SDP_NEW_SERVICE,
    BT_SDP_LIST(
        BT_SDP_ATTR_SVCLASS_ID_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
        BT_SDP_DATA_ELEM_LIST(
    {
        BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
        BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
    },
        )
    ),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROTO_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
        BT_SDP_DATA_ELEM_LIST(
    {
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
        BT_SDP_DATA_ELEM_LIST(
        {
            BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
            BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
        },
        )
    },
    {
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
        BT_SDP_DATA_ELEM_LIST(
        {
            BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
            BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
        },
        {
            BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
            BT_SDP_ARRAY_8(BT_RFCOMM_CHAN_SPP)
        },
        )
    },
        )
    ),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROFILE_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
        BT_SDP_DATA_ELEM_LIST(
    {
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
        BT_SDP_DATA_ELEM_LIST(
        {
            BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
            BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
        },
        {
            BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
            BT_SDP_ARRAY_16(0x0102)
        },
        )
    },
        )
    ),
    BT_SDP_SERVICE_NAME("Serial Port"),
};

static struct bt_sdp_record spp_rec = BT_SDP_RECORD(spp_attrs);

#endif /* CONFIG_BT_RFCOMM */

#define NAME_LEN 30

static char *addr_le_str(const bt_addr_le_t *addr)
{
    static char bufs[2][30];
    static u8_t cur;
    char *str;

    str = bufs[cur++];
    cur %= ARRAY_SIZE(bufs);
    bt_addr_le_to_str(addr, str, sizeof(bufs[cur]));

    return str;
}

static uint8_t char2u8(char c)
{
    if (c >= '0' && c <= '9') {
        return (c - '0');
    } else if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    } else {
        return 0;
    }
}

void str2hex(uint8_t hex[], char *s, uint8_t cnt)
{
    uint8_t i;

    if (!s) {
        return;
    }

    for (i = 0; (*s != '\0') && (i < cnt); i++, s += 2) {
        hex[i] = ((char2u8(*s) & 0x0f) << 4) | ((char2u8(*(s + 1))) & 0x0f);
    }
}

static inline int bt_addr2str(const dev_addr_t *addr, char *str,
                              uint16_t len)
{
    char type[10];

    switch (addr->type) {
    case BT_ADDR_LE_PUBLIC:
        strcpy(type, "public");
        break;

    case BT_ADDR_LE_RANDOM:
        strcpy(type, "random");
        break;

    default:
        snprintf(type, sizeof(type), "0x%02x", addr->type);
        break;
    }

    return snprintf(str, len, "%02X:%02X:%02X:%02X:%02X:%02X (%s)",
                    addr->val[5], addr->val[4], addr->val[3],
                    addr->val[2], addr->val[1], addr->val[0], type);
}

#if 0
static int char2hex(const char *c, u8_t *x)
{
    if (*c >= '0' && *c <= '9') {
        *x = *c - '0';
    } else if (*c >= 'a' && *c <= 'f') {
        *x = *c - 'a' + 10;
    } else if (*c >= 'A' && *c <= 'F') {
        *x = *c - 'A' + 10;
    } else {
        return -EINVAL;
    }

    return 0;
}
#endif

static int str2bt_addr(const char *str, dev_addr_t *addr)
{
    int i, j;
    u8_t tmp;

    if (strlen(str) != 17) {
        return -EINVAL;
    }

    for (i = 5, j = 1; *str != '\0'; str++, j++) {
        if (!(j % 3) && (*str != ':')) {
            return -EINVAL;
        } else if (*str == ':') {
            i--;
            continue;
        }

        addr->val[i] = addr->val[i] << 4;

        if (char2hex(*str, &tmp) < 0) {
            return -EINVAL;
        }

        addr->val[i] |= tmp;
    }

    return 0;
}

static int str2bt_addr_le(const char *str, const char *type, dev_addr_t *addr)
{
    int err;

    err = str2bt_addr(str, addr);

    if (err < 0) {
        return err;
    }

    if (!strcmp(type, "public") || !strcmp(type, "(public)")) {
        addr->type = DEV_ADDR_LE_PUBLIC;
    } else if (!strcmp(type, "random") || !strcmp(type, "(random)")) {
        addr->type = DEV_ADDR_LE_RANDOM;
    } else {
        return -EINVAL;
    }

    return 0;
}

static void conn_change(ble_event_en event, void *event_data)
{
    evt_data_gap_conn_change_t *e = (evt_data_gap_conn_change_t *)event_data;

    connect_info_t info;
    ble_stack_connect_info_get(e->conn_handle, &info);

    if (e->connected == CONNECTED && e->err == 0) {
        printf("Connected (%d): %s\n", e->conn_handle, addr_le_str((bt_addr_le_t *)&info.peer_addr));

        if (g_bt_conn_handle == -1) {
            g_bt_conn_handle = e->conn_handle;
        }

        /* clear connection reference for sec mode 3 pairing */
        if (g_pairing_handle != -1) {
            g_pairing_handle  = -1;
        }
    } else {

        printf("Disconected (%d): %s err %d\n", e->conn_handle, addr_le_str((bt_addr_le_t *)&info.peer_addr), e->err);

        if (e->err == 31) {
            while (1);
        }

        if (g_bt_conn_handle == e->conn_handle) {
            g_bt_conn_handle = -1;
        }

        g_security_level = 0;
    }
}

static void conn_security_change(ble_event_en event, void *event_data)
{
    evt_data_gap_security_change_t *e = (evt_data_gap_security_change_t *)event_data;
    printf("conn %d security level change to level%d\n", e->conn_handle, e->level);
    g_security_level = e->level;
}

static int event_callback(ble_event_en event, void *event_data)
{
    switch (event) {
    case EVENT_GAP_CONN_CHANGE:
        conn_change(event, event_data);
        break;

    case EVENT_GAP_CONN_SECURITY_CHANGE:
        conn_security_change(event, event_data);
        break;

    case EVENT_GAP_DEV_FIND:
        device_find(event, event_data);
        break;

    case EVENT_GAP_CONN_PARAM_REQ:
        conn_param_req(event, event_data);
        break;

    case EVENT_GAP_CONN_PARAM_UPDATE:
        conn_param_update(event, event_data);
        break;
#if defined(CONFIG_BT_SMP)

    case EVENT_SMP_PASSKEY_DISPLAY:
    case EVENT_SMP_PASSKEY_CONFIRM:
    case EVENT_SMP_PASSKEY_ENTER:
    case EVENT_SMP_PAIRING_CONFIRM:
    case EVENT_SMP_PAIRING_COMPLETE:
    case EVENT_SMP_CANCEL:
        smp_event(event, event_data);
        break;
#endif

    default:
        //printf("Unhandle event %d\n", event);
        break;
    }

    return 0;
}

static ble_event_cb_t ble_cb = {
    .callback = event_callback,
};

#if defined(CONFIG_BT_EXT_ADV)
#if defined(CONFIG_BT_BROADCASTER)
static void adv_sent(struct bt_le_ext_adv *adv,
		     struct bt_le_ext_adv_sent_info *info)
{
	printf("Advertiser[%d] %p sent %d",  \
		    bt_le_ext_adv_get_index(adv), adv, info->num_sent);
}

static void adv_scanned(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_scanned_info *info)
{
	char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, str, sizeof(str));

	printf("Advertiser[%d] %p scanned by %s\r\n",  \
		    bt_le_ext_adv_get_index(adv), adv, str);
}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_PERIPHERAL)
static void adv_connected(struct bt_le_ext_adv *adv,
			  struct bt_le_ext_adv_connected_info *info)
{
	char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(info->conn), str, sizeof(str));

	printf("Advertiser[%d] %p connected by %s", \
		    bt_le_ext_adv_get_index(adv), adv, str);
}
#endif /* CONFIG_BT_PERIPHERAL */
#endif /* CONFIG_BT_EXT_ADV */

#if defined(CONFIG_BT_EXT_ADV)
#if defined(CONFIG_BT_BROADCASTER)
static struct bt_le_ext_adv_cb adv_callbacks = {
	.sent = adv_sent,
	.scanned = adv_scanned,
#if defined(CONFIG_BT_PERIPHERAL)
	.connected = adv_connected,
#endif /* CONFIG_BT_PERIPHERAL */
};
#endif /* CONFIG_BT_BROADCASTER */
#endif /* CONFIG_BT_EXT_ADV */

#if defined(CONFIG_BT_PER_ADV_SYNC)
struct bt_le_per_adv_sync *per_adv_syncs[CONFIG_BT_PER_ADV_SYNC_MAX];

void conn_addr_str(struct bt_conn *conn, char *addr, size_t len)
{
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) < 0) {
		addr[0] = '\0';
		return;
	}

	switch (info.type) {
#if defined(CONFIG_BT_BREDR)
	case BT_CONN_TYPE_BR:
		bt_addr_to_str(info.br.dst, addr, len);
		break;
#endif
	case BT_CONN_TYPE_LE:
		bt_addr_le_to_str(info.le.dst, addr, len);
		break;
	}
}


static const char *phy2str(uint8_t phy)
{
    switch (phy)
    {
    case 0:
        return "No packets";
    case BT_GAP_LE_PHY_1M:
        return "LE 1M";
    case BT_GAP_LE_PHY_2M:
        return "LE 2M";
    case BT_GAP_LE_PHY_CODED:
        return "LE Coded";
    default:
        return "Unknown";
    }
}

static void per_adv_sync_sync_cb(struct bt_le_per_adv_sync *sync,
				 struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char past_peer[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	if (info->conn) {
		conn_addr_str(info->conn, past_peer, sizeof(past_peer));
	} else {
		memset(past_peer, 0, sizeof(past_peer));
	}

	printf("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
		    "Interval 0x%04x (%u ms), PHY %s, SD 0x%04X, PAST peer %s",
		    bt_le_per_adv_sync_get_index(sync), le_addr,
		    info->interval, BT_INTERVAL_TO_MS(info->interval),
		    phy2str(info->phy), info->service_data, past_peer);

	if (info->conn) { /* if from PAST */
		for (int i = 0; i < ARRAY_SIZE(per_adv_syncs); i++) {
			if (!per_adv_syncs[i]) {
				per_adv_syncs[i] = sync;
				break;
			}
		}
	}
}

static void per_adv_sync_terminated_cb(
	struct bt_le_per_adv_sync *sync,
	const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	for (int i = 0; i < ARRAY_SIZE(per_adv_syncs); i++) {
		if (per_adv_syncs[i] == sync) {
			per_adv_syncs[i] = NULL;
			break;
		}
	}

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printf( "PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated",
		    bt_le_per_adv_sync_get_index(sync), le_addr);
}

static void per_adv_sync_recv_cb(
	struct bt_le_per_adv_sync *sync,
	const struct bt_le_per_adv_sync_recv_info *info,
	struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printf( "PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
		    "RSSI %i, CTE %u, data length %u",
		    bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
		    info->rssi, info->cte_type, buf->len);
}




static struct bt_le_per_adv_sync_cb per_adv_sync_cb = {
	.synced = per_adv_sync_sync_cb,
	.term = per_adv_sync_terminated_cb,
	.recv = per_adv_sync_recv_cb,
};
#endif /* CONFIG_BT_PER_ADV_SYNC */

static int cmd_pub_addr(int argc, char *argv[])
{
    int err;
    dev_addr_t addr;

    if (argc == 2) {
        err = str2bt_addr(argv[1], &addr);
        if (err) {
            printf("Invalid address\n");
            return err;
        }

        bt_addr_le_t le_addr;
        le_addr.type = addr.type;
        memcpy(le_addr.a.val, addr.val, 6);

        extern int bt_set_bdaddr(const bt_addr_le_t *addr);
        err = bt_set_bdaddr(&le_addr);
		if(err) {
			return err;
		} else {
			return 0;
		}
    } else {
         return -EINVAL;
	}
}

static int cmd_init(int argc, char *argv[])
{
    int err;
    dev_addr_t addr;
    init_param_t init = {NULL, NULL, 1};

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
    NET_BUF_POOL_INIT(data_tx_pool);
    NET_BUF_POOL_INIT(data_rx_pool);
#endif

#if defined(CONFIG_BT_BREDR)
    NET_BUF_POOL_INIT(data_bredr_pool);
    NET_BUF_POOL_INIT(sdp_client_pool);
#endif /* CONFIG_BT_BREDR */

    if (argc == 3) {
        err = str2bt_addr_le(argv[1], argv[2], &addr);

        if (err) {
            printf("Invalid address\n");
            return err;
        }

        init.dev_addr = &addr;
    }

    err = ble_stack_init(&init);

    ble_stack_setting_load();

    if (err) {
        printf("Bluetooth init failed (err %d)\n", err);
        return err;
    }

    err = ble_stack_event_register(&ble_cb);
    if(err) {
        printf("Bluetooth stack init fail\n");
        return -1;
    } else {
        printf("Bluetooth stack init success\n");
    }

    ble_init_flag = 1;

    return 0;
}

#if defined(CONFIG_BT_HCI) || defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
void hexdump(const u8_t *data, size_t len)
{
    int n = 0;

    while (len--) {
        if (n % 16 == 0) {
            printf("%08X ", n);
        }

        printf("%02X ", *data++);

        n++;

        if (n % 8 == 0) {
            if (n % 16 == 0) {
                printf("\n");
            } else {
                printf(" ");
            }
        }
    }

    if (n % 16) {
        printf("\n");
    }
}
#endif /* CONFIG_BT_HCI || CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

#if defined(CONFIG_BT_HCI)

#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
static int cmd_hci_cmd_send_cb(u16_t opcode, u8_t status, struct net_buf *buf, void *args)
{
	struct {
		struct k_sem *sync;
		struct net_buf *rsp;
	} *arg;

	arg = args;

	if (arg && arg->sync)
	{
		arg->rsp = buf;
		k_sem_give(arg->sync);
	}
	else
	{
		net_buf_unref(buf);
	}

	return 0;
}
#endif

static int cmd_hci_cmd(int argc, char *argv[])
{
    u8_t ogf;
    u16_t ocf;
    struct net_buf *buf = NULL, *rsp;
    int err;

    if (argc < 3) {
        return -EINVAL;
    }

    ogf = strtoul(argv[1], NULL, 16);
    ocf = strtoul(argv[2], NULL, 16);

    if (argc > 3) {
        int i;

        buf = bt_hci_cmd_create(BT_OP(ogf, ocf), argc - 3);

        for (i = 3; i < argc; i++) {
            net_buf_add_u8(buf, strtoul(argv[i], NULL, 16));
        }
    }

#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE

	struct {
		struct k_sem *sync;
		struct net_buf *rsp;
	} args = {0};

	struct k_sem sync_sem;

	k_sem_init(&sync_sem, 0, 1);

	bt_hci_cmd_cb_t cb =
	{
		.func = cmd_hci_cmd_send_cb,
	};

	args.sync = &sync_sem;

	cb.args = &args;

	err = bt_hci_cmd_send_cb(BT_OP(ogf, ocf), buf, &cb);
	if (err) {
        net_buf_unref(buf);
		return err;
	}

	err = k_sem_take(&sync_sem, K_SECONDS(10));
	if (err)
	{
		return err;
	}

	k_sem_delete(&sync_sem);

	rsp = args.rsp;
#else
    err = bt_hci_cmd_send_sync(BT_OP(ogf, ocf), buf, &rsp);
#endif
    if (err) {
        printf("HCI command failed (err %d)\n", err);
    } else {
        hexdump(rsp->data, rsp->len);
        net_buf_unref(rsp);
    }

    return 0;
}
#endif /* CONFIG_BT_HCI */

static int cmd_name(int argc, char *argv[])
{
    int err;

    if (argc < 2) {
        printf("Bluetooth Local Name: %s\n", bt_get_name());
        return 0;
    }

    err = bt_set_name(argv[1]);

    if (err) {
        printf("Unable to set name %s (err %d)", argv[1], err);
    }

    return 0;
}

static int cmd_id_create(int argc, char *argv[])
{
    dev_addr_t addr;
    int err;

    if (argc > 1) {
        err = str2bt_addr_le(argv[1], "random", &addr);

        if (err) {
            printf("Invalid address\n");
            return err;
        }
    } else {
        bt_addr_le_copy((bt_addr_le_t *)&addr, BT_ADDR_LE_ANY);
    }

    err = bt_id_create((bt_addr_le_t *)&addr, NULL);

    if (err < 0) {
        printf("Creating new ID failed (err %d)\n", err);
        return 0;
    }

    printf("New identity (%d) created: %s\n", err, addr_le_str((bt_addr_le_t *)&addr));

    return 0;
}


static int cmd_phy_rate_set(int argc, char *argv[])
{
    uint8_t rx_phy_rate,tx_phy_rate;

    if(g_bt_conn_handle < 0) {
        printf("dev not connected\r\n");
        return -1;
    }

    if (argc < 3) {
        return -EINVAL;
    }

    tx_phy_rate = strtoul(argv[1], NULL, 16);
    if(tx_phy_rate != PREF_PHY_1M && tx_phy_rate != PREF_PHY_2M  && tx_phy_rate != PREF_PHY_CODED) {
        return -EINVAL;
    }

    rx_phy_rate = strtoul(argv[2], NULL, 16);
    if(rx_phy_rate != PREF_PHY_1M && rx_phy_rate != PREF_PHY_2M && rx_phy_rate != PREF_PHY_CODED) {
        return -EINVAL;
    }

    return ble_stack_pref_phy_set(g_bt_conn_handle,tx_phy_rate,rx_phy_rate);

}


static int cmd_dle_enable(int argc, char *argv[])
{
    int err = -1;
    if(g_bt_conn_handle < 0) {
        return -1;
    }

#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
    struct bt_conn *conn = bt_conn_lookup_index(g_bt_conn_handle);
    if (conn)
    {
        err = hci_cmd_le_read_max_data_len(conn);
        bt_conn_unref(conn);
    }
#else
    u16_t tx_octets, tx_time;

    extern int hci_le_read_max_data_len(u16_t *tx_octets, u16_t *tx_time);
    extern int bt_le_set_data_len(struct bt_conn *conn, u16_t tx_octets, u16_t tx_time);
    err = hci_le_read_max_data_len(&tx_octets, &tx_time);
    if (!err) {
        struct bt_conn *conn = bt_conn_lookup_index(g_bt_conn_handle);

        if (conn) {
            err = bt_le_set_data_len(conn, tx_octets, tx_time);
            if (err) {
                BT_ERR("Failed to set data len (%d)", err);
            }
            bt_conn_unref(conn);
        }
    }
#endif
    return err;
}

static int cmd_id_reset(int argc, char *argv[])
{
    bt_addr_le_t addr;
    u8_t id;
    int err;

    if (argc < 2) {
        printf("Identity identifier not specified\n");
        return -EINVAL;
    }

    id = strtol(argv[1], NULL, 10);

    if (argc > 2) {
        err = str2bt_addr_le(argv[2], "random", (dev_addr_t *)&addr);

        if (err) {
            printf("Invalid address\n");
            return err;
        }
    } else {
        bt_addr_le_copy(&addr, BT_ADDR_LE_ANY);
    }

    err = bt_id_reset(id, &addr, NULL);

    if (err < 0) {
        printf("Resetting ID %u failed (err %d)\n", id, err);
        return 0;
    }

    printf("Identity %u reset: %s\n", id, addr_le_str(&addr));

    return 0;
}

static int cmd_id_delete(int argc, char *argv[])
{
    u8_t id;
    int err;

    if (argc < 2) {
        printf("Identity identifier not specified\n");
        return -EINVAL;
    }

    id = strtol(argv[1], NULL, 10);

    err = bt_id_delete(id);

    if (err < 0) {
        printf("Deleting ID %u failed (err %d)\n", id, err);
        return 0;
    }

    printf("Identity %u deleted\n", id);

    return 0;
}

static int cmd_id_show(int argc, char *argv[])
{
    bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
    size_t i, count = CONFIG_BT_ID_MAX;

    bt_id_get(addrs, &count);

    for (i = 0; i < count; i++) {
        printf("%s%zu: %s\n", i == selected_id ? "*" : " ", i,
               addr_le_str(&addrs[i]));
    }

    return 0;
}

static int cmd_id_select(int argc, char *argv[])
{
    bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
    size_t count = CONFIG_BT_ID_MAX;
    u8_t id;

    if (argc < 2) {
        return -EINVAL;
    }

    id = strtol(argv[1], NULL, 10);

    bt_id_get(addrs, &count);

    if (count <= id) {
        printf("Invalid identity\n");
        return 0;
    }

    //printf("Selected identity: %s\n", addr_le_str(&addrs[id]));
    selected_id = id;

    return 0;
}

static inline int parse_ad(uint8_t *data, uint16_t len, int (* cb)(ad_data_t *data, void *arg), void *cb_arg)
{
    int num = 0;
    uint8_t *pdata = data;
    ad_data_t ad = {0};
    int ret;

    while (len) {
        if (pdata[0] == 0) {
            return num;
        }

        if (len < pdata[0] + 1) {
            return -1;
        };

        ad.len = pdata[0] - 1;

        ad.type = pdata[1];

        ad.data = &pdata[2];

        if (cb) {
            ret = cb(&ad, cb_arg);

            if (ret) {
                break;
            }
        }

        num++;
        len -= (pdata[0] + 1);
        pdata += (pdata[0] + 1);
    }

    return num;
}

uint8_t *pscan_ad = NULL;
uint8_t *pscan_sd = NULL;

static int scan_ad_cmp(ad_data_t *ad, void *arg)
{
    ad_data_t *ad2 = arg;

    return (ad->type == ad2->type && ad->len == ad2->len
            && 0 == memcmp(ad->data, ad2->data, ad->len));
}

static int scan_ad_callback(ad_data_t *ad, void *arg)
{
    evt_data_gap_dev_find_t *e = arg;
    int ad_num;
    uint8_t *pad = NULL;
    int ret;

    if (e->adv_len) {
        if (e->adv_type == 0x04) {
            pad = pscan_sd;
        } else {
            pad = pscan_ad;
        }

        ad_num = parse_ad(pad, 31, NULL, NULL);

        ret = parse_ad(pad, 31, scan_ad_cmp, (void *)ad);

        if (ret < ad_num) {
            return 1;
        }
    }

    return 0;
}

static void device_find(ble_event_en event, void *event_data)
{
    evt_data_gap_dev_find_t *e = event_data;
    int ad_num = parse_ad(e->adv_data, e->adv_len, NULL, NULL);
    int ret;
    char addr_str[32] = {0};

    bt_addr2str(&e->dev_addr, addr_str, sizeof(addr_str));

    if (pscan_ad || pscan_sd) {
        ret = parse_ad(e->adv_data, e->adv_len, scan_ad_callback, event_data);

        if (ret < ad_num) {
#if defined(CONFIG_BT_EXT_ADV) && CONFIG_BT_EXT_ADV > 0
            printf("[DEVICE]: %s, adv type %d, rssi %d, len %d, sid %d, Raw data:%s", addr_str, e->adv_type,  e->rssi, e->adv_len, e->sid, bt_hex(e->adv_data, e->adv_len > 31 ? 31 : e->adv_len));
#else
            printf("[DEVICE]: %s, adv type %d, rssi %d, len %d, Raw data:%s", addr_str, e->adv_type,  e->rssi, e->adv_len, bt_hex(e->adv_data, e->adv_len > 31 ? 31 : e->adv_len));
#endif
            if (e->adv_len > 31)
            {
                uint8_t len = e->adv_len - 31 < 10 ? e->adv_len - 31 : 10;
                printf(", Tail Data: %s", bt_hex(e->adv_data + e->adv_len - len, len));
            }
            printf("\n");
        }
    } else {
#if defined(CONFIG_BT_EXT_ADV) && CONFIG_BT_EXT_ADV > 0
            printf("[DEVICE]: %s, adv type %d, rssi %d, len %d, sid %d, Raw data:%s", addr_str, e->adv_type,  e->rssi, e->adv_len, e->sid, bt_hex(e->adv_data, e->adv_len > 31 ? 31 : e->adv_len));
#else
            printf("[DEVICE]: %s, adv type %d, rssi %d, len %d, Raw data:%s", addr_str, e->adv_type,  e->rssi, e->adv_len, bt_hex(e->adv_data, e->adv_len > 31 ? 31 : e->adv_len));
#endif
            if (e->adv_len > 31)
            {
                uint8_t len = e->adv_len - 31 < 10 ? e->adv_len - 31 : 10;
                printf(", Tail Data: %s", bt_hex(e->adv_data + e->adv_len - len, len));
            }
            printf("\n");
    }
}

static void conn_param_req(ble_event_en event, void *event_data)
{
    evt_data_gap_conn_param_req_t *e = event_data;
    printf("LE conn  param req: int (0x%04x, 0x%04x) lat %d to %d\n",
           e->param.interval_min, e->param.interval_max, e->param.latency,
           e->param.timeout);

    e->accept = 1;
}

static void conn_param_update(ble_event_en event, void *event_data)
{
    evt_data_gap_conn_param_update_t *e = event_data;

    printf("LE conn param updated: int 0x%04x lat %d to %d\n", e->interval,
           e->latency, e->timeout);
}

static uint8_t scan_filter = 0;
static int cmd_scan_filter(int argc, char *argv[])
{
    if (argc < 2) {
        return -EINVAL;
    }

    scan_filter = atoi(argv[1]);

    if (scan_filter > SCAN_FILTER_POLICY_WHITE_LIST) {
        scan_filter = 0;
        return -EINVAL;
    }

    printf("Set scan filter %s\n", scan_filter == 0 ? "SCAN_FILTER_POLICY_ANY_ADV" : "SCAN_FILTER_POLICY_WHITE_LIST");
    return 0;
}

static int cmd_scan(int argc, char *argv[])
{
    static uint8_t scan_ad[31] = {0};
    static uint8_t scan_sd[31] = {0};

    scan_param_t params = {
        SCAN_PASSIVE,
        SCAN_FILTER_DUP_DISABLE,
        SCAN_FAST_INTERVAL,
        SCAN_FAST_WINDOW,
	    BT_LE_SCAN_OPT_NONE,
    };

    params.scan_filter = scan_filter;

    if (argc < 2) {
        return -EINVAL;
    }


    if (argc >= 2) {
        if (!strcmp(argv[1], "active")) {
            params.type = SCAN_ACTIVE;
        } else if (!strcmp(argv[1], "off")) {
            pscan_ad = NULL;
            pscan_sd = NULL;
            return ble_stack_scan_stop();
        } else if (!strcmp(argv[1], "passive")) {
            params.type = SCAN_PASSIVE;
        } else {

            return -EINVAL;
        }
    }

    if (argc >= 3) {
        if (!strcmp(argv[2], "dups")) {
            params.filter_dup = SCAN_FILTER_DUP_DISABLE;
        } else if (!strcmp(argv[2], "nodups")) {
            params.filter_dup = SCAN_FILTER_DUP_ENABLE;
        } else {
            return -EINVAL;
        }
    }

    if (argc >= 4) {
        params.interval = strtol(argv[3], NULL, 10);
    }

    if (argc >= 5) {
        params.window = strtol(argv[4], NULL, 10);
    }

    if (argc >= 6) {
        pscan_ad = scan_ad;
        memset(scan_ad, 0, sizeof(scan_ad));
        str2hex(scan_ad, argv[5], sizeof(scan_ad));
    }

    if (argc >= 7) {
        pscan_sd = scan_sd;
        memset(scan_sd, 0, sizeof(scan_sd));
        str2hex(scan_sd, argv[6], sizeof(scan_sd));
    }

    return ble_stack_scan_start(&params);
}

static inline int parse_ad_data(uint8_t *data, uint16_t len, ad_data_t *ad, uint8_t *ad_num)
{
    uint8_t num = 0;
    uint8_t *pdata = data;

    while (len) {
        if (len < pdata[0] + 1) {
            *ad_num = 0;
            return -1;
        };

        num++;

        if (ad && num <= *ad_num) {
            ad->len = pdata[0] - 1;
            ad->type = pdata[1];
            ad->data = &pdata[2];
        }

        len -= (pdata[0] + 1);
        pdata += (pdata[0] + 1);

        if (ad) {
            ad++;
        }
    }

    *ad_num = num;
    return 0;
}

static inline int adv_ad_callback(ad_data_t *ad, void *arg)
{
    ad_data_t **pad = (ad_data_t **)arg;

    (*pad)->type = ad->type;
    (*pad)->len = ad->len;
    (*pad)->data = ad->data;
    (*pad)++;
    return 0;
}

static int cmd_advertise(int argc, char *argv[])
{
    int err;
    adv_param_t param = {0};
    uint8_t ad_hex[31] = {0};
    uint8_t sd_hex[31] = {0};
    ad_data_t ad[10] = {0};
    int8_t ad_num = 0;
    ad_data_t sd[10] = {0};
    int8_t sd_num = 0;


    if (argc < 2) {
        goto fail;
    }

    if (!strcmp(argv[1], "stop")) {
        if (bt_le_adv_stop() < 0) {
            printf("Failed to stop advertising\n");
        } else {
            printf("Advertising stopped\n");
        }

        return 0;
    }

    if (!strcmp(argv[1], "nconn")) {
        param.type = ADV_NONCONN_IND;
    } else if (!strcmp(argv[1], "conn")) {
        param.type = ADV_IND;
    }


    if (argc > 2) {
        if (strlen(argv[2]) > 62) {
            printf("max len\n");
            goto fail;
        }

        str2hex(ad_hex, argv[2], sizeof(ad_hex));

        ad_data_t *pad = ad;
        ad_num = parse_ad(ad_hex, strlen(argv[2]) / 2, adv_ad_callback, (void *)&pad);
    }

    if (argc > 3) {
        if (strlen(argv[3]) > 62) {
            printf("max len\n");
            goto fail;
        }

        str2hex(sd_hex, argv[3], sizeof(sd_hex));
        ad_data_t *psd = sd;
        sd_num = parse_ad(sd_hex, strlen(argv[3]) / 2, adv_ad_callback, (void *)&psd);
    }


    param.ad = ad_num > 0 ?ad:NULL;
    param.sd = sd_num> 0 ?sd:NULL;
    param.ad_num = (uint8_t)ad_num;
    param.sd_num = (uint8_t)sd_num;
    param.interval_min = ADV_FAST_INT_MIN_2;
    param.interval_max = ADV_FAST_INT_MAX_2;

    err = ble_stack_adv_start(&param);

    if (err < 0) {
        printf("Failed to start advertising (err %d)\n", err);
    } else {
        printf("adv_type:%x;adv_interval_min:%d (*0.625)ms;adv_interval_max:%d (*0.625)ms\n", param.type, (int)param.interval_min, (int)param.interval_max);
        printf("Advertising started\n");
    }

    return 0;

fail:

    return -EINVAL;
}

#if defined(CONFIG_BT_EXT_ADV)
static bool adv_param_parse(size_t argc, char *argv[],
			   struct bt_le_adv_param *param)
{
    static uint8_t sid = 0;
	memset(param, 0, sizeof(struct bt_le_adv_param));

	if (!strcmp(argv[1], "conn-scan")) {
		param->options |= BT_LE_ADV_OPT_CONNECTABLE;
		param->options |= BT_LE_ADV_OPT_SCANNABLE;
	} else if (!strcmp(argv[1], "conn-nscan")) {
		param->options |= BT_LE_ADV_OPT_CONNECTABLE;
	} else if (!strcmp(argv[1], "nconn-scan")) {
		param->options |= BT_LE_ADV_OPT_SCANNABLE;
	} else if (!strcmp(argv[1], "nconn-nscan")) {
		/* Acceptable option, nothing to do */
	} else {
		return false;
	}

	for (size_t argn = 2; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "ext-adv")) {
			param->options |= BT_LE_ADV_OPT_EXT_ADV;
		} else if (!strcmp(arg, "coded")) {
			param->options |= BT_LE_ADV_OPT_CODED;
		} else if (!strcmp(arg, "no-2m")) {
			param->options |= BT_LE_ADV_OPT_NO_2M;
		} else if (!strcmp(arg, "anon")) {
			param->options |= BT_LE_ADV_OPT_ANONYMOUS;
		} else if (!strcmp(arg, "tx-power")) {
			param->options |= BT_LE_ADV_OPT_USE_TX_POWER;
		} else if (!strcmp(arg, "scan-reports")) {
			param->options |= BT_LE_ADV_OPT_NOTIFY_SCAN_REQ;
		} else if (!strcmp(arg, "wl")) {
			param->options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
			param->options |= BT_LE_ADV_OPT_FILTER_CONN;
		} else if (!strcmp(arg, "wl-scan")) {
			param->options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
		} else if (!strcmp(arg, "wl-conn")) {
			param->options |= BT_LE_ADV_OPT_FILTER_CONN;
		} else if (!strcmp(arg, "identity")) {
			param->options |= BT_LE_ADV_OPT_USE_IDENTITY;
		} else if (!strcmp(arg, "name")) {
			param->options |= BT_LE_ADV_OPT_USE_NAME;
		} else if (!strcmp(arg, "low")) {
			param->options |= BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY;
		} else if (!strcmp(arg, "directed")) {
			static bt_addr_le_t addr;

			if ((argn + 2) >= argc) {
				return false;
			}

			if (bt_addr_le_from_str(argv[argn + 1], argv[argn + 2],
						&addr)) {
				return false;
			}

			param->peer = &addr;
			argn += 2;
		} else {
			return false;
		}
	}

	param->id = selected_id;
	param->sid = sid++;
	param->secondary_max_skip = 0x01;

	if (param->peer &&
	    !(param->options & BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY)) {
		param->interval_min = 0;
		param->interval_max = 0;
	} else {
		param->interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
		param->interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
	}

	return true;
}

static int cmd_adv_create(int argc, char *argv[])
{
	struct bt_le_adv_param param;
	struct bt_le_ext_adv *adv;
	uint8_t adv_index;
	int err;

	if (!adv_param_parse(argc, argv, &param)) {
		return -ENOEXEC;
	}

	err = bt_le_ext_adv_create(&param, &adv_callbacks, &adv);
	if (err) {
		printf( "Failed to create advertiser set (%d)", err);
		return -ENOEXEC;
	}
    
	adv_index = bt_le_ext_adv_get_index(adv);
	adv_sets[adv_index] = adv;

	printf( "Created adv id: %d, adv: %p", adv_index, adv);

	return 0;
}

static int cmd_adv_param(int argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	struct bt_le_adv_param param;
	int err;

	if (!adv_param_parse(argc, argv, &param)) {
		return -ENOEXEC;
	}

	err = bt_le_ext_adv_update_param(adv, &param);
	if (err) {
		printf( "Failed to update advertiser set (%d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_adv_data(int argc, char *argv[])
{
	uint8_t discov_data = (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR);
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	static uint8_t hex_data[512] = {0};
	struct bt_data *data;
	struct bt_data ad[8];
	struct bt_data sd[8];
	size_t hex_data_len;
	size_t ad_len = 0;
	size_t sd_len = 0;
	size_t *data_len;
	int err;

	if (!adv) {
		return -EINVAL;
	}

	hex_data_len = 0;
	data = ad;
	data_len = &ad_len;

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "scan-response") &&
		    *data_len == ARRAY_SIZE(ad)) {
			/* Maximum entries limit reached. */
			printf("Failed to set advertising data: "
					   "Maximum entries limit reached");

			return -ENOEXEC;
		}

		if (!strcmp(arg, "discov")) {
			data[*data_len].type = BT_DATA_FLAGS;
			data[*data_len].data_len = sizeof(discov_data);
			data[*data_len].data = &discov_data;
			(*data_len)++;
		} else if (!strcmp(arg, "name")) {
			const char *name = bt_get_name();

			data[*data_len].type = BT_DATA_NAME_COMPLETE;
			data[*data_len].data_len = strlen(name);
			data[*data_len].data = (const u8_t *)name;
			(*data_len)++;
		} else if (!strcmp(arg, "scan-response")) {
			if (data == sd) {
				printf("Failed to set advertising data: "
						   "duplicate scan-response option");
				return -ENOEXEC;
			}

			data = sd;
			data_len = &sd_len;
		} else if (!strcmp(arg, "len")) {
			if(argn + 1 == argc) {
               return -ENOEXEC;
			}
			uint32_t offset = 0;
			uint32_t len =  strtoul(argv[argn + 1], NULL, 10);
			for (int i = 0; i< len; i++){
				hex_data[i] = i % 9;
			}
			while (len >= 254)
			{
				data[*data_len].type = 0xFF;
				data[*data_len].data_len = 254;
				data[*data_len].data = hex_data + offset;
				(*data_len)++;
				offset += 254;
				len -= 254;
			}
			if (len)
			{
				data[*data_len].type = 0xFF;
				data[*data_len].data_len = len;
				data[*data_len].data = hex_data + offset;
				(*data_len)++;
			}
			argn++;
		} else {
			size_t len;

			len = hex2bin(arg, strlen(arg), &hex_data[hex_data_len],
				      sizeof(hex_data) - hex_data_len);

			if (!len || (len - 1) != (hex_data[hex_data_len])) {
				printf("Failed to set advertising data: "
						   "malformed hex data");
				return -ENOEXEC;
			}

			data[*data_len].type = hex_data[hex_data_len + 1];
			data[*data_len].data_len = len - 2;
			data[*data_len].data = &hex_data[hex_data_len + 2];
			(*data_len)++;
			hex_data_len += len;
		}
	}

	err = bt_le_ext_adv_set_data(adv, ad_len > 0 ? ad : NULL, ad_len,
					  sd_len > 0 ? sd : NULL, sd_len);
	if (err) {
		printf("Failed to set advertising set data (%d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_adv_start(int argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	struct bt_le_ext_adv_start_param param;
	uint8_t num_events = 0;
	int32_t timeout = 0;
	int err;

	if (!adv) {
		printf( "Advertiser[%d] not created", selected_adv);
		return -EINVAL;
	}

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "timeout")) {
			if (++argn == argc) {
				goto fail;
			}

			timeout = strtoul(argv[argn], NULL, 16);
		}

		if (!strcmp(arg, "num-events")) {
			if (++argn == argc) {
				goto fail;
			}

			num_events = strtoul(argv[argn], NULL, 16);
		}
	}

	param.timeout = timeout;
	param.num_events = num_events;

	err = bt_le_ext_adv_start(adv, &param);
	if (err) {
		printf( "Failed to start advertising set (%d)", err);
		return -ENOEXEC;
	}

	printf( "Advertiser[%d] %p set started", selected_adv, adv);
	return 0;

fail:

	return -ENOEXEC;
}

static int cmd_adv_stop(int argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	int err;

	if (!adv) {
		printf( "Advertiser[%d] not created", selected_adv);
		return -EINVAL;
	}

	err = bt_le_ext_adv_stop(adv);
	if (err) {
		printf( "Failed to stop advertising set (%d)", err);
		return -ENOEXEC;
	}

	printf( "Advertiser set stopped");
	return 0;
}

static int cmd_adv_delete(int argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	int err;

	if (!adv) {
		printf( "Advertiser[%d] not created", selected_adv);
		return -EINVAL;
	}

	err = bt_le_ext_adv_delete(adv);
	if (err) {
		return err;
	}

	adv_sets[selected_adv] = NULL;
	return 0;
}

static int cmd_adv_select(int argc, char *argv[])
{
	if (argc == 2) {
		uint8_t id = strtol(argv[1], NULL, 10);

		if (!(id < ARRAY_SIZE(adv_sets))) {
			return -EINVAL;
		}

		selected_adv = id;
		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(adv_sets); i++) {
		if (adv_sets[i]) {
			printf( "Advertiser[%d] %p", i, adv_sets[i]);
		}
	}

	return -ENOEXEC;
}

static int cmd_adv_info(int argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	struct bt_le_ext_adv_info info;
	int err;

	if (!adv) {
		return -EINVAL;
	}

	err = bt_le_ext_adv_get_info(adv, &info);
	if (err) {
		printf( "OOB data failed");
		return err;
	}

	printf( "Advertiser[%d] %p\r\n", selected_adv, adv);
	printf( "Id: %d, TX power: %d dBm\r\n", info.id, info.tx_power);

	return 0;
}

#if defined(CONFIG_BT_PERIPHERAL)

#define KEY_STR_LEN 33

static void print_le_oob(struct bt_le_oob *oob)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char c[KEY_STR_LEN];
	char r[KEY_STR_LEN];

	bt_addr_le_to_str(&oob->addr, addr, sizeof(addr));

	bin2hex(oob->le_sc_data.c, sizeof(oob->le_sc_data.c), c, sizeof(c));
	bin2hex(oob->le_sc_data.r, sizeof(oob->le_sc_data.r), r, sizeof(r));

	printf("OOB data:");
	printf("%-29s %-32s %-32s", "addr", "random", "confirm");
	printf("%29s %32s %32s", addr, r, c);
}

static int cmd_adv_oob(int argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	int err;

	if (!adv) {
		return -EINVAL;
	}

	err = bt_le_ext_adv_oob_get_local(adv, &oob_local);
	if (err) {
		printf( "OOB data failed");
		return err;
	}

	print_le_oob(&oob_local);

	return 0;
}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_PER_ADV)
static int cmd_per_adv(int argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];

	if (!adv) {
		printf( "No extended advertisement set selected");
		return -EINVAL;
	}

	if (!strcmp(argv[1], "off")) {
		if (bt_le_per_adv_stop(adv) < 0) {
			printf(
				    "Failed to stop periodic advertising");
		} else {
			printf( "Periodic advertising stopped");
		}
	} else if (!strcmp(argv[1], "on")) {
		if (bt_le_per_adv_start(adv) < 0) {
			printf(
				    "Failed to start periodic advertising");
		} else {
			printf( "Periodic advertising started");
		}
	} else {
		printf( "Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

	return 0;
}

static int cmd_per_adv_param(int argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	struct bt_le_per_adv_param param;
	int err;

	if (!adv) {
		printf( "No extended advertisement set selected");
		return -EINVAL;
	}

	if (argc > 1) {
		param.interval_min = strtol(argv[1], NULL, 16);
	} else {
		param.interval_min = BT_GAP_ADV_SLOW_INT_MIN;
	}

	if (argc > 2) {
		param.interval_max = strtol(argv[2], NULL, 16);
	} else {
		param.interval_max = param.interval_min * 1.2;

	}

	if (param.interval_min > param.interval_max) {
		printf(
			    "Min interval shall be less than max interval");
		return -EINVAL;
	}

	if (argc > 3 && !strcmp(argv[3], "tx-power")) {
		param.options = BT_LE_ADV_OPT_USE_TX_POWER;
	} else {
		param.options = 0;
	}

	err = bt_le_per_adv_set_param(adv, &param);
	if (err) {
		printf( "Failed to set periodic advertising "
			    "parameters (%d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_per_adv_data(int argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	static struct bt_data ad;
	static uint8_t hex_data[256];
	uint8_t ad_len = 0;
	int err;

	if (!adv) {
		printf( "No extended advertisement set selected");
		return -EINVAL;
	}

	memset(hex_data, 0, sizeof(hex_data));
	ad_len = hex2bin(argv[1], strlen(argv[1]), hex_data, sizeof(hex_data));

	if (!ad_len) {
		printf( "Could not parse adv data");
		return -ENOEXEC;
	}

	ad.data_len = hex_data[0];
	ad.type = hex_data[1];
	ad.data = &hex_data[2];

	err = bt_le_per_adv_set_data(adv, &ad, 1);
	if (err) {
		printf(
			    "Failed to set periodic advertising data (%d)",
			    err);
		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_PER_ADV */
#endif /* CONFIG_BT_EXT_ADV */



#if defined(CONFIG_BT_PER_ADV_SYNC)

static int cmd_per_adv_sync_create(int argc, char *argv[])
{
	int err;
	struct bt_le_per_adv_sync_param create_params = { 0 };
	uint32_t options = 0;
	struct bt_le_per_adv_sync **free_per_adv_sync = NULL;
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(per_adv_syncs); i++) {
		if (per_adv_syncs[i] == NULL) {
			free_per_adv_sync = &per_adv_syncs[i];
			break;
		}
	}

	if (i == ARRAY_SIZE(per_adv_syncs)) {
		printf( "Cannot create more per adv syncs");
		return -ENOEXEC;
	}

	err = bt_addr_le_from_str(argv[1], argv[2], &create_params.addr);
	if (err) {
		printf( "Invalid peer address (err %d)", err);
		return -ENOEXEC;
	}

	/* Default values */
	create_params.timeout = 1000; /* 10 seconds */
	create_params.skip = 10;

	create_params.sid = strtol(argv[3], NULL, 16);

	for (int i = 4; i < argc; i++) {
		if (!strcmp(argv[i], "aoa")) {
			options |= BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOA;
		} else if (!strcmp(argv[i], "aod_1us")) {
			options |= BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_1US;
		} else if (!strcmp(argv[i], "aod_2us")) {
			options |= BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_2US;
		} else if (!strcmp(argv[i], "only_cte")) {
			options |=
				BT_LE_PER_ADV_SYNC_OPT_SYNC_ONLY_CONST_TONE_EXT;
		} else if (!strcmp(argv[i], "timeout")) {
			if (++i == argc) {
				return 1;
			}

			create_params.timeout = strtoul(argv[i], NULL, 16);
		} else if (!strcmp(argv[i], "skip")) {
			if (++i == argc) {
				return 1;
			}

			create_params.skip = strtoul(argv[i], NULL, 16);
		} else {
			 
			return 1;
		}

		/* TODO: add support to parse using the per adv list */
	}

	create_params.options = options;

	err = bt_le_per_adv_sync_create(&create_params, free_per_adv_sync);
	if (err) {
		printf( "Per adv sync failed (%d)", err);
	} else {
		printf( "Per adv sync pending");
	}

	return 0;
}

static int cmd_per_adv_sync_delete(int argc, char *argv[])
{
	struct bt_le_per_adv_sync *per_adv_sync = NULL;
	int index;
	int err;

	if (argc > 1) {
		index = strtol(argv[1], NULL, 10);
	} else {
		index = 0;
	}

	if (index >= ARRAY_SIZE(per_adv_syncs)) {
		printf( "Maximum index is %lu but %u was requested",
			    ARRAY_SIZE(per_adv_syncs) - 1, index);
	}

	per_adv_sync = per_adv_syncs[index];

	if (!per_adv_sync) {
		return -EINVAL;
	}

	err = bt_le_per_adv_sync_delete(per_adv_sync);

	if (err) {
		printf( "Per adv sync delete failed (%d)", err);
	} else {
		printf( "Per adv sync deleted");
		per_adv_syncs[index] = NULL;
	}

	return 0;
}

static int cmd_past_subscribe(int argc, char *argv[])
{
	struct bt_le_per_adv_sync_transfer_param param;
	int err;
	int i = 0;
	bool global = true;

	if (i == ARRAY_SIZE(per_adv_syncs)) {
		printf( "Cannot create more per adv syncs");
		return -ENOEXEC;
	}

	/* Default values */
	param.options = 0;
	param.timeout = 1000; /* 10 seconds */
	param.skip = 10;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "aoa")) {
			param.options |=
				BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOA;
		} else if (!strcmp(argv[i], "aod_1us")) {
			param.options |=
				BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOD_1US;
		} else if (!strcmp(argv[i], "aod_2us")) {
			param.options |=
				BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOD_2US;
		} else if (!strcmp(argv[i], "only_cte")) {
			param.options |=
				BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_ONLY_CTE;
		} else if (!strcmp(argv[i], "timeout")) {
			if (++i == argc) {
				return 1;
			}

			param.timeout = strtoul(argv[i], NULL, 16);
		} else if (!strcmp(argv[i], "skip")) {
			if (++i == argc) {
				 
				return 1;
			}

			param.skip = strtoul(argv[i], NULL, 16);
		} else if (!strcmp(argv[i], "conn")) {
			if (!default_conn) {
				printf( "Not connected");
				return -EINVAL;
			}
			global = false;
		} else {
			return 1;
		}
	}

	bt_le_per_adv_sync_cb_register(&per_adv_sync_cb);

	err = bt_le_per_adv_sync_transfer_subscribe(
		global ? NULL : default_conn, &param);

	if (err) {
		printf( "PAST subscribe failed (%d)", err);
	} else {
		printf( "Subscribed to PAST");
	}

	return 0;
}

static int cmd_past_unsubscribe(int argc, char *argv[])
{
	int err;

	if (argc > 1) {
		if (!strcmp(argv[1], "conn")) {
			if (default_conn) {
				err =bt_le_per_adv_sync_transfer_unsubscribe(default_conn);
			} else {
				printf( "Not connected");
				return -EINVAL;
			}
		} else {
			 
			return 1;
		}
	} else {
		err = bt_le_per_adv_sync_transfer_unsubscribe(NULL);
	}

	if (err) {
		printf( "PAST unsubscribe failed (%d)", err);
	}

	return err;
}

static int cmd_per_adv_sync_transfer(int argc, char *argv[])
{
	int err;
	int index;
	struct bt_le_per_adv_sync *per_adv_sync;

	if (argc > 1) {
		index = strtol(argv[1], NULL, 10);
	} else {
		index = 0;
	}

	if (index >= ARRAY_SIZE(per_adv_syncs)) {
		printf("Maximum index is %lu but %u was requested",
			    ARRAY_SIZE(per_adv_syncs) - 1, index);
	}

	per_adv_sync = per_adv_syncs[index];
	if (!per_adv_sync) {
		return -EINVAL;
	}

	err = bt_le_per_adv_sync_transfer(per_adv_sync, default_conn, 0);
	if (err) {
		printf( "Periodic advertising sync transfer failed (%d)", err);
	}

	return err;
}
#endif /* CONFIG_BT_PER_ADV_SYNC */

#if defined(CONFIG_BT_CONN)
static int cmd_connect_le(int argc, char *argv[])
{
    int err;
    dev_addr_t addr;
    int16_t conn_handle;
    conn_param_t param = {
        CONN_INT_MIN_INTERVAL,
        CONN_INT_MAX_INTERVAL,
        0,
        400,
    };

    if (argc < 3) {
        return -EINVAL;
    }

    if (argc >= 3) {
        err = str2bt_addr_le(argv[1], argv[2], &addr);

        if (err) {
            printf("Invalid peer address (err %d)\n", err);
            return 0;
        }
    }

    if (argc >= 5) {
        param.interval_min = strtol(argv[3], NULL, 16);
        param.interval_max = strtol(argv[4], NULL, 16);
    }

    if (argc >= 7) {
        param.latency = strtol(argv[5], NULL, 16);
        param.timeout = strtol(argv[6], NULL, 16);
    }

    conn_handle = ble_stack_connect(&addr, &param, 0);

    if (conn_handle < 0) {
        printf("Connection failed\n");
        return -EIO;
    }

    return 0;
}

static int cmd_conn_select(int argc, char *argv[])
{
    int16_t  conn_handle = 0;
    conn_handle = strtol(argv[1], NULL, 10);

    g_bt_conn_handle = conn_handle;

    printf("select connect handle %d\n", g_bt_conn_handle);

    return 0;
}

static int cmd_disconnect(int argc, char *argv[])
{
    int16_t  conn_handle = 0;
    int err;

    if (g_bt_conn_handle != -1 && argc < 2) {
        conn_handle = g_bt_conn_handle;
    } else {
        if (argc < 2) {
            return -EINVAL;
        }

        conn_handle = strtol(argv[1], NULL, 10);
    }

    err = ble_stack_disconnect(conn_handle);

    if (err) {
        printf("Disconnection failed (err %d)\n", err);
    }

    return 0;
}

static int cmd_auto_conn(int argc, char *argv[])
{
    dev_addr_t addr;
    conn_param_t param = {
        CONN_INT_MIN_INTERVAL,
        CONN_INT_MAX_INTERVAL,
        0,
        400,
    };
    int err;
    conn_param_t *pparam = NULL;
    uint8_t auto_conn = 1;

    if (argc < 3) {
        return -EINVAL;
    }

    err = str2bt_addr_le(argv[1], argv[2], &addr);

    if (err) {
        printf("Invalid peer address (err %d)\n", err);
        return -EINVAL;
    }

    if (argc > 3) {
        if (!strcmp(argv[3], "on")) {
            auto_conn = 1;
            pparam = &param;
        } else if (!strcmp(argv[3], "off")) {
            auto_conn = 0;
            pparam = NULL;
        } else {
            return -EINVAL;
        }
    } else {
        auto_conn = 1;
        pparam = &param;
    }

    err = ble_stack_connect(&addr, pparam, auto_conn);

    if (err < 0) {
        return err;
    }

    printf("connect (%d) pending\n", err);
    return 0;
}

static int cmd_select(int argc, char *argv[])
{
    struct bt_conn *conn;
    bt_addr_le_t addr;
    int err;

    if (argc < 3) {
        return -EINVAL;
    }

    err = str2bt_addr_le(argv[1], argv[2], (dev_addr_t *)&addr);

    if (err) {
        printf("Invalid peer address (err %d)\n", err);
        return 0;
    }

    conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &addr);

    if (!conn) {
        printf("No matching connection found\n");
        return 0;
    }

    if (default_conn) {
        bt_conn_unref(default_conn);
    }

    default_conn = conn;

    return 0;
}

static int cmd_conn_update(int argc, char *argv[])
{
    conn_param_t param;
    int err;

    if (argc < 5) {
        return -EINVAL;
    }

    param.interval_min = strtoul(argv[1], NULL, 16);
    param.interval_max = strtoul(argv[2], NULL, 16);
    param.latency = strtoul(argv[3], NULL, 16);
    param.timeout = strtoul(argv[4], NULL, 16);
    err = ble_stack_connect_param_update(g_bt_conn_handle, &param);

    if (err) {
        printf("conn update failed (err %d).\n", err);
    } else {
        printf("conn update initiated.\n");
    }

    return 0;
}

static int cmd_oob(int argc, char *argv[])
{
    char addr[BT_ADDR_LE_STR_LEN];
    struct bt_le_oob oob;
    int err;

    err = bt_le_oob_get_local(selected_id, &oob);

    if (err) {
        printf("OOB data failed\n");
        return 0;
    }

    bt_addr_le_to_str(&oob.addr, addr, sizeof(addr));

    printf("OOB data:\n");
    printf("  addr %s\n", addr);

    return 0;
}

static int cmd_clear(int argc, char *argv[])
{
    dev_addr_t addr;
    int err;

    if (argc < 2) {
        printf("Specify remote address or \"all\"\n");
        return 0;
    }

    if (strcmp(argv[1], "all") == 0) {
        // err = bt_unpair(selected_id, NULL);
        err = ble_stack_dev_unpair(NULL);

        if (err) {
            printf("Failed to clear pairings (err %d)\n", err);
        } else {
            printf("Pairings successfully cleared\n");
        }

        return 0;
    }

    if (argc < 3) {
#if defined(CONFIG_BT_BREDR)
        addr.type = BT_ADDR_LE_PUBLIC;
        err = str2bt_addr(argv[1], &addr.a);
#else
        printf("Both address and address type needed\n");
        return 0;
#endif
    } else {
        err = str2bt_addr_le(argv[1], argv[2], (dev_addr_t *)&addr);
    }

    if (err) {
        printf("Invalid address\n");
        return 0;
    }

    //err = bt_unpair(selected_id, &addr);
    err = ble_stack_dev_unpair(&addr);

    if (err) {
        printf("Failed to clear pairing (err %d)\n", err);
    } else {
        printf("Pairing successfully cleared\n");
    }

    return 0;
}
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
static int cmd_security(int argc, char *argv[])
{
    int err, sec;

    if (g_bt_conn_handle == -1) {
        printf("Not connected\n");
        return 0;
    }

    if (argc < 2) {
        return -EINVAL;
    }

    sec = *argv[1] - '0';

    err = ble_stack_security(g_bt_conn_handle, sec);

    if (err) {
        printf("Setting security failed (err %d)\n", err);
    }

    return 0;
}

static void smp_passkey_display(evt_data_smp_passkey_display_t *e)
{
    char addr[32];

    bt_addr2str(&e->peer_addr, addr, 32);

    printf("Passkey for %s: %s\n", addr, e->passkey);
}

static void smp_passkey_confirm(evt_data_smp_passkey_confirm_t *e)
{
    char addr[32];

    bt_addr2str(&e->peer_addr, addr, 32);

    printf("Pairing passkey for %s: %s\n", addr, e->passkey);
}

static void smp_passkey_entry(evt_data_smp_passkey_enter_t *e)
{
    char addr[32];

    bt_addr2str(&e->peer_addr, addr, 32);

    printf("Enter passkey for %s\n", addr);
}

static void smp_cancel(evt_data_smp_cancel_t *e)
{
    char addr[32];

    bt_addr2str(&e->peer_addr, addr, 32);

    printf("Pairing cancelled: %s\n", addr);

    /* clear connection reference for sec mode 3 pairing */
    if (g_pairing_handle) {
        g_pairing_handle = -1;
    }
}

static void smp_pairing_confirm(evt_data_smp_pairing_confirm_t *e)
{
    char addr[32];

    bt_addr2str(&e->peer_addr, addr, 32);

    printf("Confirm pairing for %s\n", addr);
}

static void smp_pairing_complete(evt_data_smp_pairing_complete_t *e)
{
    char addr[32];

    bt_addr2str(&e->peer_addr, addr, 32);

    if (e->err) {
        printf("Pairing failed with %s, err %d\n", addr, e->err);
    } else {
        printf("%s with %s\n", e->bonded ? "Bonded" : "Paired",  addr);
    }
}

static void smp_event(ble_event_en event, void *event_data)
{
    switch (event) {
    case EVENT_SMP_PASSKEY_DISPLAY:
        smp_passkey_display(event_data);
        break;

    case EVENT_SMP_PASSKEY_CONFIRM:
        smp_passkey_confirm(event_data);
        break;

    case EVENT_SMP_PASSKEY_ENTER:
        smp_passkey_entry(event_data);
        break;

    case EVENT_SMP_PAIRING_CONFIRM:
        smp_pairing_confirm(event_data);
        break;

    case EVENT_SMP_PAIRING_COMPLETE:
        smp_pairing_complete(event_data);
        break;

    case EVENT_SMP_CANCEL:
        smp_cancel(event_data);
        break;

    default:
        break;
    }
}

static int cmd_iocap_set(int argc, char *argv[])
{
    uint8_t io_cap = 0;

    if (argc < 3) {
        return -EINVAL;
    }

    if (!strcmp(argv[1], "NONE")) {
        io_cap |= IO_CAP_IN_NONE;
    } else if (!strcmp(argv[1], "YESNO")) {
        io_cap |= IO_CAP_IN_YESNO;
    } else if (!strcmp(argv[1], "KEYBOARD")) {
        io_cap |= IO_CAP_IN_KEYBOARD;
    } else {
        return -EINVAL;
    }

    if (!strcmp(argv[2], "NONE")) {
        io_cap |= IO_CAP_OUT_NONE;
    } else if (!strcmp(argv[2], "DISPLAY")) {
        io_cap |= IO_CAP_OUT_DISPLAY;
    } else {
        return -EINVAL;
    }

    return ble_stack_iocapability_set(io_cap);
}

static int cmd_auth_cancel(int argc, char *argv[])
{
    int16_t conn_handle;

    if (g_bt_conn_handle != -1) {
        conn_handle = g_bt_conn_handle;
    } else if (g_pairing_handle != -1) {
        conn_handle = g_pairing_handle;
    } else {
        conn_handle = 0;
    }

    ble_stack_smp_cancel(conn_handle);
    return 0;
}

static int cmd_auth_passkey_confirm(int argc, char *argv[])
{
    if (g_bt_conn_handle == -1) {
        printf("Not connected\n");
        return 0;
    }

    ble_stack_smp_passkey_confirm(g_bt_conn_handle);
    return 0;
}

static int cmd_auth_pairing_confirm(int argc, char *argv[])
{
    if (g_bt_conn_handle == -1) {
        printf("Not connected\n");
        return 0;
    }

    ble_stack_smp_pairing_confirm(g_bt_conn_handle);

    return 0;
}

#if defined(CONFIG_BT_FIXED_PASSKEY)
static int cmd_fixed_passkey(int argc, char *argv[])
{
    unsigned int passkey;
    int err;

    if (argc < 2) {
        bt_passkey_set(BT_PASSKEY_INVALID);
        printf("Fixed passkey cleared\n");
        return 0;
    }

    passkey = atoi(argv[1]);

    if (passkey > 999999) {
        printf("Passkey should be between 0-999999\n");
        return 0;
    }

    err = bt_passkey_set(passkey);

    if (err) {
        printf("Setting fixed passkey failed (err %d)\n", err);
    }

    return 0;
}
#endif

static int cmd_auth_passkey(int argc, char *argv[])
{
    unsigned int passkey;

    if (g_bt_conn_handle  == -1) {
        printf("Not connected\n");
        return 0;
    }

    if (argc < 2) {
        return -EINVAL;
    }

    passkey = atoi(argv[1]);

    if (passkey > 999999) {
        printf("Passkey should be between 0-999999\n");
        return 0;
    }

    ble_stack_smp_passkey_entry(g_bt_conn_handle, passkey);
    return 0;
}
#endif /* CONFIG_BT_SMP) || CONFIG_BT_BREDR */

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
static u32_t l2cap_rate;

static void l2cap_recv_metrics(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
    return;
}

static void l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
    printf("Incoming data channel %p psm %d len %u\n", chan, chan->psm, buf->len);

    if (buf->len) {
        hexdump(buf->data, buf->len);
    }
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
    printf("Channel %p psm %d connected\n", chan, chan->psm);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
    printf("Channel %p psm %d disconnected\n", chan, chan->psm);
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
    /* print if metrics is disabled */
    if (chan->ops->recv != l2cap_recv_metrics) {
        printf("Channel %p requires buffer\n", chan);
    }

    return net_buf_alloc(&data_rx_pool, K_FOREVER);
}

static struct bt_l2cap_chan_ops l2cap_ops = {
    .alloc_buf  = l2cap_alloc_buf,
    .recv       = l2cap_recv,
    .connected  = l2cap_connected,
    .disconnected   = l2cap_disconnected,
};

static struct bt_l2cap_le_chan l2cap_chan[L2CAP_DYM_CHANNEL_NUM] = {
    0
};

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
    printf("Incoming conn %p\n", conn);

    int i;

    for (i = 0; i < L2CAP_DYM_CHANNEL_NUM; i++) {
        if (l2cap_chan[i].chan.conn == NULL) {
            break;
        }
    }

    if (i == L2CAP_DYM_CHANNEL_NUM) {
        printf("No channels available\n");
        return -ENOMEM;
    }

    l2cap_chan[i].chan.ops = &l2cap_ops;
    l2cap_chan[i].rx.mtu = DATA_MTU;

    *chan = &l2cap_chan[i].chan;

    return 0;
}

static struct bt_l2cap_server server[L2CAP_DYM_CHANNEL_NUM] = {
    0
};


static int cmd_l2cap_register(int argc, char *argv[])
{
    int i;

    if (argc < 2) {
        return -EINVAL;
    }

    for (i = 0; i < L2CAP_DYM_CHANNEL_NUM; i++) {
        if (server[i].psm == 0) {
            break;
        }
    }

    if (i == L2CAP_DYM_CHANNEL_NUM) {
        printf("No more channel\n");
        return 0;
    }

    server[i].accept = l2cap_accept;
    server[i].psm = strtoul(argv[1], NULL, 16);

    if (argc > 2) {
        server[i].sec_level = strtoul(argv[2], NULL, 10);
    }

    if (bt_l2cap_server_register(&server[i]) < 0) {
        printf("Unable to register psm\n");
        server[i].psm = 0;
    } else {
        printf("L2CAP psm %u sec_level %u registered\n", server[i].psm,
               server[i].sec_level);
    }

    return 0;
}

static int cmd_l2cap_connect(int argc, char *argv[])
{
    u16_t psm;
    int err;
    int i;

    if (g_bt_conn_handle == -1) {
        printf("Not connected\n");
        return 0;
    }

    if (argc < 2) {
        return -EINVAL;
    }

    for (i = 0; i < L2CAP_DYM_CHANNEL_NUM; i++) {
        if (l2cap_chan[i].chan.conn == NULL) {
            break;
        }
    }

    if (i == L2CAP_DYM_CHANNEL_NUM) {
        printf("No more Channel\n");
        return -EINVAL;
    }

    l2cap_chan[i].chan.ops = &l2cap_ops;
    l2cap_chan[i].rx.mtu = DATA_MTU;

    psm = strtoul(argv[1], NULL, 16);

    err = bt_l2cap_chan_connect(bt_conn_lookup_handle(g_bt_conn_handle), &l2cap_chan[i].chan, psm);

    if (err < 0) {
        printf("Unable to connect to psm %u (err %u)\n", psm, err);
    } else {
        printf("L2CAP connection pending\n");
    }

    return 0;
}

static int cmd_l2cap_disconnect(int argc, char *argv[])
{
    int err;
    u16_t psm;
    int i;

    psm = strtoul(argv[1], NULL, 16);

    for (i = 0; i < L2CAP_DYM_CHANNEL_NUM; i++) {
        if (l2cap_chan[i].chan.psm == psm) {
            err = bt_l2cap_chan_disconnect(&l2cap_chan[i].chan);

            if (err) {
                printf("Unable to disconnect: err %u\n", -err);
            }

            return err;
        }
    }

    return 0;
}

static int cmd_l2cap_send(int argc, char *argv[])
{
    static u8_t buf_data[DATA_MTU] = { [0 ...(DATA_MTU - 1)] = 0xff };
    int ret, len, count = 1;
    struct net_buf *buf;
    u16_t psm = 0;
    int i;

    if (argc > 1) {
        psm = strtoul(argv[1], NULL, 16);
    } else {
        return -EINVAL;
    }

    if (argc > 2) {
        count = strtoul(argv[2], NULL, 10);
    }

    for (i = 0; i < L2CAP_DYM_CHANNEL_NUM; i++) {
        if (l2cap_chan[i].chan.psm == psm) {
            break;
        }
    }

    if (i == L2CAP_DYM_CHANNEL_NUM) {
        printf("Can't find channel\n");
        return -EINVAL;
    }

    /* when mtu is 23, the max send num is 8, so 6*23 is safe */
    len = min(6*23, DATA_MTU - BT_L2CAP_CHAN_SEND_RESERVE);

    while (count--) {
        buf = net_buf_alloc(&data_tx_pool, K_FOREVER);
        net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);

        net_buf_add_mem(buf, buf_data, len);
        ret = bt_l2cap_chan_send(&l2cap_chan[i].chan, buf);

        if (ret < 0) {
            printf("Unable to send: %d\n", -ret);
            net_buf_unref(buf);
            break;
        }
    }

    return 0;
}

static int cmd_l2cap_metrics(int argc, char *argv[])
{
    const char *action;

    if (argc < 2) {
        printf("l2cap rate: %u bps.\n", l2cap_rate);

        return 0;
    }

    action = argv[1];

    if (!strcmp(action, "on")) {
        l2cap_ops.recv = l2cap_recv_metrics;
    } else if (!strcmp(action, "off")) {
        l2cap_ops.recv = l2cap_recv;
    } else {
        return -EINVAL;
    }

    printf("l2cap metrics %s.\n", action);

    return 0;
}

#endif




#if 0
static int write_mac_addr(partition_t handle, const uint8_t *buffer, int length, int offset)
{
    // if ((offset % lp->sector_size) == 0) {
    //     if (partition_erase(handle, offset, (1 + length / lp->sector_size)) < 0) {
    //         printf("erase addr:%x\r\n", offset);
    //         return -1;
    //     }
    // }

    // if (partition_write(handle, offset, (void *)buffer, length) >= 0) {
    //     return length;
    // }

    // printf("write fail addr:%x length:%x\r\n", offset, length);
    return -1;
}


static int str2_char(const char *str, uint8_t *addr)
{
    int i, j;
    u8_t tmp;

    if (strlen(str) != 17) {
        return -EINVAL;
    }

    for (i = 0, j = 1; *str != '\0'; str++, j++) {
        if (!(j % 3) && (*str != ':')) {
            return -EINVAL;
        } else if (*str == ':') {
            i++;
            continue;
        }

        addr[i] = addr[i] << 4;

        if (char2hex(*str, &tmp) < 0) {
            return -EINVAL;
        }

        addr[i] |= tmp;
    }

    return 0;
}


static int  flash_opt_mac(int argc, char *argv[])
{
    int err;
    uint8_t addr[6] = {0};
    uint8_t send_addr[6] = {0};
    const char *action;

    handle = partition_open("FCDS");
    lp = hal_flash_get_info(handle);

    action = argv[1];

    if ((argc == 3) && (!strcmp(action, "write"))) {
        err = str2_char(argv[2], addr);

        if (err < 0) {
            return err;
        }

        if (err) {
            printf("Invalid address\n");
            return err;
        }

        send_addr[0] = addr[2];
        send_addr[1] = addr[3];
        send_addr[2] = addr[4];
        send_addr[3] = addr[5];
        send_addr[4] = addr[0];
        send_addr[5] = addr[1];

        if (write_mac_addr(handle, send_addr, sizeof(send_addr), 0) < 0) {
            return -1;
        }
    } else if (!strcmp(action, "read")) {
        partition_read(handle, 0, addr, sizeof(addr));
        printf("mac:%x:%x:%x:%x:%x:%x", addr[4], addr[5], addr[0], addr[1], addr[2], addr[3]);
    }

    partition_close(handle);

    return  0;
}
#endif

#if defined(CONFIG_BT_WHITELIST)

static int get_wl_size()
{
    int wl_actal_szie;
    wl_actal_szie = ble_stack_white_list_size();
    if(wl_actal_szie <= 0) {
        return -1;
    }
    if(wl_actal_szie > MAX_WL_SZIE) {
        printf("actual wl size is %d but upper wl list size is %d\n", wl_actal_szie, MAX_WL_SZIE);
    }
    return wl_actal_szie < MAX_WL_SZIE? wl_actal_szie : MAX_WL_SZIE;
}

bool is_wl_addr_exist(dev_addr_t addr)
{
    uint8_t index = 0;
    int size  = 0;
    size = get_wl_size();
	if(size < 0) {
		return -1;
	}

    for(index = 0; index < size; index++) {
        if(wl_list[index].set_flag) {
            if(!memcmp(&wl_list[index].addr,&addr,sizeof(wl_list[index].addr))) {
                return 1;
            }
        }
    }
    if(index >= size) {
        return 0;
    }
    return 0;
}

int add_addr_to_wl_list(dev_addr_t addr)
{
    uint8_t index = 0;
    int size  = 0;
    size = get_wl_size();
	if(size < 0) {
		return -1;
	}

    for(index = 0; index < size; index++) {
        if(!wl_list[index].set_flag) {
            memcpy(&wl_list[index].addr,&addr,sizeof(wl_list[index].addr));
            wl_list[index].set_flag = 1;
            break;
        }
    }

    if(index >= size) {
        printf("wl list is full\r\n");
        return -1;
    } else {
        return 0;
    }

}

int remove_addr_form_wl_list(dev_addr_t addr)
{
    uint8_t index = 0;
    int size  = 0;

    size = get_wl_size();
    if(size < 0) {
		return -1;
	}

    for(index = 0; index < size; index++) {
        if(wl_list[index].set_flag) {
            if(!memcmp(&wl_list[index].addr, &addr, sizeof(addr))) {
                memset(&wl_list[index],0, sizeof(wl_addr));
                break;
            }
        }
    }

    if(index >= size) {
        printf("wl addr not exist\r\n");
        return -1;
    } else {
        return 0;
    }
}

int show_wl_list()
{
    int found_flag = 0;
    uint8_t index = 0;
    int size  = 0;
    char addr_str[32] = {0};

    size = get_wl_size();
    if(size < 0) {
       return -1;
	}
    for(index = 0; index < size; index++) {
        if(wl_list[index].set_flag) {
            bt_addr2str(&wl_list[index].addr, addr_str, sizeof(addr_str));
            found_flag = 1;
            printf("wl %d: %s\r\n",index,addr_str);
        }
    }
    if(!found_flag) {
        printf("wl addr not exit\r\n");
        return -1;
    } else {
        return 0;
    }

}

int clear_wl_list()
{
    memset(wl_list,0,sizeof(wl_list));
    return 0;
}


static int cmd_wl_size(int argc, char *argv[])
{
    if(!ble_init_flag) {
        return -BLE_STACK_ERR_INIT;
    }
    int ret = ble_stack_white_list_size();
    printf("white list size is %d\n", ret);
    return 0;
}
static int cmd_wl_add(int argc, char *argv[])
{
    dev_addr_t addr;
    int err;

    if(!ble_init_flag) {
        return -BLE_STACK_ERR_INIT;
    }

    if (argc == 3) {
        err = str2bt_addr_le(argv[1], argv[2], &addr);

        if (err) {
            printf("Invalid address\n");
            return err;
        }
    } else {
        return -EINVAL;
    }
    if(is_wl_addr_exist(addr)) {
        printf("wl addr already exist\r\n");
        return 0;
    }


    err = ble_stack_white_list_add(&addr);
    if(!err) {
        err = add_addr_to_wl_list(addr);
        if(err) {
            printf("add to upper wl list faild\r\n");
        } else {
            printf("Add %s (%s) to white list\n", argv[1], argv[2]);
        }
    } else {
        printf("add wl addr faild\r\n");
    }
    return 0;
}

static int cmd_wl_remove(int argc, char *argv[])
{
    if(!ble_init_flag) {
        return -BLE_STACK_ERR_INIT;
    }

    dev_addr_t addr;
    int err;

    if (argc == 3) {
        err = str2bt_addr_le(argv[1], argv[2], &addr);

        if (err) {
            printf("Invalid address\n");
            return err;
        }
    } else {
        return -EINVAL;
    }

    if(!is_wl_addr_exist(addr)) {
        printf("wl addr not exist\r\n");
        return -1;
    }


    err = ble_stack_white_list_remove(&addr);
    if(!err) {
        err = remove_addr_form_wl_list(addr);
        if(err) {
            printf("remove from upper wl list faild\r\n");
        } else {
            printf("Remove %s (%s) to white list\n", argv[1], argv[2]);
        }
    } else {
        printf("add wl addr faild\r\n");
    }
    return err;
}
static int cmd_wl_clear(int argc, char *argv[])
{
    if(!ble_init_flag) {
        return -BLE_STACK_ERR_INIT;
    }
    int err = 0;
    printf("Clear white list\n");
    err = ble_stack_white_list_clear();
    if(!err) {
        clear_wl_list();
    }
    return err;
}

static int cmd_wl_show(int argc, char *argv[])
{
    if(!ble_init_flag) {
        return -BLE_STACK_ERR_INIT;
    }
    show_wl_list();
    return 0;
}
#endif

#define HELP_NONE "[none]"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

#if defined(CONFIG_BT_EXT_ADV)
#define EXT_ADV_SCAN_OPT " [coded] [no-1m]"
#define EXT_ADV_PARAM "<type: conn-scan conn-nscan, nconn-scan nconn-nscan> " \
		      "[ext-adv] [no-2m] [coded] "                            \
		      "[whitelist: wl, wl-scan, wl-conn] [identity] [name] "  \
		      "[directed "HELP_ADDR_LE"] [mode: low]"                 \
		      "[disable-37] [disable-38] [disable-39]"
#else
#define EXT_ADV_SCAN_OPT ""
#endif /* defined(CONFIG_BT_EXT_ADV) */

static const struct shell_cmd bt_commands[] = {	
    { "pub-addr", cmd_pub_addr, "XX:XX:XX:XX:XX:XX"},
    { "init", cmd_init, HELP_ADDR_LE },
    //{ "mac", flash_opt_mac, "<val:read/write> exp:write 11:22:33:44:55:66" },
#if defined(CONFIG_BT_HCI)
    { "hci-cmd", cmd_hci_cmd, "<ogf> <ocf> [data]" },
#endif
    { "phy-rate-set", cmd_phy_rate_set, "<tx rate:1M:0x01/2M:0x02/Coded: 0x04> <rx rate:1M:0x01/2M:0x02/Coded: 0x04>"},
    { "dle-enable", cmd_dle_enable, HELP_NONE },
    { "id-create", cmd_id_create, "[addr]" },
    { "id-reset", cmd_id_reset, "<id> [addr]" },
    { "id-delete", cmd_id_delete, "<id>" },
    { "id-show", cmd_id_show, HELP_NONE },
    { "id-select", cmd_id_select, "<id>" },
    { "name", cmd_name, "[name]" },
    {
        "scan", cmd_scan,
        "<value: active, passive, off> <dup filter: dups, nodups> "\
        "<scan interval> <scan window> "\
        "<ad(len|adtype|addata ...): 0xxxxxxxx> <sd(len|adtype|addata ...): 0xxxxxxxx>"
    },
    {
        "scan_filter", cmd_scan_filter,
        "<filter_policy: 0 any adv 1 white list>"
    },

    {
        "adv", cmd_advertise,
        "<type: stop, conn, nconn> <ad(len|adtype|addata ...): 0xxxxxxxx> <sd(len|adtype|addata ...): 0xxxxxxxx>"
    },
    #if defined(CONFIG_BT_EXT_ADV)
	{
	    "adv-create",cmd_adv_create, EXT_ADV_PARAM
	},
	{ 
	    "adv-param", cmd_adv_param, EXT_ADV_PARAM 
	},
	{ 
	    "adv-data", cmd_adv_data, "<data> [scan-response <data> ,<type: discov, name, hex>"
	},
	{
	   "adv-start",cmd_adv_start,"[timeout <timeout>] [num-events <num events>]"
	},
	{ 
	    "adv-stop" ,cmd_adv_stop, HELP_NONE
	},
	{ "adv-delete", cmd_adv_delete, HELP_NONE},
	{ "adv-select", cmd_adv_select, "[adv]"},
	{ "adv-info", cmd_adv_info, HELP_NONE},

#if defined(CONFIG_BT_PERIPHERAL)
	{ "adv-oob",cmd_adv_oob, HELP_NONE},
#endif /* CONFIG_BT_PERIPHERAL */
#endif
#if defined(CONFIG_BT_PER_ADV)
	{ "per-adv",cmd_per_adv,"<type: off, on>"},
	{ "per-adv-param",cmd_per_adv_param,"[<interval-min> [<interval-max> [tx_power]]]"},
	{ "per-adv-data",cmd_per_adv_data,"<data>"},
#endif /* CONFIG_BT_PER_ADV */
#if defined(CONFIG_BT_PER_ADV_SYNC)
	{
	  "per-adv-sync-create",cmd_per_adv_sync_create,
                  "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>  <sid> [skip <count>] [timeout <ms>] [aoa] \
				  [aod_1us] [aod_2us] [cte_only]"

	},
	{"per-adv-sync-delete", cmd_per_adv_sync_delete,"[<index>]"},
#endif /* defined(CONFIG_BT_PER_ADV_SYNC) */

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_PER_ADV_SYNC)
    {"past-subscribe",cmd_past_subscribe,"[conn] [skip <count>] [timeout <ms>] [aoa] [aod_1us] [aod_2us] [cte_only]"},
	{"past-unsubscribe", cmd_past_unsubscribe,"[conn]"},
	{"per-adv-sync-transfer",cmd_per_adv_sync_transfer,"[<index>]"},
#endif /* defined(CONFIG_BT_PER_ADV_SYNC) */
    {
        "connect", cmd_connect_le, HELP_ADDR_LE\
        " <interval_min> <interval_max>"\
        " <latency> <timeout>"
    },
    { "conn-select", cmd_conn_select, "[conn_handle]" },
    { "disconnect", cmd_disconnect, "[conn_handle]" },
    { "auto-conn", cmd_auto_conn, HELP_ADDR_LE" <action: on|off>"},
    { "select", cmd_select, HELP_ADDR_LE },
    { "conn-update", cmd_conn_update, "<min> <max> <latency> <timeout>" },
    { "oob", cmd_oob },
    { "clear", cmd_clear,"<dst:all,address> <type: (public|random>"},
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
    { "security", cmd_security, "<security level: 0, 1, 2, 3>" },
    {
        "io-capability", cmd_iocap_set,
        "<io input: NONE,YESNO, KEYBOARD> <io output: NONE,DISPLAY>"
    },
    { "auth-cancel", cmd_auth_cancel, HELP_NONE },
    { "auth-passkey", cmd_auth_passkey, "<passkey>" },
    { "auth-passkey-confirm", cmd_auth_passkey_confirm, HELP_NONE },
    { "auth-pairing-confirm", cmd_auth_pairing_confirm, HELP_NONE },
#if defined(CONFIG_BT_FIXED_PASSKEY)
    { "fixed-passkey", cmd_fixed_passkey, "[passkey]" },
#endif

#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR) */
#if defined(CONFIG_BT_GATT_CLIENT)
    { "gatt-exchange-mtu", cmd_gatt_exchange_mtu, HELP_NONE },
    {
        "gatt-discover-primary", cmd_gatt_discover,
        "<UUID> [start handle] [end handle]"
    },
    {
        "gatt-discover-secondary", cmd_gatt_discover,
        "<UUID> [start handle] [end handle]"
    },
    {
        "gatt-discover-include", cmd_gatt_discover,
        "[UUID] [start handle] [end handle]"
    },
    {
        "gatt-discover-characteristic", cmd_gatt_discover,
        "[UUID] [start handle] [end handle]"
    },
    {
        "gatt-discover-descriptor", cmd_gatt_discover,
        "[UUID] [start handle] [end handle]"
    },
    { "gatt-read-format",cmd_gatt_read_show_format,"<format:0,1>"},
    { "gatt-read", cmd_gatt_read, "<handle> [offset]" },
    { "gatt-read-multiple", cmd_gatt_mread, "<handle 1> <handle 2> ..." },
    { "gatt-write", cmd_gatt_write, "<handle> <offset> <data> [length]" },
    {
        "gatt-write-without-response", cmd_gatt_write_without_rsp,
        "<handle> <data> [length] [repeat]"
    },
    {
        "gatt-write-signed", cmd_gatt_write_without_rsp,
        "<handle> <data> [length] [repeat]"
    },
    {
        "gatt-subscribe", cmd_gatt_subscribe,
        "<CCC handle> [ind]"
    },
    { "gatt-unsubscribe", cmd_gatt_unsubscribe, "<CCC handle>"},
#endif /* CONFIG_BT_GATT_CLIENT */
    { "gatt-show-db", cmd_gatt_show_db, HELP_NONE },
    {
        "gatt-register-service", cmd_gatt_register_test_svc,
        "register pre-predefined test service"
    },
#if 0
    {
        "gatt-register-service2", cmd_gatt_register_test_svc,
        "register pre-predefined test2 service"
    },
#endif
#ifdef CONFIG_BT_SHELL_TRANSPORT_TEST
    {
        "gatt-transport-test-config", cmd_gatt_transport_test,
        "<type 0:server 1 client> <mode 0:loop 1:single> <server tx mode 0:notify 1:indicate> <server rx handle> <client tx mode 0:write 1:write_withoutresponse> "\
        "<data_len 0:stream 1~0xFFFFFFFF data size>"
    },
    {
        "gatt-transport-test-op", cmd_gatt_transport_test,
        "<op 0:stop 1:start 2:show result 3:reset>"
    },
#endif
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
    { "l2cap-register", cmd_l2cap_register, "<psm> [sec_level]" },
    { "l2cap-connect", cmd_l2cap_connect, "<psm>" },
    { "l2cap-disconnect", cmd_l2cap_disconnect, "<psm>" },
    { "l2cap-send", cmd_l2cap_send, "<psm> <number of packets>" },
    { "l2cap-metrics", cmd_l2cap_metrics, "<value on, off>" },
#endif


#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_CTLR_ADV_EXT)
    { "advx", cmd_advx, "<on off> [coded] [anon] [txp]" },
    { "scanx", cmd_scanx, "<on passive off> [coded]" },
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#if defined(CONFIG_BT_CTLR_DTM)
    { "test_tx", cmd_test_tx, "<chan> <len> <type> <phy>" },
    { "test_rx", cmd_test_rx, "<chan> <phy> <mod_idx>" },
    { "test_end", cmd_test_end, HELP_NONE},
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#if defined(CONFIG_BT_WHITELIST)
    { "wl-size", cmd_wl_size, HELP_NONE},
    { "wl-add", cmd_wl_add, HELP_ADDR_LE},
    { "wl-remove", cmd_wl_remove, HELP_ADDR_LE},
    { "wl-clear", cmd_wl_clear, HELP_NONE},
    { "wl-show", cmd_wl_show, HELP_NONE},
#endif
    { NULL, NULL, NULL }
};


static void cmd_ble_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int i = 0;
    int err;

    if (argc < 2) {
        printf("Ble support commands\n");

        for (i = 0; bt_commands[i].cmd_name != NULL; i ++) {
            printf("    %s %s\n", bt_commands[i].cmd_name, bt_commands[i].help);
        }

        return;
    }

    for (i = 0; bt_commands[i].cmd_name != NULL; i ++) {
        if (strlen(bt_commands[i].cmd_name) == strlen(argv[1]) &&
            !strncmp(bt_commands[i].cmd_name, argv[1], strlen(bt_commands[i].cmd_name))) {
            if (bt_commands[i].cb) {
                err = bt_commands[i].cb(argc - 1, &argv[1]);

                if (err) {
                    printf("%s execute fail, %d\n", bt_commands[i].cmd_name, err);
                }

                break;
            }
        }
    }
}

void cli_reg_cmd_ble(void)
{
    static const struct cli_command cmd_info = {
        "ble",
        "ble commands",
        cmd_ble_func,
    };

    aos_cli_register_command(&cmd_info);
}
