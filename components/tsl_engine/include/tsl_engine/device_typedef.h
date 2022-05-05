/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __DEVICE_TYPEDEF_H__
#define __DEVICE_TYPEDEF_H__

#include <aos/hash.h>
#include <tsl_engine/tsl_common.h>

__BEGIN_DECLS__

typedef struct struct_device  device_t;

enum {
    DEV_TYPE_UNKNOWN,
    DEV_TYPE_MESH,
    DEV_TYPE_ZIGBEE,
};

typedef struct {
    char                      *mac;
    uint8_t                   local;        ///< TODO:
} dev_conf_t;

struct device_ops {
    int   (*open)             (device_t *dev);
    int   (*ioctl)            (device_t *dev, int cmd, void *arg, size_t *arg_size);
    int   (*send_to_device)   (device_t *dev, const uint8_t *data, size_t size);
    int   (*send_to_cloud)    (device_t *dev, const uint8_t *data, size_t size);
    void  (*close)            (device_t *dev);
};

#define DEVICE_BASE_CLASS                                      \
    int                       type;                            \
    char                      *name;                           \
    size_t                    size;                            \
    char                      mac[6];                          \
    hash_t                    map;                             \
    slist_t                   node;                            \
    uint8_t                   local;                           \
    uint8_t                   status;                          \
    aos_mutex_t               lock;                            \
    struct device_ops         *ops;

typedef struct struct_device {
    DEVICE_BASE_CLASS
} device_t;

typedef struct struct_dev_data {
    int opcode;
    char *data;
    int data_size;
} dev_data_t;

__END_DECLS__

#endif /* __DEVICE_TYPEDEF_H__ */

