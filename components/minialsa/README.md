# 声音架构（minialsa）

## 概述

MiniAlsa是一个在rtos上运行的ALSA(Advanced Linux Sound Architecture)，提供基本的pcm、mixer等接口。

## 组件安装

```bash
yoc init
yoc install minialsa
```

## 配置

无。

## 接口列表

minialsa接口如下所示：

| 函数 | 说明 |
| :--- | :--- |
| aos_mixer_open | 打开1个mixer |
| aos_mixer_close | 关闭mixer |
| aos_mixer_selem_set_playback_volume | 设置elem播放的音量 |
| aos_mixer_selem_set_capture_volume | 设置elem捕获的音量 |
| aos_mixer_selem_set_playback_volume_all | 设置elem播放的所有音量 |
| aos_mixer_selem_set_capture_volume_all | 设置elem捕获的所有音量 |
| aos_mixer_selem_set_playback_volume_range | 设置elem播放一定范围的音量 |
| aos_mixer_selem_set_capture_volume_range | 设置elem捕获一定范围的音量 |
| aos_mixer_selem_get_playback_volume | 获取elem播放的音量 |
| aos_mixer_load | 加载mixer |
| snd_elem_new | 创建声音元素 |
| aos_mixer_free | 释放mixer |
| aos_mixer_first_elem | 查找第一个elem |
| aos_mixer_last_elem | 查找最后一个elem |
| aos_mixer_elem_next | 查找下一个elem |
| aos_mixer_elem_prev | 查找前一个elem |
| aos_mixer_attach | 查找并打开一个mixer |
| aos_pcm_new | 创建一个pcm设备 |
| aos_pcm_set_ops | 补充 |
| aos_pcm_hw_params_alloca | 申请一块pcm 参数的空间 |
| aos_pcm_sw_params_alloca | 补充 |
| aos_pcm_open | 打开pcm设备 |
| aos_pcm_close | 关闭pcm设备 |
| aos_pcm_pause | 暂停/播放 pcm数据 |
| aos_pcm_drop | 退出pcm播放 |
| aos_pcm_drain | 等待缓存中的数据播放完成再退出pcm播放 |
| aos_pcm_hw_params_set_access | 设置pcm硬件中的access参数 |
| aos_pcm_hw_params_set_format | 设置pcm硬件中的format参数 |
| aos_pcm_hw_params_set_buffer_size_near | 设置pcm硬件中的buffer_size参数 |
| aos_pcm_hw_params_set_period_size_near | 设置pcm硬件中的period_size参数 |
| aos_pcm_hw_params_current | 文件中没有此接口 |
| aos_pcm_hw_free | 文件中没有此接口 |
| aos_pcm_hw_params_any | 设置pcm硬件中的部分参数，access/channel/rate/sample_bits |
| aos_pcm_hw_params | 设置pcm硬件参数并起效 |
| aos_pcm_prepare | 文件中没有此接口 |
| aos_pcm_wait | 等待pcm设备 |
| aos_pcm_recover | 覆盖pcm设备 |
| aos_pcm_hw_params_set_channels | 设置pcm硬件中的channels参数 |
| aos_pcm_hw_params_set_rate | 文件中没有此接口 |
| aos_pcm_hw_params_set_rate_near | 设置pcm硬件中的rate参数 |
| aos_pcm_hw_params_set_rate_resample | 文件中没有此接口 |
| aos_pcm_hw_params_set_export_buffer | 文件中没有此接口 |
| aos_pcm_hw_params_set_period_wakeup | 文件中没有此接口 |
| aos_pcm_hw_params_set_period_time | 文件中没有此接口 |
| aos_pcm_hw_params_set_period_size | 文件中没有此接口 |
| aos_pcm_hw_params_set_periods | 文件中没有此接口 |
| aos_pcm_hw_params_set_buffer_time | 文件中没有此接口 |
| aos_pcm_hw_params_set_buffer_size | 文件中没有此接口 |
| aos_pcm_set_params | 设置pcm硬件中的部分参数，format/access/channels/rate |
| aos_pcm_sw_params_current | 设置pcm软件当前参数 |
| aos_pcm_sw_params_set_tstamp_mode | 文件中没有此接口 |
| aos_pcm_sw_params_set_avail_min | 文件中没有此接口 |
| aos_pcm_sw_params_set_period_event | 文件中没有此接口 |
| aos_pcm_sw_params_set_start_threshold | 设置pcm软件中start_threshold参数 |
| aos_pcm_sw_params_set_stop_threshold | 设置pcm软件中stop_threshold参数 |
| aos_pcm_sw_params_set_silence_threshold | 文件中没有此接口 |
| aos_pcm_sw_params_set_silence_size | 文件中没有此接口 |
| aos_pcm_sw_params | 检查pcm软件参数合法性 |
| aos_pcm_writei | 写入交错型pcm数据 |
| aos_pcm_readi | 读取交错型pcm数据 |
| aos_pcm_writen | 文件中没有此接口 |
| aos_pcm_readn | 读取交错型pcm数据 |
| aos_pcm_bytes_to_frames | 字节数据转换成帧数据 |
| aos_pcm_frames_to_bytes | 帧数据转换成字节数据 |
| aos_card_new | 文件中没有此接口 |
| aos_card_free | 文件中没有此接口 |
| aos_card_attach | 查找并打卡一个声卡 |
| aos_card_lpm | 补充 |


