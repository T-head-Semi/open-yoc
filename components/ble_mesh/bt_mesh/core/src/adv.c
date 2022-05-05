/*  Bluetooth Mesh */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ble_os.h>
#include <errno.h>
#include <misc/stack.h>
#include <misc/util.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <api/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_ADV)
#include "common/log.h"

//#include "host/hci_core.h"

#include "adv.h"
#if defined(CONFIG_BT_MESH_EXT_ADV) && CONFIG_BT_MESH_EXT_ADV
#include "ext_adv.h"
#endif
#include "net.h"
#include "foundation.h"
#include "beacon.h"
#include "prov.h"
#include "proxy.h"

#ifdef CONFIG_BT_MESH_PROVISIONER
#include "provisioner_main.h"
#include "provisioner_prov.h"
#include "provisioner_beacon.h"
#endif

#ifdef GENIE_ULTRA_PROV
#include "genie_storage.h"
#include "genie_provision.h"
#endif


#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
#include "host/fsm.h"
#endif
#include <host/hci_core.h>

/* Convert from ms to 0.625ms units */
#define ADV_SCAN_UNIT(_ms) ((_ms) * 8 / 5)

#define ADV_SCAN_MS(interval) ((interval) * 5 / 8)

/* Window and Interval are equal for continuous scanning */
#ifndef CONFIG_MESH_SCAN_INTERVAL_MS
#define CONFIG_MESH_SCAN_INTERVAL_MS 30
#endif
#ifndef CONFIG_MESH_SCAN_WINDOW_MS
#define CONFIG_MESH_SCAN_WINDOW_MS 30
#endif

#define MESH_SCAN_INTERVAL    ADV_SCAN_UNIT(CONFIG_MESH_SCAN_INTERVAL_MS)
#define MESH_SCAN_WINDOW      ADV_SCAN_UNIT(CONFIG_MESH_SCAN_WINDOW_MS)

#ifndef CONFIG_ADV_SCAN_INTERVAL_TIMER
#define CONFIG_ADV_SCAN_INTERVAL_TIMER (1)//ms
#endif

#ifndef CONFIG_ADV_INTERVAL_TIMER
#define CONFIG_ADV_INTERVAL_TIMER      (20) //ms
#endif

/* Pre-5.0 controllers enforce a minimum interval of 100ms
 * whereas 5.0+ controllers can go down to 20ms.
 */
#define ADV_INT_DEFAULT_MS 100
#define ADV_INT_FAST_MS    20

#ifndef CONFIG_MAX_AD_NUM
#define CONFIG_MAX_AD_NUM 10
#endif


#ifdef CONFIG_GENIE_MESH_ENABLE
#ifndef GENIE_DEFAULT_DURATION
#define GENIE_DEFAULT_DURATION 120
#endif
#endif

#ifndef CONFIG_BT_ADV_STACK_SIZE

#if defined(CONFIG_BT_HOST_CRYPTO)
#define ADV_STACK_SIZE 2048
#else
#define ADV_STACK_SIZE 1024
#endif
#else
#define ADV_STACK_SIZE CONFIG_BT_ADV_STACK_SIZE
#endif

#ifndef CONFIG_BT_ADV_SCAN_SCHEDULE_IN_HOST
#define CONFIG_BT_ADV_SCAN_SCHEDULE_IN_HOST (0)
#endif

//static K_FIFO_DEFINE(adv_queue);
static struct kfifo adv_queue;
#if !(defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE)
static struct k_thread adv_thread_data;
static BT_STACK_NOINIT(adv_thread_stack, ADV_STACK_SIZE);
#endif

int bt_mesh_adv_scan_schd_init();

static const u8_t adv_type[] = {
	[BT_MESH_ADV_PROV]   = BT_DATA_MESH_PROV,
	[BT_MESH_ADV_DATA]   = BT_DATA_MESH_MESSAGE,
	[BT_MESH_ADV_BEACON] = BT_DATA_MESH_BEACON,
	[BT_MESH_ADV_URI]    = BT_DATA_URI,
};

NET_BUF_POOL_DEFINE(adv_buf_pool, CONFIG_BT_MESH_ADV_BUF_COUNT,
		    BT_MESH_ADV_DATA_SIZE, BT_MESH_ADV_USER_DATA_SIZE, NULL);

static struct bt_mesh_adv adv_pool[CONFIG_BT_MESH_ADV_BUF_COUNT];

#if defined(CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT) && CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT > 0
NET_BUF_POOL_DEFINE(relay_adv_buf_pool, CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT,
		    BT_MESH_ADV_DATA_SIZE, BT_MESH_ADV_USER_DATA_SIZE, NULL);
static struct bt_mesh_adv realy_adv_pool[CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT];
#endif

#ifdef CONFIG_BT_MESH_PROVISIONER
const bt_addr_le_t *dev_addr;
#endif

vendor_beacon_cb g_vendor_beacon_cb = NULL;


int bt_mesh_adv_scan_init(void);

static struct bt_mesh_adv *adv_alloc(int id)
{
	return &adv_pool[id];
}

#if defined(CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT) && CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT > 0
static struct bt_mesh_adv *relay_adv_alloc(int id)
{
	return &realy_adv_pool[id];
}
#endif

static inline void adv_send_start(u16_t duration, int err,
				  const struct bt_mesh_send_cb *cb,
				  void *cb_data)
{
#ifdef CONFIG_MESH_LPM
     extern void mesh_lpm_tx_handle();
     mesh_lpm_tx_handle();
#endif
	if (cb && cb->start) {
		cb->start(duration, err, cb_data);
	}
}

static inline void adv_send_end(int err, const struct bt_mesh_send_cb *cb,
				void *cb_data)
{
	if (cb && cb->end) {
		cb->end(err, cb_data);
	}
}

static struct bt_data ad[CONFIG_MAX_AD_NUM] = {0x00};
//static struct bt_data sd[CONFIG_MAX_AD_NUM];

static inline void adv_send(struct net_buf *buf)
{
    extern int bt_le_hci_version_get();
	const s32_t adv_int_min = ((bt_le_hci_version_get() >= BT_HCI_VERSION_5_0) ?
				   ADV_INT_FAST_MS : ADV_INT_DEFAULT_MS);
	const struct bt_mesh_send_cb *cb = BT_MESH_ADV(buf)->cb;
	void *cb_data = BT_MESH_ADV(buf)->cb_data;
	struct bt_le_adv_param param = {0};
	u16_t duration, adv_int;
	uint8_t ad_index = 0;
	struct net_buf *temp_buf = NULL;
	int err;


#if  defined(CONFIG_MESH_LPM) && defined(CONFIG_BT_MESH_PROVISIONER)
      if(BT_MESH_ADV(buf)->lpm_flag == 1) {
         BT_MESH_ADV(buf)->xmit = BT_MESH_TRANSMIT(7, 40);/* 8 transmissions with 40ms interval for lpm node*/
		 adv_int = 150;/* 8* 150 = 1.2s send data in 1.2s*/
	  } else {
	     adv_int = MAX(adv_int_min,
					BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit));
	  }
#else
	 adv_int = MAX(adv_int_min,BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit));
#endif


#ifdef CONFIG_GENIE_MESH_ENABLE
	duration = GENIE_DEFAULT_DURATION;
#else
	if (BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) == 0) {
		duration = adv_int;
	} else {
		duration = (((BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1) *
			     (adv_int)) +  10);
	}

#endif
	BT_DBG("type %u len %u: trans: %u %s", BT_MESH_ADV(buf)->type,
	       buf->len,BT_MESH_ADV(buf)->trans,bt_hex(buf->data, buf->len));
	BT_DBG("count %u interval %ums duration %ums",
	       BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1, adv_int,
	       duration);

        if(BT_MESH_ADV(buf)->type == BT_MESH_ADV_BEACON &&  \
		   bt_mesh_is_provisioned() &&  buf->data[0] == BEACON_TYPE_UNPROVISIONED) {
           BT_WARN("Ignore the unprov beacon data when provisioned");
		   net_buf_unref(buf);
		   return;
		}

	    temp_buf = buf;

	    while(temp_buf) {

#ifdef GENIE_ULTRA_PROV
		if (BT_MESH_ADV(buf)->tiny_adv == 1) {
			ad[ad_index].type = GENIE_PROV_ADV_TYPE;
		} else {
			ad[ad_index].type = adv_type[BT_MESH_ADV(temp_buf)->type];
		}
#else
		ad[ad_index].type = adv_type[BT_MESH_ADV(temp_buf)->type];
#endif
		ad[ad_index].data_len = temp_buf->len;
		ad[ad_index].data = temp_buf->data;
#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
		if(!ad_index) {
			param.sid    =  BT_MESH_ADV(temp_buf)->sid;
		}
#endif
		ad_index++;
		temp_buf = temp_buf->frags;
		}

		if (IS_ENABLED(CONFIG_BT_MESH_DEBUG_USE_ID_ADDR)) {
			param.options = BT_LE_ADV_OPT_USE_IDENTITY;
		} else {
			param.options = 0;
		}

	param.id = BT_ID_DEFAULT;
	param.interval_min = ADV_SCAN_UNIT(adv_int);
	param.interval_max = param.interval_min;


#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
	if (BT_MESH_ADV(buf)->trans == NET_TRANS_EXT_ADV_1M) {
		param.options |= BT_LE_ADV_OPT_EXT_ADV;
		param.options |= BT_LE_ADV_OPT_NO_2M;
	} else if (BT_MESH_ADV(buf)->trans == NET_TRANS_EXT_ADV_2M) {
		param.options |= BT_LE_ADV_OPT_EXT_ADV;
	} else if (BT_MESH_ADV(buf)->trans == NET_TRANS_EXT_ADV_CODED){
		param.options |= BT_LE_ADV_OPT_EXT_ADV;
		param.options |= BT_LE_ADV_OPT_CODED;
	}
