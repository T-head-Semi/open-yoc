/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_SAL) || defined(CONFIG_TCPIP)
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#else
#include <lwip/netdb.h>
#endif
#include <arpa/inet.h>

#include "avutil/common.h"
#include "avutil/web.h"
#include "avutil/dync_buf.h"
#include "avutil/socket_rw.h"

#ifdef CONFIG_USING_TLS
#include "mbedtls/base64.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#endif

#define TAG                   "WEB"
#ifndef __linux__
#define close                 lwip_close
#define fcntl                 lwip_fcntl
#endif

extern int select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);

#ifdef CONFIG_USING_TLS
struct mtls_session {
    uint8_t                     init;
    mbedtls_net_context         nctx;
    mbedtls_entropy_context     entropy;
    mbedtls_ctr_drbg_context    ctr_drbg;
    mbedtls_ssl_context         ssl;
    mbedtls_ssl_config          conf;
    mbedtls_x509_crt            cacert;
};
#endif

static void _wsession_init(wsession_t *session)
{
#ifdef CONFIG_USING_TLS
    struct mtls_session *mtls = session->tls;
#endif

    memset(session, 0, sizeof(wsession_t));
    session->fd               = -1;
    session->code             = -1;
    session->method           = WEB_METHOD_UNKNOWN;
    session->hdr_size_max     = WEB_HDR_SIZE_MAX_DEFAULT;
    session->body_size_max    = WEB_BODY_SIZE_MAX_DEFAULT;
    session->redirect_cnt_max = WEB_REDIRECT_CNT_MAX_DEFAULT;
#ifdef CONFIG_USING_TLS
    session->tls              = mtls;
#endif
}

#ifdef CONFIG_USING_TLS
static void _mtls_session_init(struct mtls_session *mtls)
{
    if (mtls && (!mtls->init)) {
        mbedtls_net_init(&mtls->nctx);
        mbedtls_ssl_init(&mtls->ssl);
        mbedtls_ssl_config_init(&mtls->conf);
        mbedtls_x509_crt_init(&mtls->cacert );
        mbedtls_ctr_drbg_init(&mtls->ctr_drbg);
        mbedtls_entropy_init(&mtls->entropy);
        mtls->init = 1;
    }
}

static void _mtls_session_deinit(struct mtls_session *mtls)
{
    if (mtls && mtls->init) {
        mbedtls_net_free(&mtls->nctx);
        mbedtls_x509_crt_free(&mtls->cacert);
        mbedtls_ssl_free(&mtls->ssl);
        mbedtls_ssl_config_free(&mtls->conf);
        mbedtls_ctr_drbg_free(&mtls->ctr_drbg);
        mbedtls_entropy_free(&mtls->entropy);
        mtls->init = 0;
    }
}


static int _mtls_net_send(void *ctx, const unsigned char *buf, size_t len)
{
    int fd = ((mbedtls_net_context*)ctx)->fd;

    return sock_writen(fd, (const char*)buf, len, 3000);
}

static int _mtls_net_reado(void *ctx, unsigned char *buf, size_t len, uint32_t timeout)
{
    int fd = ((mbedtls_net_context*)ctx)->fd;

    return sock_readn(fd, (char*)buf, len, timeout);
}

