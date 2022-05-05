/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifdef CONFIG_BT_MESH

#include <stdio.h>
#include <ctype.h>
#include "../at_internal.h"
#include <yoc/atserver.h>
#include "at_mesh.h"
#include <api/mesh.h>
#include <aos/kv.h>
#include <aos/ble.h>
#include <mesh_node.h>
#include <yoc/partition.h>
#include <errno.h>
#include <common/log.h>
#include <inc/net.h>

#ifdef CONFIG_BT_MESH_PROVISIONER
#include "mesh_provisioner.h"
#include "provisioner_main.h"
#include "provisioner_prov.h"
#endif

#ifdef CONFIG_BT_MESH_SIG_MODEL
#include "mesh_model/mesh_model.h"
#endif
#if defined(CONFIG_OTA_SERVER)
#include "ota_server.h"
#endif


#define TAG "at_mesh_cmd"
#define TIMEOUT 402
#define TIMEOUT1 400
#define VND_CID 0x01A8
#define  ADDR_MAC_SZIE       30
#define  MAX_MAC_FILTER_SIZE 6
#define  MAC_FILTER_ADDR_STR_LENGTH    180 // (ADDR_MAC_SZIE * MAX_MAC_FILTER_SIZE)

static uint8_t dev_addr_str[MAC_FILTER_ADDR_STR_LENGTH] = {0};


#define AT_BACK_RET_OK_HEX(cmd ,val)  atserver_send("\r\n%s:%04x\r\nOK\r\n", cmd + 2, val)
#define AT_BACK_RET_OK_INT3(cmd,val1, val2,val3)           atserver_send("\r\n%s:%d,%d,%d\r\nOK\r\n", cmd+2, val1, val2, val3)
#define AT_BACK_TEST_RET_OK(cmd, par1)             atserver_send("\r\n%s=%s\r\nOK\r\n", cmd + 2, par1)
#define BT_MESH_ADDR_IS_UNICAST(addr) ((addr) && (addr) < 0x8000)
#define BT_MESH_ADDR_IS_GROUP(addr) ((addr) >= 0xc000 && (addr) <= 0xff00)



#define ERR_CHAR 0XFF
#define TTL_DEF 7
#define CTL_TEMP_MIN 800
#define CTL_TEMP_MAX 20000

#define _LENGTH_TRS(n)  #n
#define LENGTH_TRS(n)  _LENGTH_TRS(n)

#define _LENGTH_MAC_FILTER_ADDR(n) #n
#define LENGTH_MAC_FILTER_ADDR(n)  _LENGTH_MAC_FILTER_ADDR(n)

typedef struct {
    oob_type_en oob_type;
    uint8_t oob_length;
} oob_input_info;



typedef struct _at_mesh {
    uint8_t init_flag: 1;
    uint8_t role: 2;
    uint8_t auto_config_flag: 1;
    uint8_t get_pub_flag: 1;
    uint8_t provisioner_enabled: 1;
    oob_input_info oob_info;
#ifdef CONFIG_BT_MESH_PROVISIONER
    aos_timer_t   rst_check_timer;
    aos_mutex_t   rst_list_mutex;
    slist_t       rst_node_head;
    uint8_t       rst_timer_start_flag;
#endif
} at_mesh;


#ifdef CONFIG_BT_MESH_PROVISIONER

#define NODE_RST_CHECK_TIMEOUT    (200)//MS
#define NODE_RST_MAX_RETRY_COUNT  (10)

typedef struct {
    slist_t    list;
    uint16_t   addr;
    long long  last_try_time;
    uint8_t    retry_count;
} reset_node;
#endif


static at_mesh g_mesh_server;



