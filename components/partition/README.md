## 概述
partition(分区管理)是基于imtb(image mtb)里的分区信息来统一管理flash分区的接口。它能够统一管理flash分区信息，并对分区进行统一的读写以及擦除的操作。

### 分区定义
#### 分区说明
分区信息保存在每个board组件的config中, 例如boards/pangu/config/config.yaml 该文件为文本格式可打开查看分区情况。

#### 分区示例

```c
mtb_version: 4
chip: yunvoice

diff:
  digest_type: SHA1
  signature_type: RSA1024
  fota_version: 0
  ram_buf: 50      #DEC     KB   ( max ram need)
  flash_buf: 8     #DEC     KB   ( buffer size)
  flash_sector: 4096 #DEC     byte ( flash sector)
  diff_mode: 010    #BIN
  double_control: 1
flash:
  base_address: 0
  run_base_address: 0x40000000 # base address of Storage
  sector: 4096       # Bytes
  size: 8388608      # $(sector count) * sector

partitions:
  - { name: boot,   address: 0x000000, load_addr: 0x20000, size: 0x01E000, verify: true }
  - { name: imtb,   address: 0x01E000, size: 0x001000 }
  - { name: ai,     address: 0x020000, size: 0x080000, load_addr: 0x40600000, update: DIFF, verify: true }
  - { name: prim,   address: 0x0A0000, size: 0x200000, load_addr: 0x40000000, update: DIFF, verify: true }
  - { name: kv,     address: 0x2A0000, size: 0x008000 }
  - { name: spiffs, address: 0x2A8000, size: 0x200000 }

```

##### 一级字段说明

| 字段 | 说明 |
| :--- | :--- |
| mtb_version | mtb格式版本号 |
| chip | 芯片类型 |
| diff	| 用于差分相关的配置参数 |
| flash	| flash相关参数 |
| partitions | 分区表相关参数 |

##### diff字段说明

| diff | 是否必选 | 说明 |
| :--- | :--- | :--- |
| ram_buf | 是 | 差分还原时所需的ram空间，单位KB |
| flash_buf | 是 | 差分还原时所需的flash空间，单位KB |
| flash_sector | 是 | flash每个sector的尺寸，单位byte |
| diff_mode | 是 | 差分模式，二进制字符串 |
| double_control | 否 | 是否使用双flash，此字段不填或者0：单片flash，1：双片flash |
| signature_type | 是 | 签名算法 |
| digest_type | 是 | 摘要算法 |

##### diff_mode字段说明

| mode_value | verify | auto_config |
| :--- | :--- | :--- |
| 00 | disable | disable |
| 01 | disable | enable |
| 10 | enable |	disable |
| 11 | enable |	enable |

##### partitions字段说明

| 字段 | 是否必选 | 说明 |
| :--- | :--- | :--- |
| name | 是 | 分区名字，最长8个字节 |
| address | 是 | 分区起始地址 |
| load_addr | 否 | 分区加载地址，如果没有此字段，则默认使用address为加载地址 |
| size	| 是 | 	分区大小，单位byte |
| update | 否 |	分区升级类型[DIFF，FULL]，默认值为DIFF；类似boot这种分区是不能升级的，因此不能填写此字段 |
| verify | 否 |	是否要对分区进行签名 |
| file | 否 | 指定分区对应的镜像文件名字，不填则表示文件名与分区名相同 |

#### 常用分区说明

| 分区 | 说明 |
| :--- | :--- | :--- |
| imtb | 分区表，包含所有分区位置及安全信息 |
| tee | 安全执行镜像 |
| boot | bootloader |
| prim | 应用镜像 |
| misc | 升级备份区 |
| lpm | 低功耗信息保存区 |
| otp | 只读分区，存放设备信息及安全密钥 |
imtb\tee\boot分区的调整会涉及tee镜像、boot镜像、otp安全数据的调整，不建议用户调整
prim\misc\kv 可以根据实际情况进行调整

