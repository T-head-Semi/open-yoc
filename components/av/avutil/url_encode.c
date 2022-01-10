/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

#include "avutil/url_encode.h"

#define NON_NUM '0'

#if 0
/* special encode table, don't need encode */
static const char _encode_table[] = {
    '!', '#', '$', '&', '\'', '(', ')', '*',
    '+', ',', '-', '.', '/',  ':', ';', '=',
    '?', '@', '[', ']', '_',  '~'
};
#else
static const char _encode_table[256] = {
    1,  1,  1,  1,   1,  1,  1,  1,   /* NUL SOH STX ETX  EOT ENQ ACK BEL */
    1,  1,  1,  1,   1,  1,  1,  1,   /* BS  HT  LF  VT   FF  CR  SO  SI  */
    1,  1,  1,  1,   1,  1,  1,  1,   /* DLE DC1 DC2 DC3  DC4 NAK SYN ETB */
    1,  1,  1,  1,   1,  1,  1,  1,   /* CAN EM  SUB ESC  FS  GS  RS  US  */
    1,  0,  1,  0,   0,  1,  0,  0,   /* SP  !   "   #    $   %   &   '   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* (   )   *   +    ,   -   .   /   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* 0   1   2   3    4   5   6   7   */
    0,  0,  0,  0,   1,  0,  1,  0,   /* 8   9   :   ;    <   =   >   ?   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* @   A   B   C    D   E   F   G   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* H   I   J   K    L   M   N   O   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* P   Q   R   S    T   U   V   W   */
    0,  0,  0,  0,   1,  0,  1,  0,   /* X   Y   Z   [    \   ]   ^   _   */
    1,  0,  0,  0,   0,  0,  0,  0,   /* `   a   b   c    d   e   f   g   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* h   i   j   k    l   m   n   o   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* p   q   r   s    t   u   v   w   */
    0,  0,  0,  1,   1,  1,  0,  1,   /* x   y   z   {    |   }   ~   DEL */

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
};
#endif

static int _hex2num(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;

    return NON_NUM;
}

static bool _needs_encode(const char *p)
{
    char c = *p;

    if (c == '%') {
        if (isxdigit((int)*(p + 1)) && isxdigit((int)*(p + 2)))
            return false;
        else
            return true;
    } else {
        return _encode_table[(int)c];
    }
}

/**
 * @brief  url string decode
 * @param  [in] src
 * @param  [in] src_size
 * @param  [in] dst
 * @param  [in] dst_size
 * @return -1 on error
 */
int url_decode(const char* src, const size_t src_size, char* dst, const size_t dst_size)
{
    int i, j = 0;
    char c, c1, c2;

    if (!(src && src_size && dst && dst_size)) {
        return -1;
    }

    for (i = 0; (i < src_size) && (j < dst_size); ++i) {
        c = src[i];
        switch (c) {
        case '+':
            dst[j++] = ' ';
            break;
        case '%':
            if (i + 2 < src_size) {
                c1 = _hex2num(src[i + 1]);
                c2 = _hex2num(src[i + 2]);
                if ((c1 != NON_NUM) && (c2 != NON_NUM))
                    dst[j++] = (char)((c1 << 4) | c2);
                i += 2;
                break;
            } else {
                break;
            }
        default:
            dst[j++] = c;
            break;
        }
    }

    dst[j] = 0;

    return j;
}

/**
 * @brief  encode the origin url
 * @param  [in] src
 * @param  [in] src_size
 * @param  [in] dst
 * @param  [in] dst_size
 * @return -1 on error
 */
int url_encode(const char* src, const size_t src_size, char* dst, const size_t dst_size)
{
    char c;
    int i, j = 0;

    if (!(src && src_size && dst && dst_size)) {
        return -1;
    }

    for (i = 0; (i < src_size) && (j < dst_size); ++i) {
        c = src[i];
        if (((c >= 'A') && (c <= 'Z')) ||
            ((c >= 'a') && (c <= 'z')) ||
            ((c >= '0') && (c <= '9'))) {
            dst[j++] = c;
        } else if (c == '.' || c == '-' || c == '_' || c == '*') {
            dst[j++] = c;
        } else if (c == ' ') {
            dst[j++] = '+';
        } else {
            if (j + 3 < dst_size) {
                sprintf(dst+j, "%%%02X", (unsigned char)c);
                j += 3;
            } else {
                return 0;
            }
        }
    }

    dst[j] = '\0';
    return j;
}

/**
 * @brief  encode the url(don't encode :/$#@, etc)
 * @param  [in] s
 * attention: need free the return val by caller
 * @return NULL on error
 */
char* url_encode2(const char* s)
{
    const char *p1;
    char *str, *p2;
    unsigned char c;
    int en_cnt = 0;
    int oldlen, newlen;

    for (p1 = s; *p1; p1++)
        if (_needs_encode(p1))
            ++en_cnt;

    if (!en_cnt)
        return strdup(s);

    oldlen = p1 - s;
    newlen = oldlen + 2 * en_cnt;
    str    = av_zalloc(newlen + 1);
    CHECK_RET_WITH_RET(str, NULL);

    p1 = s;
    p2 = str;
    while (*p1) {
        if (_needs_encode(p1)) {
            c = *p1++;
            sprintf(p2, "%%%02X", c);
            p2++;
        } else {
            *p2++ = *p1++;
        }
    }

    *p2 = '\0';
    return str;
}



