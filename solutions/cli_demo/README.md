# 概述
cli_demo是一个串口命令行输入示例。串口命令行一般包括调试命令， 测试命令，获取系统信息命令，控制LOG信息打印等，其允许用户可以根据自己的需要增加命令，从而达到快速开发测试的命令。在本示例里主要集成了以下CLI命令，通过help命令可以List出所有支持的CLI命令。

# CDK
在CDK的首页，通过搜索关键字cli，可以找到cli_demo，然后创建工程。

CDK的使用可以参考YoCBook [《CDK开发快速上手》](https://yoc.docs.t-head.cn/yocbook/Chapter2-%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B%E6%8C%87%E5%BC%95/%E4%BD%BF%E7%94%A8CDK%E5%BC%80%E5%8F%91%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 章节

# 烧录
通过CDK编译完成之后会在out目录下生成yoc_rtos_8M.img文件，此文件即为最终的镜像文件。
使用全志烧写工具进行烧录。如何安装及烧写请参考[全志官方网站](https://d1.docs.aw-ol.com/study/study_4compile/#phoenixsuit)。

# 启动
烧录完成之后系统会自动启动，串口会有打印输出。

```cli
[ (cli-uart)#   0.180]<I>[app]<app_task>app start........

[   0.190]<I>[app]<app_task>Enter CLI shell 

[   0.190]<I>[app]<app_task>Try to type 'help' to get all commands description
```

输入"help" 可以看到所有的命令行命令，如下所示：
​
```cli
(cli-uart)# help
================ AliOS Things Command List ==============
help           : print this
reboot         : reboot system
ccs            : Console Cli Console Show
xfex           : Enter xfex mode to burn image
debug          : show debug info
sysver         : system version
time           : system time
msleep         : sleep miliseconds
p              : print memory
m              : modify memory
f              : run a function
devname        : print device name
err2cli        : set exec runto cli
tasklist       : list all thread info
dumpsys        : dump system info
taskbt         : list thread backtrace
taskbtn        : list thread backtrace by name
cpuusage       : show cpu usage
loglevel       : set sync log level
****************** Commands Num : 19 *******************
​
================ AliOS Things Command end ===============
```
​
tasklist 命令可以列出当前系统的所有任务的状态的状态、优先级、任务对战大小等状态信息。
​
```cli
(cli-uart)# tasklist
--------------------------------------------------------------------------------
Name               ID    State    Prio StackSize MinFreesize Runtime  Candidate
--------------------------------------------------------------------------------
dyn_mem_proc_task  1     PEND     6    4096      3408        113         N           
idle_task          2     RDY      61   4288      3760        3005585509   N           
DEFAULT-WORKQUEUE  3     PEND     20   4096      3464        70          N           
timer_task         4     PEND     5    3840      3144        1038        N           
cpu_stats          5     SLP      60   2640      2040        4911        N           
app_task           6     SLP      32   65536     63672       108622      N           
cli-uart           7     RDY      60   8192      6512        31895       Y           
event_svr          8     PEND     32   8192      7496        2745        N           
select             9     PEND     32   2048      1328        201         N           
--------------------------------------------------------------------------------
```

cpuusage命令可以列出当前任务的CPU占有率

```cli
(cli-uart)# cpuusage
use default param: period 1000ms
Start to statistics CPU utilization, period 1000 ms
​
(cli-uart)# -----------------------
CPU usage :  0.01%  
---------------------------
Name               %CPU
---------------------------
dyn_mem_proc_task    0.00
idle_task           99.99
DEFAULT-WORKQUEUE    0.00
timer_task           0.00
cpu_stats            0.00
app_task             0.00
cli-uart             0.00
event_svr            0.00
select               0.00
cpuusage             0.00
---------------------------
```

loglevel显示当前的LOG层级
```cli
(cli-uart)# loglevel
log level : current log level I
```

p命令用于打印指定的内存信息

```cli
(cli-uart)# p 0x00000000
0x0x000000: 43014281 4e014381 4f014e81 44014f81
0x0x000010: 41814481 41014201 45814501 a0294601
0x0x000020: 0493a009 a82d05c0 0fe000ef 13a000ef
0x0x000030: 02b7a801 829b0700 e3035e02 83020002
```

m命令用于修改指定的内存地址

```cli
(cli-uart)# m 0x1000 0x11223344
value on 0x0x1000 change from 0xfcf43c23 to 0x11223344.
```