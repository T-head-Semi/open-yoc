/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __AEF_ALL_H__
#define __AEF_ALL_H__

#include "avutil/common.h"
#include "avutil/av_config.h"

__BEGIN_DECLS__

#define REGISTER_AEFXER(X, x)                                          \
    {                                                                  \
        extern int aefx_register_##x();                                \
        if (CONFIG_AEFXER_##X)                                         \
            aefx_register_##x();                                       \
    }

/**
 * @brief  regist audio effecter for sona
 * @return 0/-1
 */
int aefx_register_sona();

/**
 * @brief  regist audio effecter for ipc
 * @return 0/-1
 */
int aefx_register_ipc();

/**
 * @brief  regist audio effecter
 * @return 0/-1
 */
static inline int aefx_register()
{
#if defined(CONFIG_AEFXER_SONA)
    REGISTER_AEFXER(SONA, sona);
#endif
#if defined(CONFIG_AEFXER_IPC)
    REGISTER_AEFXER(IPC, ipc);
#endif
    return 0;
}

__END_DECLS__

#endif /* __AEF_ALL_H__ */

