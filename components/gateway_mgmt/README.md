# 概述

gateway_mgmt组件主要负责：

* mesh子设备管理
* 网关及子设备的FOTA管理
* 网关及子设备的OCC鉴权管理
* 飞燕云平台接入管理

## 接口定义

### gateway_ota_init

- 函数原型

```c
int gateway_ota_init()
```

- 功能描述

​    gateway OTA初始化

- 参数描述

  无

- 返回值

| 返回值 |            |
| ------ | ---------- |
| 0      | 初始化成功 |
| 非0    | 初始化失败 |

- 注意事项

  无

### gateway_ut_init

- 函数原型

```c
int gateway_ut_init()
```

- 功能描述

    网关子设备管理初始化

- 参数描述

  无

- 返回值

| 返回值 |            |
| ------ | ---------- |
| 0      | 初始化成功 |
| 非0    | 初始化失败 |

- 注意事项

  无

### gateway_fota_device_load

- 函数原型

```c
int gateway_fota_device_load()
```

- 功能描述

    将网关及mesh子设备对应cid添加到FOTA管理链表中

- 参数描述

  无

- 返回值

| 返回值 |            |
| ------ | ---------- |
| 0      | 初始化成功 |
| 非0    | 初始化失败 |

- 注意事项

  无

### gateway_fota_start

- 函数原型

```c
void gateway_fota_start()
```

- 功能描述

    开启FOTA定时查询云端版本

- 参数描述

  无

- 返回值

    无

- 注意事项

  无

### gateway_fota_rpt_gw_ver

- 函数原型

```c
void gateway_fota_rpt_gw_ver()
```

- 功能描述

    向云端上报网关版本号

- 参数描述

  无

- 返回值

    无

- 注意事项

  无

### gateway_ota_gateway_upgrade

  - 函数原型

  ```c
 int gateway_ota_gateway_upgrade(const void *image, uint32_t size, ota_firmware_event_cb firmware_callback)
  ```

  - 功能描述

  ​      升级网关controller及主控的firmware

  - 参数描述

    | IN/OUT | NAME                                    | DESC                                       |
    | ------ | --------------------------------------- | ------------------------------------------ |
    | [in]   | const void *image                       | 网关image                                  |
    | [in]   | uint32_t size                           | 网关image大小                              |
    | [in]   | ota_firmware_event_cb firmware_callback | 升级完成后的callback，用于free image等动作 |

- 返回值

  | 返回值 |          |
  | ------ | -------- |
  | 0      | 升级成功 |
  | 非0    | 升级失败 |

- 注意事项

  无

### gateway_mesh_init

  - 函数原型

  ```c
 int gateway_mesh_init(gateway_mesh_config_t *config)
  ```

  - 功能描述

        初始化网关mesh管理

  - 参数描述

    | IN/OUT | NAME                          | DESC                                                      |
    | ------ | ----------------------------- | --------------------------------------------------------- |
    | [in]   | gateway_mesh_config_t *config | mesh init参数，详见struct gateway_mesh_config_t结构体定义 |

- 返回值

  | 返回值 |            |
  | ------ | ---------- |
  | 0      | 初始化成功 |
  | 非0    | 初始化失败 |

- 注意事项

  无

### gateway_mesh_suspend

- 函数原型

```c
int gateway_mesh_suspend()
```

- 功能描述

    暂停网关mesh功能

- 参数描述

  无

- 返回值

| 返回值 |                  |
| ------ | ---------------- |
| 0      | 停止mesh功能成功 |
| 非0    | 停止mesh功能失败 |

- 注意事项

  无

### gateway_mesh_resume

- 函数原型

```c
int gateway_mesh_resume()
```

- 功能描述

    恢复网关mesh功能

- 参数描述

  无

- 返回值

| 返回值 |                  |
| ------ | ---------------- |
| 0      | 恢复mesh功能成功 |
| 非0    | 恢复mesh功能失败 |

- 注意事项

  无

### gateway_model_conv_init

- 函数原型

```c
void gateway_model_conv_init()
```

- 功能描述

    网关物模型转换相关初始化

- 参数描述

  无

- 返回值

    无

- 注意事项

  无

### gateway_occ_get_gw_triples

- 函数原型

```c
int gateway_occ_get_gw_triples()
```

- 功能描述

    从烧录的KP文件中获取网关的三元组/五元组信息

- 参数描述

  无

- 返回值

| 返回值 |          |
| ------ | -------- |
| 0      | 获取成功 |
| 非0    | 获取失败 |

- 注意事项

  无

### gateway_subdev_rpt_onoff

  - 函数原型

  ```c
 int gateway_subdev_rpt_onoff(char *mac, uint8_t pwrstate)
  ```

  - 功能描述

        向云端上报子设备onoff状态

  - 参数描述

    | IN/OUT | NAME             | DESC            |
    | ------ | ---------------- | --------------- |
    | [in]   | char *mac        | 子设备mac地址   |
    | [in]   | uint8_t pwrstate | 子设备onoff状态 |

- 返回值

  | 返回值 |          |
  | ------ | -------- |
  | 0      | 上报成功 |
  | 非0    | 上报失败 |

- 注意事项

  无
