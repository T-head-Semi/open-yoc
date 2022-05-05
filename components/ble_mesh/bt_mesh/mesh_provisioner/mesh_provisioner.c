/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <api/mesh.h>

#include "provisioner_prov.h"
#include "provisioner_main.h"
#include "provisioner_proxy.h"
#include "mesh_provisioner.h"
#include "aos/kernel.h"
#include "mesh_model/mesh_model.h"

#ifdef CONFIG_BT_MESH_PROVISIONER


typedef struct {
    mesh_node_t node;
    long long time_found;
} report_node;

typedef struct {
    uint8_t front;
    uint8_t rear;
    uint8_t length;
    report_node dev[DEF_REPORT_QUEUE_LENGTH];
} node_data_queue;



#ifndef MAX_MAC_FILTER_NUM
#define MAX_MAC_FILTER_NUM 20
#endif


typedef struct _mesh_provisoner {
    provisioner_cb prov_cb;

    uint8_t filter_policy;
    uint8_t uuid_filter[16];
    uint8_t uuid_filter_start;
    uint8_t uuid_filter_length;
    klist_t mac_filter_head;
    k_timer_t found_dev_timer;
    uint32_t found_dev_timeout;//

    k_timer_t node_report_timer;
    node_data_queue dev_queue;

    uint8_t init_flag: 1;
    uint8_t filter_flag: 1;
    uint8_t filter_mac_flag: 1;
    uint8_t filter_mac_add_dev_flag: 1;
    uint8_t found_dev_succeed: 1;
    uint8_t prov_dev_succeed: 1;
    uint8_t prov_show_dev: 1;
    uint8_t enable: 1;
    struct bt_mesh_device_delete del_dev;
    ble_mesh_provisioner_t comp;
} mesh_provisoner;

#define TAG "BT_MESH_PROVISIONER"

#define COMMON_SUB_ADDR 0xF001
#define COMMON_PUB_ADDR 0xF000
#define K_FOREVER -1
#define UUID_FILTER_MAX_LENGTH 16
#define STATIC_OOB_MAX_LENGTH 16
#define LOCAL_USER_TERM_CONN 0x16
extern struct k_sem prov_input_sem;
extern u8_t   prov_input[8];
extern u8_t   prov_input_size;
static uint8_t g_static_oob_data[STATIC_OOB_MAX_LENGTH + 1];
static int provisioner_input_static_oob();
static int provisioner_input_num(bt_mesh_output_action_t act, u8_t size);
static int provisioner_output_num(bt_mesh_input_action_t act, u8_t size);
static void provisioner_link_open(bt_mesh_prov_bearer_t bearer, uint8_t addr_val[6], uint8_t addr_type, uint8_t uuid[16], uint8_t prov_count);
static void provisioner_link_close(bt_mesh_prov_bearer_t bearer, u8_t reason, uint8_t addr_val[6], uint8_t addr_type, uint8_t uuid[16], uint8_t prov_count);
static void provisioner_complete(int node_idx, const u8_t device_uuid[16],
                                 u16_t unicast_addr, u8_t element_num,
                                 u16_t netkey_idx, bool gatt_flag);

static struct bt_mesh_provisioner g_provisioner = {
    .prov_uuid              = 0,
    .prov_attention         = 0,
    .prov_algorithm         = 0,
    .prov_pub_key_oob       = 0,
    .prov_pub_key_oob_cb    = 0,
    .prov_static_oob_val    = g_static_oob_data,
    .prov_static_oob_len    = 0,
    .prov_input_static_oob  = provisioner_input_static_oob,
    .prov_input_num         = provisioner_input_num,
    .prov_output_num        = provisioner_output_num,
    .flags                  = 0,
    .iv_index               = 0,
    .prov_link_open         = provisioner_link_open,
    .prov_link_close        = provisioner_link_close,
    .prov_complete          = provisioner_complete,
};

mesh_provisoner g_mesh_prov = {
    .comp = {.provisioner = &g_provisioner,},
};




static bool is_node_data_queue_empty(node_data_queue *queue)
{
    return queue->front == queue->rear;
}

static bool is_node_data_queue_full(node_data_queue *queue)
{
    if ((queue->rear + 1) % DEF_REPORT_QUEUE_LENGTH == queue->front) {
        return true;
    } else {
        return false;
    }
}

