/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __AVCODEC_ALL_H__
#define __AVCODEC_ALL_H__

#include "avutil/av_config.h"
#include "avcodec/ad.h"
#include "avcodec/avcodec.h"

__BEGIN_DECLS__

#define REGISTER_DECODER(X, x)                                          \
    {                                                                   \
        extern int ad_register_##x();                                   \
        if (CONFIG_DECODER_##X)                                         \
            ad_register_##x();                                          \
    }

/**
 * @brief  regist ad for pcm
 * @return 0/-1
 */
int ad_register_pcm();

/**
 * @brief  regist ad for ipc
 * @return 0/-1
 */
int ad_register_ipc();

/**
 * @brief  regist ad for pvmp3
 * @return 0/-1
 */
int ad_register_pvmp3();

/**
 * @brief  regist ad for adpcm_ms
 * @return 0/-1
 */
int ad_register_adpcm_ms();

/**
 * @brief  regist ad for flac
 * @return 0/-1
 */
int ad_register_flac();

/**
 * @brief  regist ad for amrnb
 * @return 0/-1
 */
int ad_register_amrnb();

/**
 * @brief  regist ad for amrwb
 * @return 0/-1
 */
int ad_register_amrwb();

/**
 * @brief  regist ad for opus
 * @return 0/-1
 */
int ad_register_opus();

/**
 * @brief  regist ad for speex
 * @return 0/-1
 */
int ad_register_speex();

/**
 * @brief  regist ad for alaw
 * @return 0/-1
 */
int ad_register_alaw();

/**
 * @brief  regist ad for mulaw
 * @return 0/-1
 */
int ad_register_mulaw();

/**
 * @brief  regist all decoder
 * @return 0/-1
 */
static inline int ad_register_all()
{
#if defined(CONFIG_DECODER_PCM)
    REGISTER_DECODER(PCM, pcm);
#endif
#if defined(CONFIG_DECODER_PVMP3)
    REGISTER_DECODER(PVMP3, pvmp3);
#endif
#if defined(CONFIG_DECODER_ADPCM_MS)
    REGISTER_DECODER(ADPCM_MS, adpcm_ms);
#endif
#if defined(CONFIG_DECODER_FLAC)
    REGISTER_DECODER(FLAC, flac);
#endif
#if defined(CONFIG_DECODER_AMRNB)
    REGISTER_DECODER(AMRNB, amrnb);
#endif
#if defined(CONFIG_DECODER_AMRWB)
    REGISTER_DECODER(AMRWB, amrwb);
#endif
#if defined(CONFIG_DECODER_OPUS)
    REGISTER_DECODER(OPUS, opus);
#endif
#if defined(CONFIG_DECODER_SPEEX)
    REGISTER_DECODER(SPEEX, speex);
#endif
#if defined(CONFIG_DECODER_ALAW)
    REGISTER_DECODER(ALAW, alaw);
#endif
#if defined(CONFIG_DECODER_MULAW)
    REGISTER_DECODER(MULAW, mulaw);
#endif
#if defined(CONFIG_DECODER_IPC)
    REGISTER_DECODER(IPC, ipc);
#endif

    return 0;
}

__END_DECLS__

#endif /* __AVCODEC_ALL_H__ */

