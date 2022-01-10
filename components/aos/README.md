## 概述
YoC 支持多种 RTOS 内核，已集成原生的 Rhino 内核，同时也支持 FreeRTOS 等多种开源RTOS内核。采用统一的 AOS 操作系统API 实现操作系统接口的定义，用户可以根据项目要求，选择不同的 RTOS 内核，YoC 默认并推荐使用原生的 Rhino 内核，目前 已支持平头哥全系统 CPU 架构。Rhino 内核具备更快的中断处理响应，更快的优先级抢占调度，它具有体积小、功耗低、实时性强和调试方便等特点。供了丰富多元的内核原语，包括缓冲队列，环形缓冲区、定时器、信号量、互斥量、先入先出队列、事件等。在最小的配置下（只运行一个 IDLE 任务），仅需要1.9KB 的 ROM，1KB 的 RAM 空间。
YoC 中定义了 RTOS 的基本操作，包括：任务管理、内存管理、信号量、消息队列、软件定时器等，即可以满足小系统的需要，也具备良好的可扩展性。

- 体积小
为大多数内核对象提供静态和动态分配。为小内存块设计的内存分配器既支持固定块又支持可变块，它还可以支持多个内存区域。大部分的内核特性，如work queue，和内存分配器，都可以通过修改k_config.h文件进行配置和裁剪。

- 功耗低
提供了CPU的tickless idle 模式来帮助系统节约电能和延长使用时间。
通常情况下，当CPU没有执行操作时，它将执行一个处理器指令（对于ARM来说的WFI，对于IA32位处理器来说的HLT），进入低功耗状态。此时，CPU寄存器的信息被保存，系统的 tick clock interrupts 会在每个tick时刻唤醒CPU。
为了比正常模式节省更多的电量，Rhino 为 CPU 提供了 tickless idle 模式。当操作系统检测到有一个固定时间（多个ticks或更长时间）的空闲后，它将进入tickless idle模式。系统做好中断配置，并把CPU置于C1模式，那时system tick clock中断不再被触发，system tick的计数也将停止。CPU会保持低耗电状态直到tickless idle时间结束。然后，当system tick timer interrupt再次被触发时，唤醒CPU从C1模式回到C0模式，为ticks计算好补偿时间并继续计数。

- 实时性
Rhino 提供了两个调度策略，基于优先级的抢占式调度和round-robin循环调度策略。对于两个调度策略而言，具有最高优先级的任务都是被优先处理的。
基于优先级的抢占式调度器会在遇到比当前运行的任务具有更高优先级任务时抢占CPU。这意味着，如果出现一个任务比当前任务具有更高优先级，内核将立即保存当前任务的context，并切换到高优先级的任务的context。因此，内核保证CPU总是优先处理优先级最高的任务。
round-robin调度器通过时间片来为各任务分配CPU资源。在一组具有相同优先级的任务中，每个任务都会被安排运行一个固定的时间长度，或者说时间片，之后CPU将处理下一个任务。所以，在一个任务阻塞之前，其他任务不能抢夺到处理器资源。当时间片失效时，系统将运行该优先级就绪队列中的最后一个任务。

- 方便调试
Rhino 支持stack溢出、内存泄漏、内存损坏的检测，有助于开发人员找出棘手问题的根源。结合 CDK 的集成开发环境（IDE），Rhino 的追踪功能将实现整个系统运行活动的可视化。

### 任务

在 RTOS 中，任务（Task）是程序独立运行的最小单位，由操作系统的任务调度系统实现任务的并发执行。每个任务都有独立的栈空间与任务控制块 （TCB）。RTOS 采用优先级抢占高度策略，每个任务都要备注以下基础信息：
- 任务栈：可采用动态分配与静态分配的方式，确保栈的大小能满足任务运行时不会造成栈溢出
- 任务主函数：是任务运行的主体程序，该函数的退出，表示任务的结束
- 任务优先级：高优先级的任务会抢占运行，合理的多任务同步设计可避免低优先级任务得不到运行机会
- 任务状态：是任务调度器调度决策的基础，合理的任务状态是多任务程序设计的基础

任务的基本操作：
- 创建：`aos_task_new`; `aos_task_new_ext`
- 放弃时间片：`void aos_task_yield();`
- 获取当前任务：`void aos_task_self();`
- 任务退出：`void aos_task_exit(int *code*);`

### 内存管理

RTOS 提供多种内存管理方案，满足不同应用的需要。动态内存管理是最常见的一种方式。常见的操作有：
分配内存：`void *aos_malloc(size_t size);` `void *aos_calloc(size_t size, int num);`
重新分配内存： `void *aos_realloc(void *mem, size_t size);`
内存释放：`void aos_free(void *mem);`

### 互斥锁
在编程中，引入了对象互斥锁的概念，来保证共享数据操作的完整性。每个对象都对应于一个可称为" 互斥锁" 的标记，这个标记用来保证在任一时刻，只能有一个线程访问该对象。互斥锁的基本四个操作：
- 创建 Create `int aos_mutex_new(aos_mutex_t *mutex);`
- 加锁 Lock `int aos_mutex_lock(aos_mutex_t *mutex, unsigned int timeout);`
- 解锁 Unlock `int aos_mutex_unlock(aos_mutex_t *mutex);`
- 销毁 Destroy `void aos_mutex_free(aos_mutex_t *mutex);`

### 信号量

信号量(Semaphore)，有时被称为信号灯，是在多线程环境下使用的一种设施，是可以用来保证两个或多个关键代码段不被并发调用。在进入一个关键代码段之前，线程必须获取一个信号量；一旦该关键代码段完成了，那么该线程必须释放信号量。其它想进入该关键代码段的线程必须等待直到第一个线程释放信号量。为了完成这个过程，需要创建一个信号量VI，然后将Acquire Semaphore VI以及Release Semaphore VI分别放置在每个关键代码段的首末端。确认这些信号量VI引用的是初始创建的信号量。信号量的基本四个操作：
- 创建 Create `int aos_sem_new(aos_sem_t *sem, int count);`
- 等待信号 Wait  `int aos_sem_wait(aos_sem_t *sem, unsigned int timeout);` 
- 释放信号 Signal `void aos_sem_signal(aos_sem_t *sem);`
- 销毁 Destroy `void aos_sem_free(aos_sem_t *sem);` 

### 消息队列

消息队列提供了一种从一个任务向另一个任务发送数据的方法。  每个数据都被认为含有一个类型，接收的任务可以独立地接收含有不同类型的数据结构。消息队列的特点：
1. 消息队列是消息的链表，具有特定的格式，存放在内存中并由消息队列标识符标识
2. 消息队列允许一个或多个任务向它写入与读取消息
3. 消息队列可以实现消息的随机查询,消息不一定要以先进先出的次序读取,也可以按消息的类型读取.比FIFO更有优势
消息队列的基本操作：
- 创建 Create：`int aos_queue_new(aos_queue_t *queue, void *buf, size_t size, int max_msg);`
- 发送消息 Send： `int aos_queue_send(aos_queue_t *queue, void *msg, unsigned int size);`
- 接收消息 Recv： `int aos_queue_recv(aos_queue_t *queue, unsigned int ms, void *msg, unsigned int *size);`
- 销毁 Destroy： `void aos_queue_free(aos_queue_t *queue);`

### 事件标志组
事件标志组是一种实现任务间通信的机制，可用于实现任务间的同步。事件标志组采用位来表示一个事件，通常采用支持32位的变量，其中每一位表示一种事件类型（0表示该事件类型未发生、1表示该事件类型已经发生）。事件标志组具备以下特点：
- 可以是任意一个事件发生时唤醒任务进行事件处理，也可以是几个事件都发生后才唤醒任务进行事件处理
- 多次向任务发送同一事件类型，等效于只发送一次
- 允许多个任务对同一事件进行读写操作
- 事件仅用于任务间的同步，不提供数据传输功能
事件标志组支持的基本操作：
创建 Create：`int aos_event_new(aos_event_t *event, unsigned int flags);`
事件设置 Set： `int aos_event_set(aos_event_t *event, unsigned int flags, unsigned char *opt*);`
事件获取 Get： `int aos_event_get(aos_event_t *event, `
`unsigned int flags, unsigned char opt,unsigned int *actl_flags, unsigned int timeout);`
销毁 Destroy： `void aos_event_free(aos_event_t *event);`

