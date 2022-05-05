/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _GATEWAY_MESH_H_
#define _GATEWAY_MESH_H_

#include <api/mesh.h>
#include <mesh_model/mesh_model.h>

#define GET_NODE_VER_TIMEOUT 120*1000
#define GET_NODE_VER_INTERVAL 1*1000
#define GET_NODE_VER_CNT_MAX 3

typedef struct {
    char *dev_name;
    const struct bt_mesh_comp *comp;
    model_event_cb user_model_cb;
} gateway_mesh_config_t;

int gateway_mesh_init(gateway_mesh_config_t *config);
void gateway_mesh_mac_convert(uint8_t *mac, uint8_t *mac_converted);
int gateway_mesh_set_onoff(uint8_t *mac, uint8_t onoff);
int gateway_mesh_set_brightness(uint8_t *mac, uint8_t bri);
int gateway_mesh_set_cct(uint8_t *mac, uint16_t cct);
int gateway_mesh_sub(uint8_t *mac, uint16_t mod_id, uint16_t sub_addr);
int gateway_mesh_control_sub_onoff(uint16_t sub_addr, uint8_t onoff);

int  gateway_mesh_gw_set_onoff(uint8_t onoff);
void gateway_mesh_prov_enable(uint8_t en_mesh, uint8_t prov_flag, uint16_t start_addr, uint16_t end_addr);
void gateway_mesh_add_mac_filter(uint8_t *mac_str);
void gateway_mesh_del_mac_filter(uint8_t *mac_str);
int  gateway_mesh_suspend(void);
int  gateway_mesh_resume(void);

int gateway_mesh_composition_init(void);
int gateway_btmesh_prov_config(uint32_t start_addr, uint32_t end_addr);
int gateway_btmesh_prov_autoconfig(void);
int gateway_btmesh_prov_enable(uint8_t enable);

#endif

