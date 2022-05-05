/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <aos/kernel.h>
#include <app_init.h>
#include <api/mesh.h>
#include <at_mesh.h>
#include <board_config.h>
#include "mesh_model/mesh_model.h"
#include "mesh_node.h"
#include "mesh_provisioner.h"
#include "provisioner_main.h"
#include "app_mesh.h"
#include "gateway/mesh.h"
#include "linkkit_gateway/gateway_ut.h"
#include "common/log.h"
#include "app_main.h"

#define TAG "APP_MESH"

#define BT_MESH_ADDR_IS_UNICAST(addr) ((addr) && (addr) < 0x8000)
#define BIT_MASK(n) (BIT(n) - 1)

static int print_comp_data(uint16_t addr, struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(comp, 50);

    uint16_t copy_length;


    copy_length = net_buf_simple_tailroom(&comp) < buf->len ? net_buf_simple_tailroom(&comp) :  buf->len;
    net_buf_simple_add_mem(&comp, buf->data, copy_length);

    //addr CID PID VID CRPL Features
    LOGD(TAG,"comp data:%04x,%04x,%04x,%04x,%04x,%04x\r\n", addr, net_buf_simple_pull_le16(&comp), net_buf_simple_pull_le16(&comp), \
         net_buf_simple_pull_le16(&comp), net_buf_simple_pull_le16(&comp), net_buf_simple_pull_le16(&comp));

    while (comp.len > 4) {
        uint8_t sig_model_num;
        uint8_t vnd_model_num;
        uint16_t elem_idx;
        int i;
        elem_idx = net_buf_simple_pull_le16(&comp);
        sig_model_num = net_buf_simple_pull_u8(&comp);
        vnd_model_num = net_buf_simple_pull_u8(&comp);

        LOGD("",",%04x,%02x,%02x", elem_idx, sig_model_num, vnd_model_num);

        if (comp.len < ((sig_model_num * 2) + (vnd_model_num * 4))) {
            LOGD("","\r\n");
            break;
        }

        for (i = 0; i < sig_model_num; i++) {
            u16_t mod_id = net_buf_simple_pull_le16(&comp);
            LOGD("",",%04x", mod_id);
        }


        for (i = 0; i < vnd_model_num; i++) {
            u16_t cid = net_buf_simple_pull_le16(&comp);
            u16_t mod_id = net_buf_simple_pull_le16(&comp);
            LOGD("",",%04x,%04x", cid, mod_id);
        }

    }

    return 0;
}

static void _app_vendor_status_message_process(model_message message)
{
    if(message.ven_data.data_len < 2) {
        return;
    }
    uint8_t *data = (uint8_t*)message.ven_data.user_data;
    uint16_t  status_op = data[0] | data[1] << 8;

    switch(status_op) {
    case ATTR_TYPE_REPORT_VERSION : {
		uint32_t  version   = (data[2]  & 0x00)| data[3] << 16 | data[4] << 8 | data[5];
        LOGD(TAG,"NODE:%04x,Version:%06x\r\n", message.source_addr, version);
    }
    break;
    case ATTR_TYPE_OVERWRITE_SUBLIST : {
		uint8_t status = data[2];
		LOGD(TAG,"NODE:%04x,Sublist overwrite status %02x\r\n", message.source_addr, status);
	}
    default:
        break;
    }

}