### 定时器
定时器是由操作系统提供的一类系统接口，它构建在硬件定时器基础之上，使系统能够提供不受数目限制的定时器服务。
定时器支持的基本操作：
创建 Create：`int aos_timer_new(aos_timer_t *timer, void (*fn)(void *, void *), void *arg, int ms, int repeat);`
启动 Start：`int aos_timer_start(aos_timer_t *timer);`
停止 Stop：`int aos_timer_stop(aos_timer_t *timer);`
销毁 Destory：`void aos_timer_free(aos_timer_t *timer);`
改变 Change：`int aos_timer_change(aos_timer_t *timer, int ms);`

### 工作队列
在一个操作系统中，如果我们需要进行一项工作处理，往往需要创建一个任务来加入内核的调度队列。一个任务对应一个处理函数，如果要进行不同的事务处理，则需要创建多个不同的任务。任务作为cpu调度的基础单元，任务数量越大，则调度成本越高。工作队列（workqueue）机制简化了基本的任务创建和处理机制，一个workqueue对应一个实体task任务处理，workqueue下面可以挂接多个work实体，每一个work实体都能对应不同的处理接口。即用户只需要创建一个workqueue，则可以完成多个挂接不同处理函数的工作队列。 其次，当某些实时性要求较高的任务中，需要进行较繁重钩子处理时，可以将其处理函数挂接在workqueue中，其执行过程将位于workqueue的上下文，而不会占用原有任务的处理资源。 另外，workqueue还提供了work的延时处理机制，用户可以选择立即执行或是延时处理。 由上可见，我们在需要创建大量实时性要求不高的任务时，可以使用workqueue来统一调度；或者将任务中实时性要求不高的部分处理延后到workqueue中处理。如果需要设置延后处理，则需要使用work机制。另外该机制不支持周期work的处理。
工作队列支持的基本操作：
创建工作队列: `int aos_workqueue_create(aos_workqueue_t *workqueue, int pri, int stack_size)`
删除工作队列: `void aos_workqueue_del(aos_workqueue_t *workqueue);`
工作初始化: `int aos_work_init(aos_work_t *work, void (*fn)(void *), void *arg, int dly);`
工作运行：`int aos_work_run(aos_workqueue_t *workqueue, aos_work_t *work); int aos_work_sched(aos_work_t *work);`
工作删除：`void aos_work_destroy(aos_work_t *work);`
工作取消：`int aos_work_cancel(aos_work_t *work);`

### 其他
提供系统启动/停止、内核挂起/恢复等杂项服务。

## 组件安装
```bash
yoc init
yoc install aos
```

## 配置

无。

## 接口列表
### 任务
| 函数 | 说明 |
| :--- | :--- |
| aos_task_new | 动态创建任务 |
| aos_task_new_ext | 动态创建任务（扩展） |
| aos_task_show_info | 显示任务信息 |
| aos_task_yield | 放弃时间片 |
| aos_task_exit | 任务退出 |
| aos_task_wdt_attach | 任务附着看门狗任务 |
| aos_task_wdt_detach | 任务去附着看门狗任务 |
| aos_task_wdt_feed | 喂看门狗 |
| aos_task_self | 获取本任务句柄 |
| aos_task_name | 获取本任务名称 |
| aos_task_get_name | 获取指定任务名称 |
| aos_task_key_create | 返回任务私有数据区域的空闲块索引 |
| aos_task_key_delete | 删除任务私有数据区域的空闲块索引 |
| aos_task_setspecific | 设置当前任务私有数据区域的某索引空闲块内容 |
| aos_task_getspecific | 获取当前任务私有数据区域的某索引数据块内容 |

### 内存管理
| 函数 | 说明 |
| :--- | :--- |
| aos_realloc | 重新分配内存 |
| aos_realloc_check | 重新分配内存并检查 |
| aos_malloc | 分配内存 |
| aos_malloc_check | 分配内存并检查  |
| aos_zalloc | 分配内存并初始化 |
| aos_zalloc_check | 分配内存并检查及初始化 |
| aos_calloc | 分配内存 |
| aos_calloc_check | 分配内存并检查 |
| aos_alloc_trace | 在分配的内存区加跟踪信息 |
| aos_free | 内存释放 |
| aos_freep | 内存释放及指针清空 |
| aos_get_mminfo | 获取系统内存使用情况 |
| aos_mm_dump | 打印内存 |

### 互斥锁
| 函数 | 说明 |
| :--- | :--- |
| aos_mutex_new | 创建互斥信号量 |
| aos_mutex_free | 删除互斥信号量 |
| aos_mutex_lock | 请求互斥信号量 |
| aos_mutex_unlock | 释放互斥信号量 |
| aos_mutex_is_valid | 判断互斥信号量是否有效 |
| aos_mutex_is_locked | 判断互斥信号量是否被请求 |

### 信号量
| 函数 | 说明 |
| :--- | :--- |
| aos_sem_new | 创建信号量 |
| aos_sem_free | 删除信号量 |
| aos_sem_wait | 获取信号量 |
| aos_sem_signal | 释放信号量 |
| aos_sem_is_valid | 判断信号量是否有效 |
| aos_sem_signal_all |释放信号量  |

### 事件标志组
| 函数 | 说明 |
| :--- | :--- |
| aos_event_new | 创建事件 |
| aos_event_free | 删除事件 |
| aos_event_get | 获取事件标志位 |
| aos_event_set | 设置事件标志 |
| aos_event_is_valid | 判断一个事件是否有效 |

### 消息队列
| 函数 | 说明 |
| :--- | :--- |
| aos_queue_new | 创建队列 |
| aos_queue_free | 删除队列 |
| aos_queue_send | 向队列发送数据 |
| aos_queue_recv | 从队列内收取数据 |
| aos_queue_is_valid | 判断队列是否有效 |
| aos_queue_buf_ptr | 获取队列的缓冲区起点 |
| aos_queue_get_count | 获取消息队列里面消息个数 |

### 定时器
| 函数 | 说明 |
| :--- | :--- |
| aos_timer_new | 创建定时器 |
| aos_timer_new_ext | 创建定时器 |
| aos_timer_free | 删除软件定时器 |
| aos_timer_start | 启动定时器 |
| aos_timer_stop | 停止定时器 |
| aos_timer_change | 改变软件定时器的周期  |
| aos_timer_change_once | 改变软件定时器的周期 |
| aos_timer_is_valid | 判断定时器是否有效 |

### 工作队列
| 函数 | 说明 |
| :--- | :--- |
| aos_workqueue_create | 创建工作队列 |
| aos_workqueue_del | 删除工作队列 |
| aos_work_init | 初始化work |
| aos_work_destroy | 删除work |
| aos_work_run | 运行work |
| aos_work_sched | 运行work |
| aos_work_cancel | 取消work |

### 其他
| 函数 | 说明 |
| :--- | :--- |
| aos_irq_context | 是否在中断上下文中 |
| aos_reboot | 重启系统 |
| aos_get_hz | 获取每秒TICK数量 |
| aos_version_get | 获取操作系统版本 |
| aos_now | 获取系统运行至今的时长（单位：纳秒） |
| aos_now_ms | 获取系统运行至今的时长（单位：毫秒） |
| aos_msleep | 任务挂起 |
| aos_init | 系统初始化 |
| aos_start | 系统开始运行 |
| aos_kernel_intrpt_enter | 系统进入中断 |
| aos_kernel_intrpt_exit | 系统退出中断 |
| aos_kernel_sched_suspend | 内核调度挂起 |
| aos_kernel_sched_resume | 内核调度恢复 |
| aos_kernel_tick2ms | TICK数转毫秒数 |
| aos_kernel_ms2tick | 毫秒数转TICK数 |
| aos_kernel_suspend | 内核挂起 |
| aos_kernel_resume | 内核恢复 |
| aos_set_except_callback | 设置系统异常回调函数 |
| aos_set_except_default | 设置默认的系统异常回调函数 |

## 接口详细说明

### aos_task_new
`int aos_task_new(const char *name, void (*fn)(void *), void *arg, int stack_size);`

- 功能描述:
   - 动态创建一个任务，任务句柄不返回，创建完后自动运行 采用默认优先级AOS_DEFAULT_APP_PRI（32），受宏RHINO_CONFIG_KOBJ_DYN_ALLOC开关控制。

