# TYDB使用说明

# 1. 工具说明
T-head YOC Debug Bridge，简称TYDB，即debug工具。主要功能有：

- 上位机和设备之间上传/下载/删除文件
- 运行设备的shell（命令）
- 支持cct固件下载功能
- 支持ymodem协议固件下载功能
- 录音功能
- 串口交互

目前上位机和设备之间的通信方式采用串口通信，支持高速1.5M的波特率通信，需要使用高速串口模块。

# 2. 文件传输
上位机使用在windows环境下，cmd 进入tydb.exe所在路径下执行如下控制指令。
## 2.1 从设备端pull文件
### 命令示例
```shell
#命令 串口号 功能 设备端文件         PC端保存
tydb COM25 pull /data/prompt/1.mp3  output.mp3
```
### 返回值：

- pull成功：
```shell
total length: xxxByte
pass time:  time_value
pull success,saved as output.mp3
```

- pull失败(设备端文件不存在):
```shell
failed
file do not exists, please check your file.
```
## 2.2 往设备端push文件
### 命令示例
```shell
#命令 串口号   功能  设备端文件               PC端要上传的文件
tydb  COM25  push /data/factory/save.mp3  output.mp3
```
注意：若push文件名与设备端文件系统已有文件重名，会直接覆盖设备端已有文件
### 返回值：

- push成功：

```shell
total length: xxxByte
pass time: time_value
push success,saved in mcu as dst_path
```

- push失败
```shell
failed
```
## 2.3 删除设备端文件
### 命令示例
```shell
#命令 串口号 功能          PC端要删除的文件
tydb COM25 shell rm -r   /data/factory/save.mp3
```
### 返回值：

- delete成功：
```shell
delete success.
```

- delete失败(设备端文件不存在):
```shell
please check whether the file exists.
```
## 2.4 运行设备端cli命令
### 命令示例
```shell
#命令 串口号  功能  cli命令
tydb  COM25  shell  help
```
### 返回值：
```shell
====Support 4 cmds once, seperate by ; ====
help      : print this
debug     : debug
sysver    : system version
reboot    : reboot system
time      : system time
cpuusage  : show cpu usage
..........................
fareset   : device factory
cli cmd num : 79
```
# 3. 文件传输
## 3.1 CCT空片烧写
CCT是CSKY的串口升级软件，tydb支持CCT升级模式。

### 命令示例
```shell
#命令 串口号  功能   烧录文件的地址
tydb  COM     cct   image.bin
```
下载时，先给给板子上电，然后敲击命令。
如果软件提示失败，只能断电再上电，重新敲命令。CCT烧写主要适用于救砖，可以烧写一个sboot.bin进去，在用ymodem协议下载sfota.bin和sdata.bin。
### 返回值：
```c
TRACE +16.206 connect succuss!
get_version: 3
use input imgwriter
TRACE +0.011 download_init OK
TRACE +0.022 download_init OK
TRACE +3.278 connect succuss! %)
TRACE +0.001 download_init OK
Writing at 0x00fe3000... (100 %)
update time:157s
```
### 修改cct默认使用的imgwriter：
打开ini.json文件，将 input_imgwriter 字段由False 改为 True。并修改imgwriter_path字段和imgwriter_mnt_path为实际路径。
烧录时tydb会出现 use input imgwriter 的提示，表明使用了外部输入的imgwriter进行烧录。
## 3.2 Ymodem串口升级
### 命令示例
```shell
#命令 串口号  功能     烧录文件的地址
tydb  COM   Ymodem  fota.bin
```
### 返回值：
```c
Start trainsfering
transfered (100 %)
Packet End >>>
Task Done!
File: sfota.bin
Size: 3156777Bytes
Packets: 771
Speed: 100642B/s
```
# 4. 使用录音功能
## 4.1 串口录音
### 命令示例
```shell
#命令 串口号  功能     使用串口录音 录音文件的地址  录音长度 通道数  是否使用压缩（使用填y，不使用不填）
tydb  COM   record   uart      record.pcm     30s      5        y
录音长度支持秒(s)、分(m)、时(h)输入，若不输入单位，默认为s
程序启动后会自动录音，到达时间后自动关闭
```
### 返回值：
```shell
recording 1 m
recording 60 s
start
recorded 5625 KB data
recording finish
```
## 4.2 网络录音
### 命令示例
```shell
PC端：
#命令 功能     使用网络录音  ip:port       录音文件的地址
tydb record   web        0.0.0.0:8090  record/download
MCU：
开始：
#命令 开始      主机ip:port
tgsp start ws://192.168.50.216:8090 test.pcm [1(reccount)] [200000(rbsize)]   []可选
结束：tgsp stop
```
# 5. 透传功能
实现了简单的串口助手功能
### 命令示例
```shell
#命令 串口号
tydb COM
```
打开之后，就可以看串口输出。想要输入的话就直接在命令行里输入就行了，和SecureCRT交互模式类似。

# 6. 其他
## 6.1 打开日志功能、修改波特率
第一次运行tydb时，会生成一个默认的ini.json文件。通过这个文件，可以修改波特率、打开log。
log文件会记录每次运行的过程，以便复查。
如果想开启该功能，将  "log": "False"  改为  "log": "True"，修改"log_path"字段为想要保存的地址。
如果想更改波特率，将"baudrate": "1500000"这个字段中的1500000改成你所需要的波特率即可。

## 6.2 查看帮助及查询可用串口

```shell
#命令 
tydb -h
```

返回值：

```shell
TYDB help:
pull:           tydb <COM> pull <path_on_mcu> <path_on_pc>
push:           tydb <COM> push <path_on_mcu> <path_on_pc>
remove:         tydb <COM> shell rm -r <path_to_delete>
shell:          tydb <COM> shell <cli_cmd>
cct download:   tydb <COM> cct <path_to_download_file>
Ymodem:         tydb <COM> Ymodem or ymodem <path_to_download_file> [<name>]
recording:
     wifi:      tydb record web <ip:port> <path_to_record_folder>
     uart:      tydb <COM> record uart <path_to_record_file> <recording_time(s(second),m(minute),h(hour))> <number of channels> [y](y:use compression)
                e.g. tydb COM22 record uart test.pcm 6 5 y
uart transparent transmit:
                tydb <COM>

avaliable ports:
COM8 - Silicon Labs CP210x USB to UART Bridge (COM8)
```

## 6.3 网络测试

网络录音有需要较好的网络环境，tydb支持网络测试：

```
#命令 
tydb net_test
```

返回值：

```
Retrieving speedtest.net configuration...
Testing from Alibaba (........)...
Retrieving speedtest.net server list...
Selecting best server based on ping...
Hosted by H&B Communications (Holyrood, KS) [106.27 km]: 200.81 ms
Testing download speed.....................
Download: 2.22 Mbit/s
Testing upload speed.......................
Upload: 3.40 Mbit/s
```