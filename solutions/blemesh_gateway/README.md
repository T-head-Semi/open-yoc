# 概述


# 环境搭建


# 编译

在linux下编译执行

```bash
make clean;make
```

使用CDK则直接点击编译按钮。

# 烧录

**使用CK-Link烧录**

在linux环境下烧录执行

```bash
make flashall
```

烧录成功后，可以看到所有分区烧写进度都已至100%。

```
Program partition: prim         address: 0x80d0400, size 664816 byte
erasing...
program 08172400, 100%
Program partition: imtb         address: 0x81c0000, size 4096 byte
erasing...
program 0081c1000, 100%
```


# 启动


##### 5.3.2 连接到网络

开发板获取到路由器的SSID/KEY之后，就会发起连接。连接成功后，我们可以观察到串口打印如下日志：

  ```
  [     7.801532][I][netmgr  ]start dhcp
  [     7.855701][I][netmgr  ]IP: 192.168.1.103
  ```

也可以通过命令`ifconfig`检查网络连接状态，当网络连接成功，会有如下信息输出：

  ```
  > ifconfig
  
  wifi0	Link encap:WiFi  HWaddr 28:6d:cd:54:49:c9
      	inet addr:192.168.20.100
  	GWaddr:192.168.20.254
  	Mask:255.255.255.0
  	DNS SERVER 0: 114.114.114.114
  	DNS SERVER 1: 192.168.20.254
  
  WiFi Connected to 3c:37:86:96:1b:c1 (on wifi0)
  	SSID: NETGEAR_HYJ
  	channel: 6
  	signal: -33 dBm
  ```

  此时可以ping通平头哥OCC社区

  ```
  > ping occ.t-head.cn
  	ping occ.t-head.cn(203.119.214.112)
  	from 203.119.214.112: icmp_seq=1 time=38 ms
  ```

  如果Wi-Fi连接失败，会显示“WiFi Not connected”：

  ```
  > ifconfig
  
  wifi0	Link encap:WiFi  HWaddr 28:6d:cd:54:49:c9
      	inet addr:0.0.0.0
  	GWaddr:0.0.0.0
  	Mask:0.0.0.0
  	DNS SERVER 0: 208.67.222.222
  
  WiFi Connected to 00:00:00:00:00:00 (on wifi0)
  	SSID: NETGEAR_HYJ
  	channel: 31
  	signal: -31 dBm
  
  	WiFi Not connected
  ```


#### 网关添加设备

##### 开始扫描设备
```
gw showdev 1
```
##### 停止扫描设备
```
gw showdev 0
```
##### 添加设备

```
gw adddev <mac_addr> 0 <uuid>
```

#### 配置设备

```
gw autoconfig <unicast_addr>
```

#### 开关设备

```
gw onoff <unicast_addr> 0/1
```

#### 5.4 连接生活物联网平台

当通过上述步骤确认网络通畅后，开发板会连接到生活物联网平台，打印信息如下

```
[D][SL      ]smartliving client started

[prt] log level set as: [ 2 ]
.................................
   PK : a1AodaoqL84
   DN : TEST013
   DS : ca14e4d6c9
  PID : example.demo.partner-id
  MID : example.demo.module-id
   SM : TLS + Direct
   TS : 2524608000000
.................................
```

其中，PK/DN/DS/PS/PID信息为五元组信息，在创建产品时由生活物联网平台分配。

与云端连接成功后，会有如下的打印信息

```
[D][tls     ]LD CA root Cert
[D][tls     ]SSL/TLS struct
[I][tls     ]Conn /public.iot-as-mqtt.cn-shanghai.aliyuncs.com/1883
[D][tls     ]Handshake
[D][tls     ]Verify X.509
[I][tls     ]certverify ret 0x00
Device Initialized, Devid: 0
Cloud Connected
```

此时，在生活物联网平台**设备调试**->**测试设备**页面找到对应设备并点击**调试**按钮就可以看到对应的设备上线，并可以在网页上对其进行操作和控制。