/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __OUTPUT_ALL_H__
#define __OUTPUT_ALL_H__

#include "avutil/common.h"
#include "avutil/av_config.h"

__BEGIN_DECLS__

#define REGISTER_AV_AO(X, x)                                          \
    {                                                                    \
        extern int ao_register_##x();                                    \
        if (CONFIG_AV_AO_##X)                                         \
            ao_register_##x();                                           \
    }

/**
 * @brief  regist audio output for dummy
 * @return 0/-1
 */
int ao_register_dummy();

/**
 * @brief  regist audio output for alsa
 * @return 0/-1
 */
int ao_register_alsa();

/**
 * @brief  regist audio output for alsa_standard
 * @return 0/-1
 */
int ao_register_alsa_standard();

/**
 * @brief  regist all output
 * @return 0/-1
 */
static inline int ao_register_all()
{
#if defined(CONFIG_AV_AO_DUMMY)
    REGISTER_AV_AO(DUMMY, dummy);
#endif

#if defined(CONFIG_AV_AO_ALSA)
    REGISTER_AV_AO(ALSA, alsa);
#endif

#if defined(CONFIG_AV_AO_ALSA_STANDARD)
    REGISTER_AV_AO(ALSA_STANDARD, alsa_standard);
#endif

    return 0;
}

__END_DECLS__

#endif /* __OUTPUT_ALL_H__ */