#endif

	err = bt_mesh_adv_enable(&param, ad, ad_index, NULL, 0);
	adv_send_start(duration, err, cb, cb_data);
	if (err) {
		net_buf_unref(buf);
		BT_ERR("Advertising failed: err %d", err);
		return;
	}

	BT_DBG("Advertising started. Sleeping %u ms", duration);

	k_sleep(K_MSEC(duration));

	err = bt_mesh_adv_disable();
	net_buf_unref(buf);
	adv_send_end(err, cb, cb_data);
	if (err) {
		net_buf_unref(buf);
		BT_ERR("Stopping advertising failed: err %d", err);
		return;
	}

	BT_DBG("Advertising stopped");
}

//static void adv_stack_dump(const struct k_thread *thread, void *user_data)
//{
//#if defined(CONFIG_THREAD_STACK_INFO)
//	stack_analyze((char *)user_data, (char *)thread->stack_info.start,
//						thread->stack_info.size);
//#endif
//}

static void adv_thread(void *args)
{
	BT_DBG("started");

	while (1) {
		struct net_buf *buf;

		if (IS_ENABLED(CONFIG_BT_MESH_PROXY)) {
			buf = net_buf_get(&adv_queue, K_NO_WAIT);
			while (!buf) {
				s32_t timeout;
				timeout = bt_mesh_proxy_adv_start();
				BT_DBG("Proxy Advertising up to %d ms",
				       timeout);
				buf = net_buf_get(&adv_queue, timeout);
				bt_mesh_proxy_adv_stop();
			}
		} else {
			buf = net_buf_get(&adv_queue, K_FOREVER);
		}

		if (!buf) {
			continue;
		}

		/* busy == 0 means this was canceled */
		if (BT_MESH_ADV(buf)->busy) {
			adv_send(buf);
			BT_MESH_ADV(buf)->busy = 0;
		}
		else
		{
			net_buf_unref(buf);
		}

		//STACK_ANALYZE("adv stack", adv_thread_stack);
		//k_thread_foreach(adv_stack_dump, "BT_MESH");

		/* Give other threads a chance to run */
		k_yield();
	}
}

void bt_mesh_adv_update(void)
{
	BT_DBG("");
#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
    bt_mesh_fsm_refresh_state();
#else
	k_fifo_cancel_wait(&adv_queue);
#endif
}

struct net_buf *bt_mesh_adv_create_from_pool(struct net_buf_pool *pool,
						 bt_mesh_adv_alloc_t get_id,
						 enum bt_mesh_adv_type type,
						 u8_t xmit,u8_t trans, bool sid_change_flag, s32_t timeout)
{
    struct bt_mesh_adv *adv;
    struct net_buf *buf;
#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
    static uint8_t sid  = 0;
#endif
    if (atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED))
    {
        /*[Genie begin] add by wenbing.cwb at 2021-01-21*/
#if   defined(CONFIG_GENIE_MESH_GLP) || defined(CONFIG_MESH_LPM)
        BT_WARN("tx need resume stack while suspended");
        bt_mesh_resume();
#else
        BT_WARN("Refusing to allocate buffer while suspended");
        return NULL;
#endif
        /*[Genie end] add by wenbing.cwb at 2021-01-21*/
    }

#ifdef CONFIG_GENIE_RHYTHM
    extern uint8_t genie_rhythm_recv_stat(void);
    if (genie_rhythm_recv_stat() == 1)
    {
        BT_DBG("Refusing to allocate adv buffer while in rhythm recv stat");
        return NULL;
    }
#endif

    buf = net_buf_alloc(pool, timeout);
	if (!buf) {
		return NULL;
	}

	adv = get_id(net_buf_id(buf));
	BT_MESH_ADV(buf) = adv;

	memset(adv, 0, sizeof(*adv));

	adv->type		 = type;
	adv->xmit		 = xmit;
	adv->trans       = trans;
#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
	if(trans == NET_TRANS_EXT_ADV_1M || trans == NET_TRANS_EXT_ADV_2M || trans == NET_TRANS_EXT_ADV_CODED) {
		if(sid_change_flag) {
			adv->sid         = (sid++) % 0x10;
		} else {
		adv->sid         = sid;
		}
	}
#endif
	return buf;
}

struct net_buf *bt_mesh_adv_create_with_net_if(enum bt_mesh_adv_type type, u8_t xmit, u8_t trans, bool sid_change_flag ,
				   s32_t timeout)
{
	return bt_mesh_adv_create_from_pool(&adv_buf_pool, adv_alloc, type,
					    xmit, trans, sid_change_flag, timeout);
}

#if defined(CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT) && CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT > 0
struct net_buf *bt_mesh_relay_adv_create_with_net_if(enum bt_mesh_adv_type type, u8_t xmit, u8_t trans, bool sid_change_flag ,
				   s32_t timeout)
{
	return bt_mesh_adv_create_from_pool(&relay_adv_buf_pool, relay_adv_alloc, type,
					    xmit, trans, sid_change_flag, timeout);
}
#endif

int bt_mesh_adv_free_get(uint8_t *count, uint16_t *size)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE) && CONFIG_NET_BUF_POOL_USAGE > 0
   *count = adv_buf_pool.avail_count;
#else
   *count = adv_buf_pool.uninit_count + k_lifo_num_get(&(adv_buf_pool.free));
#endif
	*size  = BT_MESH_ADV_DATA_SIZE;
	return 0;
}

int bt_mesh_adv_busy_get(uint8_t *count, uint16_t *size)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE) && CONFIG_NET_BUF_POOL_USAGE > 0
   *count = adv_buf_pool.buf_count - adv_buf_pool.avail_count;
#else
   *count = adv_buf_pool.buf_count - (adv_buf_pool.uninit_count + k_lifo_num_get(&(adv_buf_pool.free)));
#endif
	*size  = BT_MESH_ADV_DATA_SIZE;
	return 0;
}

void bt_mesh_adv_send(struct net_buf *buf, const struct bt_mesh_send_cb *cb,
		      void *cb_data)
{
	if(NULL == buf){
		BT_WARN("buf is null");
		return;
	}
    struct net_buf *frag_buf = NULL;

	BT_DBG("type 0x%02x 0x%02x len %u: %s", BT_MESH_ADV(buf)->type, BT_MESH_ADV(buf)->trans,buf->len,
	      bt_hex(buf->data, buf->len));

	BT_MESH_ADV(buf)->cb = cb;
	BT_MESH_ADV(buf)->cb_data = cb_data;
	BT_MESH_ADV(buf)->busy = 1;

    frag_buf = buf;
	while(frag_buf) {
		net_buf_ref(frag_buf);
		frag_buf = frag_buf->frags;
	}

	net_buf_put(&adv_queue, buf);


#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
    bt_mesh_fsm_adv_send();
#endif
}

#ifdef CONFIG_BT_MESH_PROVISIONER
const bt_addr_le_t *bt_mesh_pba_get_addr(void)
{
  return dev_addr;
}
#endif

