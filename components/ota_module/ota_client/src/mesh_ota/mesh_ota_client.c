#if defined(CONFIG_BT_MESH_MODEL_BLOB_SRV)  && CONFIG_BT_MESH_MODEL_BLOB_SRV > 0

#include "stdint.h"
#include "errno.h"
#include "mesh_model/mesh_model.h"
#include "mesh_ota/mesh_ota_client.h"
#include "sig_model/blob_srv.h"
#include "inc/net.h"
#include "dfu_port.h"
#include "timer_port.h"
#include "aos/kv.h"
#include "common_mesh_ota.h"


#define TAG "MESH_OTA_CLIENT"


#ifndef CONFIG_MESH_OTA_CLIENT_BUFFER_SIZE
#define CONFIG_MESH_OTA_CLIENT_BUFFER_SIZE  1024
#endif


#define  ERASE_MISC_FLAG    "ERASE_ALL_MISC"

typedef struct {
    uint64_t blob_id;
    uint32_t blob_size;
    uint16_t block_size_log;
    uint16_t mtu_size;
} ota_image_info;

typedef  enum {
    IMAGE_POS_FLASH,
    IMAGE_POS_RAM,
} ota_image_pos_en;


static uint8_t  recv_buffer[CONFIG_MESH_OTA_CLIENT_BUFFER_SIZE];

typedef struct {
    struct net_buf_simple buf;
} blob_buffer;

typedef struct _mesh_ota_client_ctx {
    uint8_t init_flag;
    uint8_t status;
    uint8_t report_try;
    uint16_t src_addr;
    uint16_t crc;
    blob_info    *blob_info;
    blob_block_info *block_info;
    blob_buffer save_buffer;
    ota_timer_t ota_client_timer;
    struct k_delayed_work store_work;
    struct k_sem        store_sem;
} mesh_ota_client_ctx;


static mesh_ota_client_ctx g_mesh_ota_client_ctx = {0x00};

typedef struct {
    uint16_t block_num;
    uint16_t chunk_num;
    uint16_t data_len;
} chunk_data_head;


static void _blob_buffer_reset(blob_buffer *buffer, uint8_t *data, uint16_t size)
{
    buffer->buf.data  =  data;
    buffer->buf.__buf =  data;
    buffer->buf.size  =  size;
    buffer->buf.len   = 0;
}


static int _blob_buffer_simple_save(blob_buffer *buffer, uint16_t block_num, uint16_t chunk_num, struct net_buf_simple *data)
{
    if (net_buf_simple_tailroom(&buffer->buf) <  sizeof(chunk_data_head) + data->len) {
        LOGE(TAG, "blob buffer full");
        return -ENOMEM;
    }

    net_buf_simple_add_le16(&buffer->buf, block_num);
    net_buf_simple_add_le16(&buffer->buf, chunk_num);
    net_buf_simple_add_le16(&buffer->buf, data->len);
    net_buf_simple_add_mem(&buffer->buf,  data->data, data->len);
    return 0;
}


static uint8_t *_blob_buffer_simple_pull(blob_buffer *buffer, chunk_data_head *head)
{
    if (buffer->buf.len < sizeof(chunk_data_head) + 1) {
        LOGE(TAG, "blob buffer empty");
        return NULL;
    }

    head->block_num  = net_buf_simple_pull_le16(&buffer->buf);
    head->chunk_num  = net_buf_simple_pull_le16(&buffer->buf);
    head->data_len   = net_buf_simple_pull_le16(&buffer->buf);
    return net_buf_simple_pull_mem(&buffer->buf, head->data_len);

}