static report_node *get_node_data(node_data_queue *queue)
{
    if (is_node_data_queue_empty(queue)) {
        return NULL;
    } else {
        report_node *data = &queue->dev[queue->front];
        queue->front = (queue->front + 1) % DEF_REPORT_QUEUE_LENGTH;
        return data;
    }
}

#if 0
static mesh_node_t *found_node(node_data_queue *queue, mesh_node_t *node)
{
    if (is_node_data_queue_empty(queue)) {
        return NULL;
    } else {

        for (uint8_t i = queue->front; i < queue->rear ; i++) {
            if (!memcmp(queue->dev[i].node.uuid, node->uuid, 16) && !memcmp(queue->dev[i].node.dev_addr, node->dev_addr, 6) &&  \
                queue->dev[i].node.addr_type == node->addr_type && queue->dev[i].node.bearer == node->bearer && queue->dev[i].node.oob_info == node->oob_info) {
                return &queue->dev[i].node;
            }
        }

        return NULL;
    }
}
#endif

static int put_node_data(node_data_queue *queue, mesh_node_t *node)
{

    if (!queue || !node) {
        return -1;
    }

    if (is_node_data_queue_full(queue)) {
        return -1;
    }

    for (int i = queue->front; i < queue->rear ; i++) {
        if (!memcmp(queue->dev[i].node.uuid, node->uuid, 16) && !memcmp(queue->dev[i].node.dev_addr, node->dev_addr, 6) &&  \
            queue->dev[i].node.addr_type == node->addr_type && queue->dev[i].node.bearer == node->bearer && queue->dev[i].node.oob_info == node->oob_info) {
            return 0;
        }
    }

    memcpy(queue->dev[queue->rear].node.uuid, node->uuid, 16);
    memcpy(queue->dev[queue->rear].node.dev_addr, node->dev_addr, 6);
    queue->dev[queue->rear].node.addr_type = node->addr_type;
    queue->dev[queue->rear].node.bearer = node->bearer;
    queue->dev[queue->rear].node.oob_info = node->oob_info;
    queue->dev[queue->rear].time_found = aos_now_ms();
    queue->rear = (queue->rear + 1) % DEF_REPORT_QUEUE_LENGTH;
    return 0;
}


static void clear_node_data()
{
    g_mesh_prov.dev_queue.front = 0;
    g_mesh_prov.dev_queue.rear  = 0;
    g_mesh_prov.dev_queue.length = DEF_REPORT_QUEUE_LENGTH;
}

void node_report_timeout(void *timer, void *args)
{
    if (!g_mesh_prov.enable) {
        return;
    }

    report_node *report_node = NULL;
    int index = 0;
    int16_t ret;
    bt_addr_le_t addr = {0};
    k_timer_start(&g_mesh_prov.node_report_timer, DEF_DEV_REPORT_TIMEOUT);
    long long time_now = aos_now_ms();
    report_node = get_node_data(&g_mesh_prov.dev_queue);

    if (!report_node) {
        return;
    }

    if (report_node->time_found > time_now || time_now - report_node->time_found > DEF_REPORT_DEV_SURVIVE_TIME) {
        LOGD(TAG, "drop the node survive %d ms", time_now - report_node->time_found);
        return;
    }

    if (0 != report_node->node.prim_unicast) {
        LOGD(TAG, "drop the node %04x has ben proved", report_node->node.prim_unicast);
        return;
    }

    memcpy(addr.a.val, report_node->node.dev_addr, sizeof(addr.a.val));
    addr.type = report_node->node.addr_type;
    extern int provisioner_dev_find(const bt_addr_le_t *addr, const u8_t uuid [ 16 ], int *index);
    ret = provisioner_dev_find(&addr, report_node->node.uuid, &index);

    if (!ret) {
        LOGD(TAG, "drop the node %04x has been added", report_node->node.prim_unicast);
        return;
    }

    if (g_mesh_prov.prov_cb) {
        g_mesh_prov.prov_cb(BT_MESH_EVENT_RECV_UNPROV_DEV_ADV, &report_node->node);
    }
}


void mac_filter_list_init(klist_t *list)
{
    klist_init(list);
}

