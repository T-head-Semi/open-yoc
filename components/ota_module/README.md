# OTA_MODULE组件介绍

## 概述

​        OTA_Module组件，包含 ota server以及ota client模块，启用ota server模块需打开CONFIG_OTA_SERVER宏；启用ota clinet模块需打开CONFIG_OTA_CLIENT宏;

## 接口定义

### ota_server_init

- 函数原型

```c
int ota_server_init()
```

- 功能描述

​    OTA server初始化

- 参数描述

  无

- 返回值

| 返回值 |            |
| ------ | ---------- |
| 0      | 初始化成功 |
| 非0    | 初始化失败 |

- 注意事项

  无

### ota_server_cb_register

  - 函数原型

  ```c
  void ota_server_cb_register(struct ota_server_cb *cb)
  ```

  - 功能描述

  ​      注册ota server 回调函数

  - 参数描述

    | IN/OUT | NAME                     | DESC                                            |
    | ------ | ------------------------ | ----------------------------------------------- |
    | [in]   | struct ota_server_cb *cb | ota回调函数，参见struct ota_server_cb结构体定义 |



| ota_server_cb（结构体）定义             |                                                           |
| --------------------------------------- | --------------------------------------------------------- |
| ota_device_event_cb   device_event_cb   | 设备升级消息回调函数，参见ota_device_event_cb函数类型定义 |
| struct ota_server_cb *_next;            | 链表指针                                                  |

######  void (*ota_device_event_cb)(ota_device_state_en event,void * data)

- 功能描述

​      设备升级消息回调函数

- 参数描述

  | IN/OUT | NAME                       | DESC                                                  |
  | ------ | -------------------------- | ----------------------------------------------------- |
  | [in]   | ota_device_state_en  event | 设备升级事件，参见ota_firmware_state_en枚举定义       |
  | [in]   | void * data                | 设备升级事件数据，具体类型由ota_firmware_state_en决定 |



| ota_device_state_en（枚举）定义 |                                                           |
| ------------------------------- | --------------------------------------------------------- |
| OTA_STATE_IDLE    = 0x00        | 设备空闲（该消息不上报）                                  |
| OTA_STATE_ONGOING = 0x01,       | 设备升级开始，升级事件数据参见ota_state_ongoing结构体     |
| OTA_STATE_LOAD_COMPLETE = 0x02  | 设备固件下载成功（该消息不上报）                          |
| OTA_STATE_SUCCESS = 0x03        | 设备固件升级成功，升级事件数据参见ota_state_success结构体 |
| OTA_STATE_FAILD   = 0X04        | 设备固件升级失败，升级事件数据参见ota_state_fail结构体    |

| ota_state_ongoing（结构体）定义 |                                           |
| ------------------------------- | ----------------------------------------- |
| uint8_t firmware_index          | 固件索引                                  |
| device_info dev_info            | 待升级设备信息，参见device_info结构体定义 |
| uint8_t channel                 | 升级使用的通道,0x00: GATT  0x01: uart_hci |
| uint32_t old_ver                | 设备旧版本                                |

| device_info（结构体）定义 |                                       |
| ------------------------- | ------------------------------------- |
| mac_t addr;               | 待升级设备MAC信息,参见mac_t结构体定义 |
| uint16_t   unicast_addr   | 待升级设备单播地址                    |
| uint32_t   old_version    | 待升级设备旧版本                      |

| ota_state_success（结构体）定义 |                                                              |
| ------------------------------- | ------------------------------------------------------------ |
| uint8_t firmware_index          | 固件索引                                                     |
| device_info dev_info            | 待升级设备信息，参见device_info结构体定义                    |
| uint8_t channel                 | 升级使用的通道,0x00: GATT  0x01: uart_hci                    |
| uint32_t old_ver                | 设备旧版本                                                   |
| uint32_t new_ver                | 设备新版本                                                   |
| uint32_t cost_time              | 升级所使用的时间（指的是固件开始下载到下载完成的时间，不包括等待设备重启，获取版本号所花费的时间） |

| ota_state_fail（结构体）定义 |                                                              |
| ---------------------------- | ------------------------------------------------------------ |
| uint8_t firmware_index       | 固件索引                                                     |
| device_info dev_info         | 待升级设备信息，参见device_info结构体定义                    |
| uint8_t channel              | 升级使用的通道,0x00: GATT  0x01: uart_hci                    |
| uint32_t old_ver             | 设备旧版本                                                   |
| uint32_t new_ver             | 设备新版本                                                   |
| uint32_t cost_time           | 升级所使用的时间（指的是固件开始下载到下载完成的时间，不包括等待设备重启，获取版本号所花费的时间） |
| uint8_t  reason              | 升级失败原因，参见ota_fail_reason枚举定义                    |

