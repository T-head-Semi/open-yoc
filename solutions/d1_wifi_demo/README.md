# 概述

基于`d1`芯片的wifi使用DEMO。

# 使用

## 通过CDK

在CDK的首页，通过搜索`d1_wifi_demo`，可以找到d1_wifi_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节， 

## 通过命令行

```bash
mkdir workspace
cd workspace
yoc init
yoc install d1_wifi_demo
```

### 编译

```bash
cd solutions/d1_wifi_demo/
make clean;make
```

## 烧录

安装[全志USB烧录驱动](https://www.aw-ol.com/downloads?cat=5)及[PhoenixSuit](https://www.aw-ol.com/downloads?cat=5)，后双击运行PhoenixSuit ，在**⼀键刷机**⻚⾯选择要烧写镜像（solutions/d1_wifi_demo/out/yoc_rtos_8M.img），烧写⽅式第一次全新烧写可选择**分区擦除升级**
按住 FEL 按键重新上电即可，具体使用可参考 https://d1.docs.aw-ol.com/source/2_gettools/#livesuit

**编译及烧写**章节

# 运行

## WiFi配置

使用`ifconfig ap ssid psk`命令

比如需要连接的ssid为`TEST`,密码为`1234567890`，显示`[netmgr]<netmgr>IP: 192.168.43.138`即为成功

```
ifconfig ap TEST 1234567890
apconfig ssid:TEST, psw:1234567890

[  28.550]<D>[xr829]<aw_wifi_task>wifi up
[wifi_normal_connect,385]:=======================
[wifi_normal_connect,386]:connect ssid: TEST
[wifi_normal_connect,387]:=======================
[  29.990]<D>[xr829]<Looper>wifi state:WIFI_CONNECTED
[  30.000]<D>[xr829]<Looper>dhcp tries time: 1
[net INF] netif (IPv4) is up
[net INF] address: 192.168.43.138
[net INF] gateway: 192.168.43.1
[net INF] netmask: 255.255.255.0
[  31.500]<D>[xr829]<Looper>Dhcp got ip 192.168.43.138
[  31.500]<D>[xr829]<Looper>wifi state:DHCP_SUCCESS
[  31.500]<I>[netmgr]<netmgr>IP: 192.168.43.138
[  31.510]<D>[xr829]<Looper>wifi connect successful:TEST
```

## PING测试

网络连接成功后，使用PING命令测试即为成功

```cli
ping www.baidu.com

ping www.baidu.com(180.101.49.11)
from 180.101.49.11: icmp_seq=1 time=50 ms
from 180.101.49.11: icmp_seq=2 time=50 ms
from 180.101.49.11: icmp_seq=3 time=50 ms
```



