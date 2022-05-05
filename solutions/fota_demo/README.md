# 概述

FOTA升级的DEMO。FOTA的云服务在OCC，包括固件的管理，许可证的管理，设备的管理等。
具体可以参考OCC上的博文《YoC RTOS 实战：FoTA系统升级》

# 使用

## 通过CDK

在CDK的首页，通过搜索关键字fota，可以找到fota_demo，然后创建工程。

CDK的使用可以参考YoCBook 《CDK开发快速上手》 章节， 链接 https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html

具体步骤参考OCC上的博文《YoC RTOS 实战：FoTA系统升级》
## 通过命令行

### 下载

```bash
mkdir workspace
cd workspace
yoc init
yoc install fota_demo
```

### 编译

```bash
make clean;make
```

### 烧录

```bash
make flashall
```

从服务器上下载镜像包镜像烧录：

```bash
product flash 20210304215611061_factory.zip -a -f ../../components/chip_d1/d1_flash.elf -x gdbinit
```

### 调试

```bash
riscv64-unknown-elf-gdb yoc.elf -x gdbinit
```

# 联网配置

通过串口终端输入:

```cli
kv set wifi_ssid <your_wifi_ssid>
kv set wifi_psk <your_wifi_password>
```

- your_wifi_ssid：你的wifi名字
- your_wifi_password：你的wifi密码

# fota配置

系统默认60秒检查一次升级，升级需要进行如下配置

## 设置设备ID

ID号从OCC上获取。

通过串口终端输入:

```cli
kv set device_id 3a1ad548044000006230e778e3b3ec26
```

## 设置产品类型

通过串口终端输入:

```cli
kv set model xxx_model
```

## 使能FOTA检测

默认使能。

通过串口终端输入:

```cli
kv setint fota_en 0
```

或者

```cli
kv setint fota_en 1
```

配置完成之后需要复位。

# 运行

从OCC下载完FOTA相关的数据并且校验成功之后就会开始自动重启并且进行固件更新。
以下为串口的打印。

```cli
[15:49:06:789][  52.650]<D>app_fota cop_fota.c[131]: {"code":0,"total_size":840248,"cur_size":840248,"percent":100,"speed":23}
[15:49:06:789][  52.660]<D>fota fota.c[242]: fota_task download! wait......
[15:49:06:799][  52.660]<W>fota-httpc httpc.c[238]: http_read done: offset:840248 tsize:840248
[15:49:06:811][  52.670]<D>fota fota.c[250]: fota_task FOTA_DOWNLOAD! total:840248 offset:840248
[15:49:06:811][  52.680]<D>fota fota.c[251]: ##read: 0
[15:49:06:811][  52.680]<D>fota fota.c[268]: read size 0.
[15:49:06:821][  52.690]<D>app_fota cop_fota.c[141]: FOTA VERIFY :2
[15:49:06:822][  52.690]<D>fotav fota_verify.c[76]: start fota verify...
[15:49:06:832][  52.700]<D>fotav fota_verify.c[128]: image_size:839836
[15:49:06:832][  52.700]<D>fotav fota_verify.c[129]: digest_type:1
[15:49:06:833][  52.710]<D>fotav fota_verify.c[130]: sign_type:1
[15:49:06:844][  52.710]<D>fotav fota_verify.c[131]: hash_len:20
[15:49:06:844][  52.710]<D>fotav fota_verify.c[132]: signature_len:128
[15:49:06:856][  52.720]<D>fotav fota_verify.c[133]: signature_offset:848028
[15:49:06:856][  52.720]<D>fotav fota_verify.c[134]: hash_offset:848284
[15:49:07:002][  52.870]<I>fotav fota_verify.c[189]: ###fota data hash v ok.
[15:49:07:003][  52.880]<D>fota fota.c[174]: fota_release,174
[15:49:07:014][  52.880]<D>fota-httpc httpc.c[144]: httpc cleanup...
[15:49:07:014][  52.890]<D>fota-httpc httpc.c[44]: HTTP_EVENT_DISCONNECTED
[15:49:07:026][  52.890]<D>fota fota.c[289]: fota data verify ok.
[15:49:07:026][  52.900]<D>app_fota cop_fota.c[144]: FOTA FINISH :4
[15:49:08:075][34]HELLO! BOOT0 is starting![Sep 18 2021, 11:27:51]
[15:49:08:075][39]BOOT0 commit : 3b45046
[15:49:08:076][42]set pll start
[15:49:08:087][44]periph0 has been enabled
[15:49:08:087][47]set pll end
[15:49:08:087][48][pmu]: bus read error
[15:49:08:087][50]board init ok
[15:49:08:087][52]enable_jtag
[15:49:08:087][54]DRAM only have internal ZQ!!
[15:49:08:109][57]get_pmu_exist() = -1
[15:49:08:109][59]ddr_efuse_type: 0x0
[15:49:08:109][62][AUTO DEBUG] single rank and full DQ!
[15:49:08:109][66]ddr_efuse_type: 0x0
[15:49:08:112][69][AUTO DEBUG] rank 0 row = 15 
[15:49:08:112][72][AUTO DEBUG] rank 0 bank = 8 
[15:49:08:112][75][AUTO DEBUG] rank 0 page size = 2 KB 
[15:49:08:112][79]DRAM BOOT DRIVE INFO: V0.24
[15:49:08:123][82]DRAM CLK = 792 MHz
[15:49:08:123][84]DRAM Type = 3 (2:DDR2,3:DDR3)
[15:49:08:123][87]DRAMC ZQ value: 0x7b7bfb
[15:49:08:123][90]DRAM ODT value: 0x42.
[15:49:08:137][93]ddr_efuse_type: 0x0
[15:49:08:137][95]DRAM SIZE =512 M
[15:49:08:137][99]DRAM simple test OK.
[15:49:08:137][101]dram size =512
[15:49:08:137][103]spinor id is: ef 40 18, read cmd: 0b
[15:49:08:148][107]Succeed in reading toc file head.
[15:49:08:148][110]The size of toc is cc000.
[15:49:08:283][247]start to copy bootloader.
[15:49:08:322][281]copy bootloader over.
[15:49:08:322][284]Entry_name        = melis-lz4
[15:49:08:322][287]Entry_data_offset = 0x400
[15:49:08:335][290]Entry_data_len    = 0xc9011
[15:49:08:335][293]run_addr          = 0x0
[15:49:08:335][295]image_base        = 0x37cd8189
[15:49:08:335][299]come to LZ4 decompress.
[15:49:08:351][308]LZ4 decompress ok.
[15:49:08:351][310]Jump to second Boot.
[15:49:08:351][313]jump to bootloader,[0x40000000]
[15:49:08:365]
[15:49:08:365]Welcome boot2.0!
[15:49:08:365]build: Feb 15 2022 15:41:15
[15:49:08:366]cpu clock is 1008000000Hz
[15:49:09:054][boot][I] fota data hash verify ok
[15:49:09:055][boot][I] start to upgrade
[15:49:09:180][boot][I] fd:0x40025b20,fd_num:0
[15:49:09:251][boot][I] start FULL update
[15:49:25:111][boot][I] fd:0x40025b20,fd_num:0
[15:49:25:184][boot][I] fd:0x40025b20,fd_num:0
[15:49:25:338][boot][I] suc update ^_^
```