| ota_fail_reason（枚举）定义            |                        |
| -------------------------------------- | ---------------------- |
| OTA_SUCCESS                   =0x00    | 设备升级成功           |
| OTA_FAIL_CONN               = 0x01     | 设备连接失败           |
| OTA_FAIL_INVAILD_VERSION    = 0x02     | 设备版本不对           |
| OTA_FAIL_DEV_REFUSED        = 0x03     | 设备拒绝升级           |
| OTA_FAIL_CRC_ERR            = 0x04     | 设备固件CRC校验失败    |
| OTA_FAIL_REBOOT_ERR_VERSION = 0x05     | 设备上报版本错误       |
| OTA_FAIL_CANCEL             = 0x06     | 设备取消升级           |
| OTA_FAIL_DISCONN            = 0x07     | 设备升级过程中断连     |
| OTA_FAIL_GET_VERSION        = 0x08     | 获取待升级设备版本出错 |
| OTA_FAIL_INVAILD_MTU        = 0x09     | 设备升级MTU协商失败    |
| OTA_FAIL_READ_FIRMWARE      = 0x0a     | 读取升级固件失败       |
| OTA_FAIL_SEND_FIRMWARE      = 0x0b     | 发送升级固件失败       |
| OTA_FAIL_DEV_FAILD          = 0x0c     | 升级设备主动停止升级   |
| OTA_FAIL_REPORT_VERSION_TIMEOUT = 0x0d | 设备上报版本超时       |
| OTA_FAIL_TIMEOUT                = 0x0e | 设备升级超时           |
| OTA_FAIL_FIRMWARE_DEL           =0x0f  | 升级固件删除           |
| OTA_FAIL_SET_TRANS_ID =  0x10          | 分发升级ID失败         |
| OTA_FAIL_ERR_TRANS_INFO = 0x11         | 升级信息交互失败       |

###### void (*ota_firmware_event_cb)(ota_firmware_state_en event,void * data)

- 功能描述

​      设备升级消息回调函数

- 参数描述

  | IN/OUT | NAME                         | DESC                                                    |
  | ------ | ---------------------------- | ------------------------------------------------------- |
  | [in]   | ota_firmware_state_en  event | 固件使用事件，参见ota_firmware_state_en枚举定义         |
  | [in]   | void * data                  | 固件升级消息数据，参见ota_firmware_state_data结构体定义 |

| ota_device_state_en（枚举）定义      |                          |
| ------------------------------------ | ------------------------ |
| FIRMWARE_STATE_IDLE           = 0x00 | 固件空闲（该消息不上报） |
| FIRMWARE_STATE_IN_USE         = 0x01 | 固件开始使用             |
| FIRMWARE_STATE_END_USE        = 0x02 | 固件使用结束             |
| FIRMWARE_STATE_REMOVED        = 0x03 | 固件移除                 |

| ota_firmware_state_data（结构体）定义 |          |
| ------------------------------------- | -------- |
| uint8_t firmware_index                | 固件索引 |

### ota_server_cb_unregister

  - 函数原型

  ```c
 void ota_server_cb_unregister(struct ota_server_cb *cb)
  ```

  - 功能描述

  ​      撤销ota server 回调函数

  - 参数描述

    | IN/OUT | NAME                     | DESC                                            |
    | ------ | ------------------------ | ----------------------------------------------- |
    | [in]   | struct ota_server_cb *cb | ota回调函数，参见struct ota_server_cb结构体定义 |

- 返回值

  无

- 注意事项

  无

### ota_server_upgrade_firmware_add

 - 函数原型

   ```c
   int  ota_server_upgrade_firmware_add(firmware_info firware)
   ```

- 功能描述

  ​      添加升级镜像

- 参数描述

  | IN/OUT | NAME                  | DESC                                  |
  | ------ | --------------------- | ------------------------------------- |
  | [in]   | firmware_info firware | 固件信息，参见firmware_info结构体定义 |

  | firmware_info（结构体）定义 |                                    |
  | --------------------------- | ---------------------------------- |
  | uint32_t address            | 固件存储起始地址                   |
  | int      size               | 固件大小                           |
  | uint32_t version            | 固件版本                           |
  | uint16_t crc16              | 固件CRC校验值                      |
  | uint8_t ota_flag            | 预留                               |
  | uint8_t  image_type         | 固件镜像类型（预留）               |
  | uint8_t  image_pos          | 固件位置，参见ota_image_pos_en定义 |
  | uint8_t ota_chanel          | 固件传输通道，参见ota_type_en定义  |
  | ota_firmware_event_cb cb    | 固件使用消息回调函数，参见firmware_event_cb函数类型定义   |

  | ota_image_pos_en（枚举）定义                |                 |
  | ------------------------------------------- | --------------- |
  | IMAGE_POS_FLASH                    = 0x00   | 固件存储在FLASH |
  | IMAGE_POS_RAM                        = 0x01 | 固件存储在RAM   |

  | ota_type_en（枚举）定义                         |                  |
  | ----------------------------------------------- | ---------------- |
  | OTA_BY_GATT                              = 0x00 | 通过GATT通道升级 |
  | OTA_BY_UART                              = 0x01 | 通过UART通道升级 |

