/*
 * Dynamic data buffer
 * Copyright (c) 2007-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "dbus_knife/kdbus_buf.h"
#include <stdarg.h>

#define kdbus_printf(...)

static void *zalloc(size_t size)
{
    void *n = malloc(size);

    if (n) {
        memset(n, 0, size);
    }

    return n;
}

static int hex2num(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return -1;
}


int hex2byte(const char *hex)
{
    int a, b;
    a = hex2num(*hex++);

    if (a < 0) {
        return -1;
    }

    b = hex2num(*hex++);

    if (b < 0) {
        return -1;
    }

    return (a << 4) | b;
}


int hexstr2bin(const char *hex, u8 *buf, size_t len)
{
    size_t i;
    int a;
    const char *ipos = hex;
    u8 *opos = buf;

    for (i = 0; i < len; i++) {
        a = hex2byte(ipos);

        if (a < 0) {
            return -1;
        }

        *opos++ = a;
        ipos += 2;
    }

    return 0;
}

#ifdef KDBUS_TRACE
#define KDBUSBUF_MAGIC 0x51a974e3

struct kdbus_buf_trace {
    unsigned int magic;
} __attribute__((aligned(8)));

static struct kdbus_buf_trace *kdbus_buf_get_trace(const struct kdbus_buf *buf)
{
    return (struct kdbus_buf_trace *)
           ((const u8 *) buf - sizeof(struct kdbus_buf_trace));
}
#endif /* KDBUS_TRACE */


static void kdbus_buf_overflow(const struct kdbus_buf *buf, size_t len)
{
#ifdef KDBUS_TRACE
    struct kdbus_buf_trace *trace = kdbus_buf_get_trace(buf);

    if (trace->magic != KDBUSBUF_MAGIC) {
        kdbus_printf(MSG_ERROR, "kdbus_buf: invalid magic %x",
                     trace->magic);
    }

#endif /* KDBUS_TRACE */
    kdbus_printf(MSG_ERROR, "kdbus_buf %p (size=%lu used=%lu) overflow len=%lu",
                 buf, (unsigned long) buf->size, (unsigned long) buf->used,
                 (unsigned long) len);
    printf("kdbus_buf overflow\n");
    abort();
}


int kdbus_buf_resize(struct kdbus_buf **_buf, size_t add_len)
{
    struct kdbus_buf *buf = *_buf;
#ifdef KDBUS_TRACE
    struct kdbus_buf_trace *trace;
#endif /* KDBUS_TRACE */

    if (buf == NULL) {
        *_buf = kdbus_buf_alloc(add_len);
        return *_buf == NULL ? -1 : 0;
    }

#ifdef KDBUS_TRACE
    trace = kdbus_buf_get_trace(buf);

    if (trace->magic != KDBUSBUF_MAGIC) {
        kdbus_printf(MSG_ERROR, "kdbus_buf: invalid magic %x",
                     trace->magic);
        kdbus_trace_show("kdbus_buf_resize invalid magic");
        abort();
    }

#endif /* KDBUS_TRACE */

    if (buf->used + add_len > buf->size) {
        unsigned char *nbuf;

        if (buf->flags & KDBUSBUF_FLAG_EXT_DATA) {
            nbuf = os_realloc(buf->buf, buf->used + add_len);

            if (nbuf == NULL) {
                return -1;
            }

            os_memset(nbuf + buf->used, 0, add_len);
            buf->buf = nbuf;
        } else {
#ifdef KDBUS_TRACE
            nbuf = os_realloc(trace, sizeof(struct kdbus_buf_trace) +
                              sizeof(struct kdbus_buf) +
                              buf->used + add_len);

            if (nbuf == NULL) {
                return -1;
            }

            trace = (struct kdbus_buf_trace *) nbuf;
            buf = (struct kdbus_buf *)(trace + 1);
            os_memset(nbuf + sizeof(struct kdbus_buf_trace) +
                      sizeof(struct kdbus_buf) + buf->used, 0,
                      add_len);
#else /* KDBUS_TRACE */
            nbuf = realloc(buf, sizeof(struct kdbus_buf) +
                           buf->used + add_len);

            if (nbuf == NULL) {
                return -1;
            }

            buf = (struct kdbus_buf *) nbuf;
            os_memset(nbuf + sizeof(struct kdbus_buf) + buf->used, 0,
                      add_len);
#endif /* KDBUS_TRACE */
            buf->buf = (u8 *)(buf + 1);
            *_buf = buf;
        }

        buf->size = buf->used + add_len;
    }

    return 0;
}


