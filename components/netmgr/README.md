# Change log
20210317
 1. 修复无法设置空密码的bug

# 网络管理器(netmgr)

## 概述

网络管理器负责所有网络的联网管理功能，具体如下：

- 链路层的开启/初始化/配置/复位等
- 链路层设备信息的获取
- 链路层事件的处理和转发(linkup,linkdown)
- 网络IP的配置（DHCP或者静态IP）

Netmgr根据联网管理的需要，调用到对应设备的网络层和链路层接口函数。YoC将不同类型的网络连接外设都看作是具有网络层功能和链路层功能的外设，这样就可以统一所有的网络设备。

Netmgr各层次功能如下：

- HAL层：是驱动适配层的函数指针封装。
- 驱动适配层：实现网络层接口(如mac、dns、ip地址等获取和配置)和链路层接口(如对于wifi设备来说，需要对接wifi_impl.h定义的接口)
- 设备实现层：实现了网络连接设备相关的一些操作

## 组件安装

```bash
yoc init
yoc install  netmgr
```

## 接口列表

| 函数                   | 说明                     |
| :--------------------- | :----------------------- |
| netmgr_dev_eth_init    | 有线网络设备初始化       |
| netmgr_dev_gprs_init   | GPRS设备初始化           |
| netmgr_dev_wifi_init   | 无线设备初始化           |
| netmgr_dev_nbiot_init  | NB-IOT设备初始化         |
| netmgr_service_init    | 服务初始化               |
| netmgr_config_wifi     | 无线配置                 |
| netmgr_config_eth      | 有线配置                 |
| netmgr_config_gprs     | GPRS配置                 |
| netmgr_start           | 使能网络设备连接         |
| netmgr_reset           | 重置网络连接             |
| netmgr_stop            | 停止网络连接             |
| netmgr_get_handle      | 获取设备句柄             |
| netmgr_is_gotip        | 网络设备是否获取到ip     |
| netmgr_dev_wifi_deinit | 删除网络管理             |
| netmgr_get_dev         | 通过网络网络句柄获取设备 |
| netmgr_ipconfig        | netmgr配置IP或者使能dhcp |
| netmgr_is_linkup       | 链路层是否连接           |
| netmgr_get_info        | 存储网络信息             |

## 接口详细说明

### netmgr_dev_eth_init

`netmgr_hdl_t netmgr_dev_eth_init();`

- 功能描述:
  - 有线网络设备初始化 。

- 参数:
  - 无。

- 返回值:
  - NULL: 错误。
  - 非NULL: 初始化成功。

### netmgr_dev_gprs_init

`netmgr_hdl_t netmgr_dev_gprs_init();`

- 功能描述:
  - GPRS设备初始化 。

- 参数:
  - 无。

- 返回值:
  - NULL: 错误。
  - 非NULL: 初始化成功。

### netmgr_dev_wifi_init

`netmgr_hdl_t netmgr_dev_wifi_init();`

- 功能描述:
  - 无线设备初始化。

- 参数:
  - 无。

- 返回值:
  - NULL: 错误。
  - 非NULL: 初始化成功。

### netmgr_dev_nbiot_init

`netmgr_hdl_t netmgr_dev_nbiot_init();`

- 功能描述:
  - NB-IOT设备初始化。

- 参数:
  - 无。

- 返回值:
  - NULL: 错误。
  - 非NULL: 初始化成功。

### netmgr_service_init

`void netmgr_service_init(utask_t *task)`

- 功能描述:
  - 服务初始化。

- 参数:
  - task: 外部微任务。

- 返回值:
  - 无。

### netmgr_config_wifi

`int netmgr_config_wifi(netmgr_hdl_t hdl, char *ssid, uint8_t ssid_length, char *psk, uint8_t psk_length);`

- 功能描述:
  - 无线配置。

- 参数:
  - hdl: wifi句柄。
- ssid: 无线网络名称。
  - ssid_length: 无线网络名称长度。
  - psk: 无线网络密码。
  - psk_length: 无线网络密码长度。
  
- 返回值:
  - 0: 成功。
  - -1: 失败。

### netmgr_config_eth

