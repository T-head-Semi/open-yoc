/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef CONFIG_SEC_CRYPTO_RSA_SW

#ifdef CONFIG_CSI_V1
#include "sec_mcu.h"
#include "sec_crypto_common.h"


/**
  \brief       Initialize RSA Interface. 1. Initializes the resources needed for the RSA interface 2.registers event callback function
*/
uint32_t sc_rsa_init(sc_rsa_t *rsa, uint32_t idx, sc_rsa_key_bits_t data_bits)
{
    CHECK_PARAM(rsa, SC_PARAM_INV);
    CHECK_PARAM(((data_bits == SC_RSA_KEY_BITS_1024) ||
     (data_bits == SC_RSA_KEY_BITS_2048) || (data_bits == 0)), SC_PARAM_INV);

    rsa->handle = csi_rsa_initialize(0, NULL);
    if (!rsa->handle) {
        return SC_DRV_FAILED;
    }
    rsa->bits = data_bits;

    return SC_OK;
}

/**
  \brief       De-initialize RSA Interface. stops operation and releases the software resources used by the interface
*/
void sc_rsa_uninit(sc_rsa_t *rsa)
{
    if (rsa && rsa->handle) {
        csi_rsa_uninitialize(rsa->handle);
        memset(rsa, 0, sizeof(sc_rsa_t));
    }
}

/**
  \brief       attach the callback handler to RSA
*/
uint32_t sc_rsa_attach_callback(sc_rsa_t *rsa, sc_rsa_callback_t cb, void *arg)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       detach the callback handler
*/
void sc_rsa_detach_callback(sc_rsa_t *rsa)
{
}

/**
  \brief       generate rsa key pair.
*/
uint32_t sc_rsa_gen_key(sc_rsa_t *rsa, sc_rsa_context_t *context)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       encrypt
*/
uint32_t sc_rsa_encrypt(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src,
                            uint32_t src_size, void *out)
{
    uint32_t len;
    int32_t ret;
    rsa_padding_t pad;
    CHECK_PARAM(rsa, SC_PARAM_INV);
    CHECK_PARAM(context, SC_PARAM_INV);
    CHECK_PARAM(src, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);
    pad.padding_type = context->padding_type;
    pad.hash_type = context->hash_type;

    csi_rsa_config(rsa->handle, rsa->bits, RSA_ENDIAN_MODE_LITTLE, (void *)context->n);
    ret =  csi_rsa_encrypt(rsa->handle, (void *)context->n, (void *)context->e, src, src_size,
                            out, &len, pad);
    if (ret) {
        return SC_DRV_FAILED;
    }
    return SC_OK;
}


/**
  \brief       decrypt
*/
uint32_t sc_rsa_decrypt(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src,
                            uint32_t src_size, void *out, uint32_t *out_size)
{
    int32_t ret;
    rsa_padding_t pad;
    CHECK_PARAM(rsa, SC_PARAM_INV);
    CHECK_PARAM(context, SC_PARAM_INV);
    CHECK_PARAM(src, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);
    CHECK_PARAM(out_size, SC_PARAM_INV);
    pad.padding_type = context->padding_type;
    pad.hash_type = context->hash_type;

    csi_rsa_config(rsa->handle, rsa->bits, RSA_ENDIAN_MODE_LITTLE, (void *)context->n);
    ret =  csi_rsa_decrypt(rsa->handle, (void *)context->n, (void *)context->d, src, src_size,
                            out, out_size, pad);
    if (ret) {
        return SC_DRV_FAILED;
    }

    return SC_OK;
}

/**
  \brief       rsa sign
*/
uint32_t sc_rsa_sign(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src, uint32_t src_size,
                         void *signature, sc_rsa_hash_type_t hash_type)
{
    int32_t ret;
    uint32_t len;
    rsa_padding_t pad;
    CHECK_PARAM(rsa, SC_PARAM_INV);
    CHECK_PARAM(context, SC_PARAM_INV);
    CHECK_PARAM(src, SC_PARAM_INV);
    CHECK_PARAM(signature, SC_PARAM_INV);
    pad.padding_type = context->padding_type;
    pad.hash_type = hash_type;

    csi_rsa_config(rsa->handle, rsa->bits, RSA_ENDIAN_MODE_LITTLE, (void *)context->n);
    ret =  csi_rsa_sign(rsa->handle, (void *)context->n, (void *)context->d, src, src_size,
                            signature, &len, pad);
    if (ret) {
        return SC_DRV_FAILED;
    }

    return SC_OK;
}

/**
  \brief       rsa verify
*/
bool sc_rsa_verify(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src, uint32_t src_size,
                    void *signature, uint32_t sig_size, sc_rsa_hash_type_t hash_type)
{
    int32_t ret;
    uint8_t result;
    rsa_padding_t pad;
    CHECK_PARAM(rsa, false);
    CHECK_PARAM(context, false);
    CHECK_PARAM(src, false);
    CHECK_PARAM(signature, false);
    pad.padding_type = context->padding_type;
    pad.hash_type = hash_type;

    csi_rsa_config(rsa->handle, rsa->bits, RSA_ENDIAN_MODE_LITTLE, (void *)context->n);
    ret =  csi_rsa_verify(rsa->handle, (void *)context->n, (void *)context->e, src, src_size,
                            signature, sig_size, &result, pad);
    if (ret) {
        return false;
    }
  /* result  suc: 1  failed:0 */

    return (result != 0);
}


/**
  \brief       encrypt(async mode)
*/
uint32_t sc_rsa_encrypt_async(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src,
                                  uint32_t src_size, void *out)
{

    return SC_NOT_SUPPORT;
}

/**
  \brief       decrypt(async mode)
*/
uint32_t sc_rsa_decrypt_async(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src,
                                  uint32_t src_size, void *out, uint32_t *out_size)
{

    return SC_NOT_SUPPORT;
}

/**
  \brief       rsa sign(async mode)
*/
uint32_t sc_rsa_sign_async(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src,
                               uint32_t src_size, void *signature, sc_rsa_hash_type_t hash_type)
{

    return SC_NOT_SUPPORT;
}

/**
  \brief       rsa verify(async mode)
*/
uint32_t sc_rsa_verify_async(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src,
                                 uint32_t src_size, void *signature, uint32_t sig_size,
                                 sc_rsa_hash_type_t hash_type)
{

    return SC_NOT_SUPPORT;
}

/**
  \brief       Get RSA state.
*/
uint32_t sc_rsa_get_state(sc_rsa_t *rsa, sc_rsa_state_t *state)
{

    return SC_NOT_SUPPORT;
}

/**
  \brief       Get big prime data
*/
uint32_t sc_rsa_get_prime(sc_rsa_t *rsa, void *p, uint32_t bit_length)
{

    return SC_NOT_SUPPORT;
}

/**
  \brief       enable rsa power manage
*/
uint32_t sc_rsa_enable_pm(sc_rsa_t *rsa)
{

    return SC_NOT_SUPPORT;
}

/**
  \brief       disable rsa power manage
*/
void sc_rsa_disable_pm(sc_rsa_t *rsa)
{
}

#endif
#endif
