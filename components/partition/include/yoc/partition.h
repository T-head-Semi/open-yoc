/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef YOC_PARTITION_H
#define YOC_PARTITION_H

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <mtb.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_MAX_PARTITION_NUM
#define CONFIG_MAX_PARTITION_NUM 12
#endif

#ifndef CONFIG_PARITION_NO_VERIFY
#define CONFIG_PARITION_NO_VERIFY 1
#endif

#ifndef CONFIG_NOT_SUPORRT_SASC
#define CONFIG_NOT_SUPORRT_SASC 1
#endif

typedef int partition_t;

typedef struct {
    char     description[MTB_IMAGE_NAME_SIZE];  // 分区名字
    uint32_t base_addr;                         // Flash基地址
    uint32_t start_addr;                        // 分区的偏移地址（不是绝对地址）
    uint32_t length;                            // 分区大小
    uint16_t sector_size;                       // Flash sector大小
    uint8_t  idx;

    void    *flash_dev;

    uint32_t load_addr;                         // 加载地址，一般指在RAM中的运行地址
    uint32_t image_size;                        // 镜像实际大小
    scn_type_t type;
    uint16_t rsv;
} partition_info_t;

/**
 * Init the partition table
 *
 * @return  partition num, >0: On success， otherwise is error
 */
int partition_init(void);

/**
 * Open partition with partition name
 *
 * @note  You can find partition name in 'config.yaml'
 *
 * @param[in]  name     The partition name, e.g., boot,kv,prim
 *
 * @return  <0: If an error occurred with any step, otherwise is the partition handle
 */
partition_t partition_open(const char *name);

/**
 * Close partition with partition handle
 *
 * @param[in]  partition    The partition handle
 *
 */
void partition_close(partition_t partition);

/**
 * Get the information of the specified flash area
 *
 * @param[in]  in_partition     The target flash logical partition
 * @param[in]  partition        The buffer to store partition info
 *
 * @return  0: On success， otherwise is error
 */
partition_info_t *partition_info_get(partition_t partition);

#define hal_flash_get_info(partition) partition_info_get(partition)

/**
 * Read data from an area on a Flash to data buffer in RAM
 *
 * @param[in]  partition       The target flash logical partition which should be read
 * @param[in]  off_set         Point to the start address that the data is read, and
 *                             point to the last unread address after this function is
 *                             returned, so you can call this function serval times without
 *                             update this start address.
 * @param[in]  data            Point to the data buffer that stores the data read from flash
 * @param[in]  size            The length of the buffer
 *
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_read(partition_t partition, off_t off_set, void *data, size_t size);

/**
 * Write data to an area on a flash logical partition without erase
 *
 * @param[in]  partition       The target flash logical partition which should be read which should be written
 * @param[in]  off_set         Point to the start address that the data is written to, and
 *                             point to the last unwritten address after this function is
 *                             returned, so you can call this function serval times without
 *                             update this start address.
 * @param[in]  data            point to the data buffer that will be written to flash
 * @param[in]  size            The length of the buffer
 *
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_write(partition_t partition, off_t off_set, void *data, size_t size);

/**
 * Erase an area on a Flash logical partition
 *
 * @note  Erase on an address will erase all data on a sector that the
 *        address is belonged to, this function does not save data that
 *        beyond the address area but in the affected sector, the data
 *        will be lost.
 *
 * @param[in]  partition     The target flash logical partition which should be erased
 * @param[in]  off_set       Offset address of the erased flash area
 * @param[in]  block_count   block_count  of the erased flash area
 *
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_erase(partition_t partition, off_t off_set, uint32_t block_count);

/**
 * Verify partition data
 *
 * @param[in]  partition     The partition handle
 *
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_verify(partition_t partition);

/**
 * Verify all partition data
 *
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_all_verify(void);

/**
 * Get SHA digest and SHA type for required partition
 *
 * @param[in]  partition     The partition handle
 * @param[in]  out_hash      Returned SHA digest for a given partition
 * @param[in]  out_len       Returned SHA digetst data length
 *
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_get_digest(partition_t partition, uint8_t *out_hash, uint32_t *out_len);

/**
 * Set the specified partitoin for safe
 *
 * @param[in]  partition     The partition handle
 *
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_set_region_safe(partition_t partition);

#if CONFIG_MULTI_FLASH_SUPPORT
/**
 * Get flash id by absolute address
 *
 * @param[in]  address   absolute address
 * @return  < 0: If an error occurred with any step, otherwise is the flash id
 */
int get_flashid_by_abs_addr(unsigned long address);
#endif

#ifdef __cplusplus
}
#endif

#endif