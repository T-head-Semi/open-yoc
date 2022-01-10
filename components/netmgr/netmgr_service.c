/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <aos/aos.h>
#include <uservice/uservice.h>
#include <uservice/eventid.h>
#include <devices/netdrv.h>
#include <yoc/netmgr.h>
#include <yoc/netmgr_service.h>
#include <ulog/ulog.h>
#ifdef CONFIG_KV_SMART
#include <aos/kv.h>
#endif

#include "activation.h"

#define KV_VAL_LENGTH       32

typedef struct {
    netmgr_hdl_t hdl;
    ip_setting_t config;
} param_ip_setting_t;

static const char *TAG = "netmgr";

struct netmgr_uservice netmgr_svc = {0};

#ifdef CONFIG_KV_SMART
char *netmgr_kv_get(const char *key)
{
    static char str[KV_VAL_LENGTH];
    int len = KV_VAL_LENGTH;

    memset(str, 0, KV_VAL_LENGTH);
    aos_kv_get(key, str, &len);

    return str;
}

int netmgr_kv_getint(const char *key)
{
    int val, ret;

    ret = aos_kv_getint(key, &val);

    if (ret == 0) {
        return val;
    }

    return 0;
}
#endif

/**
 * @brief  handle event of the netmgr uservice
 * @param  [in] event_id
 * @param  [in] param
 * @param  [in] context
 * @return
 */
void netmgr_event_cb(uint32_t event_id, const void *param, void *context)
{
    if (netmgr_svc.srv == NULL) {
        LOGE(TAG, "Netmgr already stopped[%d]", event_id);
        return;
    }
    switch(event_id) {
    case EVENT_NETWORK_RESTART:
        uservice_call_async(netmgr_svc.srv, API_NET_RESET, (void *)&param, sizeof(netmgr_hdl_t));
        break;
    case EVENT_WIFI_LINK_DOWN:
        /* param as link donw reason */
        uservice_call_async(netmgr_svc.srv, event_id, (void *)&param, 4);
        break;
    default:
        uservice_call_async(netmgr_svc.srv, event_id, NULL, 0);
        break;
    }
}

/**
 * @brief  subscribe event of the netmgr uservice
 * @param  [in] cmd_id : EVENT_WIFI_LINK_UP, etc
 * @return
 */
void netmgr_subscribe(int cmd_id)
{
    event_subscribe(cmd_id, netmgr_event_cb, NULL);
}

/**
 * @brief  unsubscribe event of the netmgr uservice
 * @param  [in] cmd_id : EVENT_WIFI_LINK_UP, etc
 * @return
 */
void netmgr_unsubscribe(int cmd_id)
{
    event_unsubscribe(cmd_id, netmgr_event_cb, NULL);
}

/**
 * @brief  find netmgr dev from netdev list by the network name
 * @param  [in] list : netmgr dev list
 * @param  [in] name
 * @return NULL on error
 */
netmgr_dev_t *netmgr_find_dev(slist_t *list, const char *name)
{
    netmgr_dev_t *node;
    slist_for_each_entry(list, node, netmgr_dev_t, next) {
        if ((name == NULL) && (node != NULL)) {
            /* if no name provided, return the first node */
            return node;
        }

        if (!strcmp(node->name, name)) {
            return node;
        }
    }

    return NULL;
}

/**
 * @brief  unregist service function
 * @param  [in] cmd_id : command id of the netmgr uservice
 * @param  [in] func : callback
 * @return 0/-1
 */
int netmgr_unreg_srv_func(int cmd_id, netmgr_srv_func func)
{
    struct netmgr_uservice *netmgr = &netmgr_svc;

    netmgr_srv_t* node;
    slist_for_each_entry(&netmgr->srv_list, node, netmgr_srv_t, next) {
        if ((node->cmd_id == cmd_id) &&  (node->func == func)) {
            slist_del((slist_t *)node, &netmgr->srv_list);
            aos_free(node);
            return 0;
        }
    }

    return -1;
}

/**
 * @brief  regist service function
 * @param  [in] cmd_id : command id of the netmgr uservice
 * @param  [in] func : callback
 * @return 0/-1
 */
