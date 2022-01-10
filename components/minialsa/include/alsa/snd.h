/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __SND_SND__
#define __SND_SND__

// typedef struct snd_card_drv {
//     slist_t          pcm_head;
// } snd_card_drv_t;

// typedef struct snd_dev {
//     void           *priv;
// } card_dev_t;

// typedef struct module
// {
//     char test;
// } module_t;

//提供声卡注册和查询

// int aos_card_new(int *parent, int idx, const char *xid, struct module *module, int size, snd_card_drv_t **card_ret);
// int aos_card_free(snd_card_drv_t *card);
// int aos_card_attach(const char *name, card_dev_t **card);
int aos_card_lpm(const char *name ,int state);

#ifdef __cplusplus
}
#endif

#endif