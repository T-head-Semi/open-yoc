# 概述
wifi_demo是一个简单的WiFi连接示例。

# CDK
在CDK的首页，通过搜索关键字wifi，可以找到wifi_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节。

# 烧录
通过CDK编译完成之后会在out目录下生成yoc_rtos_8M.img文件，此文件即为最终的镜像文件。
使用全志烧写工具进行烧录。如何安装及烧写请参考[全志官方网站](https://d1.docs.aw-ol.com/study/study_4compile/#phoenixsuit)。

# 启动
烧录完成之后系统会自动启动，串口会有打印输出。
```cli
###YoC###[Feb 14 2022,15:59:03]
(cli-uart)# cpu clock is 1008000000Hz
[   0.340]<I>[init]<app_task>find 8 partitions
[   0.350]<D>[WIFI]<app_task>Init WLAN enable

[   0.450]<D>[WIFI_IO]<app_task>__sdio_bus_probe
SD:mmc_card_create card:0x40232dd0 id:1
[ERR] SDC:__mci_irq_handler,879 raw_int:40000100 err!
[ERR] SDC:SDC err, cmd 8, [ERR] SDC:sdc 663 abnormal status: RespErr
SD:sd1.0 or mmc
SD:***** Try sdio *****
[WRN] SD:card claims to support voltages below the defined range.These will be ignored.
SD:sdio highspeed 
SD:mmc_sdio_init_card bus width type:2
SD:
============= card information ==============
SD:Card Type     : SDIO
SD:Card Spec Ver : 1.0
SD:Card RCA      : 0x0001 
SD:Card OCR      : 0x90ffffff
SD:    vol_window  : 0x00ffffff
SD:    to_1v8_acpt : 1
SD:    high_capac  : 1
SD:Card CSD      :
SD:    speed       : 50000 KHz
SD:    cmd class   : 0x0
SD:    capacity    : 0MB
SD:Card CUR_STA  :
SD:    speed_mode  : DS: 25 MHz
SD:    bus_width   : 2
SD:    speed_class : 0
SD:=============================================
SD:***** sdio init ok *****
[   0.580]<I>[netmgr]<netmgr>start wifi
[   0.580]<D>[WiFiCONF]<netmgr>WIFI is not running
[   0.590]<I>[netmgr_wifi]<netmgr>ssid{SSID_Undef}, psk{}

[   0.590]<D>[WiFiCONF]<wifi_start_sta_task>WIFI is not running
[   0.620]<D>[WiFiCONF]<wifi_start_sta_task>Initializing WIFI ...
[   0.650]<D>[WIFI_IO]<sdio_irq>sdio_irq_thread enter IRQ routine
[   3.220]<D>[WiFiCONF]<wifi_start_sta_task>WIFI initialized

[   3.220]<D>[WiFiCONF]<wifi_start_sta_task>a2dp_case_wifi_slot: 35
[   4.560]<E>[WIFI]<wifi_start_sta_task>ERROR: STA Task, wifi connect failed! try another
[   5.900]<D>[WIFI]<cmd_thread>scan done!

[   6.560]<E>[WIFI]<wifi_start_sta_task>Target AP not found

[   6.560]<I>[app]<event_svr>Net down
[   6.560]<D>[AppExp]<event_svr>EVENT_NETMGR_NET_DISCON
[   6.570]<D>[AppExp]<event_svr>Net Reset after 3 second
[   9.570]<D>[WiFiCONF]<netmgr>Deinitializing WIFI ...
[   9.670]<D>[WIFI_IO]<netmgr>__sdio_release_irq
[   9.670]<D>[WIFI_IO]<sdio_irq>sdio_irq_thread exit

[   9.670]<D>[WiFiCONF]<netmgr>WIFI deinitialized
[   9.680]<I>[netmgr_wifi]<netmgr>ssid{SSID_Undef}, psk{}

[   9.680]<D>[WiFiCONF]<wifi_start_sta_task>WIFI is not running
[   9.710]<D>[WiFiCONF]<wifi_start_sta_task>Initializing WIFI ...
[   9.750]<D>[WIFI_IO]<sdio_irq>sdio_irq_thread enter IRQ routine
```

WiFi配置：
```cli
ifconfig ap <ssid> <password>
```
其中<ssid>指的是WiFi名称，<password>指的是WiFi密码。配置完成之后输入reboot命令进行复位，或者通过开发板的复位键进行复位。

```cli
[   3.960]<D>[WIFI]<wifi_start_sta_task>@@@@@@@@@@@@@@ Connection Success @@@@@@@@@@@@@@

[   3.970]<I>[netmgr]<netmgr>start dhcp
[   4.050]<I>[netmgr]<netmgr>IP: 172.20.10.3
[   4.050]<I>[app]<event_svr>Got IP
```

PING测试：

网络连接成功后，使用PING命令测试即为成功

```cli
ping www.baidu.com

ping www.baidu.com(180.101.49.11)
from 180.101.49.11: icmp_seq=1 time=50 ms
from 180.101.49.11: icmp_seq=2 time=50 ms
from 180.101.49.11: icmp_seq=3 time=50 ms
```
