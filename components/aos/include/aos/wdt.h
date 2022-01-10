/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef __AOS_WDT_H__
#define __AOS_WDT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t aos_wdt_index();
void aos_wdt_debug(int en);
int aos_wdt_hw_enable(int id, int ms);
void aos_wdt_hw_disable(int id);
void aos_wdt_attach(uint32_t index, void (*will)(void *), void *args);
void aos_wdt_detach(uint32_t index);
int  aos_wdt_exists(uint32_t index);
void aos_wdt_feed(uint32_t index, int max_time);
void aos_wdt_show(uint32_t index);
void aos_wdt_showall();

#ifdef __cplusplus
}
#endif

#endif
