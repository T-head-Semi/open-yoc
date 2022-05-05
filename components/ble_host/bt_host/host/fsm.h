/*
 * Copyright (C) 2019-2021 Alibaba Group Holding Limited
 */

#ifndef _BT_FSM_H__
#define _BT_FSM_H__

#include "atomic.h"
#include "string.h"
#include "errno.h"

#define BT_FSM_HANDLE_UNUSED (-1)

#define BT_FSM_EV_DEFINE(__module, __ev) BT_FSM_EV_ ## __module ## _ ## __ev

#define BT_FSM_ST_DEFINE(__module, __st) BT_FSM_ST_ ## __module ## _ ## __st

#define BT_FSM_ACTION_DEFINE(__module, __ev, __hanlder) \
    case BT_FSM_EV_DEFINE(__module, __ev):        \
    {                                             \
        ret = __hanlder(msg);                     \
        break;                                    \
    }                                             \

#define BT_FSM_HANDLER_FUNC(__module)            \
    static int bt_fsm_ ## __module ## _handler(void *msg)

#define BT_FSM_HANDLER_FUNC_NAME(__module)            \
    bt_fsm_ ## __module ## _handler

#define BT_FSM_HANDLER_DEVINE(__module, ...)                 \
    static int BT_FSM_HANDLER_FUNC_NAME(__module)(void *msg)             \
    {                                                            \
        struct bt_fsm_msg_t *fsm_msg =  bt_fsm_get_fsm_msg(msg); \
        int ret = 0;                                             \
        switch (fsm_msg->event)                                  \
        {                                                       \
                __VA_ARGS__                                         \
                default:                                          \
                break;                                          \
        }                                                           \
        return ret;                                    \
    }

typedef enum {
    BT_FSM_EV_NONE = 0,

    /* fsm event define for hci core fsm */
    BT_FSM_EV_DEFINE(HCI_CORE, ADV_STARTED),
    BT_FSM_EV_DEFINE(HCI_CORE, ADV_STOPED),

    BT_FSM_EV_DEFINE(HCI_CORE, SCAN_STARTED),
    BT_FSM_EV_DEFINE(HCI_CORE, SCAN_STOPED),

    BT_FSM_EV_DEFINE(HCI_CORE, CONN_CREATE),
    BT_FSM_EV_DEFINE(HCI_CORE, CONN_SCAN),

    /* fsm event define for hci cmd fsm */
    BT_FSM_EV_DEFINE(HCI_CMD, CMD_SEND),
    BT_FSM_EV_DEFINE(HCI_CMD, CMD_CMPLETE),
    BT_FSM_EV_DEFINE(HCI_CMD, CMD_SENT),

    /* fsm event define for bt mesh fsm */
    BT_FSM_EV_DEFINE(BT_MESH, ADV_SEND),
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
    BT_FSM_EV_DEFINE(BT_MESH, ADV_PROXY_SEND),
#endif
    BT_FSM_EV_DEFINE(BT_MESH,  EXIT),

    BT_FSM_EV_MAX,
} bt_fsm_ev_en;

enum {
    BT_FSM_ST_DEFINE(HCI_CORE, CFG_PENDING),

    BT_FSM_ST_DEFINE(HCI_CORE, ADV_START),
    BT_FSM_ST_DEFINE(HCI_CORE, ADV_STOP),
    BT_FSM_ST_DEFINE(HCI_CORE, ADV_DATA_UPDATE),

    BT_FSM_ST_DEFINE(HCI_CORE, SCAN_START),
    BT_FSM_ST_DEFINE(HCI_CORE, SCAN_STOP),
    BT_FSM_ST_DEFINE(HCI_CORE, SCAN_UPDATE),

