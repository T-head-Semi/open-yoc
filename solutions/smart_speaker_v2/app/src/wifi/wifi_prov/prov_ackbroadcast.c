/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <stdlib.h>
#include <string.h>
#include <aos/aos.h>
#include <lwip/sockets.h>

static const char *TAG = "Pair Broadcast";

#define PAIR_PORT 10001

/**
    Broadcast the pair result to end user
    todo make it a thread
*/
static void pair_broadcast_thread(void *arg)
{
    /** create udp socket and broadcast */
    int                fd;
    int                enabled = 1;
    int                err;
    struct sockaddr_in addr;
    uint8_t *          mac = (uint8_t *)arg;

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_BROADCAST;
    addr.sin_port        = htons(PAIR_PORT);

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0) {
        LOGE(TAG, "Error to create socket, reason: %s", strerror(errno));
        return;
    }

    err = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *)&enabled, sizeof(enabled));

    if (err == -1) {
        close(fd);
        return;
    }

    char *report_json_fmt = "{\"code\":0,\"msg\":\"ok\",\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
                            "\"model\":\"wifi audio\"}";
    char *report_json = aos_zalloc(strlen(report_json_fmt));
    sprintf(report_json, report_json_fmt, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    int i;
    LOGD(TAG, "Sending Json to broadcast: %s", report_json);

    for (i = 0; i < 100; i++) {
        sendto(fd, report_json, strlen(report_json), 0, (struct sockaddr *)&addr,
               sizeof(struct sockaddr));
        aos_msleep(200);
    }

    LOGD(TAG, "Broadcast end");

    aos_free(report_json);
    close(fd);
    return;
}

int wifi_pair_broadcast(uint8_t *mac)
{
    aos_task_t aos_hdl;
    int ret = aos_task_new_ext(&aos_hdl, "pair_broadcast_thread", pair_broadcast_thread, mac, 1024,
                               AOS_DEFAULT_APP_PRI);
    return ret;
}