int _blob_report_crc(uint16_t netkey_idx, uint16_t appkey_idx, uint16_t addr, uint16_t crc)
{
#if defined(CONFIG_BT_MESH_MODEL_VENDOR_SRV) && CONFIG_BT_MESH_MODEL_VENDOR_SRV > 0
    struct bt_mesh_model *vendor_model;
    vendor_model = ble_mesh_model_find(0, BT_MESH_MODEL_VND_MODEL_SRV, CONFIG_CID_TAOBAO);

    if (!vendor_model) {
        return -EOPNOTSUPP;
    }

    vnd_model_msg  vnd_data = {0};
    uint8_t data[4] = {0};

    data[0] = ATTR_TYPE_REPORT_OTA_CRC & 0xff;
    data[1] = (ATTR_TYPE_REPORT_OTA_CRC >> 8) & 0xff;
    data[2] =  crc & 0xff;
    data[3] = (crc >> 8) & 0xff;

    vnd_data.netkey_idx = netkey_idx;
    vnd_data.appkey_idx = appkey_idx;
    vnd_data.dst_addr = addr;
    vnd_data.model = vendor_model;
    vnd_data.opid = VENDOR_OP_ATTR_INDICATE;
    vnd_data.data = data;
    vnd_data.retry = 5; //TEMP
    vnd_data.len = sizeof(data);
    return  ble_mesh_vendor_srv_model_msg_send(&vnd_data);
#endif
    return -EOPNOTSUPP;

}


static void _vendor_confirm_message_process(model_message message)
{

    if (message.ven_data.data_len < 2) {
        return;
    }

    uint8_t *data = (uint8_t *)message.ven_data.user_data;

    uint16_t  status_op = data[0] | data[1] << 8;

    switch (status_op) {
        case ATTR_TYPE_REPORT_OTA_CRC : {
            uint8_t   crc_check_status = data[2];
            ota_timer_stop(&g_mesh_ota_client_ctx.ota_client_timer);

            if (crc_check_status != MESH_OTA_CRC_CHECK_SUCCESS) {
                LOGE(TAG, "Node Crc fail by srv");
                erase_dfu_flash(1);
            } else {
                LOGD(TAG, "Node Crc success by srv, Dfu rebooot now");
                dfu_reboot();
            }
        }
        break;

        default:
            break;
    }
}