- 参数:
   - `name`: 任务名。
   - `fn`: 任务处理函数。
   - `arg`: 任务处理函数参数。
   - `stack_size`: 任务栈大小（单位：字节）。

- 返回值:
   - 0: 成功。
   - 小于0：失败。

### aos_task_new_ext
`int aos_task_new_ext(aos_task_t *task, const char *name, void (*fn)(void *), void *arg,
                     int stack_size, int prio);`   
		     
- 功能描述:
   - 动态创建一个任务，传入任务句柄，并指定优先级，创建完后自动运行 受宏RHINO_CONFIG_KOBJ_DYN_ALLOC开关控制。
#### aos_task_t
| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| hdl | void * | 句柄 |

- 参数:
   - `task`: 任务句柄。
   - `name`: 任务名。
   - `fn`: 任务处理函数。
   - `arg`: 任务处理函数参数。
   - `stack_size`: 任务栈大小（单位：字节）。
   - `prio`: 任务运行优先级（范围：0~ RHINO_CONFIG_PRI_MAX-2; RHINO_CONFIG_PRI_MAX-1为idle任务）。
   
- 返回值:
   - 0: 成功。
   - 小于0：失败。

### aos_task_show_info
`void aos_task_show_info(void);`

- 功能描述:
   - 显示任务信息。

- 参数:
   - 无。
   
- 返回值:
   - 无。

### aos_task_yield
`void aos_task_yield();`

- 功能描述:
   - 任务放弃时间片。

- 参数:
   - 无。
   
- 返回值:
   - 无。

### aos_task_exit
`void aos_task_exit(int code);`

- 功能描述:
   - 任务自动退出。受宏RHINO_CONFIG_KOBJ_DYN_ALLOC开关控制。

- 参数:
   - `code`: 暂时无用。
   
- 返回值:
   - 无。
   
### aos_task_wdt_attach
`void aos_task_wdt_attach(void (*will)(void *), void *args);`

- 功能描述:
   - 任务附着看门狗任务。附着之后，本任务受看门狗任务监控。如果本任务运行时间过长，看门狗任务则先运行will函数，然后重启。

- 参数:
   - `will`: 重启前的回调函数。
   - `args`: 回调函数参数。
   
- 返回值:
   - 无。

### aos_task_wdt_detach
`void aos_task_wdt_detach();`

- 功能描述:
   - 任务去附着看门狗任务。不附着之后，本任务不再受看门狗任务监控。

- 参数:
   - 无。
   
- 返回值:
   - 无。
   
### aos_task_wdt_feed
`void aos_task_wdt_feed(int time);`

- 功能描述:
   - 喂看门狗。喂狗之后，本任务还可以运行时长time。

- 参数:
   - `time`: 时长，单位：ms。
   
- 返回值:
   - 无。
   
### aos_task_self
`aos_task_t aos_task_self();`
- 功能描述:
   - 获取本任务句柄。

- 参数:
   - 无。
   
- 返回值:
   - 任务句柄。

### aos_task_name
`const char *aos_task_name(void);`

- 功能描述:
   - 获取本任务名称。

- 参数:
   - 无。
   
- 返回值:
   - 任务名称。

### aos_task_get_name
`const char *aos_task_get_name(aos_task_t *task);`

- 功能描述:
   - 获取指定任务名称。

- 参数:
   - `task`: 任务句柄。
   
- 返回值:
   - 任务名称。

### aos_task_key_create
`int aos_task_key_create(aos_task_key_t *key);`

- 功能描述:
   - 返回任务私有数据区域的空闲块索引。
   aos_task_key_t定义：
   `typedef unsigned int aos_task_key_t;`

- 参数:
   - `key`: 任务私有数据区域的空闲块索引；范围0~RHINO_CONFIG_TASK_INFO_NUM-1； 每索引块可存放一个void*地址。
   
- 返回值:
   - 0: 成功。
   - 小于0: 失败。

### aos_task_key_delete
`void aos_task_key_delete(aos_task_key_t key);`

- 功能描述:
   - 删除任务私有数据区域的空闲块索引。

- 参数:
   - `key`: 任务私有数据区域的空闲块索引；范围0~RHINO_CONFIG_TASK_INFO_NUM-1。
   
- 返回值:
   - 无。
   
### aos_task_setspecific
`int aos_task_setspecific(aos_task_key_t key, void *vp);`

- 功能描述:
   - 设置当前任务私有数据区域的某索引空闲块内容。受宏RHINO_CONFIG_TASK_INFO开关控制。

- 参数:
   - `key`: 任务私有数据区域的空闲块索引；范围0~RHINO_CONFIG_TASK_INFO_NUM-1。
   - `vp`: 需要存放进入任务私有空闲区域的地址指针。   
   
- 返回值:
   - 0: 成功。
   - 小于0: 失败。

### aos_task_getspecific
`void *aos_task_getspecific(aos_task_key_t key);`

- 功能描述:
   - 获取当前任务私有数据区域的某索引数据块内容。受宏RHINO_CONFIG_TASK_INFO开关控制。

- 参数:
   - `key`: 任务私有数据区域的空闲块索引； 范围0~RHINO_CONFIG_TASK_INFO_NUM-1。 
   
- 返回值:
   - 私有区域某索引内容。

### aos_realloc
`void *aos_realloc(void *mem, size_t size);`

- 功能描述:
   - 重新调整所分配的内存块的大小。
   如果mem指针为NULL，则新分配size大小的内存区域；如果mem指针非NULL,先判断原指针mem是否有足够的连续空间，如果有，扩大mem指向的地址，并且将mem指针返回，如果空间不够，按照size指定的大小分配空间（size的大小可以比原mem内存大，也可以比原mem内存小）。在分配成功的前提下，将原有数据从mem指向的内存区域拷贝到新分配的内存区域，而后释放原来mem所指内存区域，同时返回新分配的内存区域的首地址，若分配失败，则不释放原来的内存区域，并返回NULL。 对于拷贝数据的大小，如果新分配的内存区域size比原来mem指向内存区域大，则只拷贝原来mem指向内存区域长度字节数据到新分配的内存区域；否则，只拷贝size字节的数据到新分配的内存区域。

- 参数:
   - `mem`: 原内存区域起始地址指针。
   - `size`: 本次操作期望新分配的内存区域大小。   

- 返回值:
   - 非空: 新分配的内存区域起始地址指针。
   - 空（NULL）: 分配失败。
   
### aos_realloc_check
`void *aos_realloc_check(void *mem, size_t size);`

- 功能描述:
   - 重新调整所分配的内存块的大小，如果失败，系统做异常处理。
   如果mem指针为NULL，则新分配size大小的内存区域；如果mem指针非NULL,先判断原指针mem是否有足够的连续空间，如果有，扩大mem指向的地址，并且将mem指针返回，如果空间不够，按照size指定的大小分配空间（size的大小可以比原mem内存大，也可以比原mem内存小）。在分配成功的前提下，将原有数据从mem指向的内存区域拷贝到新分配的内存区域，而后释放原来mem所指内存区域，同时返回新分配的内存区域的首地址，若分配失败，则不释放原来的内存区域，并返回NULL。 对于拷贝数据的大小，如果新分配的内存区域size比原来mem指向内存区域大，则只拷贝原来mem指向内存区域长度字节数据到新分配的内存区域；否则，只拷贝size字节的数据到新分配的内存区域。

- 参数:
   - `mem`: 原内存区域起始地址指针。
   - `size`: 本次操作期望新分配的内存区域大小。   

- 返回值:
   - 非空: 新分配的内存区域起始地址指针。
   - 空（NULL）: 分配失败。

### aos_malloc
`void *aos_malloc(size_t size);`

- 功能描述:
   - 从系统堆中分配size字节内存区域。

- 参数:
   - `size`: 要分配内存区域的字节数。   

- 返回值:
   - 非空: 分配的内存区域起始地址指针。
   - 空（NULL）: 分配失败。

### aos_malloc_check
`void *aos_malloc_check(size_t size);`

- 功能描述:
   - 从系统堆中分配size字节内存区域。如果分配失败，系统做异常处理。

- 参数:
   - `size`: 要分配内存区域的字节数。   

- 返回值:
   - 非空: 分配的内存区域起始地址指针。
   - 空（NULL）: 分配失败。

### aos_zalloc
`void *aos_zalloc(size_t size);`

