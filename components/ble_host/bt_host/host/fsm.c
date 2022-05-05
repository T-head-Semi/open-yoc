/*
 * Copyright (C) 2019-2021 Alibaba Group Holding Limited
 */

#include "misc/slist.h"
#include "misc/util.h"
#include "ble_os.h"
#include "fsm.h"

#define CONFIG_BT_FSM_MAX_NUM (3)

enum {
    BT_FSM_EV = 1,
    BT_FSM_MSG = 2,
};

extern void hci_rx_signal();

#define FSM_STRING(x) #x

#define BT_FSM_CS uint32_t cs;
#define BT_FSM_LOCK do {cs = irq_lock();} while(0);
#define BT_FSM_UNLOCK do {irq_unlock(cs);} while(0);
#define BT_FSM_SIGNAL do {hci_rx_signal();}while(0);

#define FSM_LOGE(fmt, ...) printf("[FSM][E](%p)"fmt, k_current_get(), ##__VA_ARGS__)
#define FSM_LOGD(fmt, ...) //printf("[FSM][D](%p)"fmt, k_current_get(), ##__VA_ARGS__)

#define FSM_DEBUG_STR

#define BT_FSM_ALLOC(size) malloc(size)
#define BT_FSM_FREE(ptr) free(ptr)

typedef struct bt_fsm_t {
    sys_snode_t next;
    atomic_t cur_state;
    uint16_t ready: 1;
    uint16_t reserve: 8;
    bt_fsm_ev_en wait_event;
    bt_fsm_handle_t handle;
    bt_fsm_handler_t handler;
} bt_fsm_t;

struct bt_fsm_t fsms[CONFIG_BT_FSM_MAX_NUM] = {0};

struct fsm_t {
    sys_slist_t ready_list;
    sys_slist_t wait_list;
    sys_slist_t msg_list;
    sys_slist_t retain_msg_list;
    uint8_t event_mask;
} g_fsm = {0};

NET_BUF_POOL_FIXED_DEFINE(bt_fsm_msg_pool, 10, 10, NULL);

#ifdef FSM_DEBUG_STR
static inline const char *bt_fsm_ev_string(bt_fsm_ev_en ev)
{
    switch (ev)
    {
        case BT_FSM_EV_NONE                           : {return FSM_STRING(BT_FSM_EV_NONE);};
        case BT_FSM_EV_DEFINE(HCI_CORE, ADV_STARTED)  : {return FSM_STRING((HCI_CORE, ADV_STARTED));};
        case BT_FSM_EV_DEFINE(HCI_CORE, ADV_STOPED)   : {return FSM_STRING((HCI_CORE, ADV_STOPED));};
        case BT_FSM_EV_DEFINE(HCI_CORE, SCAN_STARTED) : {return FSM_STRING((HCI_CORE, SCAN_STARTED));};
        case BT_FSM_EV_DEFINE(HCI_CORE, SCAN_STOPED)  : {return FSM_STRING((HCI_CORE, SCAN_STOPED));};
        case BT_FSM_EV_DEFINE(HCI_CORE, CONN_CREATE)  : {return FSM_STRING((HCI_CORE, CONN_CREATE));};
        case BT_FSM_EV_DEFINE(HCI_CORE, CONN_SCAN)    : {return FSM_STRING((HCI_CORE, CONN_SCAN));};
        case BT_FSM_EV_DEFINE(HCI_CMD, CMD_SEND)      : {return FSM_STRING((HCI_CMD, CMD_SEND));};
        case BT_FSM_EV_DEFINE(HCI_CMD, CMD_CMPLETE)   : {return FSM_STRING((HCI_CMD, CMD_CMPLETE));};
        case BT_FSM_EV_DEFINE(HCI_CMD, CMD_SENT)      : {return FSM_STRING((HCI_CMD, CMD_SENT));};
        case BT_FSM_EV_DEFINE(BT_MESH, ADV_SEND)       : {return FSM_STRING((BT_MESH, ADV_SEND));};
#if defined(CONFIG_BT_MESH_PROXY) && CONFIG_BT_MESH_PROXY
        case BT_FSM_EV_DEFINE(BT_MESH, ADV_PROXY_SEND) : {return FSM_STRING((BT_MESH, ADV_PROXY_SEND));};
#endif
        default:
            break;
    }

    return "UNKNOWN";
}
#endif

