/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdlib.h>
#include <ctype.h>

#include "at_internal.h"

#define AT_STR_NR           15


int is_digit_str(const char *str)
{
    unsigned digit_len = 0;

    while (*str == ' ') {
        str++;
    }

    while (isdigit((int)(*str))) {
        str++, digit_len++;
    }

    while (*str == ' ') {
        str++;
    }

    return *str == '\0' && digit_len > 0;
}

int string2bytes(const char *src, int src_len, unsigned char *dst, int dst_size)
{
    int i;

    if (src ==  NULL || dst == NULL || src_len < 0 || dst_size < (src_len + 1) / 2) {
        return -1;
    }

    for (i = 0; i < src_len; i += 2) {
        if (*src >= '0' && *src <= '9') {
            *dst = (*src - '0') << 4;
        } else if (toupper(*src) >= 'A' && toupper(*src) <= 'F') {
            *dst = ((toupper(*src) - 'A') + 10) << 4;
        } else {
            return -1;
        }

        src++;

        if (*src >= '0' && *src <= '9') {
            *dst |= *src - '0';
        } else if (toupper(*src) >= 'A' && toupper(*src) <= 'F') {
            *dst |= (toupper(*src) - 'A') + 10;
        } else {
            return -1;
        }

        src++;
        dst++;
    }

    return src_len / 2;
}

static uint8_t is_print_str(const char *str)
{
    while (isprint((int)(*str))) {
        str++;
    }

    return *str == '\0';
}

static void recover_comma(char *data, unsigned int data_len)
{
    unsigned int  index = 0;

    for (; index < data_len; index++) {
        if (*(data + index) == '\0') {
            *(data + index) = ',';
        }
    }
}

static void recover_lr(char *data, unsigned int data_len)
{
    unsigned int  index = 0;

    for (; index < data_len; index++) {
        if (*(data + index) == '\0') {
            *(data + index) = '\r';
        }
    }
}

static void recover_qmark(char *data, unsigned int qmark[], unsigned int num)
{
    unsigned int  index = 0;

    for (; index < num; index++) {
        data[qmark[index]] = '"';
    }
}


/**
* This function used to count param in buf, separat by ','
* @param[in] buf
* @returns num of param or < 0 error
*/
int at_cnt_param(char *buf, uint32_t len)
{
    int mark_flag;
    int  nparam = 0;
    int i, j;
    char *p;
    char *sepp;

    p = buf;

    /* lonking for the number of ',', ignore ',' inside '"' */
    do {
        mark_flag = 0;

        if (*p == '"') {
            for (i = j = 0, ++p; p[j] != '\0'; i++, j++) {
                if (p[j] == '"') {
                    ++j;
                    mark_flag = 1;
                    break;
                }
            }

            if (mark_flag == 0) {
                return AT_ERR;
            }

            p[j - 1] = '\0';
            sepp = p + j;
        } else {
            sepp = p + strcspn(p, ",");
        }

        nparam++;
        p = sepp + 1;
    } while (sepp && *sepp == ',');

    return nparam;
}

