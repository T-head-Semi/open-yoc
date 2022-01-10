/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : log.h
 * Description : Log
 * History :
 *
 */

#ifndef LOG_H
#define LOG_H

enum CDX_LOG_LEVEL_TYPE {
    LOG_LEVEL_VERBOSE = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_INFO = 4,
    LOG_LEVEL_WARNING = 5,
    LOG_LEVEL_ERROR = 6,
};

extern enum CDX_LOG_LEVEL_TYPE GLOBAL_LOG_LEVEL;

#endif
