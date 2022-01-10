/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <aos/kernel.h>
#include <alsa/pcm.h>
#include <alsa/snd.h>

// static aos_task_t alsa_task_hdl;

enum {
    _PCM_WRITE_CMD,
    _PCM_READ_CMD,
};

typedef struct {
    int cmd;
    void (*cb)(void *param, int len);
} alsa_cmd_t;

void pcm_write(void *param, int len);
void pcm_read(void *param, int len);

const alsa_cmd_t alsa_cmt_table[] = {
    {_PCM_WRITE_CMD, pcm_write},
    {_PCM_READ_CMD, pcm_read},
};

void pcm_write(void *param, int len)
{

}

void pcm_read(void *param, int len)
{

}
void alsa_task(void *param)
{

}

int alsa_init(void)
{
    // aos_task_new_ext(&alsa_task_hdl, "alsa", alsa_task, NULL, 2048, AOS_DEFAULT_APP_PRI);

    return 0;
}