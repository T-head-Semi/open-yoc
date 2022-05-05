# 概述
led_demo是最简单LED显示例程。主要使用了chip_d1组件驱动组件里的GPIO，用户可以参考该组件里的CSI DRIVER的接口学习如何进行GPIO设备的初始化、模式设定、方向设定、管脚的拉高拉低等方法。

# CDK
在CDK的首页，通过搜索关键字led，可以找到led_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节

# 烧录
通过CDK编译完成之后会在out目录下生成yoc_rtos_8M.img文件，此文件即为最终的镜像文件。
使用全志烧写工具进行烧录。如何安装及烧写请参考[全志官方网站](https://d1.docs.aw-ol.com/study/study_4compile/#phoenixsuit)。

# 启动
烧录完成之后系统会自动启动，串口会有打印输出。开发板上的LED灯会不停的闪烁。

```cli
[ (cli-uart)#   0.180]<I>[app]<app_task>app start........

[   0.190]<I>[app]<app_task>Led blink triggers 3 seconds interval........
```
