# 概述
hellojs是最简小程序例程。小程序相关代码在[GITEE](https://gitee.com/yocop/haasui_demo.git)，下载的时候选择`hellojs`分支。小程序相关的开发指南可以参考[官方指南](https://haas.iot.aliyun.com/haasui/quickstart)。`data/resources`目录下预置了小程序编译打包好的相关资源文件。

# CDK
在CDK的首页，通过搜索关键字hellojs，可以找到hellojs，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节

# 烧录
通过CDK编译完成之后会在out目录下生成yoc_rtos_8M.img文件，此文件即为最终的镜像文件。
使用全志烧写工具进行烧录。如何安装及烧写请参考[全志官方网站](https://d1.docs.aw-ol.com/study/study_4compile/#phoenixsuit)。

# 启动
烧录完成之后系统会自动启动，LCD屏幕上会显示`Hello,YoC!`等相关的字符，根据界面的提示可以进行简单的触屏操作。