static inline int bt_addr_val_to_str(const uint8_t addr[6], char *str, size_t len)
{
    return snprintf(str, len, "%02X:%02X:%02X:%02X:%02X:%02X", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}



void at_provisioner_cb(mesh_provisioner_event_en event, void *p_arg)
{
    switch (event) {
    // provisioner message
    case BT_MESH_EVENT_RECV_UNPROV_DEV_ADV: {
        if (p_arg) {
            mesh_node_t *node = (mesh_node_t *)p_arg;
            uint8_t dev_addr_str[20];
            bt_addr_val_to_str(node->dev_addr, (char *)dev_addr_str, sizeof(dev_addr_str));
            atserver_send("\r\n+MESHDEV:%s,%02x,%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x,%02x,%02x\r\n",  \
                          dev_addr_str, node->addr_type,node->uuid[0], node->uuid[1], node->uuid[2], node->uuid[3], node->uuid[4], node->uuid[5], \
                          node->uuid[6], node->uuid[7],node->uuid[8], node->uuid[9], node->uuid[10], node->uuid[11], node->uuid[12], node->uuid[13], node->uuid[14], node->uuid[15], node->oob_info, node->bearer);
        }

    }
    break;

    case BT_MESH_EVENT_FOUND_DEV_TIMEOUT: {
        atserver_send("\r\n+MESHFOUNDDEVTIMEOUT\r\n");
    }
    break;

    case BT_MESH_EVENT_PROV_FAILD: {
        uint8_t reason = *(uint8_t *)p_arg;
        atserver_send("\r\n+MESHPROVFAILD:%x\r\n", reason);
    }
    break;

    case BT_MESH_EVENT_OOB_INPUT_NUM: {
        if (p_arg) {
            uint8_t size;
            size = *(uint8_t *)p_arg;
            atserver_send("\r\n+MESHOOBINPUT:%02x,%02x\r\n", OOB_NUM, size);
            g_mesh_server.oob_info.oob_type = OOB_NUM;
            g_mesh_server.oob_info.oob_length = size;//fix me
        }
    }
    break;

    case BT_MESH_EVENT_OOB_INPUT_STATIC_OOB: {
        atserver_send("\r\n+MESHOOBINPUT:%02x\r\n", OOB_STATIC);
        g_mesh_server.oob_info.oob_type = OOB_STATIC;
    }
    break;

    case BT_MESH_EVENT_OOB_INPUT_STRING: {
        if (p_arg) {
            uint8_t size;
            size = *(uint8_t *)p_arg;
            atserver_send("\r\n+MESHOOBINPUT:%02x,%02x\r\n", OOB_STR, size);
            g_mesh_server.oob_info.oob_type = OOB_STR;
            g_mesh_server.oob_info.oob_length = size;
        }
    }
    break;

    case BT_MESH_EVENT_PROV_COMP: {
        if (p_arg) {
            mesh_node_t *node = (mesh_node_t *)p_arg;

            atserver_send("\r\n+MESHNODEADD:%04x,%02x,%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\r\n" \
                          , node->prim_unicast, node->element_num, \
                          node->uuid[0], node->uuid[1], node->uuid[2], node->uuid[3], node->uuid[4], node->uuid[5], node->uuid[6], node->uuid[7], \
                          node->uuid[8], node->uuid[9], node->uuid[10], node->uuid[11], node->uuid[12], node->uuid[13], node->uuid[14], node->uuid[15]);
#if 0
            mesh_node_t *prov_node = NULL;

            prov_node = found_node(&g_mesh_server.dev_queue, node);

            if (prov_node) {
                prov_node->prim_unicast = node->prim_unicast;
            }
#endif
        }
    }
    break;

    default:
        break;
    }

}



static int print_comp_data(uint16_t addr, struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(comp, 50);

    uint16_t copy_length;


    copy_length = net_buf_simple_tailroom(&comp) < buf->len ? net_buf_simple_tailroom(&comp) :  buf->len;
    net_buf_simple_add_mem(&comp, buf->data, copy_length);

    //addr CID PID VID CRPL Features
    atserver_send("\r\n+MESHCOMPDATA:%04x,%04x,%04x,%04x,%04x,%04x", addr, net_buf_simple_pull_le16(&comp), net_buf_simple_pull_le16(&comp), \
                  net_buf_simple_pull_le16(&comp), net_buf_simple_pull_le16(&comp), net_buf_simple_pull_le16(&comp));

    while (comp.len > 4) {
        uint8_t sig_model_num;
        uint8_t vnd_model_num;
        uint16_t elem_idx;
        int i;
        elem_idx = net_buf_simple_pull_le16(&comp);
        sig_model_num = net_buf_simple_pull_u8(&comp);
        vnd_model_num = net_buf_simple_pull_u8(&comp);

        atserver_send(",%04x,%02x,%02x", elem_idx, sig_model_num, vnd_model_num);

        if (comp.len < ((sig_model_num * 2) + (vnd_model_num * 4))) {
            atserver_send("\r\n");
            break;
        }

        for (i = 0; i < sig_model_num; i++) {
            u16_t mod_id = net_buf_simple_pull_le16(&comp);
            atserver_send(",%04x", mod_id);
        }


        for (i = 0; i < vnd_model_num; i++) {
            u16_t cid = net_buf_simple_pull_le16(&comp);
            u16_t mod_id = net_buf_simple_pull_le16(&comp);
            atserver_send(",%04x,%04x", cid, mod_id);
        }

        atserver_send("\r\n");

    }

    return 0;
}


static void _at_vendor_status_message_process(model_message message)
{
    if(message.ven_data.data_len < 2) {
        return;
    }
    uint8_t *data = (uint8_t*)message.ven_data.user_data;
    uint16_t  status_op = data[0] | data[1] << 8;

    switch(status_op) {
    case ATTR_TYPE_REPORT_VERSION : {
        uint32_t  verison   = (data[2]  & 0x00)| data[3] << 16 | data[4] << 8 | data[5];
        atserver_send("\r\n+MESHNODEVER:%04x,%06x\r\n", message.source_addr, verison);
    }
    break;
    case ATTR_TYPE_OVERWRITE_SUBLIST : {
        uint8_t status = data[2];
        atserver_send("\r\n+MESHSUBLISTOVERWRITE:%04x,%02x\r\n", message.source_addr, status);
    }
    default:
        break;
    }

}



#ifdef CONFIG_BT_MESH_PROVISIONER

static void node_rst_timeout_cb(void *timer, void *arg)
{
    int ret = 0;

    aos_timer_stop(&g_mesh_server.rst_check_timer);

    reset_node   *node_iter_temp = NULL;
    slist_t  *tmp;
    long long time_now = aos_now_ms();

    aos_mutex_lock(&g_mesh_server.rst_list_mutex, AOS_WAIT_FOREVER);
    slist_for_each_entry_safe(&g_mesh_server.rst_node_head,tmp,node_iter_temp,reset_node,list) {
        if(!node_iter_temp) {
            break;
        } else {
            if(node_iter_temp->retry_count && (time_now - node_iter_temp->last_try_time) >= 1000) {
                bt_mesh_cfg_cli_timeout_set(0);
                ret = bt_mesh_cfg_node_reset(0, node_iter_temp->addr);
                if(ret && TIMEOUT1 != ret) {
                    LOGE(TAG,"Rst send fail %d",ret);
                }
                bt_mesh_cfg_cli_timeout_set(2);
                node_iter_temp->retry_count--;
                node_iter_temp->last_try_time = time_now;

            } else if (!node_iter_temp->retry_count) {
                aos_mutex_unlock(&g_mesh_server.rst_list_mutex);
                ret = provisioner_upper_reset_node(node_iter_temp->addr);//the func will call _mesh_rst_retry_node_del,so unlock it first
                if(ret == -ENODEV) {
                    LOGE(TAG,"The dev to rst not found %04x",node_iter_temp->addr);
                    aos_mutex_lock(&g_mesh_server.rst_list_mutex, AOS_WAIT_FOREVER);
                    slist_del(&node_iter_temp->list,&g_mesh_server.rst_node_head);
                    aos_free(node_iter_temp);
                    aos_mutex_unlock(&g_mesh_server.rst_list_mutex);
                    atserver_send("\r\n+MESHRST:%d\r\n", ret);
                } else {
                    LOGE(TAG,"Recv %04x rst status time out,remove it directly",node_iter_temp->addr);
                }
                aos_mutex_lock(&g_mesh_server.rst_list_mutex, AOS_WAIT_FOREVER);
            }
        }
    }

    if(!slist_empty(&g_mesh_server.rst_node_head)) {
        aos_timer_start(&g_mesh_server.rst_check_timer);
    } else {
        g_mesh_server.rst_timer_start_flag = 0;
    }

    aos_mutex_unlock(&g_mesh_server.rst_list_mutex);
}


reset_node* rst_list_node_search(slist_t *list_head,uint16_t unicast_addr)
{
    if(!list_head) {
        return NULL;
    }
    reset_node   *node_iter_temp = NULL;
    slist_t  *tmp;
    aos_mutex_lock(&g_mesh_server.rst_list_mutex, AOS_WAIT_FOREVER);
    slist_for_each_entry_safe(list_head,tmp,node_iter_temp,reset_node,list) {
        if(!node_iter_temp) {
            aos_mutex_unlock(&g_mesh_server.rst_list_mutex);
            return NULL;
        } else {
            if(node_iter_temp->addr == unicast_addr) {
                aos_mutex_unlock(&g_mesh_server.rst_list_mutex);
                return node_iter_temp;
            }
        }
    }
    aos_mutex_unlock(&g_mesh_server.rst_list_mutex);
    return NULL;
}


static void _mesh_rst_retry_timer_init()
{
    aos_timer_new_ext(&g_mesh_server.rst_check_timer, node_rst_timeout_cb, NULL, NODE_RST_CHECK_TIMEOUT, 0,0);
    aos_mutex_new(&g_mesh_server.rst_list_mutex);
    g_mesh_server.rst_node_head.next = NULL;
    g_mesh_server.rst_timer_start_flag = 0;
}


int _mesh_rst_retry_node_add(uint16_t unicast_addr)
{

    if(rst_list_node_search(&g_mesh_server.rst_node_head,unicast_addr)) {
        LOGE(TAG,"Node already exist %04x",unicast_addr);
        return -EALREADY;
    }

    reset_node *node = (reset_node *)aos_malloc(sizeof(reset_node));
    if(!node) {
        LOGE(TAG,"Rst Node malloc faild");
        return -ENOMEM;
    }

    node->addr = unicast_addr;
    node->last_try_time = aos_now_ms();
    node->retry_count = NODE_RST_MAX_RETRY_COUNT;

    aos_mutex_lock(&g_mesh_server.rst_list_mutex, AOS_WAIT_FOREVER);
    slist_add_tail(&node->list,&g_mesh_server.rst_node_head);
    if(!g_mesh_server.rst_timer_start_flag) {
        aos_timer_stop(&g_mesh_server.rst_check_timer);
        aos_timer_start(&g_mesh_server.rst_check_timer);
        g_mesh_server.rst_timer_start_flag = 1;
    }
    aos_mutex_unlock(&g_mesh_server.rst_list_mutex);
    return 0;
}



int _mesh_rst_retry_node_del(uint16_t unicast_addr)
{
    reset_node *node =  NULL;
    node =  rst_list_node_search(&g_mesh_server.rst_node_head,unicast_addr);
    if(!node) {
        LOGE(TAG,"Node not exist %04X",unicast_addr);
        return -ENODEV;
    }
    aos_mutex_lock(&g_mesh_server.rst_list_mutex,AOS_WAIT_FOREVER);
    slist_del(&node->list,&g_mesh_server.rst_node_head);
    aos_free(node);
    aos_mutex_unlock(&g_mesh_server.rst_list_mutex);

    return 0;
}

#endif

void at_model_cb(mesh_model_event_en event, void *p_arg)
{
    int16_t ret  = 0;
    struct net_buf_simple_state state = {0};

    if(!p_arg) {
        return;
    }

    switch (event) {
    case BT_MESH_MODEL_CFG_APPKEY_STATUS: {

        model_message message = *(model_message *)p_arg;
        uint8_t status = message.status_data->data[0];

        if (status) {
            atserver_send("\r\n+MESHAPPKEYADD:%04x,%x\r\n", message.source_addr, status);
        } else {
            atserver_send("\r\n+MESHAPPKEYADD:%04x,%x\r\n", message.source_addr, AT_STATUS_OK);
        }
    }

    break;

    case BT_MESH_MODEL_CFG_APPKEY_BIND_STATUS: {
        model_message message = *(model_message *)p_arg;
        uint8_t status = message.status_data->data[0];
        if (status) {
            atserver_send("\r\n+MESHAPPKEYBIND:%04x,%x\r\n", message.source_addr, status);
        } else {
            atserver_send("\r\n+MESHAPPKEYBIND:%04x,%x\r\n", message.source_addr, AT_STATUS_OK);
        }
    }
    break;

    case BT_MESH_MODEL_CFG_COMP_DATA_STATUS: {
        model_message message = *(model_message *)p_arg;
        struct net_buf_simple *buf = message.status_data;
        if (buf) {
            net_buf_simple_save(buf, &state);
            uint8_t status = net_buf_simple_pull_u8(buf);

            if (!status) {
                ret = print_comp_data(message.source_addr, buf);
            }

            if (ret || status) {
                atserver_send("\r\n+MESHCOMPDATA:%04x,%x\r\nERROR\r\n", message.source_addr, status);
            }
            net_buf_simple_restore(buf, &state);
        }
    }
    break;

    case BT_MESH_MODEL_CFG_SUB_STATUS: {

        model_message message = *(model_message *)p_arg;
        uint8_t status = message.status_data->data[0];

        if (status) {
            atserver_send("\r\n+MESHSUBSET:%04x,%x\r\n", message.source_addr, status);
        } else {
            atserver_send("\r\n+MESHSUBSET:%04x,%x\r\n", message.source_addr, AT_STATUS_OK);
        }

    }
    break;

    case BT_MESH_MODEL_CFG_SUB_LIST: {

        model_message message = *(model_message *)p_arg;
        if(message.status_data) {
            net_buf_simple_save(message.status_data, &state);
            uint8_t status_data = (uint8_t)net_buf_simple_pull_u8(message.status_data);
            uint16_t addr = (uint16_t)net_buf_simple_pull_le16(message.status_data);
            uint16_t mod_id = (uint16_t)net_buf_simple_pull_le16(message.status_data);

            if (!status_data && message.status_data->len >= 2) {
                uint8_t first_addr = 0;

                while (message.status_data->len >= 2) {
                    if (!first_addr) {
                        atserver_send("\r\n+MESHSUBLIST:%04x,%04x,%04x,%04x", addr, CID_NVAL, mod_id, (uint16_t)net_buf_simple_pull_le16(message.status_data));
                        first_addr = 1;
                    } else {
                        atserver_send(",%04x", (uint16_t)net_buf_simple_pull_le16(message.status_data));
                    }
                }

                if (first_addr) {
                    atserver_send("\r\n");
                }

            } else if (status_data) {
                atserver_send("\r\n+MESHSUBLIST:%04x,%x\r\n", addr, status_data);
            } else if (!status_data && message.status_data->len < 2) {
                atserver_send("\r\n+MESHSUBLIST:%04x,%04X,%04x,NULL\r\n", addr, CID_NVAL, mod_id);
            }
            net_buf_simple_restore(message.status_data, &state);
        }

    }
    break;

    case BT_MESH_MODEL_CFG_SUB_LIST_VND: {

        model_message message = *(model_message *)p_arg;
        if(message.status_data) {
            net_buf_simple_save(message.status_data, &state);
            uint8_t status_data = (uint8_t)net_buf_simple_pull_u8(message.status_data);
            uint16_t addr = (uint16_t)net_buf_simple_pull_le16(message.status_data);
            uint16_t company_id = (uint16_t)net_buf_simple_pull_le16(message.status_data);
            uint16_t mod_id = (uint16_t)net_buf_simple_pull_le16(message.status_data);

            if (!status_data && message.status_data->len >= 2) {
                uint8_t first_addr = 0;

                while (message.status_data->len >= 2) {
                    if (!first_addr) {
                        atserver_send("\r\n+MESHSUBLIST:%04x,%04x,%04x,%04x", addr, company_id, mod_id, (uint16_t)net_buf_simple_pull_le16(message.status_data));
                        first_addr = 1;
                    } else {
                        atserver_send(",%04x", (uint16_t)net_buf_simple_pull_le16(message.status_data));
                    }
                }

                if (first_addr) {
                    atserver_send("\r\n");
                }

            } else if (status_data) {
                atserver_send("\r\n+MESHSUBLIST:%d\r\n", status_data);
            } else if (!status_data && message.status_data->len < 2) {
                atserver_send("\r\n+MESHSUBLIST:%04x,%04x,%04x,NULL\r\n", addr, company_id, mod_id);
            }
            net_buf_simple_restore(message.status_data, &state);
        }

    }
    break;

    case BT_MESH_MODEL_CFG_PUB_STATUS: {


        model_message message = *(model_message *)p_arg;
        if(message.status_data) {
            net_buf_simple_save(message.status_data, &state);
            uint8_t status = (uint8_t)net_buf_simple_pull_u8(message.status_data);
            uint16_t elem_addr = (uint16_t)net_buf_simple_pull_le16(message.status_data);

            if (g_mesh_server.get_pub_flag) {
                if (status) {
                    atserver_send("\r\n+MESHPUB:%04x,%x\r\n", message.source_addr, status);
                    net_buf_simple_restore(message.status_data, &state);
                    return;
                }

                if (message.status_data->len < 1) {
                    atserver_send("\r\n+MESHPUB:%04x,NULL\r\n", elem_addr);
                    net_buf_simple_restore(message.status_data, &state);
                    return;
                }

                uint16_t pub_addr = (uint16_t)net_buf_simple_pull_le16(message.status_data);
                net_buf_simple_pull_le16(message.status_data);
                uint8_t ttl = (uint8_t)net_buf_simple_pull_u8(message.status_data);
                uint8_t period = (uint8_t)net_buf_simple_pull_u8(message.status_data);
                uint8_t retransmit = (uint8_t)net_buf_simple_pull_u8(message.status_data);
                uint16_t CID;
                uint16_t mod_id;

                if (message.status_data->len >= 4) {
                    CID = (uint16_t)net_buf_simple_pull_le16(message.status_data);
                    mod_id = (uint16_t)net_buf_simple_pull_le16(message.status_data);
                } else {
                    CID = CID_NVAL;
                    mod_id = (uint16_t)net_buf_simple_pull_le16(message.status_data);
                }

                if (BT_MESH_ADDR_UNASSIGNED == pub_addr) {
                    atserver_send("\r\n+MESHPUB:%04x,%04x,%04x,NULL\r\n", elem_addr, CID, mod_id);
                } else {
                    atserver_send("\r\n+MESHPUB:%04x,%04x,%04x,%04x,%02x,%02x,%02x,%02x\r\n", elem_addr, CID, mod_id, pub_addr, \
                                  ttl, period, BT_MESH_PUB_TRANSMIT_COUNT(retransmit), \
                                  BT_MESH_PUB_TRANSMIT_INT(retransmit));
                }

                g_mesh_server.get_pub_flag = 0;
            } else if (status) {
                atserver_send("\r\n+MESHPUBSET:%04x,%x\r\n", message.source_addr, status);
            } else {
                atserver_send("\r\n+MESHPUBSET:%04x,%x\r\n", message.source_addr, AT_STATUS_OK);
            }
            net_buf_simple_restore(message.status_data, &state);
        }

    }
    break;

    case BT_MESH_MODEL_VENDOR_MESH_AUTOCONFIG_STATUS: {

        model_message *message = (model_message *)p_arg;
        uint8_t *ven_data = (uint8_t *)message->ven_data.user_data;
        uint8_t status = ven_data[1];

        if (0 == status) {
            atserver_send("\r\n+MESHAUTOCONFIG:%04x,%d\r\n", message->source_addr, AT_STATUS_OK);
        } else {
            atserver_send("\r\n+MESHAUTOCONFIG:%04x,%x\r\n", message->source_addr, status);

        }

    }
    break;

    //model status
    case BT_MESH_MODEL_CFG_RELAY_STATUS: {

        model_message relay_message = *(model_message *)p_arg;
        atserver_send("\r\n+MESHRELAY:%04x,%02x\r\n", relay_message.source_addr, relay_message.status_data->data[0]);

    }
    break;

    case BT_MESH_MODEL_CFG_PROXY_STATUS: {

        model_message proxy_message = *(model_message *)p_arg;
        atserver_send("\r\n+MESHPROXY:%04x,%02x\r\n", proxy_message.source_addr, proxy_message.status_data->data[0]);

    }
    break;

    case BT_MESH_MODEL_CFG_FRIEND_STATUS: {

        model_message friend_message = *(model_message *)p_arg;
        atserver_send("\r\n+MESHFRIEND:%04x,%02x\r\n", friend_message.source_addr, friend_message.status_data->data[0]);

    }
    break;

    case BT_MESH_MODEL_CFG_RST_STATUS: {
        model_message rst_message = *(model_message *)p_arg;
        atserver_send("\r\n+MESHRST:%04x\r\n", rst_message.source_addr);
        _mesh_rst_retry_node_del(rst_message.source_addr);
    }
    break;

    case BT_MESH_MODEL_ONOFF_STATUS: {
        model_message onoff_message = *(model_message *)p_arg;
        atserver_send("\r\n+MESHONOFF:%04x,%02x\r\n", onoff_message.source_addr, onoff_message.status_data->data[0]);
    }
    break;

    case BT_MESH_MODEL_LEVEL_STATUS: {

        model_message level_message = *(model_message *)p_arg;
        if(level_message.status_data) {
            net_buf_simple_save(level_message.status_data, &state);
            int16_t level = (int16_t)net_buf_simple_pull_le16(level_message.status_data);
            net_buf_simple_restore(level_message.status_data, &state);
            atserver_send("\r\n+MESHLEVEL:%04x,%04x\r\n", level_message.source_addr, level);
        }

    }
    break;

    case BT_MESH_MODEL_LIGHTNESS_STATUS: {

        model_message lightness_message = *(model_message *)p_arg;
        if(lightness_message.status_data) {
            net_buf_simple_save(lightness_message.status_data, &state);
            int16_t lightness = (int16_t)net_buf_simple_pull_le16(lightness_message.status_data);
            net_buf_simple_restore(lightness_message.status_data, &state);
            atserver_send("\r\n+MESHLIGHTNESS:%04x,%04x\r\n", lightness_message.source_addr, lightness);

        }
        break;

        case BT_MESH_MODEL_LIGHTNESS_LINEAR_STATUS: {

            model_message lightness_lin_message = *(model_message *)p_arg;
            if(lightness_lin_message.status_data) {
                net_buf_simple_save(lightness_lin_message.status_data, &state);
                int16_t lightness_lin = (int16_t)net_buf_simple_pull_le16(lightness_lin_message.status_data);
                net_buf_simple_restore(lightness_lin_message.status_data, &state);
                atserver_send("\r\n+MESHLIGHTNESSLIN:%04x,%04x\r\n", lightness_lin_message.source_addr, lightness_lin);
            }

        }
        break;

        case BT_MESH_MODEL_LIGHTNESS_RANGE_STATUS: {
            model_message lightness_range_message = *(model_message *)p_arg;
            if(lightness_range_message.status_data) {
                net_buf_simple_save(lightness_range_message.status_data, &state);
                uint8_t status_code = (uint8_t)net_buf_simple_pull_u8(lightness_range_message.status_data);
                int16_t range_min = (int16_t)net_buf_simple_pull_le16(lightness_range_message.status_data);
                int16_t range_max = (int16_t)net_buf_simple_pull_le16(lightness_range_message.status_data);
                net_buf_simple_restore(lightness_range_message.status_data, &state);
                atserver_send("\r\n+MESHLIGHTNESSRANGE:%04x,%02x,%04x,%04x\r\n", lightness_range_message.source_addr, status_code, range_min, range_max);
            }
        }
        break;

        case BT_MESH_MODEL_LIGHTNESS_DEF_STATUS: {

            model_message lightness_def_message = *(model_message *)p_arg;
            if(lightness_def_message.status_data) {
                net_buf_simple_save(lightness_def_message.status_data, &state);
                int16_t lightness_def = (int16_t)net_buf_simple_pull_le16(lightness_def_message.status_data);
                net_buf_simple_restore(lightness_def_message.status_data, &state);
                atserver_send("\r\n+MESHLIGHTNESSDEF:%04x,%04x\r\n", lightness_def_message.source_addr, lightness_def);
            }

        }
        break;

        case BT_MESH_MODEL_LIGHTNESS_LAST_STATUS: {

            model_message lightness_last_message = *(model_message *)p_arg;
            if(lightness_last_message.status_data) {
                net_buf_simple_save(lightness_last_message.status_data, &state);
                int16_t lightness_last = (int16_t)net_buf_simple_pull_le16(lightness_last_message.status_data);
                net_buf_simple_restore(lightness_last_message.status_data, &state);
                atserver_send("\r\n+MESHLIGHTNESSLAST:%04x,%04x\r\n", lightness_last_message.source_addr, lightness_last);
            }

        }
        break;

        case BT_MESH_MODEL_LIGHT_CTL_STATUS: {

            model_message light_ctl_message = *(model_message *)p_arg;
            if(light_ctl_message.status_data) {
                net_buf_simple_save(light_ctl_message.status_data, &state);
                int16_t lightness = (int16_t)net_buf_simple_pull_le16(light_ctl_message.status_data);
                int16_t ctl_temp  = (int16_t)net_buf_simple_pull_le16(light_ctl_message.status_data);
                net_buf_simple_restore(light_ctl_message.status_data, &state);
                atserver_send("\r\n+MESHCTL:%04x,%04x,%04x\r\n", light_ctl_message.source_addr, lightness, ctl_temp);
            }

        }
        break;

        case BT_MESH_MODEL_LIGHT_CTL_TEMP_STATUS: {

            model_message light_ctl_temp_message = *(model_message *)p_arg;
            if(light_ctl_temp_message.status_data) {
                net_buf_simple_save(light_ctl_temp_message.status_data, &state);
                int16_t ctl_temp  = (int16_t)net_buf_simple_pull_le16(light_ctl_temp_message.status_data);
                int16_t uv  = (int16_t)net_buf_simple_pull_le16(light_ctl_temp_message.status_data);
                net_buf_simple_restore(light_ctl_temp_message.status_data, &state);
                atserver_send("\r\n+MESHCTLTEMP:%04x,%04x,%04x\r\n", light_ctl_temp_message.source_addr, ctl_temp, uv);
            }

        }
        break;

        case BT_MESH_MODEL_LIGHT_CTL_TEMP_RANGE_STATUS: {

            model_message light_ctl_temp_ran_message = *(model_message *)p_arg;
            if(light_ctl_temp_ran_message.status_data) {
                net_buf_simple_save(light_ctl_temp_ran_message.status_data, &state);
                uint8_t status_code = (uint8_t)net_buf_simple_pull_u8(light_ctl_temp_ran_message.status_data);
                int16_t temp_min  = (int16_t)net_buf_simple_pull_le16(light_ctl_temp_ran_message.status_data);
                int16_t temp_max  = (int16_t)net_buf_simple_pull_le16(light_ctl_temp_ran_message.status_data);
                net_buf_simple_restore(light_ctl_temp_ran_message.status_data, &state);
                atserver_send("\r\n+MESHCTLTEMPRANGE:%04x,%02x,%04x,%04x\r\n", light_ctl_temp_ran_message.source_addr, status_code, temp_min, temp_max);
            }

        }
        break;

        case BT_MESH_MODEL_LIGHT_CTL_DEF_STATUS: {

            model_message light_ctl_def_message = *(model_message *)p_arg;
            if(light_ctl_def_message.status_data) {
                net_buf_simple_save(light_ctl_def_message.status_data, &state);
                int16_t lightness = (int16_t)net_buf_simple_pull_le16(light_ctl_def_message.status_data);
                int16_t ctl_temp  = (int16_t)net_buf_simple_pull_le16(light_ctl_def_message.status_data);
                int16_t uv  = (int16_t)net_buf_simple_pull_le16(light_ctl_def_message.status_data);
                net_buf_simple_restore(light_ctl_def_message.status_data, &state);
                atserver_send("\r\n+MESHCTLDEF:%04x,%04x,%04x,%04x\r\n", light_ctl_def_message.source_addr, lightness, ctl_temp, uv);
            }

        }
        break;

        case BT_MESH_MODEL_VENDOR_MESSAGES: {
            model_message message = *(model_message *)p_arg;
            uint16_t unicast_addr = message.source_addr;
            uint8_t len = message.ven_data.data_len;
            char *data = (char *)message.ven_data.user_data;
            atserver_send("\r\n+MESHTRS:%04x,%02x,%s\r\n", unicast_addr, len, bt_hex_real(data, len));
        }
        break;
        case BT_MESH_MODEL_VENDOR_MESH_STATUS: {
            model_message message = *(model_message *)p_arg;
            if(message.status_data) {
                net_buf_simple_save(message.status_data, &state);
                _at_vendor_status_message_process(message);
                net_buf_simple_restore(message.status_data, &state);
            }

        }
        break;
        case BT_MESH_MODEL_ONOFF_SET:
        case BT_MESH_MODEL_ONOFF_SET_UNACK: {
            model_message message = *(model_message *)p_arg;
            S_ELEM_STATE *elem_state = (S_ELEM_STATE *)message.user_data;
            atserver_send("\r\n+MESHONOFFSET:%02x\r\n", elem_state->state.onoff[T_TAR]);
        }
        break;

        default:
            break;
        }

    }
}


//BASE_CMD


void at_cmd_btmesh_log(char *cmd, int type, char *data)
{
    static uint32_t log_level;
    if (type == WRITE_CMD) {
        int32_t level;
        int16_t ret;
        int16_t input_num;

        input_num = atserver_scanf("%x",&level);
        if(input_num < 1 || level < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = aos_set_log_level(level);
        if(ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }
        log_level = level;
        AT_BACK_OK();

    } else if (type == READ_CMD) {
        AT_BACK_RET_OK_INT(cmd, log_level);
    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<log_level>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}




void at_cmd_btmesh_reboot(char *cmd, int type, char *data)
{

    if (type == EXECUTE_CMD) {
        AT_BACK_OK();
        aos_reboot();
    }
}

void at_cmd_btmesh_at_enable(char *cmd, int type, char *data)
{

    if (type == WRITE_CMD) {
        int32_t onoff;
        int16_t ret;
        int16_t input_num;

        input_num = atserver_scanf("%x",&onoff);
        if(input_num < 1 || onoff > 1 || onoff < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if(onoff) {
            node_at_cb _at_cb = {
                ._prov_cb  = NULL,
                ._model_cb = at_model_cb,
                ._provisioner_cb = at_provisioner_cb,
            };

            ret = ble_mesh_node_at_cb_register(_at_cb);
            if(ret) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_INIT_FAILD));
                return;
            }
            g_mesh_server.init_flag = 1;

#ifdef CONFIG_BT_MESH_PROVISIONER
            static uint8_t rst_timer_init_flag = 0;
            if(!rst_timer_init_flag) {
                _mesh_rst_retry_timer_init();
                rst_timer_init_flag = 1;
            }

#endif
        } else {
            ret = ble_mesh_node_at_cb_unregister();
            if(ret) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_INIT_FAILD));
                return;
            }
            g_mesh_server.init_flag = 0;
        }

        g_mesh_server.provisioner_enabled = ble_mesh_node_provisioner_is_enabled();

        AT_BACK_OK();

    } else if (type == READ_CMD) {
        AT_BACK_RET_OK_INT(cmd, g_mesh_server.init_flag);
    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<onoff>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }

}



