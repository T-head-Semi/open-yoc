/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _BLOB_CLI_H_
#define _BLOB_CLI_H_

#include "blob_common.h"

#ifndef CONFIG_BLOB_CLI_DEFAULT_TTL
#define BLOB_CLI_DEFAULT_TTL   (2)
#else
#define BLOB_CLI_DEFAULT_TTL   (CONFIG_BLOB_CLI_DEFAULT_TTL)
#endif


#define BLOB_CLI_OPC_NUM 5

#if defined(CONFIG_BT_MESH_EXT_ADV) && CONFIG_BT_MESH_EXT_ADV > 0
#ifndef  CONFIG_BLOB_CLI_MTU_SIZE
#define  CONFIG_BLOB_CLI_MTU_SIZE (200)
#endif
#else
#ifndef  CONFIG_BLOB_CLI_MTU_SIZE
#define  CONFIG_BLOB_CLI_MTU_SIZE (44)
#endif
#endif


#ifndef CONFIG_MAX_BLOB_RECEIVERS_SIZE
#define CONFIG_MAX_BLOB_RECEIVERS_SIZE 32
#endif

#ifndef CONFIG_MAX_BLOB_SIZE
#define CONFIG_MAX_BLOB_SIZE 1
#endif

#ifndef  CONFIG_MAX_BLOB_SET_ID_RETRY
#define  CONFIG_MAX_BLOB_SET_ID_RETRY  30
#endif

#ifndef  CONFIG_MAX_BLOB_TRANS_GET_RETRY
#define  CONFIG_MAX_BLOB_TRANS_GET_RETRY  30
#endif

#ifndef  CONFIG_MAX_BLOB_CAPABILITY_GET_RETRY
#define  CONFIG_MAX_BLOB_CAPABILITY_GET_RETRY  30
#endif


#ifndef  CONFIG_MAX_BLOB_TRANS_START_SEND_RETRY
#define  CONFIG_MAX_BLOB_TRANS_START_SEND_RETRY  30
#endif

#ifndef  CONFIG_MAX_BLOB_TRANS_CANCEL_SEND_RETRY
#define  CONFIG_MAX_BLOB_TRANS_CANCEL_SEND_RETRY  30
#endif

#ifndef  CONFIG_MAX_BLOCK_BLOCK_START_SEND_RETRY
#define  CONFIG_MAX_BLOCK_BLOCK_START_SEND_RETRY  30
#endif


#ifndef  CONFIG_MAX_BLOCK_GET_SEND_RETRY
#define  CONFIG_MAX_BLOCK_GET_SEND_RETRY  50
#endif

#ifndef  CONFIG_MAX_BLOCK_SUB_SEND_RETRY
#define  CONFIG_MAX_BLOCK_SUB_SEND_RETRY  15
#endif

#ifndef CONFIG_BLOB_CLI_PRI
#define CONFIG_BLOB_CLI_PRI 15
#endif

#ifndef CONFIG_BLOB_CLI_STACK_SIZE
#define CONFIG_BLOB_CLI_STACK_SIZE 2048  //
#endif

#ifndef CONFIG_BLOB_CLI_QUEUE_SIZE
#define CONFIG_BLOB_CLI_QUEUE_SIZE 20
#endif

#ifndef CONFIG_BLOB_CLI_MIN_BLOCK_SIZE_LOG
#define CONFIG_BLOB_CLI_MIN_BLOCK_SIZE_LOG (2)
#endif

#ifndef CONFIG_BLOB_CLI_MAX_BLOCK_SIZE_LOG
#define CONFIG_BLOB_CLI_MAX_BLOCK_SIZE_LOG (18)
#endif


#ifndef CONFIG_BLOB_CLI_MAX_TOTAL_CHUNKS
#define CONFIG_BLOB_CLI_MAX_TOTAL_CHUNKS (1200)
#endif

#ifndef CONFIG_BLOB_CLI_MAX_TOTAL_BLOCKS
#define CONFIG_BLOB_CLI_MAX_TOTAL_BLOCKS (10)
#endif

#define CONFIG_BLOB_CLI_MAX_MISS_CHUNKS_FLAG_SIZE   ((CONFIG_BLOB_CLI_MAX_TOTAL_CHUNKS + 7) / 8)
#define CONFIG_BLOB_CLI_MAX_MISS_BLOCKS_FLAG_SIZE   ((CONFIG_BLOB_CLI_MAX_TOTAL_BLOCKS + 7) / 8)



#if defined(CONFIG_BLOB_ACK_CTRL) && CONFIG_BLOB_ACK_CTRL > 0
#define   CONFIG_BLOB_ACK_CTRL_MAX_ACK_SIZE (10)
#endif