## 接口详细说明

### aos_mixer_open
`int aos_mixer_open(aos_mixer_t **mixer, int mode);`

- 功能描述:
   - 打开一个mixer。

- 参数:
   - `mixer`: aos_mixer_t 指针。
   - `mode`: 暂未使用，默认填0。

- 返回值:
   - 0: 成功。
   - -1: 失败。

#### aos_mixer_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| card_name | const char* | 声卡名称 |
| card | card_dev_t | 声卡设备 |
| cur_elem | aos_mixer_elem_t | 当前elem |
| elems_head | slist_t | elem头指针 |

### aos_mixer_close
`int aos_mixer_close(aos_mixer_t *mixer);`

- 功能描述:
   - 关闭mixer。

- 参数:
   - `mixer`: aos_mixer_t 指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_mixer_selem_set_playback_volume
`int aos_mixer_selem_set_playback_volume(aos_mixer_elem_t *elem, aos_mixer_selem_channel_id_t channel, int value);`

- 功能描述:
   - 设置播放音量。

- 参数:
   - `elem`: aos_mixer_elem_t 指针。
   - `channel`: 通道。
   - `value`: 音量值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

#### aos_mixer_elem_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| name | const char* | 名称 |
| vol_l | elem_inter_t | 左音量 |
| vol_r | elem_inter_t | 右音量 |
| dB_l | elem_inter_t | 左声道db |
| dB_r | elem_inter_t | 右声道db |
| mixer | aos_mutex_t* | mixer |
| mutex | aos_mutex_t | 锁 |
| hdl | void* | 指针 |
| ops | struct sm_elem_ops* | 选项 |
| next | slist_t | 指针 |

#### aos_mixer_elem_t

| 类型 | 说明 |
| :--- | :--- |
| AOS_MIXER_SCHN_UNKNOWN | 未知通道 |
| AOS_MIXER_SCHN_FRONT_LEFT | 左前方通道 |
| AOS_MIXER_SCHN_FRONT_RIGHT | 右前方通道 |
| AOS_MIXER_SCHN_REAR_LEFT | 左后方通道 |
| AOS_MIXER_SCHN_REAR_RIGHT | 右后方通道 |
| AOS_MIXER_SCHN_FRONT_CENTER | 前方居中通道 |
| AOS_MIXER_SCHN_WOOFER | 低音扬声器 |
| AOS_MIXER_SCHN_SIDE_LEFT | 左侧通道 |
| AOS_MIXER_SCHN_SIDE_RIGHT | 右侧通道 |
| AOS_MIXER_SCHN_REAR_CENTER | 后方居中通道 |
| AOS_MIXER_SCHN_LAST | 最后的 |
| AOS_MIXER_SCHN_MONO | 单声道 |


