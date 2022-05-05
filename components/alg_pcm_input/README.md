## 概述

基于cxvision 的 音频输入节点，用于将从alsa采集的音频送给算法节点。
本节点也可以通过httpfile ，将从http 下载的文件送给算法节点，用于算法调试。

## Http 灌数据方法

- 下载http 文件服务程序
    https://www.rejetto.com/hfs/?f=dl
    可以下载这个hfs windows上的程序

- 启动hfs程序，并拖入已经录制好的录音文件（录音文件需要为 3 路 交织，采样率16000，顺序为 mic mic ref）
**注意：需要关闭电脑的防火墙**

- 将设备连上网络，并保证与电脑处于同一个局域网内

- 在设备端输入命令
```
pcminput http://[ip]/[record_pcm_file]
```