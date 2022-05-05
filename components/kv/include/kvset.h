/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef AOS_KVSET_H
#define AOS_KVSET_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct kvset kv_t;
typedef struct flash_ops flash_ops_t;
typedef struct kvblock kvblock_t;
typedef struct kvnode kvnode_t;
#include "block.h"
#include "kv_cache_typedef.h"

struct flash_ops {
    int (*erase)(kv_t *kv, int pos, int size);
    int (*write)(kv_t *kv, int pos, void *data, int size);
    int (*read)(kv_t *kv, int pos, void *data, int size);
};

struct kvnode {
    uint8_t    rw;
    uint8_t    version;
    uint16_t   val_size; /* value size */
    uint32_t   erase_flag;

    uint16_t  head_offset; /* offset base block */
    uint16_t  value_offset;
    uint16_t  next_offset;
    uint16_t  node_size;

    kvblock_t *block;
};

struct kvset {
    kvblock_t        *blocks;
    int              num;
    int              bid;           ///< current block_id
    int              gc_bid;
    int              handle;
    uint8_t          *mem;
    flash_ops_t      *ops;
    uint8_t          had_conflict;  ///< flag for kv-error
#if (CONFIG_KV_ENABLE_CACHE || CONFIG_KV_START_OPT)
    slist_t          head;          ///< for kv verify
    hash_t           map;
    cache_node_t     *nodes;
    size_t           node_nb;
#endif
};

/**
 * @brief  init the kv fs
 * @param  [in] kv
 * @param  [in] mem        : the start addrress of flash or mem, etc
 * @param  [in] block_num  : number of blocks
 * @param  [in] block_size : size of per-block
 * @return 0/-1
 */
int kv_init(kv_t *kv, uint8_t *mem, int block_num, int block_size);

/**
 * @brief  reset the kv fs
 * @param  [in] kv
 * @return 0/-1
 */
int kv_reset(kv_t *kv);

/**
 * @brief  set key-value pair
 * @param  [in] kv
 * @param  [in] key
 * @param  [in] value
 * @param  [in] size  : size of the value
 * @return size on success
 */
int kv_set(kv_t *kv, const char *key, void *value, int size);

/**
 * @brief  get value by the key-string
 * @param  [in] kv
 * @param  [in] key
 * @param  [in] value
 * @param  [in] size  : size of the value
 * @return > 0 on success
 */
int kv_get(kv_t *kv, const char *key, void *value, int size);

/**
 * @brief  delete the key from kv fs
 * @param  [in] kv
 * @param  [in] key
 * @return 0 on success
 */
int kv_rm(kv_t *kv, const char *key);

/**
 * @brief  find the kvnode by key
 * @param  [in] kv
 * @param  [in] key
 * @param  [in] node : used for store the result finding
 * @return 0 if find
 */
int kv_find(kv_t *kv, const char *key, kvnode_t *node);

/**
 * @brief  iterate all valid kv pair
 * @param  [in] kv
 * @param  [in] fn   : callback
 * @param  [in] data : opaque of the fn callback
 * @return 0 on success
 */
int kv_iter(kv_t *kv, int (*fn)(kvnode_t *, void *), void *data);

/**
 * @brief  kv garbage collect
 * @param  [in] kv
 * @return 0/-1
 */
int  kv_gc(kv_t *kv);

/**
 * @brief  dump all the kv pair to stdout
 * @param  [in] kv
 * @return
 */
void kv_dump(kv_t *kv);

/**
 * @brief  show all kv to stdout in hex
 * @param  [in] kv
 * @return
 */
void kv_show_data(kv_t *kv);

#define KVNODE_OFFSET2CACHE(kv_node, kv_offset) ((kv_node)->block->mem_cache + (kv_node)->kv_offset)
#define KVNODE_CACHE2OFFSET(kv_node, mem_addr) (mem_addr - (kv_node)->block->mem_cache)

#ifdef __cplusplus
}
#endif

#endif
