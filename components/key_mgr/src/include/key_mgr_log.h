/**
* Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
*/

#ifndef __KEY_MGR_LOG_H__
#define __KEY_MGR_LOG_H__

#include <stdio.h>

#undef LOG_TAG
#define LOG_TAG "[KM]"

#ifdef CONFIG_DEBUG
#if CONFIG_DEBUG >= 3
#define KM_LOGD(format, ...) printf(LOG_TAG"[D] ""[%d]"format"\r\n", __LINE__, ##__VA_ARGS__)
#define KM_LOGI(format, ...) printf(LOG_TAG"[I] ""[%d]"format"\r\n", __LINE__, ##__VA_ARGS__)
#define KM_LOGW(format, ...) printf(LOG_TAG"[W] ""[%d]"format"\r\n", __LINE__, ##__VA_ARGS__)
#define KM_LOGE(format, ...) printf(LOG_TAG"[E] ""[%d]"format"\r\n", __LINE__, ##__VA_ARGS__)
#elif CONFIG_DEBUG == 2
#define KM_LOGD(format, ...)
#define KM_LOGI(format, ...) printf(LOG_TAG"[I] "format"\r\n",##__VA_ARGS__)
#define KM_LOGW(format, ...)
#define KM_LOGE(format, ...) printf(LOG_TAG"[E] "format"\r\n",##__VA_ARGS__)
#elif CONFIG_DEBUG == 1
#define KM_LOGD(format, ...)
#define KM_LOGI(format, ...)
#define KM_LOGW(format, ...)
#define KM_LOGE(format, ...) printf(LOG_TAG"[E] "format"\r\n",##__VA_ARGS__)
#else
#define KM_LOGD(format, ...)
#define KM_LOGI(format, ...)
#define KM_LOGW(format, ...)
#define KM_LOGE(format, ...)
#endif
#else
#define KM_LOGD(format, ...)
#define KM_LOGI(format, ...)
#define KM_LOGW(format, ...)
#define KM_LOGE(format, ...)
#endif

#endif