/**
 * kdbus_buf_alloc - Allocate a kdbus_buf of the given size
 * @len: Length for the allocated buffer
 * Returns: Buffer to the allocated kdbus_buf or %NULL on failure
 */
struct kdbus_buf *kdbus_buf_alloc(size_t len)
{
#ifdef KDBUS_TRACE
    struct kdbus_buf_trace *trace = zalloc(sizeof(struct kdbus_buf_trace) +
                                           sizeof(struct kdbus_buf) + len);
    struct kdbus_buf *buf;

    if (trace == NULL) {
        return NULL;
    }

    trace->magic = KDBUSBUF_MAGIC;
    buf = (struct kdbus_buf *)(trace + 1);
#else /* KDBUS_TRACE */
    struct kdbus_buf *buf = zalloc(sizeof(struct kdbus_buf) + len);

    if (buf == NULL) {
        return NULL;
    }

#endif /* KDBUS_TRACE */

    buf->size = len;
    buf->buf = (u8 *)(buf + 1);
    return buf;
}


struct kdbus_buf *kdbus_buf_alloc_ext_data(u8 *data, size_t len)
{
#ifdef KDBUS_TRACE
    struct kdbus_buf_trace *trace = zalloc(sizeof(struct kdbus_buf_trace) +
                                           sizeof(struct kdbus_buf));
    struct kdbus_buf *buf;

    if (trace == NULL) {
        return NULL;
    }

    trace->magic = KDBUSBUF_MAGIC;
    buf = (struct kdbus_buf *)(trace + 1);
#else /* KDBUS_TRACE */
    struct kdbus_buf *buf = zalloc(sizeof(struct kdbus_buf));

    if (buf == NULL) {
        return NULL;
    }

#endif /* KDBUS_TRACE */

    buf->size = len;
    buf->used = len;
    buf->buf = data;
    buf->flags |= KDBUSBUF_FLAG_EXT_DATA;

    return buf;
}


struct kdbus_buf *kdbus_buf_alloc_copy(const void *data, size_t len)
{
    struct kdbus_buf *buf = kdbus_buf_alloc(len);

    if (buf) {
        kdbus_buf_put_data(buf, data, len);
    }

    return buf;
}


struct kdbus_buf *kdbus_buf_dup(const struct kdbus_buf *src)
{
    struct kdbus_buf *buf = kdbus_buf_alloc(kdbus_buf_len(src));

    if (buf) {
        kdbus_buf_put_data(buf, kdbus_buf_head(src), kdbus_buf_len(src));
    }

    return buf;
}


/**
 * kdbus_buf_free - Free a kdbus_buf
 * @buf: kdbus_buf buffer
 */
void kdbus_buf_free(struct kdbus_buf *buf)
{
#ifdef KDBUS_TRACE
    struct kdbus_buf_trace *trace;

    if (buf == NULL) {
        return;
    }

    trace = kdbus_buf_get_trace(buf);

    if (trace->magic != KDBUSBUF_MAGIC) {
        kdbus_printf(MSG_ERROR, "kdbus_buf_free: invalid magic %x",
                     trace->magic);
        kdbus_trace_show("kdbus_buf_free magic mismatch");
        abort();
    }

    if (buf->flags & KDBUSBUF_FLAG_EXT_DATA) {
        os_free(buf->buf);
    }

    os_free(trace);
#else /* KDBUS_TRACE */

    if (buf == NULL) {
        return;
    }

    if (buf->flags & KDBUSBUF_FLAG_EXT_DATA) {
        os_free(buf->buf);
    }

    os_free(buf);
#endif /* KDBUS_TRACE */
}


