/*
 * Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <aos/aos.h>
#include <aos/cli.h>
#include <aos/kv.h>
#include <http_client.h>
#include <aos/ringbuffer.h>

#define TAG "pcminput"

#define HELP_USAGE "usage:\r\n\tpcminput http://192.168.50.25:8000/1592214047_mic_rec.pcm\r\n\trun 'kv setint pcmignalsa 1' and reboot"

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef struct {
    dev_ringbuf_t rb;
    int downloading;
    int pushing;
    char * url;
} simu_mic_http_t;

static simu_mic_http_t g_simu_http;

int g_pcminput_ignore_ssp = 0; /* 跳过信号处理 */
int g_pcminput_ignore_alsa = 0; /* 采集节点启动不采集硬件数据 */

#define HTTP_CHUNK_SIZE (960 * 4)
#define HTTP_RB_SIZE    (HTTP_CHUNK_SIZE * 10 + 1)

static void http_sink(void *arg)
{
    const char *url = (const char *)g_simu_http.url;
    char *buffer = (char*)aos_malloc_check(HTTP_CHUNK_SIZE);
    int content_length = 0, total_read_len = 0, read_len = 0;
    char *http_buf = (char*)aos_malloc_check(HTTP_RB_SIZE);

    ringbuffer_create(&g_simu_http.rb, http_buf, HTTP_RB_SIZE);

    g_simu_http.downloading = 1;
    g_simu_http.pushing = 1;
    LOGD(TAG, "start sinking http file %s", url);

    http_client_config_t config = {
        .url = url,
        //.event_handler = _http_event_handler,
        .timeout_ms = 10000,
        .buffer_size = HTTP_CHUNK_SIZE,
        // .cert_pem = ca_cert,
    };

    http_client_handle_t client = http_client_init(&config);
    if (client == NULL) {
        printf("http request init failed, url \"%s\"\n", url);
        goto http_stop;
    }

    http_errors_t err;
    if ((err = http_client_open(client, 0)) != HTTP_CLI_OK) {
        printf("Failed to open HTTP connection: 0x%x\n", err);
        goto http_stop;
    }

    content_length =  http_client_fetch_headers(client);
    while (total_read_len < content_length) {
        int avlen = g_simu_http.rb.length - ringbuffer_available_read_space(&g_simu_http.rb);
        
        if (avlen < 640 * 5) {
            aos_msleep(20);
            continue;
        }

        int rlen = MIN(avlen, HTTP_CHUNK_SIZE);

        read_len = http_client_read(client, buffer, rlen);
        if (read_len <= 0) {
            printf("Error read data\n");
        goto http_err;
        }

        ringbuffer_write(&g_simu_http.rb, (uint8_t *)buffer, read_len);
        total_read_len += read_len;
    }


http_err:    
    LOGD(TAG, "file sink finished");
    http_client_cleanup(client);

http_stop:
    aos_free(buffer);
    aos_free(http_buf);
    aos_free(g_simu_http.url);
    g_simu_http.downloading = 0;
}

static void cmd_pcminput_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc == 2) {
        if (g_simu_http.pushing) {
            printf("The Last Command has not been completed.");
            return;
        }
        g_simu_http.url = strdup(argv[1]);

        aos_task_new("http_sink", http_sink, NULL, 1024 * 8);
    } else {
        printf("%s\r\n", HELP_USAGE);
        printf("\tignore ssp=%d ignore_alsa=%d\r\n", g_pcminput_ignore_ssp, g_pcminput_ignore_alsa);
    }

    return;
}

#ifdef __cplusplus
extern "C" {
#endif

int pcm_hook_call(void *data, int len)
{
    if (g_simu_http.pushing == 0) {
        return 0;
    }

    /*下载停止且buffer空，一次灌数据完成*/
    if (!g_simu_http.downloading && ringbuffer_available_read_space(&g_simu_http.rb) == 0) {
        LOGD(TAG, "all simulate data push finished\n");
        g_simu_http.pushing = 0;
        return 0;
    }

    /*下载中, 若Buffer不够，等待数据*/
    while (g_simu_http.downloading && ringbuffer_available_read_space(&g_simu_http.rb) < len) {
        LOGW(TAG, "no enough data req:%d rb:%d", len, ringbuffer_available_read_space(&g_simu_http.rb));
        aos_msleep(100);
    }

    /*读取数据，返回，并覆盖原始数据*/
    int rlen = MIN(ringbuffer_available_read_space(&g_simu_http.rb), len);
    ringbuffer_read(&g_simu_http.rb, (uint8_t*)data, rlen);

    return rlen;
}

void cli_reg_cmd_pcminput(void)
{
    static const struct cli_command cmd_info = {"pcminput", "pcm input from httpfile", cmd_pcminput_func};

    aos_cli_register_command(&cmd_info);

    aos_kv_getint("pcmignalsa", &g_pcminput_ignore_alsa);
    aos_kv_getint("pcmignssp", &g_pcminput_ignore_ssp);
}

#ifdef __cplusplus
}
#endif