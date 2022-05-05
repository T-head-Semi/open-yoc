/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "bz_sha256.h"

#include "auth.h"
#include "core.h"
#include "bz_utils.h"
#include "breeze_hal_ble.h"

extern core_t g_core;
auth_t g_auth;

bool g_dn_complete = false;

static void on_timeout(void *arg1, void *arg2)
{
    core_handle_err(ALI_ERROR_SRC_AUTH_PROC_TIMER_2, BZ_ETIMEOUT);
}

static void update_aes_key(bool use_device_key)
{
    SHA256_CTX context;
    uint8_t okm[SHA256_BLOCK_SIZE];

    bz_sha256_init(&context);
    sha256_update(&context, g_auth.ikm, g_auth.ikm_len);
    sha256_final(&context, okm);
    memcpy(g_auth.okm, okm, IOTB_MAX_OKM_LEN);
}

static void update_trans_key(void)
{
    // notify key updated
    transport_update_key(g_auth.okm);
}

ret_code_t auth_init(ali_init_t const *p_init, tx_func_t tx_func)
{
    int len;
    ret_code_t ret = BZ_SUCCESS;

    memset(&g_auth, 0, sizeof(auth_t));
    g_auth.state = BZ_AUTH_STATE_IDLE;
    g_auth.tx_func = tx_func;
#ifdef EN_AUTH_OFFLINE
    g_auth.offline_auth = false;
#endif

#ifdef EN_AUTH_OFFLINE
    auth_keys_init();
#endif
    ret = os_timer_new(&g_auth.timer, on_timeout, &g_auth, BZ_AUTH_TIMEOUT);
    return ret;
}

void auth_reset(void)
{
    g_auth.state = BZ_AUTH_STATE_IDLE;
    os_timer_stop(&g_auth.timer);
}

