# 概述
kv_demo是最小KV文件系统功能的操作例程。KV文件系统是基于Flash的一种Key-Value 数据存储系统，该系统采用极小的代码及内存开销（最小资源 rom：3K bytes，ram：100bytes），在小规模的Flash上实现数据的存储管理能力，支持断电保护、磨损均衡、坏块处理等功能。KV文件系统存储系统支持只读模式与读写模式共存，只读模式可以用于工厂生产数据，读写模式可用于运行时的数据存存储。

# CDK
在CDK的首页，通过搜索关键字kv，可以找到kv_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节

# 烧录
通过CDK编译完成之后会在out目录下生成yoc_rtos_8M.img文件，此文件即为最终的镜像文件。
使用全志烧写工具进行烧录。如何安装及烧写请参考[全志官方网站](https://d1.docs.aw-ol.com/study/study_4compile/#phoenixsuit)。

# 启动
烧录完成之后系统会自动启动，串口会有打印输出。

```cli
cpu clock is 1008000000Hz
[ (cli-uart)#   0.190]<I>[app]<app_task>app start........

[   0.190]<I>[app]<app_task>start kv testing...
[   0.200]<I>[app]<app_task>find 6 partitions
[   0.210]<E>[app]<app_task>kv init successfully

write one integer value into kv 
k:v = key_int:11223344
Delete one key from kv 
Delete the key(key_int)
k:v = key_int:0

write one string into kv 
k:v = key_str:hello kv
reset kv to erase all values
The key name(key_str) does not exist 
```

用户可以在cli输入以下命令进行KV数据的存取。

列出所有KV数据：
```cli
kv list
```

设置字符串：
```cli
kv set key1 string1
```

获取字符串：
```cli
kv get key1
```

设置整型：
```cli
kv setint key2 123
```

获取整型：
```cli
kv getint key2
```

删除某个key：
```cli
kv del key3
```
