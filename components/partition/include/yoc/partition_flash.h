/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef __PARTITION_FLASH_H__
#define __PARTITION_FLASH_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_FLASH_NUM
#define CONFIG_FLASH_NUM 1
#endif

#if CONFIG_FLASH_NUM < 1 || CONFIG_FLASH_NUM > 3
#error "CONFIG_FLASH_NUM must be greater than 0 and less than 4."
#endif

typedef struct {
    unsigned long    start_addr;
    uint32_t    sector_size;
    uint32_t    sector_count;
} partition_flash_info_t;

typedef struct {
    void *handle;    /*!< the device handle */
    uint32_t index;  /*!< the index of handle */
} hdl_mgr_t;

typedef struct {
    void *(*open)(int id);
    int (*close)(void *handle);
    int (*info_get)(void *handle, partition_flash_info_t *info);
    int (*read)(void *handle, unsigned long addr, void *data, size_t data_len);
    int (*write)(void *handle, unsigned long addr, void *data, size_t data_len);
    int (*erase)(void *handle, unsigned long addr, size_t len);

    hdl_mgr_t hdl_mgr;
} partition_flash_ops_t;

/**
 * Open flash with ID
 *
 * @param[in]  id   flash id
 * @return  NULL: If an error occurred with any step, otherwise is the flash handle
 */
void *partition_flash_open(int id);

/**
 * Close flash with handle
 *
 * @param[in]  handle   flash handle
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_flash_close(void *handle);

/**
 * Get flash information with handle
 *
 * @param[in]   handle  flash handle
 * @param[out]  info    point to flash info buffer
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_flash_info_get(void *handle, partition_flash_info_t *info);

/**
 * Read data from flash
 *
 * @param[in]  handle       flash handle
 * @param[in]  addr         flash address
 * @param[in]  data         Point to the data buffer that stores the data read from flash
 * @param[in]  data_len     The length of the buffer
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_flash_read(void *handle, unsigned long addr, void *data, size_t data_len);

/**
 * Write data to flash
 *
 * @param[in]  handle       flash handle
 * @param[in]  addr         flash address
 * @param[in]  data         Point to the data buffer that stores the data write to flash
 * @param[in]  data_len     The length of the buffer
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_flash_write(void *handle, unsigned long addr, void *data, size_t data_len);

/**
 * Erase flash
 *
 * @param[in]  handle       flash handle
 * @param[in]  addr         flash address
 * @param[in]  len          The length that need to erase
 * @return  0 : On success, <0 If an error occurred with any step
 */
int partition_flash_erase(void *handle, unsigned long addr, size_t len);

/**
 * Register flash operation functions
 *
 * @param[in]  ops   partition_flash_ops_t
 * @return  NONE
 */
void partition_flash_register(partition_flash_ops_t *ops);

#ifdef __cplusplus
}
#endif
#endif /* __PARTITION_FLASH_H__ */