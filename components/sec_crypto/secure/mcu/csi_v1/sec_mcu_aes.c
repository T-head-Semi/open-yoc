/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef CONFIG_SEC_CRYPTO_AES_SW

#ifdef CONFIG_CSI_V1
#include "sec_mcu.h"
#include "sec_crypto_common.h"

// Function documentation

/**
  \brief       Initialize AES Interface. Initializes the resources needed for the AES interface
*/
uint32_t sc_aes_init(sc_aes_t *aes, uint32_t idx)
{
    CHECK_PARAM(aes, SC_PARAM_INV);

    aes->handle = csi_aes_initialize(idx, NULL);
    if (!aes->handle) {
        return SC_DRV_FAILED;
    }
    return SC_OK;
}

/**
  \brief       De-initialize AES Interface. stops operation and releases the software resources used by the interface
*/
void sc_aes_uninit(sc_aes_t *aes)
{
    if (aes && aes->handle) {
        csi_aes_uninitialize(aes->handle);
        memset(aes, 0, sizeof(sc_aes_t));
    }
}

/**
  \brief       Set encrypt key
*/
uint32_t sc_aes_set_encrypt_key(sc_aes_t *aes, void *key, sc_aes_key_bits_t key_len)
{
    uint32_t to_len[] = {16, 24, 32};
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(key, SC_PARAM_INV);
    CHECK_PARAM(aes->handle, SC_PARAM_INV);
    memcpy(aes->key, key, to_len[key_len]);
    aes->key_len = to_len[key_len];

    return SC_OK;
}
/**
  \brief       Set decrypt key
*/
uint32_t sc_aes_set_decrypt_key(sc_aes_t *aes, void *key, sc_aes_key_bits_t key_len)
{
    uint32_t to_len[] = {16, 24, 32};
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(key, SC_PARAM_INV);
    CHECK_PARAM(aes->handle, SC_PARAM_INV);

    memcpy(aes->key, key, to_len[key_len]);
    aes->key_len = to_len[key_len];

    return SC_OK;
}

static uint32_t aes_csi_config(sc_aes_t *aes, aes_mode_e mode, aes_crypto_mode_e enc)
{
    uint32_t           ret;
    aes_key_len_bits_e bits;

    if (aes->key_len == 16) {
        bits = AES_KEY_LEN_BITS_128;
    } else if (aes->key_len == 24) {
        bits = AES_KEY_LEN_BITS_192;
    } else if (aes->key_len == 32) {
        bits = AES_KEY_LEN_BITS_256;
    } else {
        return SC_PARAM_INV;
    }
#if defined(CONFIG_SEC_CRYPTO_AES_ENDIAN_BIG) &&  (CONFIG_SEC_CRYPTO_AES_ENDIAN_BIG> 0)
    ret = csi_aes_config(aes->handle, mode, bits, AES_ENDIAN_BIG);
#else
    ret = csi_aes_config(aes->handle, mode, bits, AES_ENDIAN_LITTLE);
#endif
    if (ret) {
        return SC_DRV_FAILED;
    }

    ret = csi_aes_set_key(aes->handle, NULL, aes->key, bits, enc);
    if (ret) {
        return SC_DRV_FAILED;
    }

    return ret;
}

/**
  \brief       Aes ecb encrypt
*/
uint32_t sc_aes_ecb_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size)
{
    uint32_t ret;
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(in, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);
    CHECK_PARAM(aes->handle, SC_PARAM_INV);

    ret = aes_csi_config(aes, AES_MODE_ECB, AES_CRYPTO_MODE_ENCRYPT);
    if (ret) {
        return SC_DRV_FAILED;
    }

    ret = csi_aes_ecb_crypto(aes->handle, NULL, in, out, size);
    if (ret) {
        return SC_DRV_FAILED;
    }

    return SC_OK;
}