### 安全引导流程
上电BootRom验签bootloader，并跳转到bootloader
bootloader验签tee和prim分区，并跳转到tee
tee进行安全的初始化后，跳到prim执行用户应用

## 组件安装
```bash
yoc init
yoc install partition
```

## 配置
无。

## 接口列表

| 函数 | 说明 |
| :--- | :--- |
| partition_init | 分区初始化 |
| partition_open | 打开指定分区 |
| partition_close | 关闭指定分区 |
| partition_info_get | 获取指定分区信息 |
| partition_read | 从分区指定地址读取数据 |
| partition_write | 从分区指定地址写入数据 |
| partition_erase | 擦除指定区域数据 |
| partition_verify | 校验指定分区 |
| partition_all_verify | 校验所有分区 |
| partition_get_digest | 获取指定分区SHA摘要和SHA类型 |
| partition_set_region_safe | 安全设置指定分区 |
| partition_flash_open | 打开FLASH |
| partition_flash_close | 关闭FLASH |
| partition_flash_info_get | 获取FLASH信息 |
| partition_flash_read | 读取FLASH数据 |
| partition_flash_write | 写入FLASH数据 |
| partition_flash_erase | 擦除FLASH数据 |
| partition_flash_register | 注册FLASH操作函数 |
| mtb_init | MTB初始化 |
| mtb_get | 获取MTB句柄 |
| mtb_get_addr | 获取MTB地址 |
| mtb_get_bmtb_size | 获取BMTB大小 |
| mtb_get_size | 安全设置指定分区 |
| mtb_image_verify | 校验映像 |
| mtb_get_partition_info | 获取映像分区信息 |
| mtb_get_img_info | 获取映像信息 |
| get_section_buf | 获取段信息 |
| mtb_get_img_scn_addr | 获取映像段地址 |
| get_sys_partition | 获取系统分区 |
| get_app_version | 获取APP版本 |
| mtb_get_img_info_buf | 获取映像信息 |
| mtb_get_key_info | 获取KEY信息 |
| mtb_get_pubkey_info | 获取公共KEY信息 |
| get_length_with_digest_type | 获取指定摘要类型的长度 |
| get_length_with_signature_type | 获取指定签名类型的长度 |
| hash_calc_start | 哈希计算 |
| signature_verify_start | 签名校验 |
| crc32_calc_start | 计算CRC32 |

## 接口详细说明

### partition_init
`int partition_init(void);`

- 功能描述:
   - 初始化分区表。

- 参数:
   - 无。

- 返回值:
   - 大于0: 分区数目（初始化成功）。

   - -1: 初始化失败。

### partition_open
`partition_t partition_open(const char *name);`

- 功能描述:
   - 打开指定分区。

- 参数:
   - `name`: 分区名称。

- 返回值:
   - 大于等于0: 分区句柄。
   - -EINVAL: 执行失败，参数检测错误。

### partition_close
`void partition_close(partition_t partition)`;

- 功能描述:
   - 关闭指定分区。

- 参数:
   - `partition`: 分区句柄。

- 返回值:
   - 无。
   
### partition_info_get
partition_info_t *partition_info_get(partition_t partition);

- 功能描述:
   - 获取指定分区信息。

- 参数:
   - `partition`: 分区句柄。

- 返回值:
   - 非空: 指向分区信息的指针。
   - 空（NULL）: 找不到指定分区。

### partition_read
`int partition_read(partition_t partition, off_t off_set, void *data, size_t size);`

- 功能描述:
   - 从分区指定地址读取数据。

- 参数:
   - `partition`: 分区句柄。
   - `off_set`: 分区地址偏移。
   - `data`: 数据缓冲区。
   - `size`: 缓冲区长度。   

- 返回值:
   - 0: 数据读取成功。
   - 小于0: 数据读取失败，返回-EINVAL（参数检测错误）或者-EIO（IO错误）

### partition_write
`int partition_write(partition_t partition, off_t off_set, void *data, size_t size);`

- 功能描述:
   - 从分区指定地址写入数据。

