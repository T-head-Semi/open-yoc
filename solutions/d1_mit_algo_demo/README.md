# 概述

基于`d1`芯片的使用mit算法的唤醒demo。

# 使用

## 通过CDK

在CDK的首页，通过搜索`mit_algo_demo`，可以找到mit_algo_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节， 

## 通过命令行

```bash
mkdir workspace
cd workspace
yoc init
yoc install mit_algo_demo
```

### 编译

```bash
cd solutions/mit_algo_demo/
make clean;make
```

## 烧录

安装[全志USB烧录驱动](https://www.aw-ol.com/downloads?cat=5)及[PhoenixSuit](https://www.aw-ol.com/downloads?cat=5)，后双击运行PhoenixSuit ，在**⼀键刷机**⻚⾯选择要烧写镜像（solutions/mit_algo_demo/out/yoc_rtos_8M.img），烧写⽅式第一次全新烧写可选择**分区擦除升级**
按住 FEL 按键重新上电即可，具体使用可参考 https://d1.docs.aw-ol.com/source/2_gettools/#livesuit

**编译及烧写**章节

# 运行

串口输入mit 0，即可运行语音算法，通过发出语音“天猫精灵”，串口将会输出“wakeup success”，即表示唤醒成功

```cli
mit 0

(cli-uart)# [     0.000000][E][1970-01-01 00:00:09.780000][MIT-RTOS] version: MIT_RTOS_VERSION_20210830_193000 V1.10.26.099 D1[] [alg]
[     0.000000][E][1970-01-01 00:00:09.800000][MIT-RTOS] enable_init_more_log:0
[     0.000000][E][1970-01-01 00:00:09.820000][MIT-RTOS] FE parameter: gain_tune(aec_gain): 0; agc_algorithm:1; agc_power:8; agc_level:4; data_interleaved:1
[     0.000000][E][1970-01-01 00:00:09.860000][MIT-RTOS] womx: set aec_gain of fe_config_inst_switch to 30 db
[     0.000000][E][1970-01-01 00:00:09.880000][FE_MAIN] NLSFE SDK Version 1.1.0
[     0.000000][E][1970-01-01 00:00:09.920000][FE_ALG_OPT] in: ch 3, len 640; out ch 2, len 640
[     0.000000][E][1970-01-01 00:00:09.940000][FE_ALG_OPT] objMemSize 771936 tmpMemSize 23048 sramMemSize 0
[     0.000000][E][1970-01-01 00:00:09.970000][FE_MAIN] init fe handler instance
[     0.000000][E][1970-01-01 00:00:09.990000][FE_MAIN] in channels 3 out channels 2
[     0.000000][E][1970-01-01 00:00:10.050000][FE_MAIN] done, init handler instance
[     0.000000][E][1970-01-01 00:00:10.350000][KWS_WRAPPER_MULTIINST_YINJIE] kws version is: tianmaojingling_syllable_pangu
mit_rtos_memory_statistic_add(new) size of wakeup_inst_t 104 to 104 Bytes
[     0.000000][E][1970-01-01 00:00:10.400000][KWS_WRAPPER_MULTIINST_YINJIE] womx kws.local_threshold=0.400000
[     0.000000][E][1970-01-01 00:00:10.730000][KWS_WRAPPER_MULTIINST_YINJIE] kws version is: tianmaojingling_syllable_pangu
[     0.000000][E][1970-01-01 00:00:10.780000][KWS_WRAPPER_MULTIINST_YINJIE] womx kws.local_threshold=0.400000
[     0.000000][E][1970-01-01 00:00:11.180000][VAD_KWS_WRAPPER] donot call kws_init
[     0.000000][E][1970-01-01 00:00:11.230000][MIT-RTOS] mit_rtos_init MEMORY ALL 820480 Bytes
call mit_rtos_init() return 0
call mit_rtos_start() return 0
buffer size changed (request: 30708, get: 20472)
[  15.140]<I>[mit]<test_mit>wakeup success
```



