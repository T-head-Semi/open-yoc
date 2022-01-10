/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef CONFIG_SEC_CRYPTO_RSA_SW

#ifdef CONFIG_CSI_V2
#include "sec_mcu.h"
#include "sec_crypto_common.h"

static void s_get_keybit(sc_rsa_context_t *ctx, uint32_t bits)
{
        switch (bits) {
                case 2048:
                        ctx->key_bits = RSA_KEY_BITS_2048;
                        break;
                case 1024:
                        ctx->key_bits = RSA_KEY_BITS_1024;
                        break;
                /* note: add other here */
                default:
                        ctx->key_bits = RSA_KEY_BITS_2048;
                        return;
        }
}
/**
  \brief       Initialize RSA Interface. 1. Initializes the resources needed for the RSA interface 2.registers event callback function
*/
uint32_t sc_rsa_init(sc_rsa_t *rsa, uint32_t idx, sc_rsa_key_bits_t data_bits)
{
        uint32_t ret;
        CHECK_PARAM(rsa, SC_PARAM_INV);
        CHECK_PARAM(((data_bits == SC_RSA_KEY_BITS_1024) ||
                (data_bits == SC_RSA_KEY_BITS_2048) || (data_bits == 0)), SC_PARAM_INV);


        ret = csi_rsa_init(&rsa->csi_rsa, 0);
        if (ret) {
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
        if (rsa) {
                csi_rsa_uninit(&rsa->csi_rsa);
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
        csi_error_t       ret;
        csi_rsa_context_t ctx;
        CHECK_PARAM(rsa, SC_PARAM_INV);
        CHECK_PARAM(context, SC_PARAM_INV);
        CHECK_PARAM(src, SC_PARAM_INV);
        CHECK_PARAM(out, SC_PARAM_INV);
        CHECK_PARAM((context->key_bits == 1024) || (context->key_bits == 2048),
                    SC_PARAM_INV);

        ctx.n = context->n;
        ctx.d = context->d;
        ctx.e = context->e;
        s_get_keybit(&ctx, context->key_bits);
        ctx.padding_type = context->padding_type;
        ret = csi_rsa_encrypt(&rsa->csi_rsa, &ctx, src, src_size, out);
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
        int32_t           ret;
        csi_rsa_context_t ctx;
        CHECK_PARAM(rsa, SC_PARAM_INV);
        CHECK_PARAM(context, SC_PARAM_INV);
        CHECK_PARAM(src, SC_PARAM_INV);
        CHECK_PARAM(out, SC_PARAM_INV);
        CHECK_PARAM(out_size, SC_PARAM_INV);
        CHECK_PARAM((context->key_bits == 1024) || (context->key_bits == 2048),
                    SC_PARAM_INV);
        ctx.n = context->n;
        ctx.d = context->d;
        ctx.e = context->e;
        s_get_keybit(&ctx, context->key_bits);
        ctx.padding_type = context->padding_type;

        ret =
            csi_rsa_decrypt(&rsa->csi_rsa, &ctx, src, src_size, out, out_size);
        if (ret) {
                return SC_DRV_FAILED;
        }

        return SC_OK;
}

/**
  \brief       rsa sign
*/
uint32_t sc_rsa_sign(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src,
                     uint32_t src_size, void *signature,
                     sc_rsa_hash_type_t hash_type)
{
        SC_lOG("===%s, %d\n", __FUNCTION__, __LINE__);
        int32_t           ret;
        csi_rsa_context_t ctx;
        CHECK_PARAM(rsa, SC_PARAM_INV);
        CHECK_PARAM(context, SC_PARAM_INV);
        CHECK_PARAM(src, SC_PARAM_INV);
        CHECK_PARAM(signature, SC_PARAM_INV);
        CHECK_PARAM((context->key_bits == 1024) || (context->key_bits == 2048),
                    SC_PARAM_INV);
        ctx.n = context->n;
        ctx.d = context->d;
        ctx.e = context->e;
        s_get_keybit(&ctx, context->key_bits);
        ctx.padding_type   = context->padding_type;
        context->hash_type = hash_type;

        ret = csi_rsa_sign(&rsa->csi_rsa, &ctx, src, src_size, signature,
                           hash_type);
        if (ret) {
                return SC_DRV_FAILED;
        }

        return SC_OK;
}

/**
  \brief       rsa verify
*/
bool sc_rsa_verify(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src,
                   uint32_t src_size, void *signature, uint32_t sig_size,
                   sc_rsa_hash_type_t hash_type)
{
        SC_lOG("===%s, %d\n", __FUNCTION__, __LINE__);
        csi_rsa_context_t ctx;
        CHECK_PARAM(rsa, false);
        CHECK_PARAM(context, false);
        CHECK_PARAM(src, false);
        CHECK_PARAM(signature, false);
        CHECK_PARAM((context->key_bits == 1024) || (context->key_bits == 2048),
                    false);
        ctx.n = context->n;
        ctx.d = context->d;
        ctx.e = context->e;
        s_get_keybit(&ctx, context->key_bits);
        ctx.padding_type   = context->padding_type;
        context->hash_type = hash_type;

        return csi_rsa_verify(&rsa->csi_rsa, &ctx, src, src_size, signature,
                              sig_size, hash_type);
}

/**
  \brief       encrypt(async mode)
*/
uint32_t sc_rsa_encrypt_async(sc_rsa_t *rsa, sc_rsa_context_t *context,
                              void *src, uint32_t src_size, void *out)
{

        return SC_NOT_SUPPORT;
}

/**
  \brief       decrypt(async mode)
*/
uint32_t sc_rsa_decrypt_async(sc_rsa_t *rsa, sc_rsa_context_t *context,
                              void *src, uint32_t src_size, void *out,
                              uint32_t *out_size)
{

        return SC_NOT_SUPPORT;
}

/**
  \brief       rsa sign(async mode)
*/
uint32_t sc_rsa_sign_async(sc_rsa_t *rsa, sc_rsa_context_t *context, void *src,
                           uint32_t src_size, void *signature,
                           sc_rsa_hash_type_t hash_type)
{

        return SC_NOT_SUPPORT;
}

/**
  \brief       rsa verify(async mode)
*/
uint32_t sc_rsa_verify_async(sc_rsa_t *rsa, sc_rsa_context_t *context,
                             void *src, uint32_t src_size, void *signature,
                             uint32_t sig_size, sc_rsa_hash_type_t hash_type)
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
