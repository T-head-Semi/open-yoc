/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "board.h"
#include <aos/kernel.h>
#include <stdio.h>
#include <ulog/ulog.h>
#include <aos/kv.h>

#define TAG "app"

extern int partition_init(void);
extern void cxx_system_init(void);
extern void board_yoc_init(void);

static void kv_test(void);

int main(int argc, char *argv[])
{
    cxx_system_init();
    board_yoc_init();
    LOGI(TAG, "app start........\n");

    kv_test();
    while (1) {
        aos_msleep(3000);
    };
}

void kv_test(void)
{
    char *key = NULL;
    char *str = NULL;
    int value_w = 0;
    int value_r = 0;

    LOGI(TAG, "start kv testing...");

    /* Initialize KV for testing */
    int ret = partition_init();
    if (ret <= 0) {
        LOGE(TAG, "partition init failed");
    } else {
        LOGI(TAG, "find %d partitions", ret);
        if (aos_kv_init("kv")) {
            LOGE(TAG, "kv init failed.");
            return ;
        }
        LOGE(TAG, "kv init successfully\n");
    }

    /* test integer value */
    printf("write one integer value into kv \n");
    key = "key_int";
    value_w = 0x11223344;
    aos_kv_setint(key, value_w);
    value_r = 0;
    aos_kv_getint(key, &value_r);
    printf("k:v = %s:%x\n", key, value_r);

    /* delete key_int */
    printf("Delete one key from kv \n");
    aos_kv_del(key);
    printf("Delete the key(%s)\n", key);
    value_r = 0;
    aos_kv_getint(key, &value_r);
    printf("k:v = %s:%x\n", key, value_r);
    printf("\n");

    /* test integer value */
    printf("write one string into kv \n");
    key = "key_str";
    str = "hello kv";
    aos_kv_setstring(key, str);
    char mystr[32] = {0};
    aos_kv_getstring(key, mystr, 32);
    printf("k:v = %s:%s\n", key, mystr);

    /* reset kv */
    printf("reset kv to erase all values\n");
    aos_kv_reset();
    ret = aos_kv_getstring(key, mystr, 32);
    if (ret < 0)
        printf("The key name(%s) does not exist \n", key);
}
