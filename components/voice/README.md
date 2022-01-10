## 概述
专为IoT场景打造的智能语音识别框架，为用户提供数据采集接口，LPM接口，语音识别算法接口等接口，方便后期接入云端语音识别等框架，从而解决传统硬件设备智能化的痛点。

## 组件安装

```bash
yoc init
yoc install voice
```

## 配置
无。

## 接口列表
| 函数 | 说明 |
| :--- | :--- |
| voice_init | 语音模块初始化 |
| voice_deinit | 语音模块注销 |
| voice_config | 语音模块配置 |
| voice_add_mic | 添加麦克风 |
| voice_add_ref | 添加REF |
| voice_start | 语音开始 |
| voice_stop | 语音停止 |


## 接口详细说明

### voice_init 
`voice_t *voice_init(voice_evt_t cb, void *priv);`

- 功能描述:
   - 语音初始化。需要先保证dbus连接正常
   voice_evt_t定义：
`typedef void (*voice_evt_t)(void *priv, voice_evt_id_t evt_id, void *data, int len);`

#### voice_t
| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| mic_param | voice_pcm_param_t * | 麦克风参数 |
| ref_param | voice_pcm_param_t * | REF参数 |
| param | voice_param_t | 语音参数 |
| kws | csi_kws_t | kws结构 |
| priv | void * | 事件回调函数参数 |
| conn | DBusConnection | dbus连接 |
| watch_rfd | int | 监听dbus的fd |
| watch | void * | 监听dbus |
| ops | voice_ops_t * | 操作函数 |


#### voice_pcm_param_t
| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| pcm_name | char * | pcm设备名称 |
| rate | unsigned int | 采样率 |
| sample_bits | int | 采样位数 |
| access | int | 是否为交错模式，0：非交错 1：交错 |
| channles | int | 通道总数 |
| channles_sum | int | 通道总数 |
| period_bytes | int | pcm周期数据量（用户不必配置） |


#### voice_param_t
| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| cts_ms | int | buffer长度 |
| ipc_mode | int | ipc模式 |
| ai_param | void | 算法侧参数配置 |
| ai_param_len | int | 参数配置长度 |
| shm_addr | void * | 共享内存，暂未使用 |



- 参数:
   - `cb`: voice事件。
   - `priv`: 用户私有数据。   
   
- 返回值:
   - 非空: 语音句柄。
   - 空（NULL）: 失败。

### voice_deinit
`void voice_deinit(voice_t *v);`

- 功能描述:
   - 语音注销。

- 参数:
   - `v`: 语音句柄。
   
- 返回值:
   - 无。

### voice_config
`int voice_config(voice_t *v, voice_param_t *p);`

- 功能描述:
   - 语音配置。

- 参数:
   - `v`: 语音句柄。
   - `p`: 语音参数。   
   
- 返回值:
   - 0: 成功。
   - -1: 失败。

### voice_add_mic 
`int voice_add_mic(voice_t *v, voice_pcm_param_t *p);`

- 功能描述:
   - 添加麦克风。

- 参数:
   - `v`: 语音句柄。
   - `p`: 麦克风参数。   
   
- 返回值:
   - 0: 成功。
   - -1: 失败。

### voice_add_ref
`int voice_add_ref(voice_t *v, voice_pcm_param_t *p);`

- 功能描述:
   - 添加REF。？

- 参数:
   - `v`: 语音句柄。
   - `p`: REF参数。   
   
- 返回值:
   - 0: 成功。
   - -1: 失败。

### voice_start 
`int voice_start(voice_t *v);`

- 功能描述:
   - 语音开始。

- 参数:
   - `v`: 语音句柄。
   
- 返回值:
   - 0: 成功。
   - -1: 失败。

### voice_stop
`int voice_stop(voice_t *v);`

- 功能描述:
   - 语音停止。

- 参数:
   - `v`: 语音句柄。
   
- 返回值:
   - 0: 成功。
   - -1: 失败。


## 示例代码

```c
void voice_event(void *priv, voice_evt_id_t evt_id, void *data, int len)
{
    char *p = data;
    int data_len = len;

    if (evt_id == VOICE_ASR_EVT) {
        LOGD(TAG, "voice asr evt");
    } else if (evt_id == VOICE_DATA_EVT) {
        voice_data_t *vdata = (voice_data_t *)data;
        LOGD(TAG, "voice date evt(%p)(%d)", vdata->data, vdata->len);
    } else if (evt_id == VOICE_SILENCE_EVT) {
        LOGD(TAG, "voice silence evt");
    }
}

void mic_adaptor_init(void)
{
    voice_pcm_param_t pcm_p;
    voice_param_t v_p;
    voice_t *v;

    v = voice_init(voice_event, NULL);

    pcm_p.access = 0;
    pcm_p.channles = 1;
    pcm_p.channles_sum = 1;
    pcm_p.rate = 16000;
    pcm_p.sample_bits = 16;
    pcm_p.pcm_name = "pcmC0";
    voice_add_mic(v, &pcm_p);
    pcm_p.pcm_name = "pcmC2";
    voice_add_ref(v, &pcm_p);

    v_p.cts_ms = 80;
    v_p.ipc_mode = 0;
    voice_config(v, &v_p);

    voice_start(v);

```

## 诊断错误码
无。

## 运行资源
无。

## 依赖资源
  - csi
  - aos
  - minialsa
  - ipc

## 组件参考
无。   
