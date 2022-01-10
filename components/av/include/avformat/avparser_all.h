/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __AVPARSER_ALL_H__
#define __AVPARSER_ALL_H__

#include "avutil/common.h"

__BEGIN_DECLS__

#define REGISTER_AVPARSER(X, x)                                         \
    {                                                                   \
        extern int avparser_register_##x();                             \
        if (CONFIG_AVPARSER_##X)                                        \
            avparser_register_##x();                                    \
    }

/**
 * @brief  regist avparser for mp3
 * @return 0/-1
 */
int avparser_register_mp3();

/**
 * @brief  regist avparser for adts
 * @return 0/-1
 */
int avparser_register_adts();

/**
 * @brief  regist all avparser
 * @return 0/-1
 */
static inline int avparser_register_all()
{
#if defined(CONFIG_AVPARSER_MP3)
    REGISTER_AVPARSER(MP3, mp3);
#endif
#if defined(CONFIG_AVPARSER_ADTS)
    REGISTER_AVPARSER(ADTS, adts);
#endif

    return 0;
}

__END_DECLS__

#endif /* __AVPARSER_ALL_H__ */

