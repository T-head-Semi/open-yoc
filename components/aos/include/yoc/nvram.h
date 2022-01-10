/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef YOC_NVRAM_H
#define YOC_NVRAM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function will init nvram.
 *
 * @param[in]   partname   the data pair of the key, less than 64 bytes
 * @return  0 on success, negative error on failure.
 */
int nvram_init(const char *partname);

/**
 * This function will reset nvram, clear all data.
 *
 * @return  0 on success, negative error on failure.
 */
int nvram_reset();

/**
 * This function will get data from the factory setting area.
 *
 * @param[in]   key   the data pair of the key, less than 64 bytes
 * @param[in]   len   the length of the buffer
 * @param[out]  value   the buffer that will store the data
 * @return  the length of the data value, error code otherwise
 */
int nvram_get_val(const char *key, char *value, int len);

/**
 * This function will set data to the factory setting area.
 *
 * @param[in]   key   the data pair of the key, less than 64 bytes
 * @param[in]   value the data pair of the value, delete the pair if value == NULL
 * @return  0 on success, negative error on failure.
 */
int nvram_set_val(const char *key, char *value);


/**
 * This function will get ali iot info from factory setting area.
 *
 * @param[out] product_key output ali iot product key
 * @param[in&out] product_key_len in buffer len, out real len
 * @param[out] device_name output ali iot device name
 * @param[in&out] device_name_len in buffer len, out real len
 * @param[out] device_secret output ali iot device device secret
 * @param[in&out] device_secret_len in buffer len, out real len
 */
int nvram_get_iot_info(char *product_key, uint32_t *product_key_len,
                       char *device_name, uint32_t *device_name_len,
                       char *device_secret, uint32_t *device_secret_len);


#ifdef __cplusplus
}
#endif

#endif

