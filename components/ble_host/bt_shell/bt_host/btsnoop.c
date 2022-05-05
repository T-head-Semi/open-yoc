/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifdef CONFIG_BTSOOP
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <aos/aos.h>
#include <aos/cli.h>
#include <aos/ringbuffer.h>

#ifdef CONFIG_BTSOOP_MBEDTLS_BASE64_FORMATE
#include <mbedtls/base64.h>
#endif

#define LOG_TAG "bt_snoop"

#ifndef CONFIG_BTSOOP_FILE_SIZE
#define CONFIG_BTSOOP_FILE_SIZE (4096)
#endif
#define LOCK_TIMEOUT (5000)

typedef struct {
    uint32_t length_original;
    uint32_t length_captured;
    uint32_t flags;
    uint32_t dropped_packets;
    uint64_t timestamp;
    uint8_t type;
} __attribute__((__packed__)) btsnoop_header_t;

// Epoch in microseconds since 01/01/0000.
static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;

struct file_buf_t {
    uint8_t buf[CONFIG_BTSOOP_FILE_SIZE];
    uint32_t len;
};

enum btsoop_st {
    IDLE = 0,
    INIT = 1,
    START,
    STOP,
};

enum hci_type_en {
    HCI_TYPE_Reserved = 1000,
    HCI_TYPE_H1 = 1001,
    HCI_TYPE_H4 = 1002,
    HCI_TYPE_BSCP = 1003,
    HCI_TYPE_H5 = 1005,
    HCI_TYPE_INVAILD,
};

struct {
    aos_mutex_t  mutex;
    enum btsoop_st status;
    dev_ringbuf_t   rb;
    enum hci_type_en type;
    struct file_buf_t *file;
    struct file_buf_t *file1;
    struct file_buf_t *file2;
} btsnoop_file;

char btsnoop_file_head[16] = {0x62, 0x74, 0x73, 0x6e, 0x6f, 0x6f, 0x70, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};

#define H4_NONE      0x00
#define H4_CMD       0x01
#define H4_ACL       0x02
#define H4_SCO       0x03
#define H4_EVT       0x04

#ifndef htonl
#define htonl( X ) \
    ( (uint32_t)( \
                  ( ( ( (uint32_t)(X) ) << 24 ) & UINT32_C( 0xFF000000 ) ) | \
                  ( ( ( (uint32_t)(X) ) <<  8 ) & UINT32_C( 0x00FF0000 ) ) | \
                  ( ( ( (uint32_t)(X) ) >>  8 ) & UINT32_C( 0x0000FF00 ) ) | \
                  ( ( ( (uint32_t)(X) ) >> 24 ) & UINT32_C( 0x000000FF ) ) ) )
#endif

static uint64_t htonll(uint64_t ll)
{
    const uint32_t l = 1;

    if (*((uint8_t *)(&l)) == 1) {
        return (uint64_t)((((uint64_t)htonl(ll & 0xffffffff)) << 32) | htonl(ll >> 32));
    }

    return ll;
}

static int btsnoop_filewrite_check(uint32_t len, uint8_t need_swap)
{
    aos_mutex_lock(&btsnoop_file.mutex, LOCK_TIMEOUT);
    uint8_t ret = btsnoop_file.file->len + len > CONFIG_BTSOOP_FILE_SIZE;
    if (ret && need_swap)
    {
        btsnoop_file.file = btsnoop_file.file == btsnoop_file.file1 ? btsnoop_file.file2 : btsnoop_file.file1;
        btsnoop_file.file->len = 0;
    }
    aos_mutex_unlock(&btsnoop_file.mutex);
    return ret;
}