mac_filters_dev* mac_filter_dev_search(klist_t *list,dev_addr_t dev_info)
{
    klist_t    *q;
    klist_t    *start;
    klist_t    *end;
    mac_filters_dev   *device_iter_temp = NULL;

    start = list;
    end   = list;

    for (q = start->next; q != end; q = q->next) {
        device_iter_temp = krhino_list_entry(q, mac_filters_dev, list);
        if (!memcmp(&device_iter_temp->addr,&dev_info,sizeof(dev_addr_t))) {
            return device_iter_temp;
        }
    }
    return NULL;
}

mac_filters_dev* mac_filter_dev_get(klist_t *list)
{
    klist_t    *q;
    klist_t    *start;
    klist_t    *end;
    //mac_filters_dev   *device_iter_temp = NULL;

    start = list;
    end   = list;

    for (q = start->next; q != end; q = q->next) {
        return krhino_list_entry(q, mac_filters_dev, list);
    }
    return NULL;
}


int mac_filter_add_dev(klist_t *list,mac_filters_dev *dev)
{
    if(mac_filter_dev_search(list,dev->addr)) {
        LOGE(TAG,"device already exist");
        return -1;
    }
    klist_insert(list, &dev->list);
    return 0;
}


int mac_filter_rm_dev(klist_t *list,dev_addr_t dev_info)
{
    mac_filters_dev* dev = mac_filter_dev_search(list,dev_info);
    if(!dev) {
        LOGE(TAG,"no dev found");
        return -1;
    }
    klist_rm(&dev->list);
    return 0;
}


static void provisioner_unprovisioned_dev_found(const u8_t addr[6], const u8_t addr_type,
        const u8_t adv_type, const u8_t dev_uuid[16],
        u16_t oob_info, bt_mesh_prov_bearer_t bearer)
{

    if (!g_mesh_prov.prov_show_dev) {
        return;
    }

    if (g_mesh_prov.filter_flag) {
        if (memcmp(g_mesh_prov.uuid_filter, dev_uuid + g_mesh_prov.uuid_filter_start, g_mesh_prov.uuid_filter_length)) {
            return;
        }
    }

    if(g_mesh_prov.filter_mac_flag && g_mesh_prov.filter_mac_add_dev_flag) {
        dev_addr_t addr_temp = {0};
        memcpy(&addr_temp.val,addr,6);
        addr_temp.type   =  addr_type;
        if(!mac_filter_dev_search(&g_mesh_prov.mac_filter_head, addr_temp)) {
            return;
        }
    }



    mesh_node_t node;
    node.bearer = bearer;
    node.addr_type = addr_type;
    memcpy(node.dev_addr, addr, sizeof(node.dev_addr));
    node.oob_info = oob_info;
    memcpy(node.uuid, dev_uuid, sizeof(node.uuid));
    put_node_data(&g_mesh_prov.dev_queue, &node);

    g_mesh_prov.found_dev_succeed = 1;
}


static int provisioner_input_num(bt_mesh_output_action_t act, u8_t size)
{
    bool input_num_flag = 0;

    if (BT_MESH_DISPLAY_NUMBER == act) {
        input_num_flag = true;

        if (g_mesh_prov.prov_cb) {
            g_mesh_prov.prov_cb(BT_MESH_EVENT_OOB_INPUT_NUM, &size);
        }

    } else if (BT_MESH_DISPLAY_STRING == act) {
        input_num_flag = false;

        if (g_mesh_prov.prov_cb) {
            g_mesh_prov.prov_cb(BT_MESH_EVENT_OOB_INPUT_STRING, &size);
        }
    }

    k_sem_take(&prov_input_sem, K_FOREVER);
    LOGI(TAG, "xxxxxxxxxxxx: %s", prov_input);
    LOGI(TAG, "get input ");
    LOGI(TAG, "xxxxxxxxxxxx: %u", prov_input_size);

    bt_mesh_prov_input_data(prov_input, prov_input_size, input_num_flag);

    return 0;
}


static int provisioner_input_static_oob()
{

    if (g_mesh_prov.prov_cb) {
        g_mesh_prov.prov_cb(BT_MESH_EVENT_OOB_INPUT_STATIC_OOB, NULL);
    } else {
        return 0;
    }

    k_sem_take(&prov_input_sem, K_FOREVER);
    g_static_oob_data[16] = '\0';

    LOGI(TAG, "xxxxxxxxxxxx: %s", g_static_oob_data);
    LOGI(TAG, "get input ");

    return 0;

}