- 功能描述:
   - 从系统堆中分配size字节内存区域，并将内存初始化填充0。

- 参数:
   - `size`: 要分配内存区域的字节数。   

- 返回值:
   - 非空: 分配的内存区域起始地址指针。
   - 空（NULL）: 分配失败。

### aos_zalloc_check
`void *aos_zalloc_check(size_t size);`

- 功能描述:
   - 从系统堆中分配size字节内存区域，并将内存初始化填充0。如果分配失败，系统做异常处理。

- 参数:
   - `size`: 要分配内存区域的字节数。   

- 返回值:
   - 非空: 分配的内存区域起始地址指针。
   - 空（NULL）: 分配失败。
   
### aos_calloc
`void *aos_calloc(size_t size, size_t num);`

- 功能描述:
   - 从系统堆中分配nitems * size字节连续内存区域，并将内存区域初始化填充0。

- 参数:
   - `size`: 每个内存区域单元的字节数。  
   - `num`: 要分配内存区域单元个数。

- 返回值:
   - 非空: 分配的内存区域起始地址指针。
   - 空（NULL）: 分配失败。

### aos_calloc_check
`void *aos_calloc_check(size_t size, size_t num);`

- 功能描述:
   - 从系统堆中分配nitems * size字节连续内存区域，并将内存区域初始化填充0。如果分配失败，系统做异常处理。

- 参数:
   - `size`: 每个内存区域单元的字节数。  
   - `num`: 要分配内存区域单元个数。

- 返回值:
   - 非空: 分配的内存区域起始地址指针。
   - 空（NULL）: 分配失败。

### aos_alloc_trace
`void aos_alloc_trace(void *addr, size_t allocator);`

- 功能描述:
   - 在分配的内存区加分配者信息以便问题跟踪。

- 参数:
   - `addr`: 分配的内存区起始地址。  
   - `allocator`: 分配者信息。

- 返回值:
   - 无。

### aos_free
`void aos_free(void *mem);`

- 功能描述:
   - 将从系统堆中分配的内存区域释放。

- 参数:
   - `mem`: 要释放的内存区域起始地址指针。  

- 返回值:
   - 无。

### aos_freep
`void aos_freep(char **ptr);`

- 功能描述:
   - 将从系统堆中分配的内存区域释放，并将指针置空。

- 参数:
   - `ptr`: 要释放的内存区域起始地址指针所在地址。  

- 返回值:
   - 无。

### aos_get_mminfo
`int aos_get_mminfo(int32_t *total, int32_t *used, int32_t *mfree, int32_t *peak);`

- 功能描述:
   - 获取系统内存使用情况。

- 参数:
   - `total`: 所有能用的内存的大小。
   - `used`: 已分配内存的大小。 
   - `mfree`: 空闲内存的大小。 
   - `peak`: 内存使用的峰值。 

- 返回值:
   - 0: 成功。

### aos_mm_dump
`int aos_mm_dump(void);`
- 功能描述:
   - 打印内存。

- 参数:
   - 无。 

- 返回值:
   - 0: 成功。

### aos_mutex_new
`int aos_mutex_new(aos_mutex_t *mutex);`

- 功能描述:
   - 动态创建互斥信号量。
   aos_mutex_t定义：
#### aos_mutex_t定义：
| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| hdl | void * | 句柄 |

- 参数:
   - `mutex`: 互斥信号量结构体指针，需要用户定义一个aos_mutex_t结构体，并把该结构体指针传入。 
   
- 返回值:
   - 0: 成功。
   - 小于0: 失败。

### aos_mutex_free
`void aos_mutex_free(aos_mutex_t *mutex);`

- 功能描述:
   - 删除互斥信号量。

- 参数:
   - `mutex`: 互斥信号量结构体指针。 
   
- 返回值:
   - 无。

### aos_mutex_lock
`int aos_mutex_lock(aos_mutex_t *mutex, unsigned int timeout);`

- 功能描述:
   - 请求互斥信号量。

- 参数:
   - `mutex`: 互斥信号量结构体指针。 
   - `timeout`: 等待超时时间，单位ms。0表示非阻塞请求，立即返回；AOS_WAIT_FOREVER表示永久等待；其他值则等待timeout ms，超时后返回。可通过返回值判断是否获得互斥信号量。   
   
- 返回值:
   - 0: 成功。
   - 小于0: 失败。

### aos_mutex_unlock
`int aos_mutex_unlock(aos_mutex_t *mutex);`

- 功能描述:
   - 释放互斥信号量。

- 参数:
   - `mutex`: 互斥信号量结构体指针。 

- 返回值:
   - 0: 成功。
   - 小于0: 失败。

### aos_mutex_is_valid
`int aos_mutex_is_valid(aos_mutex_t *mutex);`

- 功能描述:
   - 判断是否为有效的互斥信号量。

- 参数:
   - `mutex`: 互斥信号量结构体指针。 

- 返回值:
   - 0: mutex为无效互斥信号量。
   - 1: mutex为有效的互斥信号量。
   
### aos_mutex_is_locked
`int aos_mutex_is_locked(aos_mutex_t *mutex);`

- 功能描述:
   - 判断互斥信号量是否被请求。

- 参数:
   - `mutex`: 互斥信号量结构体指针。 

- 返回值:
   - 0: 没有被请求。
   - 1: 被请求。

### aos_sem_new
`int aos_sem_new(aos_sem_t *sem, int count);`

- 功能描述:
   - 动态创建信号量。
#### aos_sem_t
| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| hdl | void * | 句柄 |

- 参数:
   - `sem`: 信号量结构体指针；需要用户定义一个sem结构体。 
   - `count`: 此sem的初始计数值，此值大于0，才能获取到信号量，获取一次，count计数减1。   

- 返回值:
   - 0: 成功。
   - 非0: 失败。

### aos_sem_free
`void aos_sem_free(aos_sem_t *sem);`
`int aos_sem_new(aos_sem_t *sem, int count);`

- 功能描述:
   - 删除信号量。

- 参数:
   - `sem`: 信号量结构体指针。   

- 返回值:
   - 无。

### aos_sem_wait
`int aos_sem_wait(aos_sem_t *sem, unsigned int timeout);`

- 功能描述:
   - 获取一个信号量。获取不到信号量，当前任务阻塞。

- 参数:
   - `sem`: 信号量结构体指针。   
   - `timeout`: 超时时间。传入0表示不超时，立即返回；AOS_WAIT_FOREVER表示永久等待； 其他数值表示超时时间，单位ms。   

- 返回值:
   - 0: 成功。
   - 非0: 失败（超时返回RHINO_BLK_TIMEOUT）。

### aos_sem_signal
`void aos_sem_signal(aos_sem_t *sem);`

- 功能描述:
   - 释放一个信号量，并唤醒一个高优先级阻塞任务。

- 参数:
   - `sem`: 信号量结构体指针。   
  
- 返回值:
   - 无。

### aos_sem_is_valid
`int aos_sem_is_valid(aos_sem_t *sem);`

- 功能描述:
   - 判断一个信号量是否有效。

- 参数:
   - `sem`: 信号量结构体指针。   
  
- 返回值:
   - 1: 有效。
   - 0: 无效。   

### aos_sem_signal_all
`void aos_sem_signal_all(aos_sem_t *sem);`

- 功能描述:
   - 释放一个信号量，并唤醒所有阻塞任务。

- 参数:
   - `sem`: 信号量结构体指针。   
  
- 返回值:
   - 无。
   
### aos_queue_new
`int aos_queue_new(aos_queue_t *queue, void *buf, size_t size, int max_msg);`

- 功能描述:
   - 创建一个队列。
#### aos_queue_t
| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| hdl | void * | 句柄 | 

- 参数:
   - `queue`: 队列描述结构体指针；需要用户定义一个queue结构体；例：‘aos_queue_t g_queue’; 传入&g_queue。 
   - `buf`: 此queue队列的缓冲区起点；例：char buf[1000];传入buf。
   - `size`: 此queue队列的缓冲区大小。 
   - `max_msg`: 一次存入缓冲区的最大数据单元。
   
- 返回值:
   - 0: 成功。
   - 非0: 失败。

### aos_queue_free
`void aos_queue_free(aos_queue_t *queue);`

- 功能描述:
   - 删除一个队列，并释放阻塞在其中的任务。

- 参数:
   - `queue`: 队列描述结构体指针。 
   