`int netmgr_config_eth(netmgr_hdl_t hdl, eth_mac_duplex_e duplex, eth_mac_speed_e speed);`

- 功能描述:
  - 有线配置。

- 参数:
  - hdl: 有线网络句柄。
- duplex: 有线网卡的双工模式。
  - speed: 工作速率。
  
- 返回值:
  - 0: 成功。
  - -1: 失败。

### netmgr_config_gprs

`int netmgr_config_gprs(netmgr_hdl_t hdl, int mode);`

- 功能描述:
  - GPRS配置。

- 参数:
  - hdl: GPRS句柄。
- mode: GPRS的工作模式。
  
- 返回值:
  - 0: 成功。
  - -1: 失败。

### netmgr_start

`int netmgr_start(netmgr_hdl_t hdl);`

- 功能描述:
  - 使能网络设备连接。

- 参数:
  - hdl: 网络管理器句柄。

- 返回值:
  - 0: 成功。
  - -1: 失败。

### netmgr_reset

`int netmgr_reset(netmgr_hdl_t hdl, uint32_t sec);`

- 功能描述:
  - 重置网络连接。

- 参数:
  - hdl: 网络管理器句柄。
- sec: 延时重置时间。
  
- 返回值:
  - 0: 成功。
  - -1: 失败。

### netmgr_stop 

`int netmgr_stop(netmgr_hdl_t hdl);`

- 功能描述:
  - 停止网络连接。

- 参数:
  - hdl: 网络管理器句柄。

- 返回值:
  - 0: 成功。
  - -1: 失败。

### netmgr_get_handle

`netmgr_hdl_t netmgr_get_handle(const char *name);`

- 功能描述:
  - 获取设备句柄。

- 参数:
  - name: 网络设备名称。

- 返回值:
  - 0: 成功。
  - -1: 失败。

### netmgr_is_gotip

`int netmgr_is_gotip(netmgr_hdl_t hdl);`

- 功能描述:
  - 网络设备是否获取到ip。

- 参数:
  - hdl: 网络管理器句柄。

- 返回值:
  - 0: 没有获取到ip。
  - 1: 当前网络设备已经成功获取到ip。

### netmgr_dev_wifi_deinit

`void netmgr_dev_wifi_deinit(netmgr_hdl_t hdl);`

- 功能描述:
  - 删除网络管理。

- 参数:
  - hdll: 网络管理器句柄。

- 返回值:
  - 无。

### netmgr_get_dev

`aos_dev_t *netmgr_get_dev(netmgr_hdl_t);`

- 功能描述:
  - 通过网络网络句柄获取设备。

- 参数:
  - netmgr_hdl_t: 设备描述符。

- 返回值:
  - 0: 成功。
  - -1: 失败。

### netmgr_ipconfig

`int netmgr_ipconfig(netmgr_hdl_t hdl, int dhcp_en, char *ipaddr, char *netmask, char *gw);`

- 功能描述:
  - netmgr配置IP或者使能dhcp。

- 参数:
  - hdll: 网络管理器句柄。
  - ipaddr: IP地址。
  - netmask: 子网掩码。
  - gw: 网关。

- 返回值:
  - 0: 成功。
  - -1: 失败。

### netmgr_is_linkup

`int netmgr_is_linkup(netmgr_hdl_t hdl);`

- 功能描述:
  - 链路层是否连接。

- 参数:
  - hdll: 网络管理器句柄。

- 返回值:
  - 0: 成功。
  - 1: 失败。

### netmgr_get_info

`int netmgr_get_info(netmgr_hdl_t hdl);`

- 功能描述:
  - 存储网络信息。

- 参数:
  - hdll: 网络管理器句柄。

- 返回值:
  - 0: 成功。
  - -1: 失败。

## 示例

###  rtl8723ds无线网卡适配