void at_cmd_btmesh_query_sta(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t mod_id;
        uint32_t netkey_idx;
        uint32_t appkey_idx;
        uint32_t status_op;
        uint32_t cid;
        int16_t ret;
        int16_t input_num;

        input_num = atserver_scanf("%x,%x,%x,%x,%x,%x", &unicast_addr, &mod_id, &status_op, &netkey_idx, &appkey_idx, &cid);

        if (input_num < 3) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num < 4) {
            netkey_idx = 0;
        }

        if (input_num < 5) {
            appkey_idx = 0;

            if (BT_MESH_MODEL_ID_CFG_CLI == mod_id) {
                appkey_idx = BT_MESH_KEY_DEV;
            }
        }

        if (input_num < 6) {
            cid = CID_NVAL;
        }

        if (CID_NVAL == cid && BT_MESH_MODEL_ID_CFG_CLI == mod_id && BT_MESH_KEY_DEV != appkey_idx) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *model = ble_mesh_model_find(0, mod_id, cid);

        if (!model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_NOT_FOUND_MODEL));
            return;
        }

        ret = ble_mesh_model_status_get(netkey_idx, appkey_idx, unicast_addr, model, status_op);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {

        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>,<query_mod_id>,<query_opcode>,[netkey_idx],[appkey_idx],[cid]");

    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }

}


