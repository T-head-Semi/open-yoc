/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <aos/aos.h>
#include <aos/debug.h>
#include <uservice/uservice.h>
#include <devices/netdrv.h>

#include <lwip/apps/dhcps.h>
#include <lwip/netifapi.h>
#include <lwip_netconf.h>
#include <lwip/dns.h>

#include <netif/ethernet.h>
#include <devices/wifi.h>
#include <devices/hal/wifi_impl.h>
#include <devices/rtl8723ds.h>

#include "wifi_io.h"
#include "drv/gpio.h"
#ifdef CONFIG_CSI_V2
#include "drv/pin.h"
#include <drv/gpio_pin.h>
#else
#include "pinmux.h"
#include "pin_name.h"
#endif
#include <ulog/ulog.h>

#include "wifi_constants.h"
#include "wifi_conf.h"

#include "rtl8723ds_devops.h"

#define MAX_AP_RECORD 100
#define IEEE80211_FCTL_TODS 0x0100
#define IEEE80211_FCTL_FROMDS 0x0200
#define IEEE80211_FTYPE_MGMT 0x0000
#define IEEE80211_FTYPE_CTL 0x0004
#define IEEE80211_FTYPE_DATA 0x0008
#define ETH_ALEN 6

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct {
    aos_dev_t   device;
    uint8_t mode;
    void (*write_event)(aos_dev_t *dev, int event_id, void *priv);
    void *priv;
} wifi_dev_t;

#define TAG "WIFI"

//static const char *TAG = "RTL8723DS OPS";

//advise: hal functions using global variable here may be deleted
static wifi_lpm_mode_t       g_wifi_lpm_mode;
static rtw_country_code_t    g_country;
static wifi_promiscuous_cb_t g_monitor_cb;
static wifi_mgnt_cb_t        g_monitor_mgnt_cb;
static wifi_event_func *     g_evt_func;
static wifi_config_t         g_config;
static rtl8723ds_gpio_pin    g_gpio_config;
#ifdef CONFIG_CSI_V2
csi_gpio_t  *                power_pin;
#else
static gpio_pin_handle_t     power_pin;
#endif
static aos_dev_t *               wifi_evt_dev;
static wifi_ap_record_t *    ap_records;
static int                   scan_count;
extern rtw_mode_t            wifi_mode;
static int                   wifi_disconnected_times;
static int                   sta_task_running;
static int                   wifi_open_mode;
static int                   wifi_crashed;
static int                   connect_failed_reason;

extern struct netif xnetif[NET_IF_NUM];

extern void dhcps_deinit(void);

void dhcps_set_addr_pool(int addr_pool_set, ip_addr_t *addr_pool_start, ip_addr_t *addr_pool_end);
static void rtl8723ds_wifi_handshake_done_hdl(char *buf, int buf_len, int flags, void *userdata);
static void rtl8723ds_wifi_connected_hdl(char *buf, int buf_len, int flags, void *userdata);
void        rtl8723ds_scan_report_hdl(char *buf, int buf_len, int flags, void *userdata);
static void rtl8723ds_scan_done_hdl(char *buf, int buf_len, int flags, void *userdata);


void rtl8723ds_set_invalid()
{
    wifi_crashed = 1;
}


#define DRIVER_INVALID_RETURN_VAL \
    do { \
        if (wifi_crashed) \
            return -1; \
    } while (0)

#define DRIVER_INVALID_RETURN \
    do { \
        if (wifi_crashed) \
            return; \
    } while (0)


#define RTL8723DS_NET_DRIVER
#ifdef RTL8723DS_NET_DRIVER

int rtl8723ds_set_mac_addr(aos_dev_t *dev, const uint8_t *mac)
{
    /** write this mac address will write to efuse, forbidden this */
    return -1;

#if 0
    char mac_str[18] = {0};
    sprintf(mac_str, MAC_FMT, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return wifi_set_mac_address(mac_str);
#endif
}


int rtl8723ds_get_mac_addr(aos_dev_t *dev, uint8_t *mac)
{
    DRIVER_INVALID_RETURN_VAL;

    int mac_tmp[6];
    char mac_str[18] = {0};
    
    int ret = wifi_get_mac_address(mac_str);

    sscanf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
           &mac_tmp[0], &mac_tmp[1], &mac_tmp[2],
           &mac_tmp[3], &mac_tmp[4], &mac_tmp[5]);

    mac[0] = mac_tmp[0];
    mac[1] = mac_tmp[1];
    mac[2] = mac_tmp[2];
    mac[3] = mac_tmp[3];
    mac[4] = mac_tmp[4];
    mac[5] = mac_tmp[5];

    return ret;
}

int rtl8723ds_set_dns_server(aos_dev_t *dev, ip_addr_t ipaddr[], uint32_t num)
{
    int n, i;

    n = MIN(num, DNS_MAX_SERVERS);

    for (i = 0; i < n; i ++) {
        dns_setserver(i, &ipaddr[i]);
    }

    return n;
}

int rtl8723ds_get_dns_server(aos_dev_t *dev, ip_addr_t ipaddr[], uint32_t num)
{
    int n, i;

    n = MIN(num, DNS_MAX_SERVERS);

    for (i = 0; i < n; i ++) {
        if (!ip_addr_isany(dns_getserver(i))) {
            memcpy(&ipaddr[i], dns_getserver(i), sizeof(ip_addr_t));
        } else {
            return i;
        }
    }

    return n;
}

int rtl8723ds_set_hostname(aos_dev_t *dev, const char *name)
{
#if LWIP_NETIF_HOSTNAME
    struct netif *netif = &xnetif[0];
    netif_set_hostname(netif, name);
    return 0;
#else
    return -1;
#endif
}

const char *rtl8723ds_get_hostname(aos_dev_t *dev)
{
#if LWIP_NETIF_HOSTNAME
    struct netif *netif = &xnetif[0];
    return netif_get_hostname(netif);
#else
    return NULL;
#endif
}

void net_status_callback(struct netif *netif)
{
    if (!ip_addr_isany(ip_2_ip4(&netif->ip_addr))) {
        event_publish(EVENT_NET_GOT_IP, NULL);
    }
}

static int rtl8723ds_start_dhcp(aos_dev_t *dev)
{
    DRIVER_INVALID_RETURN_VAL;

    struct netif *netif = &xnetif[0]; //netif_find("en0");
    aos_check_return_einval(netif);

    if (!netif_is_link_up(netif)) {
        return -1;
    }

    netif_set_ipaddr(netif, NULL);
    netif_set_netmask(netif, NULL);
    netif_set_gw(netif, NULL);

    netif_set_status_callback(netif, net_status_callback);

    return netifapi_dhcp_start(netif);
}

static int rtl8723ds_stop_dhcp(aos_dev_t *dev)
{
    DRIVER_INVALID_RETURN_VAL;

    struct netif *netif = &xnetif[0]; //netif_find("en0");
    aos_check_return_einval(netif);

    netifapi_dhcp_stop(netif);

    return 0;
}

static int rtl8723ds_set_ipaddr(aos_dev_t *dev, const ip_addr_t *ipaddr, const ip_addr_t *netmask,
                                const ip_addr_t *gw)
{
    return -1;
}

