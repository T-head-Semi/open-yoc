## 概述

基于cxvision 的 语音平台录音节点，用于将从alsa采集的数据录制下来

## websocket 录制方法

在pc端启动websocket server
```
tydb.exe record web 0.0.0.0:8090 ./download
```
在设备端连接pc websocket 服务进行录制

```
record start ws://192.168.50.205:8090 data.pcm
```

结束录制
```
record stop
```

完成录制后会在 ./download 生成 xxxxxx_mic_data.pcm 文件