static int _https_open(wsession_t *session, web_url_t *wurl, int timeout_ms)
{
    int rc, fd = -1;
    char port[16];
    struct timeval tv;
    const char *pers          = "mbedtls_pc";
    struct mtls_session *mtls = session->tls;

    _mtls_session_init(mtls);
    rc = mbedtls_ctr_drbg_seed(&mtls->ctr_drbg, mbedtls_entropy_func, &mtls->entropy,
                               (const unsigned char *) pers, strlen(pers));
    CHECK_RET_WITH_GOTO(rc == 0, err);

    snprintf(port, sizeof(port), "%d", wurl->port);
    LOGD(TAG, "tls: connect to server %s ,port is %s", wurl->host, port);

    rc = mbedtls_net_connect(&mtls->nctx, wurl->host, port, MBEDTLS_NET_PROTO_TCP);
    if (rc != 0) {
        LOGE(TAG, "failed ! mbedtls_net_connect returned -0x%x", -rc);
        AV_ERRNO_SET(AV_ERRNO_CONNECT_FAILD);
        goto err;
    }

    fd         = mtls->nctx.fd;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, sizeof(tv));

    LOGD(TAG, "Setting up the SSL/TLS structure...a");
    rc = mbedtls_ssl_config_defaults(&mtls->conf,
                                     MBEDTLS_SSL_IS_CLIENT,
                                     MBEDTLS_SSL_TRANSPORT_STREAM,
                                     MBEDTLS_SSL_PRESET_DEFAULT);
    if (rc != 0) {
        LOGE(TAG, "failed ! mbedtls_ssl_config_defaults returned -0x%x", -rc);
        goto err;
    }

    mbedtls_ssl_conf_read_timeout(&mtls->conf, timeout_ms);
    mbedtls_ssl_conf_authmode(&mtls->conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&mtls->conf, mbedtls_ctr_drbg_random, &mtls->ctr_drbg);

    rc = mbedtls_ssl_setup(&mtls->ssl, &mtls->conf);
    CHECK_RET_WITH_GOTO(rc == 0, err);

    rc = mbedtls_ssl_set_hostname(&mtls->ssl, wurl->host);
    CHECK_RET_WITH_GOTO(rc == 0, err);

    mbedtls_ssl_set_bio(&mtls->ssl, &mtls->nctx, _mtls_net_send, NULL, _mtls_net_reado);

    LOGD(TAG, "now, start handshake...");
    rc = mbedtls_ssl_handshake(&mtls->ssl);
    if (rc != 0) {
        LOGE(TAG, "handshake error, rc = %d", rc);
        AV_ERRNO_SET(AV_ERRNO_NETWORK_FAILD);
        goto err;
    }

    LOGD(TAG, " ***tls handshake ok***\n    [ Protocol is %s ]\n    [ Ciphersuite is %s ]\n",
         mbedtls_ssl_get_version(&mtls->ssl), mbedtls_ssl_get_ciphersuite(&mtls->ssl));
    rc = mbedtls_ssl_get_record_expansion(&mtls->ssl);
    LOGD(TAG, "    [ Record expansion is %d ]", rc);

    return 0;
err:
    _mtls_session_deinit(mtls);

    return -1;
}
#endif

static int _read_resp_hdr(wsession_t *session, dync_buf_t *buf, int timeout_ms)
{
    int rc;
    char ch;
    char prev = 0;

    for (;;) {
        rc = wsession_read(session, &ch, 1, timeout_ms);
        CHECK_RET_WITH_RET(rc == 1, -1);

        if (ch == '\r')
            continue;
        if (ch == '\n' && prev == '\n')
            return 0;
        dync_buf_add_char(buf, ch);
        prev = ch;
    }

    return -1;
}

static int _parse_resp_hdr(wsession_t *session, char *str)
{
    int rc;
    char *ptr, *end;
    dict_t *d = &session->hdrs;

    rc = dict_init(d, 10);
    CHECK_RET_WITH_RET(rc == 0, -1);

    session->code = 0;
    rc = strncmp(str, "HTTP/", 5);
    CHECK_RET_WITH_GOTO(rc == 0, err);

    str += 5;
    while (*str != ' ') {
        str++;
    }
    while (*str == ' ')
        str++;

    while ((*str >= '0') && (*str <= '9')) {
        session->code *= 10;
        session->code += *str - '0';
        str++;
    }
    CHECK_RET_WITH_GOTO(session->code != 0, err);

    while (*str == ' ')
        str++;
    end = strchr(str, '\n');
    CHECK_RET_WITH_GOTO(end, err);

    session->phrase = strndup(str, end - str);
    CHECK_RET_WITH_GOTO(session->phrase, err);

    str = end + 1;
    while (*str) {
        end = strchr(str, '\n');
        ptr = strchr(str, ':');
        CHECK_RET_WITH_GOTO(end && ptr && (ptr < end), err);

        *ptr++ = 0;
        while (*ptr == ' ')
            ptr++;

        rc = dict_addn(d, str, ptr, end - ptr);
        CHECK_RET_WITH_GOTO(rc == 0, err);
        str = end + 1;
    }
    dict_shrink(d);

    return 0;
err:
    if (session->phrase) {
        av_freep(&session->phrase);
    }
    dict_uninit(d);

    return -1;
}

