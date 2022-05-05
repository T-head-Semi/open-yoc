#ifndef __GATEWAY_UT_H__
#define __GATEWAY_UT_H__

#include "gateway_cmds.h"

#include "smartliving/iot_import.h"
#include "smartliving/exports/iot_export_linkkit.h"

#define GATEWAY_SUPPORT_GROUP_CTRL  1

// for demo only
#define PRODUCT_KEY "PK_XXXXX"
#define PRODUCT_SECRET "PS_XXXXX"
#define DEVICE_NAME "DN_XXXXX"
#define DEVICE_SECRET "DS_XXXXX"

#define TOPO_LIST_PK "productKey"
#define TOPO_LIST_DN "deviceName"

#define TOPO_CHANGE_STATUS "status"
#define TOPO_CHANGE_SUBLIST "subList"

// for demo currently, later will get these info from occ
#define BLE_MESH_LIGHT_PK "a1faJDbzH2E"
#define BLE_MESH_LIGHT_PS "SLEKF6F3L4xtI8yS"

#define QUEUE_MSG_SIZE sizeof(gateway_msg_t)
#define MAX_QUEUE_SIZE 10 * QUEUE_MSG_SIZE

#define SUB_DEV_QUEUE_MSG_SIZE sizeof(gateway_add_subdev_msg_t)
#define MAX_SUB_DEV_QUEUE_SIZE 10 * SUB_DEV_QUEUE_MSG_SIZE

#define MAX_RETRY_TIME 5

#ifndef MAX_DEVICES_META_NUM
#define MAX_DEVICES_META_NUM (32)
#endif

//You should undefine this Macro in your products
#define GATEWAY_UT_TESTING

//KV config gw type,VALUE:master mean it is gateway,slave is subdev
#define GATEWAY_DEVICE_TYPE_KEY "gwtype"
#define GATEWAY_DEVICE_TYPE_MASTER "master"
#define GATEWAY_DEVICE_TYPE_SLAVE "slave"
#define GATEWAY_DEVICE_TYPE_LEN (7)

#define GATEWAY_FORBIDDEN_AUTO_ADD_SUBDEV_FLAG_KEY "gwf"
#define GATEWAY_FORBIDDEN_AUTO_ADD_SUBDEV_FLAG_LEN 2

#define GATEWAY_MAGIC_CODE 0x736A

#define GATEWAY_NODE_INVAL -1
#define GATEWAY_NODE_UNREG -2
#define GATEWAY_NODE_UNBIND 0
#define GATEWAY_NODE_BINDED 1

#define ROOM_TYPE_MAX_LEN   16
#define DEVICE_TYPE_MAX_LEN 16 

#define IS_DEVICE_TYPE_EQUA(a,b) (a&&b==NULL)?0:!strcmp(a,b)
#define IS_POSITION_EQUA(a,b) (b==NULL)?1:!strcmp(a,b)  // b == NULL means no position specified

typedef enum _gw_topo_change_status_e
{
    GW_TOPO_CHANGE_STATUS_ADD = 0,
    GW_TOPO_CHANGE_STATUS_DELETE = 1,
    GW_TOPO_CHANGE_STATUS_ENABLE = 2,
    GW_TOPO_CHANGE_STATUS_DISABLE = 8,
    GW_TOPO_CHANGE_STATUS_INVALID
} gw_topo_change_status_e;

typedef enum _gw_device_type_e
{
    GW_DEVICE_MASTER = 0,
    GW_DEVICE_SLAVE,
    GW_DEVICE_INVALID
} gw_device_type_e;

typedef enum _gw_topo_get_reason_e
{
    GW_TOPO_GET_REASON_CONNECT_CLOUD = 0,
    GW_TOPO_GET_REASON_CLI_CMD,
    GW_TOPO_GET_REASON_MAX
} gw_topo_get_reason_e;

typedef enum _gateway_msg_type_e
{
    GATEWAY_MSG_TYPE_ADD,
    GATEWAY_MSG_TYPE_ADD_RANGE,
    GATEWAY_MSG_TYPE_DEL,
    GATEWAY_MSG_TYPE_DEL_RANGE,
    GATEWAY_MSG_TYPE_RESET,
    GATEWAY_MSG_TYPE_UPDATE,
    GATEWAY_MSG_TYPE_ADDALL,
    GATEWAY_MSG_TYPE_DELALL,
    GATEWAY_MSG_TYPE_QUERY_SUBDEV_ID,
    GATEWAY_MSG_TYPE_CLOUD_CONNECT,
    GATEWAY_MSG_TYPE_PERMIT_JOIN,
    GATEWAY_MSG_TYPE_LOGIN_ALL,
    GATEWAY_MSG_TYPE_LOGOUT_ALL,
    GATEWAY_MSG_TYPE_MAX
} gateway_msg_type_e;