- 参数:
   - `partition`: 分区句柄。
   - `off_set`: 分区地址偏移。
   - `data`: 数据缓冲区。
   - `size`: 缓冲区长度。   

- 返回值:
   - 0: 数据写入成功。
   - 小于0: 数据写入失败，返回-EINVAL（参数检测错误）或者-EIO（IO错误）。

### partition_erase
`int partition_erase(partition_t partition, off_t off_set, uint32_t block_count);`

- 功能描述:
   - 擦除指定区域数据。

- 参数:
   - `partition`: 分区句柄。
   - `off_set`: 分区起始地址。
   - `block_count`: 分区块数量。

- 返回值:
   - 0: 数据擦除成功。
   - 小于0: 数据擦除失败，返回-EINVAL（参数检测错误）或者-EIO（IO错误）。
   
### partition_verify
`int partition_verify(partition_t partition);`

- 功能描述:
   - 校验指定分区。

- 参数:
   - `partition`: 分区句柄。

- 返回值:
   - 0: 校验成功。
   - 小于0: 校验失败。

### partition_all_verify
`int partition_all_verify(void);`

- 功能描述:
   - 校验所有分区。

- 参数:
   - 无。

- 返回值:
   - 0: 校验成功。
   - 小于0: 校验失败。

### partition_get_digest
`int partition_get_digest(partition_t partition, uint8_t *out_hash, uint32_t *out_len);`

- 功能描述:
   - 获取指定分区SHA摘要和SHA类型。

- 参数:
   - `partition`: 分区句柄（入参）。
   - `out_hash`: 摘要数据（出参）。
   - `block_count`: 摘要数据长度（出参）。

- 返回值:
   - 0: 获取成功。
   - 小于0: 获取失败。
   
### partition_set_region_safe
`int partition_set_region_safe(partition_t partition);`

- 功能描述:
   - 安全设置指定分区。

- 参数:
   - `partition`: 分区句柄（入参）。

- 返回值:
   - 0: 设置成功。
   - 小于0: 设置失败。

### partition_flash_open
`void *partition_flash_open(int id);`

- 功能描述:
   - 打开指定FLASH。

- 参数:
   - `id`:  FLASH id。

- 返回值:
   - 非空: FLASH句柄。
   - 空（NULL）: 打开失败。
   
### partition_flash_close
`int partition_flash_close(void *handle);`

- 功能描述:
   - 关闭指定FLASH。

- 参数:
   - `handle`:  FLASH 句柄。

- 返回值:
   - 0: 关闭成功。
   - 小于0: 关闭失败。
   
### partition_flash_info_get
`int partition_flash_info_get(void *handle, partition_flash_info_t *info);`

- 功能描述:
   - 获取指定FLASH信息。

- 参数:
   - `handle`:  FLASH 句柄（入参）。
   - `info`:  指向FLASH信息的指针（出参）。

- 返回值:
   - 0: 获取成功。
   - 小于0: 获取失败。

#### partition_flash_info_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| start_addr | uint32_t | 起始地址 |
| sector_size | sector_size | 扇区大小 |
| sector_count | sector_count | 扇区计数 |

### partition_flash_read
`int partition_flash_read(void *handle, unsigned long addr, void *data, size_t data_len);`

- 功能描述:
   - 读取FLASH指定地址的数据。

- 参数:
   - `handle`:  FLASH 句柄（入参）。
   - `addr`:  FLASH地址（入参）。
   - `data`:  指向数据的指针。
   - `data_len`:  数据长度。

- 返回值:
   - 0: 读取成功。
   - 小于0: 读取失败。
   
### partition_flash_write
`int partition_flash_write(void *handle, unsigned long addr, void *data, size_t data_len);`

- 功能描述:
   - 往FLASH指定地址写入数据。

- 参数:
   - `handle`:  FLASH 句柄。
   - `addr`:  FLASH地址。
   - `data`:  指向数据的指针。
   - `data_len`:  数据长度。   

- 返回值:
   - 0: 写入成功。
   - 小于0: 写入失败。