void *bt_fsm_create_msg(uint32_t size, bt_fsm_handle_t dst_handle, bt_fsm_handle_t src_handle)
{
    struct bt_fsm_msg_t *fsm_msg;

    fsm_msg = BT_FSM_ALLOC(sizeof(struct bt_fsm_msg_t) + size);

    if (!fsm_msg) {
        return NULL;
    }

    memset(fsm_msg, 0, sizeof(struct bt_fsm_msg_t) + size);

    FSM_LOGD("bt_fsm_create_msg %p \n", fsm_msg);

    fsm_msg->fsm_handle = dst_handle;
    fsm_msg->fsm_from_handle = src_handle;

    /* reserve fsm_msg */
    fsm_msg++;

    return fsm_msg;
}

void bt_fsm_free_msg(void *msg)
{
    struct bt_fsm_msg_t *fsm_msg = msg;

    fsm_msg--;

    FSM_LOGD("bt_fsm_free_msg %p \n", fsm_msg);

    BT_FSM_FREE(fsm_msg);
}

void bt_fsm_set_msg_dst(void *msg, bt_fsm_handle_t dst_handle)
{
    struct bt_fsm_msg_t *fsm_msg = msg;
    fsm_msg--;

    fsm_msg->fsm_handle = dst_handle;

    return;
}

void bt_fsm_set_msg_from(void *msg, bt_fsm_handle_t from_handle)
{
    struct bt_fsm_msg_t *fsm_msg = msg;
    fsm_msg--;

    fsm_msg->fsm_from_handle = from_handle;

    return;
}

bt_fsm_handle_t bt_fsm_get_msg_dst(void *msg)
{
    struct bt_fsm_msg_t *fsm_msg = msg;
    fsm_msg--;

    return fsm_msg->fsm_handle;
}

bt_fsm_handle_t bt_fsm_get_msg_from(void *msg)
{
    struct bt_fsm_msg_t *fsm_msg = msg;
    fsm_msg--;

    return fsm_msg->fsm_from_handle;
}

void bt_fsm_reverse_msg(void *msg)
{
    bt_fsm_handle_t handle;
    struct bt_fsm_msg_t *fsm_msg = msg;
    fsm_msg--;

    handle = fsm_msg->fsm_from_handle;
    fsm_msg->fsm_from_handle = fsm_msg->fsm_handle;
    fsm_msg->fsm_handle = handle;

    return;
}

struct bt_fsm_msg_t *bt_fsm_get_fsm_msg(void *msg)
{
    struct bt_fsm_msg_t *fsm_msg = msg;
    fsm_msg--;
    return fsm_msg;
}

void bt_fsm_set_ready(bt_fsm_handle_t handle)
{
    if (handle >= CONFIG_BT_FSM_MAX_NUM) {
        return;
    }

    struct bt_fsm_t *fsm = &fsms[handle];

    FSM_LOGD("fsm[%d] set ready\n", handle);

    BT_FSM_CS;

    BT_FSM_LOCK;

    fsm->ready = 1;

    fsm->wait_event = 0;

    BT_FSM_UNLOCK;

    BT_FSM_SIGNAL;
}

int bt_fsm_is_ready(bt_fsm_handle_t handle)
{
    if (handle >= CONFIG_BT_FSM_MAX_NUM) {
        return 0;
    }

    struct bt_fsm_t *fsm = &fsms[handle];

    return fsm->ready;
}

bt_fsm_handle_t bt_fsm_init(bt_fsm_handler_t handler)
{
    static bt_fsm_handle_t next_fsm_handle = 0;

    if (next_fsm_handle >= CONFIG_BT_FSM_MAX_NUM) {
        return -1;
    }

    struct bt_fsm_t *fsm = &fsms[next_fsm_handle];

    memset(fsm, 0, sizeof(bt_fsm_t));

    fsm->handle = next_fsm_handle;

    fsm->handler = handler;

    bt_fsm_set_ready(fsm->handle);

    sys_slist_append(&g_fsm.ready_list, &fsm->next);

    next_fsm_handle++;

    return fsm->handle;
}

int bt_fsm_set_event(bt_fsm_ev_en event, void *msg)
{
    int ret = 0;
    struct bt_fsm_msg_t *fsm_msg =  msg;

    if (fsm_msg) {
        fsm_msg--;

        fsm_msg->event = event;

        BT_FSM_CS;
        BT_FSM_LOCK;
        sys_slist_append(&g_fsm.msg_list, &fsm_msg->next);
        BT_FSM_UNLOCK;
#ifdef FSM_DEBUG_STR
        FSM_LOGD("fsm[%d] msg %p from fsm[%d] set ev %s\n", fsm_msg->fsm_handle, msg, fsm_msg->fsm_from_handle, bt_fsm_ev_string(event));
#else
        FSM_LOGD("fsm[%d] msg %p from fsm[%d] set ev %d\n", fsm_msg->fsm_handle, msg, fsm_msg->fsm_from_handle, event);
#endif
    }

    BT_FSM_SIGNAL;
    return ret;
}