enum {
    BLOB_CLI_PROCEDURE_IDLE                     = 0x00,
    BLOB_CLI_PROCEDURE_SET_ID                   = 0x01,
    BLOB_CLI_PROCEDURE_RETRIEVE_CAPABLITY       = 0x03,
    BLOB_CLI_PROCEDURE_TRANSFER_BLOB            = 0x04,
    BLOB_CLI_PROCEDURE_BLOCK_START              = 0x05,
    BLOB_CLI_PROCEDURE_SEND_SUB                 = 0x06,
    BLOB_CLI_PROCEDURE_BLOCK_STATUS_GET         = 0x07,
    BLOB_CLI_PROCEDURE_TRANSFER_GET             = 0x08,
    BLOB_CLI_PROCEDURE_TRANSFER_CANCEL          = 0x09,
    BLOB_CLI_PROCEDURE_TRANSFER_COMPLETE        = 0x0a,
};


enum {
    PROCEDURE_STATUS_NO_RECEIVE                  = 0x00,
    PROCEDURE_STATUS_ALL_RECEIVE_ALL_SUCCESS     = 0x01,
    PROCEDURE_STATUS_ALL_RECEIVE_ALL_FAIL        = 0x02,
    PROCEDURE_STATUS_ALL_RECEIVE_SOME_FAIL       = 0x03,
    PROCEDURE_STATUS_SOME_RECEIVE_ALL_SUCCESS    = 0x04,
    PROCEDURE_STATUS_SOME_RECEIVE_ALL_FAIL       = 0x05,
    PROCEDURE_STATUS_SOME_RECEIVE_SOME_FAIL      = 0x06,
    PROCEDURE_STATUS_NO_NEED_RECEIVE             = 0x07,
};


extern const struct bt_mesh_model_op g_blob_cli_op[BLOB_CLI_OPC_NUM];

enum {
    BLOB_RECEIVER_INACTIVE = 0x00,
    BLOB_RECEIVER_ACTIVE   = 0x01,
};

typedef struct {
    u16_t address;
    u32_t id;
    u8_t  active_status: 1,
          status_get_flag: 2,
          block_complete_flag: 1;
    transfer_phase_en trans_phase;
    transfer_status_en status;
    blob_capability capability;
    u8_t  missing_blocks[CONFIG_BLOB_CLI_MAX_MISS_BLOCKS_FLAG_SIZE];
    u8_t  missing_chunks[CONFIG_BLOB_CLI_MAX_MISS_CHUNKS_FLAG_SIZE];
} blob_receiver;



typedef struct {
    u16_t multicast_addr;
    u16_t netkey_idx;
    u16_t appkey_idx;
    u8_t  ttl;
    u64_t  blob_id;
    u8_t  trans_mode;
    u8_t  trans_net_if;
    u16_t trans_options;
    u32_t timeout;
    uint16_t  dst_addr_size;
    uint16_t *dst_addr_list;
    u8_t  *blob_data;
    u32_t blob_data_len;
} blob_trans_info;


typedef struct {
    u16_t netkey_idx;
    u16_t appkey_idx;
    u8_t  trans_net_if;
    u32_t timeout;
} trans_info;


typedef struct {
    u32_t timeout;
} id_set_procedure;

typedef struct {
    u32_t timeout;
} trans_get_procedure;

typedef struct {

    uint32_t timeout;
} get_capability_procedure;

typedef struct {
    u16_t    total_block_num;
    u8_t     total_missing_blocks[CONFIG_BLOB_CLI_MAX_MISS_BLOCKS_FLAG_SIZE];
    u8_t     blocks_try_num[CONFIG_BLOB_CLI_MAX_TOTAL_BLOCKS];
    uint32_t timeout;
} blob_trans_procedure;

typedef struct {
    u16_t block_id;
    u16_t chunk_size;
    u8_t total_missing_chunks[CONFIG_BLOB_CLI_MAX_MISS_CHUNKS_FLAG_SIZE];

    uint32_t timeout;
} blob_block_trans_procedure;

typedef struct {
    u16_t chunk_num;
    u8_t send_new_block_flag: 1,
         send_start_flag: 1;
    u8_t try_num;
    long long start_time;
    uint32_t timeout;
} block_sub_send_procedure;

typedef struct {
    uint32_t timeout;
} blob_block_trans_get_procedure;


typedef struct {
    u8_t cancel_all_flag;
    uint32_t timeout;
} blob_trans_cancel_procedure;


typedef struct {
    u8_t procedure;
    uint8_t status;
} blob_all_srv_procedure_status;


typedef struct {
    id_set_procedure     id_set;
    trans_get_procedure  trans_get;
    get_capability_procedure capability_get;
    blob_trans_procedure     blob_trans;
    blob_block_trans_procedure block_trans;
    block_sub_send_procedure   sub_send;
    blob_block_trans_get_procedure block_get;
    blob_trans_cancel_procedure trans_cancel;
#if defined(CONFIG_BLOB_ACK_CTRL) && CONFIG_BLOB_ACK_CTRL > 0
    u8_t            ack_flags_size;
    u8_t            ack_set_flag;
    u8_t            ack_set_status;
    u8_t            ack_max_set_index;
    u8_t            ack_flags[CONFIG_BLOB_ACK_CTRL_MAX_ACK_SIZE];
#endif
} blob_process_sub_procedure;

