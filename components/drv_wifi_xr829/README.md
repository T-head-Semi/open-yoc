## 概述
RTL8723块是一款低功耗WIFI+蓝牙二合一WIFI模块，SDIO接口，支持IEEE802.11B/G/N 标准,可以与其它符合该标准的无线设备互相联通,支持最新的64/128 位WEP 数据加密，支持WPA-PSK/WPA2-PSK,WPA/WPA2 安全机制,可适应不同的工作环境。

在yoc移植的过程中，如果需要支持WiFi功能(例如有配网需求，参考netmgr即 （[网络管理器](https://yoc.docs.t-head.cn/yocbook/Chapter4-%E6%A0%B8%E5%BF%83%E6%A8%A1%E5%9D%97/%E7%BD%91%E7%BB%9C%E8%BF%9E%E6%8E%A5/%E7%BD%91%E7%BB%9C%E7%AE%A1%E7%90%86%E5%99%A8.html))，则需要对相关接口进行移植实现，包含`net`及`wifi`两部分：

- NET HAL的接口定义请查看头文件：`net_impl.h`。
- WiFi HAL的接口定义请查看头文件：`wifi_impl.h`。

## 配置

无。

## 接口说明

#### NET模块结构体

首先，先了解一下NETHAL相关的一个重要结构体`net_ops_t`。WiFi特有相关的操作和接口都封装在`net_ops_t`这个结构体中，相关定义在文件:`wifi_impl.h`。

```c
typedef struct net_ops {
    int (*get_mac_addr)(aos_dev_t *dev, uint8_t *mac);
    int (*set_mac_addr)(aos_dev_t *dev, const uint8_t *mac);

    int (*get_dns_server)(aos_dev_t *dev, ip_addr_t ipaddr[], uint32_t num);
    int (*set_dns_server)(aos_dev_t *dev, ip_addr_t ipaddr[], uint32_t num);

    int (*set_hostname)(aos_dev_t *dev, const char *name);
    const char* (*get_hostname)(aos_dev_t *dev);

    int (*set_link_up)(aos_dev_t *dev);
    int (*set_link_down)(aos_dev_t *dev);

    int (*start_dhcp)(aos_dev_t *dev);
    int (*stop_dhcp)(aos_dev_t *dev);
    int (*set_ipaddr)(aos_dev_t *dev, const ip_addr_t *ipaddr, const ip_addr_t *netmask, const ip_addr_t *gw);
    int (*get_ipaddr)(aos_dev_t *dev, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw);
    int (*ping)(aos_dev_t *dev, int type, char *remote_ip);

    int (*subscribe)(aos_dev_t *dev, uint32_t event, event_callback_t cb, void *param);
    int (*unsubscribe)(aos_dev_t *dev, uint32_t event, event_callback_t cb, void *param);
} net_ops_t;
```

#### NET接口说明

- **`get_mac_addr、set_mac_addr`**

  设置（获取）mac地址

- **`get_dns_server、set_dns_server`**

  设置（获取）dns地址

- **`set_hostname、get_hostname`**

  设置（获取）主机名

- **`set_link_up、set_link_down`**

  断开（连接）设备

- **`start_dhcp、stop_dhcp`**

  启动（停止）dhcp

- **`set_ipaddr、get_ipaddr`**

  设置（获取）ip相关信息，ipaddr、netmask及gw

- **`ping`**

  网络测试（Packet Internet Groper）

- **`subscribe、 unsubscribe`**

  订阅及取消订阅相关事件

#### WiFi模块结构体
首先，先了解一下WiFi HAL相关的一个重要结构体`wifi_driver_t`。WiFi特有相关的操作和接口都封装在`wifi_driver_t`这个结构体中，相关定义在文件:wifi_impl.h。
  ```c
  typedef struct wifi_driver {

    /** common APIs */
    int (*init)(aos_dev_t *dev);
    int (*)(aos_dev_t *dev);
    int (*reset)(aos_dev_t *dev);
    int (*set_mode)(aos_dev_t *dev, wifi_mode_t mode);
    int (*get_mode)(aos_dev_t *dev, wifi_mode_t *mode);
    int (*install_event_cb)(aos_dev_t *dev, wifi_event_func *evt_cb);

    /** conf APIs */
    int (*set_protocol)(aos_dev_t *dev, uint8_t protocol_bitmap); //11bgn
    int (*get_protocol)(aos_dev_t *dev, uint8_t *protocol_bitmap);
    int (*set_country)(aos_dev_t *dev, wifi_country_t country);
    int (*get_country)(aos_dev_t *dev, wifi_country_t *country);
    int (*set_mac_addr)(aos_dev_t *dev, const uint8_t *mac);
    int (*get_mac_addr)(aos_dev_t *dev, uint8_t *mac);
    int (*set_auto_reconnect)(aos_dev_t *dev, bool en);
    int (*get_auto_reconnect)(aos_dev_t *dev, bool *en);
    int (*set_lpm)(aos_dev_t *dev, wifi_lpm_mode_t mode); //ps on/pff
    int (*get_lpm)(aos_dev_t *dev, wifi_lpm_mode_t *mode);
    int (*power_on)(aos_dev_t *dev); //the wifi module power on/off
    int (*power_off)(aos_dev_t *dev); 

    /** connection APIs */
    int (*start_scan)(aos_dev_t *dev, wifi_scan_config_t *config, bool block);
    int (*start)(aos_dev_t *dev, wifi_config_t * config); //start ap or sta
    int (*stop)(aos_dev_t *dev);//stop ap or sta
    int (*sta_get_link_status)(aos_dev_t *dev, wifi_ap_record_t *ap_info);
    int (*ap_get_sta_list)(aos_dev_t *dev, wifi_sta_list_t *sta);


    /** promiscuous APIs */
    int (*start_monitor)(aos_dev_t *dev, wifi_promiscuous_cb_t cb);
    int (*stop_monitor)(aos_dev_t *dev);
    int (*send_80211_raw_frame)(aos_dev_t *dev, void *buffer, uint16_t len);
    int (*set_channel)(aos_dev_t *dev, uint8_t primary, wifi_second_chan_t second);
    int (*get_channel)(aos_dev_t *dev, uint8_t *primary, wifi_second_chan_t *second);


    /* esp8266 related API */
    int (*set_smartcfg)(aos_dev_t *dev, int enable);

} wifi_driver_t;
  ```
#### WiFi接口说明

- **`init`**

  该接口需要对wifi进行初始化，使wifi达到可以准备进行连接工作的状态，如分配wifi资源、初始化硬件模块等操作。

- **`deinit`**

  对初始化时分配的相关资源进行回收

- **`reset`**

  复位模块

- **`set_mode、get_mode`**

  设置（获取）wifi模式，模式包含

  ```c
  typedef enum {
      WIFI_MODE_NULL = 0,     /**< null mode */
      WIFI_MODE_STA,          /**< WiFi station mode */
      WIFI_MODE_AP,           /**< WiFi soft-AP mode */
      WIFI_MODE_APSTA,        /**< WiFi station + soft-AP mode */
      WIFI_MODE_P2P,          /**< WiFi P2P mode */
      WIFI_MODE_MAX
  } wifi_mode_t;
  ```

- **`install_event_cb`**

  注册事件回调函数

- **`set_protocol 、get_protocol`**

  设置（获取）协议类型

- **`set_country、get_country`**

  设置（获取）地区类型

  ```c
  typedef enum {
      WIFI_COUNTRY_CN = 0, /**< country China, channel range [1, 14] */
      WIFI_COUNTRY_JP,     /**< country Japan, channel range [1, 14] */
      WIFI_COUNTRY_US,     /**< country USA, channel range [1, 11] */
      WIFI_COUNTRY_EU,     /**< country Europe, channel range [1, 13] */
      WIFI_COUNTRY_MAX
  } wifi_country_t;
  ```

  

- **`set_mac_addr、get_mac_addr`**

  设置（获取）mac地址，注意：**回传的mac地址格式为6个字节二进制值（不含`:`号）**，如
  `uint8_t mac[6] = {0xd8,0x96,0xe0,0x03,0x04,0x01};`。

- **`set_auto_reconnect`、get_auto_reconnect`**

  设置（获取）是否重连

- **`set_lpm、`get_lpm`**

  设置（获取）低功耗模式

  ```c
  typedef enum {
      WIFI_LPM_NONE,          /**< No power save */
      WIFI_LPM_KEEP_SOCKET,   /**< power save with socket maintain */
      WIFI_LPM_KEEP_LINK,     /**< power save with wifi link connecting maintain */
      WIFI_LPM_POWEROFF,      /**< power save with power off */
  } wifi_lpm_mode_t;
  ```

- **`power_on、power_off`**

  上下电设备

- **`start_scan`**

  启动WiFi扫描

- **`start、stop`**

  启动（停止）wifi

  启动时根据启动参数不同来区分启动station模式还是AP模式，如station模式下进行连接AP的操作、AP模式下根据参数启动AP功能。在station模式下，该函数触发AP连接操作后即返回。后续底层处理过程中，拿到IP信息后，需要调用`ip_got`回调函数来通知上层获取IP事件。注意：(1) **station模式下启动WiFi连接时，传入的SSID长度不超过32位**；(2) **在连接AP后，WiFi底层需要维护


- **`sta_get_link_status`**

  获取工作状态相关信息，比如连接状态、mac、ssid等

  ```c
  typedef struct {
      wifi_status_link_t link_status;       /* if not connected, following data is not valid */
      uint8_t bssid[6];                     /* MAC address of AP */
      uint8_t ssid[MAX_SSID_SIZE + 1];      /* SSID of AP */
      uint8_t channel;                      /* channel of AP */
      int8_t  rssi;                         /* signal strength of AP */
      wifi_second_chan_t second;            /* second channel of AP */
      wifi_encrypt_type_t encryptmode;      /* encrypt mode of AP */
      wifi_auth_mode_t authmode;            /* authmode of AP */
  } wifi_ap_record_t;
  ```

- **`ap_get_sta_list`**

  获取连接设备表

  ```c
  typedef struct {
      uint8_t mac[6];  /**< mac address of sta that associated with soft-AP */
  } wifi_sta_info_t;
  
  typedef struct {
      wifi_sta_info_t sta[HAL_WIFI_MAX_CONN_NUM]; /**< station list */
      int       num; /**< number of station that associated with soft-AP */
  } wifi_sta_list_t;
  ```

- **`start_monitor、stop_monitor`**

  启动（关闭）信道扫描，扫描信息通过回调函数`wifi_promiscuous_cb_t`上传给用户


- **`send_80211_raw_frame`**

  发送802.11格式的数据帧。

- **`set_channel、get_channel`**

  设置（获取）信道


## 示例

### 驱动注册

```C
void wifi_rtl8723ds_register(rtl8723ds_gpio_pin *config)
```
- **示例详解**
  
  主要流程为**注册网卡驱动**----->**启动netmgr微服务**（netmgr详细说明 [网络管理器](https://yoc.docs.t-head.cn/yocbook/Chapter4-%E6%A0%B8%E5%BF%83%E6%A8%A1%E5%9D%97/%E7%BD%91%E7%BB%9C%E8%BF%9E%E6%8E%A5/%E7%BD%91%E7%BB%9C%E7%AE%A1%E7%90%86%E5%99%A8.html) ）

```C
void rtl8723ds_init(void)
{
    /* init wifi driver and network */
    rtl8723ds_gpio_pin pin = {
        .wl_en = PIN_WI_EN,
        .power = PIN_POWER_EN,
    };
    wifi_rtl8723ds_register(&pin);

    netmgr_hdl_t netmgr_hdl = netmgr_dev_wifi_init();
    netmgr_service_init(NULL);
    netmgr_start(netmgr_hdl);
}
```

在src/init/init.c,`board_yoc_init()`调用rtl8723ds初始化

```
void board_init_init(void)
{
    ...
    rtl8723ds_init();
    ...
}
```

### console网络测试

- 在src/init/cli_cmd.c中的console 命令行增加网络相关命令，方便测试网卡功能

```C
void board_cli_init(utask_t *task)
{
    ...
    cli_reg_cmd_ping();
    cli_reg_cmd_ifconfig();  
    ...
}
```

- **ifconfig** 

使用ifconfig命令连接网络`ifconfig ap ssid psk`，将`ssid`和`psk`更新为路由器的账号和密码

- **ping**

确认连接网络后，使用`ping`命令测试网络是否连接外网，



## 诊断错误码

无。

## 运行资源

无。

## 依赖资源

minilibc: v7.2.0及以上。
aos: v7.2.0及以上。

## 组件参考

无。