static int provisioner_output_num(bt_mesh_input_action_t act, u8_t size)
{
    u32_t div[8]    = { 10, 100, 1000, 10000, 100000,
                        1000000, 10000000, 100000000
                      };
    u32_t num       = 0;
    u8_t  temp[8];
    u8_t  i;
    bool  output_num_flag = 0;

    if (BT_MESH_ENTER_NUMBER == act) {

        output_num_flag = true;
        extern int bt_rand(void *buf, size_t len);
        bt_rand(&num, size);

        num %= div[size - 1];

        LOGI(TAG, "===================");
        LOGI(TAG, "input number %06u in the device", num);
        LOGI(TAG, "===================");

        memset(temp, 0, sizeof(temp));

        for (i = size; i > 0; i--) {
            temp[i - 1] = num & 0xFF;
            num >>= 8;

            if (num == 0) {
                break;
            }
        }
    } else if (BT_MESH_ENTER_STRING == act) {
        output_num_flag = false;

        extern int bt_rand(void *buf, size_t len);
        bt_rand(temp, size);

        /* Normalize to '0' .. '9' & 'A' .. 'Z' */
        for (i = 0; i < size; i++) {
            temp[i] %= 36;

            if (temp[i] < 10) {
                temp[i] += '0';
            } else {
                temp[i] += 'A' - 10;
            }
        }

        temp[size] = '\0';

        LOGI(TAG, "===================");
        LOGI(TAG, "input string %s in the device", temp);
        LOGI(TAG, "===================");
    }

    bt_mesh_prov_output_data(temp, size, output_num_flag);

    return 0;

}


static void provisioner_link_open(bt_mesh_prov_bearer_t bearer, uint8_t addr_val[6], uint8_t addr_type, uint8_t uuid[16], uint8_t prov_count)
{
    LOGD(TAG, "Provisioner link opened on %d\n", bearer);
#if 0
    if (addr_val && uuid) {
        memcpy(g_mesh_prov.del_dev.uuid, uuid, sizeof(g_mesh_prov.del_dev.uuid));
        memcpy(g_mesh_prov.del_dev.addr, addr_val, sizeof(g_mesh_prov.del_dev.addr));
        g_mesh_prov.del_dev.addr_type = addr_type;
    }
#endif
}


static void provisioner_link_close(bt_mesh_prov_bearer_t bearer, u8_t reason, uint8_t addr_val[6], uint8_t addr_type, uint8_t uuid[16], uint8_t prov_count)
{
    LOGD(TAG, "Provisioner link closed on %d reason %d\n", bearer, reason);
    int ret = 0;

    //ADV
    if (BT_MESH_PROV_ADV == bearer && reason && g_mesh_prov.prov_cb) {
        g_mesh_prov.prov_cb(BT_MESH_EVENT_PROV_FAILD, (void *)&reason);
    } else if (BT_MESH_PROV_GATT == bearer && (reason && LOCAL_USER_TERM_CONN != reason) && g_mesh_prov.prov_cb) {
        g_mesh_prov.prov_cb(BT_MESH_EVENT_PROV_FAILD, (void *)&reason);
    }

    if (DEF_MAX_PROV_RETRY < prov_count) {
        bt_addr_le_t addr;
        int i = 0;
        memcpy(addr.a.val,addr_val,6);
        addr.type = addr_type;
        ret = provisioner_dev_remove(&addr,uuid,&i);
        if (ret) {
            LOGE(TAG, "del the faild prov dev faild");
        } else {
            LOGD(TAG, "remove the faild prov dev succeed");
        }

    }
}


static void provisioner_complete(int node_idx, const u8_t device_uuid[16],
                                 u16_t unicast_addr, u8_t element_num,
                                 u16_t netkey_idx, bool gatt_flag)
{

    if (g_mesh_prov.prov_cb) {
        mesh_node_t node = {0};
        node.prim_unicast = unicast_addr;
        node.element_num = element_num;
        node.net_idx = netkey_idx;
        node.bearer = gatt_flag?BT_MESH_PROV_GATT:BT_MESH_PROV_ADV;
        memcpy(node.uuid, device_uuid, 16);
        g_mesh_prov.prov_dev_succeed = 1;
        g_mesh_prov.prov_cb(BT_MESH_EVENT_PROV_COMP, &node);
    }
}

