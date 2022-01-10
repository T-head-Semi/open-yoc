/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/url_encode.h"
#include "avutil/web_url.h"

/**
 * @brief  create web url, parse the url
 * @param  [in] url
 *         url format: http(s)://[[user:passwd]@]host[:port][/path][?query]
 * @return NULL on err
 */
web_url_t* web_url_new(const char *url)
{
    web_url_t *wurl;
    int port, is_https = 0;
    const char *start, *str, *colon, *at, *path_start, *host_start;

    if (!url) {
        return NULL;
    }

    if (0 == strncmp(url, "http://", 7)) {
        port = 80;
    } else if (0 == strncmp(url, "https://", 7)) {
        port     = 443;
        is_https = 1;
    } else {
        return NULL;
    }

    wurl = av_zalloc(sizeof(web_url_t));
    CHECK_RET_WITH_GOTO(wurl, err);

    wurl->port     = port;
    wurl->is_https = is_https;
    wurl->url      = url_encode2(url);
    CHECK_RET_WITH_GOTO(wurl->url, err);

    str = is_https ? url + 8 : url + 7;
    host_start = str;

    path_start = strchr(str, '/');
    if (path_start) {
        wurl->path = strdup(path_start);
    } else {
        wurl->path = strdup("/");
    }
    CHECK_RET_WITH_GOTO(wurl->path, err);

    at = strchr(str, '@');
    if (at && (!path_start || (at < path_start))) {
        host_start = at + 1;
        colon = strchr(str, ':');
        CHECK_RET_WITH_GOTO(colon, err);

        wurl->user = strndup(str, colon - str);
        CHECK_RET_WITH_GOTO(wurl->user, err);
        CHECK_RET_WITH_GOTO(strlen(wurl->user) < URL_USERNAME_LEN_MAX, err);

        wurl->passwd = strndup(colon + 1, at - (colon + 1));
        CHECK_RET_WITH_GOTO(wurl->passwd, err);
        CHECK_RET_WITH_GOTO(strlen(wurl->passwd) < URL_PASSWD_LEN_MAX, err);
    }

    colon = strchr(host_start, ':');
    if (colon && (!path_start || (colon < path_start))) {
        wurl->host = strndup(host_start, colon - host_start);
        CHECK_RET_WITH_GOTO(wurl->host, err);
        colon++;
        start = colon;

        port = 0;
        while (*colon >= '0' && *colon <= '9') {
            port *= 10;
            port += *colon - '0';
            colon++;
        }
        wurl->port = port;

        if ((colon == start) || (!((*colon == '\0') || (*colon == '/')))) {
            goto err;
        }
    } else {
        /* host */
        if (path_start) {
            wurl->host = strndup(host_start, path_start - host_start);
        } else {
            wurl->host = strdup(host_start);
        }
        CHECK_RET_WITH_GOTO(wurl->host, err);
    }

    return wurl;

err:
    if (wurl) {
        av_free(wurl->url);
        av_free(wurl->host);
        av_free(wurl->path);
        av_free(wurl->user);
        av_free(wurl->passwd);

        av_free(wurl);
    }
    return NULL;
}

/**
 * @brief  free web url
 * @param  [in] url
 * @return 0/-1
 */
int web_url_free(web_url_t *url)
{
    if (!url) {
        return -1;
    }

    av_freep(&url->url);
    av_freep(&url->host);
    av_freep(&url->path);

    av_freep(&url->user);
    av_freep(&url->passwd);

    av_free(url);

    return 0;
}



