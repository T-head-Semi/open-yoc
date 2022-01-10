/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/url_parse.h"

/**
 * @brief  get item value from a url
 * @param  [in] url
 * @param  [in] item : name of the item
 * @param  [out] value
 * @param  [in] len  : length of the value
 * @return 0/-1
 */
int url_get_item_value(const char *url, const char *item, char *value, size_t len)
{
    /* url example: mem://addr=765432&len=1024&acodec=mp3 */
    char *p1, *p2;
    int item_len, cnt = 0;

    if (!(url && item && value && len)) {
        printf("param err\n");
        return -1;
    }

    item_len = strlen(item);
    p1 = strstr(url, item);
    if (p1) {
        p2 = p1 + item_len;
        if (p2[0] == '=') {
            p2++;
            p1 = p2;
            while((p2[0] != '\0') && (p2[0] != '&')) {
                p2++;
                cnt++;
            }

            if (((cnt > 0) && (len > cnt))) {
                strncpy(value, p1, cnt);
                value[cnt] = '\0';
                return 0;
            }
        }
    }

    return -1;
}

/**
 * @brief  get item int value from a url
 * @param  [in] url
 * @param  [in] item : name of the item
 * @param  [out] value
 * @return 0/-1
 */
int url_get_item_value_int(const char *url, const char *item, int *value)
{
    char str[32] = {0};

    if (0 == url_get_item_value(url, item, str, sizeof(str))) {
        *value = atoi(str);
        return 0;
    }

    return -1;
}

/**
 * @brief  get item long value from a url
 * @param  [in] url
 * @param  [in] item : name of the item
 * @param  [out] value
 * @return 0/-1
 */
int url_get_item_value_long(const char *url, const char *item, long *value)
{
    char str[32] = {0};

    if (0 == url_get_item_value(url, item, str, sizeof(str))) {
        *value = atol(str);
        return 0;
    }

    return -1;
}




