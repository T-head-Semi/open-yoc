/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _MODEL_BLOB_SRV_H_
#define _MODEL_BLOB_SRV_H_

#include "blob_common.h"

#ifndef CONFIG_BLOB_SRV_DEFAULT_TTL
#define BLOB_SRV_DEFAULT_TTL   (3)
#else
#define BLOB_SRV_DEFAULT_TTL   (CONFIG_BLOB_SRV_DEFAULT_TTL)
#endif


#define BLOB_SRV_OPC_NUM 10

#define MESH_MODEL_BLOB_SRV(_user_data) BT_MESH_MODEL(BT_MESH_MODEL_ID_BLOB_SRV, \
        g_blob_srv_op, NULL, _user_data)

extern const struct bt_mesh_model_op g_blob_srv_op[BLOB_SRV_OPC_NUM];



#ifndef CONFIG_BLOB_SRV_MAX_BLOB_SIZE
#define CONFIG_BLOB_SRV_MAX_BLOB_SIZE  (0x400000) ////4M Bytes
#endif

#if defined(CONFIG_BT_MESH_EXT_ADV) && CONFIG_BT_MESH_EXT_ADV > 0
#ifndef CONFIG_MAX_BLOB_SRV_MTU_SIZE
#define CONFIG_MAX_BLOB_SRV_MTU_SIZE (200)
#endif
#else
#ifndef CONFIG_MAX_BLOB_SRV_MTU_SIZE
#define CONFIG_MAX_BLOB_SRV_MTU_SIZE (44)
#endif
#endif

#define  CONFIG_BLOB_SRV_MAX_CHUNK_SIZE (CONFIG_MAX_BLOB_SRV_MTU_SIZE - 4)



#ifndef CONFIG_BLOB_SRV_MIN_BLOCK_SIZE_LOG
#define CONFIG_BLOB_SRV_MIN_BLOCK_SIZE_LOG (2)
#endif

#ifndef CONFIG_BLOB_SRV_MAX_BLOCK_SIZE_LOG
#define CONFIG_BLOB_SRV_MAX_BLOCK_SIZE_LOG (18)
#endif


#ifndef CONFIG_BLOB_SRV_MAX_TOTAL_CHUNKS
#define CONFIG_BLOB_SRV_MAX_TOTAL_CHUNKS (1200)
#endif

#ifndef CONFIG_BLOB_SRV_MAX_TOTAL_BLOCKS
#define CONFIG_BLOB_SRV_MAX_TOTAL_BLOCKS (10)
#endif

#define CONFIG_BLOB_SRV_MAX_MISS_CHUNKS_FLAG_SIZE   ((CONFIG_BLOB_SRV_MAX_TOTAL_CHUNKS + 7) / 8)
#define CONFIG_BLOB_SRV_MAX_MISS_BLOCKS_FLAG_SIZE   ((CONFIG_BLOB_SRV_MAX_TOTAL_BLOCKS + 7) / 8)



#ifndef BLOB_ACK_MIN_RANDOM_DELAY
#define BLOB_ACK_MIN_RANDOM_DELAY (10) //ms
#endif

#ifndef BLOB_ACK_MAX_RANDOM_DELAY
#define BLOB_ACK_MAX_RANDOM_DELAY (100) //ms
#endif


typedef struct {
    u8_t            id_set_flag;
} blob_srv_trans_ctx;


typedef struct {
    u32_t           node_id;
    blob_info       blob_srv_info;
    blob_block_info blob_srv_block_info;
    u8_t            blob_cli_ttl;
    u8_t            blob_srv_ttl;
    u8_t            blob_srv_trans_mode;
    u16_t           blob_srv_mtu_size;
    u16_t           blob_srv_timeout_base;
    u16_t           blob_multicast_addr;
    u8_t            blob_trans_phase;
    u64_t           blob_expected_blob_id;
    u8_t            blob_blocks_missing[CONFIG_BLOB_SRV_MAX_MISS_BLOCKS_FLAG_SIZE];
    u8_t            blob_chunks_missing[CONFIG_BLOB_SRV_MAX_MISS_CHUNKS_FLAG_SIZE];
    u16_t           blob_mtu_size_of_cli;
    long long       ack_last;
    blob_srv_trans_ctx  blob_trans_ctx;
    k_timer_t       srv_timer;
} blob_srv;

#define MESH_MODEL_BLOB_SRV_NULL()  MESH_MODEL_BLOB_SRV((&(blob_srv) {0}))

int blob_srv_trans_init(uint8_t elem_id, uint8_t mode, uint64_t except_blob_id);

#endif // _MODEL_BLOB_SRV_H_