static int rtl8723ds_get_ipaddr(aos_dev_t *dev, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw)
{
    DRIVER_INVALID_RETURN_VAL;

    struct netif *netif = &xnetif[0]; //netif_find("en0");
    aos_check_return_einval(netif && ipaddr && netmask && gw);

    ip_addr_copy(*(ip4_addr_t *)ip_2_ip4(ipaddr), *netif_ip_addr4(netif));
    ip_addr_copy(*(ip4_addr_t *)ip_2_ip4(gw), *netif_ip_gw4(netif));
    ip_addr_copy(*(ip4_addr_t *)ip_2_ip4(netmask), *netif_ip_netmask4(netif));

    return 0;
}

int rtl8723ds_subscribe(aos_dev_t *dev, uint32_t event, event_callback_t cb, void *param)
{
    return -1;
}

int rtl8723ds_ping_remote(aos_dev_t *dev, int type, char *remote_ip)
{
    return -1;
}

static net_ops_t rtl8723ds_net_driver = {
    .set_mac_addr   = rtl8723ds_set_mac_addr,
    .get_mac_addr   = rtl8723ds_get_mac_addr,
    .set_dns_server = rtl8723ds_set_dns_server,
    .get_dns_server = rtl8723ds_get_dns_server,
    .set_hostname   = rtl8723ds_set_hostname,
    .get_hostname   = rtl8723ds_get_hostname,
    .start_dhcp     = rtl8723ds_start_dhcp,
    .stop_dhcp      = rtl8723ds_stop_dhcp,
    .set_ipaddr     = rtl8723ds_set_ipaddr,
    .get_ipaddr     = rtl8723ds_get_ipaddr,
    .subscribe      = rtl8723ds_subscribe,
    .ping           = rtl8723ds_ping_remote,
};

#endif

#define RTL8723DS_WIFI_DRIVER
#ifdef RTL8723DS_WIFI_DRIVER

static void wifi_promisc_hdl(u8 *in_buf, unsigned int buf_len, void *userdata)
{
    ieee80211_frame_info_t *    info = (ieee80211_frame_info_t *)userdata;
    wifi_promiscuous_pkt_t *    buf  = aos_malloc(sizeof(wifi_promiscuous_pkt_t) + buf_len);
    wifi_promiscuous_pkt_type_t type = WIFI_PKT_MISC;

    buf->rx_ctrl.rssi    = info->rssi;
    buf->rx_ctrl.sig_len = buf_len;
    memcpy(buf->payload, in_buf, buf_len);

    if ((info->i_fc & 0xc) == IEEE80211_FTYPE_DATA) {
        type = WIFI_PKT_DATA;
    } else if ((info->i_fc & 0xc) == IEEE80211_FTYPE_MGMT) {
        type = WIFI_PKT_MGMT;
    } else if ((info->i_fc & 0xc) == IEEE80211_FTYPE_CTL) {
        type = WIFI_PKT_CTRL;
    }

    g_monitor_cb(buf, type);
    aos_free(buf);
}

static void wifi_set_sta_mode()
{
    int mode = 0;
    //Check if in AP mode
    //wext_get_mode(WLAN0_NAME, &mode);
    DRIVER_INVALID_RETURN;

    if(rltk_wlan_running(WLAN0_IDX)) {
        wext_get_mode(WLAN0_NAME, &mode);

        if (mode == IW_MODE_INFRA) {
            return;
        }
	}

#if CONFIG_LWIP_LAYER
    dhcps_deinit();
#endif
    wifi_off();
    rtw_mdelay_os(20);

    if (wifi_on(RTW_MODE_STA) < 0) {
        LOGI(TAG, "[%s]: Wifi on failed!Do zconfig reset!", __FUNCTION__);
    }

    wext_set_mode(WLAN0_NAME, IW_MODE_INFRA);

    wifi_disable_powersave();

    return;
}

static void rtl8723ds_handshake_failed_hdl(char *buf, int buf_len, int flags, void *userdata)
{
    wifi_unreg_event_handler(WIFI_EVENT_HANDSHAKE_FAILED, rtl8723ds_handshake_failed_hdl);

    connect_failed_reason = NET_DISCON_REASON_WIFI_PSK_ERR;
}

/**
    netmgr_reset, which calls wifi_off, will also goes here
*/
static void rtl8723ds_wifi_disconn_hdl(char *buf, int buf_len, int flags, void *userdata)
{
    wifi_unreg_event_handler(WIFI_EVENT_HANDSHAKE_FAILED, rtl8723ds_handshake_failed_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, rtl8723ds_wifi_disconn_hdl);
    wifi_reg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, rtl8723ds_wifi_handshake_done_hdl,
                           NULL);
    wifi_reg_event_handler(WIFI_EVENT_CONNECT, rtl8723ds_wifi_connected_hdl, NULL);

    LOGD(TAG, "############### Disconnected ###############\n");

    netif_set_link_down(&xnetif[0]);
    event_publish(EVENT_WIFI_LINK_DOWN, (void *)(size_t)connect_failed_reason);
}

/**
    Group key handshake will also goes here
*/
static void rtl8723ds_wifi_handshake_done_hdl(char *buf, int buf_len, int flags, void *userdata)
{
    wifi_unreg_event_handler(WIFI_EVENT_HANDSHAKE_FAILED, rtl8723ds_handshake_failed_hdl);
    wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, rtl8723ds_wifi_disconn_hdl, NULL);
    wifi_unreg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, rtl8723ds_wifi_handshake_done_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_CONNECT, rtl8723ds_wifi_connected_hdl);

    wifi_disconnected_times = 0;
    LOGD(TAG, "@@@@@@@@@@@@@@ Connection Success @@@@@@@@@@@@@@\n");
    netif_set_link_up(&xnetif[0]);
    event_publish(EVENT_WIFI_LINK_UP, NULL);
}

static void rtl8723ds_wifi_connected_hdl(char *buf, int buf_len, int flags, void *userdata)
{
    if (wifi_open_mode) {
        wifi_unreg_event_handler(WIFI_EVENT_CONNECT, rtl8723ds_wifi_connected_hdl);
        wifi_unreg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE,
                                 rtl8723ds_wifi_handshake_done_hdl);
        wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, rtl8723ds_wifi_disconn_hdl, NULL);

        LOGD(TAG, "Call for 4-way handshake\n");
        rtl8723ds_wifi_handshake_done_hdl(buf, buf_len, flags, userdata);
    } else {

        wifi_unreg_event_handler(WIFI_EVENT_CONNECT, rtl8723ds_wifi_connected_hdl);
        wifi_reg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, rtl8723ds_wifi_handshake_done_hdl,
                               NULL);
        wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, rtl8723ds_wifi_disconn_hdl, NULL);

        LOGD(TAG, "Wait for 4-way handshake\n");
    }
}

