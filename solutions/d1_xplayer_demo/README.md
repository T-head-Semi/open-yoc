# 概述

基于`d1`芯片的xplayer播放器框架测试demo

# CDK
在CDK的首页，通过搜索d1，可以找到d1_xplayer_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节

# 烧录
通过CDK编译完成之后会在out目录下生成yoc_rtos_8M.img文件，此文件即为最终的镜像文件。
使用全志烧写工具进行烧录。如何安装及烧写请参考[全志官方网站](https://d1.docs.aw-ol.com/study/study_4compile/#phoenixsuit)。

# 命令

命令通过串口cli输入。

## 查看内置音频
```
ls /mnt
```

## 停止播放

```
xplayer stop
```

**开始播放之前需要先停止上一次的播放**

## 播放内置内存中的歌曲

```
xplayer play inter
```
## 播放内置文件系统(littlefs)中的歌曲

```
xplayer play file:///mnt/pingfan_10s.mp3
```

## 播放网络歌曲

```
xplayer play https://yocbook.oss-cn-hangzhou.aliyuncs.com/av_repo/alibaba.mp3
```

**播放网络歌曲之前请先通过ifconfig ap your_ssid your_pwd命令配置网络**