void auth_rx_command(uint8_t cmd, uint8_t *p_data, uint16_t length)
{
    uint32_t err_code;
#ifdef EN_AUTH_OFFLINE
    uint8_t rekey_rsp[] = {0x01, 0x02};
    uint8_t authkey[AUTH_KEY_LEN]={0};
    uint8_t authid[AUTH_ID_LEN] = {0};
#endif

    if (length == 0 || (cmd & BZ_CMD_TYPE_MASK) != BZ_CMD_TYPE_AUTH) {
        return;
    }

    switch (g_auth.state) {
        case BZ_AUTH_STATE_SVC_ENABLED:
#ifdef EN_AUTH_OFFLINE
            if(cmd == BZ_CMD_AUTH_REKEY){/*check whether auth-key according to auth-id*/
                 if(length != AUTH_ID_LEN) {
                      err_code = g_auth.tx_func(BZ_CMD_AUTH_REKEY_RSP, &rekey_rsp[1] , 1);
                      BREEZE_ERR("[BZ auth]: rx offline key len mismatch(%d)", err_code);
                      return;
                }
                memcpy(authid, p_data, AUTH_ID_LEN);
                if(authkey_get(authid, authkey) == true){
                    //("[BZ auth]: rx offline key\n");
                    g_dn_complete = true;
                    g_auth.state = IOTB_AUTH_STATE_RECV_RAND;
                    g_auth.offline_auth = true;
                    
                    /*1.update current auth key*/
                    transport_update_key(authkey);
                    /*2.response with "HI Client"*/
                    err_code = g_auth.tx_func(BZ_CMD_AUTH_RSP, BZ_HI_CLIENT_STR, strlen(BZ_HI_CLIENT_STR));
                    BREEZE_DEBUG("[BZ auth]: tx (%s) (%s)", BZ_HI_CLIENT_STR, err_code ? "fail": "success");
                    if (err_code != BZ_SUCCESS) {
                        core_handle_err(IOTB_ERROR_AUTH_CIPHER_RPT, err_code);
                        return;
                    }
                    err_code = bz_os_timer_start(&g_auth.timer);
                    if (err_code != BZ_SUCCESS) {
                        core_handle_err(ALI_ERROR_SRC_AUTH_PROC_TIMER_1, err_code);
                        return;
                    }
                 } else{
                    err_code = g_auth.tx_func(BZ_CMD_AUTH_REKEY_RSP, &rekey_rsp[0] , 1);
                    if (err_code != BZ_SUCCESS) {
                        core_handle_err(IOTB_ERROR_AUTH_CIPHER_RPT, err_code);
                        return;
                    }
                    /*back to re-auth state*/
                    os_timer_stop(&g_auth.timer);
                    auth_connected();
                 }
            } else
#endif
            if (cmd == BZ_CMD_AUTH_SET_RAND) {
                uint8_t str_pid[IOTB_PID_STR_LEN];
                uint8_t m_pid[4];
                if (length == IOTB_RANDOM_SEQ_LEN) {
                    memcpy(g_auth.ikm + g_auth.ikm_len, p_data, length);
                    g_auth.ikm_len += length;
                    BREEZE_DEBUG("[BZ auth]: rx rand");
                    hex_byte_dump_verbose(p_data, length, 24);
                } else {
                    BREEZE_ERR("[BZ auth]: rand len err");
                    // TODO: should err process
                    return;
                }
                g_dn_complete = true;
                g_auth.state = IOTB_AUTH_STATE_RECV_RAND;

                g_auth.ikm[g_auth.ikm_len++] = ',';
                SET_U32_BE(m_pid, g_core.product_id);
                hex2string(m_pid, sizeof(m_pid), str_pid);
                
                memcpy(g_auth.ikm + g_auth.ikm_len, str_pid, IOTB_PID_STR_LEN);
                g_auth.ikm_len += IOTB_PID_STR_LEN;
                g_auth.ikm[g_auth.ikm_len++] = ',';
                
                memcpy(g_auth.ikm + g_auth.ikm_len, g_core.device_name, g_core.device_name_len);
                g_auth.ikm_len += g_core.device_name_len;
                g_auth.ikm[g_auth.ikm_len++] = ',';
                
                memcpy(g_auth.ikm + g_auth.ikm_len, g_core.device_secret, g_core.device_secret_len);
                g_auth.ikm_len += g_core.device_secret_len;

                BREEZE_DEBUG("[BZ auth]: calc ble-key");
                BREEZE_VERBOSE("ikm(%.*s)", g_auth.ikm_len, g_auth.ikm);
                update_aes_key(true);
                update_trans_key();

                err_code = g_auth.tx_func(BZ_CMD_AUTH_RPT_CIPHER, p_data, length);
                BREEZE_DEBUG("[BZ auth]: tx cipher (%s)", err_code ? "fail": "success");
                if (err_code != BZ_SUCCESS) {
                    core_handle_err(IOTB_ERROR_AUTH_CIPHER_RPT, err_code);
                    return;
                }

                err_code = bz_os_timer_start(&g_auth.timer);
                if (err_code != BZ_SUCCESS) {
                    core_handle_err(ALI_ERROR_SRC_AUTH_PROC_TIMER_1, err_code);
                    return;
                }
            } else {
                g_auth.state = BZ_AUTH_STATE_FAILED;
            }
            break;

        case IOTB_AUTH_STATE_RECV_RAND:
            if (cmd == BZ_CMD_AUTH_RESULT_IND) {
                uint8_t m_auth_result = IOTB_AUTH_FAIL;
                if (length == IOTB_AUTH_RESULT_LEN) {
                    m_auth_result = p_data[0];
                    BREEZE_DEBUG("[BZ auth]: rx result (%s)", m_auth_result ? "fail" : "success");
                    if (m_auth_result == IOTB_AUTH_SUCCESS) {
                        BREEZE_DEBUG("[BZ auth]: update ble-key");
                        //update_trans_key();
                    }
                    err_code = g_auth.tx_func(BZ_CMD_AUTH_RESULT_CFM, &m_auth_result, IOTB_AUTH_RESULT_LEN);
                    BREEZE_DEBUG("[BZ auth]: tx result confirm (%s)", m_auth_result ? "fail": "success");
                    if (err_code != BZ_SUCCESS) {
                        core_handle_err(ALI_ERROR_SRC_AUTH_SEND_KEY, err_code);
                        return;
                    }
                } else {
                    BREEZE_ERR("[BZ auth]: rx invalid result");
                    // TODO: should err process
                    return;
                }
                if (m_auth_result == IOTB_AUTH_SUCCESS) {
                    g_auth.state = IOTB_AUTH_STATE_DONE;
                } else {
                    g_auth.state = BZ_AUTH_STATE_FAILED;
                }
            } else {
                g_auth.state = BZ_AUTH_STATE_FAILED;
            }
            break;

        default:
            err_code = g_auth.tx_func(BZ_CMD_ERR, NULL, 0);
            if (err_code != BZ_SUCCESS) {
                core_handle_err(ALI_ERROR_SRC_AUTH_SEND_ERROR, err_code);
                return;
            }
            break;
    }

    if (g_auth.state == IOTB_AUTH_STATE_DONE) {
#ifdef EN_AUTH_OFFLINE
        if(g_auth.offline_auth == false){
            store_auth_key();
        } else{
            g_auth.offline_auth = false;
        }
#endif
        core_event_notify(BZ_EVENT_AUTHENTICATED, NULL, 0);
        os_timer_stop(&g_auth.timer);
    } else if (g_auth.state == BZ_AUTH_STATE_FAILED) {
        os_timer_stop(&g_auth.timer);
        ble_disconnect(AIS_BT_REASON_REMOTE_USER_TERM_CONN);
    }
}
void auth_connected(void)
{
    uint32_t err_code;

    err_code = bz_os_timer_start(&g_auth.timer);
    if (err_code != BZ_SUCCESS) {
        core_handle_err(ALI_ERROR_SRC_AUTH_PROC_TIMER_0, err_code);
        return;
    }

    g_auth.state = BZ_AUTH_STATE_CONNECTED;
}