### partition_flash_erase
`int partition_flash_erase(void *handle, unsigned long addr, size_t len);`

- 功能描述:
   - 擦除FLASH指定地址的数据。

- 参数:
   - `handle`:  FLASH 句柄。
   - `addr`:  起始地址。
   - `len`:  地址长度。   

- 返回值:
   - 0: 擦除成功。
   - 小于0: 擦除失败。

### partition_flash_register
`void partition_flash_register(partition_flash_ops_t *ops);`

- 功能描述:
   - 注册FLASH操作函数。

- 参数:
   - `ops`:  操作函数指针。
   
- 返回值:
   - 无。

#### partition_flash_ops_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| open | void *(*open)(int id) | 打开函数 |
| close | int (*close)(void *handle) | 关闭函数 |
| info_get | int (*info_get)(void *handle, partition_flash_info_t *info) | 信息获取函数 |
| read | int (*read)(void *handle, uint32_t addr, void *data, size_t data_len) | 数据读取函数 |
| write | int (*write)(void *handle, uint32_t addr, void *data, size_t data_len) | 数据写入函数 |
| erase |  int (*erase)(void *handle, uint32_t addr, size_t len) | 数据擦除函数 |

### mtb_init
`int mtb_init(void);`

- 功能描述:
   - MTB初始化。

- 参数:
   - 无。
   
- 返回值:
   - 0: 成功。
   - 非0: 失败。
   
### mtb_get
`mtb_t *mtb_get(void);`

- 功能描述:
   - 获取MTB句柄。

- 参数:
   - 无。
   
- 返回值:
   - 非空: MTB句柄。
   - NULL: 获取失败。
   
### mtb_get_addr
`uint32_t mtb_get_addr(uint8_t is_prim);`

- 功能描述:
   - 获取MTB地址。

- 参数:
   - `is_prim`:  是否为主地址的标记，0表示备份地址，1表示主地址。
   
- 返回值:
   - FLASH地址。

### mtb_get_bmtb_size
`uint32_t mtb_get_bmtb_size(const uint8_t *data);`

- 功能描述:
   - 获取BMTB大小。

- 参数:
   - `data`:  MTB缓冲区。
   
- 返回值:
   - MTB大小。

### mtb_get_size
`uint32_t mtb_get_size(void);`

- 功能描述:
   - 获取MTB大小（ITMB大小或者BMTB+IMTB大小）。

- 参数:
   - `data`:  MTB缓冲区。
   
- 返回值:
   - MTB大小。

### mtb_image_verify
`int mtb_image_verify(const char *name);`

- 功能描述:
   - 校验映像。

- 参数:
   - `name`: 映像名称。
   
- 返回值:
   - 0: 成功。
   - 非0: 失败。

### mtb_get_partition_info
`int mtb_get_partition_info(const char *name, mtb_partition_info_t *part_info);`

- 功能描述:
   - 获取映像分区信息。

- 参数:
   - `name`:  映像名称。
   - `part_info`:  分区信息缓冲区指针。   
   
- 返回值:
   - 0: 成功。
   - 非0: 失败。

#### mtb_partition_info_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| name | 字符数组 | 分区名称 |
| pub_key_name | 字符数组 | 公钥名称 |
| start_addr | uint32_t | 起始地址 |
| end_addr | uint32_t | 结束地址 |
| load_addr | uint32_t | 加载地址 |
| img_size | uint8_t | 映像大小 |
| part_type | scn_type_t | 分区类型 |

### mtb_get_img_info
`int mtb_get_img_info(const char *name, img_info_t *img_info);`

- 功能描述:
   - 获取映像信息。

- 参数:
   - `name`:  映像名称。
   - `img_info`:  映像信息缓冲区指针。   
   
- 返回值:
   - 0: 成功。
   - 非0: 失败。

#### img_info_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| img_name | 字符数组 | 映像名称 |
| img_ver | uint8_t * | 映像版本指针 |
| img_addr | uint32_t | 映像地址 |
| img_size | uint32_t | 映像大小 |
| img_part_size | uint32_t | 映像分区大小 |
| img_type | uint8_t | 映像类型 |

