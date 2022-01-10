// Copyright 2017-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#if defined(CONFIG_USING_TLS)
#include "transport/tls.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lwip/netdb.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <aos/errno.h>
#include "mbedtls/error.h"

#if defined(MBEDTLS_DEBUG_C)
#include "mbedtls/debug.h"
#endif

static const char *TAG = "tls";
static mbedtls_x509_crt *global_cacert = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void _ssl_debug(void *ctx, int level, const char *file, int line, const char *str)
{
#define TAG1 "MBTLS"

    char *file_sep;

    /* Shorten 'file' from the whole file path to just the filename

       This is a bit wasteful because the macros are compiled in with
       the full _FILE_ path in each case.
    */
    file_sep = strstr(file, "/");
    while(file_sep) {
        file_sep ++;
        if (!strstr(file_sep, "/")) {
            break;
        }
    }
    file = file_sep;

    switch(level) {
    case 1:
        LOGW(TAG1, "%s:%d %s", file, line, str);
        break;
    case 2:
        LOGI(TAG1, "%s:%d %s", file, line, str);
        break;
    case 3:
        LOGD(TAG1, "%s:%d %s", file, line, str);
        break;
    case 4:
        LOGD(TAG1, "%s:%d %s", file, line, str);
        break;
    default:
        LOGE(TAG1, "Unexpected log level %d: %s", level, str);
        break;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static ssize_t tcp_read(tls_t *tls, char *data, size_t datalen, uint32_t timeout_ms)
{
    return recv(tls->sockfd, data, datalen, 0);
}

static ssize_t tls_read(tls_t *tls, char *data, size_t datalen, uint32_t timeout_ms)
{
    int ret;

    LOGD(TAG, "tls read...");
    mbedtls_ssl_conf_read_timeout(&(tls->conf), timeout_ms);

    while (1) {
        ret = mbedtls_ssl_read(&(tls->ssl), (unsigned char *)data, datalen);
        if (ret != MBEDTLS_ERR_SSL_WANT_READ) {
            break;
        }
    }

    return ret;
}

static void ms_to_timeval(int timeout_ms, struct timeval *tv)
{
    tv->tv_sec = timeout_ms / 1000;
    tv->tv_usec = (timeout_ms % 1000) * 1000;
}

web_err_t tls_init_global_ca_store()
{
    if (global_cacert == NULL) {
        global_cacert = (mbedtls_x509_crt *)calloc(1, sizeof(mbedtls_x509_crt));
        if (global_cacert == NULL) {
            LOGE(TAG, "global_cacert not allocated");
            return WEB_ERR_NO_MEM;
        }
        mbedtls_x509_crt_init(global_cacert);
    }
    return WEB_OK;
}

web_err_t tls_set_global_ca_store(const unsigned char *cacert_pem_buf, const unsigned int cacert_pem_bytes)
{
    int ret;
    if (cacert_pem_buf == NULL) {
        LOGE(TAG, "cacert_pem_buf is null");
        return WEB_ERR_INVALID_ARG;
    }
    if (global_cacert == NULL) {
        ret = tls_init_global_ca_store();
        if (ret != WEB_OK) {
            return ret;
        }
    }
    ret = mbedtls_x509_crt_parse(global_cacert, cacert_pem_buf, cacert_pem_bytes);
    if (ret < 0) {
        LOGE(TAG, "mbedtls_x509_crt_ parse returned -0x%x\n\n", -ret);
        mbedtls_x509_crt_free(global_cacert);
        global_cacert = NULL;
        return WEB_FAIL;
    } else if (ret > 0) {
        LOGE(TAG, "mbedtls_x509_crt_ parse was partly successful. No. of failed certificates: %d", ret);
    }
    return WEB_OK;
}

mbedtls_x509_crt *tls_get_global_ca_store()
{
    return global_cacert;
}

void tls_free_global_ca_store()
{
    if (global_cacert) {
        mbedtls_x509_crt_free(global_cacert);
        global_cacert = NULL;
    }
}

static void verify_certificate(tls_t *tls)
{
    int flags;

    if ((flags = mbedtls_ssl_get_verify_result(&tls->ssl)) != 0) {
#if defined(MBEDTLS_DEBUG_C)
        char buf[100];
        LOGI(TAG, "Failed to verify peer certificate!");
        memset(buf, 0, sizeof(buf));
        mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
        LOGI(TAG, "verification info: %s", buf);
#endif
    } else {
        LOGI(TAG, "Certificate verified.");
    }
}

static void mbedtls_cleanup(tls_t *tls)
{
    if (!tls) {
        return;
    }
    mbedtls_ssl_close_notify(&tls->ssl);
    if (tls->cacert_ptr != global_cacert) {
        mbedtls_x509_crt_free(tls->cacert_ptr);
    }
    tls->cacert_ptr = NULL;
    mbedtls_x509_crt_free(&tls->cacert);
    mbedtls_x509_crt_free(&tls->clientcert);
    mbedtls_pk_free(&tls->clientkey);
    mbedtls_entropy_free(&tls->entropy);
    mbedtls_ssl_config_free(&tls->conf);
    mbedtls_ctr_drbg_free(&tls->ctr_drbg);
    mbedtls_ssl_free(&tls->ssl);
}

static int create_ssl_handle(tls_t *tls, const char *hostname, size_t hostlen, const tls_cfg_t *cfg)
{
    int ret, value;
    const char *pers = "https";

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(5);
#endif

    tls->server_fd.fd = tls->sockfd;
    mbedtls_ssl_init(&tls->ssl);
    mbedtls_ctr_drbg_init(&tls->ctr_drbg);
    mbedtls_ssl_config_init(&tls->conf);
    mbedtls_entropy_init(&tls->entropy);
    if ((value = mbedtls_ctr_drbg_seed(&tls->ctr_drbg,
                               mbedtls_entropy_func,
                               &tls->entropy,
                               (const unsigned char*)pers,
                               strlen(pers))) != 0) {
        LOGE(TAG, "mbedtls_ctr_drbg_seed() failed, value:-0x%x.", -value);
        ret = -1;
        goto exit;
    }

    /* Hostname set here should match CN in server certificate */
    char *use_host = strndup(hostname, hostlen);
    if (!use_host) {
        goto exit;
    }

    if ((ret = mbedtls_ssl_set_hostname(&tls->ssl, use_host)) != 0) {
        LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
        free(use_host);
        goto exit;
    }
    free(use_host);

    if ((ret = mbedtls_ssl_config_defaults(&tls->conf,
                    MBEDTLS_SSL_IS_CLIENT,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }
    mbedtls_ssl_conf_read_timeout(&tls->conf, 5000);

    mbedtls_ssl_conf_max_version(&tls->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_min_version(&tls->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

#ifdef CONFIG_MBEDTLS_SSL_ALPN
    if (cfg->alpn_protos) {
        mbedtls_ssl_conf_alpn_protocols(&tls->conf, cfg->alpn_protos);
    }
#endif

    if (cfg->use_global_ca_store == true) {
        if (global_cacert == NULL) {
            LOGE(TAG, "global_cacert is NULL");
            goto exit;
        }
        tls->cacert_ptr = global_cacert;
        mbedtls_ssl_conf_authmode(&tls->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_ca_chain(&tls->conf, tls->cacert_ptr, NULL);
    } else if (cfg->cacert_pem_buf != NULL) {
        tls->cacert_ptr = &tls->cacert;
        mbedtls_x509_crt_init(tls->cacert_ptr);
        ret = mbedtls_x509_crt_parse(tls->cacert_ptr, cfg->cacert_pem_buf, cfg->cacert_pem_bytes);
        if (ret < 0) {
            LOGE(TAG, "mbedtls_x509_crt_ parse returned -0x%x\n\n", -ret);
            goto exit;
        }
        mbedtls_ssl_conf_authmode(&tls->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_ca_chain(&tls->conf, tls->cacert_ptr, NULL);
    } else {
        mbedtls_ssl_conf_authmode(&tls->conf, MBEDTLS_SSL_VERIFY_NONE);
    }

    if (cfg->clientcert_pem_buf != NULL && cfg->clientkey_pem_buf != NULL) {
        mbedtls_x509_crt_init(&tls->clientcert);
        mbedtls_pk_init(&tls->clientkey);

        ret = mbedtls_x509_crt_parse(&tls->clientcert, cfg->clientcert_pem_buf, cfg->clientcert_pem_bytes);
        if (ret < 0) {
            LOGE(TAG, "mbedtls_x509_crt_ parse returned -0x%x\n\n", -ret);
            goto exit;
        }

        ret = mbedtls_pk_parse_key(&tls->clientkey, cfg->clientkey_pem_buf, cfg->clientkey_pem_bytes,
                  cfg->clientkey_password, cfg->clientkey_password_len);
        if (ret < 0) {
            LOGE(TAG, "mbedtls_pk_parse_keyfile returned -0x%x\n\n", -ret);
            goto exit;
        }

        ret = mbedtls_ssl_conf_own_cert(&tls->conf, &tls->clientcert, &tls->clientkey);
        if (ret < 0) {
            LOGE(TAG, "mbedtls_ssl_conf_own_cert returned -0x%x\n\n", -ret);
            goto exit;
        }
    } else if (cfg->clientcert_pem_buf != NULL || cfg->clientkey_pem_buf != NULL) {
        LOGE(TAG, "You have to provide both clientcert_pem_buf and clientkey_pem_buf for mutual authentication\n\n");
        goto exit;
    }

    mbedtls_ssl_conf_rng(&tls->conf, mbedtls_ctr_drbg_random, &tls->ctr_drbg);
    mbedtls_ssl_conf_dbg(&tls->conf, _ssl_debug, NULL);
#ifdef CONFIG_MBEDTLS_DEBUG
    mbedtls_enable_debug_log(&tls->conf, 4);
#endif

    if ((ret = mbedtls_ssl_setup(&tls->ssl, &tls->conf)) != 0) {
        LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }
    mbedtls_ssl_set_bio(&tls->ssl, &tls->server_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

    return 0;
exit:
    mbedtls_cleanup(tls);
    return -1;
}

/**
 * @brief      Close the TLS connection and free any allocated resources.
 */
void tls_conn_delete(tls_t *tls)
{
    if (tls != NULL) {
        mbedtls_cleanup(tls);
        if (tls->is_tls) {
            mbedtls_net_free(&tls->server_fd);
        } else if (tls->sockfd >= 0) {
            close(tls->sockfd);
        }
        free(tls);
    }
};

static ssize_t tcp_write(tls_t *tls, const char *data, size_t datalen)
{
    return send(tls->sockfd, data, datalen, 0);
}

static ssize_t tls_write(tls_t *tls, const char *data, size_t datalen)
{
    uint32_t writtenLen = 0;
    int ret = -1;

    while (writtenLen < datalen) {
        ret = mbedtls_ssl_write(&(tls->ssl), (unsigned char *)(data + writtenLen), (datalen - writtenLen));
        if (ret > 0) {
            writtenLen += ret;
            continue;
        } else if (ret == 0) {
            LOGE(TAG, "write timeout");
            return 0;
        } else {
            LOGE(TAG, "write -0x%04x", -ret);
            return -1; /* Connnection error */
        }
    }

    return writtenLen;
}

static int tls_low_level_conn(const char *hostname, int hostlen, int port, const tls_cfg_t *cfg, tls_t *tls)
{
    if (!tls) {
        LOGE(TAG, "empty tls parameter");
        return -1;
    }
    /* These states are used to keep a tab on connection progress in case of non-blocking connect,
    and in case of blocking connect these cases will get executed one after the other */
    switch (tls->conn_state) {
        case TLS_INIT:
            LOGD(TAG, "tls init...");
            tls->sockfd = -1;
            if (cfg != NULL) {
                mbedtls_net_init(&tls->server_fd);
                tls->is_tls = true;
            }
            // int ret = tcp_connect(hostname, hostlen, port, &tls->sockfd, cfg);
            char buf[32];
            snprintf(buf, 32, "%d", port);
            char *use_host = strndup(hostname, hostlen);
            if (use_host == NULL) {
                return -ENOMEM;
            }
            LOGD(TAG, "use_host:%s, port:%d", use_host, port);
            int ret = mbedtls_net_connect(&tls->server_fd, use_host, buf, MBEDTLS_NET_PROTO_TCP);
            free(use_host);
            LOGD(TAG, "_tls_net connect %d ", ret);
            tls->sockfd = tls->server_fd.fd;
            if (ret < 0) {
                return -1;
            }
            if (!cfg) {
                tls->read = tcp_read;
                tls->write = tcp_write;
                LOGD(TAG, "non-tls connection established");
                return 1;
            }
            if (cfg->non_block) {
                FD_ZERO(&tls->rset);
                FD_SET(tls->sockfd, &tls->rset);
                tls->wset = tls->rset;
            }
            tls->conn_state = TLS_CONNECTING;
            /* falls through */
        case TLS_CONNECTING:
            LOGD(TAG, "tls connecting...");
            if (cfg->non_block) {
                LOGD(TAG, "connecting...");
                struct timeval tv;
                ms_to_timeval(cfg->timeout_ms, &tv);

                /* In case of non-blocking I/O, we use the select() API to check whether
                   connection has been estbalished or not*/
                if (select(tls->sockfd + 1, &tls->rset, &tls->wset, NULL,
                    cfg->timeout_ms ? &tv : NULL) == 0) {
                    LOGD(TAG, "select() timed out");
                    return 0;
                }
                if (FD_ISSET(tls->sockfd, &tls->rset) || FD_ISSET(tls->sockfd, &tls->wset)) {
                    int error;
                    unsigned int len = sizeof(error);
                    /* pending error check */
                    if (getsockopt(tls->sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
                        LOGD(TAG, "Non blocking connect failed");
                        tls->conn_state = TLS_FAIL;
                        return -1;
                    }
                }
            }
            /* By now, the connection has been established */
            ret = create_ssl_handle(tls, hostname, hostlen, cfg);
            if (ret != 0) {
                LOGD(TAG, "create_ssl_handshake failed");
                tls->conn_state = TLS_FAIL;
                return -1;
            }
            tls->read = tls_read;
            tls->write = tls_write;
            tls->conn_state = TLS_HANDSHAKE;
            /* falls through */
        case TLS_HANDSHAKE:
            LOGD(TAG, "handshake in progress...");
            ret = mbedtls_ssl_handshake(&tls->ssl);
            if (ret == 0) {
                tls->conn_state = TLS_DONE;
                return 1;
            } else {
                if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                    LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
                    if (cfg->cacert_pem_buf != NULL || cfg->use_global_ca_store == true) {
                        /* This is to check whether handshake failed due to invalid certificate*/
                        verify_certificate(tls);
                    }
                    tls->conn_state = TLS_FAIL;
                    return -1;
                }
                /* Irrespective of blocking or non-blocking I/O, we return on getting MBEDTLS_ERR_SSL_WANT_READ
                   or MBEDTLS_ERR_SSL_WANT_WRITE during handshake */
                return 0;
            }
            break;
        case TLS_FAIL:
            LOGE(TAG, "failed to open a new connection");;
            break;
        default:
            LOGE(TAG, "invalid esp-tls state");
            break;
    }
    return -1;
}

/**
 * @brief      Create a new TLS/SSL connection
 */
tls_t *tls_conn_new(const char *hostname, int hostlen, int port, const tls_cfg_t *cfg)
{
    tls_t *tls = (tls_t *)calloc(1, sizeof(tls_t));
    if (!tls) {
        return NULL;
    }
    /* tls_conn_new() API establishes connection in a blocking manner thus this loop ensures that tls_conn_new()
       API returns only after connection is established unless there is an error*/
    while (1) {
        int ret = tls_low_level_conn(hostname, hostlen, port, cfg, tls);
        if (ret == 1) {
            LOGD(TAG, "open new connection ok");
            return tls;
        } else if (ret == -1) {
            tls_conn_delete(tls);
            LOGE(TAG, "Failed to open new connection");
            return NULL;
        }
    }
    return NULL;
}

/*
 * @brief      Create a new TLS/SSL non-blocking connection
 */
int tls_conn_new_async(const char *hostname, int hostlen, int port, const tls_cfg_t *cfg , tls_t *tls)
{
    LOGE(TAG, "tls_conn_new async");
    return tls_low_level_conn(hostname, hostlen, port, cfg, tls);
}

size_t tls_get_bytes_avail(tls_t *tls)
{
    if (!tls) {
        LOGE(TAG, "empty arg passed to tls_get_bytes_avail()");
        return WEB_FAIL;
    }
    return mbedtls_ssl_get_bytes_avail(&tls->ssl);
}
#endif