static void _blob_model_event_cb(mesh_model_event_en event, void *p_arg)
{
    int ret = 0;
    static uint8_t erase_all_misc_flag = 1;
    static uint8_t flash_write_flag    = 0;

    switch (event) {
        case BT_MESH_MODEL_BLOB_TRANS_START: {
            if (p_arg) {
                model_message message = *(model_message *)p_arg;
                blob_srv *blob_srv_ctx = message.user_data;

                if (blob_srv_ctx->blob_trans_phase == TRANSFER_PHASE_WAITING_FOR_NEXT_BLOCK) {
                    g_mesh_ota_client_ctx.blob_info        =      &blob_srv_ctx->blob_srv_info;
                    g_mesh_ota_client_ctx.src_addr         = message.source_addr;

                    if (flash_write_flag) {
                        erase_dfu_flash(erase_all_misc_flag);
                    }
                } else {
                    LOGE(TAG, "Invalid blob trans status %02x for trans start", blob_srv_ctx->blob_trans_phase);
                }
            }
        }
        break;

        case BT_MESH_MODEL_BLOB_TRANS_CANCEL: {
            if (p_arg) {
                LOGD(TAG, "Trans cancel");
            }
        }
        break;

        case BT_MESH_MODEL_BLOB_BLOCK_START: {
            if (p_arg) {
                model_message message = *(model_message *)p_arg;
                blob_srv *blob_srv_ctx = message.user_data;

                if (blob_srv_ctx->blob_trans_phase == TRANSFER_PHASE_WAITING_FOR_NEXT_CHUNK) {
                    g_mesh_ota_client_ctx.block_info        =      &blob_srv_ctx->blob_srv_block_info;
                } else {
                    LOGE(TAG, "Invalid blob trans status %02x for block start", blob_srv_ctx->blob_trans_phase);
                }
            }
        }
        break;

        case BT_MESH_MODEL_BLOB_CHUNK_DATA: {
            if (p_arg) {
                model_message message = *(model_message *)p_arg;
                blob_srv *blob_srv_ctx = message.user_data;
                struct net_buf_simple_state state;
                net_buf_simple_save(message.status_data, &state);
                uint16_t chunk_num  =  net_buf_simple_pull_le16(message.status_data);
                uint32_t offset = g_mesh_ota_client_ctx.block_info->block_num * g_mesh_ota_client_ctx.blob_info->block_total_size + chunk_num * g_mesh_ota_client_ctx.block_info->chunk_size;
                //LOGD(TAG,"Write data %d:%d offset:%d size:%d %d %d",g_mesh_ota_client_ctx.block_info->block_num,chunk_num,offset,message.status_data->len,g_mesh_ota_client_ctx.blob_info->block_total_size, g_mesh_ota_client_ctx.block_info->chunk_size);
                ret = dfu_image_update_no_crc(0, offset, message.status_data->len, message.status_data->data);

                if (ret < 0) {
                    LOGE(TAG, "dfu update faild %d", ret);
                } else  {
                    if (!offset && erase_all_misc_flag) {
                        erase_all_misc_flag = 0;
                        aos_kv_set(ERASE_MISC_FLAG, &erase_all_misc_flag, 1, 0);
                    }

                    if (!flash_write_flag) {
                        flash_write_flag = 1;
                    }
                }

                net_buf_simple_restore(message.status_data, &state);

                if (blob_srv_ctx->blob_trans_phase == TRANSFER_PHASE_COMPLETE) { //TODO

                    //k_delayed_work_submit(&g_mesh_ota_client_ctx.store_work, 2);
                    uint32_t start_address = dfu_get_start_address();
                    ret = dfu_image_get_crc(start_address, g_mesh_ota_client_ctx.blob_info->blob_size, &g_mesh_ota_client_ctx.crc, IMAGE_POS_RAM);

                    if (ret || g_mesh_ota_client_ctx.crc != g_mesh_ota_client_ctx.blob_info->blob_id) {
                        LOGE(TAG, "calu image crc err %d,get crc %04x but the actual crc is %04x", ret, g_mesh_ota_client_ctx.crc, g_mesh_ota_client_ctx.blob_info->blob_id);
                        erase_dfu_flash(erase_all_misc_flag);
                    } else {
                        LOGD(TAG, "OTA Image Check SUCCESS");
                        g_mesh_ota_client_ctx.status  = TRANSFER_PHASE_COMPLETE;
                        ota_timer_stop(&g_mesh_ota_client_ctx.ota_client_timer);
                        ota_timer_start(&g_mesh_ota_client_ctx.ota_client_timer, 5);//5ms
                    }
                }
            }
        }
        break;

        case BT_MESH_MODEL_BLOB_BLOCK_GET: {
            if (p_arg) {
                if (g_mesh_ota_client_ctx.status == TRANSFER_PHASE_COMPLETE) {
                    // LOGD(TAG,"OTA Complete,device will reboot in %d ms",CONFIG_MESH_OTA_REBOOT_DELAY);
                    // ota_timer_stop(&g_mesh_ota_client_ctx.ota_client_timer);
                    // ota_timer_start(&g_mesh_ota_client_ctx.ota_client_timer, CONFIG_MESH_OTA_REBOOT_DELAY);
                }
            }
        }
        break;

        case BT_MESH_MODEL_VENDOR_MESH_CONFIRM: {
            if (p_arg) {
                model_message message = *(model_message *)p_arg;
                _vendor_confirm_message_process(message);
            }
        }
        break;

        default:
            break;
    }
}


struct model_cb cb = {
    .event_cb = _blob_model_event_cb,
};