### get_section_buf
`int get_section_buf(uint32_t addr, uint32_t img_len, scn_type_t *scn_type, uint32_t *scn_size);`

- 功能描述:
   
- 获取段信息。
   
- 参数:
   - `addr`:  地址。
   - `img_len`:  映像长度。  
   - `scn_type`:  段类型指针。
   - `scn_size`:  段长度指针。   
   
- 返回值:
   - 0: 成功。
   - 非0: 失败。

#### scn_type

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| father_type | scn_father_type_t | 父类型 |
| son_type | son_type | 描述段子类型 |

### mtb_get_img_scn_addr
`int mtb_get_img_scn_addr(uint8_t *mtb_buf, const char *name, unsigned long *scn_addr);`

- 功能描述:
   - 获取映像段地址。

- 参数:
   - `mtb_buf`:  MTB缓冲区指针。
   - `name`:  映像名称度。  
   - `scn_addr`:  段地址。
   
- 返回值:
   - 0: 成功。
   - 非0: 失败。

### get_sys_partition
`int32_t get_sys_partition(uint8_t *out, uint32_t *out_len);`

- 功能描述:
   - 获取系统分区。

- 参数:
   - `out`:  分区缓冲区指针。
   - `out_len`:  分区缓冲区长度。  
   
- 返回值:
   - 0: 成功。
   - -1: 失败。

### get_app_version
'int get_app_version(uint8_t *out, uint32_t *out_len);`

- 功能描述:
   - 获取APP版本。

- 参数:
   - `out`:  版本缓冲区指针。
   - `out_len`:  版本缓冲区长度。  
   
- 返回值:
   - 0: 成功。
   - -1: 失败。
   
### mtb_get_img_info_buf
`int mtb_get_img_info_buf(uint8_t *buf, uint32_t *size, char *name);`

- 功能描述:
   - 获取映像信息。

- 参数:
   - `buf`:  缓冲区指针。
   - `size`:  长度指针。
   - `name`:  映像名称。   
   
- 返回值:
   - 0: 成功。
   - -1: 失败。
   
### mtb_get_key_info
`int mtb_get_key_info(key_info_t *info);`

- 功能描述:
   - 获取KEY信息。

- 参数:
   - `info`:  信息缓冲区指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。
   
#### key_info_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| n | uint8_t * | 整数n指针 |
| e | uint8_t * | 加密钥e指针 |
| d | uint8_t * | 解密钥d指针 |
| size | uint32_t | KEY长度 |

### mtb_get_pubkey_info
`int mtb_get_pubkey_info(key_info_t *info);`

- 功能描述:
   - 获取公共KEY信息。

- 参数:
   - `info`:  信息缓冲区指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。
   
#### key_info_t

| 成员 | 类型 | 说明 |
| :--- | :--- | :--- |
| n | uint8_t * | 整数n指针 |
| e | uint8_t * | 加密钥e指针 |
| d | uint8_t * | 解密钥d指针 |
| size | uint32_t | KEY长度 |

### get_length_with_digest_type
`int get_length_with_digest_type(digest_sch_e type);`

- 功能描述:
   - 获取指定摘要类型的长度。

- 参数:
   - `type`:  摘要类型。
- 返回值:
   - 摘要长度。
   
### get_length_with_signature_type
`int get_length_with_signature_type(signature_sch_e type);`

- 功能描述:
   - 获取指定签名类型的长度。

- 参数:
   - `type`:  签名类型。

- 返回值:
   - 签名长度。

### hash_calc_start
`int hash_calc_start(digest_sch_e ds, const unsigned char *input, int ilen, 
                    unsigned char *output, uint32_t *olen, int from_mem);`

- 功能描述:
   - 哈希计算。