static void found_dev_time_out(void *timer, void *arg)
{

    k_timer_stop(&g_mesh_prov.found_dev_timer);

    if (g_mesh_prov.found_dev_succeed) {
        g_mesh_prov.found_dev_succeed = 0;
    } else {
        g_mesh_prov.prov_cb(BT_MESH_EVENT_FOUND_DEV_TIMEOUT, NULL);
    }

}

int ble_mesh_provisioner_init(provisioner_config_t *param)
{
    if (!param) {
        return -1;
    }

	if(g_mesh_prov.init_flag) {
        return -EALREADY;
	}

    const struct bt_mesh_comp *mesh_comp = ble_mesh_model_get_comp_data();
    if(!mesh_comp) {
        return -1;
    }

	k_timer_init(&g_mesh_prov.found_dev_timer, found_dev_time_out, NULL);
    k_timer_init(&g_mesh_prov.node_report_timer, node_report_timeout, NULL);

    g_provisioner.prov_unicast_addr = param->unicast_addr_start;
    g_provisioner.prov_start_address = param->unicast_addr_start + mesh_comp->elem_count;
    g_provisioner.prov_end_address   = param->unicast_addr_end;
    g_provisioner.prov_attention = param->attention_time;
    g_mesh_prov.prov_cb = param->cb;
    g_mesh_prov.comp.provisioner = &g_provisioner;
    g_mesh_prov.init_flag = 1;

    return 0;
}

const ble_mesh_provisioner_t *ble_mesh_provisioner_get_provisioner_data()
{
    return &g_mesh_prov.comp;
}


int ble_mesh_provisioner_enable()
{

    int16_t ret;

    if (!g_mesh_prov.init_flag) {
        return -1;
    }

    ret = bt_mesh_provisioner_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

    if (ret) {
        return ret;
    }

    ret = bt_mesh_prov_adv_pkt_cb_register(provisioner_unprovisioned_dev_found);

    if (ret) {
        return ret;
    }

    g_mesh_prov.enable = 1;

    return 0;
}


int ble_mesh_provisioner_disable()
{
    int16_t ret;

    if (!g_mesh_prov.init_flag) {
        return -1;
    }

    ret = bt_mesh_provisioner_disable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

    if (ret) {
        return ret;
    }

    if (g_mesh_prov.found_dev_timeout) {
        k_timer_stop(&g_mesh_prov.found_dev_timer);
    }

    g_mesh_prov.enable = 0;

    return 0;
}



int ble_mesh_provisioner_dev_filter(uint8_t enable, uuid_filter_t *filter)
{
    if (!g_mesh_prov.init_flag || !g_mesh_prov.enable) {
        return -1;
    }

    if (!enable) {
        g_mesh_prov.filter_flag = 0;
        return 0;
    }

    if (!filter || !filter->uuid || !filter->uuid_length) {
        return -1;
    }

    if ((filter->filter_start + filter->uuid_length) > 16) {
        return -1;
    }

    memcpy(g_mesh_prov.uuid_filter, filter->uuid, filter->uuid_length);
    g_mesh_prov.uuid_filter_start = filter->filter_start;
    g_mesh_prov.uuid_filter_length = filter->uuid_length;

    g_mesh_prov.filter_flag  = 1;
    return 0;
}


int ble_mesh_provisioner_mac_filter_enable()
{
    static int mac_filter_init_flag = 0;
    if (!g_mesh_prov.init_flag || !g_mesh_prov.enable) {
        return -1;
    }
    if(g_mesh_prov.filter_mac_flag) {
        return -EALREADY;
    } else {
        if(!mac_filter_init_flag) {
            mac_filter_list_init(&g_mesh_prov.mac_filter_head);
            g_mesh_prov.filter_mac_add_dev_flag = 0;
            mac_filter_init_flag = 1;
        } else {
            if(g_mesh_prov.filter_mac_add_dev_flag) {
                clear_node_data();
            }
        }
        g_mesh_prov.filter_mac_flag  = 1;

    }
    return 0;

}