void image_store_handler_t(struct k_work *work)
{
    chunk_data_head head = {0x00};
    uint8_t *data  = NULL;
    uint32_t offset = 0;
    int ret = 0;

    data =  _blob_buffer_simple_pull(&g_mesh_ota_client_ctx.save_buffer, &head);

    while (data) {
        offset = head.block_num * g_mesh_ota_client_ctx.blob_info->block_total_size + head.chunk_num * g_mesh_ota_client_ctx.block_info->chunk_size;
        ret = dfu_image_update_no_crc(0, offset, head.data_len, data);

        if (ret < 0) {
            LOGE(TAG, "dfu update faild %d", ret);
        }

        data = _blob_buffer_simple_pull(&g_mesh_ota_client_ctx.save_buffer, &head);
    }

    k_sem_give(&g_mesh_ota_client_ctx.store_sem); //TODO

    if (g_mesh_ota_client_ctx.status == TRANSFER_PHASE_COMPLETE) {
        uint32_t start_address = dfu_get_start_address();
        uint16_t crc = 0;
        ret = dfu_image_get_crc(start_address, g_mesh_ota_client_ctx.blob_info->blob_size, &crc, IMAGE_POS_RAM);

        if (ret) {
            LOGE(TAG, "calu image crc err %d", ret);
        } else {
            if (crc == g_mesh_ota_client_ctx.blob_info->blob_id) {
                LOGD(TAG, "OTA SUCCESS");
            } else {
                LOGD(TAG, "OTA check faild image crc %04x calu crc %04x", g_mesh_ota_client_ctx.blob_info->blob_id, crc);
            }
        }
    }

}



static void _mesh_ota_client_timeout(void *timer, void *arg)
{
    int ret = 0;
    g_mesh_ota_client_ctx.report_try++;
    ota_timer_stop(&g_mesh_ota_client_ctx.ota_client_timer);
    ret = _blob_report_crc(0, 0, g_mesh_ota_client_ctx.src_addr, g_mesh_ota_client_ctx.crc);

    if (ret) {
        LOGE(TAG, "Blob report crc faild %d", ret);
    }

    if (g_mesh_ota_client_ctx.report_try >= CONFIG_MESH_OTA_REPORT_CRC_RETRY) {
        LOGE(TAG, "Report crc faild,after %d try, Dfu reboot now", g_mesh_ota_client_ctx.report_try);
        dfu_reboot();
    } else {
        ota_timer_start(&g_mesh_ota_client_ctx.ota_client_timer, CONFIG_MESH_OTA_REPORT_CRC_DELAY);
    }
}



int mesh_ota_client_init()
{
    int ret  = 0;
    uint8_t  erase_all_misc_flag = 0;
    int  flag_len = 1;
    memset(&g_mesh_ota_client_ctx, 0x00, sizeof(mesh_ota_client_ctx));

    ret = blob_srv_trans_init(0, TRANSFER_MODE_PUSH, UNKNOW_EXCEPTED_BLOB_ID); ///config

    if (ret) {
        LOGE(TAG, "Mesh ota client init faild %d", ret);
        return ret;
    }

    _blob_buffer_reset(&g_mesh_ota_client_ctx.save_buffer, recv_buffer, CONFIG_MESH_OTA_CLIENT_BUFFER_SIZE);

    k_delayed_work_init(&g_mesh_ota_client_ctx.store_work, image_store_handler_t);

    ret = k_sem_init(&g_mesh_ota_client_ctx.store_sem, 0, 1);

    if (ret) {
        LOGE(TAG, "store sem init faild %d", ret);
        return ret;
    }

    ota_timer_init(&g_mesh_ota_client_ctx.ota_client_timer, _mesh_ota_client_timeout, NULL);
    ble_mesh_model_cb_register(&cb);

    aos_kv_get(ERASE_MISC_FLAG, &erase_all_misc_flag, &flag_len);
    erase_dfu_flash(erase_all_misc_flag);
    g_mesh_ota_client_ctx.init_flag = 1;

    return 0;
}

#endif

