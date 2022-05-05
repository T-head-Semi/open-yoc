/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#ifdef CONFIG_SUPPORT_YMODEM

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <aos/kernel.h>

#include "ymodem.h"
#include "ymodem_porting.h"

static unsigned short int ymodem_crc(unsigned char *data, unsigned short int len);

static uint32_t program_start_addr = 0; /*写的起始地址*/
static int      program_count      = 0; /*已经保存*/
static int      file_size          = 0; /*文件总大小*/
static int      recv_size_now      = 0; /*接收到当前帧的大小*/
static char     file_name[64];          /*当前文件名*/

/*统计信息*/
static int crc_error          = 0;
static int packet_index_error = 0;

static int ymodem_start = 0;

static unsigned char receive_buff[8192 + 5];
static int           rcv_packet_index      = 0;
static int           rcv_packet_index_last = 0;
static unsigned char IsFileEnd;

static int EOT_Flag = 0;

static unsigned char result = 0;
static int first_package = 1;

#define YmodemSendACK ymoedm_send_char

//保存并应答
static void ymodem_save_and_ack(unsigned char PacketReturn, unsigned char *IsFileEnd)
{
    switch (PacketReturn) {
        case IS_FIRST_PACKET: {
            ymodem_flash_erase(program_start_addr, file_size);
            YmodemSendACK(YMODEM_ACK);
            YmodemSendACK(YMODEM_C);
            break;
        }

        case IS_TRANSMIT_OVER: {
            YmodemSendACK(YMODEM_CAN);
            break;
        }

        case IS_NORMAL_FILE_128:
        case IS_NORMAL_PACKET: {
            //写flash
#if (UART_PORTING_SYNC == 0)
            YmodemSendACK(YMODEM_ACK);
#endif

            if ((file_size - program_count) >= recv_size_now) {
                ymodem_flash_write(program_start_addr + program_count, receive_buff + 3,
                                   recv_size_now);
                program_count += recv_size_now;
            } else {
                ymodem_flash_write(program_start_addr + program_count, receive_buff + 3,
                                   (file_size - program_count));
                program_count += (file_size - program_count);
            }

#if UART_PORTING_SYNC
            YmodemSendACK(YMODEM_ACK);
#endif
            break;
        }

        case IS_END_OF_TRANSMIT: {
            if (EOT_Flag == 1) {
                YmodemSendACK(YMODEM_NAK);
            }

            if (EOT_Flag == 2) { // 第二次收到EOT发ACK 和大写 C
                YmodemSendACK(YMODEM_ACK);
                YmodemSendACK(YMODEM_C);
            }

            if (EOT_Flag == 3) { //传输结束
                YmodemSendACK(YMODEM_ACK);
                EOT_Flag   = 0;
                *IsFileEnd = 1;
            }

            break;
        }

        case IS_ERROR_PACKET: {
            YmodemSendACK(YMODEM_NAK);
            break;
        }
    }
}
static int get_file_size(unsigned char *data)
{
    int  first = 0, second = 0;
    char first_find  = 0;
    char second_find = ' ';

    for (int i = 0; i < 50; i++) {
        if (data[i] == first_find) {
            first = i;
        }

        if (data[i] == second_find) {
            second = i;
            break;
        }
    }

    int tmp    = 0;
    int tmp_10 = 1;

    for (int i = (second - 1); i > first; i--) {
        tmp = tmp + ((data[i] - '0') * tmp_10);
        tmp_10 *= 10;
    }

    return tmp;
}

static int file_is_valid(unsigned char *data)
{
    file_size = get_file_size(data);
    strcpy(file_name, (char *)(data));
    program_count = 0;

    if (ymodem_get_write_addr(data, &program_start_addr) == 0) {
        return 1;
    }

    return 0;
}