- 返回值:
   - 无。

### aos_queue_send
`int aos_queue_send(aos_queue_t *queue, void *msg, unsigned int size);`

- 功能描述:
   - 向队列发送数据。

- 参数:
   - `queue`: 队列描述结构体指针。 
   - `msg`: 发送数据起始内存。
   - `size`: 发送数据大小。 
   
- 返回值:
   - 0: 成功。
   - 非0: 失败。

### aos_queue_recv
`int aos_queue_recv(aos_queue_t *queue, unsigned int ms, void *msg, unsigned int *size);`

- 功能描述:
   - 从队列内收取数据。

- 参数:
   - `queue`: 队列描述结构体指针。 
   - `ms`: 传入0表示不超时，立即返回；AOS_WAIT_FOREVER表示永久等待；其他数值表示超时时间，单位ms。   
   - `msg`: 返回获取到的数据的内存指针。
   - `size`: 返回获取到的数据大小。 
   
- 返回值:
   - 0: 成功。
   - 非0: 失败。

### aos_queue_is_valid
`int aos_queue_is_valid(aos_queue_t *queue);`

- 功能描述:
   - 判断队列是否有效。

- 参数:
   - `queue`: 队列描述结构体指针。 
   
- 返回值:
   - 0: 无效。
   - 非0: 有效。

### aos_queue_buf_ptr
`void *aos_queue_buf_ptr(aos_queue_t *queue);`

- 功能描述:
   - 获取一个队列的缓冲区起点。

- 参数:
   - `queue`: 队列描述结构体指针。 
   
- 返回值:
   - 非空: 队列queue的缓冲区起点。
   - 空（NULL）: 获取失败。

### aos_queue_get_count
`int aos_queue_get_count(aos_queue_t *queue);`

- 功能描述:
   - 获取消息队列里面消息个数。

- 参数:
   - `queue`: 队列描述结构体指针。 
   
- 返回值:
   - 大于等于0: 消息个数。
   - 小于0: 获取失败。

### aos_event_new
`int aos_event_new(aos_event_t *event, unsigned int flags);`

- 功能描述:
   - 动态创建一个事件，并且把事件标志初始化成flag的值。
#### aos_event_t
| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| hdl | void * | 句柄 | 

- 参数:
   - `event`: 事件句柄。   
   - `flags`: 事件标志初始值。    
  
- 返回值:
   - 0: 成功。
   - 小于0: 失败。

### aos_event_free
`void aos_event_free(aos_event_t *event);`

- 功能描述:
   - 删除事件。

- 参数:
   - `event`: 事件句柄。    
  
- 返回值:
   - 无。

### aos_event_get
`int aos_event_get(aos_event_t *event, unsigned int flags, unsigned char opt,
                  unsigned int *actl_flags, unsigned int timeout);`
		  
- 功能描述:
   - 获取事件标志位。如果事件标志位被设置了，则立即返回； 如果timeout参数设置为RHINO_NO_WAIT，则也会立即返回 如果事件标志位没有被设置并且timeout参数设置为RHINO_WAIT_FOREVER，调用的task会被挂起直到事件标志位被置起。

- 参数:
   - `event`: 事件句柄。    
   - `flags`: 需要获取的事件标志。    
   - ` opt`: 操作类型，比如AND,OR,AND_CLEAR,OR_CLEAR。    
   - `actl_flags`: 原始的事件标志。  
   - `timeout`: 超时时间。  
   
- 返回值:
   - 0: 成功。
   - 小于0: 失败。

### aos_event_set
`int aos_event_set(aos_event_t *event, unsigned int flags, unsigned char opt);`

- 功能描述:
   - 设置事件标志，并唤醒被相应事件阻塞的任务。

- 参数:
   - `event`: 事件句柄。    
   - `flags`: 需要设置的事件标志。    
   - ` opt`: 操作类型，比如AND,OR。     
   
- 返回值:
   - 0: 成功。
   - 小于0: 失败。

### aos_event_is_valid
`int aos_event_is_valid(aos_event_t *event);`

- 功能描述:
   - 判断一个事件是否有效。

- 参数:
   - `event`: 事件句柄。        
   
- 返回值:
   - 0: 无效。
   - 1: 有效。

### aos_timer_new
`int aos_timer_new(aos_timer_t *timer, void (*fn)(void *, void *),
                  void *arg, int ms, int repeat);`
- 功能描述:
   - 创建一个定时器，并自动运行。
#### aos_timer_t
| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| hdl | void * | 句柄 |  

- 参数:
   - `timer`: timer描述结构体指针；需要用户定义一个timer结构体。 
   - `fn`: 定时器处理函数。
   - `arg`: 定时器处理函数参数。
   - `ms`: 定时器超时时间（单位ms），即间隔多少时间执行fn。   
   - `repeat`: 周期或单次（1：周期，0：单次）。   
   
- 返回值:
   - 0: 成功。
   - 非0: 失败。
   
### aos_timer_new_ext
`int aos_timer_new_ext(aos_timer_t *timer, void (*fn)(void *, void *),
                      void *arg, int ms, int repeat, unsigned char auto_run);`
		      
- 功能描述:
   - 创建一个定时器，根据auto_run参数选择是否自动运行。

- 参数:
   - `timer`: timer描述结构体指针；需要用户定义一个timer结构体。 
   - `fn`: 定时器处理函数。
   - `arg`: 定时器处理函数参数。
   - `ms`: 定时器超时时间（单位ms），即间隔多少时间执行fn。   
   - `repeat`: 周期或单次（1：周期，0：单次）。
   - `auto_run`: 1表示自动运行，0表示不自动运行，需要手动调用aos_timer_start才能启动。
   
- 返回值:
   - 0: 成功。
   - 非0: 失败。

### aos_timer_free
`void aos_timer_free(aos_timer_t *timer);`

- 功能描述:
   - 删除软件定时器。

- 参数:
   - `timer`: timer描述结构体指针。 

- 返回值:
   - 无。

### aos_timer_start
`int aos_timer_start(aos_timer_t *timer);`

- 功能描述:
   - 启动一个定时器。

- 参数:
   - `timer`: timer描述结构体指针。 

- 返回值:
   - 0: 成功。
   - 非0: 失败。

### aos_timer_stop
`int aos_timer_stop(aos_timer_t *timer);`

- 功能描述:
   - 停止一个定时器。

- 参数:
   - `timer`: timer描述结构体指针。 

- 返回值:
   - 0: 成功。
   - 非0: 失败。

### aos_timer_change
`int aos_timer_change(aos_timer_t *timer, int ms);`

- 功能描述:
   - 改变软件定时器的周期。

- 参数:
   - `timer`: timer描述结构体指针。 
   - `ms`: 新的定时器周期，单位ms。   

- 返回值:
   - 0: 成功。
   - 非0: 失败。

### aos_timer_change_once
`int aos_timer_change_once(aos_timer_t *timer, int ms);`

- 功能描述:
   - 改变软件定时器的周期，仅改变一次。

- 参数:
   - `timer`: timer描述结构体指针。 
   - `ms`: 新的定时器周期，单位ms。   

- 返回值:
   - 0: 成功。
   - 非0: 失败。

### aos_timer_is_valid
`int aos_timer_is_valid(aos_timer_t *timer);`

- 功能描述:
   - 判断定时器是否有效。

- 参数:
   - `timer`: timer描述结构体指针。  

- 返回值:
   - 1: 有效。
   - 0: 无效。

### aos_workqueue_create
`int aos_workqueue_create(aos_workqueue_t *workqueue, int pri, int stack_size);`

- 功能描述:
   - 创建一个工作队列，内部会创建一个任务关联workqueue。
#### aos_workqueue_t
| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| hdl | void * | 句柄 |
| stk | void * | 任务栈 |

- 参数:
   - `workqueue`: 工作队列描述结构体指针；需要用户定义一个workqueue结构体。  
   - `pri`: 工作队列优先级，实际是关联任务优先级。
   - `stack_size`: 任务栈大小（单位：字节）。

- 返回值:
   - 0: 成功。
   - 非0: 失败。

### aos_workqueue_del
`void aos_workqueue_del(aos_workqueue_t *workqueue);`

- 功能描述:
   - 删除一个工作队列。

- 参数:
   - `workqueue`: 工作队列描述结构体指针。  

- 返回值:
   - 无。