int netmgr_reg_srv_func(int cmd_id, netmgr_srv_func func)
{
    struct netmgr_uservice *netmgr = &netmgr_svc;
    netmgr_srv_t *node = (netmgr_srv_t *)aos_zalloc(sizeof(netmgr_srv_t));

    if (node) {
        node->cmd_id = cmd_id;
        node->func = func;
        slist_add_tail((slist_t *)node, &netmgr->srv_list);
        return 0;
    }

    return -1;
}

/**
 * @brief  start dhcp by the network name
 * @param  [in] netmgr
 * @param  [in] name : network name
 * @return 0/-1
 */
int netmgr_start_dhcp(struct netmgr_uservice *netmgr, const char *name)
{
    netmgr_dev_t *node = netmgr_find_dev(&netmgr->dev_list, name);
    static int link_reason;

    if (node == NULL) {
        LOGE(TAG, "net device not found");
        return -1;
    }

    aos_dev_t *dev = node->dev;

    if (node->dhcp_en == 1) {

        LOGI(TAG, "start dhcp");

        if (hal_net_start_dhcp(dev) < 0) {
            goto failed;
        }

        event_publish_delay(API_NET_DHCP_CHECK, NULL, 30 * 1000);

        return 0;
    }

failed:
    link_reason = NET_DISCON_REASON_DHCP_ERROR;
    event_publish(EVENT_NETMGR_NET_DISCON, &link_reason);

    return -1;
}

static int netmgr_evt_ip_got(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    netmgr_dev_t *node = netmgr_find_dev(&netmgr->dev_list, NULL);
    aos_dev_t *dev = node->dev;

    hal_net_get_ipaddr(dev, &node->ipaddr, &node->netmask, &node->gw);
    LOGI(TAG, "IP: %s", ipaddr_ntoa((ip4_addr_t *)ip_2_ip4(&node->ipaddr)));

#ifdef AOS_COMP_ACTIVATION
    activation_report();
#endif
    netmgr_set_gotip(NULL, 1);
    event_publish(EVENT_NETMGR_GOT_IP, &node->ipaddr);

    return 0;
}

static int netmgr_check_dhcp(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    netmgr_dev_t *node = netmgr_find_dev(&netmgr->dev_list, NULL);
    aos_dev_t *dev = node->dev;
    static int link_reason;

    if (!netmgr_is_gotip(NULL) && netmgr_is_linkup(NULL)) {
        LOGE(TAG, "dhcp failed");
        link_reason = NET_DISCON_REASON_DHCP_ERROR;
        hal_net_stop_dhcp(dev);
        event_publish(EVENT_NETMGR_NET_DISCON, &link_reason);
    }

    return 0;
}

static int netmgr_srv_ipconfig(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    int ret = -EBADFD;
    param_ip_setting_t *param = rpc_get_point(rpc);

    if (param == NULL) {
        return ret;
    }

    netmgr_dev_t *node = (netmgr_dev_t *)param->hdl;

    if (node != NULL) {
        aos_dev_t *dev = node->dev;
        ip_setting_t *ip = &param->config;
        node->dhcp_en = ip->dhcp_en;
#ifdef CONFIG_KV_SMART

        if ((ip->ipaddr != NULL) && (ip->netmask != NULL) && (ip->gw != NULL)) {
            aos_kv_setint(KV_DHCP_EN, node->dhcp_en);
            aos_kv_set(KV_IP_ADDR, ip->ipaddr, strlen(ip->ipaddr), 1);
            aos_kv_set(KV_IP_NETMASK, ip->netmask, strlen(ip->netmask), 1);
            aos_kv_set(KV_IP_GW, ip->gw, strlen(ip->gw), 1);

            ret = 0;
        } else {
            ret = -EBADFD;
        }

#endif
        if (node->dhcp_en) {
            netmgr_start_dhcp(netmgr, node->name);
            ret = 0;
        } else {
            if ((ip->ipaddr != NULL) && (ip->netmask != NULL) && (ip->gw != NULL)) {
                ip_addr_t ipaddr;
                ip_addr_t netmask;
                ip_addr_t gw;
                ip4addr_aton(ip->ipaddr, (ip4_addr_t *)ip_2_ip4(&ipaddr));
                ip4addr_aton(ip->netmask, (ip4_addr_t *)ip_2_ip4(&netmask));
                ip4addr_aton(ip->gw, (ip4_addr_t *)ip_2_ip4(&gw));

                hal_net_set_ipaddr(dev, &ipaddr, &netmask, &gw);
                event_publish(EVENT_NET_GOT_IP, NULL);
            }
        }
    }

    rpc_put_buffer(rpc, &ret, 4);
    return ret;
}