### aos_mixer_selem_set_capture_volume
`int aos_mixer_selem_set_capture_volume(aos_mixer_elem_t *elem, aos_mixer_selem_channel_id_t channel, int value);`

- 功能描述:
   - 设置捕获音量。

- 参数:
   - `elem`: aos_mixer_elem_t 指针。
   - `channel`: 通道。
   - `value`: 音量值。

- 返回值:
   - 0: 成功。
   - 其它: 失败。

### aos_mixer_selem_set_playback_volume_all
`int aos_mixer_selem_set_playback_volume_all(aos_mixer_elem_t *elem, int value);`

- 功能描述:
   - 设置所有elem播放音量。

- 参数:
   - `elem`: aos_mixer_elem_t 指针。
   - `value`: 音量值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_mixer_selem_set_capture_volume_all

`int aos_mixer_selem_set_capture_volume_all(aos_mixer_elem_t *elem, int value);`

- 功能描述:
   - 设置所有elem捕获音量。

- 参数:
   - `elem`: aos_mixer_elem_t 指针。
   - `value`: 音量值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_mixer_selem_set_playback_volume_range
`int aos_mixer_selem_set_playback_volume_range(aos_mixer_elem_t *elem, int min, int max);`

- 功能描述:
   - 设置所有elem播放音量范围。

- 参数:
   - `elem`: aos_mixer_elem_t 指针。
   - `min`: 音量最小值。
   - `max`: 音量最大值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_mixer_selem_set_capture_volume_range
`int aos_mixer_selem_set_capture_volume_range(aos_mixer_elem_t *elem, int min, int max);`

- 功能描述:
   - 设置所有elem捕获音量范围。

- 参数:
   - `elem`: aos_mixer_elem_t 指针。
   - `min`: 音量最小值。
   - `max`: 音量最大值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_mixer_selem_get_playback_volume
`int aos_mixer_selem_get_playback_volume(aos_mixer_elem_t *elem, aos_mixer_selem_channel_id_t channel, int *value);`

- 功能描述:
   - 获取播放音量。

- 参数:
   - `elem`: aos_mixer_elem_t 指针。
   - `channel`: 通道。
   - `value`: 音量值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_mixer_load

`int aos_mixer_load(aos_mixer_t *mixer);`

- 功能描述:
   - 加载mixer。

- 参数:
   - `mixer`: 句柄。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### snd_elem_new
`int snd_elem_new(aos_mixer_elem_t **elem_ret, const char *name, sm_elem_ops_t *ops);`

- 功能描述:
   - 创建elem。

- 参数:
   - `elem_ret`: elem句柄。
   - `name`: elem名称。
   - `ops`: 回调。

- 返回值:
   - 0: 成功。
   - -1: 失败。

#### sm_elem_ops_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| set_dB | int | 回调 |
| volume_to_dB | 回调 |  |

### aos_mixer_free

`void aos_mixer_free(aos_mixer_t *mixer);`

- 功能描述:
   - 释放mixer。

- 参数:
   - `mixer`: 指针。

- 返回值:
   - 无。

### aos_mixer_first_elem
`aos_mixer_elem_t *aos_mixer_first_elem(aos_mixer_t *mixer);`

- 功能描述:
   - 查找第一个elem。

- 参数:
   - `mixer`: 指针。

- 返回值:
   - 指针: 成功。
   - NULL: 失败。

### aos_mixer_last_elem
`aos_mixer_elem_t *aos_mixer_last_elem(aos_mixer_t *mixer);`

- 功能描述:
   - 查找最后一个elem。

- 参数:
   - `mixer`: 指针。

- 返回值:
   - 指针: 成功。
   - NULL: 失败。

### aos_mixer_elem_next
`aos_mixer_elem_t *aos_mixer_elem_next(aos_mixer_elem_t *elem);`

- 功能描述:
   - 查找下一个elem。

- 参数:
   - `elem`: elem。

- 返回值:
   - 指针: 成功。
   - NULL: 失败。

### aos_mixer_elem_prev
`aos_mixer_elem_t *aos_mixer_elem_prev(aos_mixer_elem_t *elem);`

- 功能描述:
   - 查找前一个elem。

