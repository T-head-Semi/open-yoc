/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __AVFORMAT_ALL_H__
#define __AVFORMAT_ALL_H__

#include "avutil/common.h"
#include "avutil/av_config.h"

__BEGIN_DECLS__

#define REGISTER_DEMUXER(X, x)                                          \
    {                                                                   \
        extern int demux_register_##x();                                \
        if (CONFIG_DEMUXER_##X)                                         \
            demux_register_##x();                                       \
    }

/**
 * @brief  regist demux for wav
 * @return 0/-1
 */
int demux_register_wav();

/**
 * @brief  regist demux for mp3
 * @return 0/-1
 */
int demux_register_mp3();

/**
 * @brief  regist demux for mp4
 * @return 0/-1
 */
int demux_register_mp4();

/**
 * @brief  regist demux for rawaudio
 * @return 0/-1
 */
int demux_register_rawaudio();

/**
 * @brief  regist demux for adts
 * @return 0/-1
 */
int demux_register_adts();

/**
 * @brief  regist demux for flac
 * @return 0/-1
 */
int demux_register_flac();

/**
 * @brief  regist demux for asf
 * @return 0/-1
 */
int demux_register_asf();

/**
 * @brief  regist demux for amr
 * @return 0/-1
 */
int demux_register_amr();

/**
 * @brief  regist demux for ts
 * @return 0/-1
 */
int demux_register_ts();

/**
 * @brief  regist demux for ogg
 * @return 0/-1
 */
int demux_register_ogg();

/**
 * @brief  regist all demuxer
 * @return 0/-1
 */
static inline int demux_register_all()
{
#if defined(CONFIG_DEMUXER_WAV)
    REGISTER_DEMUXER(WAV, wav);
#endif
#if defined(CONFIG_DEMUXER_MP3)
    REGISTER_DEMUXER(MP3, mp3);
#endif
#if defined(CONFIG_DEMUXER_MP4) && defined(CONFIG_USING_TLS)
    REGISTER_DEMUXER(MP4, mp4);
#endif
#if defined(CONFIG_DEMUXER_ADTS)
    REGISTER_DEMUXER(ADTS, adts);
#endif
#if defined(CONFIG_DEMUXER_RAWAUDIO)
    REGISTER_DEMUXER(RAWAUDIO, rawaudio);
#endif
#if defined(CONFIG_DEMUXER_FLAC)
    REGISTER_DEMUXER(FLAC, flac);
#endif
#if defined(CONFIG_DEMUXER_ASF)
    REGISTER_DEMUXER(ASF, asf);
#endif
#if defined(CONFIG_DEMUXER_AMR)
    REGISTER_DEMUXER(AMR, amr);
#endif
#if defined(CONFIG_DEMUXER_TS)
    REGISTER_DEMUXER(TS, ts);
#endif
#if defined(CONFIG_DEMUXER_OGG)
    REGISTER_DEMUXER(OGG, ogg);
#endif

    return 0;
}

__END_DECLS__

#endif /* __AVFORMAT_ALL_H__ */