typedef struct _gateway_msg_s
{
    gateway_msg_type_e msg_type;
    int devid;
    int devid_end;
    char *payload;
    int payload_len;
} gateway_msg_t;

typedef struct _gateway_add_subdev_msg_s
{
    uint8_t addr[6];
    char cid[33];
    uint8_t retry_time;
} gateway_add_subdev_msg_t;


typedef struct _subdev_info_s
{
    short cloud_devid;
    iotx_linkkit_dev_meta_info_t linkkit_dev_meta_info;
    char occ_pid[32];
    char room_type[ROOM_TYPE_MAX_LEN];     // room_type and device_type are used for voice control
    char device_type[DEVICE_TYPE_MAX_LEN];
    uint8_t power_state;
    uint8_t bri;
    uint16_t cct;
    uint8_t bind;
} subdev_info_t;

typedef struct _gw_subdev_info_s
{
    short magic_code;   // to indicate whether infor from flash is valid
    subdev_info_t subdev_info[MAX_DEVICES_META_NUM+1];
} gw_subdev_info_t;



extern int gateway_ut_init(void);
extern int gateway_ut_handle_permit_join(void);
extern int gateway_ut_handle_topolist_reply(const char *payload, const int payload_len);
extern int gateway_ut_send_msg(gateway_msg_t *, int);
extern int gateway_ut_msg_process(int master_devid, int timeout);
extern void gateway_ut_misc_process(uint64_t time_now_sec);
extern int gateway_ut_update_subdev(gw_topo_get_reason_e reason);
extern void gateway_ut_enable_mesh_handle(uint8_t mesh_en);
extern void gateway_ut_set_mesh_prov_flag(uint8_t prov_flag);
extern void gateway_ut_set_alloc_start_addr(uint16_t start_addr);
extern void gateway_ut_set_alloc_end_addr(uint16_t end_addr);
extern void gateway_ut_set_mac_filter(uint8_t *mac);
extern void gateway_ut_del_mac_filter(uint8_t *mac);
extern void gateway_ut_arrange_cloud_id(void);
extern int gateway_ut_send_add_subdev_msg(gateway_add_subdev_msg_t *msg, int len);
extern void gateway_subdev_registered_handle(int devid);
extern void gateway_subdev_delete_handle(int devid);
extern void gateway_subdev_binded_handle(int devid);
extern int gateway_subdev_turn_onoff(char *mac, uint8_t pwrstate);
extern int gateway_subdev_set_brightness(char *mac, uint8_t bri);
extern int gateway_subdev_set_cct(char *mac, uint16_t cct);
extern int gateway_gw_turn_onoff(uint8_t pwrstate);
extern int gateway_subdev_rpt_onoff(char *mac, uint8_t pwrstate);
extern int gateway_subdev_rpt_brightness(char *mac, uint8_t bri);
extern int gateway_subdev_turn_onoff_by_position(char *device_type, char *position, uint8_t onoff);
extern int gateway_subdev_update_onoff_by_position(char *device_type, char *position, uint8_t onoff);
extern int gateway_subdev_dn2mac(const char *dn, char *mac);
extern int gateway_subdev_devid2mac(int devid, char *mac);
extern int gateway_subdev_mac2devid(int *devid, char *mac);
extern int gateway_subdev_devid2occpid(int devid, char *occ_pid);
extern int gateway_subdev_mac2occpid(uint8_t *mac, char *occ_pid);
extern int gateway_subdev_dn2index(const char *dn, uint8_t *index);
extern void gateway_subdev_mesh_add(uint8_t *mac, uint8_t *cid, uint8_t retry_time);
extern void gateway_subdev_mesh_del(uint8_t *mac);
extern void gateway_get_subdev_kv_string(int idx, char *key_string);
extern int gateway_subdev_update_cct(char *dev_addr, uint16_t cct);
extern int gateway_subdev_update_brightness(char *dev_addr, uint8_t bri);
extern int gateway_subdev_update_power_state(char *dev_addr, uint8_t onoff);
extern int gateway_subdev_del_node(char *dev_addr);
extern int gateway_subdev_sub(char *dev_addr, uint16_t mod_id, uint16_t sub_addr);
#if defined (CONFIG_SAVE_JS_TO_RAM) && (CONFIG_SAVE_JS_TO_RAM == 1)
extern void gateway_subdev_get_occ_js(void);
#endif
extern int gateway_ut_devlist2json(char *json_str);

#endif