void auth_service_enabled(void)
{
    g_auth.state = BZ_AUTH_STATE_SVC_ENABLED;
    memset(g_auth.ikm, 0, sizeof(g_auth.ikm));
    g_auth.ikm_len = 0;
    BREEZE_DEBUG("[BZ auth]: start");
}

void auth_tx_done(void)
{
    uint32_t err_code;
}

bool auth_is_authdone(void)
{
    return (bool)(g_auth.state == IOTB_AUTH_STATE_DONE);
}

ret_code_t auth_get_device_name(uint8_t **pp_device_name, uint8_t *p_length)
{
    if(g_core.device_name_len){
        *pp_device_name = g_core.device_name;
        *p_length = g_core.device_name_len;
        return BZ_SUCCESS;
    } else{
        return BZ_EDATASIZE;
    }
}

ret_code_t auth_get_product_key(uint8_t **pp_prod_key, uint8_t *p_length)
{
    *pp_prod_key = g_core.product_key;
    *p_length = BZ_DEV_PRODUCT_KEY_LEN;
    return BZ_SUCCESS;
}

#ifdef EN_AUTH_OFFLINE
void auth_keys_init(void)
{
    auth_key_storage_t auth_keys;
    int len = sizeof(auth_keys);
    memset(&auth_keys, 0, len);
    if (os_kv_get(AUTH_KEY_KV_PREFIX, &auth_keys, &len) != 0){
        if(os_kv_set(AUTH_KEY_KV_PREFIX, &auth_keys, len, 1) != 0){
            BREEZE_ERR("[BZ auth]: init keys");
            return;
        }
    }
}

bool authkey_set(uint8_t* authid, uint8_t* authkey)
{
    int32_t ret;
    auth_key_storage_t auth_keys;
    int len = sizeof(auth_keys);
    if(authid == NULL || authkey == NULL){
        return false;
    }
    memset(&auth_keys, 0, len);
    if (os_kv_get(AUTH_KEY_KV_PREFIX, &auth_keys, &len) == 0){
        uint32_t index = auth_keys.index_to_update;
        memcpy(auth_keys.kv_pairs[index].auth_id, authid, AUTH_ID_LEN);
        memcpy(auth_keys.kv_pairs[index].auth_key, authkey, AUTH_KEY_LEN);
        auth_keys.index_to_update ++;
        if(index == MAX_AUTH_KEYS -1){
           auth_keys.index_to_update = 0;
        }
        ret = os_kv_set(AUTH_KEY_KV_PREFIX, &auth_keys, len, 1);
        BREEZE_ERR("[BZ auth]: keys KV set (%s)", ret ? "fail": "success");
        return true;
    }
    return false;
}

bool authkey_get(uint8_t* authid, uint8_t* authkey)
{
    int32_t ret;
    uint32_t index = 0;
    auth_key_storage_t auth_keys;
    int len = sizeof(auth_keys);
    if(authid == NULL || authkey == NULL){
        return false;
    }
    if (os_kv_get(AUTH_KEY_KV_PREFIX, &auth_keys, &len) == 0){
        for(; index < MAX_AUTH_KEYS; ++index){
             if(!memcmp(auth_keys.kv_pairs[index].auth_id, authid, AUTH_ID_LEN)){
                memcpy(authkey, auth_keys.kv_pairs[index].auth_key, AUTH_KEY_LEN);
                return true;
             }
        }
        if(index == MAX_AUTH_KEYS){
            return false;
        }
    }
    return false;
}

void store_auth_key(void)
{
    uint8_t rand_backup[IOTB_RANDOM_SEQ_LEN];
    SHA256_CTX context;
    uint8_t auth_id[AUTH_ID_LEN];
    uint8_t okm[SHA256_BLOCK_SIZE];
    memcpy(rand_backup, g_auth.ikm + g_auth.ikm_len, IOTB_RANDOM_SEQ_LEN);
    bz_sha256_init(&context);
    sha256_update(&context, rand_backup, sizeof(rand_backup));
    sha256_final(&context, okm);
    memcpy(auth_id, okm, AUTH_ID_LEN);
    authkey_set(auth_id, g_auth.okm);
}

#endif