static void bt_mesh_scan_cb(const bt_addr_le_t *addr, s8_t rssi,
			    u8_t adv_type, struct net_buf_simple *buf)
{
#if defined (CONFIG_BT_MESH_PROVISIONER) || defined (GENIE_ULTRA_PROV) || defined (CONFIG_OTA_SERVER)
	if (adv_type != BT_LE_ADV_NONCONN_IND && adv_type != BT_LE_ADV_IND) {
#else
	if (adv_type != BT_LE_ADV_NONCONN_IND) {
#endif
		return;
	}
	//BT_DBG("len %u: %s", buf->len, bt_hex(buf->data, buf->len));
#ifdef CONFIG_BT_MESH_PROVISIONER
	dev_addr = addr;
#endif

	u16_t uuid = 0;
	while (buf->len > 1) {
		struct net_buf_simple_state state;
		u8_t len, type;

		len = net_buf_simple_pull_u8(buf);
		/* Check for early termination */
		if (len == 0) {
			return;
		}

		if (len > buf->len) {
			//BT_WARN("AD malformed");
			return;
		}

		net_buf_simple_save(buf, &state);

		type = net_buf_simple_pull_u8(buf);

		buf->len = len - 1;
		if (adv_type == BT_LE_ADV_NONCONN_IND) {
			switch (type) {
			case BT_DATA_MESH_MESSAGE:
#if defined(CONFIG_BT_MESH_RELAY_SRC_DBG)
				net_buf_trace.buf = buf;
				memcpy(net_buf_trace.addr, addr->a.val, 6);
				net_buf_trace.addr[6] = addr->type;
#endif

				bt_mesh_net_recv(buf, rssi, BT_MESH_NET_IF_ADV);
				break;
#if defined(CONFIG_BT_MESH_PB_ADV)
			case BT_DATA_MESH_PROV:
#ifdef CONFIG_BT_MESH_PROVISIONER
				if (bt_mesh_is_provisioner_en()) {
					provisioner_pb_adv_recv(buf);
					break;
				}
                else
#endif
				{
				    bt_mesh_pb_adv_recv(buf);
				}

				break;
#endif
			case BT_DATA_MESH_BEACON:
#ifdef CONFIG_BT_MESH_PROVISIONER
				if (bt_mesh_is_provisioner_en()) {
					provisioner_beacon_recv(buf);
				}
				else
#endif
				{
					bt_mesh_beacon_recv(buf);
				}

                break;

            case BT_DATA_MANUFACTURER_DATA:
                if (g_vendor_beacon_cb != NULL) {
                    g_vendor_beacon_cb(addr,rssi,adv_type,(void *)buf, buf->len);
                }

				break;
			default:
				break;
			}
		}
		else if (adv_type == BT_LE_ADV_IND) {
#ifdef GENIE_ULTRA_PROV
			genie_provision_ultra_prov_handle(type, (void *)buf);
#endif
			switch (type) {
#if defined (CONFIG_BT_MESH_PROVISIONER) && defined (CONFIG_BT_MESH_PB_GATT)
				case BT_DATA_FLAGS:
					if (bt_mesh_is_provisioner_en()) {
						if (!provisioner_flags_match(buf)) {
							BT_DBG("Flags mismatch, ignore this adv pkt");
							return;
						}
					}
					break;
				case BT_DATA_UUID16_ALL:
					if (bt_mesh_is_provisioner_en()) {
						uuid = provisioner_srv_uuid_recv(buf);
						if (!uuid) {
							BT_DBG("Service UUID mismatch, ignore this adv pkt");
							return;
						}
					}
					break;
				case BT_DATA_SVC_DATA16:
					if (bt_mesh_is_provisioner_en()) {
						provisioner_srv_data_recv(buf, addr, uuid);
					}
					break;
#endif
				default:
					break;
			}
			(void)uuid;
		}
		net_buf_simple_restore(buf, &state);
		net_buf_simple_pull(buf, len);
	}
}

#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0

extern adv_buf_frag_head *g_buf_frag_head;
extern struct net_buf * ext_adv_buf_data_gather(const struct bt_le_scan_recv_info *info, struct net_buf *buf, u8_t *gather_index);
extern int ext_adv_buf_data_process(const struct bt_le_scan_recv_info *info, struct net_buf *buf_head);
extern u8_t get_net_if(u8_t gap_phy);

int ext_process_siggle_buffer(const struct bt_le_scan_recv_info *info, struct net_buf_simple* buf)
{
    u16_t uuid = 0;
    s8_t rssi = info->rssi;
    u8_t bear_trans = 0;
    u8_t adv_type = info->adv_type;
#if defined (CONFIG_BT_MESH_PROVISIONER) && defined (CONFIG_BT_MESH_PB_GATT)
    const bt_addr_le_t *addr = info->addr;
#endif
    while (buf->len > 1) {
        struct net_buf_simple_state state;
        u8_t len, type;

        len = net_buf_simple_pull_u8(buf);
        /* Check for early termination */
        if (len == 0) {
            return -EINVAL;
        }

        if (len > buf->len) {
            //BT_WARN("AD malformed");
            return -EINVAL;
        }

        net_buf_simple_save(buf, &state);

        type = net_buf_simple_pull_u8(buf);

        buf->len = len - 1;
        if (adv_type == BT_LE_ADV_NONCONN_IND || adv_type == BT_LE_ADV_EXT_ADV_COMPLETE) {
            switch (type) {
            case BT_DATA_MESH_MESSAGE:
#if defined(CONFIG_BT_MESH_RELAY_SRC_DBG)
                net_buf_trace.buf = buf;
                memcpy(net_buf_trace.addr, info->addr->a.val, 6);
                net_buf_trace.addr[6] = info->addr->type;
#endif

                if(adv_type == BT_LE_ADV_EXT_ADV_COMPLETE) {
                    bear_trans =  get_net_if(info->secondary_phy);
                    if(bear_trans == INVALID_NET_IF) {
                        BT_ERR("Invalid bear trans");
                    }
                } else {
                    bear_trans = BT_MESH_NET_IF_ADV;
                }
                bt_mesh_net_recv(buf, rssi, bear_trans);
                break;

#if defined(CONFIG_BT_MESH_PB_ADV)
            case BT_DATA_MESH_PROV:
#ifdef CONFIG_BT_MESH_PROVISIONER
                if (bt_mesh_is_provisioner_en()) {
                    provisioner_pb_adv_recv(buf);
                    break;
                } else
#endif
                {
                    bt_mesh_pb_adv_recv(buf);
                }

                break;
#endif
            case BT_DATA_MESH_BEACON:
#ifdef CONFIG_BT_MESH_PROVISIONER
                if (bt_mesh_is_provisioner_en()) {
                    provisioner_beacon_recv(buf);
                } else
#endif
                {
                    bt_mesh_beacon_recv(buf);
                }
                break;

            case BT_DATA_MANUFACTURER_DATA:
                if (g_vendor_beacon_cb != NULL) {
                    g_vendor_beacon_cb(info->addr,rssi,adv_type,(void *)buf, buf->len);
                }

                break;
            default:
                break;
            }
        } else if (adv_type == BT_LE_ADV_IND) {
#ifdef GENIE_ULTRA_PROV
            genie_provision_ultra_prov_handle(type, (void *)buf);
#endif
            switch (type) {
#if defined (CONFIG_BT_MESH_PROVISIONER) && defined (CONFIG_BT_MESH_PB_GATT)
            case BT_DATA_FLAGS:
                if (bt_mesh_is_provisioner_en()) {
                    if (!provisioner_flags_match(buf)) {
                        BT_DBG("Flags mismatch, ignore this adv pkt");
                        return -EINVAL;
                    }
                }
                break;
            case BT_DATA_UUID16_ALL:
                if (bt_mesh_is_provisioner_en()) {
                    uuid = provisioner_srv_uuid_recv(buf);
                    if (!uuid) {
                        BT_DBG("Service UUID mismatch, ignore this adv pkt");
                        return 0;
                    }
                }
                break;
            case BT_DATA_SVC_DATA16:
                if (bt_mesh_is_provisioner_en()) {
                    provisioner_srv_data_recv(buf, addr, uuid);
                }
                break;
#endif
            default:
                break;
            }
            (void)uuid;
        }

        net_buf_simple_restore(buf, &state);
        net_buf_simple_pull(buf, len);
    }
    return 0;
}

void bt_mesh_ext_scan_cb(const struct bt_le_scan_recv_info *info, struct net_buf *buf)
{
#ifdef CONFIG_BT_MESH_PROVISIONER
    const bt_addr_le_t *addr = info->addr;
#endif

    u8_t adv_type = info->adv_type;

    u8_t gather_index = 0;
    struct net_buf* buf_head = NULL;

#if defined (CONFIG_BT_MESH_PROVISIONER) || defined (GENIE_ULTRA_PROV) || defined (CONFIG_OTA_SERVER)
    if (adv_type != BT_LE_ADV_NONCONN_IND && adv_type != BT_LE_ADV_IND	&& adv_type != BT_LE_ADV_EXT_ADV_COMPLETE  &&\
        adv_type != BT_LE_ADV_EXT_ADV_INCOMPLETE) {
#else
    if (adv_type != BT_LE_ADV_NONCONN_IND && adv_type != BT_LE_ADV_EXT_ADV_COMPLETE  && \
        adv_type != BT_LE_ADV_EXT_ADV_INCOMPLETE) {
#endif
        return;
    }


#ifdef CONFIG_BT_MESH_PROVISIONER
    dev_addr = addr;
#endif

    if(adv_type == BT_LE_ADV_EXT_ADV_INCOMPLETE || adv_type == BT_LE_ADV_EXT_ADV_COMPLETE) {

        buf_head = ext_adv_buf_data_gather(info,buf,&gather_index);
        if(!buf_head) {
            goto exist;
        } else {

        }
    } else {
        buf_head = buf;
        gather_index = HEAD_BUF_INDEX_NOT_FOUND;
    }

    ext_adv_buf_data_process(info,buf_head);

    if(gather_index != HEAD_BUF_INDEX_NOT_FOUND) {
        memset(&g_buf_frag_head[gather_index],0x00,sizeof(adv_buf_frag_head));
    }
exist:
    return;
}

#endif

void bt_mesh_adv_init(void)
{

	k_fifo_init(&adv_queue);

#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE
    bt_mesh_fsm_init();
#endif

	NET_BUF_POOL_INIT(adv_buf_pool);
#if defined(CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT) && CONFIG_BT_MESH_RELAY_ADV_BUF_COUNT > 0
    NET_BUF_POOL_INIT(relay_adv_buf_pool);
#endif

#if !(defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE)

#if (CONFIG_BT_ADV_SCAN_SCHEDULE_IN_HOST > 0)
	bt_mesh_adv_scan_schd_init();
#endif
	k_thread_spawn(&adv_thread_data, "mesh adv", (uint32_t *)adv_thread_stack, K_THREAD_STACK_SIZEOF(adv_thread_stack),\
		 adv_thread, NULL, 7);
#endif

#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
	bt_mesh_ext_adv_init();
#endif
}

#define CONN_ADV_DATA_TIEMOUT   (2)
#define NOCONN_ADV_DATA_TIEMOUT (4)

#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
#define NOCONN_EXT_ADV_DATA_1M_TIMEOUT (10)
#define NOCONN_EXT_ADV_DATA_2M_TIMEOUT (10)
#define NOCONN_EXT_ADV_DATA_CODED_TIMEOUT (15)
#endif

static inline int get_adv_time(uint32_t adv_options)
{
#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
	if(adv_options & BT_LE_ADV_OPT_EXT_ADV) {
		if(adv_options & BT_LE_ADV_OPT_NO_2M) {
			return NOCONN_EXT_ADV_DATA_1M_TIMEOUT;
		} else if(adv_options & BT_LE_ADV_OPT_CODED) {
            return NOCONN_EXT_ADV_DATA_CODED_TIMEOUT;
		} else {
            return NOCONN_EXT_ADV_DATA_2M_TIMEOUT;
		}
     } else {
		if(adv_options & BT_LE_ADV_OPT_CONNECTABLE) {
            return CONN_ADV_DATA_TIEMOUT;
	    } else {
            return NOCONN_ADV_DATA_TIEMOUT;
	    }
     }
#else
	if(adv_options & BT_LE_ADV_OPT_CONNECTABLE) {
		return CONN_ADV_DATA_TIEMOUT;
	} else {
		return NOCONN_ADV_DATA_TIEMOUT;
	}
#endif
}

#if (CONFIG_BT_ADV_SCAN_SCHEDULE_IN_HOST > 0)

extern int bt_le_adv_start_instant(const struct bt_le_adv_param *param,
		      uint8_t *ad_data, size_t ad_len,
		      uint8_t *sd_data, size_t sd_len);
extern int bt_le_adv_stop_instant(void);

#define SCHD_LOGD(fmt, ...) //printf(fmt, ##__VA_ARGS__)
#define SCHD_LOGE(...) LOGE("ADV", ##__VA_ARGS__)

typedef enum {
    SCHD_IDLE = 0,
    SCHD_ADV,
    SCHD_SCAN,
    SCHD_ADV_SCAN,

    SCHD_INVAILD,
} adv_scan_schd_state_en;

typedef enum {
    ADV_ON = 0,
    ADV_OFF,
    SCAN_ON,
    SCAN_OFF,

    ACTION_INVAILD,
} adv_scan_schd_action_en;

typedef int (*adv_scan_schd_func_t)(adv_scan_schd_state_en st);
static int adv_scan_schd_idle_enter(adv_scan_schd_state_en st);
static int adv_scan_schd_idle_exit(adv_scan_schd_state_en st);
static int adv_scan_schd_adv_enter(adv_scan_schd_state_en st);
static int adv_scan_schd_adv_exit(adv_scan_schd_state_en st);
static int adv_scan_schd_scan_enter(adv_scan_schd_state_en st);
static int adv_scan_schd_scan_exit(adv_scan_schd_state_en st);
static int adv_scan_schd_adv_scan_enter(adv_scan_schd_state_en st);
static int adv_scan_schd_adv_scan_exit(adv_scan_schd_state_en st);

struct {
    adv_scan_schd_func_t enter;
    adv_scan_schd_func_t exit;
} adv_scan_schd_funcs[] = {
    {adv_scan_schd_idle_enter, adv_scan_schd_idle_exit},
    {adv_scan_schd_adv_enter, adv_scan_schd_adv_exit},
    {adv_scan_schd_scan_enter, adv_scan_schd_scan_exit},
    {adv_scan_schd_adv_scan_enter, adv_scan_schd_adv_scan_exit},
};

adv_scan_schd_state_en adv_scan_schd_st_change_map[4][4] = {
    {SCHD_ADV, SCHD_IDLE, SCHD_SCAN, SCHD_IDLE},
    {SCHD_ADV, SCHD_IDLE, SCHD_ADV_SCAN, SCHD_ADV},
    {SCHD_ADV_SCAN, SCHD_SCAN, SCHD_SCAN, SCHD_IDLE},
    {SCHD_ADV_SCAN, SCHD_SCAN, SCHD_ADV_SCAN, SCHD_ADV},
};

#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
  #define MAX_AD_LEN 255
#else
  #define MAX_AD_LEN 31
#endif

struct adv_scan_data_t {
    struct bt_data *ad;
    uint8_t ad_len;
    struct bt_data *sd;
    uint8_t sd_len;
    struct bt_le_adv_param adv_param;
    struct bt_le_scan_param scan_param;
    bt_le_scan_cb_t *scan_cb;
};

#define FLAG_RESTART 1
#define FLAG_STOP    2

struct {
    struct k_mutex mutex;
    k_timer_t timer;
    uint8_t flag;
    adv_scan_schd_state_en cur_st;
    struct adv_scan_data_t param;
} adv_scan_schd = {0};

static int adv_scan_schd_idle_enter(adv_scan_schd_state_en st)
{
    SCHD_LOGD("idle enter\n");
    memset(&adv_scan_schd.param, 0, sizeof(struct adv_scan_data_t));
    return 0;
}

static int adv_scan_schd_idle_exit(adv_scan_schd_state_en st)
{
    SCHD_LOGD("idle exit\n");
    // do nothing
    return 0;
}

static int adv_scan_schd_adv_enter(adv_scan_schd_state_en st)
{
    SCHD_LOGD("adv on enter\n");

    if (st == SCHD_IDLE || st == SCHD_ADV_SCAN || st == SCHD_ADV) {
        if (adv_scan_schd.param.ad_len) {
            adv_scan_schd.flag = FLAG_RESTART;
            k_timer_start(&adv_scan_schd.timer, 1);
            return 0;
        }
    }

    return -EINVAL;;
}

static int adv_scan_schd_adv_exit(adv_scan_schd_state_en st)
{
    SCHD_LOGD("adv on exit\n");
    int ret = 0;
    if (st == SCHD_ADV_SCAN || st == SCHD_IDLE || st == SCHD_ADV) {
        adv_scan_schd.flag = FLAG_STOP;
        k_timer_stop(&adv_scan_schd.timer);
        ret = bt_le_adv_stop_instant();
        if (ret && ret != -EALREADY) {
            SCHD_LOGE("adv stop err %d\n", ret);
            return ret;
        }
        return 0;
    }

    return -EINVAL;
}

static int adv_scan_schd_scan_enter(adv_scan_schd_state_en st)
{
    SCHD_LOGD("scan on enter\n");
    int ret = 0;
    if (st == SCHD_IDLE || st == SCHD_ADV_SCAN || st == SCHD_SCAN) {
        ret = bt_le_scan_start(&adv_scan_schd.param.scan_param, adv_scan_schd.param.scan_cb);
        if (ret && ret != -EALREADY) {
            SCHD_LOGE("scan start err %d\n", ret);
            return ret;
        }
        return 0;
    }
    return -EINVAL;
}

static int adv_scan_schd_scan_exit(adv_scan_schd_state_en st)
{
    SCHD_LOGD("scan on exit\n");
    int ret = 0;
    if (st == SCHD_ADV_SCAN || st == SCHD_IDLE || st == SCHD_SCAN) {
         ret = bt_le_scan_stop();
         if(ret && ret != -EALREADY) {
            SCHD_LOGE("scan stop err %d\n", ret);
            return ret;
         }
         return 0;
    }

    return -EINVAL;
}

static int adv_scan_schd_adv_scan_enter(adv_scan_schd_state_en st)
{
    SCHD_LOGD("adv scan on enter\n");

    if (st == SCHD_ADV || st == SCHD_SCAN || st == SCHD_ADV_SCAN || st == SCHD_IDLE) {
        adv_scan_schd.flag = FLAG_RESTART;
        k_timer_start(&adv_scan_schd.timer, 1);
        return 0;
    }

    return -EINVAL;
}

static int adv_scan_schd_adv_scan_exit(adv_scan_schd_state_en st)
{
    int ret;
    SCHD_LOGD("adv scan on exit\n");

    if (st == SCHD_ADV || st == SCHD_SCAN || st == SCHD_ADV_SCAN) {
        adv_scan_schd.flag = FLAG_STOP;
        k_timer_stop(&adv_scan_schd.timer);

        ret = bt_le_scan_stop();

        if (ret && ret != -EALREADY) {
            SCHD_LOGE("scan stop err %d\n", ret);
            return ret;
        }

        ret = bt_le_adv_stop();

        if (ret && ret != -EALREADY) {
            SCHD_LOGE("adv stop err %d\n", ret);
            return ret;
        }

        return 0;
    }

    return -EINVAL;
}

int bt_mesh_adv_scan_schd(adv_scan_schd_state_en st)
{
    int ret;
    SCHD_LOGD("%d->%d\n", adv_scan_schd.cur_st, st);

    if (st < SCHD_INVAILD) {
        ret = adv_scan_schd_funcs[adv_scan_schd.cur_st].exit(st);

        if (ret) {
            return ret;
        }

        adv_scan_schd.cur_st = SCHD_IDLE;

        ret = adv_scan_schd_funcs[st].enter(adv_scan_schd.cur_st);

        if (ret) {
            return ret;
        }

        adv_scan_schd.cur_st = st;

        return 0;
    }

    return -EINVAL;
}

int bt_mesh_adv_scan_schd_action(adv_scan_schd_action_en action)
{
    int ret;

    if (action < ACTION_INVAILD) {
        adv_scan_schd_state_en cur_st = adv_scan_schd.cur_st;
        adv_scan_schd_state_en target_st = adv_scan_schd_st_change_map[cur_st][action];
        k_mutex_lock(&adv_scan_schd.mutex, K_FOREVER);
        ret = bt_mesh_adv_scan_schd(target_st);
        k_mutex_unlock(&adv_scan_schd.mutex);
        if (ret && ret != -EALREADY) {
            SCHD_LOGE("action %d, cur_st %d target_st %d, ret %d\n", action, cur_st, target_st, ret);
        }

        return ret;
    }

    return -EINVAL;
}

void adv_scan_timer(void *timer, void *arg)
{
    int ret;
    static enum  {
        ADV = 0,
        SCAN,
        ADV_IDLE,
    } next_state = ADV;
    static int adv_time = 0;
	static int negative_flag = 0;
    uint8_t random_delay = 0;

    uint32_t next_time = 0;
    k_mutex_lock(&adv_scan_schd.mutex, K_FOREVER);

    if (adv_scan_schd.flag == FLAG_RESTART) {
        next_state = ADV;
        adv_scan_schd.flag = 0;
    } else if (adv_scan_schd.flag == FLAG_STOP)
    {
        k_mutex_unlock(&adv_scan_schd.mutex);
        return;
    }

    SCHD_LOGD("adv_time %d, next_state %d, flag %d\n", adv_time, next_state, adv_scan_schd.flag);

    if (next_state == ADV) {
        ret = bt_le_scan_stop();

        if (ret && ret != -EALREADY) {
            SCHD_LOGE("scan stop err %d\n", ret);
        }

#if 0
        //struct bt_le_adv_param param = adv_scan_schd.param.adv_param;

        param.interval_min = BT_GAP_ADV_SLOW_INT_MIN;

        param.interval_max = param.interval_min;
#endif

        ret = bt_le_adv_start(&adv_scan_schd.param.adv_param,
                                      adv_scan_schd.param.ad, adv_scan_schd.param.ad_len,
                                      adv_scan_schd.param.sd, adv_scan_schd.param.sd_len);

        if (ret && ret != -EALREADY && ret != -ENOMEM) {
            SCHD_LOGE("adv start err %d\n", ret);
        }

        if (adv_scan_schd.cur_st == SCHD_ADV_SCAN)
        {
            next_state = SCAN;
        } else if (adv_scan_schd.cur_st == SCHD_ADV)
        {
            next_state = ADV_IDLE;
        }

        adv_time = get_adv_time(adv_scan_schd.param.adv_param.options);
        next_time = adv_time;
    } else if (next_state == SCAN) {
		//io_test_set(PB14,0);
        ret = bt_le_adv_stop();

        if (ret && ret != -EALREADY) {
            SCHD_LOGE("adv stop err %d\n", ret);
        }

         /* Here, we define the adv window of each package in adv duration (120ms or xmit related time)*/
        if (adv_scan_schd.param.adv_param.options & BT_LE_ADV_OPT_CONNECTABLE) {
            next_time = adv_scan_schd.param.adv_param.interval_min * 5 / 8 - adv_time;
        } else {
			//next_time = CONFIG_ADV_INTERVAL_TIMER - adv_time; //adv_scan_schd.param.adv_param.interval_min * 5 / 8 - adv_time;
			bt_rand(&random_delay, 1);

			random_delay = random_delay % 10;
			if(random_delay <= 5) {
				random_delay = 5;
			}

	        if(negative_flag) {
	            next_time = CONFIG_ADV_INTERVAL_TIMER - NOCONN_ADV_DATA_TIEMOUT - random_delay;
			}else {
	            next_time = CONFIG_ADV_INTERVAL_TIMER - NOCONN_ADV_DATA_TIEMOUT + random_delay;
			}

				negative_flag = !negative_flag;
	    }

        if (next_time > CONFIG_ADV_SCAN_INTERVAL_TIMER) {
            ret = bt_le_scan_start(&adv_scan_schd.param.scan_param, adv_scan_schd.param.scan_cb);
            if (ret) {
                SCHD_LOGE("scan err %d\n", ret);
            }
        }
        adv_time = 0;
        next_state = ADV;

    } else if (next_state == ADV_IDLE) {
		ret = bt_le_adv_stop();
        if (ret && ret != -EALREADY) {
            SCHD_LOGE("adv stop err %d\n", ret);
        }

        if (adv_scan_schd.param.adv_param.options & BT_LE_ADV_OPT_CONNECTABLE) {
            next_time = adv_scan_schd.param.adv_param.interval_min * 5 / 8 - adv_time;
        } else {
            next_time = CONFIG_ADV_INTERVAL_TIMER - adv_time; //adv_scan_schd.param.adv_param.interval_min * 5 / 8 - adv_time;
        }
        adv_time = 0;
        next_state = ADV;
    }
    k_mutex_unlock(&adv_scan_schd.mutex);
    k_timer_start(&adv_scan_schd.timer, next_time);
}

int bt_mesh_adv_scan_schd_init()
{
    memset(&adv_scan_schd, 0, sizeof(adv_scan_schd));
    k_timer_init(&adv_scan_schd.timer, adv_scan_timer,  &adv_scan_schd);
    k_mutex_init(&adv_scan_schd.mutex);
    return 0;
}

static int set_ad_data(uint8_t *data, const struct bt_data *ad, size_t ad_len)
{
    int i;
    int set_len = 0;

    for (i = 0; i < ad_len; i++) {
        int len = ad[i].data_len;
        u8_t type = ad[i].type;

        /* Check if ad fit in the remaining buffer */
        if (set_len + len + 2 > MAX_AD_LEN) {
            len = MAX_AD_LEN - (set_len + 2);

            if (type != BT_DATA_NAME_COMPLETE || len <= 0) {
                return -EINVAL;
            }

            type = BT_DATA_NAME_SHORTENED;
        }

        data[set_len++] = len + 1;
        data[set_len++] = type;

        memcpy(&data[set_len], ad[i].data, len);
        set_len += len;
    }

    return set_len;
}
int bt_mesh_adv_enable(const struct bt_le_adv_param *param,
                       const struct bt_data *ad, size_t ad_len,
                       const struct bt_data *sd, size_t sd_len)
{
    if (param == NULL || (ad && !ad_len) || (sd && !sd_len)) {
        return -EINVAL;
    }

    BT_DBG("");
    k_mutex_lock(&adv_scan_schd.mutex, K_FOREVER);
    adv_scan_schd.param.adv_param = *param;
	adv_scan_schd.param.ad     = ad;
    adv_scan_schd.param.ad_len = ad_len;
	adv_scan_schd.param.sd     = sd;
    adv_scan_schd.param.sd_len = sd_len;

    k_mutex_unlock(&adv_scan_schd.mutex);
    bt_mesh_adv_scan_schd_action(ADV_ON);
    return 0;
}

int bt_mesh_adv_disable()
{
    BT_DBG("");
    bt_mesh_adv_scan_schd_action(ADV_OFF);
    return 0;
}

int bt_mesh_scan_enable(void)
{
    struct bt_le_scan_param scan_param = {
        .type      = BT_HCI_LE_SCAN_PASSIVE,
        .filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
        .interval   = MESH_SCAN_INTERVAL,
        .window     = MESH_SCAN_WINDOW
    };

    BT_DBG("");
    k_mutex_lock(&adv_scan_schd.mutex, K_FOREVER);
    adv_scan_schd.param.scan_param = scan_param;
#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
    adv_scan_schd.param.scan_cb = NULL;
#else
    adv_scan_schd.param.scan_cb = bt_mesh_scan_cb;
#endif
    k_mutex_unlock(&adv_scan_schd.mutex);
    bt_mesh_adv_scan_schd_action(SCAN_ON);
    return 0;
}

int bt_mesh_scan_disable(void)
{
    BT_DBG("");
    bt_mesh_adv_scan_schd_action(SCAN_OFF);
    return 0;
}

#elif defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE

enum {
    MESH_FSM_ST_NONE = 0,
    MESH_FSM_ST_ADV_START,
    MESH_FSM_ST_ADV_STARTED,
    MESH_FSM_ST_ADV_STOP,
    MESH_FSM_ST_ADV_STOPED,
    MESH_FSM_ST_SCAN_START,
    MESH_FSM_ST_SCAN_STARTED,
    MESH_FSM_ST_SCAN_STOP,
    MESH_FSM_ST_SCAN_STOPED,

    MESH_FSM_ST_INVAILD,
};

struct bt_mesh_fsm_t {
    bt_fsm_handle_t handle;
    u8_t cur_state: 4;
    u8_t next_state: 4;
    u8_t scan_background: 1;

    union {
        u16_t scan_interval;
        u16_t adv_interval;
        u16_t proxy_adv_interval;
    };

    u8_t adv_xmit;
    u32_t adv_options;
    struct net_buf *adv_buf;
    struct k_delayed_work work;
} bt_mesh_fsm = {0};

enum {
    ADV_SEND_DONE,
    ADV_SEND_CONTINUE,
};

static int set_ad_data(uint8_t *data, const struct bt_data *ad, size_t ad_len)
{
    int i;
    int set_len = 0;

    for (i = 0; i < ad_len; i++) {
        int len = ad[i].data_len;
        u8_t type = ad[i].type;

        /* Check if ad fit in the remaining buffer */
        if (set_len + len + 2 > 31) {
            len = 31 - (set_len + 2);

            if (type != BT_DATA_NAME_COMPLETE || len <= 0) {
                return -EINVAL;
            }

            type = BT_DATA_NAME_SHORTENED;
        }

        data[set_len++] = len + 1;
        data[set_len++] = type;

        memcpy(&data[set_len], ad[i].data, len);
        set_len += len;
    }

    return set_len;
}

int bt_mesh_adv_enable(const struct bt_le_adv_param *param,
                       const struct bt_data *ad, size_t ad_len,
                       const struct bt_data *sd, size_t sd_len)
{
    int err;
    if (param == NULL) {
        return -EINVAL;
    }

    BT_DBG("");

	err = bt_fsm_le_adv_start(param, ad, ad_len, sd, sd_len, NULL, NULL, bt_mesh_fsm.handle);
	if (err) {
		return err;
	}

    bt_mesh_fsm.adv_interval = ADV_SCAN_MS(param->interval_min);

    bt_mesh_fsm.adv_options = param->options;

    bt_mesh_fsm.cur_state = MESH_FSM_ST_ADV_START;

    bt_fsm_wait_event(bt_mesh_fsm.handle, BT_FSM_EV_DEFINE(HCI_CORE, ADV_STARTED));

    return 0;
}

int bt_mesh_adv_disable()
{
    int err;
    err = bt_mesh_fsm_adv_stop();

	if (err) {
		return err;
	}

    return 0;
}

int bt_mesh_scan_enable(void)
{
    BT_DBG("");

    return  bt_mesh_fsm_scan_start();
}

int bt_mesh_scan_disable(void)
{
    BT_DBG("");
    return  bt_mesh_fsm_scan_stop();
}

#else

int bt_mesh_adv_enable(const struct bt_le_adv_param *param,
                       const struct bt_data *ad, size_t ad_len,
                       const struct bt_data *sd, size_t sd_len)
{
    if (param == NULL) {
        return -EINVAL;
    }
    BT_DBG("");
    return bt_le_adv_start(param, ad, ad_len, sd, sd_len);
}

int bt_mesh_adv_disable()
{
    BT_DBG("");
    return bt_le_adv_stop();
}

int bt_mesh_scan_enable(void)
{
    struct bt_le_scan_param scan_param = {
        .type      = BT_HCI_LE_SCAN_PASSIVE,
        .filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
        .interval   = MESH_SCAN_INTERVAL,
        .window     = MESH_SCAN_WINDOW
    };
#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
   return bt_le_scan_start(&scan_param, NULL);
#else
   return bt_le_scan_start(&scan_param, bt_mesh_scan_cb);
#endif
}

int bt_mesh_scan_disable(void)
{
    return bt_le_scan_stop();
}

#endif

int bt_mesh_adv_vnd_scan_register(vendor_beacon_cb bacon_cb)
{
    if(!bacon_cb){
      return -EINVAL;
    }
    g_vendor_beacon_cb = bacon_cb;
    return 0;
}

#if defined(CONFIG_BT_HOST_OPTIMIZE) && CONFIG_BT_HOST_OPTIMIZE

static int bt_fsm_action_bt_mesh_adv_stoped(void *msg);

static int bt_fsm_action_bt_mesh_scan_stoped(void *msg);

extern bool bt_mesh_proxy_adv_enable();

int bt_mesh_fsm_proxy_adv_send();

static inline int _bt_fsm_adv_send(struct net_buf *buf)
{
    extern int bt_le_hci_version_get();
    const s32_t adv_int_min = ((bt_le_hci_version_get() >= BT_HCI_VERSION_5_0) ?
				   ADV_INT_FAST_MS : ADV_INT_DEFAULT_MS);
	struct bt_le_adv_param param = {0};
	u16_t adv_int;
	struct bt_data ad[CONFIG_MAX_AD_NUM];
	uint8_t ad_index = 0;
    struct net_buf *temp_buf = NULL;

#if  defined(CONFIG_MESH_LPM) && defined(CONFIG_BT_MESH_PROVISIONER)
      if(BT_MESH_ADV(buf)->lpm_flag == 1) {
         BT_MESH_ADV(buf)->xmit = BT_MESH_TRANSMIT(7, 40);/* 8 transmissions with 40ms interval for lpm node*/
		 adv_int = 150;/* 8* 150 = 1.2s send data in 1.2s*/
	  } else {
	     adv_int = MAX(adv_int_min,
					BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit));
	  }
#else
	 adv_int = MAX(adv_int_min,
		      BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit));
#endif

#ifdef CONFIG_GENIE_MESH_ENABLE
	//BT_MESH_ADV(buf)->xmit = BT_MESH_TRANSMIT(6, 20);
#endif

	//BT_DBG("type %u len %u: %s", BT_MESH_ADV(buf)->type,
	       //buf->len, bt_hex(buf->data, buf->len));
    //BT_DBG("count %u interval %ums duration %ums",
	//       BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1, adv_int);



	temp_buf = buf;

	while(temp_buf) {
#ifdef GENIE_ULTRA_PROV
        if (BT_MESH_ADV(buf)->tiny_adv == 1) {
            ad[ad_index].type = GENIE_PROV_ADV_TYPE;
        } else {
            ad[ad_index].type = adv_type[BT_MESH_ADV(temp_buf)->type];
        }
#else
	    ad[ad_index].type = adv_type[BT_MESH_ADV(temp_buf)->type];
#endif
	    ad[ad_index].data_len = temp_buf->len;
	    ad[ad_index].data = temp_buf->data;
#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
		if(!ad_index) {
			param.sid    =  BT_MESH_ADV(temp_buf)->sid;
		}
#endif
		ad_index++;
		temp_buf = temp_buf->frags;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_DEBUG_USE_ID_ADDR)) {
		param.options = BT_LE_ADV_OPT_USE_IDENTITY;
	} else {
		param.options = 0;
	}

	param.id = BT_ID_DEFAULT;
	param.interval_min = ADV_SCAN_UNIT(adv_int);
	param.interval_max = param.interval_min;