/**
* This function used to get param buf, separat by ','
* @param[in] buf
* @returns num of param or < 0 error
*/
int at_parse_param_safe(char *fmt, char *buf, void **pval)
{
    char *p;
    char *sepp;
    char sepc;
    char *pstr[AT_STR_NR] = { 0 };
    int  flag, buf_data_len, at_data_len;
    int i, j;
    int  n = 0;
    int  nparam = 0;
    int  ret  = AT_OK;
    int  mark_flag = 0;
    unsigned int qmark_index = 0;
    unsigned int qmark[AT_STR_NR] = { 0 };

    /* remove spcae in head */
    while (*buf == ' ') {
        buf ++;
    }

    buf_data_len = strlen(buf);
    p = buf;

    for (i = 0; i < buf_data_len; i++) {
        if (p[i] == '\r') {
            p[i] = '\0';
            break;
        }
    }

    at_data_len = strlen(buf);

    /* check if str is printable */
    if (!is_print_str(p)) {
        return AT_ERR;
    }

    /* lonking for the number of ',', ignore ',' inside '"' */
    do {
        flag = 0;
        mark_flag = 0;

        if (*p == '"') {
            for (i = j = 0, ++p; p[j] != '\0'; i++, j++) {
                /* ignore """ inside string */
                if (p[j] == '"' && (p[j + 1] == '\n' || p[j + 1] == ',' || p[j + 1] == '\r' || p[j + 1] == '\0')) {
                    ++j;
                    mark_flag = 1;
                    break;
                }
            }

            if (mark_flag == 0) {
                recover_comma(buf, at_data_len);
                recover_lr(buf, buf_data_len);
                recover_qmark(buf, qmark, qmark_index);
                return AT_ERR;
            }

            if (j == 1) {
                flag = 1;
            }

            p[j - 1] = '\0';
            qmark[qmark_index++] = &p[j - 1] - buf;
            sepp = p + j;
        } else {
            sepp = p + strcspn(p, ",");

            if (p == sepp) {
                flag = 1;
            }
        }

        sepc = sepp[0];
        sepp[0] = '\0';

        if (flag == 1) {
            pstr[n++] = NULL;
        } else {
            pstr[n++] = p;
            nparam++;
        }

        p = sepp + 1;
    } while ((n < AT_STR_NR) && (sepc == ','));

    ret = nparam;
#if 0

    if (nparam < get_fmt_param_count(fmt)) {
        ret = nparam;
    } else if (nparam > get_fmt_param_count(fmt)) {
        ret = nparam;
    }

#endif

    /* convert the string params to the types that fmt Appointed */
    int param_count = 0;
    int param_str_index = 0;
    int param_val_index = 0;

    for (; fmt !=  NULL && param_str_index < nparam; param_str_index++) {
        char type;
        unsigned int  size;
        char str_param_size[6] = { 0 };
        int  param_size_index  = 0;

        while (*fmt == ' ') {
            fmt++;
        }

        if (*fmt == ',') {
            fmt++;
            continue;  /* ignore the param that is without type */
        }

        if (*fmt++ != '%') {
            ret = AT_ERR;
            break;
        }

        for (; param_size_index < sizeof(str_param_size) - 1; fmt++, param_size_index++) {
            if (!isdigit((int)(*fmt))) {
                break;
            }

            str_param_size[param_size_index] = *fmt;
        }

        type = *(fmt++);
        size = atoi(str_param_size);

        while (*fmt == ' ') {
            fmt++;    /* ignore the blanks */
        }

        fmt = (*fmt == ',' ? fmt + 1 : NULL);

        if (type == 'd') {
            if (size == 0 || size == 4) {
                if (pstr[param_str_index] != NULL) {
                    if (!is_digit_str(pstr[param_str_index])) {
                        ret = AT_ERR;
                        break;
                    }

                    *((long *)pval[param_val_index]) = (long)atoi(pstr[param_str_index]);
                    param_count++;
                }
            } else if (size == 1) {
                if (pstr[param_str_index] != NULL) {
                    if (!is_digit_str(pstr[param_str_index])) {
                        ret = AT_ERR;
                        break;
                    }

                    *((char *)pval[param_val_index]) = (char)atoi(pstr[param_str_index]);
                    param_count++;
                }
            } else if (size == 2) {
                if (pstr[param_str_index] != NULL) {
                    if (!is_digit_str(pstr[param_str_index])) {
                        ret = AT_ERR;
                        break;
                    }

                    *((short *)pval[param_val_index]) = (short)atoi(pstr[param_str_index]);
                    param_count++;
                }
            } else {
                break;
            }
        } else if (type == 's') {
            if (size == 0) {
                if (pstr[param_str_index] != NULL) {
                    strcpy((char *)pval[param_val_index], pstr[param_str_index]);
                    param_count++;
                }
            } else {
                if (size < strlen(pstr[param_str_index])) {
                    ret = AT_ERR;
                    break;
                }

                if (pstr[param_str_index] != NULL) {
                    strncpy((char *)pval[param_val_index], pstr[param_str_index], size - 1);
                    param_count++;
                }
            }
        } else {
            break;
        }

        param_val_index++;
    }

    recover_comma(buf, at_data_len);
    recover_lr(buf, buf_data_len);
    recover_qmark(buf, qmark, qmark_index);

    return ret;
}
