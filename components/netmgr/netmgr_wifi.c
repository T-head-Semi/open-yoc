/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <aos/aos.h>
#include <uservice/uservice.h>
#include <uservice/eventid.h>
#include <yoc/netmgr.h>
#include <yoc/netmgr_service.h>

#include <devices/netdrv.h>
#include <devices/wifi.h>
#ifdef CONFIG_KV_SMART
#include <aos/kv.h>
#endif

typedef struct {
    netmgr_hdl_t hdl;
    wifi_ssid_psk_t ssid_psk;
} param_ssid_psk_t;

static const char *TAG = "netmgr_wifi";

static int netmgr_wifi_provision(netmgr_dev_t *node)
{

    aos_dev_t *dev = node->dev;
    wifi_setting_t *config = &node->config.wifi_config;
    //wifi_ap_record_t ap_info;
    //hal_wifi_sta_get_link_status(dev, &ap_info);

    netmgr_subscribe(EVENT_WIFI_LINK_UP);
    netmgr_subscribe(EVENT_WIFI_LINK_DOWN);
    netmgr_subscribe(EVENT_WIFI_EXCEPTION);
    netmgr_subscribe(EVENT_WIFI_SMARTCFG);

    if (hal_wifi_reset(dev) < 0) {
        event_publish(EVENT_NETMGR_NET_DISCON, NULL);
        return -1;
    }

    //hal_wifi_set_mode(dev, WIFI_MODE_STA);
    wifi_config_t *wifi_config = aos_zalloc(sizeof(wifi_config_t));
    if (wifi_config == NULL)
        return -1;
    wifi_config->mode = WIFI_MODE_STA;
    strcpy(wifi_config->ssid, config->ssid_psk.ssid);
    strcpy(wifi_config->password, config->ssid_psk.psk);
    int ret = hal_wifi_start(dev, wifi_config);
    aos_free(wifi_config);

    if (ret == 0) {
        LOGI(TAG, "ssid{%s}, psk{%s}\n", config->ssid_psk.ssid, config->ssid_psk.psk);
    } else {
        LOGW(TAG, "no ap info");
        return -1;
    }

    return 0;
}


static int netmgr_wifi_unprovision(netmgr_dev_t *node)
{
    aos_dev_t *dev = node->dev;

    netmgr_unsubscribe(EVENT_WIFI_LINK_UP);
    netmgr_unsubscribe(EVENT_WIFI_LINK_DOWN);
    netmgr_unsubscribe(EVENT_WIFI_EXCEPTION);
    netmgr_unsubscribe(EVENT_WIFI_SMARTCFG);

    hal_wifi_deinit(dev);

    return 0;
}


static int netmgr_wifi_reset(netmgr_dev_t *node)
{
    return netmgr_wifi_provision(node);
}

