# 概述

该示例为基于`d1`芯片的HaaS UI扩展服务dbus使用demo。其中`jsapp`目录即为小程序的相关代码。小程序相关的开发指南可以参考[官方指南](https://haas.iot.aliyun.com/haasui/quickstart)。`data/resources`目录下预置了小程序编译打包好的相关资源文件。
其提供了一个简单的开关功能，用户可以通过在界面上点击按钮来控制开关，当底层开关状态发生改变时，其同时会上报开关事件到GUI层。当点击时，开关消息会通过javascript脚本传递到jsapi_dbus层(语言绑定层)。绑定层进而将消息传递到开关DBUS服务层(C/C++)，最终在native层真正操作开关。当开关状态改变时，其消息流动方向反之。
DBUS扩展服务具体开发请参考《YoC HaaS小程序开发指南》文档中说明。

# CDK
在CDK的首页，通过搜索d1，可以找到d1_dbus_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节

# 烧录
通过CDK编译完成之后会在out目录下生成yoc_rtos_8M.img文件，此文件即为最终的镜像文件。
使用全志烧写工具进行烧录。如何安装及烧写请参考[全志官方网站](https://d1.docs.aw-ol.com/study/study_4compile/#phoenixsuit)。



