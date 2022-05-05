# 麦克风服务（mic）

## 概述

专为IoT场景打造的麦克风服务，为用户提供本地语音识别各类事件及各类语音控制命令，从而解决传统硬件设备智能化的痛点，并提供麦克风适配层，方便用户适配不同的语音框架。

## 组件安装

```bash
yoc init
yoc install mic
```

## 接口列表

| 函数                    | 说明               |
| :---------------------- | :----------------- |
| aui_mic_evt_t           | 麦克风事件回调     |
| aui_mic_init            | 麦克风初始化       |
| aui_mic_deinit          | 关闭指定麦克风     |
| aui_mic_start           | 启动麦克风服务     |
| aui_mic_stop            | 停止麦克风服务     |
| aui_mic_set_param       | 设置mic相关参数    |
| aui_mic_set_wake_enable | 设置麦克风唤醒使能 |
| aui_mic_control         | 麦克风控制命令     |
| aui_mic_get_state       | 获取麦克风状态     |

## 接口详细说明

### aui_mic_evt_t

`int aui_mic_start(utask_t *task, aui_mic_evt_t evt_cb);`

- 功能描述: 
  - 麦克风事件回调。

- 参数: 
  - `task`: utask_t 指针。
  - `evt_cb`: 用户事件回掉。

- 返回值: 
  - 0: 成功。
  - -1: 失败。

### aui_mic_init

`int aui_mic_init(utask_t *task, aui_mic_evt_t evt_cb);`

- 功能描述: 
  - 语音模块初始化。

- 参数: 
  - `task`: utask_t 指针。
  - `evt_cb`: 用户事件回掉。

- 返回值: 
  - 0: 成功。
  - -1: 失败。

### aui_mic_deinit

`int aui_mic_deinit(void);`

- 功能描述: 
  - 去初始化语音模块。

- 参数: 
  - 无。

- 返回值: 
  - 0: 成功。
  - -1: 失败。

### aui_mic_start

`int aui_mic_start(utask_t *task, aui_mic_evt_t evt_cb);`

- 功能描述: 
  - 启动语音服务。

- 参数: 
  - `task`: utask_t 指针。
  - `evt_cb`: 用户事件回掉。

- 返回值: 
  - 0: 成功。
  - -1: 失败。

### aui_mic_stop

`int aui_mic_stop(void);`

- 功能描述: 
  - 停止语音服务。

- 参数: 
  - 无。

- 返回值: 
  - 0: 成功。
  - -1: 失败。

### aui_mic_set_param

`int aui_mic_set_param(void *param);`

- 功能描述: 
  - 设置语音算法参数。语音离线算法使用

- 参数: 
  - `param`: 算法参数。

- 返回值: 
  - 0: 成功。
  - -1: 失败。

### aui_mic_set_wake_enable

`int aui_mic_set_wake_enable(int en);`

- 功能描述: 
  - 设置麦克风唤醒使能。

- 参数: 
  - `en`: 1: 使能 0: 关闭

- 返回值: 
  - 0: 成功。
  - -1: 失败。

### aui_mic_control

`int aui_mic_control(mic_ctrl_cmd_t cmd);`

- 功能描述: 
  - 语音数据控制命令。

- 参数: 
  - `cmd`: 控制命令
    - MIC_CTRL_START_PCM 启动音频数据传输
    - MIC_CTRL_STOP_PCM 停止音频数据传输
    - MIC_CTRL_START_SESSION 强制进入对话模式

- 返回值: 
  - 0: 成功。
  - -1: 失败。

### aui_mic_get_state

`int aui_mic_get_state(mic_state_t *state);`

- 功能描述: 
  - 获取语音算法状态。

- 参数: 
  - `state`: 麦克风状态。

- 返回值: 
  - 0: 成功。
  - -1: 失败。

## 示例

```c
/* mic event*/
void mic_evt_cb(int source, mic_event_id_t evt_id, void *data, int size)
{

    switch (evt_id) {
        case MIC_EVENT_PCM_DATA: 
            LOGD(TAG, "mic pcm evt");
            break;
        case MIC_EVENT_SESSION_START: 
            LOGD(TAG, "mic start evt");
            break;
        case MIC_EVENT_SESSION_STOP: 
            LOGD(TAG, "mic stop evt");
            break;
        default: ;
    }
}

/* mic init */
int app_mic_init(utask_t *task)
{
    int ret;

    voice_mic_register();

    ret = aui_mic_start(task, mic_evt_cb);

    return ret;
}
```

## 诊断错误码

无。

## 运行资源

无。

## 依赖资源

- csi
- aos
- record

## 组件参考

无。