#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
	if(BT_MESH_ADV(buf)->trans == NET_TRANS_EXT_ADV_1M) {
        param.options |= BT_LE_ADV_OPT_EXT_ADV;
	    param.options |= BT_LE_ADV_OPT_NO_2M;
	} else if(BT_MESH_ADV(buf)->trans == NET_TRANS_EXT_ADV_2M) {
        param.options |= BT_LE_ADV_OPT_EXT_ADV;
	} else if (BT_MESH_ADV(buf)->trans == NET_TRANS_EXT_ADV_CODED) {
        param.options |= BT_LE_ADV_OPT_EXT_ADV;
	    param.options |= BT_LE_ADV_OPT_CODED;
	}
#endif

	return bt_mesh_adv_enable(&param, ad, ad_index, NULL, 0);
}

static inline struct net_buf* check_and_get_adv_buf(struct net_buf* buf)
{
    if(!buf) {
        return NULL;
    }

    struct net_buf* temp_buf = buf;
    do {
        if(BT_MESH_ADV(temp_buf)->type == BT_MESH_ADV_BEACON &&  \
           bt_mesh_is_provisioned() &&  temp_buf->data[0] == BEACON_TYPE_UNPROVISIONED) {
            net_buf_unref(temp_buf);
            BT_WARN("Ignore the unprov beacon data when provisioned");
            temp_buf = net_buf_get(&adv_queue, K_NO_WAIT);
        } else {
            return temp_buf;
        }
    } while(temp_buf);

    return temp_buf;
}

