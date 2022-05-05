/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _MESH_OCC_AUTH_H
#define _MESH_OCC_AUTH_H

#include "aos/ble.h"

#ifdef   CONFIG_BT_MESH_PROVISIONER

#ifndef CONFIG_OCC_AUTH_TASK_PRIO
#define CONFIG_OCC_AUTH_TASK_PRIO (16)
#endif

#ifndef CONFIG_OCC_AUTH_STACK_SIZE
#define CONFIG_OCC_AUTH_STACK_SIZE (4096)
#endif

#ifndef CONFIG_OCC_AUTH_NODE_MAX_MSG_SIZE
#define CONFIG_OCC_AUTH_NODE_MAX_MSG_SIZE 20
#endif

typedef struct {
    uint8_t         auth_status;
    uint32_t        short_oob;
    char         CID[33];
} occ_auth_data;

typedef void (*occ_auth_cb)(uint8_t addr[6], uint8_t addr_type, uint8_t uuid[16], occ_auth_data *data);

typedef enum {
    MESH_AUTH_IDLE     = 0x00,
    MESH_AUTH_FAILD    = 0x01,
    MESH_AUTH_SUCCESS  = 0x02,
} mesh_occ_auth_status_en;
#endif



#ifdef   CONFIG_BT_MESH_PROVISIONER
int mesh_occ_auth_prov_init(occ_auth_cb cb);
int mesh_occ_auth_prov_dev_add(uint8_t addr[6], uint8_t addr_type, uint8_t uuid[16]);
#else
int mesh_occ_auth_node_init();
int mesh_occ_auth_node_get_uuid(uint8_t uuid[16]);
int mesh_occ_auth_node_get_oob(uint8_t oob[16]);
#endif


#endif