//for provisioner


void at_cmd_btmesh_prov_config(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        int16_t ret;
        int16_t input_num;
        uint32_t start_addr = 0;
        uint32_t end_addr   = 0;
        provisioner_node_config_t g_provisioner_param;

        input_num = atserver_scanf("%x,%x", &start_addr,&end_addr);

        if (input_num < 2 ) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if(!BT_MESH_ADDR_IS_UNICAST(start_addr) ||  !BT_MESH_ADDR_IS_UNICAST(end_addr)) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        g_provisioner_param.unicast_start_addr = start_addr;
        g_provisioner_param.unicast_end_addr   = end_addr;
        g_provisioner_param.attention_time     = DEF_ATTENTION_TIMEOUT;
        ret = ble_mesh_node_provisioner_config(&g_provisioner_param);
        if(ret) {
            if(ret) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_CONFIG_FAILD));
                return;
            }
        }

        AT_BACK_OK();

    } else if (type == READ_CMD) {
        if (g_mesh_server.role == NODE) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
        } else {
            const ble_mesh_provisioner_t* comp = NULL;
            comp = ble_mesh_provisioner_get_provisioner_data();
            if(!comp) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_GET_CONFIG_FAILD));
            } else {
                const struct bt_mesh_comp *mesh_comp = ble_mesh_model_get_comp_data();

                if (!mesh_comp) {
                    LOGE(TAG, "provisioner config get model comp data faild");
                    return;
                }
                atserver_send("\r\n%s:%04x,%04x\r\n", cmd + 2, comp->provisioner->prov_start_address - mesh_comp->elem_count,comp->provisioner->prov_end_address);
            }
        }
    } else if (type == TEST_CMD) {
        if (g_mesh_server.role == NODE) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
        } else {
            AT_BACK_TEST_RET_OK(cmd, "<start_addr>,<end_addr>");
        }
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_prov_auto_prov(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == EXECUTE_CMD) {
        int16_t ret;
        ret = ble_mesh_node_provisioner_autoprov();
        if(ret < 0 && ret != -EALREADY) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_AUTO_PROV_FAILD));
            return;
        }
        AT_BACK_OK();

    }  else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}



void at_cmd_btmesh_prov_en(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        int16_t ret;
        int16_t input_num;
        int32_t onoff = 0;

        input_num = atserver_scanf("%x", &onoff);

        if (input_num < 1 || onoff > 1 || onoff < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (onoff) {
            ret = ble_mesh_node_provisioner_enable();
            if (ret && ret != -EALREADY) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_ENABLE_FAILD));
                return;
            }

            g_mesh_server.provisioner_enabled = 1;
            g_mesh_server.role = NODE | PROVISIONER;

        } else {
            ret = ble_mesh_node_provisioner_disable();
            if (ret && ret != -EALREADY) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_DISABLE_FAILD));
                return;
            }

            g_mesh_server.provisioner_enabled = 0;
            g_mesh_server.role = NODE;
        }

        AT_BACK_OK();

    } else if (type == READ_CMD) {
        if (g_mesh_server.role == NODE) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
        } else {
            AT_BACK_RET_OK_INT(cmd, g_mesh_server.provisioner_enabled);
        }
    } else if (type == TEST_CMD) {
        if (g_mesh_server.role == NODE) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
        } else {
            AT_BACK_TEST_RET_OK(cmd, "<onoff>");
        }
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }

}


void at_cmd_btmesh_rst_dev(char *cmd, int type, char *data)
{
    int16_t ret = 0;

    if (type == EXECUTE_CMD) {
        ret = ble_mesh_node_reset();
        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_RST_FAILD));
            return;
        } else {
            AT_BACK_OK();
            aos_reboot();
        }
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_rpl_clear(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == EXECUTE_CMD) {
        extern void bt_mesh_rpl_clear_all();
        bt_mesh_rpl_clear_all();
        AT_BACK_OK();
    } else if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        int16_t input_num;
        mesh_node_t *node = NULL;
        input_num = atserver_scanf("%x", &unicast_addr);

        if (input_num < 1 || unicast_addr < DEF_PROVISIONER_UNICAST_ADDR_START) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        extern mesh_node_t *ble_mesh_provisioner_get_node_info(u16_t unicast_addr);
        node = ble_mesh_provisioner_get_node_info(unicast_addr);

        if (!node) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_GET_NODE_INFO_FAILD));
            return;
        }
        extern void bt_mesh_rpl_clear_node(uint16_t unicast_addr, uint8_t elem_num);
        bt_mesh_rpl_clear_node(node->prim_unicast, node->element_num);
        AT_BACK_OK();

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "[unicast_addr]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }

}

void at_cmd_btmesh_prov_show_dev(char *cmd, int type, char *data)
{
    static uint8_t show_dev_flag = 0;
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {
        int16_t input_num;
        int32_t show_dev;
        uint32_t timeout;
        int16_t ret;
        input_num = atserver_scanf("%x,%x", &show_dev, &timeout);

        if (input_num < 1 || show_dev > 1 || show_dev < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num < 2) {
            timeout = DEF_FOUND_DEV_TIMEOUT;
        }

        ret = ble_mesh_provisioner_show_dev(show_dev, timeout);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_SHOW_DEV_FAILD));
            return;
        }

        show_dev_flag = show_dev;

        AT_BACK_OK();

    } else if (type == READ_CMD) {
        if (g_mesh_server.role == NODE) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
        } else {
            AT_BACK_RET_OK_INT(cmd,show_dev_flag);
        }
    } else if (type == TEST_CMD) {
        if (g_mesh_server.role == NODE) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
        } else {
            AT_BACK_TEST_RET_OK(cmd, "<onoff>,[timeout]");
        }
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }

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
        return ERR_CHAR;
    }
}

static int str2bt_addr(const char *str, uint8_t addr[6])
{
    uint8_t i, j;
    uint8_t tmp;

    if (strlen(str) != 17 || !addr) {
        return -EINVAL;
    }

    for (i = 5, j = 1; *str != '\0' ; str++, j++) {
        if (!(j % 3) && (*str != ':')) {
            return -EINVAL;
        } else if (*str == ':') {
            i--;
            continue;
        }

        addr[i] = addr[i] << 4;

        tmp = char2u8(*str);

        if (tmp == ERR_CHAR) {
            return -EINVAL;
        }

        addr[i] |= tmp;
    }

    return 0;
}

static int str2hex(uint8_t hex[], char *s, uint8_t cnt)
{
    uint8_t i;

    if (!s) {
        return 0;
    }

    for (i = 0; (*s != '\0') && (i < cnt); i++, s += 2) {
        hex[i] = ((char2u8(*s) & 0x0f) << 4) | ((char2u8(*(s + 1))) & 0x0f);
    }

    return i;
}

void at_cmd_btmesh_prov_add_dev(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {
        int16_t ret;
        uint8_t dev_addr_str[20] = {0};
        uint8_t uuid_str[33] = {0};
        mesh_node_t node = {0};
        uint32_t addr_type;
        uint32_t oob_info;
        uint32_t bearer;
        uint32_t auto_add_appkey;
        int16_t input_num;
        input_num = atserver_scanf("%19[^,],%x,%32[^,],%x,%x,%x", dev_addr_str, &addr_type, uuid_str, &oob_info, &bearer, &auto_add_appkey);

        if (input_num < 5) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        node.bearer = bearer;
        node.oob_info = oob_info;
        node.addr_type = addr_type;


        if (input_num < 6) {
            auto_add_appkey = DEF_AUTO_ADD_APPKEY_ONOFF;
        }


        if (node.addr_type != DEV_ADDR_LE_RANDOM && node.addr_type != DEV_ADDR_LE_PUBLIC) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (node.bearer != BT_MESH_PROV_ADV && node.bearer != BT_MESH_PROV_GATT) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = str2bt_addr((char *)dev_addr_str, node.dev_addr);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (str2hex(node.uuid, (char *)uuid_str, sizeof(uuid_str)) < 1) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = ble_mesh_provisioner_dev_add(&node, auto_add_appkey);

        if (ret < 0 && ret != -EALREADY) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_ADD_DEV_FAILD));
        } else {
            AT_BACK_OK();
        }

        g_mesh_server.oob_info.oob_type = OOB_NULL;

    } else if (type == TEST_CMD) {
        if (g_mesh_server.role == NODE) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
        } else {
            AT_BACK_TEST_RET_OK(cmd, "<addr>,<addr_type>,<uuid>,<oob_info>,<bearer>,[auto_add_appkey]");
        }

    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_prov_node_auto_config(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {
        int16_t ret;
        uint32_t  addr;
        uint32_t sub_addr;
        uint32_t timeout;
        uint32_t retry;
        model_auto_config_t auto_config;
        int16_t input_num;

        input_num = atserver_scanf("%x,%x,%x,%x", &addr, &sub_addr, &timeout, &retry);

        if (input_num < 1) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num < 2) {
            sub_addr = AUTOCONFIG_DST_SUB_ADDR;
        }

        if (input_num < 3) {
            timeout  = DEF_AUTO_CONFIG_TIMEOUT;
        }

        if (input_num < 4) {
            if (BT_MESH_ADDR_IS_UNICAST(addr)) {
                retry = DEF_UNICAST_ADDR_AUTO_CONFIG_RETRY;
            } else {
                retry = DEF_GROUP_ADDR_AUTO_CONFIG_RETRY;
            }
        }

        if (timeout < DEF_AUTO_CONFIG_TIMEOUT_MIN) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (!BT_MESH_ADDR_IS_GROUP(sub_addr)) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }
        auto_config.sub_addr = sub_addr;

        ret = ble_mesh_node_model_autoconfig(0, 0, addr, auto_config);

        if (ret < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();

        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,[sub_addr],[timeout],[retry]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

static int version_char_to_u32(char* ver_str,unsigned int *version)
{
    int v1 = 0,v2 = 0,v3 = 0;
    if(*ver_str == 'v') {
        sscanf(ver_str, "v%d.%d.%d", &v1,&v2, &v3);
    } else {
        sscanf(ver_str, "%d.%d.%d", &v1,&v2, &v3);
    }
    *version = (v1 << 16 ) | (v2 << 8) | (v3);
    return 0;
}


extern char *aos_get_app_version(void);

unsigned int _version_appver_get(void)
{
    unsigned int version = 0;
    int ret = 0;
    char* version_str = aos_get_app_version();
    if(!version_str) {
        LOGE(TAG,"get version faild\r\n");
        return 0;
    }
    ret = version_char_to_u32(version_str,&version);
    if(ret < 0) {
        LOGE(TAG,"trans version faild %d\r\n",ret);
        return 0;
    }
    return version;
}


void at_cmd_btmesh_get_node_version(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {
        int16_t ret;
        uint32_t  addr;
        int16_t input_num;
        uint16_t prov_start_addr = 0;
        uint8_t  prov_elem_num   = 0;
        uint32_t prov_version = 0;

        input_num = atserver_scanf("%x", &addr);

        if (input_num < 1) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = bt_mesh_provisioner_get_own_unicast_addr(&prov_start_addr,&prov_elem_num);
        if(ret < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
            return;
        }

        if(addr >= prov_start_addr && addr < prov_start_addr + prov_elem_num) {
            prov_version = _version_appver_get();
            if(!prov_version) {
                LOGE(TAG,"get version faild");
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_GET_VERSION_FAIL));
            } else {
                AT_BACK_OK();
                atserver_send("\r\n+MESHNODEVER:%04x,%06x\r\n", addr, prov_version);
            }
            return;
        }

        ret = ble_mesh_node_get_node_app_version(0, 0, addr);
        if (ret < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_GET_VERSION_FAIL));
        } else {
            AT_BACK_OK();

        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_prov_del_dev(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {
        int16_t ret  = 0;
        uint8_t dev_addr_str[20] = {0};
        uint8_t uuid_str[33] = {0};
        uint8_t dev_addr[6] = {0};
        uint8_t uuid[16]    = {0};
        uint32_t addr_type  = 0;
        int16_t input_num;

        input_num = atserver_scanf("%19[^,],%x,%32[^,]", dev_addr_str, &addr_type, uuid_str);

        if (input_num < 3) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (addr_type != DEV_ADDR_LE_RANDOM && addr_type != DEV_ADDR_LE_PUBLIC) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = str2bt_addr((char *)dev_addr_str, dev_addr);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (str2hex(uuid, (char *)uuid_str, sizeof(uuid_str)) < 1) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = ble_mesh_provisioner_dev_del(dev_addr, addr_type, uuid);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_DEL_DEV_FAILD));
        } else {
            AT_BACK_OK();
        }


    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<addr_type>,<uuid>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