static int ymodem_data_check(unsigned char *rcvbuf, unsigned char *result)
{
    unsigned short int crc;
    unsigned char      debug;
    unsigned char      debug1;

    unsigned short int crc_result;

    switch (*(rcvbuf)) {
        case YMODEM_EOT: {
            EOT_Flag++;
            *result = IS_END_OF_TRANSMIT;
            break;
        }

        case YMODEM_128: {
            debug  = ((~(*(rcvbuf + 1))));
            debug1 = ((*(rcvbuf + 2)));

            if (debug != debug1) {
                *result = IS_ERROR_PACKET;
                return -1;
            }

            crc        = *(rcvbuf + 128 + 3) << 8 | *(rcvbuf + 128 + 3 + 1);
            crc_result = ymodem_crc(rcvbuf + 3, 128);

            if (crc_result != crc) {
                *result = IS_ERROR_PACKET;
                crc_error++;
                return -1;
            }

            rcv_packet_index = *(rcvbuf + 1);

            if (rcv_packet_index == 0) {
                if (*result == IS_END_OF_TRANSMIT) {
                    *result = IS_END_OF_TRANSMIT;
                    EOT_Flag++;
                    return 0;
                }

                if (first_package) {
                    if (file_is_valid(rcvbuf + 3)) {
                        first_package = 0;
                        *result       = IS_FIRST_PACKET;
                    } else {
                        *result = IS_TRANSMIT_OVER;
                        return -2;
                    }
                } else {
                    if ((rcv_packet_index_last) != 255) {
                        packet_index_error++;
                        *result = IS_ERROR_PACKET;
                    } else {
                        rcv_packet_index_last = rcv_packet_index;
                        *result               = IS_NORMAL_FILE_128;
                    }
                }
            } else {
                if (rcv_packet_index != (rcv_packet_index_last + 1)) {
                    packet_index_error++;
                    *result = IS_ERROR_PACKET;
                } else {
                    rcv_packet_index_last = rcv_packet_index;
                    *result               = IS_NORMAL_FILE_128;
                }
            }

            break;
        }

        case YMODEM_1024:
        case YMODEM_4096:
        case YMODEM_8192: {
            debug  = ((~(*(rcvbuf + 1))));
            debug1 = ((*(rcvbuf + 2)));

            if (debug != debug1) {
                *result = IS_ERROR_PACKET;
                return -1;
            }

            crc        = *(rcvbuf + recv_size_now + 3) << 8 | *(rcvbuf + recv_size_now + 3 + 1);
            crc_result = ymodem_crc(rcvbuf + 3, recv_size_now);

            if (crc_result != crc) {
                *result = IS_ERROR_PACKET;
                crc_error++;
                return -1;
            }

            rcv_packet_index = *(rcvbuf + 1);

            if (rcv_packet_index == 0) {
                if (first_package) {
                    if (file_is_valid(rcvbuf + 3)) {
                        first_package = 0;
                        *result       = IS_FIRST_PACKET;
                    } else {
                        *result = IS_TRANSMIT_OVER;
                        return -2;
                    }
                } else {
                    if ((rcv_packet_index_last) != 255) {
                        packet_index_error++;
                        *result = IS_ERROR_PACKET;
                    } else {
                        rcv_packet_index_last = rcv_packet_index;
                        *result               = IS_NORMAL_PACKET;
                        if (EOT_Flag > 0)
                        {
                            EOT_Flag = 0;
                            printf("may recive EIO_FLAG in Transmit\r\n");
                        }
                    }
                }
            } else {
                if (rcv_packet_index != (rcv_packet_index_last + 1)) {
                    packet_index_error++;
                    *result = IS_ERROR_PACKET;
                } else {
                    rcv_packet_index_last = rcv_packet_index;
                    *result               = IS_NORMAL_PACKET;
                }
            }

            break;
        }

        default:
            break;
    }

    return 0;
}
//CRC16_XMODEM  x16+x12+x5+1 初始值 0x0000
static unsigned short int ymodem_crc(unsigned char *data, unsigned short int len)
{
    int crc = 0;
    int i, j;

    for (i = 0; i < len; i++) {
        crc = crc ^ (data[i] << 8);

        for (j = 0; j < 8; j++) {
            if ((crc & ((int)0x8000)) != 0) {
                crc = ((crc << 1) ^ 0x1021);
            } else {
                crc = crc << 1;
            }
        }
    }

    return (crc & 0xFFFF);
}

