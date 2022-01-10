/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     tee_rsa.c
 * @brief    rsa
 * @version  V1.0
 * @date     21. May 2020
 * @model    sse
 ******************************************************************************/
#ifndef CONFIG_SEC_CRYPTO_RSA_SW
#include <stdint.h>
#include <stdbool.h>
#include <sec_crypto_rsa.h>
#include <sec_crypto_errcode.h>

#include "drv/tee.h"
#include "sec_crypto_common.h"

// Function documentation

/**
  \brief       Initialize RSA Interface. 1. Initializes the resources needed for the RSA interface 2.registers event callback function
  \param[in]   rsa  rsa handle to operate.
  \param[in]   idx  device id
  \return      \ref uint32_t
*/
uint32_t sc_rsa_init(sc_rsa_t *rsa, uint32_t idx, sc_rsa_key_bits_t data_bits)
{
    CHECK_PARAM(rsa, SC_PARAM_INV);
    CHECK_PARAM(((data_bits == SC_RSA_KEY_BITS_1024) ||
            (data_bits == SC_RSA_KEY_BITS_2048) || (data_bits == 0)), SC_PARAM_INV);
    (void)idx;
    rsa->bits = data_bits;
    return SC_OK;
}

/**
  \brief       De-initialize RSA Interface. stops operation and releases the software resources used by the interface
  \param[in]   rsa  rsa handle to operate.
  \return      none
*/
void sc_rsa_uninit(sc_rsa_t *rsa)
{
}

/**
  \brief       attach the callback handler to RSA
  \param[in]   rsa  operate handle.
  \param[in]   cb    callback function
  \param[in]   arg   user can define it by himself as callback's param
  \return      error code
*/
uint32_t sc_rsa_attach_callback(sc_rsa_t *rsa, sc_rsa_callback_t cb, void *arg)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       detach the callback handler
  \param[in]   rsa  operate handle.
*/
void sc_rsa_detach_callback(sc_rsa_t *rsa)
{
}

/**
  \brief       generate rsa key pair.
  \param[in]   rsa       rsa handle to operate.
  \param[out]  context   Pointer to the rsa context
  \return      \ref uint32_t
*/
uint32_t sc_rsa_gen_key(sc_rsa_t *rsa, sc_rsa_context_t *context)
{
    return SC_NOT_SUPPORT;
}

int rsa_encrypt_asy(sc_rsa_t *rsa, uint8_t *n, uint8_t *e, uint8_t *src, uint32_t src_size,
                    uint8_t *out, uint32_t padding, uint32_t keybits_len)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       encrypt
  \param[in]   rsa       rsa handle to operate.
  \param[in]   context   Pointer to the rsa context
  \param[in]   src       Pointer to the source data.
  \param[in]   src_size  the source data len
  \param[out]  out       Pointer to the result buffer
  \return      \ref uint32_t
*/

uint32_t sc_rsa_encrypt(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src, uint32_t src_size,
                        void *out)
{
    uint32_t len;
    CHECK_PARAM(rsa, SC_PARAM_INV);
    CHECK_PARAM(context, SC_PARAM_INV);
    CHECK_PARAM(src, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);

    if (context->padding_type != SC_RSA_PADDING_MODE_PKCS1) {
        return SC_PARAM_INV;
    }
    sc_common_set_key_bits(rsa, context);
    CHECK_PARAM(context->key_bits != 0, SC_PARAM_INV);

    return csi_tee_rsa_encrypt(src, src_size, (const uint8_t *)context->n,
                               (context->key_bits / 8) * 2, out, &len, TEE_RSA_PKCS1_PADDING);
}

/**
  \brief       decrypt
  \param[in]   rsa       rsa handle to operate.
  \param[in]   context   Pointer to the rsa context
  \param[in]   src       Pointer to the source data.
  \param[in]   src_size  the source data len
  \param[out]  out       Pointer to the result buffer
  \param[out]  out_size  the result size
  \return      \ref uint32_t
*/
uint32_t sc_rsa_decrypt(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src, uint32_t src_size,
                        void *out, uint32_t *out_size)
{
    CHECK_PARAM(rsa, SC_PARAM_INV);
    CHECK_PARAM(context, SC_PARAM_INV);
    CHECK_PARAM(src, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);
    CHECK_PARAM(out_size, SC_PARAM_INV);
    if (context->padding_type != SC_RSA_PADDING_MODE_PKCS1) {
        return SC_PARAM_INV;
    }
    sc_common_set_key_bits(rsa, context);
    CHECK_PARAM(context->key_bits != 0, SC_PARAM_INV);

    return csi_tee_rsa_decrypt(src, src_size, (const uint8_t *)context->n,
                               (context->key_bits / 8) * 3, out, out_size, TEE_RSA_PKCS1_PADDING);
}