// fix me

void at_cmd_btmesh_prov_add_oob(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {

        uint8_t oob_data[33] = {0};
        uint8_t oob_hex[16] = {0};
        uint32_t oob_num;
        int16_t input_num;
        int16_t ret;

        if (OOB_NUM == g_mesh_server.oob_info.oob_type) {
            input_num = atserver_scanf("%d", &oob_num);

            if (input_num < 1) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
                return;
            }

            ret = ble_mesh_provisioner_OOB_input_num(oob_num);

        } else if (OOB_STR == g_mesh_server.oob_info.oob_type) {
            input_num = atserver_scanf("%[^]", oob_data);

            if (input_num < 1 || g_mesh_server.oob_info.oob_length != strlen((char *)oob_data)) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
                return;
            }

            ret = ble_mesh_provisioner_OOB_input_string((char *)oob_data);
        } else if (OOB_STATIC == g_mesh_server.oob_info.oob_type) {
            input_num = atserver_scanf("%[^]", oob_data);

            if (input_num < 1) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
                return;
            } else {
                int oob_len = str2hex(oob_hex, (char *)oob_data, sizeof(oob_hex));
                if (oob_len < 1) {
                    AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
                    return;
                }
                ret = ble_mesh_provisioner_static_OOB_set(oob_hex, oob_len);
            }

        } else {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_OOB_NOT_NEED));
            return;
        }

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_INPUT_OOB_FAILD));
            return;
        }

        AT_BACK_OK();

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<oob>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

#if defined(CONFIG_OTA_SERVER)
void at_cmd_btmesh_add_ota_dev(char *cmd, int type, char *data)
{
#if 0
    if (type == WRITE_CMD) {
        uint8_t dev_addr_str[20];
        uint32_t addr_type;
        uint32_t addr;
        uint32_t size;
        uint32_t version;
        int16_t input_num;
        int16_t ret = 0;
        upgrade_device device;

        input_num = atserver_scanf("%[^,],%x,%x,%x,%x", dev_addr_str, &addr_type, &addr, &size, &version);

        if(input_num  < 5) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (addr_type != DEV_ADDR_LE_RANDOM && addr_type != DEV_ADDR_LE_PUBLIC) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = str2bt_addr((char *)dev_addr_str, device.addr.val);
        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        device.addr.type = addr_type;
        device.info.address = addr;
        device.info.image_type = 0x00;
        device.info.size  = size;
        device.info.version = version;
        ret = ota_server_calu_image_crc(&device);
        if(ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }
        ret = ota_server_update_device_add(&device);
        if(ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }
        AT_BACK_OK();

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<addr_type>,<start_addr>,<size>,<version>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
#endif
}
#endif


void at_cmd_btmesh_prov_netkey_set(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }


    if (type == WRITE_CMD) {
        uint32_t netkey_idx = 0;
        uint8_t net_key_str[33] = {0};
        uint8_t net_key[16];
        int16_t input_num= 0;
        int16_t ret;
        input_num = atserver_scanf("%x,%33[^\n]",&netkey_idx, net_key_str);
        if (input_num < 1) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (1 == input_num) {
            do {
                ret = bt_mesh_provisioner_local_net_key_add(NULL,(uint16_t *)&netkey_idx);
            } while (-EEXIST == ret && netkey_idx++ < CONFIG_BT_MESH_SUBNET_COUNT);
        } else {
            if(strlen((const char *)net_key_str) != 32) {

                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
                return;
            }
            if (str2hex(net_key, (char *)net_key_str, sizeof(net_key_str)) < 1) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
                return;
            }

            ret = bt_mesh_provisioner_local_net_key_add(net_key, (uint16_t *)&netkey_idx);

            if (-EEXIST == ret) {
                ret = bt_mesh_provisioner_local_net_key_delete(netkey_idx);
                if (ret) {
                    AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_ADD_NETKEY_FAILD));
                    return;
                } else {
                    ret = bt_mesh_provisioner_local_net_key_add(net_key, (uint16_t *)&netkey_idx);
                }
            }

        }


        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_ADD_NETKEY_FAILD));
            return;
        } else {
            AT_BACK_RET_OK_INT(cmd, netkey_idx);
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<netkey_idx>,<key_val>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_prov_netkey_get(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }


    if (type == WRITE_CMD) {
        uint32_t netkey_idx;
        const uint8_t *key_val;
        int16_t input_num;
        input_num = atserver_scanf("%x", &netkey_idx);

        if (input_num < 1) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        key_val = bt_mesh_provisioner_local_net_key_get(netkey_idx);

        if (!key_val) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_GET_NETKEY_FAILD));
            return;
        }

        atserver_send("\r\n+MESHGETNETKEY:%02x,%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\r\n", netkey_idx, key_val[0], key_val[1], key_val[2], \
                      key_val[3], key_val[4], key_val[5], key_val[6], key_val[7], key_val[8], \
                      key_val[9], key_val[10], key_val[11], key_val[12], key_val[13], key_val[14], key_val[15]);
        AT_BACK_OK();

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<netkey_idx>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_prov_appkey_set(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }


    if (type == WRITE_CMD) {
        uint32_t appkey_idx = 0;
        uint32_t netkey_idx = 0;
        uint8_t app_key_str[33];
        uint8_t app_key[16];
        int16_t input_num = 0;
        int16_t ret  = 0;

        input_num = atserver_scanf("%x,%x,%33[^\n]",&netkey_idx,&appkey_idx, app_key_str);

        if (input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (2 == input_num) {
            do {
                ret = bt_mesh_provisioner_local_app_key_add(NULL, netkey_idx, (uint16_t *)&appkey_idx);
            } while (-EEXIST == ret && appkey_idx++ < CONFIG_BT_MESH_APP_KEY_COUNT);
        } else {
            if(strlen((const char *)app_key_str) != 32) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
                return;
            }
            if (str2hex(app_key, (char *)app_key_str, sizeof(app_key_str)) < 1) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
                return;
            }

            ret = bt_mesh_provisioner_local_app_key_add(app_key, netkey_idx, (uint16_t *)&appkey_idx);

            if (-EEXIST == ret) {
                ret = bt_mesh_provisioner_local_app_key_delete(netkey_idx, appkey_idx);

                if (ret) {
                    AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_ADD_APPKEY_FAILD));
                    return;
                } else {
                    ret = bt_mesh_provisioner_local_app_key_add(app_key, netkey_idx, (uint16_t *)&appkey_idx);
                }
            }
        }

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_ADD_APPKEY_FAILD));
        } else {
            AT_BACK_RET_OK_INT2(cmd, netkey_idx,appkey_idx);
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<netkey_idx>,<appkey_idx>,[key_val]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_prov_appkey_get(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }


    if (type == WRITE_CMD) {
        uint32_t netkey_idx;
        uint32_t appkey_idx;
        const uint8_t *key_val;
        int16_t input_num;
        input_num = atserver_scanf("%x,%x", &netkey_idx, &appkey_idx);

        if (input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        key_val = bt_mesh_provisioner_local_app_key_get(netkey_idx, appkey_idx);

        if (!key_val) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_GET_APPKEY_FAILD));
            return;
        }

        atserver_send("\r\n+MESHGETAPPKEY:%02x,%02x,%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\r\n", netkey_idx, appkey_idx, key_val[0], key_val[1], key_val[2], \
                      key_val[3], key_val[4], key_val[5], key_val[6], key_val[7], key_val[8], \
                      key_val[9], key_val[10], key_val[11], key_val[12], key_val[13], key_val[14], key_val[15]);

        AT_BACK_OK();
    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<netkey_idx>,<appkey_idx>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

//static uint8_t mac_str[] = {0};

void at_cmd_btmesh_prov_set_uuid_filter(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {
        char uuid_str[33];
        uint8_t uuid_hex[16];
        int32_t onoff;
        uint32_t filter_start;
        uuid_filter_t filter;
        int16_t input_num;
        int16_t ret;
        input_num = atserver_scanf("%x,%32[^,],%x", &onoff, uuid_str, &filter_start);

        if (input_num < 1 || onoff < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (!onoff) {
            ret = ble_mesh_provisioner_dev_filter(onoff, NULL);
        } else {
            if (3 != input_num) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
                return;
            }

            uint8_t str_len = strlen((char *)uuid_str);

            if (str_len < 1 || str_len > 32 || str_len % 2 != 0) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
                return;
            }

            str2hex(uuid_hex, (char *)uuid_str, 16);

            filter.uuid = uuid_hex;
            filter.uuid_length = (str_len + 1) / 2;
            filter.filter_start = filter_start;
            ret = ble_mesh_provisioner_dev_filter(onoff, &filter);
        }

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_SET_UUID_FILTER_FAILD));
            return;
        }

        AT_BACK_OK();

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<onoff>,[uuid_str],[filter_start]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_prov_get_node_info(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        int16_t input_num;
        mesh_node_t *node;
        uint8_t dev_addr_str[20];

        input_num = atserver_scanf("%x", &unicast_addr);

        if (input_num < 1) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (unicast_addr > 0x8000) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        node = ble_mesh_provisioner_get_node_info(unicast_addr);

        if (!node) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_GET_NODE_INFO_FAILD));
            return;
        } else {
            bt_addr_val_to_str(node->dev_addr, (char *)dev_addr_str, sizeof(dev_addr_str));

            if (node->node_name && strlen((char *)node->node_name)) {
                atserver_send("\r\n+MESHNODEINFO:%04x,%s", unicast_addr, node->node_name);
            } else {
                atserver_send("\r\n+MESHNODEINFO:%04x,NULL", unicast_addr);
            }

            atserver_send(",%s,%02x", dev_addr_str, node->addr_type);
            atserver_send(",%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", node->uuid[0], node->uuid[1], node->uuid[2], node->uuid[3], \
                          node->uuid[4], node->uuid[5], node->uuid[6], node->uuid[7], node->uuid[8], node->uuid[9], node->uuid[10], \
                          node->uuid[11], node->uuid[12], node->uuid[13], node->uuid[14], node->uuid[15]);
            atserver_send(",%04x,%02x,%02x,%02x,%08x", node->net_idx, node->element_num, node->oob_info, node->flags, node->iv_index);
            const uint8_t *key_val;
            key_val = provisioner_get_device_key(unicast_addr);

            if (!key_val) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_GET_DEVKEY_FAILD));
                return;
            }

            atserver_send(",%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\r\n", key_val[0], key_val[1], key_val[2], \
                          key_val[3], key_val[4], key_val[5], key_val[6], key_val[7], key_val[8], \
                          key_val[9], key_val[10], key_val[11], key_val[12], key_val[13], key_val[14], key_val[15]);
        }

        AT_BACK_OK();
    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_prov_set_mac_filter(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {
        int32_t onoff;

        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x", &onoff);
        (void)input_num;
        if(onoff > 1 || onoff < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
        }

        if(onoff) {
            ret = ble_mesh_provisioner_mac_filter_enable();
            if (ret < 0 && ret != -EALREADY) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_ENABLE_MAC_FILTER_FAILD));
                return;
            }
        } else {
            ret = ble_mesh_provisioner_mac_filter_disable();
            if (ret < 0 && ret != -EALREADY) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_DISABLE_MAC_FILTER_FAILD));
                return;
            }
        }

        AT_BACK_OK();

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<onoff>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