static int _dict_add_basic_auth(dict_t *c, const char *header, const char *user, const char *passwd)
{
#ifdef CONFIG_USING_TLS
    char *b64;
    char buf[256];
    size_t dlen, olen, slen;

    snprintf(buf, sizeof(buf), "%s:%s", user, passwd);
    slen = strlen(buf);
    dlen = slen * 3 / 2 + 4;
    b64  = av_zalloc(dlen);
    CHECK_RET_WITH_RET(b64, -1);

    mbedtls_base64_encode((unsigned char*)b64, dlen, &olen, (unsigned char *)buf, slen);

    snprintf(buf, sizeof(buf), "Basic %s", b64);
    av_free(b64);

    return dict_add(c, header, buf);
#else
    return -1;
#endif
}

/**
 * @brief  create a web session
 * @return NULL on err
 */
wsession_t* wsession_create()
{
#ifdef CONFIG_USING_TLS
    wsession_t *session    = NULL;
    struct mtls_session *mtls = NULL;

    session = av_zalloc(sizeof(wsession_t));
    mtls    = av_zalloc(sizeof(struct mtls_session));
    CHECK_RET_WITH_GOTO(session && mtls, err);

    session->tls = mtls;
    _wsession_init(session);

    return session;
err:
    av_free(mtls);
    av_free(session);
    return NULL;
#else
    wsession_t *session;

    session = av_zalloc(sizeof(wsession_t));
    if (session) {
        _wsession_init(session);
    }

    return session;
#endif
}

static int _http_open(wsession_t *session, web_url_t *wurl, int timeout_ms)
{
    struct addrinfo *res = NULL;
    const struct addrinfo hints = {
        .ai_socktype = SOCK_STREAM
    };
    fd_set fds;
    char port[16];
    struct sockaddr sa;
    struct timeval tv;
    int rc, flags, fd = -1, recnt = 0;

    snprintf(port, sizeof(port), "%d", wurl->port);
retry:
    rc = getaddrinfo(wurl->host, port, &hints, &res);
    if (rc != 0) {
        LOGE(TAG, "getaddrinfo fail, may be retry again later. rc = %d, host = %s, port = %s\n", rc, wurl->host, port);
        if (rc == EAI_AGAIN && recnt++ < 2) {
            aos_msleep(20);
            goto retry;
        }
        AV_ERRNO_SET(AV_ERRNO_DNS_FAILD);
        goto err;
    }

    memcpy(&sa, res->ai_addr, res->ai_addrlen);
    fd = socket(sa.sa_family, SOCK_STREAM, IPPROTO_TCP);
    CHECK_RET_WITH_GOTO(fd != -1, err);

    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, sizeof(tv));

    flags = fcntl(fd, F_GETFL, 0);
    rc = fcntl(fd, F_SETFL, O_NONBLOCK);
    CHECK_RET_WITH_GOTO(rc != -1, err);

    while (1) {
        rc = connect(fd, &sa, res->ai_addrlen);
        if (rc == 0) {
            break;
        } else {
            if (errno == EISCONN) {
                break;
            }
            if ((errno != EAGAIN) && (errno != EINPROGRESS)) {
                LOGE(TAG, "wsession_open connect fail. errno = %d", errno);
                AV_ERRNO_SET(AV_ERRNO_CONNECT_FAILD);
                goto err;
            }
        }

        while (1) {
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            rc = select(fd + 1, NULL, &fds, NULL, &tv);
            if (rc == -1) {
                if ((errno == EINTR) || (errno == EAGAIN)) {
                    aos_msleep(20);
                    continue;
                }
                LOGE(TAG, "wsession_open select fail. errno = %d", errno);
                AV_ERRNO_SET(AV_ERRNO_CONNECT_FAILD);
                goto err;
            } else if (rc == 1) {
                goto conn;
            } else if (rc == 0) {
                LOGE(TAG, "wsession_open select timeout. errno = %d", errno);
                AV_ERRNO_SET(AV_ERRNO_CONNECT_FAILD);
                goto err;
            }
        }
    }