static int netmgr_srv_reset(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    int ret = -1;
    netmgr_dev_t *node = (netmgr_dev_t *)rpc_get_point(rpc);

    if (node == NULL) {
        return -EBADFD;
    }

    if (node->reset) {
        if (!node->is_gotip) {
            ret = node->reset(node);
        }
    }

    return ret;
}

static int netmgr_srv_start(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    int ret = -1;
    netmgr_dev_t *node = (netmgr_dev_t *)rpc_get_point(rpc);

    if (node == NULL) {
        return -EBADFD;
    }

    LOGI(TAG, "start %s", node->name);

    netmgr_subscribe(EVENT_NETWORK_RESTART);
    netmgr_subscribe(EVENT_NET_GOT_IP);
    netmgr_subscribe(API_NET_DHCP_CHECK);

    if (node->provision) {
        ret = node->provision(node);
    }

    return ret;

}

static int netmgr_srv_stop(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    int ret = -1;
    netmgr_dev_t *node = *(netmgr_dev_t **)rpc_get_point(rpc);


    if (node && node->unprovision) {
        ret = node->unprovision(node);
    }

    return ret;
}

static int netmgr_srv_info(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    int ret = 0;
    netmgr_dev_t *node = *(netmgr_dev_t **)rpc_get_point(rpc);

    if (node == NULL) {
        return -EBADFD;
    }

    if (node->info) {
        node->info(node);
    }

    return ret;
}

static int netmgr_service(void *context, rpc_t *rpc)
{
    int ret = 0;
    struct netmgr_uservice *netmgr = (struct netmgr_uservice *)context;

    // if (rpc->cmd_id < API_WIFI_END) {
    //     netmgr->current_cmd = rpc;
    // }

    /**
        special handle with other return parameter in switch case,
        common handle with int return val in chain loop
    */
    switch (rpc->cmd_id) {

    case API_NET_GET_HDL: {
        char *name = rpc_get_point(rpc);
        netmgr_dev_t *node;

        node = netmgr_find_dev(&netmgr->dev_list, name);

        if (node == NULL) {
            node = (netmgr_dev_t *)aos_zalloc(sizeof(netmgr_dev_t));

            if (node) {
                node->dev = device_open(name);

                if (node->dev) {
                    strncpy(node->name, name, NETMGR_NAME_LEN - 1);
                    node->name[NETMGR_NAME_LEN - 1] = 0;
                    slist_add_tail((slist_t *)node, &netmgr->dev_list);
                } else {
                    aos_free(node);
                    node = NULL;
                }
            }
        }

        rpc_put_reset(rpc);
        rpc_put_point(rpc, node);
        rpc_reply(rpc);
        return 0;
    }

    default: {
        netmgr_srv_t *node;
        slist_for_each_entry(&netmgr->srv_list, node, netmgr_srv_t, next) {
            if (node->cmd_id == rpc->cmd_id) {
                ret = node->func(netmgr, rpc);
                break;
            }
        }
    }

    break;
    }

    rpc_put_reset(rpc);

    /*
     * When an asynchronous API call,
     * rpc->data is empty and cannot have a return value.
     */
    if (rpc->data) {
        rpc_put_int(rpc, ret);
    }

    rpc_reply(rpc);

    return 0;
}

/**
 * @brief  create & init the netmgr uservice
 * @param  [in] task
 * @return
 */