static inline bool _bt_fsm_st_adv_send_enter(void *msg)
{
    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND)))
    {
        return true;
    }

    if (!bt_mesh_fsm.adv_buf)
    {
        bt_mesh_fsm.adv_buf = net_buf_get(&adv_queue, K_NO_WAIT);
    }

    if (bt_mesh_fsm.adv_buf)
    {
        bt_mesh_fsm.adv_buf = check_and_get_adv_buf(bt_mesh_fsm.adv_buf);
		if(!bt_mesh_fsm.adv_buf) {
           return false;
		}

        int err = _bt_fsm_adv_send(bt_mesh_fsm.adv_buf);
        if (err)
        {
            BT_ERR("adv send error %d\n", err);
            return false;
        }

        if (bt_mesh_fsm.adv_xmit == 0)
        {
            /* for mesh adv xmit 0 means 1 count, here add 1 for convenient */
            bt_mesh_fsm.adv_xmit = BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(bt_mesh_fsm.adv_buf)->xmit) + 1;
        }

        bt_mesh_fsm.cur_state = MESH_FSM_ST_ADV_START;

        /* all adv buff will process next, flush BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND) here */
        bt_fsm_flush_event(bt_mesh_fsm.handle, BT_FSM_EV_DEFINE(BT_MESH, ADV_SEND));
        bt_fsm_set_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND));

        return true;
    }

    return false;
}