int rtl8723ds_get_security(char *ssid, rtw_security_t *security_type)
{
    static int scan_done;
    int        hit = 0;
    int        i   = 0;
    int        scan_retry = 0;

    int scan_type = RTW_SCAN_COMMAMD << 4 | RTW_SCAN_TYPE_ACTIVE;
    int bss_type  = RTW_BSS_TYPE_ANY;
    u16 flags     = scan_type | (bss_type << 8);
    scan_done     = 0;

    ap_records = aos_zalloc(sizeof(wifi_ap_record_t) * MAX_AP_RECORD);
    scan_count = 0;

    wifi_reg_event_handler(WIFI_EVENT_SCAN_RESULT_REPORT, rtl8723ds_scan_report_hdl, NULL);
    wifi_reg_event_handler(WIFI_EVENT_SCAN_DONE, rtl8723ds_scan_done_hdl, &scan_done);
    wext_set_scan(WLAN0_NAME, NULL, 0, flags);

    while (scan_done == 0 && ++scan_retry < 10) {
        aos_msleep(1000);
    }

    wifi_unreg_event_handler(WIFI_EVENT_SCAN_DONE, rtl8723ds_scan_done_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_SCAN_RESULT_REPORT, rtl8723ds_scan_report_hdl);

    for (i = 0; i < scan_count; i++) {
        wifi_ap_record_t *ap_record = &ap_records[i];
        if (strcmp((char *)ap_record->ssid, ssid) == 0) {
            *security_type = ap_record->encryptmode;

            if ((ap_record->encryptmode == WIFI_ENC_OPEN) &&
                (ap_record->authmode == WIFI_AUTH_OPEN))
                *security_type = RTW_SECURITY_OPEN;
            else if ((ap_record->encryptmode == WIFI_ENC_WEP) &&
                     (ap_record->authmode == WIFI_AUTH_OPEN))
                *security_type = RTW_SECURITY_WEP_PSK;
            else if ((ap_record->encryptmode == WIFI_ENC_WEP) &&
                     (ap_record->authmode == WIFI_AUTH_WEP))
                *security_type = RTW_SECURITY_WEP_SHARED;
            else if ((ap_record->encryptmode == WIFI_ENC_TKIP) &&
                     (ap_record->authmode == WIFI_AUTH_WPA_PSK))
                *security_type = RTW_SECURITY_WPA_TKIP_PSK;
            else if ((ap_record->encryptmode == WIFI_ENC_AES) &&
                     (ap_record->authmode == WIFI_AUTH_WPA_PSK))
                *security_type = RTW_SECURITY_WPA_AES_PSK;
            else if ((ap_record->encryptmode == WIFI_ENC_AES) &&
                     (ap_record->authmode == WIFI_AUTH_WPA2_PSK))
                *security_type = RTW_SECURITY_WPA2_AES_PSK;
            else if ((ap_record->encryptmode == WIFI_ENC_TKIP) &&
                     (ap_record->authmode == WIFI_AUTH_WPA2_PSK))
                *security_type = RTW_SECURITY_WPA2_TKIP_PSK;
            else if ((ap_record->encryptmode == WIFI_ENC_TKIP_AES_MIX) &&
                     (ap_record->authmode == WIFI_AUTH_WPA2_PSK))
                *security_type = RTW_SECURITY_WPA2_MIXED_PSK;
            else if ((ap_record->encryptmode == WIFI_ENC_TKIP_AES_MIX) &&
                     (ap_record->authmode == WIFI_AUTH_WPA_WPA2_PSK))
                *security_type = RTW_SECURITY_WPA_WPA2_MIXED;
            else {
                LOGE(TAG, "Error security %d,%d\n", ap_record->encryptmode, ap_record->authmode);
                *security_type = RTW_SECURITY_UNKNOWN;
                break;
            }
            hit = 1;
            break;
        }
    }

    aos_free(ap_records);
    if (hit == 0) {
        LOGE(TAG, "Target AP not found\n");
        connect_failed_reason = NET_DISCON_REASON_WIFI_NOEXIST;
        return -1;
    } else {
        //LOGD(TAG, ">Get  security %d\n", *security_type);
        return 0;
    }
}

void wifi_start_sta_task(void *arg)
{
    DRIVER_INVALID_RETURN;

    int            ret    = 0;
    wifi_config_t *config = (wifi_config_t *)arg;

    rtw_security_t   security_type;
    char            *password;
    int              password_len;
    int              key_id;

    sta_task_running        = 1;
    wifi_disconnected_times = 0;
    connect_failed_reason = 0;

    /** avoid wifi_off send wifi disconnect event */
    wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, rtl8723ds_wifi_disconn_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, rtl8723ds_wifi_handshake_done_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_CONNECT, rtl8723ds_wifi_connected_hdl);
    
	if((rltk_wlan_running(WLAN0_IDX) != 0) ||
		(rltk_wlan_running(WLAN1_IDX) != 0)) {
		/** disable auto reconnect */
#if CONFIG_AUTO_RECONNECT
        wifi_set_autoreconnect(RTW_AUTORECONNECT_DISABLE);
#endif
    }

#if CONFIG_AUTO_RECONNECT
    wifi_wait_auto_reconnect();
#endif

    wifi_set_sta_mode();
    
#if CONFIG_UNICAST_WAKE
    wext_wowlan_unicast_wake_ctrl(WLAN0_NAME, 1);
#endif

#if CONFIG_AUTO_RECONNECT
    wifi_set_autoreconnect(RTW_AUTORECONNECT_FINITE);
#endif

    if (*(config->ssid) == '\0') {
        goto exit;
    }

    if(*(config->password) == '\0') {
        wifi_open_mode = 1;
        security_type = RTW_SECURITY_OPEN;
        password = NULL;
        password_len = 0;
        key_id = 0;
    } else {
        wifi_open_mode = 0;
        security_type = RTW_SECURITY_WPA_WPA2_MIXED;
        password = config->password;
        password_len = strlen(config->password);
        key_id = 0;
    }

    wifi_reg_event_handler(WIFI_EVENT_HANDSHAKE_FAILED, rtl8723ds_handshake_failed_hdl, NULL);

    /** ''wifi_connect' will connect only once and exit no matter connected or not */
    ret = wifi_connect(config->ssid, security_type, password,
                      strlen(config->ssid), password_len, key_id, NULL);

    if (ret == RTW_SUCCESS) {
        LOGD(TAG, "@@@@@@@@@@@@@@ Connection Success @@@@@@@@@@@@@@\n");
        netif_set_link_up(&xnetif[0]);
        event_publish(EVENT_WIFI_LINK_UP, NULL);
        wifi_unreg_event_handler(WIFI_EVENT_HANDSHAKE_FAILED, rtl8723ds_handshake_failed_hdl);
        wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, rtl8723ds_wifi_disconn_hdl, NULL);
        sta_task_running = 0;
        return;
    }

    wifi_set_sta_mode();

    LOGE(TAG, "ERROR: STA Task, wifi connect failed! try another");

    password_len = strlen(config->password);

    ret = rtl8723ds_get_security(config->ssid, &security_type);

    if (ret != RTW_SUCCESS) {
        event_publish(EVENT_WIFI_LINK_DOWN, (void *)(size_t)connect_failed_reason);
        goto exit;
    }

    if (security_type == RTW_SECURITY_UNKNOWN) {
        /* force connect  wifi */
        LOGE(TAG, "Scan fail, auto security !\n");
        if (password_len != 0)
            security_type = RTW_SECURITY_WPA_WPA2_MIXED;
        else
            security_type = RTW_SECURITY_OPEN;
    }

    if (security_type == RTW_SECURITY_OPEN) {
        if (password_len != 0) {
            LOGE(TAG, "Security mismatch1 !\n");
            password_len = 0;
        }
        password       = NULL;
        key_id         = 0;
        wifi_open_mode = 1;
    } else if (security_type == RTW_SECURITY_WEP_PSK) {
        if (password_len == 0) {
            LOGE(TAG, "Security mismatch2 !\n");
            event_publish(EVENT_WIFI_LINK_DOWN, (void *)(size_t)connect_failed_reason);
            goto exit;
        }
        password       = config->password;
        key_id         = 0;
        wifi_open_mode = 1;
    }  else {
        if (password_len == 0) {
            LOGE(TAG, "Security mismatch3 !\n");
            event_publish(EVENT_WIFI_LINK_DOWN, (void *)(size_t)connect_failed_reason);
            goto exit;
        }
        password       = config->password;
        key_id         = 0;
        wifi_open_mode = 0;
    }

    wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, rtl8723ds_wifi_disconn_hdl, NULL);

    wifi_reg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE,
                           rtl8723ds_wifi_handshake_done_hdl, NULL);
    wifi_reg_event_handler(WIFI_EVENT_CONNECT, rtl8723ds_wifi_connected_hdl, NULL);

    /** ''wifi_connect' will connect only once and exit no matter connected or not */
    ret = wifi_connect(config->ssid, security_type, password,
                       strlen(config->ssid), password_len, key_id, NULL);

    if (ret != RTW_SUCCESS) {
        LOGE(TAG, "ERROR: STA Task, wifi connect failed!: %d", ret);

        if (ret == RTW_INVALID_KEY || ret == RTW_TIMEOUT || ret == RTW_NOMEM) {
            /* password_len not match publish WIFI DOWN */
            event_publish(EVENT_WIFI_LINK_DOWN, (void *)(size_t)connect_failed_reason);
        }
        goto exit;
    } else {
        sta_task_running = 0;
        return;
    }

