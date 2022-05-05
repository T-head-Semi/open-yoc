/*
 * Copyright (C) 2018-2021 Alibaba Group Holding Limited
 */

#ifndef __DEVICE_MESH_H__
#define __DEVICE_MESH_H__

#include <tsl_engine/device.h>

__BEGIN_DECLS__

typedef struct device_mesh {
    DEVICE_BASE_CLASS;

    //FIXME:
    uint16_t                   netkey_idx;
    uint16_t                   appkey_idx;
    uint16_t                   unicast_addr;
} device_mesh_t;

enum {
    MESH_CMD_UNKNOWN,
    MESH_CMD_SET_ONOFF,
    MESH_CMD_FIND_MODEL,
};

typedef struct mesh_set_onoff {
    void                       *cli_model;
    uint8_t                    onoff;
} mesh_set_onoff_t;

typedef struct mesh_find_model {
    uint16_t                   elem_idx;
    uint16_t                   mod_idx;
    uint16_t                   cid;

    //out
    void                       *cli_model;
} mesh_find_model_t;

__END_DECLS__

#endif /* __DEVICE_MESH_H__ */