### aos_work_init
`int aos_work_init(aos_work_t *work, void (*fn)(void *), void *arg, int dly);`

- 功能描述:
   - 初始化一个work。

- 参数:
   - `work`: work工作描述结构体指针；需要用户定义一个work结构体。  
   - `fn`: work回调处理函数。
   - `arg`: work回调处理函数参数。
   - `dly`: 延迟处理时间，单位ms，0表示不延迟。   

- 返回值:
   - 0: 成功。
   - 非0: 失败。
   
### aos_work_destroy
`void aos_work_destroy(aos_work_t *work);`

- 功能描述:
   - 删除一个work。
   aos_work_t定义：
#### aos_work_t
| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| hdl | void * | 句柄 |       

- 参数:
   - `work`: work工作描述结构体指针。   

- 返回值:
   - 无。

### aos_work_run
`int aos_work_run(aos_workqueue_t *workqueue, aos_work_t *work);`

- 功能描述:
   - 运行一个work，使其在指定的工作队列内调度执行。

- 参数:
   - `workqueue`: 工作队列描述结构体指针。  
   - `work`: 需要执行的工作描述结构体指针。

- 返回值:
   - 0: 成功。
   - 非0: 失败。
   
### aos_work_sched
`int aos_work_sched(aos_work_t *work);`

- 功能描述:
   - 运行一个work，使其在默认工作队列g_workqueue_default内调度执行。

- 参数:
   - `work`: 需要执行的工作描述结构体指针。

- 返回值:
   - 0: 成功。
   - 非0: 失败。

### aos_work_cancel
`int aos_work_cancel(aos_work_t *work);`

- 功能描述:
   - 取消一个work，使其从所在的工作队列中删除。

- 参数:
   - `work`: 需要取消的工作描述结构体指针。

- 返回值:
   - 0: 成功。
   - 非0: 失败。
   
### aos_irq_context
`int32_t aos_irq_context(void);`

- 功能描述:
   - 是否在中断上下文中。

- 参数:
   - 无。

- 返回值:
   - 1: 在中断上下文中。
   - 0: 不在中断上下文中。

### aos_reboot
`void aos_reboot(void);`

- 功能描述:
   - 重启系统。

- 参数:
   - 无。

- 返回值:
   - 无。

### aos_get_hz
`int aos_get_hz(void);`

- 功能描述:
   - 获取每秒TICK数量。

- 参数:
   - 无。

- 返回值:
   - 每秒TICK数量。

### aos_version_get
`const char *aos_version_get(void);`

- 功能描述:
   - 获取操作系统版本。

- 参数:
   - 无。

- 返回值:
   - 操作系统版本。
   
### aos_now
`long long aos_now(void);`

- 功能描述:
   - 获取系统运行至今的时长（单位：纳秒）。

- 参数:
   - 无。  

- 返回值:
   - 系统运行至今的时长，单位：ns。

### aos_now_ms
`long long aos_now_ms(void);`

- 功能描述:
   - 获取系统运行至今的时长（单位：毫秒）。

- 参数:
   - 无。  

- 返回值:
   - 系统运行至今的时长，单位：ms。

### aos_msleep
`void aos_msleep(int ms);`

- 功能描述:
   - 任务挂起若干毫秒。

- 参数:
   - `ms`: 毫秒数。  

- 返回值:
   - 无。

### aos_init
`void aos_init(void);`

- 功能描述:
   - 系统初始化。

- 参数:
   - 无。  

- 返回值:
   - 无。

### aos_start
`void aos_start(void);`

- 功能描述:
   - 系统开始运行。

- 参数:
   - 无。  

- 返回值:
   - 无。

### aos_kernel_intrpt_enter
`k_status_t aos_kernel_intrpt_enter(void);`

- 功能描述:
   - 系统进入中断。

- 参数:
   - 无。  

- 返回值:
   - 0: 成功。
   - 小于0: 失败。   

### aos_kernel_intrpt_exit
`k_status_t aos_kernel_intrpt_exit(void);`

- 功能描述:
   - 系统退出中断状态。

- 参数:
   - 无。  

- 返回值:
   - 0: 成功。
   - 小于0: 失败。

### aos_kernel_sched_suspend
`void aos_kernel_sched_suspend(void);`

- 功能描述:
   - 内核调度挂起。

- 参数:
   - 无。 

- 返回值:
   - 无。
   
### aos_kernel_sched_resume
`void aos_kernel_sched_resume(void);`

- 功能描述:
   - 内核调度恢复。

- 参数:
   - 无。 

- 返回值:
   - 无。
   
### aos_kernel_tick2ms
`uint64_t aos_kernel_tick2ms(uint32_t ticks);`

- 功能描述:
   - TICK数转毫秒数。

- 参数:
   - `ticks`:tick数。 

- 返回值:
   - 毫秒数。
   
### aos_kernel_ms2tick
`uint64_t aos_kernel_ms2tick(uint32_t ms);`

- 功能描述:
   - 毫秒数转TICK数。

- 参数:
   - `ms`:毫秒数。 

- 返回值:
   - TICK数。
   
### aos_kernel_suspend
`int32_t aos_kernel_suspend(void);`

- 功能描述:
   - 内核挂起。

- 参数:
   - 无。 

- 返回值:
   - 大于等于0: 内核挂起的TICK数。
   - 小于0：获取失败

### aos_kernel_resume
`void aos_kernel_resume(int32_t ticks);`

- 功能描述:
   - 内核恢复。

- 参数:
   - `ticks`: TICK数。 

- 返回值:
   - 无。
   
### aos_set_except_callback
`void aos_set_except_callback(except_process_t except);`

- 功能描述:
   - 设置系统异常回调函数，系统检测到异常情况时调用此函数。
   except_process_t定义：
   `typedef void (*except_process_t)(int errno, const char *file, int line, const char *func_name, void *caller);`
   
- 参数:
   - `except`: 异常回调函数。 

- 返回值:
   - 无。
   
### aos_set_except_default
`void aos_set_except_default();`

- 功能描述:
   - 设置默认的系统异常回调函数，系统检测到异常情况时调用此函数。   

- 参数:
   - 无。 

- 返回值:
   - 无。   
   
## 示例

### 任务
创建一个任务，获取任务名字，创建任务私有数据区域，设置私有区域内容、获取私有区域内容，删除私有数据区域，任务退出

```c
/* 创建任务test_task，任务栈大小为1024字节 */
aos_task_new("test_task", test_task, NULL, 1024)

static void test_task(void *paras)
{
    int ret = -1;
    int var = 0;
    aos_task_key_t task_key;
    void *task_value = NULL;

     /* 创建任务私有数据区域*/
    ret = aos_task_key_create(&task_key);
    /* 打印任务名和任务私有数据区域索引值*/
    printf("%s task key 0x%x. \r\n", aos_task_name(), task_key);

    aos_msleep(10);    // 任务休眠10ms
    var = 0x5a5a;
     /* 设置当前任务私有数据区域的某索引空闲块内容 */
    ret = aos_task_setspecific(task_key, &var);

    /* 获取当前任务私有数据区域的某索引数据块内容 */
    task_value = aos_task_getspecific(task_key);
    printf("%s task key 0x%x. \r\n", aos_task_name(), *(int*)task_value);

    /* 删除任务私有数据区域的空闲块索引 */
    aos_task_key_delete(task_key);

    /* 任务退出 */
    aos_task_exit(0);
}
```

### 内存

#### 内存申请和释放
```c
char *ptr = NULL;
unsigned int size = 64;

ptr = (char*)aos_malloc(size);
if (NULL == ptr) {
    printf("aos_malloc failed\r\n");
    ...
}
...
aos_free(ptr);
```

### 互斥锁