- 参数:
   - `elem`: elem。

- 返回值:
   - 指针: 成功。
   - NULL: 失败。

### aos_mixer_attach
`int aos_mixer_attach(aos_mixer_t *mixer, const char *name);`

- 功能描述:
   - 打开指定名称的mixer。

- 参数:
   - `mixer`: 句柄。
   - `name`: 名称。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_new
`int aos_pcm_new(aos_pcm_t **pcm, int type, const char *name, aos_pcm_stream_t stream, int mode);`

- 功能描述:
   - 创建pcm。

- 参数:
   - `pcm`: 句柄。
   - `type`: 类型。
   - `name`: 名称。
   - `stream`: 输入流。
   - `mode`: 模式。

- 返回值:
   - 0: 成功。
   - -1: 失败。

#### aos_pcm_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| card_name | const char* | 声卡名称 |
| pcm_name | const char* | pcm名称 |
| hdl | void* | 空指针 |
| state | aos_pcm_state_t | elem头指针 |
| stream | aos_pcm_stream_t | 流 |
| mode | int | 模式 |
| event | pcm_event_cb_t | 事件 |
| card | int | 声卡 |
| device | int | 设备 |
| ref | int | 参考 |
| ringbuffer | dev_ringbuf_t | 环形缓存 |
| mutex | aos_mutex_t | 锁 |
| evt | aos_event_t | 事件 |
| hw_params | aos_pcm_hw_params_t | 硬件参数 |
| sw_params | aos_pcm_sw_params_t | 软件参数 |
| ops | struct aos_pcm_ops* | 配置选项 |
| next | slist_t | 链表 |

#### aos_pcm_stream_t

| 类型 | 说明 |
| :--- | :--- |
| AOS_PCM_STREAM_PLAYBACK | 播放流 |
| AOS_PCM_STREAM_CAPTURE | 捕获流 |
| AOS_PCM_STREAM_LAST | 最近设置的流 |

### aos_pcm_set_ops
`void aos_pcm_set_ops(aos_pcm_t *pcm, int direction, struct aos_pcm_ops *ops);`

- 功能描述:
   - 设置pcm的配置选项。

- 参数:
   - `pcm`: 句柄。
   - `direction`: 方向选择，暂未使用。
   - `ops`: 配置选项。

- 返回值:
   - 无。

#### aos_pcm_ops

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| hw_params_set | int | 设置硬件参数 |
| hw_get_size | int | 获取size参数 |
| hw_get_remain_size | int | 获取remain_size参数 |
| prepare | int | 准备 |
| pause | int | 暂停 |
| write | int | 写 |
| read | int | 读 |
| set_event | int | 设置事件 |

### aos_pcm_hw_params_alloca
`int aos_pcm_hw_params_alloca(aos_pcm_hw_params_t **p);`

- 功能描述:
   - 申请一块pcm硬件参数的空间。

- 参数:
   - `p`: pcm硬件参数指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。

#### aos_pcm_hw_params_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| access | int | 数据模式 |
| format | int | 格式 |
| subformat | int | 子格式 |
| sample_bits | int | 采样率位数 |
| frame_bits | int | 帧位数 |
| channels | int | 通道 |
| rate | int | 速率 |
| period_time | int | 录音事件触发周期 |
| period_size | int | 录音事件触发周期 |
| period_bytes | int | 录音事件触发周期 |
| periods | int | 录音事件触发周期 |
| buffer_time | int | 录音缓存 |
| buffer_size | int | 录音缓存 |
| buffer_bytes | int | 录音缓存 |
| tick_time | int | tick |

### aos_pcm_sw_params_alloca
`int aos_pcm_sw_params_alloca(aos_pcm_sw_params_t **p);`

- 功能描述:
   - 申请一块pcm软件参数的空间。

- 参数:
   - `p`: pcm软件参数指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。