- 参数:
   - `ds`:  摘要哈希类型。
   - `input`:  输入缓冲区指针。
   - `ilen`:  输入缓冲区长度。
   - `output`:  输出缓冲区指针。
   - `olen`:  输出缓冲区指针。
   - `from_mem`:  输入缓冲区是否在内存中, 0表示输入缓冲区在分区中，非0表示在内存中。

- 返回值:
   - 0: 成功。
   - -1: 失败。

### signature_verify_start
`int signature_verify_start(digest_sch_e ds, signature_sch_e ss,
                            uint8_t *key_buf, int key_size,
                            const uint8_t *src, int src_size,
                            const uint8_t *signature, int sig_size);`

- 功能描述:
   - 签名校验。

- 参数:
   - `ds`:  摘要哈希类型。
   - `ss`:  签名类型。
   - `key_buf`:  KEY缓冲区指针。
   - `key_size`:  KEY缓冲区长度。
   - `src`:  源数据指针。
   - `src_size`:  源数据长度。
   - `signature`:  签名指针。
   - `sig_size`:  签名长度。   

- 返回值:
   - 0: 成功。
   - -1: 失败。

### crc32_calc_start
`int crc32_calc_start(const uint8_t *input, uint32_t ilen, uint32_t *output);`

- 功能描述:
   - 计算CRC32。

- 参数:
   - `input`:  待计算数据指针。
   - `ilen`:  待计算数据长度。
   - `output`:  CRC232数据指针。

- 返回值:
   - 0: 成功。
   - -1: 失败。
   
## 示例

### 分区信息接口

```c
#ifndef YOC_PARTITION_H
#define YOC_PARTITION_H

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <devices/device.h>

#ifndef MTB_IMAGE_NAME_SIZE
#define MTB_IMAGE_NAME_SIZE 8
#endif

#define MAX_FLASH_NUM   10

typedef int partition_t;

typedef struct {
    char     description[MTB_IMAGE_NAME_SIZE];
    uint32_t start_addr;
    uint32_t length;
    uint16_t sector_size;
    uint8_t  idx;

    dev_t   *g_eflash_dev;
} hal_logic_partition_t;
#endif
```

### partition初始化

```c
#include <yoc_init.h>
void board_base_init()
{
    flash_csky_register(0);
    partition_init();
}
```

### partition接口对接kv api

```c
#include <yoc_config.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <devices/flash.h>
#include <yoc/partition.h>
#include <aos/debug.h>

#ifndef CONFIG_KVFS_EBLOCK_FOR_GC
#define CONFIG_KVFS_EBLOCK_FOR_GC 1
#endif

static int32_t     g_block_size;
static int32_t     g_kv_blk_cnt;
static partition_t g_eflash_part = -1;

ssize_t kvp_flash_read(int32_t bytesize, void *buff, int32_t off, int32_t blk_num)
{
    return partition_read(g_eflash_part, blk_num * g_block_size + off, buff, bytesize);
}

ssize_t kvp_flash_write(int32_t bytesize, void *buff, int32_t off, int32_t blk_num)
{
    return partition_write(g_eflash_part, blk_num * g_block_size + off, buff, bytesize);
}

ssize_t kvp_flash_erase(int32_t blk_num)
{
    return partition_erase(g_eflash_part, blk_num * g_block_size, 1);
}

extern int __kv_core_init(int eblock_num, int eblock_for_gc, int eblock_size);

int aos_kv_init(const char *partname)
{
    aos_check_return_einval(partname);

    g_eflash_part = partition_open(partname);

    if (g_eflash_part >= 0) {
        hal_logic_partition_t *lp = hal_flash_get_info(g_eflash_part);
        aos_assert(lp);

        g_block_size = lp->sector_size;
        g_kv_blk_cnt = lp->length / lp->sector_size;

        return __kv_core_init(g_kv_blk_cnt, CONFIG_KVFS_EBLOCK_FOR_GC, g_block_size);
    }

    return -1;
}
```

## 诊断错误码
无。

## 运行资源
无。

## 依赖资源
  - csi
  - sec_crypto
  - key_mgr

## 组件参考
无。