static inline bool _bt_fsm_st_adv_send_exit(void *msg)
{
    if (!bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND)))
    {
        return true;
    }

    switch (bt_mesh_fsm.cur_state)
    {
        case MESH_FSM_ST_ADV_START:
        case MESH_FSM_ST_ADV_STARTED:
        {
            k_delayed_work_cancel(&bt_mesh_fsm.work);
            int err = bt_mesh_fsm_adv_stop();
            if (err == -EALREADY)
            {
                BT_ERR("adv stop error %d\n", err);
                break;
            }

            return false;
        }
        case MESH_FSM_ST_ADV_STOP:
            return false;
        case MESH_FSM_ST_ADV_STOPED:
            break;
        default:
            break;
    }

    bt_fsm_clear_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND));

    return true;
}

static inline bool _bt_fsm_st_scan_enter(void *msg)
{
    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, SCAN)))
    {
        return true;
    }

    struct bt_le_scan_param scan_param = {
        .type      = BT_HCI_LE_SCAN_PASSIVE,
        .filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
        .interval   = MESH_SCAN_INTERVAL,
        .window     = MESH_SCAN_WINDOW
    };

#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
    int err = bt_fsm_le_scan_start(&scan_param, NULL, NULL, NULL, bt_mesh_fsm.handle);
#else
    int err = bt_fsm_le_scan_start(&scan_param, bt_mesh_scan_cb, NULL, NULL, bt_mesh_fsm.handle);
#endif
    if (err && err != -EALREADY)
    {
        BT_ERR("scan start error %d\n", err);
        return false;
    }

    bt_fsm_wait_event(bt_mesh_fsm.handle, BT_FSM_EV_DEFINE(HCI_CORE, SCAN_STARTED));

    bt_mesh_fsm.cur_state = MESH_FSM_ST_SCAN_START;

    bt_fsm_set_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, SCAN));

    return true;
}

static inline bool _bt_fsm_st_scan_exit(void *msg)
{
    if (!bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, SCAN)))
    {
        return true;
    }

    switch (bt_mesh_fsm.cur_state)
    {
        case MESH_FSM_ST_SCAN_START:
        case MESH_FSM_ST_SCAN_STARTED:
        {
            k_delayed_work_cancel(&bt_mesh_fsm.work);
            bt_mesh_fsm.cur_state = MESH_FSM_ST_SCAN_STOP;
            int err = bt_fsm_le_scan_stop(NULL, NULL, bt_mesh_fsm.handle);
            if (err)
            {
                BT_ERR("scan stop error %d\n", err);
                return false;
            }
            return false;
        }
        case MESH_FSM_ST_SCAN_STOP:
            return false;
        case MESH_FSM_ST_SCAN_STOPED:
            break;
        default:
            break;
    }

    bt_fsm_clear_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, SCAN));

    return true;
}
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
static inline bool _bt_fsm_st_proxy_adv_send_enter(void *msg)
{
    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND)))
    {
        return true;
    }

    bt_mesh_proxy_adv_start();

    if (bt_mesh_proxy_adv_enable())
    {
        bt_mesh_fsm.cur_state = MESH_FSM_ST_ADV_START;
        bt_fsm_set_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND));
        return true;
    }

    return false;
}