void kdbus_buf_clear_free(struct kdbus_buf *buf)
{
    if (buf) {
        os_memset(kdbus_buf_mhead(buf), 0, kdbus_buf_len(buf));
        kdbus_buf_free(buf);
    }
}


void *kdbus_buf_put(struct kdbus_buf *buf, size_t len)
{
    void *tmp = kdbus_buf_mhead_u8(buf) + kdbus_buf_len(buf);
    buf->used += len;

    if (buf->used > buf->size) {
        kdbus_buf_overflow(buf, len);
    }

    return tmp;
}


/**
 * kdbus_buf_concat - Concatenate two buffers into a newly allocated one
 * @a: First buffer
 * @b: Second buffer
 * Returns: kdbus_buf with concatenated a + b data or %NULL on failure
 *
 * Both buffers a and b will be freed regardless of the return value. Input
 * buffers can be %NULL which is interpreted as an empty buffer.
 */
struct kdbus_buf *kdbus_buf_concat(struct kdbus_buf *a, struct kdbus_buf *b)
{
    struct kdbus_buf *n = NULL;
    size_t len = 0;

    if (b == NULL) {
        return a;
    }

    if (a) {
        len += kdbus_buf_len(a);
    }

    len += kdbus_buf_len(b);

    n = kdbus_buf_alloc(len);

    if (n) {
        if (a) {
            kdbus_buf_put_buf(n, a);
        }

        kdbus_buf_put_buf(n, b);
    }

    kdbus_buf_free(a);
    kdbus_buf_free(b);

    return n;
}


/**
 * kdbus_buf_zeropad - Pad buffer with 0x00 octets (prefix) to specified length
 * @buf: Buffer to be padded
 * @len: Length for the padded buffer
 * Returns: kdbus_buf padded to len octets or %NULL on failure
 *
 * If buf is longer than len octets or of same size, it will be returned as-is.
 * Otherwise a new buffer is allocated and prefixed with 0x00 octets followed
 * by the source data. The source buffer will be freed on error, i.e., caller
 * will only be responsible on freeing the returned buffer. If buf is %NULL,
 * %NULL will be returned.
 */
struct kdbus_buf *kdbus_buf_zeropad(struct kdbus_buf *buf, size_t len)
{
    struct kdbus_buf *ret;
    size_t blen;

    if (buf == NULL) {
        return NULL;
    }

    blen = kdbus_buf_len(buf);

    if (blen >= len) {
        return buf;
    }

    ret = kdbus_buf_alloc(len);

    if (ret) {
        os_memset(kdbus_buf_put(ret, len - blen), 0, len - blen);
        kdbus_buf_put_buf(ret, buf);
    }

    kdbus_buf_free(buf);

    return ret;
}


void kdbus_buf_printf(struct kdbus_buf *buf, char *fmt, ...)
{
    va_list ap;
    void *tmp = kdbus_buf_mhead_u8(buf) + kdbus_buf_len(buf);
    int res;

    va_start(ap, fmt);
    res = vsnprintf(tmp, buf->size - buf->used, fmt, ap);
    va_end(ap);

    if (res < 0 || (size_t) res >= buf->size - buf->used) {
        kdbus_buf_overflow(buf, res);
    }

    buf->used += res;
}


/**
 * kdbus_buf_parse_bin - Parse a null terminated string of binary data to a kdbus_buf
 * @buf: Buffer with null terminated string (hexdump) of binary data
 * Returns: kdbus_buf or %NULL on failure
 *
 * The string len must be a multiple of two and contain only hexadecimal digits.
 */
struct kdbus_buf *kdbus_buf_parse_bin(const char *buf)
{
    size_t len;
    struct kdbus_buf *ret;

    len = os_strlen(buf);

    if (len & 0x01) {
        return NULL;
    }

    len /= 2;

    ret = kdbus_buf_alloc(len);

    if (ret == NULL) {
        return NULL;
    }

    if (hexstr2bin(buf, kdbus_buf_put(ret, len), len)) {
        kdbus_buf_free(ret);
        return NULL;
    }

    return ret;
}