exit:
    wifi_unreg_event_handler(WIFI_EVENT_HANDSHAKE_FAILED, rtl8723ds_handshake_failed_hdl);
    wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, rtl8723ds_wifi_disconn_hdl, NULL);

    wifi_reg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE,
                           rtl8723ds_wifi_handshake_done_hdl, NULL);
    wifi_reg_event_handler(WIFI_EVENT_CONNECT, rtl8723ds_wifi_connected_hdl, NULL);
    sta_task_running = 0;
    //aos_task_exit(0);
}

/*
    return---- 0:succese, -1: fail
*/
int wifi_start_softap(wifi_config_t *config)
{
    DRIVER_INVALID_RETURN_VAL;

    int timeout = 20;

    ip_addr_t     ipaddr;
    ip_addr_t     netmask;
    ip_addr_t     gw;
    ip_addr_t     ipaddr_pool_start;
    ip_addr_t     ipaddr_pool_end;
    struct netif *pnetif = &xnetif[0];


    IP4_ADDR(&ipaddr, 192, 168, 1, 1);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 1, 1);
    netif_set_addr(pnetif, &ipaddr, &netmask, &gw);
    
    dhcps_deinit();
    IP4_ADDR(&ipaddr_pool_start, 192, 168, 1, 100);
    IP4_ADDR(&ipaddr_pool_end, 192, 168, 1, 115); /** limited to only 16 client */
    dhcps_set_addr_pool(1, &ipaddr_pool_start, &ipaddr_pool_end); 
    dhcps_init(pnetif);


    int channel = 6;
    int ret     = 0;
    
	if((rltk_wlan_running(WLAN0_IDX) != 0) ||
		(rltk_wlan_running(WLAN1_IDX) != 0)) {
		/** disable auto reconnect */
#if CONFIG_AUTO_RECONNECT
        wifi_set_autoreconnect(RTW_AUTORECONNECT_DISABLE);
#endif
    }

#if CONFIG_AUTO_RECONNECT
    wifi_wait_auto_reconnect();
#endif

    /** avoid wifi_off send wifi disconnect event */
    wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, rtl8723ds_wifi_disconn_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, rtl8723ds_wifi_handshake_done_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_CONNECT, rtl8723ds_wifi_connected_hdl);

    wifi_off();
    rtw_mdelay_os(20);

    if (wifi_on(RTW_MODE_AP) < 0) {
        LOGE(TAG, "Wifi on failed!");
        return -1;
    }

    //  wifi_disable_powersave();//add to close powersave
    if (channel > 14) {
        LOGE(TAG, "Invalid Channel,plese check!\r\n");
        channel = 1;
    }

    if (strlen(config->password) > 0) {
        /** Start AP with WPA/WPA2 */
        if (wifi_start_ap((char *)config->ssid, RTW_SECURITY_WPA2_AES_PSK, (char *)config->password,
                          strlen((const char *)config->ssid),
                          strlen((const char *)config->password), channel) != RTW_SUCCESS) {
            LOGE(TAG, "wifi start ap mode failed!\n\r");
            return -1;
        }
    } else {
        /** Start AP with open mode */
        if (wifi_start_ap((char *)config->ssid, RTW_SECURITY_OPEN, NULL,
                          strlen((const char *)config->ssid), 0, channel) != RTW_SUCCESS) {
            LOGE(TAG, "wifi start ap mode failed!\n\r");
            return -1;
        }
    }

    while (1) {
        char essid[33];

        if (wext_get_ssid(WLAN0_NAME, (unsigned char *)essid) > 0) {
            if (strcmp((const char *)essid, (const char *)config->ssid) == 0) {
                LOGD(TAG, "%s started\n", config->ssid);
                ret = 0;
                break;
            }
        }

        if (timeout == 0) {
            LOGE(TAG, "Start AP timeout!\n\r");
            ret = -1;
            break;
        }

        aos_msleep(1000);
        timeout--;
    }
    
    netif_set_link_up(&xnetif[0]);

    return ret;
}

/**
    The wifi mode is controlled by the global variable 'wifi_mode'
*/
static int rtl8723ds_set_mode(aos_dev_t *dev, wifi_mode_t mode)
{
    DRIVER_INVALID_RETURN_VAL;

    rtw_mode_t rtw_mode;

    switch (mode) {
        case WIFI_MODE_NULL:
            rtw_mode = RTW_MODE_NONE;
            break;

        case WIFI_MODE_STA:
            rtw_mode = RTW_MODE_STA;
            break;

        case WIFI_MODE_AP:
            rtw_mode = RTW_MODE_AP;
            break;

        case WIFI_MODE_APSTA:
            rtw_mode = RTW_MODE_STA_AP;
            break;

        case WIFI_MODE_P2P:
            rtw_mode = RTW_MODE_P2P;
            break;

        case WIFI_MODE_MAX:
        default:
            rtw_mode = RTW_MODE_NONE;
            LOGE(TAG, "ERR mode\n");
    }

    wifi_mode = rtw_mode;

    wifi_off();
    rtw_mdelay_os(20);
    wifi_on(wifi_mode);

    return 0;
}

static int rtl8723ds_get_mode(aos_dev_t *dev, wifi_mode_t *mode)
{
    DRIVER_INVALID_RETURN_VAL;

    switch (wifi_mode) {
        case RTW_MODE_NONE:
            *mode = WIFI_MODE_NULL;
            break;

        case RTW_MODE_STA:
            *mode = WIFI_MODE_STA;
            break;

        case RTW_MODE_AP:
            *mode = WIFI_MODE_AP;
            break;

        case RTW_MODE_STA_AP:
            *mode = WIFI_MODE_APSTA;
            break;

        case RTW_MODE_P2P:
            *mode = WIFI_MODE_P2P;
            break;
    }

    return 0;
}

static int rtl8723ds_install_event_cb(aos_dev_t *dev, wifi_event_func *evt_func)
{
    DRIVER_INVALID_RETURN_VAL;

    g_evt_func   = evt_func;
    wifi_evt_dev = dev;

    return -1;
}

