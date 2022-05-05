/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef BZ_AUTH_H
#define BZ_AUTH_H

#include <stdint.h>
#include "common.h"
#include "breeze_hal_os.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define IOTB_AUTH_SUCCESS       (0x00)
#define IOTB_AUTH_FAIL          (0x01)
#define IOTB_RANDOM_SEQ_LEN     16
#define IOTB_AUTH_RESULT_LEN    1
#define IOTB_PID_STR_LEN        8
#define IOTB_MAC_STR_LEN        12
#define IOTB_MAX_OKM_LEN        16
#define IOTB_MAX_IKM_LEN        (IOTB_RANDOM_SEQ_LEN + IOTB_PID_STR_LEN + IOTB_MAC_STR_LEN + IOTB_DEVICE_SECRET_LEN + 4)

enum {
    BZ_AUTH_STATE_IDLE,                                // Auth idle state
    BZ_AUTH_STATE_CONNECTED,                           // BLE link connected
    BZ_AUTH_STATE_SVC_ENABLED,                         // AIS service enabled
    IOTB_AUTH_STATE_RECV_RAND,                         // Auth rx random from peer
    //IOTB_AUTH_STATE_RECV_RESULT,                       // Auth rx auth result from peer
    IOTB_AUTH_STATE_DONE,                              // Auth finished
    //IOTB_AUTH_STATE_RECV_BIND,                         // Auth rx bind result from peer, need do later
    BZ_AUTH_STATE_FAILED,                              // Auth failed
};

enum {
    BZ_AUTH_TYPE_NONE,
    BZ_AUTH_TYPE_PER_PK,
    BZ_AUTH_TYPE_PER_DEV,
};

typedef struct auth_s {
    uint8_t state;                                     // Auth state
    tx_func_t tx_func;                                 // Auth data send, use indication
    os_timer_t timer;                                  // Auth timeout timer, start from send random
    uint8_t ikm[IOTB_MAX_IKM_LEN];                     // Auth sign calc input buffer
    uint16_t ikm_len;                                  // Auth sign calc input buffer length
    uint8_t okm[IOTB_MAX_OKM_LEN];                     // Auth key output
    uint8_t *p_product_key;
    uint8_t product_key_len;
    uint8_t *p_device_name;
    uint8_t device_name_len;
    uint8_t device_secret[IOTB_DEVICE_SECRET_LEN];
    uint8_t device_secret_len;
    bool offline_auth;
    uint8_t auth_type;
    uint8_t check_hi_server_cnt; // Check "Hi,Server" message.
} auth_t;


#ifdef EN_AUTH_OFFLINE
#define MAX_AUTH_KEYS   5
#define AUTH_ID_LEN     8
#define AUTH_KEY_LEN    16
struct auth_key_pair{
    uint8_t auth_id[AUTH_ID_LEN];
    uint8_t auth_key[AUTH_KEY_LEN];
};
typedef struct auth_key_storage_s{
    uint8_t index_to_update;
    struct auth_key_pair kv_pairs[MAX_AUTH_KEYS];
} auth_key_storage_t;
#endif

ret_code_t auth_init(ali_init_t const *p_init, tx_func_t tx_func);
void auth_reset(void);
void auth_rx_command(uint8_t cmd, uint8_t *p_data, uint16_t length);
void auth_connected(void);
void auth_service_enabled(void);
void auth_tx_done(void);
bool auth_is_authdone(void);

ret_code_t auth_get_device_name(uint8_t **pp_device_name, uint8_t *p_length);
ret_code_t auth_get_product_key(uint8_t **pp_prod_key, uint8_t *p_length);
int auth_calc_adv_sign(uint32_t seq, uint8_t *sign);

#ifdef EN_AUTH_OFFLINE
void auth_keys_init(void);
bool authkey_set(uint8_t* authid, uint8_t* authkey);
bool authkey_get(uint8_t* authid, uint8_t* authkey);
#endif

#ifdef __cplusplus
}
#endif

#endif // BZ_AUTH_H
