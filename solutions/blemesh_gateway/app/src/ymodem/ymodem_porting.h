/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _YMODEM_UART_H_
#define _YMODEM_UART_H_

/* 模式配置 */
#define UART_PORTING_SYNC 0 //0 使用串口异步模式
#define WRITE_ERASE  1
#define READ_COMPARE 1  /* WRITE_ERASE 使能才有效 */

#define YMODEM_TIME_OUT 50000*8*10

/* 数据通讯接口 */
int ymoedm_send_char(unsigned char ch);
int ymodem_rcv(unsigned char *data, int size);
int ymodem_check();

/* Flash读写接口 */
int ymodem_get_write_addr(uint8_t *data, uint32_t *flash_addr);
int ymodem_flash_read(uint32_t address, uint8_t *buf, uint32_t b_len);
int ymodem_flash_write(uint32_t address, uint8_t *buf, uint32_t b_len);
int ymodem_flash_erase(uint32_t address, uint32_t b_len);

/* 初始化 */
int ymodem_init_porting(int baud_rate);
int ymodem_uninit_porting(void);
#endif