static int rtl8723ds_init(aos_dev_t *dev)
{
    //wifi_on();
    return 0;
}

int rtl8723ds_deinit(aos_dev_t *dev)
{
    DRIVER_INVALID_RETURN_VAL;

    while (sta_task_running) {
#if CONFIG_AUTO_RECONNECT
        wifi_set_autoreconnect(RTW_AUTORECONNECT_DISABLE);
#endif
        LOGW(TAG, "%s Wait STA Task Exit!\n", __FUNCTION__);
        aos_msleep(1000);
    }
    /** avoid wifi_off send wifi disconnect event */
    wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, rtl8723ds_wifi_disconn_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, rtl8723ds_wifi_handshake_done_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_CONNECT, rtl8723ds_wifi_connected_hdl);
    netif_set_link_down(&xnetif[0]);
    return wifi_off();
}

int rtl8723ds_start(aos_dev_t *dev, wifi_config_t *config)
{
    DRIVER_INVALID_RETURN_VAL;

    if (config->mode == WIFI_MODE_STA) {
        if (sta_task_running) {
            LOGD(TAG, "ERROR: STA Task Already starting!\n");
            return -1;
        }
        memcpy(&g_config, config, sizeof(wifi_config_t));
        aos_task_t task_handle;
        sta_task_running        = 1;
        aos_task_new_ext(&task_handle, "wifi_start_sta_task", wifi_start_sta_task, &g_config, 1024 * 10, 35);
    } else if (config->mode == WIFI_MODE_AP) {
        LOGD(TAG, "start softap!\n");
        while (sta_task_running) {
            LOGW(TAG, "%s Wait STA Task Exit!\n", __FUNCTION__);
            aos_msleep(1000);
        }
        if (wifi_start_softap(config) == 0) {
            LOGI(TAG, "SoftAP UP\n");
            //m->ev_cb->stat_chg(m, NOTIFY_AP_UP, NULL);
        } else {
            LOGI(TAG, "SoftAP DOWN\n");
            //m->ev_cb->stat_chg(m, NOTIFY_AP_UP_FAILED, NULL);
        }
    } else {
        LOGD(TAG, "Not supported wifi mode!\n");
    }

    return 0;
}

int rtl8723ds_stop(aos_dev_t *dev)
{
    DRIVER_INVALID_RETURN_VAL;

    while (sta_task_running) {
#if CONFIG_AUTO_RECONNECT
        wifi_set_autoreconnect(RTW_AUTORECONNECT_DISABLE);
#endif
        LOGI(TAG, "%s Wait STA Task Exit!\n", __FUNCTION__);
        aos_msleep(1000);
    }

    netif_set_link_down(&xnetif[0]);
    return wifi_off();
}

int rtl8723ds_reset(aos_dev_t *dev)
{
    DRIVER_INVALID_RETURN_VAL;

    while (sta_task_running) {
#if CONFIG_AUTO_RECONNECT
        wifi_set_autoreconnect(RTW_AUTORECONNECT_DISABLE);
#endif
        LOGW(TAG, "%s Wait STA Task Exit!\n", __FUNCTION__);
        aos_msleep(1000);
    }
    /** avoid wifi_off send wifi disconnect event */
    wifi_unreg_event_handler(WIFI_EVENT_HANDSHAKE_FAILED, rtl8723ds_handshake_failed_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, rtl8723ds_wifi_disconn_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, rtl8723ds_wifi_handshake_done_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_CONNECT, rtl8723ds_wifi_connected_hdl);
    netif_set_link_down(&xnetif[0]);
    return wifi_off();
}

/** conf APIs */
int rtl8723ds_set_lpm(aos_dev_t *dev, wifi_lpm_mode_t mode)
{
    DRIVER_INVALID_RETURN_VAL;

    if (mode == WIFI_LPM_NONE) {

        if (g_wifi_lpm_mode == WIFI_LPM_KEEP_LINK) {
            rtw_resume(0);
        } else if (g_wifi_lpm_mode == WIFI_LPM_KEEP_SOCKET) {
            wifi_disable_powersave();
        }
    } else if (mode == WIFI_LPM_KEEP_SOCKET) {
        if (g_wifi_lpm_mode == WIFI_LPM_NONE) {
            wifi_enable_powersave();
        } else if (g_wifi_lpm_mode == WIFI_LPM_KEEP_LINK) {
            rtw_resume(0);
            wifi_enable_powersave();
        }
    } else if (mode == WIFI_LPM_KEEP_LINK) {

        rtw_suspend(0);

    } else {
        return -1;
    }

    g_wifi_lpm_mode = mode;

    return 0;
}

int rtl8723ds_get_lpm(aos_dev_t *dev, wifi_lpm_mode_t *mode)
{
    *mode = g_wifi_lpm_mode;
    return 0;
}

int rtl8723ds_set_protocol(aos_dev_t *dev, uint8_t protocol_bitmap)
{
    DRIVER_INVALID_RETURN_VAL;

    rtw_network_mode_t mode = RTW_NETWORK_B;

    if (protocol_bitmap & (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N)) {
        mode = RTW_NETWORK_BGN;
    } else if (protocol_bitmap & (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G)) {
        mode = RTW_NETWORK_BG;
    } else if (protocol_bitmap & (WIFI_PROTOCOL_11B)) {
        mode = RTW_NETWORK_B;
    }

    wifi_set_network_mode(mode);
    return 0;
}

