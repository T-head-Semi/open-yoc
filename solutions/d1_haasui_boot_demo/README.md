# 概述
haasui_boot_demo是基于d1开发板的UI demo，集成了各种基础控件的简单示例。UI小程序相关的开发指南可以参考[官方指南](https://haas.iot.aliyun.com/haasui/quickstart)。`data/resources`目录下预置了小程序编译打包好的相关资源文件。

# CDK
在CDK的首页，通过搜索d1，可以找到d1_haasui_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节

# 烧录
通过CDK编译完成之后会在out目录下生成yoc_rtos_8M.img文件，此文件即为最终的镜像文件。
使用全志烧写工具进行烧录。如何安装及烧写请参考[全志官方网站](https://d1.docs.aw-ol.com/study/study_4compile/#phoenixsuit)。

# 启动
烧录完成之后系统会自动启动，LCD屏幕上会显示UI，根据界面的提示可以进行触屏操作。

```cli
[16:53:38:591][142]HELLO! BOOT0 is starting![Sep 18 2021, 11:27:51]
[16:53:38:592][147]BOOT0 commit : 3b45046
[16:53:38:592][149]set pll start
[16:53:38:604][151]periph0 has been enabled
[16:53:38:604][154]set pll end
[16:53:38:604][156][pmu]: bus read error
[16:53:38:604][158]board init ok
[16:53:38:604][160]enable_jtag
[16:53:38:604][162]DRAM only have internal ZQ!!
[16:53:38:614][165]get_pmu_exist() = -1
[16:53:38:615][167]DRAM BOOT DRIVE INFO: V0.24
[16:53:38:615][170]DRAM CLK = 792 MHz
[16:53:38:615][172]DRAM Type = 3 (2:DDR2,3:DDR3)
[16:53:38:628][176]DRAMC ZQ value: 0x7b7bfb
[16:53:38:628][178]DRAM ODT value: 0x42.
[16:53:38:629][181]ddr_efuse_type: 0x0
[16:53:38:629][184]DRAM SIZE =1024 M
[16:53:38:629][188]DRAM simple test OK.
[16:53:38:640][190]dram size =1024
[16:53:38:640][192]spinor id is: 85 60 17, read cmd: 6b
[16:53:38:640][196]SF: Need set QEB func for 85 flash
[16:53:38:650][199]Succeed in reading toc file head.
[16:53:38:650][203]The size of toc is 25c000.
[16:53:38:751][306]start to copy bootloader.
[16:53:38:767][316]copy bootloader over.
[16:53:38:767][319]Entry_name        = melis-lz4
[16:53:38:767][322]Entry_data_offset = 0x400
[16:53:38:781][325]Entry_data_len    = 0x25b169
[16:53:38:781][328]run_addr          = 0x0
[16:53:38:781][331]image_base        = 0x930036b1
[16:53:38:782][334]come to LZ4 decompress.
[16:53:38:813][360]LZ4 decompress ok.
[16:53:38:813][363]Jump to second Boot.
[16:53:38:814][365]jump to bootloader,[0x40000000]
[16:53:38:826]
[16:53:38:826]Welcome boot2.0!
[16:53:38:826]build: Sep 18 2021 02:15:14
[16:53:38:827]cpu clock is 1008000000Hz
[16:53:38:846]jump to [0x40040000]
[16:53:38:846]j m
[16:53:38:846]j 0x40040000
[16:53:38:875](cli-uart)# ###YoC###[Dec 22 2021,07:45:41]
[16:53:38:875]cpu clock is 1008000000Hz
[16:53:38:875]spi0 clock is 50000000Hz
[16:53:38:876]display init ok.
[16:53:38:906][W][hal_twi_sys_pinctrl_init 1758]twi[2] not support sys_config format 
[16:53:38:906]
[16:53:38:906][TP] GT9xx init
[16:53:38:907]
[16:53:38:907][TP] start to probe![2, 0x5d]
[16:53:38:907]
[16:53:38:927]haas0X39 0X31 0X31 0X0 0X0
[16:53:38:927]
[16:53:38:927][TP] Found chip gt911
[16:53:38:927]
[16:53:38:927][TP] GT9xx Config version: 0x5C
[16:53:38:927]
[16:53:38:927][TP] GT9xx Sensor id: 0x03
[16:53:38:927]
[16:53:38:938]touchscreen init ok ^_^
[16:53:38:939]ui entry here!
[16:53:38:939]haasui build time: Dec 22 2021, 08:26:26
[16:53:40:006]@@@show homepage@@@
```

从上电启动到`@@@show homepage@@@`，消耗了1415ms，即上电到出UI Homepage的时间为1.4秒左右。

