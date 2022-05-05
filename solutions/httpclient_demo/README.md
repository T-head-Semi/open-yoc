# 概述
httpclient_demo是一个简单的httpclient组件使用示例。httpclient是一个开源的http客户端，支持HTTP和HTTPS的访问。

# CDK
在CDK的首页，通过搜索关键字httpclient，可以找到httpclient_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节。

# 烧录
通过CDK编译完成之后会在out目录下生成yoc_rtos_8M.img文件，此文件即为最终的镜像文件。
使用全志烧写工具进行烧录。如何安装及烧写请参考[全志官方网站](https://d1.docs.aw-ol.com/study/study_4compile/#phoenixsuit)。

# 启动
烧录完成之后系统会自动启动，串口会有打印输出。
```cli
###YoC###[Feb 14 2022,15:27:16]
(cli-uart)# cpu clock is 1008000000Hz
[   0.350]<I>[init]<app_task>find 8 partitions
[   0.360]<D>[WIFI]<app_task>Init WLAN enable

[   0.470]<D>[WIFI_IO]<app_task>__sdio_bus_probe
SD:mmc_card_create card:0x4024f680 id:1
[ERR] SDC:__mci_irq_handler,879 raw_int:100 err!
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
[   0.600]<I>[netmgr]<netmgr>start wifi
[   0.600]<D>[WiFiCONF]<netmgr>WIFI is not running
[   0.610]<I>[netmgr_wifi]<netmgr>ssid{SSID_Undef}, psk{}

[   0.610]<D>[WiFiCONF]<wifi_start_sta_task>WIFI is not running
[   0.640]<D>[WiFiCONF]<wifi_start_sta_task>Initializing WIFI ...
[   0.670]<D>[WIFI_IO]<sdio_irq>sdio_irq_thread enter IRQ routine
[   3.240]<D>[WiFiCONF]<wifi_start_sta_task>WIFI initialized

[   3.240]<D>[WiFiCONF]<wifi_start_sta_task>a2dp_case_wifi_slot: 35
[   4.580]<E>[WIFI]<wifi_start_sta_task>ERROR: STA Task, wifi connect failed! try another
[   5.920]<D>[WIFI]<cmd_thread>scan done!

[   6.580]<E>[WIFI]<wifi_start_sta_task>Target AP not found

[   6.580]<I>[app]<event_svr>Net down
[   6.580]<D>[AppExp]<event_svr>EVENT_NETMGR_NET_DISCON
[   6.590]<D>[AppExp]<event_svr>Net Reset after 3 second
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
-------------------------
Usage: web http|https
     : web http get <url>, such as web http get http://occ.t-head.cn
```
看到以上信息表示WiFi连接成功了。可以通过cli的命令进行HTTP和HTTPS的测试。

● HTTP测试：
通过串口CLI输入 web http 命令进行HTTP访问测试。
```cli
[10:45:45:999](cli-uart)# web http
[10:45:45:999][  36.910]<D>[HTTP_CLIENT]<cli-uart>###path:/get
[10:45:45:999][  36.920]<D>[HTTP_CLIENT]<cli-uart>New path assign = /get
[10:45:46:009][  36.920]<D>[HTTP_CLIENT]<cli-uart>client->state: 1, client->process_again: 0
[10:45:46:016][  36.930]<D>[HTTP_CLIENT]<cli-uart>Begin connect to: http://httpbin.org:80
[10:45:46:214][  37.120]<D>[TRANS_TCP]<cli-uart>[sock=20],connecting to server IP:54.91.120.77,Port:80...
[10:45:46:502][  37.420]<D>[example]<cli-uart>HTTP_EVENT_ON_CONNECTED
[10:45:46:502][  37.420]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_INIT
[10:45:46:513][  37.420]<D>[HTTP_CLIENT]<cli-uart>Write header[3]: GET /get HTTP/1.1
[10:45:46:513]User-Agent: CK HTTP Client/1.0
[10:45:46:513]Host: httpbin.org
[10:45:46:513]Content-Length: 0
[10:45:46:513]
[10:45:46:513]
[10:45:46:525][  37.440]<D>[example]<cli-uart>HTTP_EVENT_HEADER_SENT
[10:45:46:525][  37.440]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_CONNECTED
[10:45:46:535][  37.450]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_HEADER
[10:45:47:463][  38.380]<D>[HTTP_CLIENT]<cli-uart>on_message_begin
[10:45:47:475][  38.380]<D>[HTTP_CLIENT]<cli-uart>HEADER=Date:Mon, 21 Mar 2022 02:45:47 GMT
[10:45:47:475][  38.390]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Date, value=Mon, 21 Mar 2022 02:45:47 GMT
[10:45:47:485][  38.400]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Type:application/json
[10:45:47:497][  38.400]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Type, value=application/json
[10:45:47:497][  38.410]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Length:266
[10:45:47:508][  38.420]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Length, value=266
[10:45:47:508][  38.420]<D>[HTTP_CLIENT]<cli-uart>HEADER=Connection:keep-alive
[10:45:47:519][  38.430]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
[10:45:47:530][  38.440]<D>[HTTP_CLIENT]<cli-uart>HEADER=Server:gunicorn/19.9.0
[10:45:47:530][  38.440]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Server, value=gunicorn/19.9.0
[10:45:47:541][  38.450]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Origin:*
[10:45:47:552][  38.460]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Origin, value=*
[10:45:47:552][  38.470]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Credentials:true
[10:45:47:563][  38.470]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Credentials, value=true
[10:45:47:575][  38.480]<D>[HTTP_CLIENT]<cli-uart>http_on_headers_complete, status=200, offset=230, nread=230
[10:45:47:575][  38.490]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=266
[10:45:47:586][  38.500]<D>[HTTP_CLIENT]<cli-uart>http_on_message_complete, parser=0x4023cb00
[10:45:47:586][  38.500]<D>[HTTP_CLIENT]<cli-uart>content_length = 266
[10:45:47:597][  38.510]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_DATA
[10:45:47:597][  38.510]<D>[example]<cli-uart>HTTP_EVENT_ON_FINISH
[10:45:47:608][  38.520]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_RES_COMPLETE_HEADER
[10:45:47:608][  38.520]<I>[example]<cli-uart>HTTP GET Status = 200, content_length = 266
[10:45:47:619][  38.530]<I>[example]<cli-uart>++++++++++++++ HTTP GET TEST OK
[10:45:47:619]
[10:45:47:619][  38.540]<D>[HTTP_CLIENT]<cli-uart>###path:/post
[10:45:47:630][  38.540]<D>[HTTP_CLIENT]<cli-uart>New path assign = /post
[10:45:47:630][  38.550]<D>[example]<cli-uart>HTTP_EVENT_DISCONNECTED
[10:45:47:641][  38.550]<D>[HTTP_CLIENT]<cli-uart>set post file length = 27
[10:45:47:641][  38.560]<D>[HTTP_CLIENT]<cli-uart>client->state: 1, client->process_again: 0
[10:45:47:653][  38.560]<D>[HTTP_CLIENT]<cli-uart>Begin connect to: http://httpbin.org:80
[10:45:47:660][  38.570]<D>[TRANS_TCP]<cli-uart>[sock=20],connecting to server IP:54.91.120.77,Port:80...
[10:45:48:105][  39.020]<D>[example]<cli-uart>HTTP_EVENT_ON_CONNECTED
[10:45:48:105][  39.020]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_INIT
[10:45:48:116][  39.030]<D>[HTTP_CLIENT]<cli-uart>Write header[4]: POST /post HTTP/1.1
[10:45:48:116]User-Agent: CK HTTP Client/1.0
[10:45:48:116]Host: httpbin.org
[10:45:48:116]Content-Length: 27
[10:45:48:127]Content-Type: application/x-www-form-urlencoded
[10:45:48:127]
[10:45:48:127]
[10:45:48:127][  39.040]<D>[example]<cli-uart>HTTP_EVENT_HEADER_SENT
[10:45:48:142][  39.050]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_CONNECTED
[10:45:48:142][  39.050]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_HEADER
[10:45:49:081][  39.990]<D>[HTTP_CLIENT]<cli-uart>on_message_begin
[10:45:49:093][  40.000]<D>[HTTP_CLIENT]<cli-uart>HEADER=Date:Mon, 21 Mar 2022 02:45:48 GMT
[10:45:49:093][  40.010]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Date, value=Mon, 21 Mar 2022 02:45:48 GMT
[10:45:49:104][  40.010]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Type:application/json
[10:45:49:114][  40.020]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Type, value=application/json
[10:45:49:115][  40.030]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Length:440
[10:45:49:126][  40.030]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Length, value=440
[10:45:49:126][  40.040]<D>[HTTP_CLIENT]<cli-uart>HEADER=Connection:keep-alive
[10:45:49:137][  40.050]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
[10:45:49:148][  40.060]<D>[HTTP_CLIENT]<cli-uart>HEADER=Server:gunicorn/19.9.0
[10:45:49:148][  40.060]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Server, value=gunicorn/19.9.0
[10:45:49:159][  40.070]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Origin:*
[10:45:49:171][  40.080]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Origin, value=*
[10:45:49:171][  40.080]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Credentials:true
[10:45:49:181][  40.090]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Credentials, value=true
[10:45:49:192][  40.100]<D>[HTTP_CLIENT]<cli-uart>http_on_headers_complete, status=200, offset=230, nread=230
[10:45:49:193][  40.110]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=282
[10:45:49:204][  40.110]<D>[HTTP_CLIENT]<cli-uart>content_length = 440
[10:45:49:204][  40.120]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_DATA
[10:45:49:215][  40.120]<D>[HTTP_CLIENT]<cli-uart>data_process=282, content_length=440
[10:45:49:215][  40.130]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=158
[10:45:49:226][  40.140]<D>[HTTP_CLIENT]<cli-uart>http_on_message_complete, parser=0x4023cb00
[10:45:49:226][  40.140]<D>[example]<cli-uart>HTTP_EVENT_ON_FINISH
[10:45:49:237][  40.150]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_RES_COMPLETE_HEADER
[10:45:49:237][  40.150]<I>[example]<cli-uart>HTTP POST Status = 200, content_length = 440
[10:45:49:248][  40.160]<I>[example]<cli-uart>++++++++++++++ HTTP POST TEST OK
[10:45:49:248]
[10:45:49:248][  40.170]<D>[HTTP_CLIENT]<cli-uart>###path:/put
[10:45:49:259][  40.170]<D>[HTTP_CLIENT]<cli-uart>New path assign = /put
[10:45:49:259][  40.180]<D>[example]<cli-uart>HTTP_EVENT_DISCONNECTED
[10:45:49:270][  40.180]<D>[HTTP_CLIENT]<cli-uart>client->state: 1, client->process_again: 0
[10:45:49:285][  40.190]<D>[HTTP_CLIENT]<cli-uart>Begin connect to: http://httpbin.org:80
[10:45:49:285][  40.190]<D>[TRANS_TCP]<cli-uart>[sock=20],connecting to server IP:54.91.120.77,Port:80...
[10:45:49:772][  40.690]<D>[example]<cli-uart>HTTP_EVENT_ON_CONNECTED
[10:45:49:772][  40.690]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_INIT
[10:45:49:783][  40.690]<D>[HTTP_CLIENT]<cli-uart>Write header[4]: PUT /put HTTP/1.1
[10:45:49:783]User-Agent: CK HTTP Client/1.0
[10:45:49:783]Host: httpbin.org
[10:45:49:783]Content-Length: 27
[10:45:49:794]Content-Type: application/x-www-form-urlencoded
[10:45:49:794]
[10:45:49:794]
[10:45:49:795][  40.710]<D>[example]<cli-uart>HTTP_EVENT_HEADER_SENT
[10:45:49:808][  40.720]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_CONNECTED
[10:45:49:809][  40.720]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_HEADER
[10:45:50:662][  41.580]<D>[HTTP_CLIENT]<cli-uart>on_message_begin
[10:45:50:673][  41.580]<D>[HTTP_CLIENT]<cli-uart>HEADER=Date:Mon, 21 Mar 2022 02:45:50 GMT
[10:45:50:673][  41.590]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Date, value=Mon, 21 Mar 2022 02:45:50 GMT
[10:45:50:684][  41.600]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Type:application/json
[10:45:50:695][  41.600]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Type, value=application/json
[10:45:50:696][  41.610]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Length:439
[10:45:50:707][  41.620]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Length, value=439
[10:45:50:707][  41.620]<D>[HTTP_CLIENT]<cli-uart>HEADER=Connection:keep-alive
[10:45:50:718][  41.630]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
[10:45:50:729][  41.640]<D>[HTTP_CLIENT]<cli-uart>HEADER=Server:gunicorn/19.9.0
[10:45:50:729][  41.640]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Server, value=gunicorn/19.9.0
[10:45:50:740][  41.650]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Origin:*
[10:45:50:751][  41.660]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Origin, value=*
[10:45:50:751][  41.660]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Credentials:true
[10:45:50:762][  41.670]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Credentials, value=true
[10:45:50:773][  41.680]<D>[HTTP_CLIENT]<cli-uart>http_on_headers_complete, status=200, offset=230, nread=230
[10:45:50:773][  41.690]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=282
[10:45:50:784][  41.690]<D>[HTTP_CLIENT]<cli-uart>content_length = 439
[10:45:50:785][  41.700]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_DATA
[10:45:50:796][  41.710]<D>[HTTP_CLIENT]<cli-uart>data_process=282, content_length=439
[10:45:50:796][  41.710]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=157
[10:45:50:807][  41.720]<D>[HTTP_CLIENT]<cli-uart>http_on_message_complete, parser=0x4023cb00
[10:45:50:807][  41.720]<D>[example]<cli-uart>HTTP_EVENT_ON_FINISH
[10:45:50:818][  41.730]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_RES_COMPLETE_HEADER
[10:45:50:818][  41.730]<I>[example]<cli-uart>HTTP PUT Status = 200, content_length = 439
[10:45:50:829][  41.740]<I>[example]<cli-uart>++++++++++++++ HTTP PUT TEST OK
[10:45:50:829]
[10:45:50:829][  41.750]<D>[HTTP_CLIENT]<cli-uart>###path:/patch
[10:45:50:840][  41.750]<D>[HTTP_CLIENT]<cli-uart>New path assign = /patch
[10:45:50:840][  41.760]<D>[example]<cli-uart>HTTP_EVENT_DISCONNECTED
[10:45:50:851][  41.760]<D>[HTTP_CLIENT]<cli-uart>set post file length = 0
[10:45:50:851][  41.770]<D>[HTTP_CLIENT]<cli-uart>client->state: 1, client->process_again: 0
[10:45:50:862][  41.770]<D>[HTTP_CLIENT]<cli-uart>Begin connect to: http://httpbin.org:80
[10:45:50:871][  41.780]<D>[TRANS_TCP]<cli-uart>[sock=20],connecting to server IP:54.91.120.77,Port:80...
[10:45:51:305][  42.220]<D>[example]<cli-uart>HTTP_EVENT_ON_CONNECTED
[10:45:51:305][  42.220]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_INIT
[10:45:51:315][  42.230]<D>[HTTP_CLIENT]<cli-uart>Write header[3]: PATCH /patch HTTP/1.1
[10:45:51:315]User-Agent: CK HTTP Client/1.0
[10:45:51:315]Host: httpbin.org
[10:45:51:315]Content-Length: 0
[10:45:51:327]
[10:45:51:327]
[10:45:51:327][  42.240]<D>[example]<cli-uart>HTTP_EVENT_HEADER_SENT
[10:45:51:327][  42.240]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_CONNECTED
[10:45:51:336][  42.250]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_HEADER
[10:45:51:622][  42.530]<D>[HTTP_CLIENT]<cli-uart>on_message_begin
[10:45:51:633][  42.540]<D>[HTTP_CLIENT]<cli-uart>HEADER=Date:Mon, 21 Mar 2022 02:45:51 GMT
[10:45:51:633][  42.550]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Date, value=Mon, 21 Mar 2022 02:45:51 GMT
[10:45:51:644][  42.560]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Type:application/json
[10:45:51:655][  42.560]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Type, value=application/json
[10:45:51:656][  42.570]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Length:331
[10:45:51:667][  42.580]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Length, value=331
[10:45:51:667][  42.580]<D>[HTTP_CLIENT]<cli-uart>HEADER=Connection:keep-alive
[10:45:51:678][  42.590]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
[10:45:51:688][  42.600]<D>[HTTP_CLIENT]<cli-uart>HEADER=Server:gunicorn/19.9.0
[10:45:51:688][  42.600]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Server, value=gunicorn/19.9.0
[10:45:51:699][  42.610]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Origin:*
[10:45:51:710][  42.620]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Origin, value=*
[10:45:51:710][  42.620]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Credentials:true
[10:45:51:721][  42.630]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Credentials, value=true
[10:45:51:732][  42.640]<D>[HTTP_CLIENT]<cli-uart>http_on_headers_complete, status=200, offset=230, nread=230
[10:45:51:732][  42.650]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=282
[10:45:51:743][  42.650]<D>[HTTP_CLIENT]<cli-uart>content_length = 331
[10:45:51:743][  42.660]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_DATA
[10:45:51:755][  42.660]<D>[HTTP_CLIENT]<cli-uart>data_process=282, content_length=331
[10:45:51:755][  42.670]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=49
[10:45:51:766][  42.680]<D>[HTTP_CLIENT]<cli-uart>http_on_message_complete, parser=0x4023cb00
[10:45:51:766][  42.680]<D>[example]<cli-uart>HTTP_EVENT_ON_FINISH
[10:45:51:777][  42.690]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_RES_COMPLETE_HEADER
[10:45:51:777][  42.690]<I>[example]<cli-uart>HTTP PATCH Status = 200, content_length = 331
[10:45:51:788][  42.700]<I>[example]<cli-uart>++++++++++++++ HTTP PATCH TEST OK
[10:45:51:788]
[10:45:51:788][  42.710]<D>[HTTP_CLIENT]<cli-uart>###path:/delete
[10:45:51:799][  42.710]<D>[HTTP_CLIENT]<cli-uart>New path assign = /delete
[10:45:51:799][  42.720]<D>[example]<cli-uart>HTTP_EVENT_DISCONNECTED
[10:45:51:810][  42.720]<D>[HTTP_CLIENT]<cli-uart>client->state: 1, client->process_again: 0
[10:45:51:824][  42.730]<D>[HTTP_CLIENT]<cli-uart>Begin connect to: http://httpbin.org:80
[10:45:51:825][  42.740]<D>[TRANS_TCP]<cli-uart>[sock=20],connecting to server IP:54.91.120.77,Port:80...
[10:45:52:249][  43.160]<D>[example]<cli-uart>HTTP_EVENT_ON_CONNECTED
[10:45:52:249][  43.170]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_INIT
[10:45:52:260][  43.170]<D>[HTTP_CLIENT]<cli-uart>Write header[3]: DELETE /delete HTTP/1.1
[10:45:52:261]User-Agent: CK HTTP Client/1.0
[10:45:52:261]Host: httpbin.org
[10:45:52:271]Content-Length: 0
[10:45:52:271]
[10:45:52:271]
[10:45:52:272][  43.180]<D>[example]<cli-uart>HTTP_EVENT_HEADER_SENT
[10:45:52:272][  43.190]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_CONNECTED
[10:45:52:282][  43.190]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_HEADER
[10:45:52:583][  43.500]<D>[HTTP_CLIENT]<cli-uart>on_message_begin
[10:45:52:594][  43.500]<D>[HTTP_CLIENT]<cli-uart>HEADER=Date:Mon, 21 Mar 2022 02:45:52 GMT
[10:45:52:594][  43.510]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Date, value=Mon, 21 Mar 2022 02:45:52 GMT
[10:45:52:605][  43.520]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Type:application/json
[10:45:52:616][  43.520]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Type, value=application/json
[10:45:52:616][  43.530]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Length:332
[10:45:52:627][  43.540]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Length, value=332
[10:45:52:627][  43.540]<D>[HTTP_CLIENT]<cli-uart>HEADER=Connection:keep-alive
[10:45:52:638][  43.550]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
[10:45:52:649][  43.560]<D>[HTTP_CLIENT]<cli-uart>HEADER=Server:gunicorn/19.9.0
[10:45:52:650][  43.560]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Server, value=gunicorn/19.9.0
[10:45:52:660][  43.570]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Origin:*
[10:45:52:672][  43.580]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Origin, value=*
[10:45:52:672][  43.590]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Credentials:true
[10:45:52:683][  43.590]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Credentials, value=true
[10:45:52:694][  43.600]<D>[HTTP_CLIENT]<cli-uart>http_on_headers_complete, status=200, offset=230, nread=230
[10:45:52:694][  43.610]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=282
[10:45:52:705][  43.610]<D>[HTTP_CLIENT]<cli-uart>content_length = 332
[10:45:52:705][  43.620]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_DATA
[10:45:52:716][  43.630]<D>[HTTP_CLIENT]<cli-uart>data_process=282, content_length=332
[10:45:52:716][  43.630]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=50
[10:45:52:728][  43.640]<D>[HTTP_CLIENT]<cli-uart>http_on_message_complete, parser=0x4023cb00
[10:45:52:728][  43.640]<D>[example]<cli-uart>HTTP_EVENT_ON_FINISH
[10:45:52:738][  43.650]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_RES_COMPLETE_HEADER
[10:45:52:739][  43.650]<I>[example]<cli-uart>HTTP DELETE Status = 200, content_length = 332
[10:45:52:750][  43.660]<I>[example]<cli-uart>++++++++++++++ HTTP DELETE TEST OK
[10:45:52:750]
[10:45:52:750][  43.670]<D>[HTTP_CLIENT]<cli-uart>###path:/head
[10:45:52:761][  43.670]<D>[HTTP_CLIENT]<cli-uart>New path assign = /head
[10:45:52:761][  43.680]<D>[example]<cli-uart>HTTP_EVENT_DISCONNECTED
[10:45:52:772][  43.680]<D>[HTTP_CLIENT]<cli-uart>client->state: 1, client->process_again: 0
[10:45:52:787][  43.690]<D>[HTTP_CLIENT]<cli-uart>Begin connect to: http://httpbin.org:80
[10:45:52:787][  43.700]<D>[TRANS_TCP]<cli-uart>[sock=20],connecting to server IP:54.91.120.77,Port:80...
[10:45:53:222][  44.140]<D>[example]<cli-uart>HTTP_EVENT_ON_CONNECTED
[10:45:53:223][  44.140]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_INIT
[10:45:53:233][  44.150]<D>[HTTP_CLIENT]<cli-uart>Write header[3]: HEAD /head HTTP/1.1
[10:45:53:234]User-Agent: CK HTTP Client/1.0
[10:45:53:234]Host: httpbin.org
[10:45:53:234]Content-Length: 0
[10:45:53:234]
[10:45:53:245]
[10:45:53:245][  44.160]<D>[example]<cli-uart>HTTP_EVENT_HEADER_SENT
[10:45:53:245][  44.160]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_CONNECTED
[10:45:53:256][  44.170]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_HEADER
[10:45:53:545][  44.460]<D>[HTTP_CLIENT]<cli-uart>on_message_begin
[10:45:53:555][  44.460]<D>[HTTP_CLIENT]<cli-uart>HEADER=Date:Mon, 21 Mar 2022 02:45:53 GMT
[10:45:53:555][  44.470]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Date, value=Mon, 21 Mar 2022 02:45:53 GMT
[10:45:53:566][  44.480]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Type:text/html
[10:45:53:578][  44.480]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Type, value=text/html
[10:45:53:578][  44.490]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Length:233
[10:45:53:588][  44.500]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Length, value=233
[10:45:53:588][  44.500]<D>[HTTP_CLIENT]<cli-uart>HEADER=Connection:keep-alive
[10:45:53:599][  44.510]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
[10:45:53:599][  44.520]<D>[HTTP_CLIENT]<cli-uart>HEADER=Server:gunicorn/19.9.0
[10:45:53:610][  44.520]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Server, value=gunicorn/19.9.0
[10:45:53:621][  44.530]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Origin:*
[10:45:53:622][  44.540]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Origin, value=*
[10:45:53:632][  44.550]<D>[HTTP_CLIENT]<cli-uart>HEADER=Access-Control-Allow-Credentials:true
[10:45:53:644][  44.550]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Access-Control-Allow-Credentials, value=true
[10:45:53:655][  44.560]<D>[HTTP_CLIENT]<cli-uart>http_on_headers_complete, status=404, offset=230, nread=230
[10:45:53:655][  44.570]<D>[HTTP_CLIENT]<cli-uart>content_length = 233
[10:45:53:666][  44.570]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_DATA
[10:45:53:666][  44.580]<D>[HTTP_CLIENT]<cli-uart>Read finish or server requests close
[10:45:53:677][  44.590]<D>[example]<cli-uart>HTTP_EVENT_ON_FINISH
[10:45:53:677][  44.590]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_RES_COMPLETE_HEADER
[10:45:53:688][  44.600]<I>[example]<cli-uart>HTTP HEAD Status = 404, content_length = 233
[10:45:53:688][  44.600]<I>[example]<cli-uart>++++++++++++++ HTTP HEAD TEST OK
[10:45:53:688]
[10:45:53:699][  44.610]<D>[example]<cli-uart>HTTP_EVENT_DISCONNECTED
[10:45:53:699]
[10:45:53:699](cli-uart)#
```

● HTTPS测试：
通过串口CLI输入 web https 命令进行HTTPS访问测试。
```cli
[10:47:49:162]web https
[10:47:49:162][ 160.080]<D>[HTTP_CLIENT]<cli-uart>###path:/post
[10:47:49:162][ 160.080]<D>[HTTP_CLIENT]<cli-uart>New path assign = /post
[10:47:49:173][ 160.090]<D>[HTTP_CLIENT]<cli-uart>set post file length = 415
[10:47:49:185][ 160.090]<D>[HTTP_CLIENT]<cli-uart>client->state: 1, client->process_again: 0
[10:47:49:185][ 160.100]<D>[HTTP_CLIENT]<cli-uart>Begin connect to: https://postman-echo.com:443
[10:47:49:200][ 160.110]<E>[tls]<cli-uart>tls_conn_new async
[10:47:49:201][ 160.110]<D>[tls]<cli-uart>tls init...
[10:47:49:201][ 160.110]<D>[tls]<cli-uart>use_host:postman-echo.com, port:443
[10:47:50:375][ 161.290]<D>[tls]<cli-uart>_tls_net connect 0 
[10:47:50:375][ 161.290]<D>[tls]<cli-uart>tls connecting...
[10:47:50:385][ 161.300]<D>[tls]<cli-uart>connecting...
[10:47:50:385][ 161.300]<D>[tls]<cli-uart>handshake in progress...
[10:47:51:932][ 162.850]<D>[example]<cli-uart>HTTP_EVENT_ON_CONNECTED
[10:47:51:932][ 162.850]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_INIT
[10:47:51:943][ 162.860]<D>[HTTP_CLIENT]<cli-uart>Write header[4]: POST /post HTTP/1.1
[10:47:51:943]User-Agent: CK HTTP Client/1.0
[10:47:51:943]Host: postman-echo.com
[10:47:51:954]Content-Type: application/x-www-form-urlencoded
[10:47:51:954]Content-Length: 415
[10:47:51:954]
[10:47:51:954]
[10:47:51:954][ 162.870]<D>[example]<cli-uart>HTTP_EVENT_HEADER_SENT
[10:47:51:965][ 162.880]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_CONNECTED
[10:47:51:965][ 162.880]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_HEADER
[10:47:51:972][ 162.890]<D>[TRANS_SSL]<cli-uart>ssl read...
[10:47:53:244][ 164.160]<D>[tls]<cli-uart>tls read...
[10:47:53:244][ 164.160]<D>[HTTP_CLIENT]<cli-uart>on_message_begin
[10:47:53:255][ 164.170]<D>[HTTP_CLIENT]<cli-uart>HEADER=Date:Mon, 21 Mar 2022 02:47:52 GMT
[10:47:53:266][ 164.170]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Date, value=Mon, 21 Mar 2022 02:47:52 GMT
[10:47:53:266][ 164.180]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Type:application/json; charset=utf-8
[10:47:53:278][ 164.190]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Type, value=application/json; charset=utf-8
[10:47:53:288][ 164.200]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Length:1190
[10:47:53:288][ 164.200]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Length, value=1190
[10:47:53:299][ 164.210]<D>[HTTP_CLIENT]<cli-uart>HEADER=Connection:keep-alive
[10:47:53:311][ 164.220]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
[10:47:53:312][ 164.220]<D>[HTTP_CLIENT]<cli-uart>HEADER=ETag:W/"4a6-cOFrX2Qfhq0rvuEDMPW8DiA07s0"
[10:47:53:322][ 164.230]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=ETag, value=W/"4a6-cOFrX2Qfhq0rvuEDMPW8DiA07s0"
[10:47:53:332][ 164.240]<D>[HTTP_CLIENT]<cli-uart>HEADER=Vary:Accept-Encoding
[10:47:53:332][ 164.250]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Vary, value=Accept-Encoding
[10:47:53:354][ 164.250]<D>[HTTP_CLIENT]<cli-uart>HEADER=set-cookie:sails.sid=s%3ARNP37mZ1yg5JPN_nztkfrLTw7T6DfiHZ.IpMWnY10KXFx3TWMzrWaIozFuWeWlLH2C1aagI07nwo; Path=/; HttpOnly
[10:47:53:365][ 164.270]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=set-cookie, value=sails.sid=s%3ARNP37mZ1yg5JPN_nztkfrLTw7T6DfiHZ.IpMWnY10KXFx3TWMzrWaIozFuWeWlLH2C1aagI07nwo; Path=/; HttpOnly
[10:47:53:382][ 164.280]<D>[HTTP_CLIENT]<cli-uart>http_on_headers_complete, status=200, offset=337, nread=337
[10:47:53:382][ 164.290]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=175
[10:47:53:387][ 164.300]<D>[HTTP_CLIENT]<cli-uart>content_length = 1190
[10:47:53:388][ 164.300]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_DATA
[10:47:53:399][ 164.310]<D>[HTTP_CLIENT]<cli-uart>data_process=175, content_length=1190
[10:47:53:399][ 164.320]<D>[TRANS_SSL]<cli-uart>ssl read...
[10:47:53:410][ 164.320]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[10:47:53:410][ 164.330]<D>[tls]<cli-uart>tls read...
[10:47:53:421][ 164.330]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[10:47:53:421][ 164.340]<D>[HTTP_CLIENT]<cli-uart>data_process=687, content_length=1190
[10:47:53:432][ 164.340]<D>[TRANS_SSL]<cli-uart>ssl read...
[10:47:53:432][ 164.350]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[10:47:53:432][ 164.350]<D>[tls]<cli-uart>tls read...
[10:47:53:448][ 164.360]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=502
[10:47:53:448][ 164.360]<D>[HTTP_CLIENT]<cli-uart>data_process=1189, content_length=1190
[10:47:53:454][ 164.370]<D>[TRANS_SSL]<cli-uart>ssl read...
[10:47:53:454][ 164.370]<D>[tls]<cli-uart>tls read...
[10:47:53:466][ 164.380]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=1
[10:47:53:466][ 164.380]<D>[HTTP_CLIENT]<cli-uart>http_on_message_complete, parser=0x4023cb00
[10:47:53:482][ 164.390]<D>[example]<cli-uart>HTTP_EVENT_ON_FINISH
[10:47:53:482][ 164.390]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_RES_COMPLETE_HEADER
[10:47:53:488][ 164.400]<I>[example]<cli-uart>HTTPS Status = 200, content_length = 1190
[10:47:53:488][ 164.400]<I>[example]<cli-uart>++++++++++++++ HTTPS TEST OK
[10:47:53:488]
[10:47:53:499][ 164.410]<D>[example]<cli-uart>HTTP_EVENT_DISCONNECTED
[10:47:53:499]
[10:47:53:499](cli-uart)# 
```

● 自定义URL测试：
通过串口CLI输入 web http get <URL> 命令进行HTTP GET访问测试。比如输入 web http get http://occ.t-head.cn。
```cli
[13:53:35:495]web http get http://occ.t-head.cn
[13:53:35:495][ 273.070]<D>[HTTP_CLIENT]<cli-uart>###path:/
[13:53:35:511][ 273.070]<D>[HTTP_CLIENT]<cli-uart>client->state: 1, client->process_again: 0
[13:53:35:512][ 273.080]<D>[HTTP_CLIENT]<cli-uart>Begin connect to: http://occ.t-head.cn:80
[13:53:35:594][ 273.160]<D>[TRANS_TCP]<cli-uart>[sock=22],connecting to server IP:203.119.214.111,Port:80...
[13:53:35:656][ 273.230]<D>[example]<cli-uart>HTTP_EVENT_ON_CONNECTED
[13:53:35:657][ 273.230]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_INIT
[13:53:35:668][ 273.240]<D>[HTTP_CLIENT]<cli-uart>Write header[3]: GET / HTTP/1.1
[13:53:35:668]User-Agent: CK HTTP Client/1.0
[13:53:35:668]Host: occ.t-head.cn
[13:53:35:668]Content-Length: 0
[13:53:35:668]
[13:53:35:668]
[13:53:35:679][ 273.250]<D>[example]<cli-uart>HTTP_EVENT_HEADER_SENT
[13:53:35:679][ 273.250]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_CONNECTED
[13:53:35:689][ 273.260]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_HEADER
[13:53:35:745][ 273.320]<D>[HTTP_CLIENT]<cli-uart>on_message_begin
[13:53:35:756][ 273.320]<D>[HTTP_CLIENT]<cli-uart>HEADER=Date:Mon, 21 Mar 2022 05:53:35 GMT
[13:53:35:757][ 273.330]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Date, value=Mon, 21 Mar 2022 05:53:35 GMT
[13:53:35:767][ 273.340]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Type:text/html
[13:53:35:778][ 273.340]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Type, value=text/html
[13:53:35:778][ 273.350]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Length:357
[13:53:35:789][ 273.350]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Length, value=357
[13:53:35:789][ 273.360]<D>[HTTP_CLIENT]<cli-uart>HEADER=Connection:keep-alive
[13:53:35:800][ 273.370]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
[13:53:35:800][ 273.380]<D>[HTTP_CLIENT]<cli-uart>HEADER=Location:https://occ.t-head.cn/
[13:53:35:811][ 273.380]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Location, value=https://occ.t-head.cn/
[13:53:35:822][ 273.390]<D>[HTTP_CLIENT]<cli-uart>HEADER=Server:Tengine/Aserver
[13:53:35:822][ 273.400]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Server, value=Tengine/Aserver
[13:53:35:833][ 273.400]<D>[HTTP_CLIENT]<cli-uart>HEADER=EagleEye-TraceId:0b01b03b16478420157794068e9ee2
[13:53:35:844][ 273.410]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=EagleEye-TraceId, value=0b01b03b16478420157794068e9ee2
[13:53:35:856][ 273.420]<D>[HTTP_CLIENT]<cli-uart>HEADER=Timing-Allow-Origin:*
[13:53:35:856][ 273.430]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Timing-Allow-Origin, value=*
[13:53:35:867][ 273.440]<D>[HTTP_CLIENT]<cli-uart>http_on_headers_complete, status=301, offset=274, nread=274
[13:53:35:878][ 273.440]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=238
[13:53:35:878][ 273.450]<D>[HTTP_CLIENT]<cli-uart>content_length = 357
[13:53:35:879][ 273.450]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_DATA
[13:53:35:889][ 273.460]<I>[HTTP_CLIENT]<cli-uart>Redirect to https://occ.t-head.cn/
[13:53:35:900][ 273.470]<D>[example]<cli-uart>HTTP_EVENT_DISCONNECTED
[13:53:35:900][ 273.470]<D>[HTTP_CLIENT]<cli-uart>###path:/
[13:53:35:901][ 273.470]<D>[HTTP_CLIENT]<cli-uart>Read finish or server requests close
[13:53:35:912][ 273.480]<D>[example]<cli-uart>HTTP_EVENT_ON_FINISH
[13:53:35:912][ 273.490]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_RES_COMPLETE_HEADER
[13:53:35:923][ 273.490]<D>[HTTP_CLIENT]<cli-uart>client->state: 1, client->process_again: 1
[13:53:35:940][ 273.500]<D>[HTTP_CLIENT]<cli-uart>Begin connect to: https://occ.t-head.cn:443
[13:53:35:940][ 273.510]<D>[tls]<cli-uart>tls init...
[13:53:35:940][ 273.510]<D>[tls]<cli-uart>use_host:occ.t-head.cn, port:443
[13:53:36:035][ 273.610]<D>[tls]<cli-uart>_tls_net connect 0 
[13:53:36:036][ 273.610]<D>[tls]<cli-uart>tls connecting...
[13:53:36:043][ 273.610]<D>[tls]<cli-uart>handshake in progress...
[13:53:36:433][ 274.000]<D>[tls]<cli-uart>open new connection ok
[13:53:36:433][ 274.010]<D>[example]<cli-uart>HTTP_EVENT_ON_CONNECTED
[13:53:36:443][ 274.010]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_INIT
[13:53:36:443][ 274.020]<D>[HTTP_CLIENT]<cli-uart>Write header[3]: GET / HTTP/1.1
[13:53:36:454]User-Agent: CK HTTP Client/1.0
[13:53:36:454]Host: occ.t-head.cn
[13:53:36:454]Content-Length: 0
[13:53:36:454]
[13:53:36:454]
[13:53:36:455][ 274.030]<D>[example]<cli-uart>HTTP_EVENT_HEADER_SENT
[13:53:36:470][ 274.030]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_CONNECTED
[13:53:36:471][ 274.040]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_HEADER
[13:53:36:473][ 274.040]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:36:566][ 274.140]<D>[tls]<cli-uart>tls read...
[13:53:36:567][ 274.140]<D>[HTTP_CLIENT]<cli-uart>on_message_begin
[13:53:36:577][ 274.140]<D>[HTTP_CLIENT]<cli-uart>HEADER=Date:Mon, 21 Mar 2022 05:53:36 GMT
[13:53:36:588][ 274.150]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Date, value=Mon, 21 Mar 2022 05:53:36 GMT
[13:53:36:588][ 274.160]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Type:text/html;charset=UTF-8
[13:53:36:603][ 274.170]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Type, value=text/html;charset=UTF-8
[13:53:36:610][ 274.180]<D>[HTTP_CLIENT]<cli-uart>HEADER=Transfer-Encoding:chunked
[13:53:36:610][ 274.180]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Transfer-Encoding, value=chunked
[13:53:36:621][ 274.190]<D>[HTTP_CLIENT]<cli-uart>HEADER=Connection:keep-alive
[13:53:36:633][ 274.200]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Connection, value=keep-alive
[13:53:36:633][ 274.200]<D>[HTTP_CLIENT]<cli-uart>HEADER=Vary:Accept-Encoding
[13:53:36:644][ 274.210]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Vary, value=Accept-Encoding
[13:53:36:644][ 274.220]<D>[HTTP_CLIENT]<cli-uart>HEADER=Vary:Accept-Encoding
[13:53:36:655][ 274.220]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Vary, value=Accept-Encoding
[13:53:36:670][ 274.230]<D>[HTTP_CLIENT]<cli-uart>HEADER=Set-Cookie:XSRF-TOKEN=4d57b299-84e1-431f-877f-6e1ef183f30e; Path=/
[13:53:36:677][ 274.240]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Set-Cookie, value=XSRF-TOKEN=4d57b299-84e1-431f-877f-6e1ef183f30e; Path=/
[13:53:36:678][ 274.250]<D>[HTTP_CLIENT]<cli-uart>HEADER=X-Content-Type-Options:nosniff
[13:53:36:689][ 274.260]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=X-Content-Type-Options, value=nosniff
[13:53:36:701][ 274.270]<D>[HTTP_CLIENT]<cli-uart>HEADER=X-XSS-Protection:1; mode=block
[13:53:36:711][ 274.270]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=X-XSS-Protection, value=1; mode=block
[13:53:36:711][ 274.280]<D>[HTTP_CLIENT]<cli-uart>HEADER=Cache-Control:no-cache, no-store, max-age=0, must-revalidate
[13:53:36:722][ 274.290]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Cache-Control, value=no-cache, no-store, max-age=0, must-revalidate
[13:53:36:733][ 274.300]<D>[HTTP_CLIENT]<cli-uart>HEADER=Pragma:no-cache
[13:53:36:733][ 274.310]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Pragma, value=no-cache
[13:53:36:745][ 274.310]<D>[HTTP_CLIENT]<cli-uart>HEADER=Expires:0
[13:53:36:745][ 274.320]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Expires, value=0
[13:53:36:756][ 274.330]<D>[HTTP_CLIENT]<cli-uart>HEADER=X-Frame-Options:DENY
[13:53:36:767][ 274.330]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=X-Frame-Options, value=DENY
[13:53:36:767][ 274.340]<D>[HTTP_CLIENT]<cli-uart>HEADER=Content-Language:zh-CN
[13:53:36:778][ 274.340]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Content-Language, value=zh-CN
[13:53:36:778][ 274.350]<D>[HTTP_CLIENT]<cli-uart>HEADER=Server:Tengine/Aserver
[13:53:36:789][ 274.360]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Server, value=Tengine/Aserver
[13:53:36:800][ 274.370]<D>[HTTP_CLIENT]<cli-uart>HEADER=EagleEye-TraceId:0b83
[13:53:36:801][ 274.370]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=EagleEye-TraceId, value=0b83
[13:53:36:811][ 274.380]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:36:811][ 274.380]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:36:823][ 274.390]<D>[tls]<cli-uart>tls read...
[13:53:36:823][ 274.390]<D>[HTTP_CLIENT]<cli-uart>HEADER=Strict-Transport-Security:max-age=31536000
[13:53:36:837][ 274.400]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Strict-Transport-Security, value=max-age=31536000
[13:53:36:844][ 274.410]<D>[HTTP_CLIENT]<cli-uart>HEADER=Timing-Allow-Origin:*
[13:53:36:845][ 274.420]<D>[example]<cli-uart>HTTP_EVENT_ON_HEADER, key=Timing-Allow-Origin, value=*
[13:53:36:856][ 274.420]<D>[HTTP_CLIENT]<cli-uart>http_on_headers_complete, status=200, offset=612, nread=612
[13:53:36:867][ 274.430]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=2
[13:53:36:867][ 274.440]<D>[HTTP_CLIENT]<cli-uart>http_on_chunk_complete
[13:53:36:867][ 274.440]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=401
[13:53:36:878][ 274.450]<D>[HTTP_CLIENT]<cli-uart>content_length = -1
[13:53:36:878][ 274.450]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_REQ_COMPLETE_DATA
[13:53:36:888][ 274.460]<D>[HTTP_CLIENT]<cli-uart>data_process=403, content_length=-1
[13:53:36:888][ 274.460]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:36:900][ 274.470]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:36:900][ 274.470]<D>[tls]<cli-uart>tls read...
[13:53:36:910][ 274.480]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:36:910][ 274.480]<D>[HTTP_CLIENT]<cli-uart>data_process=779, content_length=-1
[13:53:36:921][ 274.490]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:36:922][ 274.490]<D>[tls]<cli-uart>tls read...
[13:53:36:922][ 274.500]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:36:936][ 274.500]<D>[HTTP_CLIENT]<cli-uart>data_process=1291, content_length=-1
[13:53:36:937][ 274.510]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:36:944][ 274.510]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:36:944][ 274.520]<D>[tls]<cli-uart>tls read...
[13:53:36:955][ 274.520]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:36:955][ 274.530]<D>[HTTP_CLIENT]<cli-uart>data_process=1803, content_length=-1
[13:53:36:966][ 274.540]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:36:966][ 274.540]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:36:978][ 274.550]<D>[tls]<cli-uart>tls read...
[13:53:36:978][ 274.550]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:36:988][ 274.550]<D>[HTTP_CLIENT]<cli-uart>data_process=2179, content_length=-1
[13:53:36:989][ 274.560]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:36:989][ 274.570]<D>[tls]<cli-uart>tls read...
[13:53:37:004][ 274.570]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:004][ 274.570]<D>[HTTP_CLIENT]<cli-uart>data_process=2691, content_length=-1
[13:53:37:011][ 274.580]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:011][ 274.580]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:022][ 274.590]<D>[tls]<cli-uart>tls read...
[13:53:37:022][ 274.590]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:033][ 274.600]<D>[HTTP_CLIENT]<cli-uart>data_process=3203, content_length=-1
[13:53:37:033][ 274.610]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:044][ 274.610]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:044][ 274.620]<D>[tls]<cli-uart>tls read...
[13:53:37:055][ 274.620]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:055][ 274.630]<D>[HTTP_CLIENT]<cli-uart>data_process=3579, content_length=-1
[13:53:37:066][ 274.630]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:066][ 274.640]<D>[tls]<cli-uart>tls read...
[13:53:37:066][ 274.640]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=133
[13:53:37:077][ 274.650]<D>[HTTP_CLIENT]<cli-uart>http_on_chunk_complete
[13:53:37:077][ 274.650]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=372
[13:53:37:088][ 274.660]<D>[HTTP_CLIENT]<cli-uart>data_process=4084, content_length=-1
[13:53:37:088][ 274.660]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:103][ 274.670]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:103][ 274.670]<D>[tls]<cli-uart>tls read...
[13:53:37:110][ 274.680]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:110][ 274.680]<D>[HTTP_CLIENT]<cli-uart>data_process=4596, content_length=-1
[13:53:37:122][ 274.690]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:122][ 274.690]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:122][ 274.700]<D>[tls]<cli-uart>tls read...
[13:53:37:137][ 274.700]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:143][ 274.710]<D>[HTTP_CLIENT]<cli-uart>data_process=4972, content_length=-1
[13:53:37:144][ 274.710]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:144][ 274.720]<D>[tls]<cli-uart>tls read...
[13:53:37:155][ 274.720]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:155][ 274.730]<D>[HTTP_CLIENT]<cli-uart>data_process=5484, content_length=-1
[13:53:37:166][ 274.730]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:167][ 274.740]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:177][ 274.740]<D>[tls]<cli-uart>tls read...
[13:53:37:177][ 274.750]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:188][ 274.750]<D>[HTTP_CLIENT]<cli-uart>data_process=5996, content_length=-1
[13:53:37:188][ 274.760]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:203][ 274.760]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:204][ 274.770]<D>[tls]<cli-uart>tls read...
[13:53:37:204][ 274.770]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:210][ 274.780]<D>[HTTP_CLIENT]<cli-uart>data_process=6372, content_length=-1
[13:53:37:210][ 274.790]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:221][ 274.790]<D>[tls]<cli-uart>tls read...
[13:53:37:222][ 274.790]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=254
[13:53:37:233][ 274.800]<D>[HTTP_CLIENT]<cli-uart>http_on_chunk_complete
[13:53:37:233][ 274.800]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=250
[13:53:37:243][ 274.810]<D>[HTTP_CLIENT]<cli-uart>data_process=6876, content_length=-1
[13:53:37:243][ 274.820]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:254][ 274.820]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:254][ 274.830]<D>[tls]<cli-uart>tls read...
[13:53:37:255][ 274.830]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:270][ 274.830]<D>[HTTP_CLIENT]<cli-uart>data_process=7388, content_length=-1
[13:53:37:270][ 274.840]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:277][ 274.850]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:277][ 274.850]<D>[tls]<cli-uart>tls read...
[13:53:37:288][ 274.860]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:288][ 274.860]<D>[HTTP_CLIENT]<cli-uart>data_process=7764, content_length=-1
[13:53:37:299][ 274.870]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:299][ 274.870]<D>[tls]<cli-uart>tls read...
[13:53:37:299][ 274.870]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:310][ 274.880]<D>[HTTP_CLIENT]<cli-uart>data_process=8276, content_length=-1
[13:53:37:310][ 274.890]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:322][ 274.890]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:322][ 274.900]<D>[tls]<cli-uart>tls read...
[13:53:37:337][ 274.900]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:337][ 274.910]<D>[HTTP_CLIENT]<cli-uart>data_process=8788, content_length=-1
[13:53:37:344][ 274.910]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:344][ 274.920]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:355][ 274.920]<D>[tls]<cli-uart>tls read...
[13:53:37:355][ 274.930]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:366][ 274.930]<D>[HTTP_CLIENT]<cli-uart>data_process=9164, content_length=-1
[13:53:37:366][ 274.940]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:366][ 274.940]<D>[tls]<cli-uart>tls read...
[13:53:37:377][ 274.950]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:377][ 274.950]<D>[HTTP_CLIENT]<cli-uart>data_process=9676, content_length=-1
[13:53:37:387][ 274.960]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:387][ 274.960]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:402][ 274.970]<D>[tls]<cli-uart>tls read...
[13:53:37:403][ 274.970]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:409][ 274.980]<D>[HTTP_CLIENT]<cli-uart>data_process=10188, content_length=-1
[13:53:37:410][ 274.980]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:421][ 274.990]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:421][ 274.990]<D>[tls]<cli-uart>tls read...
[13:53:37:432][ 275.000]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:432][ 275.000]<D>[HTTP_CLIENT]<cli-uart>data_process=10564, content_length=-1
[13:53:37:432][ 275.010]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:443][ 275.010]<D>[tls]<cli-uart>tls read...
[13:53:37:443][ 275.020]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:454][ 275.020]<D>[HTTP_CLIENT]<cli-uart>data_process=11076, content_length=-1
[13:53:37:455][ 275.030]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:470][ 275.030]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:470][ 275.040]<D>[tls]<cli-uart>tls read...
[13:53:37:478][ 275.040]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:478][ 275.050]<D>[HTTP_CLIENT]<cli-uart>data_process=11588, content_length=-1
[13:53:37:488][ 275.050]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:488][ 275.060]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:503][ 275.070]<D>[tls]<cli-uart>tls read...
[13:53:37:503][ 275.070]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:510][ 275.070]<D>[HTTP_CLIENT]<cli-uart>data_process=11964, content_length=-1
[13:53:37:510][ 275.080]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:510][ 275.080]<D>[tls]<cli-uart>tls read...
[13:53:37:521][ 275.090]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:521][ 275.090]<D>[HTTP_CLIENT]<cli-uart>data_process=12476, content_length=-1
[13:53:37:532][ 275.100]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:532][ 275.100]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:543][ 275.110]<D>[tls]<cli-uart>tls read...
[13:53:37:543][ 275.110]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:554][ 275.120]<D>[HTTP_CLIENT]<cli-uart>data_process=12988, content_length=-1
[13:53:37:554][ 275.130]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:570][ 275.130]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:570][ 275.140]<D>[tls]<cli-uart>tls read...
[13:53:37:570][ 275.140]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:576][ 275.150]<D>[HTTP_CLIENT]<cli-uart>data_process=13364, content_length=-1
[13:53:37:576][ 275.150]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:588][ 275.160]<D>[tls]<cli-uart>tls read...
[13:53:37:588][ 275.160]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:599][ 275.170]<D>[HTTP_CLIENT]<cli-uart>data_process=13876, content_length=-1
[13:53:37:599][ 275.170]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:609][ 275.180]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:610][ 275.180]<D>[tls]<cli-uart>tls read...
[13:53:37:621][ 275.190]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:621][ 275.190]<D>[HTTP_CLIENT]<cli-uart>data_process=14388, content_length=-1
[13:53:37:621][ 275.200]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:636][ 275.200]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:637][ 275.210]<D>[tls]<cli-uart>tls read...
[13:53:37:643][ 275.210]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:643][ 275.220]<D>[HTTP_CLIENT]<cli-uart>data_process=14764, content_length=-1
[13:53:37:654][ 275.220]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:654][ 275.230]<D>[tls]<cli-uart>tls read...
[13:53:37:670][ 275.230]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:670][ 275.240]<D>[HTTP_CLIENT]<cli-uart>data_process=15276, content_length=-1
[13:53:37:676][ 275.240]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:676][ 275.250]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:677][ 275.250]<D>[tls]<cli-uart>tls read...
[13:53:37:688][ 275.260]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:699][ 275.260]<D>[HTTP_CLIENT]<cli-uart>data_process=15788, content_length=-1
[13:53:37:699][ 275.270]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:699][ 275.270]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:710][ 275.280]<D>[tls]<cli-uart>tls read...
[13:53:37:710][ 275.280]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:721][ 275.290]<D>[HTTP_CLIENT]<cli-uart>data_process=16164, content_length=-1
[13:53:37:721][ 275.300]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:732][ 275.300]<D>[tls]<cli-uart>tls read...
[13:53:37:733][ 275.300]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:743][ 275.310]<D>[HTTP_CLIENT]<cli-uart>data_process=16676, content_length=-1
[13:53:37:743][ 275.310]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:754][ 275.320]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:754][ 275.330]<D>[tls]<cli-uart>tls read...
[13:53:37:754][ 275.330]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:770][ 275.330]<D>[HTTP_CLIENT]<cli-uart>data_process=17188, content_length=-1
[13:53:37:770][ 275.340]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:776][ 275.340]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:776][ 275.350]<D>[tls]<cli-uart>tls read...
[13:53:37:788][ 275.350]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:788][ 275.360]<D>[HTTP_CLIENT]<cli-uart>data_process=17564, content_length=-1
[13:53:37:799][ 275.370]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:799][ 275.370]<D>[tls]<cli-uart>tls read...
[13:53:37:799][ 275.370]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:810][ 275.380]<D>[HTTP_CLIENT]<cli-uart>data_process=18076, content_length=-1
[13:53:37:810][ 275.390]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:821][ 275.390]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:821][ 275.400]<D>[tls]<cli-uart>tls read...
[13:53:37:836][ 275.400]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:836][ 275.410]<D>[HTTP_CLIENT]<cli-uart>data_process=18588, content_length=-1
[13:53:37:842][ 275.410]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:842][ 275.420]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:854][ 275.420]<D>[tls]<cli-uart>tls read...
[13:53:37:854][ 275.430]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:865][ 275.430]<D>[HTTP_CLIENT]<cli-uart>data_process=18964, content_length=-1
[13:53:37:865][ 275.440]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:865][ 275.440]<D>[tls]<cli-uart>tls read...
[13:53:37:875][ 275.450]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:887][ 275.450]<D>[HTTP_CLIENT]<cli-uart>data_process=19476, content_length=-1
[13:53:37:887][ 275.460]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:887][ 275.460]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:903][ 275.470]<D>[tls]<cli-uart>tls read...
[13:53:37:903][ 275.470]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:909][ 275.480]<D>[HTTP_CLIENT]<cli-uart>data_process=19988, content_length=-1
[13:53:37:909][ 275.480]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:920][ 275.490]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:920][ 275.490]<D>[tls]<cli-uart>tls read...
[13:53:37:931][ 275.500]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:37:931][ 275.500]<D>[HTTP_CLIENT]<cli-uart>data_process=20364, content_length=-1
[13:53:37:942][ 275.510]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:942][ 275.510]<D>[tls]<cli-uart>tls read...
[13:53:37:943][ 275.520]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:37:954][ 275.520]<D>[HTTP_CLIENT]<cli-uart>data_process=20876, content_length=-1
[13:53:37:954][ 275.530]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:37:965][ 275.530]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:37:965][ 275.540]<D>[tls]<cli-uart>tls read...
[13:53:37:976][ 275.540]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=14
[13:53:37:976][ 275.550]<D>[HTTP_CLIENT]<cli-uart>http_on_chunk_complete
[13:53:37:987][ 275.550]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=491
[13:53:37:987][ 275.560]<D>[HTTP_CLIENT]<cli-uart>data_process=21381, content_length=-1
[13:53:38:003][ 275.570]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:38:003][ 275.570]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:38:010][ 275.580]<D>[tls]<cli-uart>tls read...
[13:53:38:010][ 275.580]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:38:020][ 275.580]<D>[HTTP_CLIENT]<cli-uart>data_process=21757, content_length=-1
[13:53:38:020][ 275.590]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:38:020][ 275.600]<D>[tls]<cli-uart>tls read...
[13:53:38:031][ 275.600]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:38:031][ 275.600]<D>[HTTP_CLIENT]<cli-uart>data_process=22269, content_length=-1
[13:53:38:043][ 275.610]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:38:043][ 275.610]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:38:053][ 275.620]<D>[tls]<cli-uart>tls read...
[13:53:38:054][ 275.620]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:38:070][ 275.630]<D>[HTTP_CLIENT]<cli-uart>data_process=22781, content_length=-1
[13:53:38:070][ 275.640]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:38:076][ 275.640]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:38:076][ 275.650]<D>[tls]<cli-uart>tls read...
[13:53:38:076][ 275.650]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:38:087][ 275.660]<D>[HTTP_CLIENT]<cli-uart>data_process=23157, content_length=-1
[13:53:38:087][ 275.660]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:38:098][ 275.670]<D>[tls]<cli-uart>tls read...
[13:53:38:098][ 275.670]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:38:109][ 275.680]<D>[HTTP_CLIENT]<cli-uart>data_process=23669, content_length=-1
[13:53:38:109][ 275.680]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:38:121][ 275.690]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:38:121][ 275.690]<D>[tls]<cli-uart>tls read...
[13:53:38:131][ 275.700]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=512
[13:53:38:132][ 275.700]<D>[HTTP_CLIENT]<cli-uart>data_process=24181, content_length=-1
[13:53:38:132][ 275.710]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:38:142][ 275.710]<D>[TRANS_SSL]<cli-uart>remain data in cache, need to read again
[13:53:38:143][ 275.720]<D>[tls]<cli-uart>tls read...
[13:53:38:154][ 275.720]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=376
[13:53:38:154][ 275.730]<D>[HTTP_CLIENT]<cli-uart>data_process=24557, content_length=-1
[13:53:38:165][ 275.730]<D>[TRANS_SSL]<cli-uart>ssl read...
[13:53:38:165][ 275.740]<D>[tls]<cli-uart>tls read...
[13:53:38:176][ 275.740]<D>[example]<cli-uart>HTTP_EVENT_ON_DATA, len=332
[13:53:38:176][ 275.750]<D>[HTTP_CLIENT]<cli-uart>http_on_chunk_complete
[13:53:38:187][ 275.750]<D>[HTTP_CLIENT]<cli-uart>http_on_chunk_complete
[13:53:38:187][ 275.760]<D>[HTTP_CLIENT]<cli-uart>http_on_message_complete, parser=0x4026ad70
[13:53:38:203][ 275.760]<D>[example]<cli-uart>HTTP_EVENT_ON_FINISH
[13:53:38:203][ 275.770]<D>[HTTP_CLIENT]<cli-uart>HTTP_STATE_RES_COMPLETE_HEADER
[13:53:38:214][ 275.770]<I>[example]<cli-uart>HTTP GET Status = 200, content_length = -1
[13:53:38:215][ 275.780]<I>[example]<cli-uart>++++++++++++++ HTTP GET TEST OK
[13:53:38:215]
[13:53:38:215]
[13:53:38:215](cli-uart)#
```