#### aos_pcm_sw_params_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| tstamp_mode | int | 时间戳模式 |
| period_step | unsigned int | 周期 |
| sleep_min | unsigned int | 睡眠时间 |
| avail_min | aos_pcm_uframes_t | 可用最小帧数据 |
| xfer_align | aos_pcm_uframes_t | xfer对齐 |
| start_threshold | aos_pcm_uframes_t | 开始阈值 |
| stop_threshold | aos_pcm_uframes_t | 停止阈值 |
| silence_threshold | aos_pcm_uframes_t | 静音阈值 |
| silence_size | aos_pcm_uframes_t | 静音大小 |
| boundary | aos_pcm_uframes_t | 边界 |
| tstamp_type | unsigned int | 时间戳类型 |
| pads | int | 填充类型 |
| reserved | unsigned char | 保留 |
| period_event | unsigned int | 事件周期 |


### aos_pcm_open
`int aos_pcm_open(aos_pcm_t **pcm, const char *name, aos_pcm_stream_t stream, int mode);`

- 功能描述:
   - 打开pcm设备。

- 参数:
   - `pcm`: 打开成功的pcm设备所返回的指针。
   - `name`: 设备名。
   - `stream`: pcm流格式。
   - `mode`: pcm 操作方式（阻塞或非阻塞）。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_close
`int aos_pcm_close(aos_pcm_t *pcm);`

- 功能描述:
   - 关闭pcm设备。

- 参数:
   - `pcm`: pcm指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_pause

`int aos_pcm_pause(aos_pcm_t *pcm, int enable);`
- 功能描述:
   - 暂停/播放 pcm数据。

- 参数:
   - `pcm`: 指针。
   - `enable`: 0：播放 1：暂停。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_drop
`int aos_pcm_drop(aos_pcm_t *pcm);`

- 功能描述:
   - 退出pcm播放。

- 参数:
   - `pcm`: 指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_drain
`int aos_pcm_drain(aos_pcm_t *pcm);`

- 功能描述:
   - 等待缓存中的数据播放完成再退出pcm播放。

- 参数:
   - `pcm`: 指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_access
`int aos_pcm_hw_params_set_access(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, aos_pcm_access_t _access);`

- 功能描述:
   - 。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `_access`: 交错模式。

- 返回值:
   - 0: 成功。
   - -1: 失败。

#### aos_pcm_access_t

| 类型 | 说明 |
| :--- | :--- |
| AOS_PCM_ACCESS_RW_INTERLEAVED | 交错模式 |
| AOS_PCM_ACCESS_RW_NONINTERLEAVED | 非交错模式 |
| AOS_PCM_ACCESS_LAST | 最近的模式 |


### aos_pcm_hw_params_set_format
`int aos_pcm_hw_params_set_format(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, int val);`

- 功能描述:
   - 设置format参数。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_buffer_size_near
`int aos_pcm_hw_params_set_buffer_size_near(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, aos_pcm_uframes_t *val);`

- 功能描述:
   - 设置buffer_size参数。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

#### aos_pcm_uframes_t

typedef int aos_pcm_uframes_t;

### aos_pcm_hw_params_set_period_size_near
`int aos_pcm_hw_params_set_period_size_near(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, aos_pcm_uframes_t *val, int *dir);`

- 功能描述:
   - 设置period_size参数。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。
   - `dir`: 方向选择。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_current
`int aos_pcm_hw_params_current(aos_pcm_t *pcm, aos_pcm_hw_params_t *params);`

- 功能描述:
   - 设置当前参数。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_free
`int aos_pcm_hw_free(aos_pcm_t *pcm);`

- 功能描述:
   - 释放pcm资源。

- 参数:
   - `pcm`: 指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_any
`int aos_pcm_hw_params_any(aos_pcm_t *pcm, aos_pcm_hw_params_t *params);`

- 功能描述:
   - 设置参数。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params
`int aos_pcm_hw_params(aos_pcm_t *pcm, aos_pcm_hw_params_t *params);`

- 功能描述:
   - 设置pcm硬件参数并起效。

- 参数:
   - `pcm`: 指针。
   - `params`: 硬件参数。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_prepare
`int aos_pcm_prepare(aos_pcm_t *pcm);`

- 功能描述:
   - 准备pcm设备。

- 参数:
   - `pcm`: 指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_wait
`int aos_pcm_wait(aos_pcm_t *pcm, int timeout);`