int ymodem_init(int baud_rate)
{
    int ret;
    ret = ymodem_init_porting(baud_rate);

    if (ret < 0) {
        return -1;
    }
    result = 0;
    first_package = 1;
    ymodem_start          = 0;
    rcv_packet_index      = 0;
    rcv_packet_index_last = 0;
    IsFileEnd             = 0;
    EOT_Flag              = 0;
    memset(receive_buff, 0, sizeof(receive_buff));
    return 0;
}

int ymodem_uninit(void)
{
    return ymodem_uninit_porting();
}

char* ymodem_get_file_name()
{
    return file_name;
}
static int get_rcv_size(unsigned char packet_head)
{
    int rcv_size;

    switch (packet_head) {
        case YMODEM_128:
            rcv_size = 128;
            break;

        case YMODEM_1024:
            rcv_size = 1024;
            break;

        case YMODEM_4096:
            rcv_size = 4096;
            break;

        case YMODEM_8192:
            rcv_size = 8192;
            break;

        case YMODEM_EOT:
            rcv_size = -4;
            break;

        default:
            rcv_size = 0;
            break;
    }

    return rcv_size;
}
int ymodem_upgrade(void)
{
    static int rcv_count = 0;
    int        ret       = -1;
    uint32_t t, t1 = 0;
    //printf("\n");
    //printf("Serial Upgrade ...\n");
    //*(uint32_t*)(0x31000060) = 0x0;
    t = aos_now_ms();

    while (1) {
        if (ymodem_start == 0) {
            aos_msleep(100);
            YmodemSendACK(YMODEM_C);
        }

        ret = ymodem_rcv(receive_buff, 1);

        if ((ret > 0) && ((receive_buff[0] == YMODEM_128) || (receive_buff[0] == YMODEM_1024) ||
                          (receive_buff[0] == YMODEM_4096) || (receive_buff[0] == YMODEM_8192) ||
                          (receive_buff[0] == YMODEM_EOT))) {
            ymodem_start = 1;
            rcv_count += ret;
            break;
        }

        if (aos_now_ms() -  t > 60000) {
            printf("upgrade start timeout\n");
            return -1;
        }
    }

    t = aos_now_ms();

    while (1) {
        ret = ymodem_rcv(receive_buff + rcv_count, sizeof(receive_buff) - rcv_count);
        rcv_count += ret;

        recv_size_now = get_rcv_size(receive_buff[0]);

        if (recv_size_now == 0) {
            rcv_count = 0;
        }

        if (rcv_count > 0) {
            t1 = aos_now_ms();

            while (1) {
                ret = ymodem_rcv(receive_buff + rcv_count, sizeof(receive_buff) - rcv_count);
                rcv_count += ret;

                if (rcv_count >= (recv_size_now + 5)) {
                    if (ymodem_data_check(receive_buff, &result) == -2) {
                        ymodem_save_and_ack(result, &IsFileEnd);
                        return 0;
                    }

                    ymodem_save_and_ack(result, &IsFileEnd);
                    rcv_count = 0;
                    break;
                }

                if (aos_now_ms() - t1 > 1000) {
                    /* clear all recevied data */
                    while(ret > 0)
                    {
                        ret = ymodem_rcv(receive_buff, sizeof(receive_buff));
                    }

                    rcv_count = 0;
                    ymodem_save_and_ack(IS_ERROR_PACKET, &IsFileEnd);
                    t1 = aos_now_ms();
                    printf("transmit timeout, retry\n");
                }

                if (aos_now_ms() - t > 30 * 1000) {
                    printf("upgrade fail,  timeout\n");
                    return -1;
                }
            }
        }

        if (aos_now_ms() - t > 5 * 60 * 1000) {
            printf("upgrade fail,  timeout\n");
            return -1;
        }

        if (IsFileEnd) {
#if 0

            for (int i = 0; i < 10; i++) {
                printf("********************************************\n");
                printf("...0x%x\n", program_start_addr);
                printf("%s\n", file_name);
                printf("error: %d %d\n", packet_index_error, crc_error);
                printf("%d %d\n", file_size, program_count);
            }

            for (int i = 0; i < 64; i++) {
                printf("%#x ", *(uint8_t *)(program_start_addr + i));
            }

#endif
            return 0;
        }
    }
}
#endif
