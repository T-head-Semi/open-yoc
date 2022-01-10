/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     softse_rsa.c
 * @brief    rsa
 * @version  V1.0
 * @date     21. Aug 2020
 * @model    sse
 ******************************************************************************/
#ifdef CONFIG_SEC_CRYPTO_RSA_SW
#include <stdint.h>
#include <stdbool.h>
#include <sec_crypto_rsa.h>
#include <sec_crypto_errcode.h>
#include "crypto_rsa.h"
#include "sec_crypto_common.h"

// #ifndef CONFIG_SYSTEM_SECURE
// #include "ulog/ulog.h"
// #else
// int aos_log_hexdump(const char *tag, char *buffer, int len)
// {
//   int i;

//   printf("[%s]\n", tag);
//   printf("0x0000: ");
//   for (i = 0; i < len; i++)
//   {
//     printf("0x%02x ", buffer[i]);

//     if (i % 8 == 7)
//     {
//       printf("\n");
//       printf("0x%04x: ", i + 1);
//     }
//   }

//   printf("\n");
//   return 0;
// }
// #endif

// Function documentation

/**
  \brief       Initialize RSA Interface. 1. Initializes the resources needed for the RSA interface 2.registers event callback function
  \param[in]   rsa  rsa handle to operate.
  \param[in]   idx  device id
  \return      \ref uint32_t
*/
uint32_t sc_rsa_init(sc_rsa_t *rsa, uint32_t idx, sc_rsa_key_bits_t data_bits)
{
    SC_lOG("===%s, %d, %d\n", __FUNCTION__, __LINE__, data_bits);
    CHECK_PARAM(rsa, SC_PARAM_INV);
    CHECK_PARAM(((data_bits == SC_RSA_KEY_BITS_1024) ||
                (data_bits == SC_RSA_KEY_BITS_2048) || (data_bits == 0)), SC_PARAM_INV);
    memset(rsa, 0, sizeof(sc_rsa_t));
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
    SC_lOG("===%s, %d\n", __FUNCTION__, __LINE__);
    if (rsa)
    {
      memset(rsa, 0, sizeof(sc_rsa_t));
    }
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
  SC_lOG("===%s, %d\n", __FUNCTION__, __LINE__);
  uint32_t ret;
  size_t len;
  size_t ilen = src_size;
  sc_mbedtls_md_type_t hid[] = {SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_SHA1, SC_MBEDTLS_MD_SHA224,
                                SC_MBEDTLS_MD_SHA256, SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_NONE};

  CHECK_PARAM(rsa, SC_PARAM_INV);
  CHECK_PARAM(context, SC_PARAM_INV);
  CHECK_PARAM(src, SC_PARAM_INV);
  CHECK_PARAM(out, SC_PARAM_INV);

  if (context->padding_type != SC_RSA_PADDING_MODE_PKCS1)
  {
    return SC_NOT_SUPPORT;
  }

  len = context->key_bits / 8;

  sc_mbedtls_rsa_init(&rsa->rsa_ctx, SC_MBEDTLS_RSA_PKCS_V15, hid[context->hash_type]);
  rsa->rsa_ctx.len = len;

  ret = sc_mbedtls_mpi_read_binary(&rsa->rsa_ctx.N, context->n, len);
  CHECK_RET_WITH_RET(ret == 0, SC_CRYPT_FAIL);
  ret = sc_mbedtls_mpi_read_binary(&rsa->rsa_ctx.E, context->e, len);
  CHECK_RET_WITH_RET(ret == 0, SC_CRYPT_FAIL);
  ret = sc_mbedtls_rsa_pkcs1_encrypt(&rsa->rsa_ctx, rsa_rand, NULL, SC_MBEDTLS_RSA_PUBLIC,
                                     ilen, src, out);
  sc_mbedtls_mpi_free(&rsa->rsa_ctx.N);
  sc_mbedtls_mpi_free(&rsa->rsa_ctx.E);

  return ret;
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
  SC_lOG("===%s, %d\n", __FUNCTION__, __LINE__);
  uint32_t ret;
  size_t len;
  size_t olen;
  sc_mbedtls_md_type_t hid[] = {SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_SHA1, SC_MBEDTLS_MD_SHA224,
                                SC_MBEDTLS_MD_SHA256, SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_NONE};

  CHECK_PARAM(rsa, SC_PARAM_INV);
  CHECK_PARAM(context, SC_PARAM_INV);
  CHECK_PARAM(src, SC_PARAM_INV);
  CHECK_PARAM(out, SC_PARAM_INV);
  CHECK_PARAM(out_size, SC_PARAM_INV);
  if (context->padding_type != SC_RSA_PADDING_MODE_PKCS1)
  {
    return SC_NOT_SUPPORT;
  }

  len = context->key_bits / 8;

  sc_mbedtls_rsa_init(&rsa->rsa_ctx, SC_MBEDTLS_RSA_PKCS_V15, hid[context->hash_type]);
  rsa->rsa_ctx.len = len;
  ret = sc_mbedtls_mpi_read_binary(&rsa->rsa_ctx.N, context->n, len);
  CHECK_RET_WITH_RET(ret == 0, SC_CRYPT_FAIL);
  ret = sc_mbedtls_mpi_read_binary(&rsa->rsa_ctx.E, context->e, len);
  CHECK_RET_WITH_RET(ret == 0, SC_CRYPT_FAIL);
  ret = sc_mbedtls_mpi_read_binary(&rsa->rsa_ctx.D, context->d, len);
  CHECK_RET_WITH_RET(ret == 0, SC_CRYPT_FAIL);
  ret = sc_mbedtls_rsa_pkcs1_decrypt(&rsa->rsa_ctx, rsa_rand, NULL, SC_MBEDTLS_RSA_PRIVATE,
                                     &olen, src, out, len);
  *out_size = olen;
  sc_mbedtls_mpi_free(&rsa->rsa_ctx.N);
  sc_mbedtls_mpi_free(&rsa->rsa_ctx.E);
  sc_mbedtls_mpi_free(&rsa->rsa_ctx.D);

  return ret;
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
  SC_lOG("===%s, %d\n", __FUNCTION__, __LINE__);
  uint32_t ret;
  size_t len;
  sc_mbedtls_md_type_t hid[] = {SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_SHA1, SC_MBEDTLS_MD_SHA224,
                                SC_MBEDTLS_MD_SHA256, SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_NONE};
  sc_mbedtls_md_type_t types[] = {SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_SHA1, SC_MBEDTLS_MD_SHA224,
                                  SC_MBEDTLS_MD_SHA256, SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_NONE};

  CHECK_PARAM(rsa, SC_PARAM_INV);
  CHECK_PARAM(context, SC_PARAM_INV);
  CHECK_PARAM(src, SC_PARAM_INV);
  CHECK_PARAM(signature, SC_PARAM_INV);
  if (context->padding_type != SC_RSA_PADDING_MODE_PKCS1)
  {
    return SC_NOT_SUPPORT;
  }
  context->hash_type = hash_type;

  len = context->key_bits / 8;
  sc_mbedtls_rsa_init(&rsa->rsa_ctx, SC_MBEDTLS_RSA_PKCS_V15, hid[context->hash_type]);
  rsa->rsa_ctx.len = len;
  ret = sc_mbedtls_mpi_read_binary(&rsa->rsa_ctx.N, context->n, len);
  CHECK_RET_WITH_RET(ret == 0, SC_CRYPT_FAIL);
  ret = sc_mbedtls_mpi_read_binary(&rsa->rsa_ctx.E, context->e, len);
  CHECK_RET_WITH_RET(ret == 0, SC_CRYPT_FAIL);
  ret = sc_mbedtls_mpi_read_binary(&rsa->rsa_ctx.D, context->d, len);
  CHECK_RET_WITH_RET(ret == 0, SC_CRYPT_FAIL);
  /* src:degist */

  ret = sc_mbedtls_rsa_pkcs1_sign(&rsa->rsa_ctx, rsa_rand, NULL, SC_MBEDTLS_RSA_PRIVATE,
                                  types[hash_type], 0, src, signature);
  sc_mbedtls_mpi_free(&rsa->rsa_ctx.N);
  sc_mbedtls_mpi_free(&rsa->rsa_ctx.E);
  sc_mbedtls_mpi_free(&rsa->rsa_ctx.D);
  return ret;
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
  SC_lOG("===%s, %d, %d \n", __FUNCTION__, __LINE__, hash_type);
  SC_lOG("===%s, %d, %d \n", __FUNCTION__, __LINE__, context->hash_type);
  SC_lOG("===%s, %d, %d \n", __FUNCTION__, __LINE__, src_size);
  SC_lOG("===%s, %d, %d \n", __FUNCTION__, __LINE__, sig_size);
  bool ret;
  uint32_t len;
  sc_mbedtls_md_type_t hid[] = {SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_SHA1, SC_MBEDTLS_MD_SHA224,
                                SC_MBEDTLS_MD_SHA256, SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_NONE};
  sc_mbedtls_md_type_t types[] = {SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_SHA1, SC_MBEDTLS_MD_SHA224,
                                  SC_MBEDTLS_MD_SHA256, SC_MBEDTLS_MD_NONE, SC_MBEDTLS_MD_NONE};
  CHECK_PARAM(rsa, false);
  CHECK_PARAM(context, false);
  CHECK_PARAM(src, false);
  CHECK_PARAM(signature, false);
  if (context->padding_type != SC_RSA_PADDING_MODE_PKCS1)
  {
    return false;
  }
  context->hash_type = hash_type;

  len = context->key_bits / 8;
  SC_lOG("===%s, %d, %d \n", __FUNCTION__, __LINE__, len);
  sc_mbedtls_rsa_init(&rsa->rsa_ctx, SC_MBEDTLS_RSA_PKCS_V15, hid[context->hash_type]);
  rsa->rsa_ctx.len = len;

  // aos_log_hexdump("n", (char *)context->n, len);
  // aos_log_hexdump("e", (char *)context->e, len);
  // aos_log_hexdump("src", (char *)src, src_size);
  // aos_log_hexdump("signature", (char *)signature, sig_size);

  ret = sc_mbedtls_mpi_read_binary(&rsa->rsa_ctx.N, context->n, len);
  CHECK_RET_WITH_RET(ret == 0, false);
  ret = sc_mbedtls_mpi_read_binary(&rsa->rsa_ctx.E, context->e, len);
  CHECK_RET_WITH_RET(ret == 0, false);
  /* src:degist */

  ret = sc_mbedtls_rsa_pkcs1_verify(&rsa->rsa_ctx, NULL, NULL, SC_MBEDTLS_RSA_PUBLIC,
                                    types[hash_type], 0, src, signature);
  sc_mbedtls_mpi_free(&rsa->rsa_ctx.N);
  sc_mbedtls_mpi_free(&rsa->rsa_ctx.E);

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