static int btsnoop_filewrite(const uint8_t *data, uint32_t len)
{
    aos_mutex_lock(&btsnoop_file.mutex, LOCK_TIMEOUT);

    if (btsnoop_file.file->len + len > CONFIG_BTSOOP_FILE_SIZE) {
        btsnoop_file.file = btsnoop_file.file == btsnoop_file.file1 ? btsnoop_file.file2 : btsnoop_file.file1;
        btsnoop_file.file->len = 0;
    }

    if (btsnoop_file.file->len == 0) {
        memcpy(btsnoop_file.file->buf, btsnoop_file_head, sizeof(btsnoop_file_head));
        btsnoop_file.file->len += sizeof(btsnoop_file_head);
    }

    struct file_buf_t *file = btsnoop_file.file;

    memcpy(file->buf + file->len , data, len);

    file->len += len;

    aos_mutex_unlock(&btsnoop_file.mutex);

    return 0;
}

int btsnoop_write(uint8_t type, const uint8_t *data, uint32_t len, bool is_income)
{
    const uint8_t *packet;

    if (!data || 0 == len) {
        return -1;
    }

    if (btsnoop_file.status <= IDLE) {
        return -1;
    }

    packet = data;

    aos_mutex_lock(&btsnoop_file.mutex, LOCK_TIMEOUT);

    if (btsnoop_file.status != START) {
        aos_mutex_unlock(&btsnoop_file.mutex);
        return -1;
    }

    uint64_t timestamp_us;
    struct timeval tv = {0, 0};

    gettimeofday(&tv, NULL);
    timestamp_us = (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;

    uint32_t length_he = 0;
    uint32_t flags = 0;
    btsnoop_header_t header;

    if (btsnoop_file.type = HCI_TYPE_H4) {
        switch (type) {
            case H4_CMD:
                length_he = packet[2] + 4;
                flags = 2;
                break;

            case H4_ACL:
                length_he = (packet[3] << 8) + packet[2] + 5;
                flags = is_income;
                break;

            case H4_SCO:
                length_he = packet[2] + 4;
                flags = is_income;
                break;

            case H4_EVT:
                length_he = packet[1] + 3;
                flags = 3;
                break;
        }
    } else {
        return -1;
    }

    header.length_original = htonl(length_he);
    header.length_captured = header.length_original;
    header.flags = htonl(flags);
    header.dropped_packets = 0;
    header.timestamp = htonll(timestamp_us + BTSNOOP_EPOCH_DELTA);
    header.type = type;

    btsnoop_filewrite_check(sizeof(btsnoop_header_t) +  length_he - 1, 1);

    int ret = btsnoop_filewrite((const uint8_t *)&header, sizeof(btsnoop_header_t));

    if (ret) {
        return ret;
    }

    ret = btsnoop_filewrite(packet, length_he - 1);

    if (ret) {
        return ret;
    }

    aos_mutex_unlock(&btsnoop_file.mutex);

    return 0;
}

int btsnoop_start()
{
    if (btsnoop_file.status <= IDLE) {
        return -1;
    }

    aos_mutex_lock(&btsnoop_file.mutex, LOCK_TIMEOUT);
    btsnoop_file.status = START;
    aos_mutex_unlock(&btsnoop_file.mutex);

    return 0;
}

int btsnoop_stop()
{
    if (btsnoop_file.status <= IDLE) {
        return -1;
    }

    aos_mutex_lock(&btsnoop_file.mutex, LOCK_TIMEOUT);
    btsnoop_file.status = STOP;
    aos_mutex_unlock(&btsnoop_file.mutex);
    return 0;
}

static void btsnoop_print_hexstring(const uint8_t *data, uint32_t len)
{
#ifdef CONFIG_BTSOOP_MBEDTLS_BASE64_FORMATE
    char *b64;
    size_t dlen, olen;

    dlen = len * 3 / 2 + 4;
    b64  = aos_zalloc(dlen);

    if (!b64) {
        return;
    }

    mbedtls_base64_encode((unsigned char *)b64, dlen, &olen, (unsigned char *)data, len);

    printf(b64);
    aos_free(b64);
#else
    uint32_t i;

    for (i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }

#endif
}

int btsnoop_print()
{
    if (btsnoop_file.status <= IDLE) {
        return -1;
    }

    aos_mutex_lock(&btsnoop_file.mutex, LOCK_TIMEOUT);
    printf("btsnoop file1:\r\n");

    if (btsnoop_file.file1->len) {
        btsnoop_print_hexstring(btsnoop_file.file1->buf, btsnoop_file.file1->len);
    }

    printf("\r\n");
    printf("btsnoop file2:\r\n");

    if (btsnoop_file.file1->len) {
        btsnoop_print_hexstring(btsnoop_file.file2->buf, btsnoop_file.file2->len);
    }

    printf("\r\n");

    aos_mutex_unlock(&btsnoop_file.mutex);
    return 0;
}

int btsnoop_init(enum hci_type_en type)
{
    int ret;

    if (btsnoop_file.status > IDLE) {
        return 0;
    }

    if (type != HCI_TYPE_H4) {
        LOGE(LOG_TAG, "invaild type");
        return -1;
    }

    ret = aos_mutex_new(&btsnoop_file.mutex);

    if (ret) {
        LOGE(LOG_TAG, "mutex init fail");
        return -1;
    }

    struct file_buf_t *file1  = (struct file_buf_t *)aos_zalloc(sizeof(struct file_buf_t));

    if (!file1) {
        aos_mutex_free(&btsnoop_file.mutex);
        LOGE(LOG_TAG, "buf alloc fail");
        return -1;
    }

    struct file_buf_t *file2  = (struct file_buf_t *)aos_zalloc(sizeof(struct file_buf_t));

    if (!file2) {
        aos_mutex_free(&btsnoop_file.mutex);
        aos_free(file1);
        LOGE(LOG_TAG, "buf alloc fail");
        return -1;
    }

    btsnoop_file_head[12] = (uint8_t)(((uint32_t)type >> 24) & 0xff);
    btsnoop_file_head[13] = (uint8_t)(((uint32_t)type >> 16) & 0xff);
    btsnoop_file_head[14] = (uint8_t)(((uint32_t)type >> 8) & 0xff);
    btsnoop_file_head[15] = (uint8_t)(((uint32_t)type >> 0) & 0xff);

    btsnoop_file.type = type;
    btsnoop_file.file1 = file1;
    btsnoop_file.file2 = file2;
    btsnoop_file.file = btsnoop_file.file1;
    btsnoop_file.status = INIT;

    return 0;
}

static void cmd_btsnoop_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int i = 0;
    int err;

    if (argc < 2) {
        printf("ivaild params\n");
        printf("btsnoop <start, stop, print>");
        return;
    }

    if (!strncmp("start", argv[1], strlen("start"))) {
        err = btsnoop_init(HCI_TYPE_H4);

        if (err) {
            printf("btsnoop start fail, %d\r\n", err);
            return;
        }

        btsnoop_start();
        printf("btsnoop file start recored\r\n");
    } else if (!strncmp("stop", argv[1], strlen("stop"))) {
        btsnoop_stop();
        printf("btsnoop file stop recored\r\n");
    } else if (!strncmp("print", argv[1], strlen("print"))) {
        btsnoop_print();
    } else {
        printf("invaild params\n");
        printf("btsnoop <start, stop, print>");
    }

    return;
}

void cli_reg_cmd_btsnoop(void)
{
    int ret;

    static const struct cli_command cmd_info = {
        "btsnoop",
        "btsnoop <start, stop, print>\r\nexamples:\r\nbtsnoop start\r\r\nbtsnoop stop\r\nbtsnoop print\r\n",
        cmd_btsnoop_func,
    };

    if (0 != aos_cli_register_command(&cmd_info)) {
        LOGE(LOG_TAG, "comand regeist fail");
        return;
    }

    return;
}
#endif //CONFIG_BTSOOP