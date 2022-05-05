## 概述

常用的音频功放驱动，可同时支持多种驱动，通过ID选择，方便一个产品同时支持两种功放

## 配置

无

## 接口列表

| 函数                 | 说明        |
| :------------------- | :---------- |
| amplifier_init       | 初始化      |
| amplifier_deinit     | 卸载驱动    |
| amplifier_onoff      | 声音输出使能控制|
| amplifier_getid      | IIC型功放获取ID |
| amplifier_cfgbin_read | IIC型功放，读配置 |
| amplifier_cfgbin_read | IIC型功放，写配置 |
## 接口详细说明

### amplifier_init

`int amplifier_init(int amp_id, int mute_pin, int power_pin, int amp_mode);`

- 功能描述:
  - 初始化设备。
- 参数:
  - `amp_id`: 参见声明文件中AMP_ID开头的宏
  - `mute_pin`: 静音控制引脚号，无引脚配置-1
  - `power_pin`: 电源控制引脚号，无引脚配置-1
  - `amp_mode`: 功放模式，参见声明文件中AMP_MOD开头的宏
- 返回值:
  - 0: 成功。
  - <0: 失败。

### amplifier_deinit

`int amplifier_deinit(void);`

- 功能描述:
  - 卸载设备。
- 参数:
  - 无
- 返回值:
  - 0: 成功。
  - <0: 失败。

### amplifier_onoff
`int amplifier_onoff(int onoff);`
- 功能描述:
  - 设备使能
- 参数:
  - onoff 0:关闭 1:开启
- 返回值:
  - 0: 成功。
  - <0: 失败。

## 诊断错误码

无。

## 运行资源

无。

## 依赖资源
无。

## 组件参考

无。