- 功能描述:
   - 等待pcm设备。

- 参数:
   - `pcm`: 指针。
   - `timeout`: 超时时间。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_recover
`int aos_pcm_recover(aos_pcm_t *pcm, int err, int silent);`

- 功能描述:
   - 覆盖pcm设备。

- 参数:
   - `pcm`: 指针。
   - `err`: 错误码。
   - `silent`: 静音模式。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_channels
`int aos_pcm_hw_params_set_channels(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val);`

- 功能描述:
   - 设置通道参数。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_rate
`int aos_pcm_hw_params_set_rate(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val, int dir);`

- 功能描述:
   - 设置速率参数。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。
   - `dir`: 方向选择。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_rate_near
`int aos_pcm_hw_params_set_rate_near(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int *val, int *dir);`

- 功能描述:
   - 设置速率参数。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。
   - `dir`: 方向选择。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_rate_resample
`int aos_pcm_hw_params_set_rate_resample(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val);`

- 功能描述:
   - 设置采样率。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_export_buffer
`int aos_pcm_hw_params_set_export_buffer(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val);`

- 功能描述:
   - 设置buffer。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_period_wakeup
`int aos_pcm_hw_params_set_period_wakeup(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val);`

- 功能描述:
   - 设置周期。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_period_time
`int aos_pcm_hw_params_set_period_time(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val, int dir);`

- 功能描述:
   - 设置周期。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。
   - `dir`: 方向选择。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_period_size
`int aos_pcm_hw_params_set_period_size(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, aos_pcm_uframes_t val, int dir);`

- 功能描述:
   - 设置周期。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。
   - `dir`: 方向选择。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_periods
`int aos_pcm_hw_params_set_periods(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val, int dir);`

- 功能描述:
   - 设置周期。。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。
   - `dir`: 方向选择。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_buffer_time
`int aos_pcm_hw_params_set_buffer_time(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, unsigned int val, int dir);`

- 功能描述:
   - 设置buffer。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。
   - `dir`: 方向选择。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_hw_params_set_buffer_size
`int aos_pcm_hw_params_set_buffer_size(aos_pcm_t *pcm, aos_pcm_hw_params_t *params, aos_pcm_uframes_t val);`

- 功能描述:
   - 设置buffer。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_set_params
`int aos_pcm_set_params(aos_pcm_t *pcm, int format, aos_pcm_access_t acc, unsigned int channels, unsigned int rate, int soft_resample, unsigned int latency);`

- 功能描述:
   - 设置pcm参数。

- 参数:
   - `pcm`: 指针。
   - `format`: 格式。
   - `acc`: 交错模式。
   - `channels`: 通道。
   - `rate`: 比特率。
   - `soft_resample`: 采样率。
   - `latency`: 延迟。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_sw_params_current
`int aos_pcm_sw_params_current(aos_pcm_t *pcm, aos_pcm_sw_params_t *params);`

- 功能描述:
   - 设置当前参数。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_sw_params_set_tstamp_mode
`int aos_pcm_sw_params_set_tstamp_mode(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, int val);`

- 功能描述:
   - 设置时间戳模式。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_sw_params_set_avail_min
`int aos_pcm_sw_params_set_avail_min(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, aos_pcm_uframes_t val);`

- 功能描述:
   - 设置参数。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_sw_params_set_period_event
`int aos_pcm_sw_params_set_period_event(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, int val);`

- 功能描述:
   - 设置事件周期。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_sw_params_set_start_threshold
`int aos_pcm_sw_params_set_start_threshold(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, aos_pcm_uframes_t val);`

- 功能描述:
   - 设置开始阈值。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_sw_params_set_stop_threshold
`int aos_pcm_sw_params_set_stop_threshold(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, aos_pcm_uframes_t val);`

- 功能描述:
   - 设置停止阈值。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_sw_params_set_silence_threshold
`int aos_pcm_sw_params_set_silence_threshold(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, aos_pcm_uframes_t val);`

- 功能描述:
   - 设置静音阈值。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_sw_params_set_silence_size
