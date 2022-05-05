/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef __DFU_PORT_H
#define __DFU_PORT_H

#include "stdint.h"
#include <stdbool.h>

#define DFU_IMAGE_SIZE_MAX 240*1024
#ifndef CONFIG_AIS_TOTAL_FRAME
#define CONFIG_AIS_TOTAL_FRAME 16
#endif

#ifndef CONFIG_MAX_FOTA_APP_VERSION_LENGTH
#define CONFIG_MAX_FOTA_APP_VERSION_LENGTH 64
#endif



 enum  {
   POS_FLASH,
   POS_RAM,
 };



void unlock_flash_all();
void dfu_reboot(void);
unsigned char dfu_check_checksum(short image_id, unsigned short *crc16_output);
int dfu_image_update(short signature, int offset, int length, int *p_void);
void lock_flash(void);

/**
 * @brief get the current runing partition.
 * @return the current runing partition.
 */
uint8_t get_program_image(void);

/**
 * @brief switch the running partition, without reboot.
 * @param[in] the partition which switch to.
 * @return the runing partition when next boot.
 */
uint8_t change_program_image(uint8_t dfu_image);

int erase_dfu_flash(uint8_t force);

int sal_ota_is_allow_reboot();
int dfu_image_update_no_crc(short signature, int offset, int length, uint8_t *buf);
uint8_t* dfu_get_start_address();
int dfu_image_read(uint8_t* start_address,uint32_t offset,uint32_t length,uint8_t* buffer);
int dfu_get_ota_partition_max_size(void);
int dfu_get_fota_file_size(uint8_t* start_addr, uint32_t pos);
uint32_t  dfu_get_fota_file_app_version(uint8_t* start_addr, uint32_t pos);
int dfu_image_get_crc(uint8_t* address,uint32_t size, uint16_t *crc16, uint8_t data_type);
int  get_version_str(uint32_t version,char* ver_str);

#endif