```c

#include <aos/aos.h>

aos_mutex_t test_mutex;

void task1_entry(void)
{
    int ret = -1;
    while (1) {
        /* 判断mutex是否有效 */
        ret = aos_mutex_is_valid(&test_mutex);
        if (ret != 1) {
            return;
        }
        /* 永久请求互斥信号量 */
        ret = aos_mutex_lock(&test_mutex, AOS_WAIT_FOREVER);
        if (ret != 0) {
            continue;
        }
        /* 访问临界资源 */
        printf("task1 entry access critical zone\n");

        /* 释放互斥信号量 */
        aos_mutex_unlock(&test_mutex);
    }
}

void task2_entry(void)
{
    int ret = -1;
    while (1) {
        /* 判断mutex是否有效 */
        ret = aos_mutex_is_valid(&test_mutex);
        if (ret != 1) {
            return;
        }
        /* 请求互斥信号量，timeout为1000ms */
        ret = aos_mutex_lock(&test_mutex, 1000);
        if (ret != 0) {
            continue;
        }
        /*访问临界资源*/
        printf("task2 entry access critical zone\n");

        /* 释放互斥信号量 */
        aos_mutex_unlock(&test_mutex);
    }
}

int test_mutex_start(void)
{
    int ret = -1;
    /* 创建一个mutex */
    ret = aos_mutex_new(&test_mutex);
    if (ret != 0) {
        return;
    }
    /* 创建task1 & task2 */
    aos_task_new("task1", task1_entry, NULL, 512);
    aos_task_new("task2", task2_entry, NULL, 512);
}

```

### 信号量

当前任务创建一个信号量和子任务，并等待子任务释放信号量
```c
aos_sem_t test_sem;

static void task1_entry(void *arg)
{
    aos_msleep(3000);    // 任务休眠3000ms

    printf("task1 send semaphore\n");

    /*释放信号量*/
    aos_sem_signal(&test_sem);
}

static void task2_entry(void *arg)
{
    printf("task2 wait semaphore\n");

    /*获取信号量，由于初始值为0，这里获取不到信号量，当前任务进入睡眠并发生切换
      参数 -1 表示AOS_WAIT_FOREVER，直到task1任务释放信号量*/
    aos_sem_wait(&test_sem, -1);

    /*获取到信号量，当前任务继续执行下去*/
    printf("sem test successfully!\n");

    /*删除信号量*/
    aos_sem_free(&test_sem);
}

void test_sem_start(void)
{
    int ret = -1;

    aos_msleep(1000);    // 任务休眠1000ms

    /*当前任务：创建信号量，信号量初始count为0*/
    ret = aos_sem_new(&test_sem, 0);
    if (ret != 0) {
        printf("sem create failed\n");
        return;
    }

    /*判断信号量是否可用*/
    ret = aos_sem_is_valid(&test_sem);
    if (ret == 0) {
        printf("sem is invalid\n");
    }

    aos_task_new("task1", task1_entry, NULL, 512);
    aos_task_new("task2", task2_entry, NULL, 512);
}
```
#### 使用注意事项
1）在中断中禁止信号量获取检测 信号量的获取接口在中断上下文调用很容易发生死锁问题。当被打断的上下文和打断的中断上下文要获取同一个信号量时，会发生互相等待的情况。有些内核将这种判断处理交由上层软件进行判断和使用，本内核会在take信号量检测，如果是中断上下文，则直接返回失败。
2） 占用信号量非等待、永远等待、延时使用区别 上层应用在获取信号量时，需要按照实际的需求，来安排信号量获取策略。krhino_sem_take传入延时ticks为0，获取不到信号量会立即报失败；ticks为全F时，会永远在此等待，直到获取到信号量，可能会造成该任务无法继续运行；其他值标识最大延迟的时间上限，达到上限时，及时未获取到信号量，tick中断处理会将任务唤醒，并返回状态为超时。
3） 信号量优先级反转问题 优先级反转出现在高、中、低三个优先级任务同时访问使用信号量互斥资源时，可能存在的问题。当高优先级的任务需要的信号量被低优先级任务占用时，cpu资源会调度给低优先级任务。此时如果低优先级需要获取的另一个信号量被中优先级的pend任务所占用，那么低优先级的任务则需要等待中优先级的任务事件到来，并释放信号量，则就出现了高、中优先级的任务并不是等待一个信号量，但是中优先级任务先运行的现象。 该优先级反转的缺陷，在互斥量mutex得以解决，其途径在于动态提高C任务运行优先级来避免任务优先级的反转问题，详细内容见下一章节。
4 ) 信号量整体受位于k_config.h 中的宏 RHINO_CONFIG_KOBJ_DYN_ALLOC和 RHINO_CONFIG_SEM开关控制。

### 消息队列

```c

| QUEUE_BUF_LEN 100

static aos_queue_t test_queue;
static char queue_buf[QUEUE_BUF_LEN];

static void task1_entry(void *arg)
{
    char *msg_send = "Hello, Queue!";

    aos_msleep(3000);    // 任务休眠3000ms

    printf("task1 send msg\n");

    /*发送消息*/
    aos_queue_send(&test_queue, msg_send, strlen(msg_send));
}

static void task2_entry(void *arg)
{
    char msg_recv[16] = {0};
    unsigned int size_recv = 16;

    printf("task2 wait msg\n");

    memset(msg_recv, 0, size_recv);
    aos_queue_recv(&test_queue, 100000, msg_recv, &size_recv);

    printf("task2 get msg: ", size_recv);
    for (int i = 0; i < size_recv; i++) {
        printf("%c", msg_recv[i]);
    }
    printf("\n");

    /*获取到消息，当前任务继续执行下去*/
    printf("queue test successfully!\n");

    /*删除消息队列*/
    aos_queue_free(&test_queue);
}

void test_queue_start(void)
{
    int ret = -1;

    aos_msleep(1000);    // 任务休眠1000ms

    /*当前任务：创建消息队列，消息队列最大为长度为100，单条消息最大为20*/
    ret = aos_queue_new(&test_queue, queue_buf, QUEUE_BUF_LEN, 20);
    if (ret != 0) {
        printf("queue create failed\n");
        return;
    }

    aos_task_new("task1", task1_entry, NULL, 512);
    aos_task_new("task2", task2_entry, NULL, 512);
}
```

### 事件

```c
| EVENT_FLAG_3 0x000000F0

aos_event_t test_event;

static void task1_entry(void *arg)
{
    aos_msleep(3000);    // 任务休眠3000ms

    printf("task1 send event\n");

    /*设置事件标志(0x000000F0), 或操作*/

    aos_event_set(&test_event, EVENT_FLAG_3, AOS_EVENT_OR);
}

static void task2_entry(void *arg)
{
    uint32_t actl_flags;

    printf("task2 wait event\n");

    /* 获取事件标志(0x000000F0)，或操作 */
    aos_event_get(&test_event, EVENT_FLAG_3, AOS_EVENT_OR, &actl_flags, 0);

    /*
     * try to get flag EVENT_FLAG_3(0x000000F0) with OR operation should wait here,
     * task1 will set the flags with 0x000000F0, and then task2 will continue
     */
    aos_event_get(&test_event, EVENT_FLAG_3, AOS_EVENT_OR, &actl_flags, AOS_WAIT_FOREVER);

    printf("event test successfully!\n");

    /*删除事件*/
    aos_event_free(&test_event);
}

void test_event_start(void)
{
    int ret = -1;
    uint32_t flags = 0;

    aos_msleep(1000);    // 任务休眠1000ms

    /*当前任务：创建事件*/
    ret = aos_event_new(&test_event, flags);
    if (ret != 0) {
        printf("event create failed\n");
        return;
    }

    /*判断事件是否可用*/
    ret = aos_event_is_valid(&test_event);
    if (ret == 0) {
        printf("event is invalid\n");
    }

    aos_task_new("task1", task1_entry, NULL, 512);
    aos_task_new("task2", task2_entry, NULL, 512);
}
```

### 定时器

创建自动运行的周期执行定时器

```c

aos_timer_t test_timer;

static void timer_handler(void *arg1, void* arg2)
{
    printf("timer handler\r\n");
}

void test_timer_periodic(void)
{
    int ret = -1;

    printf("test timer periodic\n");

    /*创建定时周期为200ms的周期执行的定时器，并自动运行*/
    ret = aos_timer_new(&test_timer, timer_handler, NULL, 200, 1);
    if (ret != 0) {
        printf("timer create failed\r\n");
    }

    aos_msleep(1000);

    /*停止定时器*/
    aos_timer_stop(&test_timer);

    /*释放定时器*/
    aos_timer_free(&test_timer);
}
```

创建不自动运行的周期执行定时器，并在使用中改变定时周期。

