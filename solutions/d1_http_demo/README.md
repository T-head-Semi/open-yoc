# 概述

基于`d1`芯片的http使用DEMO。

# 使用

## 通过CDK

在CDK的首页，通过搜索`d1_http_demo`，可以找到d1_http_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节， 

## 通过命令行

```bash
mkdir workspace
cd workspace
yoc init
yoc install d1_http_demo
```

### 编译

```bash
cd solutions/d1_http_demo/
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

## HTTP运行

网络连接成功后，http将自动运行demo，显示`<http_test>Finish http example [0]`即为成功

```cli
[   8.930]<D>[xr829]<Looper>Dhcp got ip 192.168.43.138
[   8.930]<D>[xr829]<Looper>wifi state:DHCP_SUCCESS
[   8.930]<I>[netmgr]<netmgr>IP: 192.168.43.138
[   8.940]<D>[xr829]<Looper>wifi connect successful:TEST11
[      88..994400]]<<II>>[[aapppp]]<<eevveenntt__ssvvrr>>GGoott  IIPPs
[  16.630]<D>[https_example]<http_test>HTTP_EVENT_HEADER_SENT
[  16.640]<D>[HTTP_CLIENT]<http_test>HTTP_STATE_CONNECTED
[  16.640]<D>[HTTP_CLIENT]<http_test>HTTP_STATE_REQ_COMPLETE_HEADER
[  16.910]<D>[HTTP_CLIENT]<http_test>on_message_begin
[  16.920]<D>[HTTP_CLIENT]<http_test>HEADER=Date:Tue, 30 Nov 2021 08:54:37 GMT
[  16.920]<D>[https_example]<http_test>HTTP_EVENT_ON_HEADER, key=Date, value=Tue, 30 Nov 2021 08:54:37 GMT
[  16.930]<D>[HTTP_CLIENT]<http_test>HEADER=Content-Type:text/html
[  16.940]<D>[https_example]<http_test>HTTP_EVENT_ON_HEADER, key=Content-Type, value=text/html
[  16.950]<D>[HTTP_CLIENT]<http_test>HEADER=Content-Length:233
[  16.950]<D>[https_example]<http_test>HTTP_EVENT_ON_HEADER, key=Content-Length, value=233
[  16.960]<D>[HTTP_CLIENT]<http_test>HEADER=Connection:keep-alive
[  16.970]<D>[https_example]<http_test>HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
[  16.980]<D>[HTTP_CLIENT]<http_test>HEADER=Server:gunicorn/19.9.0
[  16.980]<D>[https_example]<http_test>HTTP_EVENT_ON_HEADER, key=Server, value=gunicorn/19.9.0
[  16.990]<D>[HTTP_CLIENT]<http_test>HEADER=Access-Control-Allow-Origin:*
[  17.000]<D>[https_example]<http_test>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Origin, value=*
[  17.010]<D>[HTTP_CLIENT]<http_test>HEADER=Access-Control-Allow-Credentials:true
[  17.010]<D>[https_example]<http_test>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Credentials, value=true
[  17.020]<D>[HTTP_CLIENT]<http_test>http_on_headers_complete, status=404, offset=230, nread=230
[  17.030]<D>[HTTP_CLIENT]<http_test>content_length = 233
[  17.040]<D>[HTTP_CLIENT]<http_test>HTTP_STATE_REQ_COMPLETE_DATA
[  17.040]<D>[HTTP_CLIENT]<http_test>Read finish or server requests close
[  17.050]<D>[https_example]<http_test>HTTP_EVENT_ON_FINISH
[  17.050]<D>[HTTP_CLIENT]<http_test>HTTP_STATE_RES_COMPLETE_HEADER
[  17.060]<I>[https_example]<http_test>HTTP HEAD Status = 404, content_length = 233 
[  17.070]<D>[https_example]<http_test>HTTP_EVENT_DISCONNECTED
[  17.070]<I>[https_example]<http_test>Finish http example [0]
```



