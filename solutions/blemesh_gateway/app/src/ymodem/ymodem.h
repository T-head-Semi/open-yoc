/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _YMODEM_H_
#define _YMODEM_H_

/*YModem standard CMD*/
#define YMODEM_128  (0x01)
#define YMODEM_1024 (0x02)
#define YMODEM_4096 (0x07)
#define YMODEM_8192 (0x08)
#define YMODEM_EOT  (0x04)
#define YMODEM_ACK  (0x06)
#define YMODEM_NAK  (0x15)
#define YMODEM_CAN  (0x18)
#define YMODEM_C    (0x43)
/*Rcv_CMD define*/
#define IS_NOT_FIRST_PACKET   (0x00)
#define IS_FIRST_PACKET       (0x01)
#define IS_NORMAL_FILE_128    (0X02)
#define IS_NORMAL_PACKET      (0X03)
#define IS_END_OF_TRANSMIT    (0X04)
#define IS_ERROR_PACKET       (0x05)
#define IS_TRANSMIT_OVER      (0x06)

/**
 * ymodem init
 */
int  ymodem_init(int baud_rate);

/**
 * enter the ymodem serial port upgrade
 * @return -1: client no ack at start
 */
int ymodem_upgrade(void);

/**
 * ymodem uninit
 */
int  ymodem_uninit(void);

#endif