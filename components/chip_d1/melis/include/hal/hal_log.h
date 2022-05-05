#ifndef SUNXI_HAL_LOG_H
#define SUNXI_HAL_LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <syslog.h>
#ifndef CONFIG_KERNEL_NONE
#ifdef AOS_COMP_DEBUG
#include <debug/dbg.h>
#else
#define printk printf
#endif /*AOS_COMP_DEBUG*/
#else
#define printk printf
#endif

#ifdef CONFIG_LOG_DEFAULT_LEVEL
#if CONFIG_LOG_DEFAULT_LEVEL == 4
#define hal_log_err(fmt, ...)           do {printk("[E][%s %d]"fmt"\r\n", __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define hal_log_warn(fmt, ...)          do {printk("[W][%s %d]"fmt"\r\n", __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define hal_log_info(fmt, ...)          do {printk("[I][%s %d]"fmt"\r\n", __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define hal_log_debug(fmt, ...)         do {printk("[D][%s %d]"fmt"\r\n", __func__, __LINE__, ##__VA_ARGS__);} while (0)
#elif CONFIG_LOG_DEFAULT_LEVEL == 3
#define hal_log_err(fmt, ...)           do {printk("[E][%s %d]"fmt"\r\n", __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define hal_log_warn(fmt, ...)          do {printk("[W][%s %d]"fmt"\r\n", __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define hal_log_info(fmt, ...)          do {printk("[I][%s %d]"fmt"\r\n", __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define hal_log_debug(fmt, ...)         do {} while (0)
#elif CONFIG_LOG_DEFAULT_LEVEL == 2
#define hal_log_err(fmt, ...)           do {printk("[E][%s %d]"fmt"\r\n", __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define hal_log_warn(fmt, ...)          do {printk("[W][%s %d]"fmt"\r\n", __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define hal_log_info(fmt, ...)          do {} while (0)
#define hal_log_debug(fmt, ...)         do {} while (0)
#elif CONFIG_LOG_DEFAULT_LEVEL == 1
#define hal_log_err(fmt, ...)           do {printk("[E][%s %d]"fmt"\r\n", __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define hal_log_warn(fmt, ...)          do {} while (0)
#define hal_log_info(fmt, ...)          do {} while (0)
#define hal_log_debug(fmt, ...)         do {} while (0)
#else
#define hal_log_err(fmt, ...)           do {} while (0)
#define hal_log_warn(fmt, ...)          do {} while (0)
#define hal_log_info(fmt, ...)          do {} while (0)
#define hal_log_debug(fmt, ...)         do {} while (0)
#endif

#else

#define hal_log_err(fmt, ...)           do {} while (0)
#define hal_log_warn(fmt, ...)          do {} while (0)
#define hal_log_info(fmt, ...)          do {} while (0)
#define hal_log_debug(fmt, ...)         do {} while (0)
#endif


#ifdef __cplusplus
}
#endif

#endif
