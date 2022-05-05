/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef YOC_NETIO_H
#define YOC_NETIO_H

#include <stdint.h>
#include <stddef.h>
#include <aos/list.h>

#ifndef CONFIG_FOTA_BUFFER_SIZE
#define CONFIG_FOTA_BUFFER_SIZE 512
#define BUFFER_SIZE 2048
#else
#define BUFFER_SIZE (CONFIG_FOTA_BUFFER_SIZE * 2)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct netio_cls netio_cls_t;

typedef struct {
    const netio_cls_t *cls;     /*!< netio ops */
    size_t offset;              /*!< offset for seek */
    size_t size;                /*!< file size or partition size */
    size_t block_size;          /*!< the size for transmission(sector size) */

    void *private;              /*!< user data */
} netio_t;

struct netio_cls {
    const char *name;
    int (*open)(netio_t *io, const char *path);
    int (*close)(netio_t *io);

    int (*read)(netio_t *io, uint8_t *buffer, int length, int timeoutms);
    int (*write)(netio_t *io, uint8_t *buffer, int length, int timeoutms);
    int (*remove)(netio_t *io);
    int (*seek)(netio_t *io, size_t offset, int whence);

    void *private;
};

/**
 * @brief  netio 注册
 * @param  [in] cls: netio 操作接口
 * @return 0 on success, -1 on failed
 */
int netio_register(const netio_cls_t *cls);

/**
 * @brief  将http操作功能注册到netio
 * @return 0 on success, -1 on failed
 */
int netio_register_http(void);

/**
 * @brief  将http client操作功能注册到netio
 * @param  [in] cert: https证书，可以为NULL
 * @return 0 on success, -1 on failed
 */
int netio_register_httpc(const char *cert);

/**
 * @brief  将flash操作功能注册到netio
 * @return 0 on success, -1 on failed
 */
int netio_register_flash(void);

/**
 * @brief  将coap操作功能注册到netio
 * @return 0 on success, -1 on failed
 */
int netio_register_coap(void);

/**
 * @brief  netio 打开
 * @param  [in] path: 下载或者储存 url
 * @return netio_t句柄或者NULL
 */
netio_t *netio_open(const char *path);

/**
 * @brief  netio 关闭
 * @param  [in] io: netio句柄
 * @return 0 on success, -1 on failed
 */
int netio_close(netio_t *io);

/**
 * @brief  netio 读取
 * @param  [in] io: netio句柄
 * @param  [in] buffer: 用于存放读取数据的buffer
 * @param  [in] lenght: buffer大小
 * @param  [in] timeoutms: 超时时长
 * @return 0表示文件读完，-1超时失败，否则为读取的长度
 */
int netio_read(netio_t *io, uint8_t *buffer, size_t lenght, int timeoutms);

/**
 * @brief  netio 写
 * @param  [in] io: netio句柄
 * @param  [in] buffer: 用于存放读取数据的buffer
 * @param  [in] lenght: buffer大小
 * @param  [in] timeoutms: 超时时长
 * @return -1超时失败，否则为写入的长度
 */
int netio_write(netio_t *io, uint8_t *buffer, size_t lenght, int timeoutms);

/**
 * @brief  netio 寻址
 * @param  [in] io: netio句柄
 * @param  [in] offset：偏移
 * @param  [in] whence：偏移方向
 * @return 0 on success, -1 on failed
 */
int netio_seek(netio_t *io, size_t offset, int whence);

#ifdef __cplusplus
}
#endif

#endif