void netmgr_service_init(utask_t *task)
{
    aos_kernel_sched_suspend();
    if (netmgr_svc.inited == 0) {
        netmgr_svc.inited = 1;
    } else {
        aos_kernel_sched_resume();
        return;
    }
    aos_kernel_sched_resume();

    netmgr_reg_srv_func(API_NET_IPCONFIG, netmgr_srv_ipconfig);
    netmgr_reg_srv_func(API_NET_START, netmgr_srv_start);
    netmgr_reg_srv_func(API_NET_STOP, netmgr_srv_stop);
    netmgr_reg_srv_func(API_NET_INFO, netmgr_srv_info);
    netmgr_reg_srv_func(API_NET_RESET, netmgr_srv_reset);
    netmgr_reg_srv_func(EVENT_NET_GOT_IP, netmgr_evt_ip_got);
    netmgr_reg_srv_func(API_NET_DHCP_CHECK, netmgr_check_dhcp);

    if (task == NULL) {
        task = utask_new("netmgr", 32 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);
    }

    if (task) {
        netmgr_svc.srv = uservice_new("netmgr_svc", netmgr_service, &netmgr_svc);
        utask_add(task, netmgr_svc.srv);
        netmgr_svc.task = task;
    }
}

/**
 * @brief  destroy & uninit the netmgr uservice
 * @return
 */
void netmgr_service_deinit()
{
    aos_kernel_sched_suspend();
    if (netmgr_svc.inited == 1) {
        netmgr_svc.inited = 0;
    } else {
        aos_kernel_sched_resume();
        printf("Netmgr already stopped\n");
        return;
    }
    aos_kernel_sched_resume();

    netmgr_unsubscribe(EVENT_NETWORK_RESTART);
    netmgr_unsubscribe(EVENT_NET_GOT_IP);
    netmgr_unsubscribe(API_NET_DHCP_CHECK);

    netmgr_unreg_srv_func(API_NET_IPCONFIG, netmgr_srv_ipconfig);
    netmgr_unreg_srv_func(API_NET_START, netmgr_srv_start);
    netmgr_unreg_srv_func(API_NET_STOP, netmgr_srv_stop);
    netmgr_unreg_srv_func(API_NET_INFO, netmgr_srv_info);
    netmgr_unreg_srv_func(API_NET_RESET, netmgr_srv_reset);
    netmgr_unreg_srv_func(EVENT_NET_GOT_IP, netmgr_evt_ip_got);
    netmgr_unreg_srv_func(API_NET_DHCP_CHECK, netmgr_check_dhcp);

    utask_destroy(netmgr_svc.task);
    uservice_destroy(netmgr_svc.srv);

    netmgr_svc.task = NULL;
    netmgr_svc.srv = NULL;
}

/**
 * @brief  set gotip flag by network name
 * @param  [in] name
 * @param  [in] gotip
 * @return -1 on error
 */
int netmgr_set_gotip(const char *name, int gotip)
{
    CHECK_PARAM(netmgr_svc.inited, -1);

    netmgr_dev_t *node = netmgr_find_dev(&netmgr_svc.dev_list, name);

    if (node) {
        node->is_gotip = gotip;
        return gotip;
    }

    return -1;
}

/**
 * @brief  set link layer up flag by network name
 * @param  [in] name
 * @param  [in] linkup
 * @return -1 on error
 */
int netmgr_set_linkup(const char *name, int linkup)
{
    CHECK_PARAM(netmgr_svc.inited, -1);

    netmgr_dev_t *node = netmgr_find_dev(&netmgr_svc.dev_list, name);

    if (node) {
        node->is_linkup = linkup;
        return linkup;
    }

    return -1;
}


////////////////////////////////////////////
/* below is for YOC API */
//API don't include dev operations (aos_dev_t was hiden)
////////////////////////////////////////////

netmgr_hdl_t netmgr_get_handle(const char *name)
{
    CHECK_PARAM(netmgr_svc.inited, NULL);

    netmgr_hdl_t hdl;

    if (netmgr_svc.srv == NULL) {
        LOGI(TAG, "Netmgr not running");
        return NULL;
    }

    uservice_call_sync(netmgr_svc.srv, API_NET_GET_HDL, (void *)name, &hdl, sizeof(netmgr_hdl_t));
    return hdl;
}

/**
 * @brief  get aos_dev by the netmgr hdl
 * @param  [in] netmgr_hdl_t
 * @return NULL on error
 */
aos_dev_t *netmgr_get_dev(netmgr_hdl_t hdl)
{
    CHECK_PARAM(netmgr_svc.inited, NULL);

    netmgr_dev_t *dev = (netmgr_dev_t *)hdl;
    aos_check_return_null(dev);

    return ((netmgr_dev_t *)dev)->dev;
}

