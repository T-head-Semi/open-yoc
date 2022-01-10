/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include "sec_crypto_platform.h"
#include <sec_crypto_auth.h>
#include <stdio.h>
#include "string.h"
#include "crypto_md.h"
#include "key_mgr.h"
#include "sec_crypto_aes.h"
#include "crypto_sha1.h"

#define CHECK_RETURN(ret)                                                      \
        do {                                                                   \
                if (ret != 0) {                                                \
                        while (1)                                              \
                                ;                                              \
                        return -1;                                             \
                }                                                              \
        } while (0);

int sc_srv_authenticate(uint8_t *authinfo, int authinfo_size)
{

        return 0;
}

int sc_srv_get_message_auth(message_auth_t *message, uint32_t *authcode)
{
        int           ret;
        key_handle    key;
        uint32_t      key_len;
        unsigned char hmac[32];

        sc_mbedtls_md_context_t     ctx;
        const sc_mbedtls_md_info_t *info;

        ret = km_get_key(KEY_ID_MESSAGE_AUTH, &key, &key_len);

        if (ret != KM_OK) {
                goto exit;
        }

        /* 1. init mbedtls_md_context_t structure */
        sc_mbedtls_md_init(&ctx);

        /* 2. get md info structure pointer */
        info = sc_mbedtls_md_info_from_type(SC_MBEDTLS_MD_SHA1);

        /* 3. setup md info structure */
        ret = sc_mbedtls_md_setup(&ctx, info, 1);
        if (ret != 0) {
                goto exit;
        }

        /* 4. start */
        ret = sc_mbedtls_md_hmac_starts(&ctx, (unsigned char *)key, key_len);
        if (ret != 0) {
                goto exit;
        }
        /* 5. update */
        ret = sc_mbedtls_md_hmac_update(&ctx, (unsigned char *)message,
                                        sizeof(message_auth_t));
        if (ret != 0) {
                goto exit;
        }
        /* 6. finish */
        ret = sc_mbedtls_md_hmac_finish(&ctx, hmac);
        if (ret != 0) {
                goto exit;
        }

        /* show */
        sc_mbedtls_md_get_size(info);

        *authcode = *(uint32_t *)hmac;

exit:
        /* 7. free */
        sc_mbedtls_md_free(&ctx);

        return ret;
}

int key_derivation(uint8_t *salt, uint32_t salt_len, void *key_out,
                   uint32_t *key_olen)
{
        uint8_t    sha1sum[32] = {0};
        sc_aes_t   aes_hdl;
        key_handle key_input;
        uint32_t   key_ilen;

        int ret = km_get_key(KEY_ID_MESSAGE_AUTH, &key_input, &key_ilen);

        if (ret != KM_OK) {
                return -1;
        }

        memset(sha1sum, 0, 32);
        sc_mbedtls_sha1((const unsigned char *)key_input, key_ilen, sha1sum);

        ret = sc_aes_init(&aes_hdl, 0);
        CHECK_RETURN(ret);

        ret = sc_aes_set_encrypt_key(&aes_hdl, (void *)sha1sum,
                                     SC_AES_KEY_LEN_BITS_128);
        CHECK_RETURN(ret);

        ret = sc_aes_ecb_encrypt(&aes_hdl, (void *)salt, (void *)key_out,
                                 salt_len);
        CHECK_RETURN(ret);

        sc_aes_uninit(&aes_hdl);

        *key_olen = salt_len;

        return 0;
}

int sc_srv_sensitive_data_encrypto(uint8_t *input, uint8_t *output,
                                   uint32_t size)
{
        sc_aes_t aes_hdl;
        uint32_t key_olen;
        uint8_t  key_der[32];
        uint8_t  salt[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                          0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};

        int ret = key_derivation(salt, sizeof(salt), key_der, &key_olen);
        CHECK_RETURN(ret);

        ret = sc_aes_init(&aes_hdl, 0);
        CHECK_RETURN(ret);

        ret = sc_aes_set_encrypt_key(&aes_hdl, (void *)key_der,
                                     SC_AES_KEY_LEN_BITS_128);
        CHECK_RETURN(ret);

        ret = sc_aes_ecb_encrypt(&aes_hdl, (void *)input, (void *)output, size);
        CHECK_RETURN(ret);

        sc_aes_uninit(&aes_hdl);

        return 0;
}

int sc_srv_sensitive_data_decrypto(uint8_t *input, uint8_t *output,
                                   uint32_t size)
{
        sc_aes_t aes_hdl;
        uint8_t  key_der[32];
        uint32_t key_olen;
        uint8_t  salt[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                          0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};

        int ret = key_derivation(salt, sizeof(salt), key_der, &key_olen);
        CHECK_RETURN(ret);

        ret = sc_aes_init(&aes_hdl, 0);
        CHECK_RETURN(ret);

        ret = sc_aes_set_decrypt_key(&aes_hdl, (void *)key_der,
                                     SC_AES_KEY_LEN_BITS_128);
        CHECK_RETURN(ret);

        ret = sc_aes_ecb_decrypt(&aes_hdl, (void *)input, (void *)output, size);
        CHECK_RETURN(ret);

        sc_aes_uninit(&aes_hdl);

        return 0;
}

int sc_srv_get_authmsg(uint8_t *cid, uint32_t cid_len, uint8_t *tstmp,
                       uint32_t tstmp_len, uint8_t *authmsg)
{
        int        ret;
        key_handle key;
        uint32_t   key_len;

        sc_mbedtls_md_context_t     ctx;
        const sc_mbedtls_md_info_t *info;

        ret = km_get_key(KEY_ID_MESSAGE_AUTH, &key, &key_len);
        if (ret != KM_OK) {
                goto exit;
        }

        sc_mbedtls_md_init(&ctx);
        info = sc_mbedtls_md_info_from_type(SC_MBEDTLS_MD_SHA1);
        ret  = sc_mbedtls_md_setup(&ctx, info, 1);
        if (ret != 0) {
                goto exit;
        }
        ret = sc_mbedtls_md_hmac_starts(&ctx, (unsigned char *)key, key_len);
        if (ret != 0) {
                goto exit;
        }
        ret = sc_mbedtls_md_hmac_update(&ctx, (unsigned char *)cid, cid_len);
        if (ret != 0) {
                goto exit;
        }
        ret = sc_mbedtls_md_hmac_update(&ctx, tstmp, tstmp_len);
        if (ret != 0) {
                goto exit;
        }
        ret = sc_mbedtls_md_hmac_finish(&ctx, authmsg);
        if (ret != 0) {
                goto exit;
        }
        sc_mbedtls_md_get_size(info);

exit:
        sc_mbedtls_md_free(&ctx);

        return ret;
}