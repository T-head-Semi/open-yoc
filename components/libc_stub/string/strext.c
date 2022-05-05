#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

size_t strcount(const char *haystack, const char *needle);

/**
* This function used to convert bytes to hexstring
* @param[out] buf store hexstr
* @param[in]  bytes need to convert
* @param[in]  bytes len
*/
char *bytes2hexstr(char *str, size_t str_size, void *hex, size_t len)
{
    static char * encode = "0123456789ABCDEF";

    size_t i;

    if ((2*len + 1) > str_size)
        return NULL;

    int j = 0;

    for (i = 0; i < len; i++) {
        unsigned char a = ((unsigned char *) hex)[i];
        str[j++] = encode[(a >> 4) & 0xf];
        str[j++] = encode[a & 0xf];
    }

    str[j] = 0;

    return str;
}

static int char2byte(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }

    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }

    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' +10;
    }

    return -1;
}

/**
* This function used to convert hexstring to bytes
* @param[in/out] buf with hex string, converted bytes will in this str also
* @returns len of bytes
*/
int hexstr2bytes(char *str1)
{
    int len = 0;
    uint8_t *str = (uint8_t *)str1;;

    while (isxdigit(*(str + len)) && isxdigit(*(str + len + 1))) {
        *str = char2byte(*(str + len)) * 16 + char2byte(*(str + len + 1));
        len ++;
        str ++;
    }

    return len;
}

int strsplit(char **array, size_t count, char *data, const char *delim)
{
    int i = 0;
    char *saveptr = NULL;

    char *next = strtok_r(data, delim, &saveptr);

    while (next && i < count) {
        array[i++] = next;
        next = strtok_r(NULL, delim , &saveptr);
    }

    return i;
}

char **strasplit(char *data, const char *delim, int *count)
{
    int i = strcount(data, delim) + 1;
    char **array = (char **) calloc(sizeof(char*), i);
    if (array)
        *count = strsplit(array, i, data, delim);

    return array;
}


int str2mac(const char *str, unsigned char mac[])
{
    int i, c;

    if (NULL == str || NULL == mac || strlen(str) < 12) {
        return -1;
    }

    for (i = 0; i < 6; i++) {
        if (*str == '-' || *str == ':') {
            str++;
        }

        if (*str >= '0' && *str <= '9') {
            c = (unsigned char)(*str++ - '0');
        } else if (*str >= 'a' && *str <= 'f') {
            c = (unsigned char)(*str++ - 'a') + 10;
        } else if (*str >= 'A' && *str <= 'F') {
            c = (unsigned char)(*str++ - 'A') + 10;
        } else {
            return -1;
        }

        c <<= 4;

        if (*str >= '0' && *str <= '9') {
            c |= (unsigned char)(*str++ - '0');
        } else if (*str >= 'a' && *str <= 'f') {
            c |= (unsigned char)(*str++ - 'a') + 10;
        } else {
            c |= (unsigned char)(*str++ - 'A') + 10;
        }

        mac[i] = (unsigned char) c;
    }

    return 0;
}

int isalnumstring(const char *str)
{
    if(str == NULL) {
        return 0;
    }

    const uint8_t *ch = (uint8_t *)str;

    while(*ch) {
        if (!isalnum(*ch)) {
            return 0;
        }
        ch ++;
    }

    return 1;
}

const char *strstri(const char *str, const char *substr)
{
    int len = strlen(substr);

    if (len == 0) {
        return NULL;
    }

    while(*str) {
        if (strncasecmp(str, substr, len) == 0) {
            return str;
        }
        ++str;
    }
    return NULL;
}
