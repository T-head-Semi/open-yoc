/*
 * Dynamic data buffer
 * Copyright (c) 2007-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef KDBUSBUF_H
#define KDBUSBUF_H

#include "dbus_knife/common.h"

/* kdbus_buf::buf is a pointer to external data */
#define KDBUSBUF_FLAG_EXT_DATA BIT(0)

/*
 * Internal data structure for kdbus_buf. Please do not touch this directly from
 * elsewhere. This is only defined in header file to allow inline functions
 * from this file to access data.
 */
struct kdbus_buf {
    size_t size; /* total size of the allocated buffer */
    size_t used; /* length of data in the buffer */
    uint8_t *buf; /* pointer to the head of the buffer */
    unsigned int flags;
    /* optionally followed by the allocated buffer */
};


int kdbus_buf_resize(struct kdbus_buf **buf, size_t add_len);
struct kdbus_buf *kdbus_buf_alloc(size_t len);
struct kdbus_buf *kdbus_buf_alloc_ext_data(uint8_t *data, size_t len);
struct kdbus_buf *kdbus_buf_alloc_copy(const void *data, size_t len);
struct kdbus_buf *kdbus_buf_dup(const struct kdbus_buf *src);
void kdbus_buf_free(struct kdbus_buf *buf);
void kdbus_buf_clear_free(struct kdbus_buf *buf);
void *kdbus_buf_put(struct kdbus_buf *buf, size_t len);
struct kdbus_buf *kdbus_buf_concat(struct kdbus_buf *a, struct kdbus_buf *b);
struct kdbus_buf *kdbus_buf_zeropad(struct kdbus_buf *buf, size_t len);
void kdbus_buf_printf(struct kdbus_buf *buf, char *fmt, ...);
struct kdbus_buf *kdbus_buf_parse_bin(const char *buf);


/**
 * kdbus_buf_size - Get the currently allocated size of a kdbus_buf buffer
 * @buf: kdbus_buf buffer
 * Returns: Currently allocated size of the buffer
 */
static inline size_t kdbus_buf_size(const struct kdbus_buf *buf)
{
    return buf->size;
}

/**
 * kdbus_buf_len - Get the current length of a kdbus_buf buffer data
 * @buf: kdbus_buf buffer
 * Returns: Currently used length of the buffer
 */
static inline size_t kdbus_buf_len(const struct kdbus_buf *buf)
{
    return buf->used;
}

/**
 * kdbus_buf_tailroom - Get size of available tail room in the end of the buffer
 * @buf: kdbus_buf buffer
 * Returns: Tail room (in bytes) of available space in the end of the buffer
 */
static inline size_t kdbus_buf_tailroom(const struct kdbus_buf *buf)
{
    return buf->size - buf->used;
}

/**
 * kdbus_buf_head - Get pointer to the head of the buffer data
 * @buf: kdbus_buf buffer
 * Returns: Pointer to the head of the buffer data
 */
static inline const void *kdbus_buf_head(const struct kdbus_buf *buf)
{
    return buf->buf;
}

static inline const u8 *kdbus_buf_head_u8(const struct kdbus_buf *buf)
{
    return (const u8 *) kdbus_buf_head(buf);
}

/**
 * kdbus_buf_mhead - Get modifiable pointer to the head of the buffer data
 * @buf: kdbus_buf buffer
 * Returns: Pointer to the head of the buffer data
 */
static inline void *kdbus_buf_mhead(struct kdbus_buf *buf)
{
    return buf->buf;
}

static inline u8 *kdbus_buf_mhead_u8(struct kdbus_buf *buf)
{
    return (u8 *) kdbus_buf_mhead(buf);
}

static inline void kdbus_buf_put_u8(struct kdbus_buf *buf, u8 data)
{
    u8 *pos = (u8 *) kdbus_buf_put(buf, 1);
    *pos = data;
}

static inline void kdbus_buf_put_le16(struct kdbus_buf *buf, u16 data)
{
    u8 *pos = (u8 *) kdbus_buf_put(buf, 2);
    KDBUS_PUT_LE16(pos, data);
}

static inline void kdbus_buf_put_le32(struct kdbus_buf *buf, u32 data)
{
    u8 *pos = (u8 *) kdbus_buf_put(buf, 4);
    KDBUS_PUT_LE32(pos, data);
}

static inline void kdbus_buf_put_be16(struct kdbus_buf *buf, u16 data)
{
    u8 *pos = (u8 *) kdbus_buf_put(buf, 2);
    KDBUS_PUT_BE16(pos, data);
}

static inline void kdbus_buf_put_be24(struct kdbus_buf *buf, u32 data)
{
    u8 *pos = (u8 *) kdbus_buf_put(buf, 3);
    KDBUS_PUT_BE24(pos, data);
}

static inline void kdbus_buf_put_be32(struct kdbus_buf *buf, u32 data)
{
    u8 *pos = (u8 *) kdbus_buf_put(buf, 4);
    KDBUS_PUT_BE32(pos, data);
}

static inline void kdbus_buf_put_data(struct kdbus_buf *buf, const void *data,
                                      size_t len)
{
    if (data) {
        os_memcpy(kdbus_buf_put(buf, len), data, len);
    }
}

static inline void kdbus_buf_put_buf(struct kdbus_buf *dst,
                                     const struct kdbus_buf *src)
{
    kdbus_buf_put_data(dst, kdbus_buf_head(src), kdbus_buf_len(src));
}

static inline void kdbus_buf_set(struct kdbus_buf *buf, const void *data, size_t len)
{
    buf->buf = (u8 *) data;
    buf->flags = KDBUSBUF_FLAG_EXT_DATA;
    buf->size = buf->used = len;
}

static inline void kdbus_buf_put_str(struct kdbus_buf *dst, const char *str)
{
    kdbus_buf_put_data(dst, str, os_strlen(str));
}

#endif /* KDBUSBUF_H */