conn:
    fcntl(session->fd, F_SETFL, flags);
    freeaddrinfo(res);

    session->fd  = fd;
    session->url = wurl;
    return 0;

err:
    if (res)
        freeaddrinfo(res);
    if (fd >= 0)
        close(fd);
    return -1;
}


/**
 * @brief  open the session by url(parse the url, create fd and connect)
 * @param  [in] session
 * @param  [in] url
 * @param  [in] timeout_ms
 * @return 0/-1
 */
int wsession_open(wsession_t *session, const char *url, int timeout_ms)
{
    int rc;
    web_url_t *wurl;

    if (!(session && url && (timeout_ms > 0))) {
        return -1;
    }

    wurl = web_url_new(url);
    CHECK_RET_WITH_GOTO(wurl, err);

    rc = dict_init(&session->hdrs, 10);
    CHECK_RET_WITH_GOTO(rc == 0, err);

#ifdef CONFIG_USING_TLS
    if (URL_IS_HTTPS(wurl))
        rc = _https_open(session, wurl, timeout_ms);
    else
        rc = _http_open(session, wurl, timeout_ms);
#else
    rc = _http_open(session, wurl, timeout_ms);
#endif
    CHECK_RET_WITH_GOTO(rc == 0, err);
    session->url = wurl;

    return 0;
err:
    dict_uninit(&session->hdrs);
    web_url_free(wurl);
    return -1;
}

/**
 * @brief  write data to session
 * @param  [in] session
 * @param  [in] buf
 * @param  [in] count
 * @param  [in] timeout_ms
 * @return -1 on error
 */
int wsession_write(wsession_t *session, const char *buf, size_t count, int timeout_ms)
{
    if (!(session && buf && count && (timeout_ms > 0))) {
        return -1;
    }

#ifdef CONFIG_USING_TLS
    int rc;
    struct mtls_session *mtls = session->tls;

    UNUSED(timeout_ms);
    if (URL_IS_HTTPS(session->url))
        rc = mbedtls_ssl_write(&mtls->ssl, (const unsigned char *)buf, count);
    else
        rc = sock_writen(session->fd, buf, count, timeout_ms);

    return rc;
#else
    return sock_writen(session->fd, buf, count, timeout_ms);
#endif
}

/**
 * @brief  read data from session
 * @param  [in] session
 * @param  [in] buf
 * @param  [in] count
 * @param  [in] timeout_ms
 * @return -1 on error
 */
int wsession_read(wsession_t *session, char *buf, size_t count, int timeout_ms)
{
    if (!(session && buf && count && (timeout_ms > 0))) {
        return -1;
    }

#ifdef CONFIG_USING_TLS
    int rc;
    struct mtls_session *mtls = session->tls;

    if (URL_IS_HTTPS(session->url)) {
        mbedtls_ssl_conf_read_timeout(&mtls->conf, timeout_ms);
        count = count >= MBEDTLS_SSL_MAX_CONTENT_LEN ? MBEDTLS_SSL_MAX_CONTENT_LEN / 2 : count;
        rc = mbedtls_ssl_read(&mtls->ssl, (unsigned char *)buf, count);
        //printf("====>>>count = %10d, rc = %10d, %d\n", count, rc, MBEDTLS_SSL_MAX_CONTENT_LEN);
        if (rc < 0) {
            LOGI(TAG, "ssl read may be eof: rc = 0x%x.", -rc);
            /* FIXME: patch for mbedtls. may be read eof normaly */
            rc = (rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) ? 0 : -1;
        }
    } else {
        rc = sock_readn(session->fd, buf, count, timeout_ms);
    }

    return rc;
#else
    return sock_readn(session->fd, buf, count, timeout_ms);
#endif
}

