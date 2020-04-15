/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __STREAM_H__
#define __STREAM_H__

#include "stream/stream_cls.h"
#include "stream/stream_all.h"

__BEGIN_DECLS__

typedef struct stream_header {
    char                      *url;             ///< url of the stream
    enum stream_mode          mode;
    irq_av_t                  irq;
    uint32_t                  rcv_timeout;      ///< ms. 0 use default & AOS_WAIT_FOREVER means wait forever
    uint32_t                  cache_size;       ///< size of the web cache. 0 use default
    uint32_t                  cache_percent;    ///< (0~100)start read for player when up to cache_percent. 0 use default
    get_decrypt_cb_t          get_dec_cb;       ///< used for get decrypt info
} sth_t;

/**
 * @brief  regist stream ops
 * @param  [in] ops
 * @return 0/-1
 */
int stream_ops_register(const struct stream_ops *ops);

/**
 * @brief  open a stream
 * @param  [in] sth
 * @return NULL on error
 */
stream_cls_t* stream_open(const sth_t *sth);

/**
 * @brief  read from a stream
 * @param  [in] o
 * @param  [in] buf
 * @param  [in] count
 * @return -1 when err happens
 */
int stream_read(stream_cls_t *o, uint8_t *buf, size_t count);

/**
 * @brief  write data to a stream
 * @param  [in] o
 * @param  [in] buf
 * @param  [in] count
 * @return -1 when err happens
 */
int stream_write(stream_cls_t *o, const uint8_t *buf, size_t count);

/**
 * @brief  seek to the position
 * @param  [in] o
 * @param  [in] offset
 * @param  [in] whence : SEEK_SET/SEEK_CUR/SEEK_END
 * @return 0/-1
 */
int stream_seek(stream_cls_t *o, int32_t offset, int whence);

/**
* @brief  skip offset of the stream
* @param  [in] o
* @param  [in] offset
* @return 0/-1
*/
int stream_skip(stream_cls_t *o, int32_t offset);

/**
 * @brief  close a stream
 * @param  [in] o
 * @return 0/-1
 */
int stream_close(stream_cls_t *o);

/**
 * @brief  control a stream
 * @param  [in] o
 * @param  [in] cmd : command
 * @param  [in] arg
 * @param  [in/out] arg_size
 * @return
 */
int stream_control(stream_cls_t *o, int cmd, void *arg, size_t *arg_size);

/**
 * @brief  stream support seek whether
 * @param  [in] o
 * @return 0/1
 */
int stream_is_seekable(stream_cls_t *o);

/**
 * @brief  stream is eof whether
 * @param  [in] o
 * @return 0/1
 */
int stream_is_eof(stream_cls_t *o);

/**
 * @brief  get the stream size
 * @param  [in] o
 * @return -1 on err
 */
int stream_get_size(stream_cls_t *o);

/**
 * @brief  get the url
 * @param  [in] o
 * @return
 */
const char* stream_get_url(stream_cls_t *o);

int stream_r8(stream_cls_t *o);
uint16_t stream_r16be(stream_cls_t *o);
uint32_t stream_r24be(stream_cls_t *o);
uint32_t stream_r32be(stream_cls_t *o);
uint64_t stream_r64be(stream_cls_t *o);
uint16_t stream_r16le(stream_cls_t *o);
uint32_t stream_r24le(stream_cls_t *o);
uint32_t stream_r32le(stream_cls_t *o);
uint64_t stream_r64le(stream_cls_t *o);

/**
 * @brief  get the stream cur pos
 * @param  [in] o
 * @return -1 on err
 */
int stream_tell(stream_cls_t *o);

__END_DECLS__

#endif /* __STREAM_H__ */
