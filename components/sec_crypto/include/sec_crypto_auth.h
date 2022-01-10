/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


#ifndef _SC_AUTH_H_
#define _SC_AUTH_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t  mac[6];
    uint8_t  rand;
    unsigned vendor_id: 9;
    unsigned : 7;/**< reserve */
} __attribute__((packed)) message_auth_t;

int sc_srv_authenticate(uint8_t *authinfo, int authinfo_size);

/**
  \brief       Generate message authorization code
  \param[in]   message  rsa handle to operate.
  \param[in]   authcode  authorization code
  \return      0: succfull other: error code
*/
int sc_srv_get_message_auth(message_auth_t *message, uint32_t *authcode);

/**
  \brief       Generate message authorization code
  \param[in]   key  rsa handle to operate.
  \param[in]   len  authorization code
  \return      0: succfull other: error code
*/
int sc_srv_get_auth_key(uint8_t *salt, uint32_t salt_len, uint8_t *key, uint32_t *len);

/**
  \brief       data encrypto/decrypto
  \param[in]   input  input data.
  \param[in]   output  output data
  \param[in]   size  data size
  \return      0: succfull other: error code
*/
int sc_srv_sensitive_data_encrypto(uint8_t *input, uint8_t *output, uint32_t size);
int sc_srv_sensitive_data_decrypto(uint8_t *input, uint8_t *output, uint32_t size);

/**
  \brief       Generate occ authmsg
  \param[in]   cid  cid
  \param[in]   cid_len
  \param[in]   tstmp  timestamp
  \param[in]   tstmp_len
  \param[in]   authmsg  out
  \return      0: succfull other: error code
*/
int sc_srv_get_authmsg(uint8_t *cid, uint32_t cid_len,
    uint8_t *tstmp, uint32_t tstmp_len, uint8_t *authmsg);

#ifdef __cplusplus
}
#endif

#endif /* _SC_RSA_H_ */