static int str2bt_dev_addr(const char *str, dev_addr_t* dev)
{
    uint8_t i, j;
    uint8_t tmp;
    uint8_t val_null[6] = {0x00};

    if (strlen(str) != 19 || !dev) {
        return -EINVAL;
    }

    for (i = 5, j = 1; j <= 17 ; str++, j++) {
        if (!(j % 3) && (*str != ':')) {
            return -EINVAL;
        } else if (*str == ':') {
            i--;
            continue;
        }

        dev->val[i] = dev->val[i] << 4;

        tmp = char2u8(*str);
        if (tmp == ERR_CHAR) {
            return -EINVAL;
        }

        dev->val[i] |= tmp;

    }

    if(!memcmp(dev->val,val_null,6)) {
        return -EINVAL;
    }


    str++;

    dev->type = char2u8(*str);

    if(dev->type != 0 &&  dev->type != 1) {
        return -EINVAL;
    }
#if 0
    if((dev->val[5] & 0xC0) == 0xC0 && dev->type == 0  || \
       (dev->val[5] & 0xC0) != 0xC0 && dev->type != 0 ) {
        return -EINVAL;
    }
#endif
    return 0;
}


static int grep_mac_addr_from_str(char* mac_str,dev_addr_t* devices,uint8_t size)
{
    uint8_t dev_index = 0;
    char mac_str_temp[20] = {0x0};
    //uint16_t str_size = strlen(mac_str);

    int ret = 0;
    for(dev_index = 0 ; dev_index < size && *mac_str != '\0'; dev_index++) {
        memcpy(mac_str_temp,mac_str,19);
        ret = str2bt_dev_addr(mac_str_temp, &devices[dev_index]);
        if(ret) {
            return -1;
        }
        mac_str += 20;
    }
    if(dev_index != size) {
        return -1;
    } else {
        return 0;
    }
}




void at_cmd_btmesh_prov_add_mac_filter_dev(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {
        int32_t size;
        int16_t input_num;
        dev_addr_t *device = NULL;
        int16_t ret;

        input_num = atserver_scanf("%x,%"LENGTH_MAC_FILTER_ADDR(MAC_FILTER_ADDR_STR_LENGTH)"[^\n]", &size,dev_addr_str);
        if(input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if(size > MAX_MAC_FILTER_SIZE || !size || size < 0 ) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if(strlen((const char *)dev_addr_str) != (19 * size + size -1)) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        device = (dev_addr_t*)aos_zalloc(size * sizeof(dev_addr_t));
        if(!device) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_ADD_MAC_FILTER_DEV_FAILD));
            return;
        }

        ret = grep_mac_addr_from_str((char *)dev_addr_str,device,size);
        if(ret < 0 ) {
            aos_free(device);
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = ble_mesh_provisioner_mac_filter_dev_add(size,device);
        if(ret < 0) {
            aos_free(device);
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_ADD_MAC_FILTER_DEV_FAILD));
            return;
        }
        aos_free(device);
        AT_BACK_OK();

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<size>,<addr>,<addr_type>,...");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_prov_rm_mac_filter_dev(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {
        int32_t size;
        int16_t input_num;
        dev_addr_t *device = NULL;
        int16_t ret;

        input_num = atserver_scanf("%x,%"LENGTH_MAC_FILTER_ADDR(MAC_FILTER_ADDR_STR_LENGTH)"[^\n]", &size,dev_addr_str);
        if(input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }


        if(size > MAX_MAC_FILTER_SIZE || !size || size < 0 ) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if(strlen((const char *)dev_addr_str) != (19 * size + size -1)) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        device = (dev_addr_t*)aos_zalloc(size * sizeof(dev_addr_t));
        if(!device) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_DEL_MAC_FILTER_DEV_FAILD));
            return;
        }

        ret = grep_mac_addr_from_str((char *)dev_addr_str,device,size);
        if(ret < 0 ) {
            aos_free(device);
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = ble_mesh_provisioner_mac_filter_dev_rm(size,device);
        if(ret < 0) {
            aos_free(device);
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_DEL_MAC_FILTER_DEV_FAILD));
            return;
        }
        aos_free(device);
        AT_BACK_OK();

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<size>,<addr>,<addr_type>,...");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_prov_clear_mac_filter(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == EXECUTE_CMD) {
        int ret = 0;
        ret = ble_mesh_provisioner_mac_filter_clear();
        if (ret < 0 ) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CLR_MAC_FILTER_DEV_FAILD));
            return;
        }

        AT_BACK_OK();

    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}



void at_cmd_btmesh_prov_add_node(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {

        struct node_info node;
        uint8_t dev_key_str[33];
        uint8_t node_name[33];
        uint8_t dev_key[16];
        uint8_t dev_addr_str[20] = {0};
        uint8_t uuid_str[33] = {0};
        uint32_t oob_info;
        uint32_t elem_num;
        uint32_t unicast_addr;
        uint32_t net_idx;
        uint32_t flags;
        uint32_t iv_index;
        uint32_t addr_type;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%32[^,],%19[^,],%x,%32[^,],%x,%x,%x,%x,%x,%32[^,]", &unicast_addr,node_name,dev_addr_str, &addr_type, uuid_str,&net_idx, &elem_num, &oob_info,  &flags, &iv_index,dev_key_str);

        if (input_num < 11) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = str2bt_addr((char *)dev_addr_str, node.addr.a.val);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (addr_type != DEV_ADDR_LE_RANDOM && addr_type != DEV_ADDR_LE_PUBLIC) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        node.addr.type = addr_type;

        if (str2hex(node.uuid, (char *)uuid_str, sizeof(uuid_str)) < 1) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (str2hex(dev_key, (char *)dev_key_str, sizeof(dev_key_str)) < 1) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        node.net_idx = net_idx;
        node.unicast_addr = unicast_addr;
        node.element_num = elem_num;
        node.oob_info = oob_info;
        node.flags = flags;
        node.iv_index = iv_index;

        ret =  bt_mesh_provisioner_add_node(&node, dev_key);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_ADD_NODE_FAILD));
            return;
        }

        AT_BACK_OK();
    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>,<node_name>,<addr>,<addr_type>,<uuid>,<net_idx>,<element_num>,<oob_info>,<flags>,<iv_index>,<dev_key>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

#ifdef CONFIG_MESH_LPM
void at_cmd_btmesh_prov_set_node_lpm_flag(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (!g_mesh_server.provisioner_enabled) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_NOT_ENABLED));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t lpm_flag;
        uint32_t unicast_addr;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x", &unicast_addr,&lpm_flag);

        if (input_num < 2 || (lpm_flag != 0 && lpm_flag != 1)) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = bt_mesh_provisioner_set_node_lpm_flag(unicast_addr,lpm_flag);
        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_PROVISIONER_SET_LPM_FLAG_FAIL));
            return;
        }

        AT_BACK_OK();
    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>,<flag>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}
#endif

/*-----------------------------------------------------------
config cli cmd api
-----------------------------------------------------------*/

void at_cmd_btmesh_get_comp(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t comp_page;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x", &unicast_addr, &comp_page);

        if (input_num < 1) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num < 2) {
            comp_page = 0;
        }

        ret = bt_mesh_cfg_comp_data_get(0, unicast_addr, comp_page, NULL, NULL);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
            return;
        }

        AT_BACK_OK();

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>,[page]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }

}




void at_cmd_btmesh_appkey_add(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t appkey_idx;
        int input_num;
        int ret;

        input_num = atserver_scanf("%x,%x", &unicast_addr, &appkey_idx);

        if (input_num != 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = ble_mesh_node_appkey_add(0, appkey_idx, unicast_addr);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }
    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>,<appkey_idx>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }

}


void at_cmd_btmesh_appkey_bind(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t appkey_idx;
        uint32_t unicast_addr;
        uint32_t mod_id;
        uint32_t cid;
        int input_num;
        int ret;

        input_num = atserver_scanf("%x,%x,%x,%x", &unicast_addr, &appkey_idx, &mod_id, &cid);

        if (input_num < 3) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num == 3) {
            ret = bt_mesh_cfg_mod_app_bind(0, unicast_addr, unicast_addr, appkey_idx, mod_id, NULL);
        } else {
            ret = bt_mesh_cfg_mod_app_bind_vnd(0, unicast_addr, unicast_addr, appkey_idx, mod_id, cid, NULL);
        }

        if (ret && TIMEOUT != ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>,<appkey_idx>,<mod_id>,[cid]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }

}

void at_cmd_btmesh_appkey_unbind(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t appkey_idx;
        uint32_t unicast_addr;
        uint32_t mod_id;
        uint32_t cid;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x", &unicast_addr, &appkey_idx, &mod_id, &cid);

        if (input_num < 3) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num == 3) {
            ret = bt_mesh_cfg_mod_app_unbind(0, unicast_addr, unicast_addr, appkey_idx, mod_id, NULL);
        } else {
            ret = bt_mesh_cfg_mod_app_unbind_vnd(0, unicast_addr, unicast_addr, appkey_idx, mod_id, cid, NULL);
        }

        if (ret && TIMEOUT != ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>,<appkey_idx>,<mod_id>,[cid]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }

}



void at_cmd_btmesh_relay(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        int32_t onoff;
        uint32_t count;
        uint32_t interval;
        uint16_t new_transmit;
        uint8_t transmit;
        int16_t input_num;
        int16_t ret;
        input_num = atserver_scanf("%x,%x,%x,%x", &unicast_addr, &onoff, &count, &interval);

        if (input_num < 2 || onoff < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num < 3) {
            count = 2;
        }

        if (input_num < 4) {
            interval = 20;
        }

        new_transmit = BT_MESH_TRANSMIT(count, interval);

        ret = bt_mesh_cfg_relay_set(0, unicast_addr, onoff,
                                    new_transmit, NULL, &transmit);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
            return;
        }

        AT_BACK_OK();

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>,<onoff>,[count],[interval]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_proxy(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }


    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        int32_t onoff;
        int16_t input_num;
        int16_t ret;
        input_num = atserver_scanf("%x,%x", &unicast_addr, &onoff);

        if (input_num < 2 || onoff < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = bt_mesh_cfg_gatt_proxy_set(0, unicast_addr, onoff, NULL);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
            return;
        }

        AT_BACK_OK();

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>,<onoff>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_friend(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }


    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        int32_t onoff;
        int16_t input_num;
        int16_t ret;
        input_num = atserver_scanf("%x,%x", &unicast_addr, &onoff);

        if (input_num < 2 || onoff < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = bt_mesh_cfg_friend_set(0, unicast_addr, onoff, NULL);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
            return;
        }

        AT_BACK_OK();
    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>,<onoff>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_pub(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t mod_id;
        struct bt_mesh_cfg_mod_pub pub;
        uint32_t count;
        uint32_t ttl;
        uint32_t period;
        uint32_t cid;
        uint32_t interval;
        uint32_t pub_addr;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x,%x,%x,%x,%x,%x", &unicast_addr, &mod_id, &pub_addr, &ttl, &period, &count, &interval, &appkey_idx, &cid);

        if (input_num < 3) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num < 4) {
            ttl = DEF_PUB_SET_TTL;
        }

        if (input_num < 5) {
            period = DEF_PUB_SET_PERIOD;
        }

        if (input_num < 6) {
            count = DEF_PUB_SET_COUNT;
        }

        if (input_num < 7) {
            interval = DEF_PUB_SET_INTERVAlL;
        }

        if (input_num < 8) {
            appkey_idx = DEF_PUB_SET_APPKEY_IDX;
        }

        if (interval > (31 * 50) || (interval % 50)) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        pub.ttl = ttl;
        pub.period = period;
        pub.addr = pub_addr;
        pub.app_idx = appkey_idx;
        pub.transmit = BT_MESH_PUB_TRANSMIT(count, interval);

        if (input_num == 9) {
            ret = bt_mesh_cfg_mod_pub_set_vnd(0, unicast_addr, unicast_addr, mod_id, cid, &pub, NULL);
        } else {
            ret = bt_mesh_cfg_mod_pub_set(0, unicast_addr, unicast_addr, mod_id, &pub, NULL);
        }

        if (ret && TIMEOUT != ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_ADD_PUB_ADDR_FAILD));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicat_addr>,<mod_id>,<pub_addr>,[ttl],[period],[count],[interval],[appkey_idx],[cid]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}




