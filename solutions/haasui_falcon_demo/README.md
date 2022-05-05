# 概述
haasui_falcon_demo是一个UI demo，集成了各种基础控件的简单示例。UI小程序相关的开发指南可以参考[官方指南](https://haas.iot.aliyun.com/haasui/quickstart)。`data/resources`目录下预置了小程序编译打包好的相关资源文件。

# CDK
在CDK的首页，通过搜索关键字haasui，可以找到haasui_falcon_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节

# 烧录
通过CDK编译完成之后会在out目录下生成yoc_rtos_8M.img文件，此文件即为最终的镜像文件。
使用全志烧写工具进行烧录。如何安装及烧写请参考[全志官方网站](https://d1.docs.aw-ol.com/study/study_4compile/#phoenixsuit)。

# 启动
烧录完成之后系统会自动启动，LCD屏幕上会显示UI，根据界面的提示可以进行触屏操作。

```cli
(cli-uart)# ###YoC###[Feb 17 2022,10:44:07]
cpu clock is 1008000000Hz
display init ok.
haasui entry here!
haasui build time: Feb 17 2022, 10:39:48
[TP] GT9xx init

[TP] start to probe![2, 0x5d]

0X39 0X31 0X31 0X0 0X0

[TP] Found chip gt911

[TP] GT9xx Config version: 0x5C

[TP] GT9xx Sensor id: 0x03
```
