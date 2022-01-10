/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __STREAM_ALL_H__
#define __STREAM_ALL_H__

#include "avutil/common.h"
#include "avutil/av_config.h"

__BEGIN_DECLS__

#define REGISTER_STREAMER(X, x)                                          \
    {                                                                    \
        extern int stream_register_##x();                                \
        if (CONFIG_STREAMER_##X)                                         \
            stream_register_##x();                                       \
    }

/**
 * @brief  regist stream for memory
 * @return 0/-1
 */
int stream_register_mem();

/**
 * @brief  regist stream for file
 * @return 0/-1
 */
int stream_register_file();

/**
 * @brief  regist stream for http
 * @return 0/-1
 */
int stream_register_http();

/**
 * @brief  regist stream for fifo
 * @return 0/-1
 */
int stream_register_fifo();

/**
 * @brief  regist stream for crypto
 * @return 0/-1
 */
int stream_register_crypto();

/**
 * @brief  regist stream for hls
 * @return 0/-1
 */
int stream_register_hls();

/**
 * @brief  regist all streamer
 * @return 0/-1
 */
static inline int stream_register_all()
{
#if defined(CONFIG_STREAMER_MEM)
    REGISTER_STREAMER(MEM, mem);
#endif
#if defined(CONFIG_STREAMER_FILE)
    REGISTER_STREAMER(FILE, file);
#endif
#if defined(CONFIG_STREAMER_HTTP)
    REGISTER_STREAMER(HTTP, http);
#endif
#if defined(CONFIG_STREAMER_FIFO)
    REGISTER_STREAMER(FIFO, fifo);
#endif
#if defined(CONFIG_STREAMER_CRYPTO) && defined(CONFIG_USING_TLS)
    REGISTER_STREAMER(CRYPTO, crypto);
#endif
#if defined(CONFIG_STREAMER_HLS)
    REGISTER_STREAMER(HLS, hls);
#endif

    return 0;
}

__END_DECLS__

#endif /* __STREAM_ALL_H__ */