static int netmgr_wifi_info(netmgr_dev_t *node)
{

    aos_dev_t *dev = node->dev;
    int ret, i;

    unsigned char mac[6] = {0};
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;
    ip_addr_t dns_svr[2];

    wifi_ap_record_t ap_info = {0};


    /** ifconfig */
    hal_net_get_ipaddr(dev, &ipaddr, &netmask, &gw);
    hal_net_get_mac_addr(dev, mac);
    ret = hal_net_get_dns_server(dev, dns_svr, 2);

    printf("\nwifi0\tLink encap:WiFi  HWaddr ");
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("    \tinet addr:%s\n", ipaddr_ntoa(&ipaddr));
    printf("\tGWaddr:%s\n", ipaddr_ntoa(&gw));
    printf("\tMask:%s\n", ipaddr_ntoa(&netmask));

    if (ret <= 0) {
        printf("\tDNS SERVER NONE\n");
    }

    for (i = 0; i < ret; i ++) {
        printf("\tDNS SERVER %d: %s\n", i, ipaddr_ntoa(&dns_svr[i]));
    }

    /** iw dev wlan0 link */
    hal_wifi_sta_get_link_status(dev, &ap_info);

    if ((ap_info.link_status == WIFI_STATUS_LINK_UP) ||
        (ap_info.link_status == WIFI_STATUS_GOTIP)) {
        printf("\nWiFi Connected to %02x:%02x:%02x:%02x:%02x:%02x (on wifi0)\n",
               ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
               ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
        printf("\tSSID: %s\n", ap_info.ssid);
        printf("\tchannel: %d\n", ap_info.channel);
        //printf("\tRX: %d bytes (%d packets)\n", 0, 0); //TBD
        //printf("\tTX: %d bytes (%d packets)\n", 0, 0); //TBD
        printf("\tsignal: %d dBm\n", ap_info.rssi);
        //printf("\tx bitrate: %d MBit/s\n", 0); //TBD

    }

    if (ap_info.link_status == WIFI_STATUS_LINK_DOWN) {
        printf("\tWiFi Not connected\n");
    }

    return 0;
}


static int wifi_cfg_ssid_psk(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    int ret = 0;

    netmgr_dev_t *node = NULL;
    param_ssid_psk_t *param = rpc_get_point(rpc);

    if (param != NULL) {
        node = (netmgr_dev_t *)param->hdl;
    } else {
        ret = -EINVAL;
    }

    if (node != NULL) {
        //aos_dev_t *dev = node->dev;
        //netdev_driver_t *drv = dev->drv;
        wifi_setting_t *config = &node->config.wifi_config;

        if ((param->ssid_psk.ssid_length > 0) && (param->ssid_psk.psk_length >= 0)) {
            config->ssid_psk = param->ssid_psk;

#ifdef CONFIG_KV_SMART
            aos_kv_set(KV_WIFI_PSK, (void *)config->ssid_psk.psk, config->ssid_psk.psk_length, 1);
            aos_kv_set(KV_WIFI_SSID, (void *)config->ssid_psk.ssid, config->ssid_psk.ssid_length, 1);
#endif
        } else {
            ret = -EBADFD;
        }
    } else {
        ret = -EBADFD;
    }

    return ret;
}

static int wifi_evt_link_up(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    netmgr_set_linkup("wifi", 1);
    return netmgr_start_dhcp(netmgr, "wifi");
}

static int wifi_evt_link_down(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    int reason = rpc_get_int(rpc);
    static int link_reason;

    link_reason = reason ? reason : NET_DISCON_REASON_WIFI_TIMEOUT;

    netmgr_set_gotip("wifi", 0);
    netmgr_set_linkup("wifi", 0);
    event_publish(EVENT_NETMGR_NET_DISCON, &link_reason);
    return 0;
}

static int wifi_evt_link_execept(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    static int link_reason = NET_DISCON_REASON_ERROR;
    netmgr_set_gotip("wifi", 0);
    netmgr_set_linkup("wifi", 0);
    event_publish(EVENT_NETMGR_NET_DISCON, &link_reason);
    return 0;
}

static int wifi_evt_smartcfg(struct netmgr_uservice *netmgr, rpc_t *rpc)
{
    netmgr_dev_t *node = netmgr_find_dev(&netmgr->dev_list, "wifi");
    aos_dev_t *dev = node->dev;

    static int do_action = 1;

    if (do_action) {
        hal_wifi_set_smartcfg(dev, do_action);
        do_action = 0;
        LOGI(TAG, "Smartconfig ENABLE");
    } else {
        hal_wifi_set_smartcfg(dev, do_action);
        do_action = 1;
        LOGI(TAG, "Smartconfig DISABLE");
    }

    return 0;
}

static netmgr_dev_t *netmgr_wifi_init(struct netmgr_uservice *netmgr)
{
    netmgr_dev_t *node = NULL;

    int ival = 0;
    // get wifi configuration
#ifdef CONFIG_KV_SMART
    ival = netmgr_kv_getint(KV_WIFI_EN);
#else
    ival = 1;
#endif
    ival = 1;

    if (ival == 1) {

        netmgr_reg_srv_func(API_WIFI_CONFIG_SSID_PSK, wifi_cfg_ssid_psk);
        netmgr_reg_srv_func(EVENT_WIFI_LINK_UP, wifi_evt_link_up);
        netmgr_reg_srv_func(EVENT_WIFI_LINK_DOWN, wifi_evt_link_down);
        netmgr_reg_srv_func(EVENT_WIFI_EXCEPTION, wifi_evt_link_execept);
        netmgr_reg_srv_func(EVENT_WIFI_SMARTCFG, wifi_evt_smartcfg);

        node = (netmgr_dev_t *)aos_zalloc(sizeof(netmgr_dev_t));

        if (node) {
            wifi_setting_t *config = &node->config.wifi_config;
            node->dev = device_open_id("wifi", 0);
            aos_assert(node->dev);
            node->provision = netmgr_wifi_provision;
            node->unprovision = netmgr_wifi_unprovision;
            node->info = netmgr_wifi_info;
            node->reset = netmgr_wifi_reset;
            node->dhcp_en = 1; //wifi just support dhcp
            strcpy(node->name, "wifi");

            memset(config->ssid_psk.ssid, 0, WIFI_SSID_MAX_LEN + 1);
            config->ssid_psk.ssid_length = WIFI_SSID_MAX_LEN;
            memset(config->ssid_psk.psk, 0, WIFI_PSK_MAX_LEN + 1);
            config->ssid_psk.psk_length = WIFI_PSK_MAX_LEN;
#ifdef CONFIG_KV_SMART
            if ( aos_kv_get(KV_WIFI_SSID, config->ssid_psk.ssid, &config->ssid_psk.ssid_length) < 0 ||
                 aos_kv_get(KV_WIFI_PSK, config->ssid_psk.psk, &config->ssid_psk.psk_length) < 0) {

                strcpy(config->ssid_psk.ssid, "SSID_Undef");
                config->ssid_psk.ssid_length = 11;
            }
#endif
            slist_add_tail((slist_t *)node, &netmgr->dev_list);

            hal_wifi_init(node->dev);
        }


    }

    return (netmgr_hdl_t)node;

}

/**
 * @brief deinit the net manage handle
 * @param  [in] hdl
 * @return
 */
void netmgr_dev_wifi_deinit(netmgr_hdl_t hdl)
{
    netmgr_dev_t *node = (netmgr_dev_t *)hdl;
    aos_dev_t *dev = node->dev;
    netmgr_unreg_srv_func(API_WIFI_CONFIG_SSID_PSK, wifi_cfg_ssid_psk);
    netmgr_unreg_srv_func(EVENT_WIFI_LINK_UP, wifi_evt_link_up);
    netmgr_unreg_srv_func(EVENT_WIFI_LINK_DOWN, wifi_evt_link_down);
    netmgr_unreg_srv_func(EVENT_WIFI_EXCEPTION, wifi_evt_link_execept);
    netmgr_unreg_srv_func(EVENT_WIFI_SMARTCFG, wifi_evt_smartcfg);

    hal_wifi_deinit(dev);
    device_close(dev);
    slist_del((slist_t *)node, &netmgr_svc.dev_list);
    aos_free(node);
}

/**
 * @brief net manage init for wifi
 * @return NULL on error
 */
netmgr_hdl_t netmgr_dev_wifi_init()
{
    return netmgr_wifi_init(&netmgr_svc);
}

/**
 * @brief  net manage config for wifi device
 * @param  [in] hdl
 * @param  [in] ssid
 * @param  [in] ssid_length
 * @param  [in] psk
 * @param  [in] psk_length
 * @return 0 on success
 */
int netmgr_config_wifi(netmgr_hdl_t hdl, char *ssid, uint8_t ssid_length, char *psk, uint8_t psk_length)
{
    int ret = -1;
    param_ssid_psk_t param;
    wifi_ssid_psk_t *ssid_psk = &param.ssid_psk;

    aos_check_return_einval(ssid && ssid_length);

    if (psk_length > WIFI_PSK_MAX_LEN || ssid_length > WIFI_SSID_MAX_LEN) {
        return -EINVAL;
    }

    param.hdl = hdl;

    memcpy(ssid_psk->ssid, ssid, ssid_length);
    ssid_psk->ssid_length = ssid_length;

    if (psk_length == 0) {
        LOGI(TAG, "Wifi PSK is empty!");
    } else {
        memcpy(ssid_psk->psk, psk, psk_length);
    }
    ssid_psk->psk_length = psk_length;

    /* adpator string */
    ssid_psk->ssid[ssid_length] = '\0';
    ssid_psk->psk[psk_length] = '\0';

    uservice_call_sync(netmgr_svc.srv, API_WIFI_CONFIG_SSID_PSK, &param, &ret, sizeof(int));

    return ret;
}