int ble_mesh_provisioner_mac_filter_disable()
{
    if (!g_mesh_prov.init_flag || !g_mesh_prov.enable) {
        return -1;
    }

    if(!g_mesh_prov.filter_mac_flag) {
        return -EALREADY;
    }

    g_mesh_prov.filter_mac_flag  = 0;
    return 0;

}


int ble_mesh_provisioner_mac_filter_clear()
{
    if (!g_mesh_prov.init_flag) {
        return -1;
    }

    mac_filters_dev * dev = NULL;
    dev = mac_filter_dev_get(&g_mesh_prov.mac_filter_head);
    while(dev) {
        klist_rm(&dev->list);
        aos_free(dev);
        dev = mac_filter_dev_get(&g_mesh_prov.mac_filter_head);
    }
    mac_filter_list_init(&g_mesh_prov.mac_filter_head);
    g_mesh_prov.filter_mac_add_dev_flag = 0;

    return 0;

}


int ble_mesh_provisioner_mac_filter_dev_add(uint8_t             mac_size,dev_addr_t *mac)
{
    if(!g_mesh_prov.filter_mac_flag) {
        return -1;
    }

    if(!mac_size || !mac) {
        return -1;
    }

    mac_filters_dev* mac_list  = NULL;
    mac_list = (mac_filters_dev* )aos_malloc(mac_size * sizeof(mac_filters_dev));
    if(!mac_list) {
        return -1;
    }

    for(int i = 0; i< mac_size; i++) {
        if(mac_filter_dev_search(&g_mesh_prov.mac_filter_head, mac[i])) {
            aos_free(&mac_list[i]);
        } else {
            memcpy(&mac_list[i].addr,&mac[i],sizeof(dev_addr_t));
            if(mac_filter_add_dev(&g_mesh_prov.mac_filter_head, &mac_list[i])) {
                aos_free(&mac_list[i]);
                return -1;
            }
        }
    }

    clear_node_data();

    g_mesh_prov.filter_mac_add_dev_flag = 1;
    return 0;

}


int ble_mesh_provisioner_mac_filter_dev_rm(uint8_t            mac_size,dev_addr_t *mac)
{
    if(!g_mesh_prov.filter_mac_flag) {
        return -1;
    }

    if(!mac_size || !mac) {
        return -1;
    }

    uint8_t found_dev_flag = 0;
    mac_filters_dev* dev= NULL;

    for(int i = 0; i < mac_size; i++) {
        dev = mac_filter_dev_search(&g_mesh_prov.mac_filter_head,mac[i]);
        if(dev) {
            klist_rm(&dev->list);
            aos_free(dev);
            found_dev_flag = 1;
        }
    }

    if(is_klist_empty(&g_mesh_prov.mac_filter_head)) {
        g_mesh_prov.filter_mac_add_dev_flag = 0;
    }

    if(found_dev_flag) {
        return 0;
    } else {
        return -1;
    }

}




int ble_mesh_provisioner_show_dev(uint8_t enable, uint32_t timeout)
{
    if (!g_mesh_prov.init_flag || !g_mesh_prov.enable) {
        return -1;
    }

    g_mesh_prov.dev_queue.front = 0;
    g_mesh_prov.dev_queue.rear = 0;
    g_mesh_prov.dev_queue.length = DEF_REPORT_QUEUE_LENGTH;

    if (enable) {
        g_mesh_prov.prov_show_dev = 1;

        if (timeout) {
            k_timer_start(&g_mesh_prov.found_dev_timer, timeout * 1000);
            g_mesh_prov.found_dev_succeed = 0;
        }
        k_timer_start(&g_mesh_prov.node_report_timer,DEF_DEV_REPORT_TIMEOUT);
    } else {
        k_timer_stop(&g_mesh_prov.found_dev_timer);
        k_timer_stop(&g_mesh_prov.node_report_timer);
        g_mesh_prov.prov_show_dev = 0;
    }

    return 0;

}