typedef enum {
    BLOB_TRANS_NO_INIT_TRANS_GET      = BIT(0),
} transfer_option_en;

typedef struct {
    uint8_t procedure;
    uint8_t status;
} exist_data;



typedef struct {
    void (*send_start_cb)(u8_t elem_id, u64_t blob_id, int err, void *cb_data);
    void (*send_end_cb)(u8_t elem_id, u64_t blob_id, int err, void *cb_data);
    void (*node_status_cb)(u8_t elem_id, u64_t blob_id, u16_t unicast_addr, transfer_phase_en phase, transfer_status_en status);
} blob_send_cb;


typedef struct {
    u8_t            blob_status;
    u8_t            blob_sec_status;
    u8_t            blob_status_last;
    u8_t            elem_id;
    u16_t           trans_options;
    u8_t            srv_min_capability_set_flag;
    u16_t           srv_max_min_block_size_log;  //diffrent with spec
    u16_t           srv_min_max_block_size_log;  //diffrent with spec
    u16_t           srv_min_max_total_chunks;
    u16_t           srv_min_max_chunk_size;
    u32_t           srv_min_max_blob_size;
    u16_t           srv_min_server_mtu_size;
    trans_info      info;
    blob_process_sub_procedure sub_procedures;
    blob_send_cb        cb;
} blob_cli_trans_ctx;




typedef struct {
    struct k_sem sem;
    u8_t  cancel_flag;
} buffer_send_sem;



typedef struct _blob_cli_t {
    blob_info        blob_cli_info;
    blob_block_info  blob_cli_block_info;
    u8_t             blob_cli_ttl;
    u8_t             blob_cli_trans_mode;
    u16_t            blob_cli_timeout_base;
    u16_t            blob_multicast_addr;
    u16_t            receiver_size;
    blob_receiver    receiver_list[CONFIG_MAX_BLOB_RECEIVERS_SIZE];
    blob_cli_trans_ctx blob_client_trans;
    k_timer_t       cli_procedure_timer;
    u8_t            cli_try_num;
    k_timer_t       cli_sec_procedure_timer;
    u8_t            cli_sec_try_num;
    buffer_send_sem   send_sem;
    buffer_send_sem   send_end_sem;
    struct k_mutex         cli_mutex;
} blob_cli;

#define MESH_MODEL_BLOB_CLI(_user_data) BT_MESH_MODEL(BT_MESH_MODEL_ID_BLOB_CLI,g_blob_cli_op, \
        NULL,_user_data)

#define MESH_MODEL_BLOB_CLI_NULL()   MESH_MODEL_BLOB_CLI((&(struct _blob_cli_t) {0}))

static inline char* procedure_str(uint8_t procedure)
{
    switch (procedure) {
        case BLOB_CLI_PROCEDURE_IDLE :
            return "IDLE";

        case BLOB_CLI_PROCEDURE_SET_ID :
            return "SET_ID";

        case BLOB_CLI_PROCEDURE_RETRIEVE_CAPABLITY:
            return "RETRIEVE_CAPABLITY";

        case BLOB_CLI_PROCEDURE_TRANSFER_BLOB :
            return "TRANS_START";

        case BLOB_CLI_PROCEDURE_BLOCK_START :
            return "BLOCK_START";

        case BLOB_CLI_PROCEDURE_SEND_SUB :
            return "SEND_SUB";

        case BLOB_CLI_PROCEDURE_BLOCK_STATUS_GET:
            return "BLOCK_STATUS_GET";

        case BLOB_CLI_PROCEDURE_TRANSFER_GET:
            return "TRANS_STATUS_GET";

        case BLOB_CLI_PROCEDURE_TRANSFER_CANCEL:
            return "TRANS_CANCEL";

        case BLOB_CLI_PROCEDURE_TRANSFER_COMPLETE:
            return "TRANS_COMPLETE";

        default:
            return "UNKONW";
    }
}


int ble_mesh_blob_cli_blob_trans(uint8_t elem_id, blob_trans_info *info, blob_send_cb cb);
int ble_mesh_blob_cli_blob_trans_get_status_all(uint8_t elem_id, uint64_t blob_id);
int ble_mesh_blob_cli_blob_trans_get_status(uint8_t elem_id, uint64_t blob_id, uint16_t uncast_addr_size, uint16_t *uncast_addr_list);
int ble_mesh_blob_cli_blob_trans_cancel_all(uint8_t elem_id, uint64_t blob_id);
int ble_mesh_blob_cli_blob_trans_cancel(uint8_t elem_id, uint64_t blob_id, uint16_t uncast_addr_size, uint16_t *uncast_addr_list);
int ble_mesh_blob_cli_blob_trans_upper_set_success(uint8_t elem_id, uint64_t blob_id, uint16_t uncast_addr);
int ble_mesh_blob_cli_blob_reset(uint8_t elem_id);

#endif