int rtl8723ds_get_protocol(aos_dev_t *dev, uint8_t *protocol_bitmap)
{
    DRIVER_INVALID_RETURN_VAL;

    rtw_network_mode_t mode = RTW_NETWORK_B;
    wifi_get_network_mode(&mode);

    switch (mode) {
        case RTW_NETWORK_B:
            *protocol_bitmap = (WIFI_PROTOCOL_11B);
            break;

        case RTW_NETWORK_BG:
            *protocol_bitmap = (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
            break;

        case RTW_NETWORK_BGN:
            *protocol_bitmap = (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
            break;
    }

    return 0;
}

int rtl8723ds_set_country(aos_dev_t *dev, wifi_country_t country)
{
    DRIVER_INVALID_RETURN_VAL;

    rtw_country_code_t rtw_country_code;

    switch (country) {
        case WIFI_COUNTRY_CN:
            rtw_country_code = RTW_COUNTRY_CN;
            break;

        case WIFI_COUNTRY_JP:
            rtw_country_code = RTW_COUNTRY_JP;
            break;

        case WIFI_COUNTRY_US:
            rtw_country_code = RTW_COUNTRY_US;
            break;

        case WIFI_COUNTRY_EU:
            rtw_country_code = RTW_COUNTRY_EU;
            break;

        default:
            rtw_country_code = RTW_COUNTRY_CN;
    }

    g_country = rtw_country_code;
    return wifi_set_country(rtw_country_code);
}

int rtl8723ds_get_country(aos_dev_t *dev, wifi_country_t *country)
{
    DRIVER_INVALID_RETURN_VAL;

    switch (g_country) {
        case RTW_COUNTRY_CN:
            *country = WIFI_COUNTRY_CN;
            break;

        case RTW_COUNTRY_JP:
            *country = WIFI_COUNTRY_JP;
            break;

        case RTW_COUNTRY_US:
            *country = WIFI_COUNTRY_US;
            break;

        case RTW_COUNTRY_EU:
            *country = WIFI_COUNTRY_EU;
            break;

        default:
            *country = WIFI_COUNTRY_CN;
    }

    return 0;
}

/**
    When wifi is disconnected, judge if reconnect wifi
*/
int rtl8723ds_set_auto_reconnect(aos_dev_t *dev, bool en)
{
    DRIVER_INVALID_RETURN_VAL;

#if CONFIG_AUTO_RECONNECT
    if (en)
        wifi_set_autoreconnect(RTW_AUTORECONNECT_DISABLE);
    else
        wifi_set_autoreconnect(RTW_AUTORECONNECT_FINITE);

    return 0;
#else
    return -1;
#endif
}

int rtl8723ds_get_auto_reconnect(aos_dev_t *dev, bool *en)
{
    DRIVER_INVALID_RETURN_VAL;

#if CONFIG_AUTO_RECONNECT
    unsigned char enable;

    wifi_get_autoreconnect(&enable);
    if (enable != RTW_AUTORECONNECT_DISABLE)
        *en = 1;
    else
        *en = 0;
    return 0;
#else
    return -1;
#endif
}

int rtl8723ds_power_on(aos_dev_t *dev)
{
    DRIVER_INVALID_RETURN_VAL;

    return wifi_rf_on();
}

int rtl8723ds_power_off(aos_dev_t *dev)
{
    return wifi_rf_off();
}

static void rtl8723ds_scan_done_hdl(char *buf, int buf_len, int flags, void *userdata)
{

    LOGD(TAG, "scan done!\n");
    int *scan_done = (int *)userdata;

    if (g_evt_func && wifi_evt_dev)
        g_evt_func->scan_compeleted(wifi_evt_dev, scan_count, ap_records);
    *scan_done     = 1;
}

/**
    There may be duplicate entry comes in, replace old with new
*/
void rtl8723ds_scan_report_hdl(char *buf, int buf_len, int flags, void *userdata)
{
    int i = 0;
    //int j = 0;
    //int insert_pos = 0;
    rtw_scan_result_t *record = *(rtw_scan_result_t **)buf;
#if 0
    LOGD(TAG, "SSID=%s BSSID=%02x:%02x:%02x:%02x:%02x:%02x\n",
           record->SSID.val,
           record->BSSID.octet[0],
           record->BSSID.octet[1],
           record->BSSID.octet[2],
           record->BSSID.octet[3],
           record->BSSID.octet[4],
           record->BSSID.octet[5]);
#endif

    /** filtering out already exist record */
    for (i = 0; i < scan_count; i++) {
        wifi_ap_record_t *ap_record = &ap_records[i];
        if (memcmp(ap_record->bssid, record->BSSID.octet, 6) == 0) {
            if (record->signal_strength > ap_record->rssi) {
                ap_record->rssi = record->signal_strength;
            }
            return;
        }
    }

    if (scan_count >= MAX_AP_RECORD - 1) {
        LOGE(TAG, "Too many AP!\n");
        return;
    }
    wifi_ap_record_t *ap_record = &ap_records[scan_count++];
    memcpy(ap_record->ssid, record->SSID.val, record->SSID.len);
    memcpy(ap_record->bssid, record->BSSID.octet, 6);
    ap_record->channel = record->channel;
    ap_record->rssi    = record->signal_strength;

    switch (record->security) {
        case RTW_SECURITY_OPEN:
            ap_record->encryptmode = WIFI_ENC_OPEN;
            ap_record->authmode    = WIFI_AUTH_OPEN;
            break;

        case RTW_SECURITY_WEP_PSK:
            ap_record->encryptmode = WIFI_ENC_WEP;
            ap_record->authmode    = WIFI_AUTH_OPEN;
            break;

        case RTW_SECURITY_WEP_SHARED:
            ap_record->encryptmode = WIFI_ENC_WEP;
            ap_record->authmode    = WIFI_AUTH_WEP;
            break;

        case RTW_SECURITY_WPA_TKIP_PSK:
            ap_record->encryptmode = WIFI_ENC_TKIP;
            ap_record->authmode    = WIFI_AUTH_WPA_PSK;
            break;

        case RTW_SECURITY_WPA_AES_PSK:
            ap_record->encryptmode = WIFI_ENC_AES;
            ap_record->authmode    = WIFI_AUTH_WPA_PSK;
            break;

        case RTW_SECURITY_WPA2_AES_PSK:
            ap_record->encryptmode = WIFI_ENC_AES;
            ap_record->authmode    = WIFI_AUTH_WPA2_PSK;
            break;

        case RTW_SECURITY_WPA2_TKIP_PSK:
            ap_record->encryptmode = WIFI_ENC_TKIP;
            ap_record->authmode    = WIFI_AUTH_WPA2_PSK;
            break;

        case RTW_SECURITY_WPA2_MIXED_PSK:
            ap_record->encryptmode = WIFI_ENC_TKIP_AES_MIX;
            ap_record->authmode    = WIFI_AUTH_WPA2_PSK;
            break;

        case RTW_SECURITY_WPA_WPA2_MIXED:
            ap_record->encryptmode = WIFI_ENC_TKIP_AES_MIX;
            ap_record->authmode    = WIFI_AUTH_WPA_WPA2_PSK;
            break;

        default:
            LOGE(TAG, "WiFi encrypt not support = %d\n", record->security);
            ap_record->encryptmode = WIFI_ENC_MAX;
            ap_record->authmode    = WIFI_AUTH_MAX;
    }

    return;
}

static int rtl8723ds_block_scan()
{
    int ret = 0;

    static int scan_done;
    static int scan_running;

    if (scan_running) {
        LOGE(TAG, "scan in progress\n");
        return -1;
    }
    scan_running = 1;

    int scan_type = RTW_SCAN_COMMAMD << 4 | RTW_SCAN_TYPE_ACTIVE;
    int bss_type  = RTW_BSS_TYPE_ANY;
    u16 flags     = scan_type | (bss_type << 8);
    scan_done     = 0;

    ap_records = aos_zalloc(sizeof(wifi_ap_record_t) * MAX_AP_RECORD);
    scan_count = 0;

    wifi_reg_event_handler(WIFI_EVENT_SCAN_RESULT_REPORT, rtl8723ds_scan_report_hdl, NULL);
    wifi_reg_event_handler(WIFI_EVENT_SCAN_DONE, rtl8723ds_scan_done_hdl, &scan_done);
    ret = wext_set_scan(WLAN0_NAME, NULL, 0, flags);

    while (scan_done == 0)
        aos_msleep(100);

    //ret = wext_get_scan(WLAN0_NAME, NULL, 0);
    wifi_unreg_event_handler(WIFI_EVENT_SCAN_DONE, rtl8723ds_scan_done_hdl);
    wifi_unreg_event_handler(WIFI_EVENT_SCAN_RESULT_REPORT, rtl8723ds_scan_report_hdl);
    //LOGD(TAG, "scan count=(All=%d, not duplicate=%d)\n", ret, scan_count);

    scan_running = 0;
    aos_free(ap_records);

    return ret;
    //#endif
}

/**
    TBD, scan with scan config
*/
int rtl8723ds_start_scan(aos_dev_t *dev, wifi_scan_config_t *config, bool block)
{
    int ret = 0;
    DRIVER_INVALID_RETURN_VAL;

    //only support block mode now
    if (block == 0)
        return -1;

    wifi_set_sta_mode();

#if 0
    if ((ret = wifi_scan_networks(wifi_scan_result_handler, NULL)) != RTW_SUCCESS) {
        LOGD(TAG, "[ATWS]ERROR: wifi scan failed\n\r");
        goto exit;
    }
#else
    ret = rtl8723ds_block_scan();
#endif

    //exit:
    return ret;
}

int rtl8723ds_sta_get_link_status(aos_dev_t *dev, wifi_ap_record_t *ap_info)
{
    DRIVER_INVALID_RETURN_VAL;

    const char *       ifname[2] = {WLAN0_NAME, WLAN1_NAME};
    rtw_wifi_setting_t setting;
    //int rssi = 0;
    int channel = 0;

    if (wifi_is_connected_to_ap() == RTW_SUCCESS) {
        ap_info->link_status = WIFI_STATUS_LINK_UP;
    } else {
        ap_info->link_status = WIFI_STATUS_LINK_DOWN;
        return 0;
    }

    wifi_get_setting((const char *)ifname[0], &setting);
    //wifi_show_setting((const char *)ifname[0], &setting);
    strcpy((char*)ap_info->ssid, (char*)setting.ssid);

    if (wifi_get_ap_bssid(ap_info->bssid) == RTW_ERROR)
        LOGE(TAG, "AP SSID Get Failed\n");
    wifi_get_rssi((int*)&ap_info->rssi);
    wifi_get_channel(&channel);

    ap_info->channel = channel;

    return 0;
}

int rtl8723ds_ap_get_sta_list(aos_dev_t *dev, wifi_sta_list_t *sta)
{
    DRIVER_INVALID_RETURN_VAL;

    int i;
    struct {
        int       count;
        rtw_mac_t mac_list[AP_STA_NUM];
    } client_info;

    client_info.count = AP_STA_NUM;
    wifi_get_associated_client_list(&client_info, sizeof(client_info));

    LOGD(TAG, "\n\rAssociated Client List:");
    LOGD(TAG, "\n\r==============================");

    sta->num = client_info.count;

    if (client_info.count == 0) {
        LOGD(TAG, "\n\rClient Num: 0\n\r");
    } else {
        LOGD(TAG, "\n\rClient Num: %d", client_info.count);

        for (i = 0; i < client_info.count; i++) {
            if (i >= HAL_WIFI_MAX_CONN_NUM) {
                LOGD(TAG, "WiFi connection exceeds max number of HAL\n");
                break;
            }

            memcpy(sta->sta[i].mac, client_info.mac_list[i].octet, 6);

            LOGD(TAG, "\n\rClient [%d]:", i);
            LOGD(TAG, "\n\r\tMAC => " MAC_FMT "", MAC_ARG(client_info.mac_list[i].octet));
        }

        LOGD(TAG, "\n\r");
    }

    return 0;
}

static int frame_handler(const unsigned char* frame_buf, unsigned int frame_len)
{
#ifdef WIFI_PORTING_DISABLED
    return 0;
#endif

    g_monitor_mgnt_cb((uint8_t*)frame_buf, frame_len);

    return 0;
}

int rtl8723ds_start_mgnt_monitor(aos_dev_t *dev, wifi_mgnt_cb_t cb)
{
    DRIVER_INVALID_RETURN_VAL;

#if CONFIG_AUTO_RECONNECT
    wifi_set_autoreconnect(RTW_AUTORECONNECT_DISABLE);
    wifi_wait_auto_reconnect();
#endif
    while (sta_task_running) {
        LOGI(TAG, "%s Wait STA Task Exit!\n", __FUNCTION__);
        aos_msleep(1000);
    }

    g_monitor_mgnt_cb = cb;

    wext_enable_rawdata_recv(WLAN0_NAME, (void*)frame_handler);

    return 0;
}

int rtl8723ds_stop_mgnt_monitor(aos_dev_t *dev)
{
    DRIVER_INVALID_RETURN_VAL;

    wext_disable_rawdata_recv(WLAN0_NAME);

    return 0;
}

int rtl8723ds_start_monitor(aos_dev_t *dev, wifi_promiscuous_cb_t cb)
{
    DRIVER_INVALID_RETURN_VAL;

#if CONFIG_AUTO_RECONNECT
    wifi_set_autoreconnect(RTW_AUTORECONNECT_DISABLE);
    wifi_wait_auto_reconnect();
#endif
    while (sta_task_running) {
        LOGI(TAG, "%s Wait STA Task Exit!\n", __FUNCTION__);
        aos_msleep(1000);
    }

    g_monitor_cb = cb;

    wifi_enter_promisc_mode();
    wifi_set_promisc(RTW_PROMISC_ENABLE_2, wifi_promisc_hdl, 0);

    return 0;
}

int rtl8723ds_stop_monitor(aos_dev_t *dev)
{
    DRIVER_INVALID_RETURN_VAL;

    wifi_set_promisc(RTW_PROMISC_DISABLE, NULL, 0);

    return 0;
}

int rtl8723ds_set_channel(aos_dev_t *dev, uint8_t primary, wifi_second_chan_t second)
{
    DRIVER_INVALID_RETURN_VAL;

    return wifi_set_channel(primary);
}

int rtl8723ds_get_channel(aos_dev_t *dev, uint8_t *primary, wifi_second_chan_t *second)
{
    DRIVER_INVALID_RETURN_VAL;

    int channel;
    if (wifi_get_channel(&channel) == 0) {
        *primary = channel;
        return 0;
    } else {
        return -1;
    }
}

static int rtl8723ds_send_80211_raw_frame(aos_dev_t *dev, void *buffer, uint16_t len)
{
    DRIVER_INVALID_RETURN_VAL;

    int         ret    = 0;
    const char *ifname = WLAN0_NAME;

    ret = wext_send_mgnt(ifname, (char *)buffer, len, 1);
    return ret;
}

static wifi_driver_t rtl8723ds_wifi_driver = {
    /** basic APIs */

    .init             = rtl8723ds_init,
    .deinit           = rtl8723ds_deinit,
    .reset            = rtl8723ds_reset,
    .set_mode         = rtl8723ds_set_mode,
    .get_mode         = rtl8723ds_get_mode,
    .install_event_cb = rtl8723ds_install_event_cb,

    .set_protocol        = rtl8723ds_set_protocol,
    .get_protocol        = rtl8723ds_get_protocol,
    .set_country         = rtl8723ds_set_country,
    .get_country         = rtl8723ds_get_country,
    .set_mac_addr        = rtl8723ds_set_mac_addr,
    .get_mac_addr        = rtl8723ds_get_mac_addr,
    .set_auto_reconnect  = rtl8723ds_set_auto_reconnect,
    .get_auto_reconnect  = rtl8723ds_get_auto_reconnect,
    .set_lpm             = rtl8723ds_set_lpm,
    .get_lpm             = rtl8723ds_get_lpm,
    .power_on            = rtl8723ds_power_on,
    .power_off           = rtl8723ds_power_off,
    .start_scan          = rtl8723ds_start_scan,
    .start               = rtl8723ds_start,
    .stop                = rtl8723ds_stop,
    .sta_get_link_status = rtl8723ds_sta_get_link_status,
    .ap_get_sta_list     = rtl8723ds_ap_get_sta_list,

    /** promiscuous APIs */
    .start_monitor        = rtl8723ds_start_monitor,
    .stop_monitor         = rtl8723ds_stop_monitor,
    .start_mgnt_monitor   = rtl8723ds_start_mgnt_monitor,
    .stop_mgnt_monitor    = rtl8723ds_stop_mgnt_monitor,
    .send_80211_raw_frame = rtl8723ds_send_80211_raw_frame,
    .set_channel          = rtl8723ds_set_channel,
    .get_channel          = rtl8723ds_get_channel,

    .set_smartcfg = NULL,
};

#endif

#define RTL8723DS_DRIVER
#ifdef RTL8723DS_DRIVER
static aos_dev_t *rtl8723ds_dev_init(driver_t *drv, void *config, int id)
{
    aos_dev_t *dev = device_new(drv, sizeof(wifi_dev_t), id);

    return dev;
}

static void rtl8723ds_dev_uninit(aos_dev_t *dev)
{
    aos_check_param(dev);

    device_free(dev);
}

static int rtl8723ds_dev_open(aos_dev_t *dev)
{
    return 0;
}

static int rtl8723ds_dev_close(aos_dev_t *dev)
{
    return 0;
}
#endif

static netdev_driver_t rtl8723ds_driver = {
    .drv =
        {
            .name   = "wifi",
            .init   = rtl8723ds_dev_init,
            .uninit = rtl8723ds_dev_uninit,
            .open   = rtl8723ds_dev_open,
            .close  = rtl8723ds_dev_close,
        },
    .link_type = NETDEV_TYPE_WIFI,
    .net_ops   = &rtl8723ds_net_driver,
    .link_ops  = &rtl8723ds_wifi_driver,
};

#if 0
static void rtl8723ds_cut_off_gpio(int gpio)
{
    LOGD(TAG, "Turn off %d\n", gpio);
    drv_pinmux_config(gpio, PIN_FUNC_GPIO);
    gpio_pin_handle_t pin = csi_gpio_pin_initialize(gpio, NULL);
    csi_gpio_pin_config_mode(pin, GPIO_MODE_PULLNONE);
    csi_gpio_pin_config_direction(pin, GPIO_DIRECTION_INPUT);
    //csi_gpio_pin_write(pin, 1);
}
#endif

/**
 * @brief  register wifi driver of rtl8723 
 * @param  [in] config
 * @return  
 */
void wifi_rtl8723ds_register(rtl8723ds_gpio_pin *config)
{
    if (config) {
        memcpy(&g_gpio_config, config, sizeof(rtl8723ds_gpio_pin));
    } else {
        LOGE(TAG, "Err:Please input gpio config\n");
        return;
    }

#ifdef CONFIG_CSI_V2
    extern csi_gpio_t chip_gpio_handler;
    if (config->power != 0xFFFFFFFF) {
        LOGD(TAG, "pull up WLAN power\n");
        csi_pin_set_mux(g_gpio_config.power, PIN_FUNC_GPIO);
#if defined(CONFIG_CHIP_D1)
        (void)chip_gpio_handler;
        csi_gpio_pin_t power_pin;
        csi_gpio_pin_init(&power_pin, g_gpio_config.power);
        csi_gpio_pin_mode(&power_pin, GPIO_MODE_PULLNONE);
        csi_gpio_pin_dir(&power_pin, GPIO_DIRECTION_OUTPUT);
        csi_gpio_pin_write(&power_pin, GPIO_PIN_HIGH);
        aos_msleep(50);
#else
        uint32_t pin_mask = csi_pin_get_gpio_channel(g_gpio_config.power);
        csi_gpio_mode(&chip_gpio_handler, pin_mask, GPIO_MODE_PUSH_PULL);
        csi_gpio_dir(&chip_gpio_handler, pin_mask, GPIO_DIRECTION_OUTPUT);
        csi_gpio_write(&chip_gpio_handler, pin_mask, 1);
        aos_msleep(200);
#endif
    }
    if (config->wl_en != 0xFFFFFFFF) {
        LOGD(TAG, "Init WLAN enable\n");
        csi_pin_set_mux(g_gpio_config.wl_en, PIN_FUNC_GPIO);
#if defined(CONFIG_CHIP_D1)
        csi_gpio_pin_t wl_en_pin;
        csi_gpio_pin_init(&wl_en_pin, g_gpio_config.wl_en);
        csi_gpio_pin_mode(&wl_en_pin, GPIO_MODE_PULLNONE);
        csi_gpio_pin_dir(&wl_en_pin, GPIO_DIRECTION_OUTPUT);
        csi_gpio_pin_write(&wl_en_pin, GPIO_PIN_LOW);
        aos_msleep(50);
        csi_gpio_pin_write(&wl_en_pin, GPIO_PIN_HIGH);
        aos_msleep(50);
#else
        uint32_t pin_mask = csi_pin_get_gpio_channel(g_gpio_config.wl_en);
        csi_gpio_mode(&chip_gpio_handler, pin_mask, GPIO_MODE_PUSH_PULL);
        csi_gpio_dir(&chip_gpio_handler, pin_mask, GPIO_DIRECTION_OUTPUT);
        csi_gpio_write(&chip_gpio_handler, pin_mask, 0);
        aos_msleep(50);
        csi_gpio_write(&chip_gpio_handler, pin_mask, 1);
        aos_msleep(50);
#endif
    }
#else
    gpio_pin_handle_t wl_en_pin;

    /** main power up*/
    if (config->power != 0xFFFFFFFF) {
        LOGD(TAG, "pull up WLAN power\n");
        drv_pinmux_config(g_gpio_config.power, PIN_FUNC_GPIO);
        power_pin = csi_gpio_pin_initialize(g_gpio_config.power, NULL);
        csi_gpio_pin_config_mode(power_pin, GPIO_MODE_PULLNONE);
        csi_gpio_pin_config_direction(power_pin, GPIO_DIRECTION_OUTPUT);
        csi_gpio_pin_write(power_pin, 1);
        aos_msleep(200);
    }

    /** WL_EN */
    if (config->wl_en != 0xFFFFFFFF) {
        LOGD(TAG, "Init WLAN enable\n");
        drv_pinmux_config(g_gpio_config.wl_en, PIN_FUNC_GPIO);
        wl_en_pin = csi_gpio_pin_initialize(g_gpio_config.wl_en, NULL);
        csi_gpio_pin_config_mode(wl_en_pin, GPIO_MODE_PULLNONE);
        csi_gpio_pin_config_direction(wl_en_pin, GPIO_DIRECTION_OUTPUT);
        csi_gpio_pin_write(wl_en_pin, 0);
        aos_msleep(50);
        csi_gpio_pin_write(wl_en_pin, 1);
        aos_msleep(50);
    }
#endif    

    extern SDIO_BUS_OPS rtw_sdio_bus_ops;
    rtw_sdio_bus_ops.bus_probe(0);

    LwIP_Init();

    wifi_manager_init();

    driver_register(&rtl8723ds_driver.drv, NULL, 0);
}

void rtl8723ds_wifi_test()
{
    while (1) {

        int total = 0, used = 0, mfree = 0, peak = 0;
        aos_get_mminfo(&total, &used, &mfree, &peak);
        LOGD(TAG, "                total   used   free   peak \r\n");
        LOGD(TAG, "memory usage: %7d%7d%7d%7d\r\n\r\n", total, used, mfree, peak);

        wifi_off();
        rtw_mdelay_os(20);
        wifi_on(RTW_MODE_STA);

        aos_msleep(5000);
    }
}