/**
 * @brief  netmgr config the ip or enable dhcp
 * @param  [in] hdl
 * @param  [in] dhcp_en : if enable, dhcp priority. otherwise use the static ip
 * @param  [in] ipaddr
 * @param  [in] netmask
 * @param  [in] gw
 * @return 0 on success
 */
int netmgr_ipconfig(netmgr_hdl_t hdl, int dhcp_en, char *ipaddr, char *netmask, char *gw)
{
    CHECK_PARAM(netmgr_svc.inited, -1);

    int ret = -1;
    param_ip_setting_t param;
    ip_setting_t *config = &param.config;

    aos_check_return_einval(hdl);

    if ((dhcp_en == 0) && ((ipaddr == NULL) || (netmask == NULL) || (gw == NULL))) {
        return -EINVAL;
    }

    config->dhcp_en = dhcp_en;
    config->ipaddr = ipaddr;
    config->netmask = netmask;
    config->gw = gw;

    param.hdl = hdl;

    uservice_call_sync(netmgr_svc.srv, API_NET_IPCONFIG, &param, &ret, sizeof(int));

    return ret;
}

/**
 * @brief  netmgr start provision, async.
 * if provison ok, EVENT_NETMGR_GOT_IP post, otherwise EVENT_NETMGR_NET_DISCON
 * @param  [in] hdl
 * @return 0/-1
 */
int netmgr_start(netmgr_hdl_t hdl)
{
    CHECK_PARAM(netmgr_svc.inited, -1);
    
    int ret = 0;
    aos_check_return_einval(hdl);

    uservice_call_async(netmgr_svc.srv, API_NET_START, (void *)&hdl, sizeof(hdl));

    return ret;
}

/**
 * @brief  reset the netmgr, and re-provision after sec
 * @param  [in] hdl
 * @param  [in] sec : 0s means re-provision immediatly
 * @return 0/-1
 */
int netmgr_reset(netmgr_hdl_t hdl, uint32_t sec)
{
    CHECK_PARAM(netmgr_svc.inited, -1);
    aos_check_return_einval(hdl);

    /*
     * The network device will be reset,
     * first set the goip falg, and then cancel the reset
     * if the network recovers during the delay
     */
    ((netmgr_dev_t*)hdl)->is_gotip = 0;

    if (sec <= 0) {
        uservice_call_async(netmgr_svc.srv, API_NET_RESET, (void *)&hdl, sizeof(hdl));
        return 0;
    }

    event_publish_delay(EVENT_NETWORK_RESTART, (void *)hdl, sec * 1000);

    return 0;
}

/**
 * @brief  stop netmgr(sync), will call the unprovison callback
 * @param  [in] hdl
 * @return 0/-1
 */
int netmgr_stop(netmgr_hdl_t hdl)
{
    CHECK_PARAM(netmgr_svc.inited, -1);

    int ret = -1;

    aos_check_return_einval(hdl);

    uservice_call_sync(netmgr_svc.srv, API_NET_STOP, &hdl, &ret, sizeof(int));
    return ret;
}

/**
 * @brief  got ip whether
 * @param  [in] hdl
 * @return 0/1
 */
int netmgr_is_gotip(netmgr_hdl_t hdl)
{
    CHECK_PARAM(netmgr_svc.inited, -1);

    netmgr_dev_t *node = netmgr_find_dev(&netmgr_svc.dev_list, NULL);

    if (node) {
        return node->is_gotip;
    }

    return 0;
}

/**
 * @brief  link layer is up whether
 * @param  [in] hdl
 * @return 0/1
 */
int netmgr_is_linkup(netmgr_hdl_t hdl)
{
    CHECK_PARAM(netmgr_svc.inited, -1);

    netmgr_dev_t *node = netmgr_find_dev(&netmgr_svc.dev_list, NULL);

    if (node) {
        return node->is_linkup;
    }

    return 0;
}

/**
 * @brief  dump network info to stdout(ip/gateway/netmask, etc)
 * @param  [in] hdl
 * @return 0/-1
 */
int netmgr_get_info(netmgr_hdl_t hdl)
{
    CHECK_PARAM(netmgr_svc.inited, -1);

    int ret = -1;

    aos_check_return_einval(hdl);

    uservice_call_sync(netmgr_svc.srv, API_NET_INFO, &hdl, &ret, sizeof(int));
    return ret;
}