void app_model_cb(mesh_model_event_en event, void *p_arg)
{
    int16_t ret = 0;
    switch (event) {
    case BT_MESH_MODEL_CFG_APPKEY_STATUS: {
        if (p_arg) {
            model_message message = *(model_message *)p_arg;
            uint8_t status = message.status_data->data[0];

            if (status) {
                LOGD(TAG,"appkey add:%04x,%x", message.source_addr, status);
            }
        }
    }
    break;

    case BT_MESH_MODEL_CFG_APPKEY_BIND_STATUS: {
        if (p_arg) {
            model_message message = *(model_message *)p_arg;
            uint8_t status = message.status_data->data[0];

            if (status) {
                LOGD(TAG,"appkey bind:%04x,%x", message.source_addr, status);
            }
        }
    }
    break;

    case BT_MESH_MODEL_CFG_COMP_DATA_STATUS: {
        if (p_arg) {
            model_message message = *(model_message *)p_arg;

            struct net_buf_simple *buf = message.status_data;

            if (buf) {
                uint8_t status = net_buf_simple_pull_u8(buf);

                if (!status) {
                    ret = print_comp_data(message.source_addr, buf);
                }

                if (ret || status) {
                    LOGD(TAG,"get comp data faild:%04x,%x", message.source_addr, status);
                }

            }
        }

    }
    break;

    case BT_MESH_MODEL_CFG_SUB_STATUS: {
        if (p_arg) {
            model_message message = *(model_message *)p_arg;
            uint8_t status = message.status_data->data[0];

            if (status) {
                LOGD(TAG,"set sub:%04x,%x", message.source_addr, status);
            }
        }

    }
    break;

    case BT_MESH_MODEL_CFG_SUB_LIST: {
        if (p_arg) {
            model_message message = *(model_message *)p_arg;
            uint8_t status_data = (uint8_t)net_buf_simple_pull_u8(message.status_data);
            uint16_t addr = (uint16_t)net_buf_simple_pull_le16(message.status_data);
            uint16_t mod_id = (uint16_t)net_buf_simple_pull_le16(message.status_data);

            if (!status_data && message.status_data->len >= 2) {
                uint8_t first_addr = 0;

                while (message.status_data->len >= 2) {
                    if (!first_addr) {
                        LOGD(TAG,"sublist:%04x,%04x,%04x,%04x", addr, CID_NVAL, mod_id, (uint16_t)net_buf_simple_pull_le16(message.status_data));
                        first_addr = 1;
                    } else {
                        LOGD("",",%04x", (uint16_t)net_buf_simple_pull_le16(message.status_data));
                    }
                }

                if (first_addr) {
                    LOGD("","\r\n");
                }

            } else if (status_data) {
                LOGD(TAG,"sublist:%04x,%x", addr, status_data);
            } else if (!status_data && message.status_data->len < 2) {
                LOGD(TAG,"sublist:%04x,%04X,%04x,NULL", addr, CID_NVAL, mod_id);
            }

        }
    }
    break;

    case BT_MESH_MODEL_CFG_SUB_LIST_VND: {
        if (p_arg) {
            model_message message = *(model_message *)p_arg;
            uint8_t status_data = (uint8_t)net_buf_simple_pull_u8(message.status_data);
            uint16_t addr = (uint16_t)net_buf_simple_pull_le16(message.status_data);
            uint16_t company_id = (uint16_t)net_buf_simple_pull_le16(message.status_data);
            uint16_t mod_id = (uint16_t)net_buf_simple_pull_le16(message.status_data);

            if (!status_data && message.status_data->len >= 2) {
                uint8_t first_addr = 0;

                while (message.status_data->len >= 2) {
                    if (!first_addr) {
                        LOGD(TAG,"sublist vnd:%04x,%04x,%04x,%04x", addr, company_id, mod_id, (uint16_t)net_buf_simple_pull_le16(message.status_data));
                        first_addr = 1;
                    } else {
                        LOGD("",",%04x", (uint16_t)net_buf_simple_pull_le16(message.status_data));
                    }
                }

                if (first_addr) {
                    LOGD("","\r\n");
                }

            } else if (status_data) {
                LOGD(TAG,"sublist vnd:%d", status_data);
            } else if (!status_data && message.status_data->len < 2) {
                LOGD(TAG,"sublist vnd:%04x,%04x,%04x,NULL", addr, company_id, mod_id);
            }

        }
    }
    break;

    case BT_MESH_MODEL_CFG_PUB_STATUS: {

        if (p_arg) {
            model_message message = *(model_message *)p_arg;
            uint8_t status = (uint8_t)net_buf_simple_pull_u8(message.status_data);
            // uint16_t elem_addr = (uint16_t)net_buf_simple_pull_le16(message.status_data);
            
            if (status) {
                LOGD(TAG,"setpub:%04x,%x", message.source_addr, status);
            } else {
                LOGD(TAG,"setpub:%04x,%x", message.source_addr, AT_STATUS_OK);
            }

        }


    }
    break;

    //model status
    case BT_MESH_MODEL_CFG_RELAY_STATUS: {
        if (p_arg) {
            model_message relay_message = *(model_message *)p_arg;
            LOGD(TAG,"relay:%04x,%02x", relay_message.source_addr, relay_message.status_data->data[0]);
        }
    }
    break;

    case BT_MESH_MODEL_CFG_PROXY_STATUS: {
        if (p_arg) {
            model_message proxy_message = *(model_message *)p_arg;
            LOGD(TAG,"proxy:%04x,%02x", proxy_message.source_addr, proxy_message.status_data->data[0]);
        }
    }
    break;

    case BT_MESH_MODEL_CFG_FRIEND_STATUS: {
        if (p_arg) {
            model_message friend_message = *(model_message *)p_arg;
            LOGD(TAG,"friend:%04x,%02x", friend_message.source_addr, friend_message.status_data->data[0]);
        }
    }
    break;

    case BT_MESH_MODEL_CFG_RST_STATUS: {
        if (p_arg) {
            model_message rst_message = *(model_message *)p_arg;
            // struct bt_mesh_node_t *node = NULL;

            LOGD(TAG,"rst:%04x", rst_message.source_addr);
        }
    }
    break;

    case BT_MESH_MODEL_ONOFF_STATUS: {
        if (p_arg) {
            model_message onoff_message = *(model_message *)p_arg;
            struct bt_mesh_node_t *node = NULL;

            LOGD(TAG,"onoff:%04x,%02x", onoff_message.source_addr, onoff_message.status_data->data[0]);
            node = bt_mesh_provisioner_get_node_info(onoff_message.source_addr);
            if (!node) {
                return;
            }
            
#if defined(CONFIG_GW_SMARTLIVING_SUPPORT) && CONFIG_GW_SMARTLIVING_SUPPORT
            app_mesh_rpt_onoff(node->addr_val, onoff_message.status_data->data[0]);
#endif
        }
    }
    break;

    case BT_MESH_MODEL_LEVEL_STATUS: {
        if (p_arg) {
            model_message level_message = *(model_message *)p_arg;
            int16_t level = (int16_t)net_buf_simple_pull_le16(level_message.status_data);
            LOGD(TAG,"level:%04x,%04x", level_message.source_addr, level);
        }
    }
    break;

    case BT_MESH_MODEL_LIGHTNESS_STATUS: {
        if (p_arg) {
            model_message lightness_message = *(model_message *)p_arg;
            int16_t lightness = (int16_t)net_buf_simple_pull_le16(lightness_message.status_data);
            LOGD(TAG,"lightness:%04x,%04x", lightness_message.source_addr, lightness);
        }
    }
    break;

    case BT_MESH_MODEL_LIGHTNESS_LINEAR_STATUS: {
        if (p_arg) {
            model_message lightness_lin_message = *(model_message *)p_arg;
            int16_t lightness_lin = (int16_t)net_buf_simple_pull_le16(lightness_lin_message.status_data);
            LOGD(TAG,"lightness lin:%04x,%04x", lightness_lin_message.source_addr, lightness_lin);
        }
    }
    break;

    case BT_MESH_MODEL_LIGHTNESS_RANGE_STATUS: {
        if (p_arg) {
            model_message lightness_range_message = *(model_message *)p_arg;
            uint8_t status_code = (uint8_t)net_buf_simple_pull_u8(lightness_range_message.status_data);
            int16_t range_min = (int16_t)net_buf_simple_pull_le16(lightness_range_message.status_data);
            int16_t range_max = (int16_t)net_buf_simple_pull_le16(lightness_range_message.status_data);
            LOGD(TAG,"lightness range:%04x,%02x,%04x,%04x", lightness_range_message.source_addr, status_code, range_min, range_max);
        }

    }
    break;

    case BT_MESH_MODEL_LIGHTNESS_DEF_STATUS: {
        if (p_arg) {
            model_message lightness_def_message = *(model_message *)p_arg;
            int16_t lightness_def = (int16_t)net_buf_simple_pull_le16(lightness_def_message.status_data);
            LOGD(TAG,"lightness def:%04x,%04x", lightness_def_message.source_addr, lightness_def);
        }
    }
    break;

    case BT_MESH_MODEL_LIGHTNESS_LAST_STATUS: {
        if (p_arg) {
            model_message lightness_last_message = *(model_message *)p_arg;
            int16_t lightness_last = (int16_t)net_buf_simple_pull_le16(lightness_last_message.status_data);
            LOGD(TAG,"lightness last:%04x,%04x", lightness_last_message.source_addr, lightness_last);
        }
    }
    break;

    case BT_MESH_MODEL_LIGHT_CTL_STATUS: {
        if (p_arg) {
            model_message light_ctl_message = *(model_message *)p_arg;
            int16_t lightness = (int16_t)net_buf_simple_pull_le16(light_ctl_message.status_data);
            int16_t ctl_temp  = (int16_t)net_buf_simple_pull_le16(light_ctl_message.status_data);
            LOGD(TAG,"mesh ctl:%04x,%04x,%04x", light_ctl_message.source_addr, lightness, ctl_temp);
        }
    }
    break;

    case BT_MESH_MODEL_LIGHT_CTL_TEMP_STATUS: {
        if (p_arg) {
            model_message light_ctl_temp_message = *(model_message *)p_arg;
            int16_t ctl_temp  = (int16_t)net_buf_simple_pull_le16(light_ctl_temp_message.status_data);
            int16_t uv  = (int16_t)net_buf_simple_pull_le16(light_ctl_temp_message.status_data);
            LOGD(TAG,"mesh ctl temp:%04x,%04x,%04x", light_ctl_temp_message.source_addr, ctl_temp, uv);
        }
    }
    break;

    case BT_MESH_MODEL_LIGHT_CTL_TEMP_RANGE_STATUS: {
        if (p_arg) {
            model_message light_ctl_temp_ran_message = *(model_message *)p_arg;
            uint8_t status_code = (uint8_t)net_buf_simple_pull_u8(light_ctl_temp_ran_message.status_data);
            int16_t temp_min  = (int16_t)net_buf_simple_pull_le16(light_ctl_temp_ran_message.status_data);
            int16_t temp_max  = (int16_t)net_buf_simple_pull_le16(light_ctl_temp_ran_message.status_data);
            LOGD(TAG,"mesh ctl temp range:%04x,%02x,%04x,%04x", light_ctl_temp_ran_message.source_addr, status_code, temp_min, temp_max);
        }
    }
    break;

    case BT_MESH_MODEL_LIGHT_CTL_DEF_STATUS: {
        if (p_arg) {
            model_message light_ctl_def_message = *(model_message *)p_arg;

            int16_t lightness = (int16_t)net_buf_simple_pull_le16(light_ctl_def_message.status_data);
            int16_t ctl_temp  = (int16_t)net_buf_simple_pull_le16(light_ctl_def_message.status_data);
            int16_t uv  = (int16_t)net_buf_simple_pull_le16(light_ctl_def_message.status_data);
            LOGD(TAG,"ctl def:%04x,%04x,%04x,%04x", light_ctl_def_message.source_addr, lightness, ctl_temp, uv);
        }
    }
    break;

    case BT_MESH_MODEL_VENDOR_MESSAGES: {
        model_message message = *(model_message *)p_arg;
        uint16_t unicast_addr = message.source_addr;
        uint8_t len = message.ven_data.data_len;
        char *data = (char *)message.ven_data.user_data;
        LOGD(TAG,"mesh trs:%04x,%02x,%s", unicast_addr, len, bt_hex(data, len));
    }
    break;

    case BT_MESH_MODEL_VENDOR_MESH_STATUS: {
        model_message message = *(model_message *)p_arg;
        _app_vendor_status_message_process(message);
    }
    break;

    case BT_MESH_MODEL_ONOFF_SET:
    case BT_MESH_MODEL_ONOFF_SET_UNACK: {
        model_message message = *(model_message *)p_arg;
        S_ELEM_STATE *elem_state = (S_ELEM_STATE *)message.user_data;
        LOGD(TAG,"onoff set:%02x\r\n",elem_state->state.onoff[T_TAR]);
        // app_pwm_led_control(elem_state->state.onoff[T_TAR]);
    }
    break;

    default:
        break;
    }
}

int app_mesh_rpt_onoff(uint8_t *mac, uint8_t onoff)
{
    // dev_addr_t dev_addr;
    // struct bt_mesh_node_t *node;
    uint8_t mac_conv[6];

    gateway_mesh_mac_convert(mac, mac_conv);

    gateway_subdev_rpt_onoff((char*)mac_conv, onoff);

	return 0;
}

int app_mesh_init()
{
    static uint8_t init_flag = 0;
    int err = 0;

    gateway_mesh_config_t config = {
        .dev_name = DEV_NAME,
        .user_model_cb = app_model_cb,
    };

    if (init_flag == 1) {
        LOGD(TAG, "mesh already inited");
        return 0;
    }

    extern const struct bt_mesh_comp *app_mesh_composition_init();
    config.comp = app_mesh_composition_init();

    err = gateway_mesh_init(&config);
    if(err) {
        LOGE(TAG,"gateway_mesh_init %d\r\n",err);
        return err;
    }

    gateway_btmesh_prov_config(0x01, 0xFF);
    gateway_btmesh_prov_autoconfig();
    gateway_btmesh_prov_enable(1);

    init_flag = 1;
    return err;
}