/**
  \brief       rsa sign
  \param[in]   rsa       rsa handle to operate.
  \param[in]   context   Pointer to the rsa context
  \param[in]   src       Pointer to the source data.
  \param[in]   src_size  the source data len
  \param[out]  signature Pointer to the signature
  \param[in]   hash_type the source data hash type
  \return      \ref uint32_t
*/
uint32_t sc_rsa_sign(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src, uint32_t src_size,
                     void *signature, sc_rsa_hash_type_t hash_type)
{
    uint32_t            len;
    tee_rsa_sign_type_e types[] = {TEE_RSA_MD5,    TEE_RSA_SHA1,          TEE_RSA_SIGN_TYPE_MAX,
                                   TEE_RSA_SHA256, TEE_RSA_SIGN_TYPE_MAX, TEE_RSA_SIGN_TYPE_MAX};

    CHECK_PARAM(rsa, SC_PARAM_INV);
    CHECK_PARAM(context, SC_PARAM_INV);
    CHECK_PARAM(src, SC_PARAM_INV);
    CHECK_PARAM(signature, SC_PARAM_INV);
    /* src:degist */
    sc_common_set_key_bits(rsa, context);
    CHECK_PARAM(context->key_bits != 0, SC_PARAM_INV);
    return csi_tee_rsa_sign(src, src_size, (const uint8_t *)context->n, (context->key_bits / 8) * 3,
                            signature, &len, types[hash_type]);
}

/**
  \brief       rsa verify
  \param[in]   rsa       rsa handle to operate.
  \param[in]   context   Pointer to the rsa context
  \param[in]   src       Pointer to the source data.
  \param[in]   src_size  the source data len
  \param[in]   signature Pointer to the signature
  \param[in]   sig_size  the signature size
  \param[in]   hash_type the source data hash type
  \return      verify result
*/
bool sc_rsa_verify(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src, uint32_t src_size,
                   void *signature, uint32_t sig_size, sc_rsa_hash_type_t hash_type)
{
    tee_rsa_sign_type_e types[] = {TEE_RSA_MD5,    TEE_RSA_SHA1,          TEE_RSA_SIGN_TYPE_MAX,
                                   TEE_RSA_SHA256, TEE_RSA_SIGN_TYPE_MAX, TEE_RSA_SIGN_TYPE_MAX};

    CHECK_PARAM(rsa, false);
    CHECK_PARAM(context, false);
    CHECK_PARAM(src, false);
    CHECK_PARAM(signature, false);
    uint32_t ret;

    /* src:degist */
    sc_common_set_key_bits(rsa, context);
    CHECK_PARAM(context->key_bits != 0, false);
    ret = csi_tee_rsa_verify(src, src_size, (const uint8_t *)context->n,
                             (context->key_bits / 8) * 2, signature, sig_size, types[hash_type]);
  /* ret  suc: 0  failed:other */
    return (ret == 0);
}

/**
  \brief       encrypt(async mode)
  \param[in]   rsa       rsa handle to operate.
  \param[in]   context   Pointer to the rsa context
  \param[in]   src       Pointer to the source data.
  \param[in]   src_size  the source data len
  \param[out]  out       Pointer to the result buffer
  \return      \ref uint32_t
*/
uint32_t sc_rsa_encrypt_async(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src,
                              uint32_t src_size, void *out)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       decrypt(async mode)
  \param[in]   rsa       rsa handle to operate.
  \param[in]   context   Pointer to the rsa context
  \param[in]   src       Pointer to the source data.
  \param[in]   src_size  the source data len
  \param[out]  out       Pointer to the result buffer
  \param[out]  out_size  the result size
  \return      \ref uint32_t
*/
uint32_t sc_rsa_decrypt_async(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src,
                              uint32_t src_size, void *out, uint32_t *out_size)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       rsa sign(async mode)
  \param[in]   rsa       rsa handle to operate.
  \param[in]   context   Pointer to the rsa context
  \param[in]   src       Pointer to the source data.
  \param[in]   src_size  the source data len
  \param[out]  signature Pointer to the signature
  \param[in]   hash_type the source data hash type
  \return      \ref uint32_t
*/
uint32_t sc_rsa_sign_async(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src, uint32_t src_size,
                           void *signature, sc_rsa_hash_type_t hash_type)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       rsa verify(async mode)
  \param[in]   rsa       rsa handle to operate.
  \param[in]   context   Pointer to the rsa context
  \param[in]   src       Pointer to the source data.
  \param[in]   src_size  the source data len
  \param[in]   signature Pointer to the signature
  \param[in]   sig_size  the signature size
  \param[in]   hash_type the source data hash type
  \return      verify result
*/
uint32_t sc_rsa_verify_async(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src, uint32_t src_size,
                             void *signature, uint32_t sig_size, sc_rsa_hash_type_t hash_type)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Get RSA state.
  \param[in]   rsa      rsa handle to operate.
  \param[out]  state    rsa state \ref sc_rsa_state_t.
  \return      \ref uint32_t
*/
uint32_t sc_rsa_get_state(sc_rsa_t *rsa, sc_rsa_state_t *state)
{

    return SC_NOT_SUPPORT;
}
/**
  \brief       Get big prime data
  \param[in]   rsa          rsa handle to operate.
  \param[in]   p            Pointer to the prime
  \param[in]   bit_length   Pointer to the prime bit length
  \return      \ref uint32_t
*/
uint32_t sc_rsa_get_prime(sc_rsa_t *rsa, void *p, uint32_t bit_length)
{
    return SC_NOT_SUPPORT;
}
#endif
