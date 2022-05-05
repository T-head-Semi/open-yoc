/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef __AT_MESH_H
#define __AT_MESH_H

#ifdef CONFIG_UNITTEST_MODE
#define _STATIC
#else
#define _STATIC static
#endif

#define ABS(errno)  ((errno) < (0) ? (-errno) : (errno))
#define MESH_BASE_ERR  0x00000000
#define MESH_AT_ERR   0x000007D0
#define ERR_MESH(errno) (-(MESH_BASE_ERR + ABS(errno)))
#define ERR_MESH_AT(errno)  (-(MESH_AT_ERR  + ABS(errno)))


#define AT_STATUS_OK                                 (0)
#define AT_ERR_MESH_AT_INIT_FAILD                   (-11)
#define AT_ERR_MESH_AT_NOT_INIT                     (-12)
#define AT_ERR_MESH_RST_FAILD                       (-13)
#define AT_ERR_MESH_KV_UNLOAD                       (-14)
#define AT_ERR_MESH_CMD_NOT_SUPPORTED               (-15)
#define AT_ERR_MESH_INIT_COMP_FAILD                 (-16)
#define AT_ERR_MESH_INIT_NODE_FAILD                 (-17)
#define AT_ERR_MESH_NOT_FOUND_MODEL                 (-18)
#define AT_ERR_MESH_PROVISIONER_NOT_ENABLED         (-19)
#define AT_ERR_MESH_SET_UUID_FILTER_FAILD           (-20)
#define AT_ERR_MESH_SHOW_DEV_FAILD                  (-21)
#define AT_ERR_MESH_ADD_NODE_FAILD                  (-22)
#define AT_ERR_MESH_ADD_DEV_FAILD                   (-23)
#define AT_ERR_MESH_DEL_DEV_FAILD                   (-24)
#define AT_ERR_MESH_INPUT_OOB_FAILD                 (-25)
#define AT_ERR_MESH_OOB_NOT_NEED                    (-26)
#define AT_ERR_MESH_GET_NODE_INFO_FAILD             (-27)
#define AT_ERR_MESH_GET_NODE_COMP_FAILD             (-28)
#define AT_ERR_MESH_GET_NETKEY_FAILD                (-29)
#define AT_ERR_MESH_GET_APPKEY_FAILD                (-30)
#define AT_ERR_MESH_ADD_NETKEY_FAILD                (-31)
#define AT_ERR_MESH_ADD_APPKEY_FAILD                (-32)
#define AT_ERR_MESH_BIND_APPKEY_FAILD               (-33)
#define AT_ERR_MESH_GET_SUB_ADDR_FAILD              (-34)
#define AT_ERR_MESH_ADD_SUB_ADDR_FAILD              (-35)
#define AT_ERR_MESH_DEL_SUB_ADDR_FAILD              (-36)
#define AT_ERR_MESH_GET_PUB_ADDR_FAILD              (-37)
#define AT_ERR_MESH_ADD_PUB_ADDR_FAILD              (-38)
#define AT_ERR_MESH_AUTO_CONFIG_FAILD               (-39)
#define AT_ERR_MESH_TX_FAILED                       (-40)
#define AT_ERR_MESH_ENABLE_MAC_FILTER_FAILD         (-41)
#define AT_ERR_MESH_DISABLE_MAC_FILTER_FAILD        (-42)
#define AT_ERR_MESH_ADD_MAC_FILTER_DEV_FAILD        (-43)
#define AT_ERR_MESH_DEL_MAC_FILTER_DEV_FAILD        (-44)
#define AT_ERR_MESH_CLR_MAC_FILTER_DEV_FAILD        (-45)
#define AT_ERR_MESH_GET_DEVKEY_FAILD                (-46)
#define AT_ERR_MESH_PROVISIONER_CONFIG_FAILD        (-47)
#define AT_ERR_MESH_PROVISIONER_GET_CONFIG_FAILD    (-48)
#define AT_ERR_MESH_PROVISIONER_AUTO_PROV_FAILD     (-49)
#define AT_ERR_MESH_PROVISIONER_ENABLE_FAILD        (-50)
#define AT_ERR_MESH_PROVISIONER_DISABLE_FAILD       (-51)
#define AT_ERR_MESH_PROVISIONER_SET_LPM_FLAG_FAIL   (-52)
#define AT_ERR_MESH_GET_VERSION_FAIL                (-53)
#define AT_ERR_MESH_OVERWRITE_SUB_LIST_FAIL         (-54)




#define DEF_BEARER 0X03
#define DEF_PROVISIONER_UNICAST_ADDR_LOCAL 0x0001
#define DEF_PROVISIONER_UNICAST_ADDR_START 0x0002
#define DEV_UUID {0xcf, 0xa0, 0xe3, 0x7e, 0x17, 0xd9, 0x11, 0xe8, 0x86, 0xd1, 0x5f, 0x1c, 0xe2, 0x8a, 0xde, 0x02}
#define DEV_NAME "AT_MESH_PROVISIONER"

#define DEF_AUTO_ADD_APPKEY_ONOFF (1)
#define DEF_FOUND_DEV_TIMEOUT (0)//s

#define DEF_AUTO_CONFIG_TIMEOUT_MIN (5) //s
#define DEF_AUTO_CONFIG_TIMEOUT DEF_AUTO_CONFIG_TIMEOUT_MIN

#define DEF_ATTENTION_TIMEOUT (15)//s
#define DEF_PUB_SET_TTL (3)
#define DEF_PUB_SET_PERIOD (0)
#define DEF_PUB_SET_COUNT (0)
#define DEF_PUB_SET_INTERVAlL (0)
#define DEF_PUB_SET_APPKEY_IDX (0)

#define DEF_SEND_APPKEY_IDX (0)
#if defined(CONFIG_BT_MESH_EXT_ADV)  && CONFIG_BT_MESH_EXT_ADV > 0
#define DEF_TRS_MAX_LENGTH  410
#else
#define DEF_TRS_MAX_LENGTH  88
#endif

#define AUTOCONFIG_DST_SUB_ADDR   CONFIG_MESH_AUTOCONFIG_DST_SUB_ADDR
#define AUTOCONFIG_DST_PUB_ADDR   CONFIG_MESH_AUTOCONFIG_DST_PUB_ADDR


typedef enum {
    OOB_NULL,
    OOB_NUM,
    OOB_STR,
    OOB_STATIC,
} oob_type_en;


#define DEF_NODE_ROLE (NODE|PROVISIONER)

typedef void (*at_mesh_cb)(uint32_t event, void *p_arg);

int at_mesh_init(at_mesh_cb app_cb);

#endif