int bt_fsm_wait_event(bt_fsm_handle_t handle, bt_fsm_ev_en event)
{
    int ret = 0;

    if (handle >= CONFIG_BT_FSM_MAX_NUM) {
        return -1;
    }

    struct bt_fsm_t *fsm = &fsms[handle];

    if (!fsm->ready) {
        return -1;
    }
#ifdef FSM_DEBUG_STR
    FSM_LOGD("fsm[%d] wait ev %s\n", handle,bt_fsm_ev_string(event));
#else
    FSM_LOGD("fsm[%d] wait ev %d\n", handle, event);
#endif

    BT_FSM_CS;
    BT_FSM_LOCK;
    fsm->ready = 0;
    fsm->wait_event = event;
    BT_FSM_UNLOCK;

    BT_FSM_SIGNAL;
    return ret;
}

int bt_fsm_flush_event(bt_fsm_handle_t handle, bt_fsm_ev_en event)
{
    struct bt_fsm_msg_t *tmp;
    struct bt_fsm_msg_t *fsm_msg;
    sys_slist_t free_list = {0};

    BT_FSM_CS;
    BT_FSM_LOCK;

    if (!sys_slist_is_empty(&g_fsm.retain_msg_list)) {
        SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&g_fsm.retain_msg_list, fsm_msg, tmp, next) {
            if (fsm_msg->fsm_handle == handle && event == fsm_msg->event) {
                sys_slist_find_and_remove(&g_fsm.retain_msg_list, &fsm_msg->next);
                sys_slist_append(&free_list, &fsm_msg->next);
            }
        }
    }

    if (!sys_slist_is_empty(&g_fsm.msg_list)) {
            SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&g_fsm.msg_list, fsm_msg, tmp, next) {
            if (fsm_msg->fsm_handle == handle && event == fsm_msg->event) {
                sys_slist_find_and_remove(&g_fsm.msg_list, &fsm_msg->next);
                sys_slist_append(&free_list, &fsm_msg->next);
            }
        }
    }

    BT_FSM_UNLOCK;

    if (!sys_slist_is_empty(&free_list)) {
            SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&free_list, fsm_msg, tmp, next) {
                bt_fsm_free_msg(++fsm_msg);
        }
    }

    return 0;
}

static inline struct bt_fsm_t *_bt_fsm_get_ready_fsm(struct bt_fsm_msg_t *fsm_msg)
{
    struct bt_fsm_t *fsm;

    ASSERT(fsm_msg->fsm_handle < CONFIG_BT_FSM_MAX_NUM, "fsm handle invaild");

    fsm = &fsms[fsm_msg->fsm_handle];

    return fsm;
}

static inline struct bt_fsm_msg_t *_bt_fsm_get_fsm_msg()
{
    struct bt_fsm_msg_t *fsm_msg;

    BT_FSM_CS;
    BT_FSM_LOCK;
    fsm_msg = (struct bt_fsm_msg_t *)sys_slist_get(&g_fsm.msg_list);
    BT_FSM_UNLOCK;

    return fsm_msg;
}

static inline  void _bt_fsm_retain_fsm_msg(struct bt_fsm_msg_t *fsm_msg)
{
    BT_FSM_CS;
    BT_FSM_LOCK;
    sys_slist_append(&g_fsm.retain_msg_list, &fsm_msg->next);
    BT_FSM_UNLOCK;
    FSM_LOGD("retain fsm msg %p\n", fsm_msg);
    return;
}

static inline  void _bt_fsm_update_fsm_msg(bt_fsm_handle_t fsm_handle)
{
    int8_t update = 0;

    BT_FSM_CS;
    BT_FSM_LOCK;

    if (!sys_slist_is_empty(&g_fsm.retain_msg_list)) {
        struct bt_fsm_msg_t *tmp;
        struct bt_fsm_msg_t *fsm_msg;

        SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&g_fsm.retain_msg_list, fsm_msg, tmp, next) {
            if (fsm_msg->fsm_handle == fsm_handle) {
                sys_slist_find_and_remove(&g_fsm.retain_msg_list, &fsm_msg->next);
                sys_slist_append(&g_fsm.msg_list, &fsm_msg->next);
                update = 1;
            }
        }

        if (update) {
            bt_fsm_set_event(BT_FSM_EV_NONE, NULL);
        }
    }

    BT_FSM_UNLOCK;

    return;
}

