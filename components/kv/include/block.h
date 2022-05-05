/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef KV_BLOCK_H
#define KV_BLOCK_H

#include <stdint.h>

#include "kvset.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ERASE_FLAG 0

enum {
    NODE_ALL = 0,
    NODE_EXISTS = 1,
};

#define NODE_VAILD(n) ((n)->erase_flag != ERASE_FLAG)
struct kvblock {
    int          id;
    uint8_t      *mem;
    uint8_t      *mem_cache;
    uint8_t      mem_cache_ref;
    uint16_t     size;
    uint16_t     write_offset;
    uint16_t     kv_size;
    uint16_t     dirty_size;
    uint16_t     count;
    uint16_t     ro_count; // readonly kv count
    kv_t        *kv;
};

/**
 * @brief  malloc memory for the block
 * @param  [in] block
 * @return
 */
void kvblock_cache_malloc(kvblock_t *block);

/**
 * @brief  free memory for the block
 * @param  [in] block
 * @return
 */
void kvblock_cache_free(kvblock_t *block);

/**
 * @brief  init the block
 * @param  [in] block
 * @param  [in] mem  : addr of the block
 * @param  [in] size : size of block mem
 * @return
 */
void kvblock_init(kvblock_t *block, uint8_t *mem, int size);

/**
 * @brief  reset the block, detele valid kv pair
 * @param  [in] block
 * @return
 */
void kvblock_reset(kvblock_t *block);

/**
 * @brief  count the block usage
 * @param  [in] block
 * @return
 */
void kvblock_calc(kvblock_t *block);

/**
 * @brief  dump the block to stdout
 * @param  [in] block
 * @return
 */
void kvblock_dump(kvblock_t *block);

static inline int kvblock_free_size(kvblock_t *block)
{
    return block->size - block->write_offset;
}

/**
 * @brief  find the kvnode by key
 * @param  [in] block
 * @param  [in] key
 * @param  [in] node : used for store the result finding
 * @return 0 if find
 */
int       kvblock_find(kvblock_t *block, const char *key, kvnode_t *node);

/**
 * @brief  search kv-node in the block
 * @param  [in] block
 * @param  [in] c    : start addr begin search of the block
 * @param  [in] node
 * @return 0/-1
 */
int       kvblock_search(kvblock_t *block, uint8_t *c, kvnode_t *node);

/**
 * @brief  check the two kvnode(same key) & delete the old one
 * @param  [in] node1
 * @param  [in] node2
 * @return node of the delete
 */
kvnode_t *kvblock_check_version(kvnode_t *node1, kvnode_t *node2);

/**
 * @brief  alloc a kv node, return node start write address
 * @param  [in] block
 * @param  [in] size
 * @return -1 on error
 */
int  kvblock_alloc_node(kvblock_t *block, int size);

/**
 * @brief  write the kv pair to the block
 * @param  [in] block
 * @param  [in] key
 * @param  [in] value
 * @param  [in] size    : size of the value
 * @param  [in] version : 1~255
 * @return -1 on error
 */
int  kvblock_set(kvblock_t *block, const char *key, void *value, int size, int version);

/**
 * @brief  iterate kv-pair of the block
 * @param  [in] block
 * @param  [in] exists
 * @param  [in] fn
 * @param  [in] data
 * @return 0 on success
 */
int  kvblock_iter(kvblock_t *block, int exists, int (*fn)(kvnode_t *, void *), void *data);


/**
 * @brief  show all kv to stdout in hex
 * @param  [in] block
 * @param  [in] num
 * @return
 */
void kvblock_show_data(kvblock_t *block, int num);

/**
 * @brief  delete the node
 * @param  [in] node
 * @return
 */
void kvnode_rm(kvnode_t *node);

/**
 * @brief  show the kvnode to stdout in hex
 * @param  [in] node
 * @return
 */
void kvnode_show(kvnode_t *node);

/**
 * @brief  compare the kvnode by key
 * @param  [in] node
 * @param  [in] key
 * @return 0 on equal
 */
int kvnode_cmp_name(kvnode_t *node, const char *key);

#ifdef __cplusplus
}
#endif

#endif
