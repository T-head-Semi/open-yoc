/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef __RINGBUFFER__
#define __RINGBUFFER__

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *buffer;
    uint32_t length;
    uint32_t ridx;
    uint32_t widx;
} dev_ringbuf_t;

/**
 * @brief  create a ring buffer
 * @param[in] length length space of the ring buffer
 * @return  pointer to ring buffer
 */
int ringbuffer_create(dev_ringbuf_t *ringbuffer, char *buffer, int length);

/**
 * @brief   destroy the ring buffer
 * @param[in] buffer pointer to the ring buffer
 * @return  null
 */
int ringbuffer_destroy(dev_ringbuf_t *buffer);

/**
 * @brief   read data from ring buffer.
 * @param[in] buffer pointer to the ring buffer
 * @param[in] target pointer to buffer for data to read from ring buffer
 * @param[in] amount amount of data items to read
 * @return  number of actual read data
 */
int ringbuffer_read(dev_ringbuf_t *buffer, uint8_t *target, uint32_t amount);

/**
 * @brief   write data to ring buffer
 * @param[in] buffer pointer to the ring buffer
 * @param[in] data pointer to buffer for data to write to ring buffer
 * @param[in] length length of data items to write
 * @return  0 for success
 */
int ringbuffer_write(dev_ringbuf_t *buffer, uint8_t *data, uint32_t length);

/**
 * @brief   is the ring buffer empty?
 * @param[in] buffer pointer to the ring buffer
 * @return  0 for success
 */
int ringbuffer_empty(dev_ringbuf_t *buffer);
/**
 * @brief   is the ring buffer full?
 * @param[in] buffer pointer to the ring buffer
 * @return  0 for success
 */
int ringbuffer_full(dev_ringbuf_t *buffer);
/**
 * @brief   available write space to ring buffer
 * @param[in] buffer pointer to the ring buffer
 * @return   number of write spcae
 */
int ringbuffer_available_write_space(dev_ringbuf_t *buffer);
/**
 * @brief    available read space to ring buffer
 * @param[in] buffer pointer to the ring buffer
 * @return  number of read spcae
 */
int ringbuffer_available_read_space(dev_ringbuf_t *buffer);

#define ringbuffer_available_write_space(B) (\
        (B)->length - ringbuffer_available_read_space(B))

#define ringbuffer_full(B) (ringbuffer_available_write_space(B) == 0)

#define ringbuffer_empty(B) (ringbuffer_available_read_space((B)) == 0)

#define ringbuffer_clear(B) ((B)->widx = (B)->ridx = 0)

#ifdef __cplusplus
}
#endif

#endif

