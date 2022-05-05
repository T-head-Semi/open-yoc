#ifndef _BLOB_COMMON_H
#define _BLOB_COMMON_H



typedef enum {
    TRANSFER_MODE_NO_ACTIVE  = 0x00,
    TRANSFER_MODE_PUSH       = 0x01,
    TRANSFER_MODE_PULL       = 0x02,
} transfer_mode_en;

typedef enum {
    ACK_STATUS_RECV_NULL = 0x00,
    ACK_STATUS_RECV_SOME = 0x01,
    ACK_STATUS_RECV_ALL  = 0x02,
} blob_cli_ack_recv_status;

typedef enum {
    TRANSFER_PHASE_INACTIVE                    = 0x00,
    TRANSFER_PHASE_WAITING_FOR_START           = 0x01,
    TRANSFER_PHASE_WAITING_FOR_NEXT_BLOCK      = 0x02,
    TRANSFER_PHASE_WAITING_FOR_NEXT_CHUNK      = 0x03,
    TRANSFER_PHASE_COMPLETE                    = 0x04,
    TRANSFER_PHASE_SUSPEND                     = 0x05,
    TRANSFER_PHASE_WAITING_CANCEL              = 0xfc,
    TRANSFER_PHASE_UNKNOW                      = 0xfe,
} transfer_phase_en;


typedef enum {
    TRANSFER_SET_SUCCESS                   = 0x00,
    TRANSFER_SET_SUB_INVALID_ADDRESS       = 0x01,
    TRANSFER_SET_SUB_INVALID_MODEL         = 0x02,
} transfer_set_status_en;


typedef enum {
    TRANSFER_STATUS_SUCCESS                   = 0x00,
    TRANSFER_STATUS_INVALID_BLOCK_NUMBER      = 0x01,
    TRANSFER_STATUS_INVALID_BLOCK_SIZE        = 0x02,
    TRANSFER_STATUS_INVALID_CHUNK_SIZE        = 0x03,
    TRANSFER_STATUS_WRONG_PHASE               = 0x04,
    TRANSFER_STATUS_INVALID_PARAMETER         = 0x05,
    TRANSFER_STATUS_WRONG_BLOB_ID             = 0x06,
    TRANSFER_STATUS_BLOB_TOO_LARGE            = 0x07,
    TRANSFER_STATUS_UNSUPPORTED_TRANSFER_MODE = 0x08,
    TRANSFER_STATUS_INTERNAL_ERROR            = 0x09,
    TRANSFER_STATUS_INFOMATION_UNAVAILABLE    = 0x0a,
    TRANSFER_STATUS_SET_ID_FAILD              = 0x0b,
    TRANSFER_STATUS_DEL_ID_FAILD              = 0x0c,
} transfer_status_en;

typedef enum {
    FORMAT_ALL_CHUNKS_MISSING     = 0x00,
    FORMAT_NO_CHUNKS_MISSING      = 0x01,
    FORMAT_SOME_CHUNKS_MISSING    = 0x02,
    FORMAT_ENCODED_MISSING_CHUNKS = 0x03,
} miss_chunks_indicate_format_en;


typedef struct {
    u64_t  blob_id;
    u16_t  block_size_log; //diffrent with spec
    u32_t block_total_size;
    u8_t  block_total;
    u32_t blob_size;
    u8_t  *data;
} blob_info;

typedef struct {
    u16_t block_num;
    u32_t block_size;
    u16_t chunk_size;
    u16_t total_chunk;
    u8_t  *data;
} blob_block_info;

typedef struct {
    u16_t     min_block_size_log;  //diffrent with spec
    u16_t     max_block_size_log; //diffrent with spec
    u16_t    max_total_chunks;
    u16_t    max_chunk_size;
    u32_t    max_blob_size;
    u16_t    server_mtu_size;
    transfer_mode_en support_mode;
} blob_capability;


typedef struct {
    u16_t multicast_addr;
    u32_t node_id;
	u8_t  ttl;
} __attribute__((packed))  blob_set_id_message;


typedef struct {
    u16_t multicast_addr;
} __attribute__((packed))  blob_del_id_message;

typedef struct {
    u8_t  transfer_mode:2,
          rfu:6;
    u64_t blob_id;
    u32_t blob_size;
    u16_t  block_size_log;  //TODO 7.3.1.2
    u16_t cli_mtu_size;
} __attribute__((packed))  blob_trans_start_message;


typedef struct {
    u16_t block_num;
    u16_t chunk_size;
} __attribute__((packed))  blob_block_start_message;

typedef struct {
    u16_t chunk_num;
    u8_t* chunk_data;
} __attribute__((packed))  blob_block_chunk_message;