static inline bool _bt_fsm_st_proxy_adv_send_exit(void *msg)
{
    if (!bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND)))
    {
        return true;
    }

    switch (bt_mesh_fsm.cur_state)
    {
        case MESH_FSM_ST_ADV_START:
        case MESH_FSM_ST_ADV_STARTED:
        {
            k_delayed_work_cancel(&bt_mesh_fsm.work);
            bt_mesh_fsm.cur_state = MESH_FSM_ST_ADV_STOP;
            int err = bt_mesh_fsm_adv_stop();
            if (err == -EALREADY)
            {
                break;
            }

            return false;
        }
        case MESH_FSM_ST_ADV_STOP:
        case MESH_FSM_ST_ADV_STOPED:
            break;
        default:
            break;
    }

    bt_fsm_clear_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND));

    return true;
}
#endif
static void inline _bt_fsm_mesh_work(struct k_work *work)
{
    if (bt_mesh_fsm.next_state == MESH_FSM_ST_NONE)
    {
        return;
    }

    if (bt_mesh_fsm.next_state == MESH_FSM_ST_ADV_START)
    {
        if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND)))
        {
            if (bt_mesh_fsm.adv_buf)
            {
                _bt_fsm_adv_send(bt_mesh_fsm.adv_buf);
            }
        }
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
        else if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND)))
        {
            bt_mesh_proxy_adv_start();

            if (bt_mesh_proxy_adv_enable())
            {
                bt_mesh_fsm.cur_state = MESH_FSM_ST_ADV_START;
            }
        }
#endif
    }
    else if (bt_mesh_fsm.next_state == MESH_FSM_ST_ADV_STOP)
    {
        if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND)))
        {
            int err = bt_mesh_fsm_adv_stop();
            if (err == -EALREADY)
            {
                bt_mesh_fsm.cur_state = MESH_FSM_ST_ADV_STOPED;
            }
            else if (err)
            {
                BT_ERR("adv stop fail %d", err);
            }
        }
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
        else if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND)))
        {
            int err = bt_mesh_fsm_adv_stop();
            if (err)
            {
                BT_ERR("adv stop fail %d", err);
            }
            else
            {
                bt_fsm_wait_event(bt_mesh_fsm.handle, BT_FSM_EV_DEFINE(HCI_CORE, ADV_STOPED));
            }
        }
#endif
    }
    else if (bt_mesh_fsm.next_state == MESH_FSM_ST_SCAN_STOP)
    {
            int err = bt_fsm_le_scan_stop(NULL, NULL, bt_mesh_fsm.handle);
            if (err)
            {
                BT_ERR("scan stop fail %d", err);
            }
            else
            {
                bt_fsm_wait_event(bt_mesh_fsm.handle, BT_FSM_EV_DEFINE(HCI_CORE, SCAN_STOPED));
            }
    }

    bt_mesh_fsm.next_state = MESH_FSM_ST_NONE;
}

static int bt_fsm_action_bt_mesh_adv_send(void *msg)
{
    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND)) || bt_mesh_fsm.adv_buf)
    {
        return 0;
    }

#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND)))
    {
        _bt_fsm_st_proxy_adv_send_exit(msg);
        return FSM_RET_MSG_RETAIN;
    }
#endif

    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, SCAN)))
    {
        _bt_fsm_st_scan_exit(msg);

        return FSM_RET_MSG_RETAIN;
    }

    if (_bt_fsm_st_adv_send_enter(msg))
    {
        return 0;
    }
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
    else if (IS_ENABLED(CONFIG_BT_MESH_PROXY)) {
        if (!_bt_fsm_st_proxy_adv_send_enter(msg))
        {
            if (bt_mesh_fsm.scan_background)
            {
                _bt_fsm_st_scan_enter(msg);
            }
        }
    }
#endif
    else if (bt_mesh_fsm.scan_background)
    {
        _bt_fsm_st_scan_enter(msg);
    }

    return 0;
}
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
static int bt_fsm_action_bt_mesh_adv_proxy_send(void *msg)
{
    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND)))
    {
        return FSM_RET_MSG_RETAIN;
    }

    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, SCAN)))
    {
        _bt_fsm_st_scan_exit(msg);
        return FSM_RET_MSG_RETAIN;
    }

    if (!_bt_fsm_st_proxy_adv_send_enter(msg))
    {
        if (bt_mesh_fsm.scan_background)
        {
           // _bt_fsm_st_scan_enter(msg);
        }
    }

    return 0;
}
#endif



static int bt_fsm_action_bt_mesh_exit(void *msg)
{
    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND)))
    {
        if (_bt_fsm_st_adv_send_exit(msg))
        {
            return 0;
        }
    }
    else if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, SCAN)))
    {
        if (_bt_fsm_st_scan_exit(msg))
        {
            return 0;
        }
    }
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
    else if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND)))
    {
        if (_bt_fsm_st_proxy_adv_send_exit(msg))
        {
            return 0;
        }
    }
#endif

    return FSM_RET_MSG_RETAIN;
}

static int bt_fsm_action_bt_mesh_adv_started(void *msg)
{
    struct hci_core_fsm_msg_t *hci_core_msg = msg;

    if (!hci_core_msg->status)
    {
        bt_mesh_fsm.cur_state = MESH_FSM_ST_ADV_STARTED;
    }
    else
    {
        bt_mesh_fsm.cur_state = MESH_FSM_ST_ADV_STOPED;
    }

    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND)))
    {
        if (bt_mesh_fsm.cur_state != MESH_FSM_ST_ADV_STARTED)
        {
            /* adv start failed, retart adv send if possiable */
            _bt_fsm_st_adv_send_exit(msg);
            _bt_fsm_st_adv_send_enter(msg);
        }
        else
        {
            BT_DBG("adv started");
            if (bt_mesh_fsm.adv_buf)
            {
                bt_mesh_fsm.next_state = MESH_FSM_ST_ADV_STOP;
                k_delayed_work_submit(&bt_mesh_fsm.work, get_adv_time(bt_mesh_fsm.adv_options));

                /* first adv */
                if (bt_mesh_fsm.adv_xmit == BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(bt_mesh_fsm.adv_buf)->xmit) + 1)
                {
                    const struct bt_mesh_send_cb *cb = BT_MESH_ADV(bt_mesh_fsm.adv_buf)->cb;
                    void *cb_data = BT_MESH_ADV(bt_mesh_fsm.adv_buf)->cb_data;
                    adv_send_start(0, 0, cb, cb_data);
                }
            }
        }
        return 0;
    }
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
    else if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND)))
    {
        if (bt_mesh_fsm.cur_state != MESH_FSM_ST_ADV_STARTED)
        {
            _bt_fsm_st_proxy_adv_send_exit(msg);
        }
        else
        {
            BT_DBG("proxy adv started");
            bt_mesh_fsm.next_state = MESH_FSM_ST_ADV_STOP;
            k_delayed_work_submit(&bt_mesh_fsm.work, get_adv_time(bt_mesh_fsm.adv_options));
        }
        return 0;
    }
#endif
    else if (bt_mesh_fsm.scan_background)
    {
        _bt_fsm_st_scan_enter(msg);
    }
    return 0;
}

static int bt_fsm_action_bt_mesh_adv_stoped(void *msg)
{
    struct hci_core_fsm_msg_t *hci_core_msg = msg;

    if (!hci_core_msg->status)
    {
        bt_mesh_fsm.cur_state = MESH_FSM_ST_ADV_STOPED;
    }

    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND)))
    {
        if (bt_mesh_fsm.cur_state != MESH_FSM_ST_ADV_STOPED)
        {
            /*adv stop failed, try restop */
            _bt_fsm_st_adv_send_exit(msg);
        }
        else
        {
            BT_DBG("adv stoped");
             bt_mesh_fsm.adv_xmit--;
            if (bt_mesh_fsm.adv_xmit == 0)
            {
                const struct bt_mesh_send_cb *cb = BT_MESH_ADV(bt_mesh_fsm.adv_buf)->cb;
                void *cb_data = BT_MESH_ADV(bt_mesh_fsm.adv_buf)->cb_data;

                adv_send_end(0, cb, cb_data);

                if (bt_mesh_fsm.adv_buf)
                {
                    BT_MESH_ADV(bt_mesh_fsm.adv_buf)->busy = 0;
                    net_buf_frag_del_all(bt_mesh_fsm.adv_buf);
                }
                bt_mesh_fsm.adv_buf = NULL;

                _bt_fsm_st_adv_send_exit(msg);

                if (k_fifo_num_get(&adv_queue))
                {
                    _bt_fsm_st_adv_send_enter(msg);
                }
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
                else if (IS_ENABLED(CONFIG_BT_MESH_PROXY)) {
                    if (!_bt_fsm_st_proxy_adv_send_enter(msg))
                    {
                        if (bt_mesh_fsm.scan_background)
                        {
                            _bt_fsm_st_scan_enter(msg);
                        }
                    }
                }
#endif
                else if (bt_mesh_fsm.scan_background)
                {
                    _bt_fsm_st_scan_enter(msg);
                }

                return 0;
            }
            else
            {
                if (bt_mesh_fsm.scan_background)
                {
                    _bt_fsm_st_adv_send_exit(msg);
                    _bt_fsm_st_scan_enter(msg);
                }
                else if (bt_mesh_fsm.adv_xmit)
                {
                    bt_mesh_fsm.next_state = MESH_FSM_ST_ADV_START;
                    uint8_t random_delay = 1;
                    bt_rand(&random_delay, 1);
                    random_delay %= 5;
                    k_delayed_work_submit(&bt_mesh_fsm.work, bt_mesh_fsm.adv_interval - get_adv_time(bt_mesh_fsm.adv_options) + random_delay);
                }
            }
        }
        return 0;
    }
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
    else if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND)))
    {
        BT_DBG("proxy adv stoped");

        if (bt_mesh_fsm.cur_state != MESH_FSM_ST_ADV_STOPED)
        {
            /*adv stop failed, try restop */
            _bt_fsm_st_proxy_adv_send_exit(msg);
            return 0;
        }
        else
        {
            if (k_fifo_num_get(&adv_queue))
            {
                _bt_fsm_st_proxy_adv_send_exit(msg);
                //bt_mesh_fsm_adv_send();
            }
            else if (bt_mesh_fsm.scan_background)
            {
                _bt_fsm_st_proxy_adv_send_exit(msg);
                _bt_fsm_st_scan_enter(msg);
            }
            else
            {
                bt_mesh_fsm.next_state = MESH_FSM_ST_ADV_START;
                uint8_t random_delay = 1;
                bt_rand(&random_delay, 1);
                random_delay %= 5;
                k_delayed_work_submit(&bt_mesh_fsm.work, bt_mesh_fsm.proxy_adv_interval - get_adv_time(bt_mesh_fsm.adv_options) + random_delay);
            }
        }
    }
