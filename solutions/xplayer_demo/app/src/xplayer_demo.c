/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include "ulog/ulog.h"
#include "aos/cli.h"
#include <avutil/named_straightfifo.h>
#include "xplayer/xplayer.h"
#include "pingfan_30s.h"

#define TAG "xplayer_demo"

static xplayer_t* g_xplayer;
static nsfifo_t *aui_fifo;

static void player_event(void *user_data, uint8_t event, const void *data, uint32_t len)
{
    LOGD(TAG, "xplayer_evt_id %u", event);

    switch (event) {
    case XPLAYER_EVENT_START:
        LOGD(TAG, "play start, url = %s", xplayer_get_url(g_xplayer));
        break;
    case XPLAYER_EVENT_ERROR:
        LOGD(TAG, "play error, url = %s", xplayer_get_url(g_xplayer));
        xplayer_stop(g_xplayer);
        break;
    case XPLAYER_EVENT_FINISH:
        LOGD(TAG, "play finish, url = %s", xplayer_get_url(g_xplayer));
        xplayer_stop(g_xplayer);
        break;
    default:
        break;
    }
    return;
}

static void _tts_task(void *arg)
{
    char *pos;
    char *url = "fifo://tts/1";
    int data_len = _pingfan_30s_mp3_len;
    int total_len = _pingfan_30s_mp3_len;

    aui_fifo = nsfifo_open(url, O_CREAT, 16 * 1024);
    xplayer_set_url(g_xplayer, url);
    xplayer_play(g_xplayer);

    while (total_len > 0) {
        uint8_t reof = 0;
        nsfifo_get_eof(aui_fifo, &reof, NULL);
        if (reof) {
            LOGD(TAG, "named fifo read eof");
            break;
        }

        int len = nsfifo_get_wpos(aui_fifo, &pos, 500);
        if (len <= 0) {
            continue;
        }

        len = MIN(len, total_len);
        memcpy(pos, (char *)_pingfan_30s_mp3 + (data_len - total_len), len);
        nsfifo_set_wpos(aui_fifo, len);

        total_len -= len;
    }
    nsfifo_set_eof(aui_fifo, 0, 1); //set weof
    nsfifo_close(aui_fifo);

    return;
}

static void cmd_xplayer_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    char url[128];

    if (argc == 3) {
        printf("xplayer %s ", argv[1]);
        if (strcmp(argv[1], "play") == 0) {
            printf("%s\n", argv[2]);
            if (strcmp(argv[2], "inter") == 0) {
                snprintf(url, sizeof(url), "mem://addr=%lu&size=%u", (unsigned long)&_pingfan_30s_mp3, _pingfan_30s_mp3_len);
                xplayer_set_url(g_xplayer, url);
                xplayer_play(g_xplayer);
            } else if (strcmp(argv[2], "tts") == 0) {
                aos_task_t ptask;
                aos_task_new_ext(&ptask, "tts_task", _tts_task, NULL, 4*1024, AOS_DEFAULT_APP_PRI);
            } else {
                xplayer_set_url(g_xplayer, argv[2]);
                xplayer_play(g_xplayer);
            }
        }
    } else if (argc == 2) {
        if (strcmp(argv[1], "stop") == 0) {
            xplayer_stop(g_xplayer);
        } else if (strcmp(argv[1], "pause") == 0) {
            xplayer_pause(g_xplayer);
        } else if (strcmp(argv[1], "resume") == 0) {
            xplayer_resume(g_xplayer);
        } else if (strcmp(argv[1], "mute") == 0) {
            xplayer_set_mute(g_xplayer, 1);
        } else if (strcmp(argv[1], "unmute") == 0) {
            xplayer_set_mute(g_xplayer, 0);
        }
    } else {
        printf("xplayer: invaild argv");
    }
}

int cli_reg_cmd_xplayer(void)
{
    xplayer_mdl_cnf_t conf;

    static const struct cli_command cmd_info = {
        "xplayer",
        "\r\n\txplayer play url"
        "\r\n\txplayer pause"
        "\r\n\txplayer resume"
        "\r\n\txplayer stop"
        "\r\n\txplayer play inter"
        "\r\n\txplayer play tts",
        cmd_xplayer_func,
    };

    aos_cli_register_command(&cmd_info);

    xplayer_module_config_init(&conf);
    xplayer_module_init(&conf);

    g_xplayer = xplayer_new(NULL);
    xplayer_set_speed(g_xplayer, 1);
    xplayer_set_vol(g_xplayer, 160);
    xplayer_set_start_time(g_xplayer, 0);
    xplayer_set_callback(g_xplayer, player_event, g_xplayer);

    return 0;
}

