/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     softse_aes.c
 * @brief    aes
 * @version  V1.0
 * @date     21. May 2020
 * @model    sse
 ******************************************************************************/
#ifdef CONFIG_SEC_CRYPTO_AES_SW

#include <stdint.h>
#include <sec_crypto_aes.h>
#include <sec_crypto_errcode.h>

#include "crypto_aes.h"
#include "sec_crypto_common.h"

// Function documentation

/**
  \brief       Initialize AES Interface. Initializes the resources needed for the AES interface
  \param[in]   aes    operate handle
  \param[in]   idx    device id
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_init(sc_aes_t *aes, uint32_t idx)
{
    SC_lOG("===%s, %d\n", __FUNCTION__, __LINE__);
    CHECK_PARAM(aes, SC_PARAM_INV);
    sc_mbedtls_aes_init(&aes->aes_ctx);

    return SC_OK;
}
/**
  \brief       De-initialize AES Interface. stops operation and releases the software resources used by the interface
  \param[in]   aes    aes handle to operate
  \return      None
*/
void sc_aes_uninit(sc_aes_t *aes)
{
    if (aes) {
        sc_mbedtls_aes_free(&aes->aes_ctx);
    }
}
/**
  \brief       Set encrypt key
  \param[in]   aes        aes handle to operate
  \param[in]   key        Pointer to the key buf
  \param[in]   key_len    Pointer to \ref sc_aes_key_bits_t
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_set_encrypt_key(sc_aes_t *aes, void *key, sc_aes_key_bits_t key_len)
{
    uint32_t to_bit_len[] = {16 * 8, 24 * 8, 32 * 8};
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(key, SC_PARAM_INV);

    sc_mbedtls_aes_setkey_enc(&aes->aes_ctx, key, to_bit_len[key_len]);
    return SC_OK;
}
/**
  \brief       Set decrypt key
  \param[in]   aes        aes handle to operate
  \param[in]   key        Pointer to the key buf
  \param[in]   key_len    Pointer to \ref sc_aes_key_bits_t
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_set_decrypt_key(sc_aes_t *aes, void *key, sc_aes_key_bits_t key_len)
{
    uint32_t to_bit_len[] = {16 * 8, 24 * 8, 32 * 8};
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(key, SC_PARAM_INV);
    sc_mbedtls_aes_setkey_dec(&aes->aes_ctx, key, to_bit_len[key_len]);
    return SC_OK;
}

/**
  \brief       Aes ecb encrypt or decrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_ecb_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size)
{
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(in, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);
    uint32_t ret;

    if (size % 16) {
        return SC_PARAM_INV;
    }
    uint8_t *pin  = (uint8_t *)in;
    uint8_t *pout = (uint8_t *)out;
    while (size > 0) {
        ret = sc_mbedtls_aes_crypt_ecb(&aes->aes_ctx, MBEDTLS_AES_ENCRYPT, pin, pout);
        if (ret != SC_OK) {
            return ret;
        }
        pin += 16;
        pout += 16;
        size -= 16;
    }
    return ret;
}
/**
  \brief       Aes ecb decrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_ecb_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size)
{
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(in, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);
    uint32_t ret;

    if (size % 16) {
        return SC_PARAM_INV;
    }
    uint8_t *pin  = (uint8_t *)in;
    uint8_t *pout = (uint8_t *)out;
    while (size > 0) {
        ret = sc_mbedtls_aes_crypt_ecb(&aes->aes_ctx, MBEDTLS_AES_DECRYPT, pin, pout);
        if (ret != SC_OK) {
            return ret;
        }
        pin += 16;
        pout += 16;
        size -= 16;
    }
    return ret;
}
/**
  \brief       Aes cbc encrypt or decrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_cbc_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv)
{
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(in, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);
    CHECK_PARAM(iv, SC_PARAM_INV);
    uint8_t temp[16];

    if (size % 16) {
        return SC_PARAM_INV;
    }
    /* NOTE modify iv in sc_mbedtls_aes_crypt_cbc */
    memcpy(temp, iv, 16);

    return sc_mbedtls_aes_crypt_cbc(&aes->aes_ctx, MBEDTLS_AES_ENCRYPT, size, temp, in, out);
}
/**
  \brief       Aes cbc decrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \param[in]   iv      init vector
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_cbc_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv)
{
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(in, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);
    CHECK_PARAM(iv, SC_PARAM_INV);
    uint8_t temp[16];

    if (size % 16) {
        return SC_PARAM_INV;
    }
    /* NOTE modify iv in sc_mbedtls_aes_crypt_cbc */
    memcpy(temp, iv, 16);

    return sc_mbedtls_aes_crypt_cbc(&aes->aes_ctx, MBEDTLS_AES_DECRYPT, size, temp, in, out);
}

/**
  \brief       Aes cfb1 encrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \param[in]   iv      init vector
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_cfb1_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes cfb1 decrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \param[in]   iv      init vector
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_cfb1_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void*iv)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes cfb8 encrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \param[in]   iv      init vector
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_cfb8_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes cfb8 decrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \param[in]   iv      init vector
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_cfb8_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes cfb128 decrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \param[in]   iv      init vector
  \param[out]  num     the number of the 128-bit block we have used
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_cfb128_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv,
                               uint32_t *num)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes cfb128 encrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \param[in]   iv      init vector
  \param[out]  num     the number of the 128-bit block we have used
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_cfb128_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv,
                               uint32_t *num)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes ofb encrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \param[in]   iv      init vector
  \param[out]  num     the number of the 128-bit block we have used
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_ofb_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv,
                            uint32_t *num)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes ofb decrypt
  \param[in]   aes     aes handle to operate
  \param[in]   in      Pointer to the Source data
  \param[out]  out     Pointer to the Result data
  \param[in]   size    the Source data size
  \param[in]   iv      init vector
  \param[out]  num     the number of the 128-bit block we have used
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_ofb_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv,
                            uint32_t *num)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes ctr encrypt
  \param[in]   aes              aes handle to operate
  \param[in]   in               Pointer to the Source data
  \param[out]  out              Pointer to the Result data
  \param[in]   size             the Source data size
  \param[in]   nonce_counter    Pointer to the 128-bit nonce and counter
  \param[in]   stream_block     Pointer to the saved stream-block for resuming
  \param[in]   iv               init vector
  \param[out]  num              the number of the 128-bit block we have used
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_ctr_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size,
                            uint8_t nonce_counter[16], uint8_t stream_block[16], void *iv,
                            uint32_t *num)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       Aes ctr decrypt
  \param[in]   aes              aes handle to operate
  \param[in]   in               Pointer to the Source data
  \param[out]  out              Pointer to the Result data
  \param[in]   size             the Source data size
  \param[in]   nonce_counter    Pointer to the 128-bit nonce and counter
  \param[in]   stream_block     Pointer to the saved stream-block for resuming
  \param[in]   iv               init vecotr
  \param[out]  num              the number of the 128-bit block we have used
  \return      error code \ref uint32_t
*/
uint32_t sc_aes_ctr_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size,
                            uint8_t nonce_counter[16], uint8_t stream_block[16], void *iv,
                            uint32_t *num)
{
    return SC_NOT_SUPPORT;
}

#endif