    BT_FSM_ST_DEFINE(HCI_CORE, CONN_CREATE),

#if defined(CONFIG_BT_EXT_ADV)
    BT_FSM_ST_DEFINE(HCI_CORE, EXT_ADV_CREATE),
    BT_FSM_ST_DEFINE(HCI_CORE, EXT_ADV_PARAM_UPDATE),
    BT_FSM_ST_DEFINE(HCI_CORE, EXT_ADV_DATA_UPDATE),
    BT_FSM_ST_DEFINE(HCI_CORE, EXT_ADV_START),
    BT_FSM_ST_DEFINE(HCI_CORE, EXT_ADV_STOP),
    BT_FSM_ST_DEFINE(HCI_CORE, EXT_CONN_CREATE),
#endif

    BT_FSM_ST_DEFINE(HCI_CORE, INVAILD), /* MAX support is 31*/
};

/* fsm state define for hci cmd fsm*/
enum {
    BT_FSM_ST_DEFINE(HCI_CMD, SET_PENDING),
};

/* fsm state define for bt mes fsm*/
enum {
    BT_FSM_ST_DEFINE(BT_MESH, ADV_SEND),
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
    BT_FSM_ST_DEFINE(BT_MESH, ADV_PROXY_SEND),
#endif
    BT_FSM_ST_DEFINE(BT_MESH, SCAN),

    BT_FSM_ST_DEFINE(BT_MESH, ADV_START),
    BT_FSM_ST_DEFINE(BT_MESH, ADV_STARTED),
    BT_FSM_ST_DEFINE(BT_MESH, ADV_STOP),
    BT_FSM_ST_DEFINE(BT_MESH, ADV_STOPED),
    BT_FSM_ST_DEFINE(BT_MESH, SCAN_START),
    BT_FSM_ST_DEFINE(BT_MESH, SCAN_STARTED),
    BT_FSM_ST_DEFINE(BT_MESH, SCAN_STOP),
    BT_FSM_ST_DEFINE(BT_MESH, SCAN_STOPED),
};

typedef int8_t bt_fsm_handle_t;

typedef int (*bt_fsm_handler_t)(void *msg);

struct bt_fsm_msg_t {
    sys_snode_t next;
    bt_fsm_handle_t fsm_handle;
    bt_fsm_handle_t fsm_from_handle;
    bt_fsm_ev_en event;
};

enum {
    FSM_RET_SUCCESS = 0,
    FSM_RET_MSG_FREE = FSM_RET_SUCCESS,
    FSM_RET_MSG_RETAIN = 1,
    FSM_RET_MSG_NOFREE = 2,
};

struct hci_core_fsm_msg_t {
	union
	{
		int status;
		void *msg;
	};
	void *args;
};

struct hci_cmd_fsm_msg_t {
	union
	{
		int  status;
		void *msg;
	};
	void *args;
};

bt_fsm_handle_t bt_fsm_init(bt_fsm_handler_t handler);

void *bt_fsm_create_msg(uint32_t size, bt_fsm_handle_t dst_handle, bt_fsm_handle_t src_handle);

void bt_fsm_free_msg(void *msg);

void bt_fsm_set_msg_dst(void *msg, bt_fsm_handle_t dst_handle);

void bt_fsm_set_msg_from(void *msg, bt_fsm_handle_t from_handle);

bt_fsm_handle_t bt_fsm_get_msg_from(void *msg);

struct bt_fsm_msg_t *bt_fsm_get_fsm_msg(void *msg);

void bt_fsm_reverse_msg(void *msg);

int bt_fsm_set_event(bt_fsm_ev_en event, void *msg);

int bt_fsm_wait_event(bt_fsm_handle_t handle, bt_fsm_ev_en event);

int bt_fsm_flush_event(bt_fsm_handle_t handle, bt_fsm_ev_en event);

void bt_fsm_set_state(bt_fsm_handle_t fsm_handle, int state);

void bt_fsm_clear_state(bt_fsm_handle_t fsm_handle, int state);

void bt_fsm_clear_state_all(bt_fsm_handle_t fsm_handle);

int bt_fsm_test_state(bt_fsm_handle_t fsm_handle, int state);

void bt_fsm_set_ready(bt_fsm_handle_t handle);

int bt_fsm_process();

#endif