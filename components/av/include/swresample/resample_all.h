/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __RESAMPLE_ALL_H__
#define __RESAMPLE_ALL_H__

#include "avutil/common.h"
#include "avutil/av_config.h"
#include "swresample/resample.h"
#include "swresample/resample_all.h"

__BEGIN_DECLS__

#define REGISTER_RESAMPLER(X, x)                                          \
    {                                                                     \
        extern int resample_register_##x();                               \
        if (CONFIG_RESAMPLER_##X)                                         \
            resample_register_##x();                                      \
    }

/**
 * @brief  regist resample for ipc
 * @return 0/-1
 */
int resample_register_ipc();

/**
 * @brief  regist swresample for speex
 * @return 0/-1
 */
int resample_register_speex();

/**
 * @brief  regist resampler
 * @return 0/-1
 */
static inline int resample_register()
{
#if defined(CONFIG_RESAMPLER_IPC)
    REGISTER_RESAMPLER(IPC, ipc);
#endif
#if defined(CONFIG_RESAMPLER_SPEEX)
    REGISTER_RESAMPLER(SPEEX, speex);
#endif
    return 0;
}

__END_DECLS__

#endif /* __RESAMPLE_ALL_H__ */