`int aos_pcm_sw_params_set_silence_size(aos_pcm_t *pcm, aos_pcm_sw_params_t *params, aos_pcm_uframes_t val);`

- 功能描述:
   - 设置静音大小。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。
   - `val`: 参数值。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_sw_params
`int aos_pcm_sw_params(aos_pcm_t *pcm, aos_pcm_sw_params_t *params);`

- 功能描述:
   - 设置pcm软件参数。

- 参数:
   - `pcm`: 指针。
   - `params`: 参数。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_writei
`aos_pcm_sframes_t aos_pcm_writei(aos_pcm_t *pcm, const void *buffer, aos_pcm_uframes_t size);`

- 功能描述:
   - 写入交错型pcm数据。

- 参数:
   - `pcm`: 指针。
   - `buffer`: pcm 数据。
   - `size`: pcm 数据长度，以frame为单位。

- 返回值:
   - 0: 成功。
   - -1: 失败。

#### aos_pcm_sframes_t

typedef int aos_pcm_sframes_t;

### aos_pcm_readi
`aos_pcm_sframes_t aos_pcm_readi(aos_pcm_t *pcm, void *buffer, aos_pcm_uframes_t size);`

- 功能描述:
   - 读取交错型pcm数据。

- 参数:
   - `pcm`: 指针。
   - `buffer`: pcm 数据。
   - `size`: pcm 数据长度，以frame为单位。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_writen
`aos_pcm_sframes_t aos_pcm_writen(aos_pcm_t *pcm, void **bufs, aos_pcm_uframes_t size);`

- 功能描述:
   - 写入交错型pcm数据。

- 参数:
   - `pcm`: 指针
   - `bufs`: pcm 数据。
   - `size`: pcm 数据长度，以frame为单位。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_readn
`aos_pcm_sframes_t aos_pcm_readn(aos_pcm_t *pcm, void **bufs, aos_pcm_uframes_t size);`

- 功能描述:
   - 读取交错型pcm数据。

- 参数:
   - `pcm`: 指针
   - `bufs`: pcm 数据。
   - `size`: pcm 数据长度，以frame为单位。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_pcm_bytes_to_frames
`aos_pcm_sframes_t aos_pcm_bytes_to_frames(aos_pcm_t *pcm, ssize_t bytes);`

- 功能描述:
   - 字节数据转换帧格式数据。

- 参数:
   - `pcm`: 指针。
   - `bytes`: 字节。

- 返回值:
   - aos_pcm_sframes_t: 返回帧格式数据。

### aos_pcm_frames_to_bytes
`ssize_t aos_pcm_frames_to_bytes(aos_pcm_t *pcm, aos_pcm_sframes_t frames);`

- 功能描述:
   - 帧格式数据转换成字节数。

- 参数:
   - `pcm`: 指针。
   - `frames`: 帧格式数据。

- 返回值:
   - ssize_t: 返回字节数。

### aos_card_new
`int aos_card_new(int *parent, int idx, const char *xid, struct module *module, int size, snd_card_drv_t **card_ret);`

- 功能描述:
   - 创建声卡。

- 参数:
   - `parent`: 声卡的父设备。
   - `idx`: 声卡的编号。
   - `xid`: 声卡的标识符。
   - `module`: top level module for locking。
   - `size`: 内存大小。
   - `card_ret`: 返回创建声卡成功的指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。

#### snd_card_drv_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| drv | driver_t | 驱动 |
| pcm_head | slist_t | pcm头指针 |
| mixer_head | slist_t | mixer头指针 |


### aos_card_free
`int aos_card_free(snd_card_drv_t *card);`

- 功能描述:
   - 释放声卡。

- 参数:
   - `card`: 声卡指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### aos_card_attach
`int aos_card_attach(const char *name, card_dev_t **card);`

- 功能描述:
   - 查找并打开一个声卡。

- 参数:
   - `name`: 声卡名。
   - `card`: 设备打开成功后所存放位置。

- 返回值:
   - 0: 成功。
   - -1: 失败。

#### card_dev_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| device | aos_dev_t | 设备 |
| priv | void* | 自定义数据 |


### aos_card_lpm
`int aos_card_lpm(const char *name ,int state);`