void at_cmd_btmesh_sub(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t sub_addr;
        uint32_t mod_id;
        uint32_t cid;
        int input_num;
        int ret;

        input_num = atserver_scanf("%x,%x,%x,%X", &unicast_addr, &mod_id, &sub_addr, &cid);

        if (input_num < 3) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num == 3) {
            ret = bt_mesh_cfg_mod_sub_add(0, unicast_addr, unicast_addr, sub_addr, mod_id, NULL);
        } else {
            ret = bt_mesh_cfg_mod_sub_add_vnd(0, unicast_addr, unicast_addr, sub_addr, mod_id, cid, NULL);
        }


        if (ret && TIMEOUT != ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_ADD_SUB_ADDR_FAILD));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicat_addr>,<mod_id>,<sub_addr>,[cid]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_sub_del(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t sub_addr;
        uint32_t mod_id;
        uint32_t cid;
        int input_num;
        int ret;

        input_num = atserver_scanf("%x,%x,%x,%X", &unicast_addr, &mod_id, &sub_addr, &cid);

        if (input_num < 3) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num == 3) {
            if(!sub_addr) {
                ret = bt_mesh_cfg_mod_sub_del_all(0, unicast_addr, unicast_addr, sub_addr, mod_id, NULL);
            } else {
                ret = bt_mesh_cfg_mod_sub_del(0, unicast_addr, unicast_addr, sub_addr, mod_id, NULL);
            }

        } else {
            if(!sub_addr) {
                ret = bt_mesh_cfg_mod_sub_del_vnd_all(0, unicast_addr, unicast_addr, sub_addr, mod_id, cid, NULL);
            } else {
                ret = bt_mesh_cfg_mod_sub_del_vnd(0, unicast_addr, unicast_addr, sub_addr, mod_id, cid, NULL);
            }
        }

        if (ret && TIMEOUT != ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_DEL_SUB_ADDR_FAILD));
        } else {

            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicat_addr>,<mod_id>,<sub_addr>,[cid]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

static int atohex(char ch)
{
    int hex;

    hex = -1;

    if ((ch >= '0') && (ch <= '9')) {
        hex = ch - '0';
    } else if ((ch >= 'a') && (ch <= 'f')) {
        hex = ch - 'a' + 0xa;
    } else if ((ch >= 'A') && (ch <= 'F')) {
        hex = ch - 'A' + 0xa;
    }

    return hex;
}

static int strtohex16(uint16_t *hex, char *str)
{
    int n;
    int i;
    uint16_t dd;

    n = -1;
    i = 0;
    dd = 0;

    while(*str) {
        n = atohex(*str++);
        if (n < 0) {
            return -1;
        }
        dd = (dd<<4) + n;
        if (++i > 4) {
            return -1;
        }
    }

    *hex = dd;

    return (n<0?-1:0);
}


static int sublist_str_to_sub(uint8_t num, uint16_t* sub, uint8_t* sub_str)
{
    if(!num || !sub || !sub_str) {
        return -EINVAL;
    }

    char str_temp[7] = {0x00};
    uint8_t char_size = 0;
    uint8_t* sub_str_head = NULL;

    int i = 0;

    for(i = 0; i < num && *sub_str != '\0'; i++) {
        sub_str_head = sub_str;
        char_size = 0;
        while(*sub_str_head != ',' && *sub_str_head != '\0') {
            char_size++;
            sub_str_head++;
        }

        if(char_size > 6) {
            return -EINVAL;
        }
        if(*sub_str == '0' && (*(sub_str + 1) == 'x' || *(sub_str + 1) == 'X')) {
            memcpy(str_temp,sub_str + 2, char_size - 2);
        } else {
            memcpy(str_temp,sub_str, char_size);
        }

        if(strtohex16(&sub[i], str_temp) < 0) {
            return -EINVAL;
        }
        sub_str += char_size + 1;
    }

    if(i != num) {
        return -EINVAL;
    }

    return 0;
}

void at_cmd_btmesh_sub_overwrite(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t mod_id;
        uint32_t cid;
        uint32_t sublist_size;
        char  sub_addr_str[7 * CONFIG_MAX_SUBLIST_OVERWRITE_SIZE] = {0x00};
        uint16_t sub_addr[CONFIG_MAX_SUBLIST_OVERWRITE_SIZE] = {0x00};
        int input_num;
        int ret;

        input_num = atserver_scanf("%x,%x,%x,%x,%[^\r\n]", &unicast_addr, &mod_id,&cid,&sublist_size,&sub_addr_str);
        if (input_num < 5 || sublist_size > CONFIG_MAX_SUBLIST_OVERWRITE_SIZE) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        ret = sublist_str_to_sub(sublist_size, sub_addr, (uint8_t *)sub_addr_str);
        if(ret ) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }
        ret = ble_mesh_node_overwrite_node_sublist(0, 0, unicast_addr,mod_id, sublist_size, sub_addr,cid);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_OVERWRITE_SUB_LIST_FAIL));
        } else {

            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicat_addr>,<mod_id>,<cid>,<list_size>,<subaddr_list>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

int print_local_subaddr(uint16_t unicast_addr, uint8_t elem_id, uint16_t mod_id, uint16_t CID)
{
    const struct bt_mesh_comp *comp = ble_mesh_model_get_comp_data();

    if (NULL == comp) {
        atserver_send("\r\n+MESHGETSUB:%d\r\nERROR\r\n", ERR_MESH_AT(AT_ERR_MESH_GET_SUB_ADDR_FAILD));
        return -1;
    }

    if (elem_id >= comp->elem_count) {
        atserver_send("\r\n+MESHGETSUB:%d\r\nERROR\r\n", ERR_MESH_AT(AT_ERR_MESH_GET_SUB_ADDR_FAILD));
        return -1;
    }

    struct bt_mesh_model *model = NULL;

    model = ble_mesh_model_find(elem_id, mod_id, CID);

    if (!model) {
        atserver_send("\r\n+MESHGETSUB:%d\r\nERROR\r\n", ERR_MESH_AT(AT_ERR_MESH_GET_SUB_ADDR_FAILD));
        return -1;
    }

    AT_BACK_OK();

    uint8_t found_addr = 0;

    for (int i = 0 ; i < sizeof(model->groups) / sizeof(model->groups[0]); i++) {
        if (model->groups[i] != BT_MESH_ADDR_UNASSIGNED) {
            if (!found_addr) {
                atserver_send("\r\n+MESHSUBLIST:%04x,%04x,%04x,%04x", unicast_addr, CID, mod_id, model->groups[i]);
                found_addr = 1;
            } else {
                atserver_send(",%04x", model->groups[i]);
            }
        }
    }

    if (found_addr) {
        atserver_send("\r\n");
    }

    return 0;
}

void at_cmd_btmesh_sub_get(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t mod_id;
        uint32_t cid;
        int input_num;
        int ret;

        input_num = atserver_scanf("%x,%x,%x", &unicast_addr, &mod_id, &cid);

        if (input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num == 2) {
            ret =  bt_mesh_cfg_mod_sub_get(0, unicast_addr, mod_id, NULL, NULL);
        } else {
            ret = bt_mesh_cfg_mod_sub_get_vnd(0, unicast_addr, mod_id, cid, NULL, NULL);
        }

        if (ret && TIMEOUT != ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_GET_SUB_ADDR_FAILD));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicat_addr>,<mod_id>,[cid]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_pub_get(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t mod_id;
        uint32_t cid;
        int input_num;
        int ret;

        input_num = atserver_scanf("%x,%x,%x", &unicast_addr, &mod_id, &cid);

        if (input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (input_num == 2) {
            ret =  bt_mesh_cfg_mod_pub_get(0, unicast_addr, unicast_addr, mod_id, NULL, NULL);
        } else {
            ret = bt_mesh_cfg_mod_pub_get_vnd(0, unicast_addr, unicast_addr, mod_id, cid, NULL, NULL);
        }

        g_mesh_server.get_pub_flag = 1;

        if (ret && TIMEOUT != ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_GET_PUB_ADDR_FAILD));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicat_addr>,<mod_id>,[cid]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_rst(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        int input_num;
        int ret;

        input_num = atserver_scanf("%x", &unicast_addr);

        if (1 != input_num) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if(!BT_MESH_ADDR_IS_UNICAST(unicast_addr)) { //not allowed rst by group addr
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if(!bt_mesh_provisioner_get_node_info(unicast_addr)) {
            LOGE(TAG,"Node not found");
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(-ENODEV));
            return;
        }

        AT_BACK_OK();

        bt_mesh_cfg_cli_timeout_set(0);
        ret = bt_mesh_cfg_node_reset(0, unicast_addr);
        bt_mesh_cfg_cli_timeout_set(2);

        if (ret && TIMEOUT1 != ret) {
            atserver_send("\r\n%s:%d\r\n", cmd + 2, ret);
        }

        ret = _mesh_rst_retry_node_add(unicast_addr);
        if(ret) {
            LOGE(TAG,"mesh node rst fail %d",ret);
        }
    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<unicast_addr>");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


/*-----------------------------------------------------------
onoff cli cmd api
-----------------------------------------------------------*/

set_onoff_arg set_onoff_data;

void at_cmd_btmesh_onoff(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }


    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        int32_t onoff;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x", &unicast_addr, &onoff, &ack, &appkey_idx);

        if (input_num < 2 || onoff < 0) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *onoff_cli_model;

        onoff_cli_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_GEN_ONOFF_CLI, CID_NVAL);

        if (!onoff_cli_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num <= 2) {
            ack = 0;
        }

        if (input_num <= 3) {
            appkey_idx = 0;
        }

        set_onoff_data.onoff = onoff;

        ret = ble_mesh_generic_onoff_set(0, appkey_idx, unicast_addr, onoff_cli_model, &set_onoff_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<onoff>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

/*-----------------------------------------------------------
level cli cmd api
-----------------------------------------------------------*/


set_level_arg set_level_data;


void at_cmd_btmesh_level(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t level;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x", &unicast_addr, &level, &ack, &appkey_idx);

        if (input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *level_cli_model;

        level_cli_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_GEN_LEVEL_CLI, CID_NVAL);

        if (!level_cli_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num <= 2) {
            ack = 0;
        }

        if (input_num <= 3) {
            appkey_idx = 0;
        }

        set_level_data.level = level;

        ret = ble_mesh_generic_level_set(0, appkey_idx, unicast_addr, level_cli_model, &set_level_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<level>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_level_move(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t move;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x", &unicast_addr, &move, &ack, &appkey_idx);

        if (input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *level_cli_model;

        level_cli_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_GEN_LEVEL_CLI, CID_NVAL);

        if (!level_cli_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num <= 2) {
            ack = 0;
        }

        if (input_num <= 3) {
            appkey_idx = 0;
        }

        set_level_data.move = move;

        ret = ble_mesh_generic_level_move_set(0, appkey_idx, unicast_addr, level_cli_model, &set_level_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<move>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_level_move_delta(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }


    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t move_delta;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x", &unicast_addr, &move_delta, &ack, &appkey_idx);

        if (input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *level_cli_model;

        level_cli_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_GEN_LEVEL_CLI, CID_NVAL);

        if (!level_cli_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num <= 2) {
            ack = 0;
        }

        if (input_num <= 3) {
            appkey_idx = 0;
        }

        set_level_data.delta = move_delta;

        ret = ble_mesh_generic_level_delta_set(0, appkey_idx, unicast_addr, level_cli_model, &set_level_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<delta>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}



/*-----------------------------------------------------------
lightness cli cmd api
-----------------------------------------------------------*/


set_lightness_arg set_lightness_data;


void at_cmd_btmesh_lightness(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t lightness;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x", &unicast_addr, &lightness, &ack, &appkey_idx);

        if (input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *lightness_cli_model;

        lightness_cli_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI, CID_NVAL);

        if (!lightness_cli_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num <= 2) {
            ack = 0;
        }

        if (input_num <= 3) {
            appkey_idx = 0;
        }

        set_lightness_data.lightness = lightness;
        ret = ble_mesh_light_lightness_set(0, appkey_idx, unicast_addr, lightness_cli_model, &set_lightness_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<lightness>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_lightness_linear(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t lightness_linear;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x", &unicast_addr, &lightness_linear, &ack, &appkey_idx);

        if (input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *lightness_cli_model;

        lightness_cli_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI, CID_NVAL);

        if (!lightness_cli_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num <= 2) {
            ack = 0;
        }

        if (input_num <= 3) {
            appkey_idx = 0;
        }

        set_lightness_data.lightness_linear = lightness_linear;
        ret = ble_mesh_light_lightness_linear_set(0, appkey_idx, unicast_addr, lightness_cli_model, &set_lightness_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<lightnesslin>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_lightness_range(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }


    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t range_min;
        uint32_t range_max;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x,%x", &unicast_addr, &range_min, &range_max, &ack, &appkey_idx);

        if (input_num < 3 || !range_min || !range_max || range_min > range_max) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *lightness_cli_model;

        lightness_cli_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI, CID_NVAL);

        if (!lightness_cli_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num <= 3) {
            ack = 0;
        }

        if (input_num <= 4) {
            appkey_idx = 0;
        }

        set_lightness_data.range_min = range_min;
        set_lightness_data.range_max = range_max;

        ret = ble_mesh_light_lightness_range_set(0, appkey_idx, unicast_addr, lightness_cli_model, &set_lightness_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<range_min>,<range_max>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_lightness_def(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t lightness_def;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x", &unicast_addr, &lightness_def, &ack, &appkey_idx);

        if (input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *lightness_cli_model;

        lightness_cli_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI, CID_NVAL);

        if (!lightness_cli_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num <= 2) {
            ack = 0;
        }

        if (input_num <= 3) {
            appkey_idx = 0;
        }

        set_lightness_data.def = lightness_def;

        ret = ble_mesh_light_lightness_def_set(0, appkey_idx, unicast_addr, lightness_cli_model, &set_lightness_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<lightness>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


/*-----------------------------------------------------------
light ctl cli cmd api
-----------------------------------------------------------*/
set_light_ctl_arg set_light_ctl_data;



void at_cmd_btmesh_light_ctl(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t lightness;
        uint32_t temperature;
        uint32_t delta_uv;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x,%x,%x", &unicast_addr, &lightness, &temperature, &delta_uv, &ack, &appkey_idx);

        if (input_num < 4 || temperature < CTL_TEMP_MIN || temperature > CTL_TEMP_MAX) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *light_ctl_model;

        light_ctl_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_LIGHT_CTL_CLI, CID_NVAL);

        if (!light_ctl_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num < 5) {
            ack = 0;
        }

        if (input_num < 6) {
            appkey_idx = DEF_SEND_APPKEY_IDX;
        }

        set_light_ctl_data.lightness  = lightness;
        set_light_ctl_data.temperature = temperature;
        set_light_ctl_data.delta_uv = delta_uv;
        set_light_ctl_data.tid++;

        ret = ble_mesh_light_ctl_set(0, appkey_idx, unicast_addr, light_ctl_model, &set_light_ctl_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<lightness>,<temperature>,<delta_UV>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_light_ctl_temp(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t temperature;
        uint32_t delta_uv;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x,%x,%x", &unicast_addr, &temperature, &delta_uv, &ack, &appkey_idx);

        if (input_num < 3 || temperature < CTL_TEMP_MIN || temperature > CTL_TEMP_MAX) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *light_ctl_model;

        light_ctl_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_LIGHT_CTL_CLI, CID_NVAL);

        if (!light_ctl_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num < 4) {
            ack = 0;
        }

        if (input_num < 5) {
            appkey_idx = DEF_SEND_APPKEY_IDX;
        }

        set_light_ctl_data.temperature = temperature;
        set_light_ctl_data.delta_uv = delta_uv;
        set_light_ctl_data.tid++;

        ret = ble_mesh_light_ctl_temp_set(0, appkey_idx,  unicast_addr, light_ctl_model, &set_light_ctl_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<temperature>,<delta_UV>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}

void at_cmd_btmesh_light_ctl_range(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t range_min;
        uint32_t range_max;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x,%x", &unicast_addr, &range_min, &range_max, &ack, &appkey_idx);

        if (input_num < 3) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        if (range_min < CTL_TEMP_MIN || range_max > CTL_TEMP_MAX || range_min > range_max) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *light_ctl_model;

        light_ctl_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_LIGHT_CTL_CLI, CID_NVAL);

        if (!light_ctl_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num < 4) {
            ack = 0;
        }

        if (input_num < 5) {
            appkey_idx = DEF_SEND_APPKEY_IDX;
        }

        set_light_ctl_data.range_min = range_min;
        set_light_ctl_data.range_max = range_max;


        ret = ble_mesh_light_ctl_temp_range_set(0, appkey_idx, unicast_addr, light_ctl_model, &set_light_ctl_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<range_min>,<range_max>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


void at_cmd_btmesh_light_ctl_def(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t lightness;
        uint32_t temperature;
        uint32_t delta_uv;
        uint32_t ack;
        uint32_t appkey_idx;
        int16_t input_num;
        int16_t ret;

        input_num = atserver_scanf("%x,%x,%x,%x,%x,%x", &unicast_addr, &lightness, &temperature, &delta_uv, &ack, &appkey_idx);

        if (input_num < 4 || temperature < CTL_TEMP_MIN || temperature > CTL_TEMP_MAX) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        struct bt_mesh_model *light_ctl_model;

        light_ctl_model = ble_mesh_model_find(0, BT_MESH_MODEL_ID_LIGHT_CTL_CLI, CID_NVAL);

        if (!light_ctl_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

        if (input_num < 5) {
            ack = 0;
        }

        if (input_num < 6) {
            appkey_idx = DEF_SEND_APPKEY_IDX;
        }

        set_light_ctl_data.lightness = lightness;
        set_light_ctl_data.temperature = temperature;
        set_light_ctl_data.delta_uv = delta_uv;
        set_light_ctl_data.tid++;

        ret = ble_mesh_light_ctl_def_set(0, appkey_idx, unicast_addr, light_ctl_model, &set_light_ctl_data, ack);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

    } else if (type == TEST_CMD) {
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<lightness>,<temperature>,<delta_UV>,[ack],[appkey_idx]");
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }
}


/*-----------------------------------------------------------
vendor cli cmd api
-----------------------------------------------------------*/
void at_cmd_btmesh_vendor_send(char *cmd, int type, char *data)
{
    if (!g_mesh_server.init_flag) {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_AT_NOT_INIT));
        return;
    }

    if (type == WRITE_CMD) {
        uint32_t unicast_addr;
        uint32_t len;
        uint32_t trans;
        uint32_t appkey_idx;
        int16_t input_num;
        static uint8_t tid = 0;
        int16_t ret;
        uint16_t msg_len = 0;
        uint8_t  data_str[DEF_TRS_MAX_LENGTH + 1];
        uint8_t  data_hex[DEF_TRS_MAX_LENGTH];
#if defined(CONFIG_AT_MESH_TRS_TEST)	&& CONFIG_AT_MESH_TRS_TEST > 0
        uint8_t *data_temp = NULL;
#endif
        vnd_model_msg vendor_msg = {0};

#if defined(CONFIG_AT_MESH_TRS_TEST)	&& CONFIG_AT_MESH_TRS_TEST > 0
        input_num = atserver_scanf("%x,%x,%x,%x,%"LENGTH_TRS(DEF_TRS_MAX_LENGTH)"[^\r\n]", &unicast_addr, &len, &appkey_idx, &trans, data_str);
#else
        input_num = atserver_scanf("%x,%x,%"LENGTH_TRS(DEF_TRS_MAX_LENGTH)"[^,],%x,%x", &unicast_addr, &len, data_str, &appkey_idx, &trans);
#endif

#if defined(CONFIG_AT_MESH_TRS_TEST) && CONFIG_AT_MESH_TRS_TEST > 0
        if (input_num < 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }
#else
        if (input_num < 3) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }
#endif
        struct bt_mesh_model *vendor_model;

        vendor_model = ble_mesh_model_find(0, BT_MESH_MODEL_VND_MODEL_CLI, VND_CID);

        if (!vendor_model) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_MESH_CMD_NOT_SUPPORTED));
            return;
        }

#if defined(CONFIG_AT_MESH_TRS_TEST) && CONFIG_AT_MESH_TRS_TEST > 0
        if (input_num <= 2) {
            appkey_idx = 0;
        }
        if(input_num <= 3) {
            trans = NET_TRANS_LEGACY;
        }
#if defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0
        if(trans > NET_TRANS_EXT_ADV_2M) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }
#endif
#else
        if (input_num <= 3) {
            appkey_idx = 0;
        }
        if(input_num <= 4) {
            trans = NET_TRANS_LEGACY;
        }

#endif

#if !(defined(CONFIG_BT_MESH_EXT_ADV)	&& CONFIG_BT_MESH_EXT_ADV > 0)
        if(trans > NET_TRANS_LEGACY) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }
#endif


#if defined(CONFIG_AT_MESH_TRS_TEST) && CONFIG_AT_MESH_TRS_TEST > 0
        if(input_num == 5) {
            msg_len = (strlen((char *)data_str) + 1) / 2 < len ? (strlen((char *)data_str) + 1) / 2 : len;
        } else {
            msg_len = len;
        }
#else
        msg_len = (strlen((char *)data_str) + 1) / 2 < len ? (strlen((char *)data_str) + 1) / 2 : len;
#endif
        if (msg_len > DEF_TRS_MAX_LENGTH / 2) {
            AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
            return;
        }

        vendor_msg.netkey_idx = 0;
        vendor_msg.appkey_idx = appkey_idx;
        vendor_msg.dst_addr = unicast_addr;
        vendor_msg.model = vendor_model;
        vendor_msg.len = msg_len;
        vendor_msg.tid = tid++;
        vendor_msg.opid  = VENDOR_OP_ATTR_TRANS_MSG;
        vendor_msg.trans = trans;

#if defined(CONFIG_AT_MESH_TRS_TEST)	&& CONFIG_AT_MESH_TRS_TEST > 0
        if(input_num < 5) {
            data_temp = aos_zalloc(msg_len);
            if(!data_temp) {
                AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
                return;
            }
            for(int i = 0; i < msg_len; i++) {
                data_temp[i] = i % 9;
            }

			if(msg_len >= 2) { //for data loss test
               data_temp[0]  = 0xf4;
			   data_temp[1]  = 0x04;
			}
            vendor_msg.data = data_temp;
        } else {
            str2hex(data_hex, (char *)data_str, vendor_msg.len);
            vendor_msg.data = data_hex;
        }
#else
        str2hex(data_hex, (char *)data_str, vendor_msg.len);
        vendor_msg.data = data_hex;
#endif
        ret = ble_mesh_vendor_cli_model_msg_send(&vendor_msg);

        if (ret) {
            AT_BACK_RET_ERR(cmd, ERR_MESH(ret));
        } else {
            AT_BACK_OK();
        }

#if defined(CONFIG_AT_MESH_TRS_TEST) && CONFIG_AT_MESH_TRS_TEST > 0
        if(input_num < 5) {
            aos_free(data_temp);
        }
#endif

    } else if (type == TEST_CMD) {
#if defined(CONFIG_AT_MESH_TRS_TEST) && CONFIG_AT_MESH_TRS_TEST > 0
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<len>,[appkey_idx],[trans],[data]");
#else
        AT_BACK_TEST_RET_OK(cmd, "<addr>,<len>,<data>,[appkey_idx],[trans]");
#endif
    } else {
        AT_BACK_RET_ERR(cmd, ERR_MESH_AT(AT_ERR_INVAL));
    }

}

#endif
