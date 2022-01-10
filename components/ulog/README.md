# 日志管理（ulog）

## 编译说明

Linux环境中适配syslog，使用CMake编译，编译配置文件CMakeLists.txt。

YoC环境编译配置package.yaml。

## 概述

在一个系统中日志管理是一个很重要的部分，因为当系统发布到线上后出了问题只能看系统日志了，这个时候系统日志起到了一个错误排查功能。

YOC平台提供了一个带错误等级、模块名称及时间标签的标准日志信息输出。当前，终端用户也可根据自身需要加以扩展或另外实现。

log.h头文件中主要定义了5中日志级别，分别如下：

```c
typedef enum {
    AOS_LL_NONE = 0,  /* disable log */
    AOS_LL_FATAL,     /* fatal log will output */
    AOS_LL_ERROR,     /* fatal + error log will output */
    AOS_LL_WARN,      /* fatal + warn + error log will output(default level) */
    AOS_LL_INFO,      /* info + warn + error log will output */
    AOS_LL_DEBUG,     /* debug + info + warn + error + fatal log will output */
    LOG_MAX
} aos_log_level_t;
```

| AOS_LL_NONE  | 不输出任何信息 |
| :------------ | :-------------- |
| AOS_LL_FATAL | 致命错误输出   |
| AOS_LL_ERROR | 错误输出       |
| AOS_LL_WARN  | 警告输出       |
| AOS_LL_INFO  | 信息输出       |
| AOS_LL_DEBUG | 调试信息输出   |

从AOS_LL_NONE到AOS_LL_DEBUG等级值增大，指定等级后，输出小于等于该等级的日志信息。

## 组件安装

```bash
yoc init
yoc install  ulog
```

## 接口列表

| 函数              | 说明                     |
| :---------------- | :----------------------- |
| aos_set_log_level | 运行时日志输出等级控制   |
| ulog_init         | 日志内部初始化           |
| ulog_man          | 日志管理                 |
| aos_get_ulog_list | 在文件系统中记录日志列表 |
| aos_log_hexdump   | 十六进制存储缓冲区       |

## 接口详细说明

### aos_set_log_level

`void aos_set_log_level(aos_log_level_t log_level);`

- 功能描述:
  - 运行时日志输出等级控制。

- 参数:
  - `log_level`: 日志等级。

- 返回值:
  - 无。

### ulog_init

`void ulog_init(void);`

- 功能描述:
  - 日志内部初始化。

- 参数:
  - 无。

- 返回值:
  - 无。

### ulog_man

`int ulog_man(const char *cmd_str);`

- 功能描述:
  - 日志管理。

- 参数:
  - `cmd_str`: 命令字符串。

- 返回值:
  - 0: 成功。
  - <:0 失败。

### aos_get_ulog_list

`int aos_get_ulog_list(char *buf, const unsigned short len);`

- 功能描述:
  - 在文件系统中记录日志列表。

- 参数:
  - `buf`: 缓冲区接收文件列表的上下文。
  - `len`: 缓冲器长度。

- 返回值:
  - 0: 成功。
  - <0: 失败。

### aos_log_hexdump

`int aos_log_hexdump(const char* tag, char *buffer, int len);`

- 功能描述:
  - 十六进制存储缓冲区。

- 参数:
  - `tag`: 标签。
  - `buffer`: 缓冲区地址。
  - `len`: 缓冲区长度。

- 返回值:
  - 0: 成功。
  - yoc_err: 失败。

## 示例

### 命令行服务接口

```c
#define TAG                    "player" // 在具体模块文件中定义模块名

/**
 * @brief  new a player obj
 * @param  [in] event_cb : callback of the player event
 * @return NULL on err
 */
player_t* player_new(player_event_t event_cb)
{
    player_t *player = NULL;

    LOGD(TAG, "%s, %d enter.", __FUNCTION__, __LINE__); // 使用LOGD级别打印
    player = (struct player_cb*)aos_zalloc(sizeof(struct player_cb));
    CHECK_RET_TAG_WITH_RET(player != NULL, NULL);

    player->event_cb    = event_cb;
    player->status      = PLAYER_STATUS_STOPED;
    player->rcv_timeout = RECV_TIMEOUT_MS_DEFAULT;
    aos_event_new(&player->evt, 0); 
    aos_mutex_new(&player->lock);
    _player_once_init(player);

    LOGD(TAG, "%s, %d leave. player = %p", __FUNCTION__, __LINE__, player);
    return player;
}
```

## 诊断错误码

无。

## 运行资源

无。

## 依赖资源

无。

## 组件参考

无。