```c
aos_timer_t test_timer;

static void timer_handler(void *arg1, void* arg2)
{
    printf("timer handler\r\n");
}
void test_timer_change(void)
{
    int ret = -1;

    printf("test timer change time\n");

    /*创建定时周期为200ms的周期执行的定时器，不自动运行*/
    ret = aos_timer_new_ext(&test_timer, timer_handler, NULL, 200, 0, 0);
    if (ret != 0) {
        printf("timer create failed\r\n");
    }

    /*需要手动启动定时器*/
    aos_timer_start(&test_timer);

    aos_msleep(1000);

    /*停止定时器*/
    aos_timer_stop(&test_timer);

    /*改变定时周期为300ms, 注意：需要在定时器未启动状态是才能修改*/
    aos_timer_change(&test_timer, 300);

    /*启动定时器*/
    aos_timer_start(&test_timer);

    /*停止定时器*/
    aos_timer_stop(&test_timer);

    /*释放定时器*/
    aos_timer_free(&test_timer);
}
```

### 工作队列
创建一个工作队列，初始化一个work，把这个work加入到这个工作队列中，主线程等待，work执行，work执行完毕销毁

```c
static aos_workqueue_t workqueue;
static aos_work_t      work;
static aos_sem_t       sync_sem;

static void workqueue_custom(void *arg)
{
    aos_msleep(1000);
    printf("workqueue custom");
    aos_sem_signal(&sync_sem);
}

static void test_workqueue()
{
    int ret = 0;
    aos_sem_new(&sync_sem, 0);

    /* 创建一个工作队列workqueue，内部会创建一个任务关联该workqueue */
    aos_workqueue_create(&workqueue, 10, 1024);
    /* 初始化一个work，暂不执行，等待run */
    aos_work_init(&work, workqueue_custom, NULL, 100);

    /* 运行work，使其在某worqueue内调度执行 */
    aos_work_run(&workqueue, &work);
    /* sem等待，workqueue_custom得到执行 */
    aos_sem_wait(&sync_sem, AOS_WAIT_FOREVER);

    /* 释放sem */
    aos_sem_free(&sync_sem);
    /* 销毁work */
    aos_work_destroy(&work);
}

/* aos_work_cancel(aos_work_t *work) */
/* 删除work前，要确保work没有正在或将要被worqueue执行，否则会返回错误 */
/* 删除workqueue需要确保没有待处理或正在处理的wok，否则会返回错误 */
```

## 诊断错误码
| 错误码 | 错误码说明 |
| :--- | :--- |
|  EPERM | Operation not permitted |  
|  ENOENT | No such file or directory |  
|  ESRCH | No such process |  
|  EINTR | Interrupted system call |  
|  EIO | I/O error |  
|  ENXIO | No such device or address |  
|  E2BIG | Arg list too long |  
|  ENOEXEC | Exec format error |  
|  EBADF | Bad file number |  
|  ECHILD | No child processes |  
|  EAGAIN | Try again |  
|  ENOMEM | Out of memory |  
|  EACCES | Permission denied |  
|  EFAULT | Bad address |  
|  ENOTBLK | Block device required |  
|  EBUSY | Device or resource busy |  
|  EEXIST | File exists |  
|  EXDEV | Cross-device link |  
|  ENODEV | No such device |  
|  ENOTDIR | Not a directory |  
|  EISDIR | Is a directory |  
|  EINVAL | Invalid argument |  
|  ENFILE | File table overflow |  
|  EMFILE | Too many open files |  
|  ENOTTY | Not a typewriter |  
|  ETXTBSY | Text file busy |  
|  EFBIG | File too large |  
|  ENOSPC | No space left on device |  
|  ESPIPE | Illegal seek |  
|  EROFS |  Read-only file system |  
|  EMLINK | Too many links |  
|  EPIPE | Broken pipe |  
|  EDOM | Math argument out of domain of func |  
|  ERANGE | Math result not representable |  
|  EDEADLK | Resource deadlock would occur |  
|  ENAMETOOLONG | File name too long |  
|  ENOLCK | No record locks available |  
|  ENOSYS | Function not implemented |  
|  ENOTEMPTY | Directory not empty |  
|  ELOOP | Too many symbolic links encountered |  
|  EWOULDBLOCK | Operation would block |  
|  ENOMSG | No message of desired type |  
|  EIDRM | Identifier removed |  
|  ECHRNG | Channel number out of range |  
|  EL2NSYNC | Level 2 not synchronized |  
|  EL3HLT | Level 3 halted |  
|  EL3RST | Level 3 reset |  
|  ELNRNG | Link number out of range |  
|  EUNATCH |Protocol driver not attached |  
|  ENOCSI | No CSI structure available |  
|  EL2HLT | Level 2 halted |  
|  EBADE | Invalid exchange |  
|  EBADR | Invalid request descriptor |  
|  EXFULL | Exchange full |  
|  ENOANO | No anode |  
|  EBADRQC | Invalid request code |  
|  EBADSLT | Invalid slot |  
|  EDEADLOCK | Resource deadlock would occur |
|  EBFONT | Bad font file format |  
|  ENOSTR | Device not a stream |  
|  ENODATA | No data available |  
|  ETIME | Timer expired |  
|  ENOSR | Out of streams resources |  
|  ENONET | Machine is not on the network |  
|  ENOPKG | Package not installed |  
|  EREMOTE |  Object is remote |  
|  ENOLINK | Link has been severed |  
|  EADV | Advertise error |  
|  ESRMNT | Srmount error |  
|  ECOMM | Communication error on send |  
|  EPROTO | Protocol error |  
|  EMULTIHOP | Multihop attempted |  
|  EDOTDOT | RFS specific error |  
|  EBADMSG | Not a data message |  
|  EOVERFLOW | Value too large for defined data type |  
|  ENOTUNIQ | Name not unique on network |  
|  EBADFD | File descriptor in bad state |  
|  EREMCHG | Remote address changed |  
|  ELIBACC | Can not access a needed shared library |  
|  ELIBBAD | Accessing a corrupted shared library |  
|  ELIBSCN | .lib section in a.out corrupted |  
|  ELIBMAX | Attempting to link in too many shared libraries |  
|  ELIBEXEC | Cannot exec a shared library directly |  
|  EILSEQ | Illegal byte sequence |  
|  ERESTART | Interrupted system call should be restarted |  
|  ESTRPIPE | Streams pipe error |  
|  EUSERS | Too many users |  
|  ENOTSOCK | Socket operation on non-socket |  
|  EDESTADDRREQ | Destination address required |  
|  EMSGSIZE | Message too long |  
|  EPROTOTYPE | Protocol wrong type for socket |  
|  ENOPROTOOPT | Protocol not available |  
|  EPROTONOSUPPORT | Protocol not supported |  
|  ESOCKTNOSUPPORT | Socket type not supported |  
|  EOPNOTSUPP | Operation not supported on transport endpoint |  
|  EPFNOSUPPORT | Protocol family not supported |  
|  EAFNOSUPPORT | Address family not supported by protocol |  
|  EADDRINUSE | Address already in use |  
|  EADDRNOTAVAIL | Cannot assign requested address |  
|  ENETDOWN | Network is down |  
|  ENETUNREACH | Network is unreachable |  
|  ENETRESET | Network dropped connection because of reset |  
|  ECONNABORTED | Software caused connection abort |  
|  ECONNRESET | Connection reset by peer |  
|  ENOBUFS | No buffer space available |  
|  EISCONN | Transport endpoint is already connected |  
|  ENOTCONN | Transport endpoint is not connected |  
|  ESHUTDOWN | Cannot send after transport endpoint shutdown |  
|  ETOOMANYREFS | Too many references: cannot splice |  
|  ETIMEDOUT | Connection timed out |  
|  ECONNREFUSED | Connection refused |  
|  EHOSTDOWN | Host is down |  
|  EHOSTUNREACH | No route to host |  
|  EALREADY | Operation already in progress |  
|  EINPROGRESS | Operation now in progress |  
|  ESTALE | Stale NFS file handle |  
|  EUCLEAN | Structure needs cleaning |  
|  ENOTNAM | Not a XENIX named type file |  
|  ENAVAIL | No XENIX semaphores available |  
|  EISNAM | Is a named type file |  
|  EREMOTEIO| Remote I/O error |  
|  EDQUOT | Quota exceeded |  
|  ENOMEDIUM | No medium found |  
|  EMEDIUMTYPE| Wrong medium type |  

## 运行资源
无。

## 依赖资源
  - kv
  - ulog
  - uservice
  - partition

## 组件参考
无。