int ble_mesh_provisioner_dev_add(mesh_node_t *node, uint8_t add_appkey)
{
    if (!g_mesh_prov.init_flag || !g_mesh_prov.enable) {
        return -1;
    }

    int16_t err;
    struct bt_mesh_unprov_dev_add unprov_dev = {0};
    if (!node) {
        return -1;
    }


    unprov_dev.addr_type = node->addr_type;
    memcpy(unprov_dev.addr, node->dev_addr, 6);
    memcpy(unprov_dev.uuid, node->uuid, sizeof(unprov_dev.uuid));

    unprov_dev.oob_info = node->oob_info;
    unprov_dev.bearer = node->bearer;
    unprov_dev.auto_add_appkey = add_appkey;

    err = bt_mesh_provisioner_add_unprov_dev(&unprov_dev, RM_AFTER_PROV | FLUSHABLE_DEV);

    if (err) {
        LOGE(TAG, "add unprov device fail err %d\n", err);
        return err;
    }

    return 0;
}




int ble_mesh_provisioner_get_add_appkey_flag(u16_t unicast_addr)
{
    if (!g_mesh_prov.init_flag || !g_mesh_prov.enable) {
        return -1;
    }
    extern uint8_t get_node_auto_add_appkey_flag(u16_t unicast_addr);
    return get_node_auto_add_appkey_flag(unicast_addr);
}



int ble_mesh_provisioner_dev_del(uint8_t addr[6], uint8_t addr_type, uint8_t uuid[16])
{

    if (!g_mesh_prov.init_flag || !g_mesh_prov.enable || !addr || !uuid) {
        return -1;
    }

    int16_t err;
    struct bt_mesh_device_delete del_dev;

    memcpy(del_dev.addr, addr, sizeof(del_dev.addr));
    memcpy(del_dev.uuid, uuid, sizeof(del_dev.uuid));
    del_dev.addr_type = addr_type;

    err = bt_mesh_provisioner_delete_unprov_device(&del_dev);

    if (err) {
        LOGE(TAG, "del unprov device fail err %d\n", err);
        return err;
    }

    return 0;
}


int ble_mesh_provisioner_OOB_input_string(const char *str)
{
    if (!g_mesh_prov.init_flag || !g_mesh_prov.enable || !str) {
        return -1;
    }

    uint8_t input_length = strlen(str) < prov_input_size ? strlen(str) : prov_input_size;

    memcpy(prov_input, str, input_length);
    k_sem_give(&prov_input_sem);
    return 0;
}

int ble_mesh_provisioner_OOB_input_num(uint32_t num)
{
    if (!g_mesh_prov.init_flag || !g_mesh_prov.enable || !num) {
        return -1;
    }

    uint8_t i;
    memset(prov_input, 0, sizeof(prov_input));

    for (i = prov_input_size; i > 0; i--) {
        prov_input[i - 1] = num & 0xFF;
        num >>= 8;
    }

    k_sem_give(&prov_input_sem);

    return 0;
}


int ble_mesh_provisioner_static_OOB_set(const uint8_t *oob, uint16_t oob_size)
{
    if (!g_mesh_prov.init_flag || !g_mesh_prov.enable || !oob || !oob_size) {
        return -1;
    }

    memcpy(g_static_oob_data, oob, oob_size);
    g_provisioner.prov_static_oob_len = oob_size;
    k_sem_give(&prov_input_sem);

    return 0;

}


mesh_node_t *ble_mesh_provisioner_get_node_info(u16_t unicast_addr)
{
    if (!g_mesh_prov.init_flag || !g_mesh_prov.enable || !unicast_addr || unicast_addr > 0x8000) {
        return NULL;
    }

    static mesh_node_t g_node_info;

    struct bt_mesh_node_t *node;

    memset(&g_node_info, 0, sizeof(mesh_node_t));

    node = bt_mesh_provisioner_get_node_info(unicast_addr);

    if (!node) {
        return NULL;
    } else {
        memcpy(g_node_info.uuid, node->dev_uuid, 16);
        memcpy(g_node_info.dev_addr, node->addr_val, 6);
        g_node_info.addr_type = node->addr_type;
        g_node_info.prim_unicast = node->unicast_addr;
        g_node_info.oob_info = node->oob_info;
        g_node_info.element_num = node->element_num;
        g_node_info.node_name = (uint8_t *)node->node_name;
        g_node_info.flags = node->flags;
        g_node_info.iv_index = node->iv_index;
        g_node_info.net_idx = node->net_idx;
    }

    return &g_node_info;
}

#endif