/**
  \brief       Aes ecb decrypt
*/
uint32_t sc_aes_ecb_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size)
{
    uint32_t ret;
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(in, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);
    CHECK_PARAM(aes->handle, SC_PARAM_INV);

    ret = aes_csi_config(aes, AES_MODE_ECB, AES_CRYPTO_MODE_DECRYPT);
    if (ret) {
        return SC_DRV_FAILED;
    }

    ret = csi_aes_ecb_crypto(aes->handle, NULL, in, out, size);
    if (ret) {
        return SC_DRV_FAILED;
    }

    return SC_OK;
}

/**
  \brief       Aes cbc encrypt
*/
uint32_t sc_aes_cbc_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv)
{
    uint32_t ret;
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(in, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);
    CHECK_PARAM(iv, SC_PARAM_INV);
    CHECK_PARAM(aes->handle, SC_PARAM_INV);
    uint8_t temp[16];

    ret = aes_csi_config(aes, AES_MODE_CBC, AES_CRYPTO_MODE_ENCRYPT);
    if (ret) {
        return SC_DRV_FAILED;
    }

    memcpy(temp, iv, 16);
    ret = csi_aes_cbc_crypto(aes->handle, NULL, in, out, size, temp);
    if (ret) {
        return SC_DRV_FAILED;
    }

    return SC_OK;
}

/**
  \brief       Aes cbc decrypt
*/
uint32_t sc_aes_cbc_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv)
{
    uint32_t ret;
    CHECK_PARAM(aes, SC_PARAM_INV);
    CHECK_PARAM(in, SC_PARAM_INV);
    CHECK_PARAM(out, SC_PARAM_INV);
    CHECK_PARAM(iv, SC_PARAM_INV);
    CHECK_PARAM(aes->handle, SC_PARAM_INV);
    uint8_t temp[16];

    ret = aes_csi_config(aes, AES_MODE_CBC, AES_CRYPTO_MODE_DECRYPT);
    if (ret) {
        return SC_DRV_FAILED;
    }

    memcpy(temp, iv, 16);
    ret = csi_aes_cbc_crypto(aes->handle, NULL, in, out, size, temp);
    if (ret) {
        return SC_DRV_FAILED;
    }

    return SC_OK;
}

/**
  \brief       Aes cfb1 encrypt
*/
uint32_t sc_aes_cfb1_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       Aes cfb1 decrypt
*/
uint32_t sc_aes_cfb1_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       Aes cfb8 encrypt
*/
uint32_t sc_aes_cfb8_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       Aes cfb8 decrypt
*/
uint32_t sc_aes_cfb8_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       Aes cfb128 decrypt
*/
uint32_t sc_aes_cfb128_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv,
                               uint32_t *num)
{
    return SC_NOT_SUPPORT;
}

/**
  \brief       Aes cfb128 encrypt
*/
uint32_t sc_aes_cfb128_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv,
                               uint32_t *num)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes ofb encrypt
*/
uint32_t sc_aes_ofb_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv,
                            uint32_t *num)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes ofb decrypt
*/
uint32_t sc_aes_ofb_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size, void *iv,
                            uint32_t *num)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes ctr encrypt
*/
uint32_t sc_aes_ctr_encrypt(sc_aes_t *aes, void *in, void *out, uint32_t size,
                            uint8_t nonce_counter[16], uint8_t stream_block[16], void *iv,
                            uint32_t *num)
{
    return SC_NOT_SUPPORT;
}
/**
  \brief       Aes ctr decrypt
*/
uint32_t sc_aes_ctr_decrypt(sc_aes_t *aes, void *in, void *out, uint32_t size,
                            uint8_t nonce_counter[16], uint8_t stream_block[16], void *iv,
                            uint32_t *num)
{
    return SC_NOT_SUPPORT;
}

int sc_mpu_set_memory_attr(uint32_t region_addr, int region_size, enum region_access_perm rap)
{
    return SC_NOT_SUPPORT;
}
#endif

#endif