/**
 * @brief  send hdr of get/post request
 * @param  [in] session
 * @param  [in] method     : WEB_METHOD_GET/WEB_METHOD_POST
 * @param  [in] timeout_ms
 * @return 0/-1
 */
int wsession_send_hdr(wsession_t *session, int method, int timeout_ms)
{
    int rc, i, cnt;
    dync_buf_t dbuf;
    char *pmethod;
    dict_t *d;

    if (!(session && (timeout_ms > 0))) {
        return -1;
    }

    switch (method) {
    case WEB_METHOD_GET:
        pmethod = "GET";
        break;
    case WEB_METHOD_POST:
        pmethod = "POST";
        break;
    default:
        return -1;
    }

    d = &session->hdrs;
    if (session->url->user && session->url->passwd) {
        rc = _dict_add_basic_auth(d, "Authorization", session->url->user, session->url->passwd);
        CHECK_RET_WITH_RET(rc == 0, -1);
    }

    rc = dync_buf_init(&dbuf, 512, session->hdr_size_max);
    CHECK_RET_WITH_RET(rc == 0, -1);

    rc |= dync_buf_add_fmt(&dbuf, "%s %s HTTP/1.0\r\n", pmethod, session->url->path);
    rc |= dync_buf_add_fmt(&dbuf, "Host: %s\r\n", session->url->host);
    rc |= dync_buf_add_fmt(&dbuf, "User-Agent: %s\r\n", WEB_USER_AGENT);


    cnt = dict_count(d);
    for (i = 0; i < cnt; i++) {
        rc |= dync_buf_add_fmt(&dbuf, "%s: %s\r\n",  d->keyvals[i].key, d->keyvals[i].val);
    }
    rc |= dync_buf_add_string(&dbuf, "\r\n");
    CHECK_RET_WITH_GOTO(rc == 0, err);

    LOGD(TAG, "http request: \n%s\n", dbuf.data);
    rc = wsession_write(session, dbuf.data, dbuf.len, timeout_ms);
    CHECK_RET_WITH_GOTO(rc == dbuf.len, err);
    rc = 0;

    /* free hdrs before handle resp hdr */
    dict_uninit(d);

err:
    dync_buf_uninit(&dbuf);
    return rc;
}

/**
 * @brief  read and parse the header of response
 * @param  [in] session
 * @param  [in] timeout_ms
 * @return 0/-1
 */
int wsession_read_resp_hdr(wsession_t *session, int timeout_ms)
{
    int rc;
    dync_buf_t dbuf;

    if (!(session && (timeout_ms > 0))) {
        return -1;
    }

    rc = dync_buf_init(&dbuf, 512, session->hdr_size_max);
    CHECK_RET_WITH_RET(rc == 0, -1);

    rc = _read_resp_hdr(session, &dbuf, timeout_ms);
    CHECK_RET_WITH_GOTO(rc == 0, out);

    LOGD(TAG, "http response: \n%s\n", dbuf.data);
    rc = _parse_resp_hdr(session, dbuf.data);

out:
    dync_buf_uninit(&dbuf);
    return rc;
}

/**
 * @brief  close the websession and release resource related
 * @param  [in] session
 * @return 0/-1
 */
