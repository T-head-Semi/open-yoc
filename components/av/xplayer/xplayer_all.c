/*
 * Copyright (C) 2018-2022 Alibaba Group Holding Limited
 */

#include "xplayer/xplayer_cls.h"
#include "xplayer/xplayer.h"
#include "xplayer/xplayer_all.h"

/**
 * @brief  regist xplayer for yoc
 * @return 0/-1
 */
int xplayer_register_yoc()
{
    extern const struct xplayer_cls xplayer_cls_yoc;
    return xplayer_cls_register(&xplayer_cls_yoc);
}

/**
 * @brief  regist xplayer for tplayer
 * @return 0/-1
 */
int xplayer_register_tplayer()
{
    extern const struct xplayer_cls xplayer_cls_tplayer;
    return xplayer_cls_register(&xplayer_cls_tplayer);
}


