/**
 * @file aiot_kv_api.h
 * @brief 
 * @date 2020-07-08
 *
 * @copyright Copyright (C) 2017-2020 Alibaba Group Holding Limited
 *
 * @details
 *
 */

#ifndef __AIOT_KV_API_H__
#define __AIOT_KV_API_H__

#include <stdint.h>
#include <errno.h>

#ifdef BUILD_AOS
#include <hal/soc/flash.h>
#else
//#include "aiot_al_kv.h"
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

#define KV_TOTAL_SIZE (8192)

/* The physical parition for key-value store */
#ifndef CONFIG_AOS_KV_PTN
#define KV_PTN HAL_PARTITION_PARAMETER_2
#else
#define KV_PTN CONFIG_AOS_KV_PTN
#endif

/* Defination of block information */
#define BLK_BITS 12                          /* The number of bits in block size */
#define BLK_SIZE (1 << BLK_BITS)             /* Block size, current is 4k bytes */
#define BLK_NUMS (KV_TOTAL_SIZE >> BLK_BITS) /* The number of blocks, must be bigger than KV_GC_RESERVED */
#define BLK_OFF_MASK ~(BLK_SIZE - 1)         /* The mask of block offset in key-value store */
#define BLK_STATE_USED 0xCC                  /* Block state: USED --> block is inused and without dirty data */
#define BLK_STATE_CLEAN 0xEE                 /* Block state: CLEAN --> block is clean, ready for used */
#define BLK_STATE_DIRTY 0x44                 /* Block state: DIRTY --> block is inused and with dirty data */
#define BLK_HEADER_SIZE 4                    /* The block header size 4bytes */

#define INVALID_BLK_STATE(state)     \
    (((state) != BLK_STATE_USED) &&  \
     ((state) != BLK_STATE_CLEAN) && \
     ((state) != BLK_STATE_DIRTY))

/* Defination of key-value item information */
#define ITEM_HEADER_SIZE 8     /* The key-value item header size 8bytes */
#define ITEM_STATE_NORMAL 0xEE /* Key-value item state: NORMAL --> the key-value item is valid */
#define ITEM_STATE_DELETE 0    /* Key-value item state: DELETE --> the key-value item is deleted */
#define ITEM_MAX_KEY_LEN 128   /* The max key length for key-value item */

#ifdef AIOT_DEVICE_TIMER_ENABLE
#define ITEM_MAX_VAL_LEN 1536 /* The max value length for key-value item */
#else
#define ITEM_MAX_VAL_LEN 1024 /* The max value length for key-value item */
#endif

#define ITEM_MAX_LEN (ITEM_HEADER_SIZE + ITEM_MAX_KEY_LEN + ITEM_MAX_VAL_LEN)

/* Defination of key-value store information */
#define KV_STATE_OFF 1                      /* The offset of block/item state in header structure */
#define KV_ALIGN_MASK ~(sizeof(void *) - 1) /* The mask of key-value store alignment */
#define KV_GC_RESERVED 1                    /* The reserved block for garbage collection */
#define KV_GC_STACK_SIZE 1024

#define KV_SELF_REMOVE 0
#define KV_ORIG_REMOVE 1

#define KV_MODULE_NAME "kv"

#define KV_VERSION "1.0"
#define KV_VERSION_MAX_LEN (8)
#define KV_VERSION_KEY "kv_verion"

#define KV_GC_TASK_NAME "kv_gc"
#define KV_GC_TASK_STACK_SIZE 4096

    typedef enum _kv_get_type_e
    {
        KV_GET_TYPE_STRING = 1,
        KV_GET_TYPE_BINARY,
        KV_GET_TYPE_INT,
        KV_GET_TYPE_FLOAT,
        KV_GET_TYPE_MAX
    } kv_get_type_e;

    /* Key-value function return code description */
    typedef enum
    {
        RES_OK = 0, /* Successed */
        RES_INIT_FAILED = -1,
        RES_CONT = -EAGAIN,           /* Loop continued */
        RES_NO_SPACE = -ENOSPC,       /* The space is out of range */
        RES_INVALID_PARAM = -EINVAL,  /* The parameter is invalid */
        RES_MALLOC_FAILED = -ENOMEM,  /* Error related to malloc */
        RES_ITEM_NOT_FOUND = -ENOENT, /* Could not find the key-value item */
        RES_FLASH_READ_ERR = -EIO,    /* The flash read operation failed */
        RES_FLASH_WRITE_ERR = -EIO,   /* The flash write operation failed */
        RES_FLASH_EARSE_ERR = -EIO    /* The flash earse operation failed */
    } result_e;

    typedef struct
    {
        char *p;
        int ret;
        uint16_t len;
    } kv_storeage_t;

    /* Flash block header description */
    typedef struct _block_header_t
    {
        uint8_t magic; /* The magic number of block */
        uint8_t state; /* The state of the block */
        uint8_t reserved[2];
    } __attribute__((packed)) block_hdr_t;

    /* Key-value item header description */
    typedef struct _item_header_t
    {
        uint8_t magic;       /* The magic number of key-value item */
        uint8_t state;       /* The state of key-value item */
        uint8_t crc;         /* The crc-8 value of key-value item */
        uint8_t key_len;     /* The length of the key */
        uint16_t val_len;    /* The length of the value */
        uint16_t origin_off; /* The origin key-value item offset, it will be used when updating */
    } __attribute__((packed)) item_hdr_t;

    /* Key-value item description */
    typedef struct _kv_item_t
    {
        item_hdr_t hdr; /* The header of the key-value item, detail see the item_hdr_t structure */
        char *store;    /* The store buffer for key-value */
        uint16_t len;   /* The length of the buffer */
        uint16_t pos;   /* The store position of the key-value item */
    } kv_item_t;

    /* Block information structure for management */
    typedef struct _block_info_t
    {
        uint16_t space; /* Free space in current block */
        uint8_t state;  /* The state of current block */
    } block_info_t;

    typedef struct _kv_handle_t
    {
        uint8_t kv_initialize;  /* The flag to indicate the key-value store is initialized */
        uint8_t gc_triggered;   /* The flag to indicate garbage collection is triggered */
        uint8_t clean_blk_nums; /* The number of block which state is clean */
        uint16_t write_pos;     /* Current write position for key-value item */
        void *kv_mutex;
        block_info_t block_info[BLK_NUMS]; /* The array to record block management information */
#ifndef BUILD_AOS
        int fd;
#ifdef COMPATIBLE_LK_KV
        uint8_t is_lk_kv;
#endif
#endif
    } kv_handle_t;

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef __AIOT_KV_API_H__ */