int wsession_close(wsession_t *session)
{
    if (session) {
        dict_uninit(&session->hdrs);
        web_url_free(session->url);
        av_free(session->phrase);

#ifdef CONFIG_USING_TLS
        _mtls_session_deinit((struct mtls_session*)session->tls);
#endif

        if (session->fd >= 0)
            close(session->fd);

        _wsession_init(session);
        return 0;
    }

    return -1;
}

/**
 * @brief  destroy a web session
 * @param  [in] session
 * @return 0/-1
 */
int wsession_destroy(wsession_t *session)
{
    if (session) {
        wsession_close(session);
#ifdef CONFIG_USING_TLS
        av_free(session->tls);
#endif
        av_free(session);
        return 0;
    }

    return -1;
}

#if 0
char *web_read_body(int fd, size_t *size, int timeout_ms)
{
    dync_buf_t dbuf;

    dync_buf_init(&dbuf, 16);
    while (1) {
        int count = 1023;
        int rc;

        dync_buf_increase(&dbuf, count);
        rc = read(fd, dbuf.data + dbuf.len, count);
        if (rc == -1) {
            dync_buf_uninit(&dbuf);
            return NULL;
        }
        dbuf.len += rc;
        if (rc == 0) {
            *size = dbuf.len;
            return dync_buf_get_data(&dbuf);
        }
    }
}
#endif


/**
 * @brief  open the url, send requst header and read/parse the resp header
 * @param  [in] session
 * @param  [in] url
 * @param  [in] redirect : the max times of redirect(3xx)
 * @return 0/-1
 */
int wsession_get(wsession_t *session, const char *url, int redirect)
{
    return wsession_get_range(session, url, redirect, -1, -1);
}

/**
 * @brief  open the url, send requst header and read/parse the resp header
 * @param  [in] session
 * @param  [in] url
 * @param  [in] redirect : the max times of redirect(3xx)
 * @param  [in] range_s  : start pos of the range request
 * @param  [in] range_e  : end pos of the range request. -1 means the end default
 * @return 0/-1
 */
int wsession_get_range(wsession_t *session, const char *url, int redirect, int range_s, int range_e)
{
    int rc;
    const char *val;
    char *r_url;
    char range_v[48];

    if (!(session && url)) {
        return -1;
    }

    rc = wsession_open(session, url, 3000);
    CHECK_RET_WITH_RET(rc == 0, -1);

    if (range_s >= 0) {
        if (range_e > 0) {
            snprintf(range_v, sizeof(range_v), "bytes=%d-%d", range_s, range_e);
        } else {
            snprintf(range_v, sizeof(range_v), "bytes=%d-", range_s);
        }
        wsession_hdrs_add(session, "Range", range_v);
    }

    rc = wsession_send_hdr(session, WEB_METHOD_GET, 3000);
    CHECK_RET_WITH_GOTO(rc == 0, err);

    rc = wsession_read_resp_hdr(session, 3000);
    CHECK_RET_WITH_GOTO(rc == 0, err);

    if (session->code != 200) {
        LOGI(TAG, "HTTP response: %d %s, url = %s", session->code, session->phrase, url);
    }
    switch (session->code) {
    case 200:
    /* Partial Content */
    case 206:
        return 0;
    case 301:
    case 302:
    case 303:
    case 307:
        val = dict_get_val(&session->hdrs, "location");
        CHECK_RET_WITH_GOTO(rc == 0, err);

        redirect++;
        CHECK_RET_WITH_GOTO(redirect < session->redirect_cnt_max, err);

        r_url = strdup(val);
        CHECK_RET_WITH_GOTO(r_url, err);

        wsession_close(session);

        rc = wsession_get_range(session, r_url, redirect, range_s, range_e);

        av_free(r_url);
        return rc;
    case 404:
        AV_ERRNO_SET(AV_ERRNO_FILE_NOT_FOUND);
        return -1;
    default:
        AV_ERRNO_SET(AV_ERRNO_HTTP_FAILD);
        return -1;
    }

err:
    wsession_close(session);
    return -1;
}
#endif