#define OP_BLOB_TRANS_GET                                     BT_MESH_MODEL_OP_2(0x90, 0x01)
#define OP_BLOB_TRANS_START                                   BT_MESH_MODEL_OP_2(0x90, 0x02)
#define OP_BLOB_TRANS_CANCEL                                  BT_MESH_MODEL_OP_2(0x90, 0x03)
#define OP_BLOB_TRANS_STATUS                          	      BT_MESH_MODEL_OP_2(0x90, 0x04)
#define OP_BLOB_BLOCK_GET                          		      BT_MESH_MODEL_OP_2(0x90, 0x05)
#define OP_BLOB_BLOCK_START                        		      BT_MESH_MODEL_OP_2(0x90, 0x06)
#define OP_BLOB_BLOCK_STATUS                       		      BT_MESH_MODEL_OP_2(0x90, 0x07)
#define OP_BLOB_PARTIAL_BLOCK_REPORT                          BT_MESH_MODEL_OP_2(0x90, 0x08)
#define OP_BLOB_CHUNK_TRANS                          	      BT_MESH_MODEL_OP_2(0x90, 0x09)
#define OP_BLOB_INFO_GET                        		      BT_MESH_MODEL_OP_2(0x90, 0x0A)
#define OP_BLOB_INFO_STATUS                       		  	  BT_MESH_MODEL_OP_2(0x90, 0x0B)
#define OP_BLOB_TRANS_SET_ID                        		  BT_MESH_MODEL_OP_2(0x90, 0x0C)
#define OP_BLOB_TRANS_DEL_ID                        		  BT_MESH_MODEL_OP_2(0x90, 0x0D)
#define OP_BLOB_TRANS_SET_STATUS                       		  BT_MESH_MODEL_OP_2(0x90, 0x0E)



#define OP_BLOB_ALL_SRV_PROCEDURE_STATUS                      BT_MESH_MODEL_OP_2(0x90, 0x0F)


#define MIN_BLOB_TRANS_STATUS_LEN (2)
#define MIN_BLOCK_STATUS_LEN      (5)
#define MAX_BLOCK_STATUS_LEN      (5 + CONFIG_BLOB_SRV_MAX_MISS_CHUNKS_FLAG_SIZE)
#define MAX_BLOB_TRANS_STATUS_LEN (18 + CONFIG_BLOB_SRV_MAX_MISS_BLOCKS_FLAG_SIZE)
#define DEF_MAX_STATUS_GET_RETRY (1000)



#define LOG_SIZE(log)   (0x01 << log)
#define TRANS_STATUS_HDR(status, mode)      (status << 4 | (mode & 0x03))
#define BLOCK_STATUS_HDR(status, format)      (status << 4 | (format & 0x03))
#define UNKNOW_EXCEPTED_BLOB_ID                      (0xFFFFFFFFFFFFFFFF)
#define CLEAR_BIT(data,bit)    (data &= !(1 << bit))


#ifndef CONFIG_BLOB_ACK_CTRL
#define CONFIG_BLOB_ACK_CTRL (1)
#endif


static inline char* phase_str(transfer_phase_en phase)
{
    switch(phase) {
    case TRANSFER_PHASE_INACTIVE :
        return "INACTIVE";
    case TRANSFER_PHASE_WAITING_FOR_START:
        return "WAITING_FOR_START";
    case TRANSFER_PHASE_WAITING_FOR_NEXT_BLOCK :
        return "WAITING_FOR_NEXT_BLOCK";
    case TRANSFER_PHASE_WAITING_FOR_NEXT_CHUNK:
        return "WAITING_FOR_NEXT_CHUNK";
    case TRANSFER_PHASE_COMPLETE :
        return "COMPLETE";
    case TRANSFER_PHASE_SUSPEND :
        return "SUSPEND";
    case TRANSFER_PHASE_WAITING_CANCEL :
        return "WAITING_CANCEL";
    case TRANSFER_PHASE_UNKNOW:
        return "UNKNOW";
    default:
        return "UNKONW";
    }
}

static inline char* trans_status_str(transfer_status_en phase)
{
    switch(phase) {
    case TRANSFER_STATUS_SUCCESS:
        return "SUCCESS";
    case TRANSFER_STATUS_INVALID_BLOCK_NUMBER:
        return "INVALID_BLOCK_NUMBER";
    case TRANSFER_STATUS_INVALID_BLOCK_SIZE :
        return "INVALID_BLOCK_SIZE";
    case TRANSFER_STATUS_INVALID_CHUNK_SIZE:
        return "INVALID_CHUNK_SIZE";
    case TRANSFER_STATUS_WRONG_PHASE :
        return "WRONG_PHASE";
    case TRANSFER_STATUS_INVALID_PARAMETER :
        return "INVALID_PARAMETER";
    case TRANSFER_STATUS_WRONG_BLOB_ID :
        return "WRONG_BLOB_ID";
    case TRANSFER_STATUS_BLOB_TOO_LARGE:
        return "UNKNOW";
    case TRANSFER_STATUS_UNSUPPORTED_TRANSFER_MODE:
        return "UNSUPPORTED_TRANSFER_MODE";
    case TRANSFER_STATUS_INTERNAL_ERROR:
        return "INTERNAL_ERROR";
    case TRANSFER_STATUS_INFOMATION_UNAVAILABLE :
        return "INFOMATION_UNAVAILABLE";
    case TRANSFER_STATUS_SET_ID_FAILD:
        return "SET_ID_FAILD";
    default:
        return "UNKONW";
    }
}

#endif