int bt_fsm_process()
{
    int ret = 0;
    struct bt_fsm_msg_t *fsm_msg = NULL;
    bt_fsm_t *fsm = NULL;

#if 0

    if (!sys_slist_is_empty(&g_fsm.retain_msg_list)) {
        struct bt_fsm_msg_t *tmp;
        struct bt_fsm_msg_t *fsm_msg;

        SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&g_fsm.retain_msg_list, fsm_msg, tmp, next) {
            ASSERT(fsm_msg->fsm_handle < CONFIG_BT_FSM_MAX_NUM, "fsm handle invaild");
            /* skip fsm msg head */
            msg = fsm_msg + 1;

            if (bt_fsm_is_ready(fsm_msg->fsm_handle)) {
                BT_FSM_CS;
                BT_FSM_LOCK;
                sys_slist_find_and_remove(&g_fsm.retain_msg_list, &fsm_msg->next);
                BT_FSM_UNLOCK;

                ret = fsms[fsm_msg->fsm_handle].handler(msg);

                if (ret < 0) {
                    FSM_LOGE("fsm handler %d ret %d\n", fsm_msg->fsm_handle, ret);
                }

                if (ret != FSM_RET_MSG_RETAIN) {
                    bt_fsm_free_msg(msg);
                }
            }
        }
    }

#endif

    while (1) {
        bt_fsm_handler_t handler = NULL;
        void *msg;

        fsm_msg = _bt_fsm_get_fsm_msg();

        if (!fsm_msg) {
            break;
        }

        fsm = _bt_fsm_get_ready_fsm(fsm_msg);

        if (!fsm) {
            FSM_LOGE("invaild fsm_msg %p\n", fsm_msg);
            bt_fsm_free_msg(++fsm_msg);
            continue;
        }

        if (fsm->ready) {
            handler = fsm->handler;
        } else if (fsm->wait_event == fsm_msg->event) {
            bt_fsm_set_ready(fsm->handle);
            handler = fsm->handler;
        } else {
            _bt_fsm_retain_fsm_msg(fsm_msg);
            continue;
        }

        /* skip fsm msg head */
        msg = fsm_msg + 1;

        ret = handler(msg);

#ifdef FSM_DEBUG_STR
        FSM_LOGD("fsm[%d]->handler[%p](msg %p) ev %s ret %d\n", fsm->handle, fsm->handler, msg, bt_fsm_ev_string(fsm_msg->event), ret);
#else
        FSM_LOGD("fsm[%d]->handler[%p](msg %p) ev %d ret %d\n", fsm->handle, fsm->handler, msg, fsm_msg->event, ret);
#endif

        if (ret == FSM_RET_MSG_RETAIN) {
            _bt_fsm_retain_fsm_msg(fsm_msg);
        } else if (ret == FSM_RET_MSG_FREE) {
            bt_fsm_free_msg(msg);
        } else if (ret == FSM_RET_MSG_NOFREE) {
            /* this fsm msg will free by fsm handler */
        } else {
            FSM_LOGE("fsm[%d]->handler[%p](msg %p) ret %d\n", fsm->handle, fsm->handler, msg, ret);
            bt_fsm_free_msg(msg);
        }
    }

    return 0;
}

void bt_fsm_set_state(bt_fsm_handle_t fsm_handle, int state)
{
    struct bt_fsm_t *fsm = &fsms[fsm_handle];
    atomic_set_bit(&fsm->cur_state, state);
    _bt_fsm_update_fsm_msg(fsm_handle);
    FSM_LOGD("fsm[%d] set st %d, cur_st %x\n", fsm_handle, state, fsm->cur_state);
}

void bt_fsm_clear_state(bt_fsm_handle_t fsm_handle, int state)
{
    struct bt_fsm_t *fsm = &fsms[fsm_handle];
    atomic_clear_bit(&fsm->cur_state, state);
    _bt_fsm_update_fsm_msg(fsm_handle);
    FSM_LOGD("fsm[%d] clr st %d, cur_st %x\n", fsm_handle, state, fsm->cur_state);
}

void bt_fsm_clear_state_all(bt_fsm_handle_t fsm_handle)
{
    struct bt_fsm_t *fsm = &fsms[fsm_handle];
    atomic_set(&fsm->cur_state, 0);
    _bt_fsm_update_fsm_msg(fsm_handle);
    FSM_LOGD("fsm[%d] clra cur_st %x\n", fsm_handle, fsm->cur_state);
}

int bt_fsm_test_state(bt_fsm_handle_t fsm_handle, int state)
{
    struct bt_fsm_t *fsm = &fsms[fsm_handle];
    return atomic_test_bit(&fsm->cur_state, state);
}