- 功能描述:
   - 设置声卡低功耗模式。

- 参数:
   - `name`: 声卡名称。
   - `state`: 状态。

- 返回值:
   - 0: 成功。
   - -1: 失败。

## 示例

### A Minimal Playback Program

```C
void playback (void)
{
    int i;
    short buf[128];
    aos_pcm_t *playback_handle;
    aos_pcm_hw_params_t *hw_params;

    aos_pcm_open (&playback_handle, "pcmP0", AOS_PCM_STREAM_PLAYBACK, 0); //打开设备“pcmP0”

    aos_pcm_hw_params_alloca(&hw_params); //申请硬件参数内存空间

    aos_pcm_hw_params_any(playback_handle, hw_params); //初始化硬件参数
    aos_pcm_hw_params_set_access(playback_handle, hw_params, AOS_PCM_ACCESS_RW_INTERLEAVED); // 设置音频数据参数为交错模式
    aos_pcm_hw_params_set_format(playback_handle, hw_params, AOS_PCM_FORMAT_S16_LE); //设置音频数据参数为小端16bit
    aos_pcm_hw_params_set_rate_near(playback_handle, hw_params, 16000, 0); //设置音频数据参数采样率为16K
    aos_pcm_hw_params_set_channels(playback_handle, hw_params, 2); //设置音频数据参数为2通道

    aos_pcm_hw_params(playback_handle, hw_params); //设置硬件参数到具体硬件中

    aos_pcm_hw_params_free(hw_params); //释放硬件参数内存空间

    aos_pcm_prepare(playback_handle); //启动“pcmP0”设备

    for (i = 0; i < 10; ++i) {
        aos_pcm_writei(playback_handle, buf, 128);//发送交错音频数据
    }

    aos_pcm_close (playback_handle); //关闭设备
}
```

### A Minimal Capture Program

```c
void capture(void)
{
    int i;
    int err;
    short buf[128];
    aos_pcm_t *capture_handle;
    aos_pcm_hw_params_t *hw_params;

    aos_pcm_open (&capture_handle, "pcmC0", AOS_PCM_STREAM_CAPTURE, 0);//打开设备“pcmC0”

    aos_pcm_hw_params_malloc (&hw_params);//申请硬件参数内存空间

    aos_pcm_hw_params_any (capture_handle, hw_params);//初始化硬件参数
    aos_pcm_hw_params_set_access (capture_handle, hw_params, AOS_PCM_ACCESS_RW_INTERLEAVED);// 设置音频数据参数为交错模式
    aos_pcm_hw_params_set_format (capture_handle, hw_params, AOS_PCM_FORMAT_S16_LE);//设置音频数据参数为小端16bit
    aos_pcm_hw_params_set_rate_near (capture_handle, hw_params, 16000, 0);//设置音频数据参数采样率为16K
    aos_pcm_hw_params_set_channels (capture_handle, hw_params, 2);//设置音频数据参数为2通道

    aos_pcm_hw_params (capture_handle, hw_params);//设置硬件参数到具体硬件中

    aos_pcm_hw_params_free (hw_params);//释放硬件参数内存空间

    aos_pcm_prepare (capture_handle);/启动“pcmC0”设备

    for (i = 0; i < 10; ++i) {
        aos_pcm_readi (capture_handle, buf, 128);//接收交错音频数据
    }

    aos_pcm_close (capture_handle);//关闭设备
}
```

### A Minimal Mixer

``` C
void mixer(void)
{
    aos_mixer_t *mixer_hdl;
    aos_mixer_elem_t *elem;

    aos_mixer_open(&mixer_hdl, 0);  //申请mixer
    aos_mixer_attach(mixer_hdl, "card0"); //查找声卡 ”card0“
    aos_mixer_load(mixer_hdl); // 装载mxier
    elem = aos_mixer_first_elem(mixer_hdl); //查找第一个元素

    aos_mixer_selem_set_playback_volume_all(elem, 60); //设置音量为60
}
```


## 诊断错误码
无。


## 运行资源
无。


## 依赖资源
  - csi
  - aos


## 组件参考
无。