- 返回值

    | 返回值 |                                |
    | ------ | ------------------------------ |
    | >= 0   | 添加固件成功，返回值为固件索引 |
    | < 0    | 添加固件失败                   |

- 注意事项

    无

### ota_server_upgrade_firmware_rm

- 函数原型

```c
int  ota_server_upgrade_firmware_rm(uint16_t index)
```

- 功能描述

​      待删除固件版本信息

- 参数描述

  | IN/OUT | NAME  | DESC     |
  | ------ | ----- | -------- |
  | [in]   | index | 固件索引 |

- 返回值

| 返回值 |      |
| ------ | ---- |
| 0      | 成功 |
| 非0    | 失败 |

- 注意事项

  删除固件信息后，该升级固件对应的所有升级设备将会删除，若该固件下存在节点正在升级，将会停止该升级

  ### ota_server_upgrade_firmware_rm

- 函数原型

  ```c
  ota_firmware* ota_server_upgrade_firmware_get(uint16_t index)
  ```

- 功能描述

  ​     根据固件索引获取固件信息

- 参数描述

  | IN/OUT | NAME  | DESC     |
  | ------ | ----- | -------- |
  | [in]   | index | 固件索引 |

- 返回值

  | 返回值 |                                        |
  | ------ | -------------------------------------- |
  | NULL   | 失败                                   |
  | 非NULL | 成功，返回值参见ota_firmware结构体定义 |



| ota_firmware（结构体）定义 |                      |
| -------------------------- | -------------------- |
| uint32_t address           | 固件存储起始地址     |
| uint32_t size              | 固件大小             |
| uint32_t version           | 固件版本             |
| uint8_t image_type         | 固件类型（默认0x00） |
| uint16_t crc16             | 固件CRC校验值        |
| uint8_t ota_flag           | 预留                 |

- 注意事项

  删除固件信息后，该升级固件对应的所有升级设备将会删除，若该固件下存在节点正在升级，将会停止该升级

### ota_server_upgrade_device_add

- 函数原型

```c
int ota_server_upgrade_device_add(uint16_t index,uint16_t device_list_size,device_info* devices_list)
```

- 功能描述

​      添加待OTA设备

- 参数描述

  | IN/OUT | NAME                      | DESC                              |
  | ------ | ------------------------- | --------------------------------- |
  | [in]   | uint16_t index            | 固件索引                          |
  | [in]   | uint16_t device_list_size | 设备列表大小                      |
  | [in]   | uint16_t device_size      | 设备列表，见device_info结构体定义 |

- 返回值

| 返回值 |          |
| ------ | -------- |
| 0      | 添加成功 |
| 非0    | 添加失败 |

- 注意事项

  无

### ota_server_upgrade_devices_rm

- 函数原型

```c
int ota_server_upgrade_device_rm(uint16_t index,uint16_t device_list_size,device_info* devices_list)
```

- 功能描述

​     删除待升级设备

- 参数描述

  | IN/OUT | NAME                      | DESC                              |
  | ------ | ------------------------- | --------------------------------- |
  | [in]   | uint16_t index            | 固件索引                          |
  | [in]   | uint16_t device_list_size | 设备列表大小                      |
  | [in]   | uint16_t device_size      | 设备列表，见device_info结构体定义 |

- 返回值

| 返回值 |      |
| ------ | ---- |
| 0      | 成功 |
| 非0    | 失败 |

- 注意事项

  删除待升级节点,若节点正在升级，停止该升级

### ota_server_upgrade_start

- 函数原型

```c
int ota_server_upgrade_start(uint16_t index)
```

- 功能描述

​     开始OTA升级

- 参数描述

  | IN/OUT | NAME           | DESC     |
  | ------ | -------------- | -------- |
  | [in]   | uint16_t index | 固件索引 |

- 返回值

| 返回值 |      |
| ------ | ---- |
| 0      | 成功 |
| 非0    | 失败 |

- 注意事项

  使用mesh ota协议升级需要在升级节点全部添加完成后调用该接口