```c
// 网络层接口
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

// wifi 定义的链路层接口
static wifi_driver_t rtl8723ds_wifi_driver = {
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
    .send_80211_raw_frame = rtl8723ds_send_80211_raw_frame,
    .set_channel          = rtl8723ds_set_channel,
    .get_channel          = rtl8723ds_get_channel,

    .set_smartcfg = NULL,
};

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

void wifi_rtl8723ds_register(rtl8723ds_gpio_pin *config)
{
    if (config) {
        memcpy(&g_gpio_config, config, sizeof(rtl8723ds_gpio_pin));
    } else {
        LOGE(TAG, "Err:Please input gpio config\n");
        return;
    }

    gpio_pin_handle_t wl_en_pin;

    /* 网络主芯片上电管脚配置 */
    if (config->power != 0xFFFFFFFF) {
        LOGD(TAG, "pull up WLAN power\n");
        drv_pinmux_config(g_gpio_config.power, PIN_FUNC_GPIO);
        power_pin = csi_gpio_pin_initialize(g_gpio_config.power, NULL);
        csi_gpio_pin_config_mode(power_pin, GPIO_MODE_PUSH_PULL);
        csi_gpio_pin_config_direction(power_pin, GPIO_DIRECTION_OUTPUT);
        csi_gpio_pin_write(power_pin, 1);
    }
    /* WLAN使能管脚配置 */
    if (config->wl_en != 0xFFFFFFFF) {
        LOGD(TAG, "Init WLAN enable\n");
        drv_pinmux_config(g_gpio_config.wl_en, PIN_FUNC_GPIO);
        wl_en_pin = csi_gpio_pin_initialize(g_gpio_config.wl_en, NULL);
        csi_gpio_pin_config_mode(wl_en_pin, GPIO_MODE_PUSH_PULL);
        csi_gpio_pin_config_direction(wl_en_pin, GPIO_DIRECTION_OUTPUT);
        csi_gpio_pin_write(wl_en_pin, 1);
    }

    extern SDIO_BUS_OPS rtw_sdio_bus_ops;
    rtw_sdio_bus_ops.bus_probe();
    // 初始化lwip网络协议栈
    LwIP_Init();

    wifi_manager_init();
    // 注册rtl8723ds驱动，调用注册的rtl8723ds_dev_init函数
    driver_register(&rtl8723ds_driver.drv, NULL, 0);
}
```

### rtl8723ds无线网卡连接

```c
#define PIN_WI_EN     PA16
#define PIN_POWER_EN  0xFFFFFFFF

/* network event callback */
void user_local_event_cb(uint32_t event_id, const void *param, void *context)
{
    char url[128];
    if (event_id == EVENT_NETMGR_GOT_IP) { // 网络连接成功
        LOGD(TAG, "Net up");
    } else if (event_id == EVENT_NETMGR_NET_DISCON) {
        if ((int)param == NET_DISCON_REASON_DHCP_ERROR) {
            LOGD(TAG, "Net down"); 
            netmgr_reset(netmgr_get_handle("wifi"), 30); // 本次网络连接失败，30s后自动重连
        }
    }
}

static netmgr_hdl_t network_init()
{
    /* kv config check */
    aos_kv_setstring("wifi_ssid", "your ssid"); // 配置无线网络的名称和密码，并使能wifi
    aos_kv_setstring"wifi_psk", "your psk");
    aos_kv_setint("wifi_en", 1);

    netmgr_hdl_t netmgr_hdl = netmgr_dev_wifi_init(); // 初始化wifi网卡

    netmgr_service_init(NULL); // 初始化网络管理服务
    if (netmgr_hdl)
        netmgr_start(netmgr_hdl); // 启动wifi管理器

    return netmgr_hdl;
}

void main()
{
    board_yoc_init();

    /* init wifi driver and network */
    rtl8723ds_gpio_pin pin = { 
        .wl_en = PIN_WI_EN,
        .power = PIN_POWER_EN,
    };  

    wifi_rtl8723ds_register(&pin);// 根据rtl8723ds网卡相应的管脚注册无线网卡驱动
    network_init();
    /* 系统事件订阅 */
    event_subscribe(EVENT_NETMGR_GOT_IP, user_local_event_cb, NULL); // 订阅网络连接成功事件
    event_subscribe(EVENT_NETMGR_NET_DISCON, user_local_event_cb, NULL); // 订阅网络连接失败事件
}
```

## 诊断错误码

无。

## 运行资源

无。

## 依赖资源

无。

## 组件参考

无。