#endif
    return 0;
}

static int bt_fsm_action_bt_mesh_scan_started(void *msg)
{
    BT_DBG("scan started");

    struct hci_core_fsm_msg_t *hci_core_msg = msg;

    if (!hci_core_msg->status)
    {
        bt_mesh_fsm.cur_state = MESH_FSM_ST_SCAN_STARTED;
    }
    else
    {
        bt_mesh_fsm.cur_state = MESH_FSM_ST_SCAN_STOPED;
    }

    if (bt_mesh_fsm.cur_state != MESH_FSM_ST_SCAN_STARTED)
    {
        /* reenter scan state */
        if (bt_mesh_fsm.scan_background)
        {
            _bt_fsm_st_scan_enter(msg);
        }
    }
    else
    {
        if (bt_mesh_fsm.adv_xmit)
        {
            bt_mesh_fsm.next_state = MESH_FSM_ST_SCAN_STOP;
            k_delayed_work_submit(&bt_mesh_fsm.work, bt_mesh_fsm.adv_interval - get_adv_time(bt_mesh_fsm.adv_options));
        }
        else
        {
    #if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
            if (bt_mesh_proxy_adv_enable())
            {
                bt_mesh_fsm.next_state = MESH_FSM_ST_SCAN_STOP;
                k_delayed_work_submit(&bt_mesh_fsm.work, bt_mesh_fsm.proxy_adv_interval - get_adv_time(bt_mesh_fsm.adv_options));
            }
    #endif
        }
    }

    return 0;
}

static int bt_fsm_action_bt_mesh_scan_stoped(void *msg)
{
    BT_DBG("scan stoped");

    struct hci_core_fsm_msg_t *hci_core_msg = msg;

    if (!hci_core_msg->status)
    {
        bt_mesh_fsm.cur_state = MESH_FSM_ST_SCAN_STOPED;
    }

    _bt_fsm_st_scan_exit(msg);

    if (bt_mesh_fsm.cur_state != MESH_FSM_ST_SCAN_STOPED)
    {
        /* try exit scan state */
        return 0;
    }
    else
    {
        if (bt_mesh_fsm.adv_buf)
        {
            _bt_fsm_st_adv_send_enter(msg);
            return 0;
        }
        else if (k_fifo_num_get(&adv_queue))
        {
            _bt_fsm_st_adv_send_enter(msg);
            return 0;
        }
        else
        {
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
            if (IS_ENABLED(CONFIG_BT_MESH_PROXY))
            {
                if (!_bt_fsm_st_proxy_adv_send_enter(msg))
                {
                    if (bt_mesh_fsm.scan_background)
                    {
                       // _bt_fsm_st_scan_enter(msg);
                    }
                }
            }
            else
#endif
            if (bt_mesh_fsm.scan_background)
            {
                _bt_fsm_st_scan_enter(msg);
            }
        }
    }

    return 0;
}

BT_FSM_HANDLER_DEVINE(BT_MESH,
	BT_FSM_ACTION_DEFINE(BT_MESH, ADV_SEND, bt_fsm_action_bt_mesh_adv_send);
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
    BT_FSM_ACTION_DEFINE(BT_MESH, ADV_PROXY_SEND, bt_fsm_action_bt_mesh_adv_proxy_send);
#endif
    BT_FSM_ACTION_DEFINE(BT_MESH,  EXIT, bt_fsm_action_bt_mesh_exit);
    BT_FSM_ACTION_DEFINE(HCI_CORE, ADV_STARTED, bt_fsm_action_bt_mesh_adv_started);
    BT_FSM_ACTION_DEFINE(HCI_CORE, ADV_STOPED, bt_fsm_action_bt_mesh_adv_stoped);
    BT_FSM_ACTION_DEFINE(HCI_CORE, SCAN_STARTED, bt_fsm_action_bt_mesh_scan_started);
    BT_FSM_ACTION_DEFINE(HCI_CORE, SCAN_STOPED, bt_fsm_action_bt_mesh_scan_stoped);
)

int bt_mesh_fsm_init()
{
    int err = bt_fsm_init(BT_FSM_HANDLER_FUNC_NAME(BT_MESH));
	if (err >= 0)
	{
		bt_mesh_fsm.handle = err;
	}

    k_delayed_work_init(&bt_mesh_fsm.work, _bt_fsm_mesh_work);

#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
    bt_mesh_fsm_proxy_adv_send();
#endif
	return err;
}

#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
int bt_mesh_fsm_proxy_adv_send()
{
    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND)))
    {
        return -EALREADY;
    }

    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND)))
    {
        return -EBUSY;
    }

    void *msg = bt_fsm_create_msg(0, bt_mesh_fsm.handle, BT_FSM_HANDLE_UNUSED);
	if (!msg)
	{
		return -ENOMEM;
	}

    return bt_fsm_set_event(BT_FSM_EV_DEFINE(BT_MESH, ADV_PROXY_SEND), msg);
}
#endif

int bt_mesh_fsm_adv_stop()
{
    bt_fsm_wait_event(bt_mesh_fsm.handle, BT_FSM_EV_DEFINE(HCI_CORE, ADV_STOPED));

    int err = bt_fsm_le_adv_stop(NULL, NULL, bt_mesh_fsm.handle);
    if (err)
    {
        bt_fsm_set_ready(bt_mesh_fsm.handle);
        return err;
    }

    return err;
}

int bt_mesh_fsm_scan_start(void)
{
    bt_mesh_fsm.scan_background = 1;

    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, SCAN)))
    {
        return -EALREADY;
    }

    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND))
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
        || bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND))
#endif
    )
    {
        return 0;
    }

    struct bt_le_scan_param scan_param = {
        .type      = BT_HCI_LE_SCAN_PASSIVE,
        .filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
        .interval   = MESH_SCAN_INTERVAL,
        .window     = MESH_SCAN_WINDOW
    };

    bt_fsm_set_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, SCAN));


    k_delayed_work_cancel(&bt_mesh_fsm.work);

    bt_fsm_wait_event(bt_mesh_fsm.handle, BT_FSM_EV_DEFINE(HCI_CORE, SCAN_STARTED));
#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
    int err = bt_fsm_le_scan_start(&scan_param, NULL, NULL, NULL, bt_mesh_fsm.handle);
#else
    int err = bt_fsm_le_scan_start(&scan_param, bt_mesh_scan_cb, NULL, NULL, bt_mesh_fsm.handle);
#endif
    if (err)
    {
        bt_fsm_set_ready(bt_mesh_fsm.handle);
        bt_fsm_clear_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, SCAN));
        return err;
    }

    return err;
}

int bt_mesh_fsm_scan_stop(void)
{
    bt_mesh_fsm.scan_background = 0;

    if (!bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, SCAN)))
    {
        return 0;
    }

    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND))
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
        || bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND))
#endif
    )
    {
        return 0;
    }

    k_delayed_work_cancel(&bt_mesh_fsm.work);
    int err = bt_fsm_le_scan_stop(NULL, NULL, bt_mesh_fsm.handle);
    if (err)
    {
        return err;
    }

    return bt_fsm_wait_event(bt_mesh_fsm.handle, BT_FSM_EV_DEFINE(HCI_CORE, SCAN_STOPED));
}

int bt_mesh_fsm_adv_send()
{
    if (bt_fsm_test_state(bt_mesh_fsm.handle, BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND)) || bt_mesh_fsm.adv_buf)
    {
        return 0;
    }

    void *msg = bt_fsm_create_msg(0, bt_mesh_fsm.handle, BT_FSM_HANDLE_UNUSED);
	if (!msg)
	{
		return -ENOMEM;
	}

    return bt_fsm_set_event(BT_FSM_EV_DEFINE(BT_MESH, ADV_SEND), msg);
}

int bt_mesh_fsm_refresh_state()
{
    bt_mesh_fsm_adv_send();
    return 0;
}

#endif