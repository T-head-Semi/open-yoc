/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include "at_ble.h"
#include <stdio.h>
#include <aos/kv.h>

uint8_t flag1 = AD_FLAG_GENERAL | AD_FLAG_NO_BREDR;

ad_data_t ad_def[2] = {
    {.type = AD_DATA_TYPE_FLAGS, .data = &flag1, .len = 1},
    {.type = AD_DATA_TYPE_UUID128_ALL, .data  = UUID128(YOC_UART_SERVICE_UUID), .len = 16},
};


uint8_t manu_data[6] = {0xA8, 0x01, 0x00, 0x01, 0x02, 0x03};

ad_data_t sd_def[2] = {
    {.type = AD_DATA_TYPE_NAME_COMPLETE, .data = (uint8_t *)SLAVE_DEVICE_NAME_DEF, .len = 0},
    {.type = AD_DATA_TYPE_MANUFACTURER_DATA, .data = manu_data, .len = sizeof(manu_data)},
};

static tx_power tx_power_table[13] = {{-30, 0X00}, {-20, 0X01}, {-15, 0x02}, {-10, 0x03}, {-6, 0x04}, {-5, 0x05}, \
    {-3, 0x06}, {0, 0x0a}, {3, 0x0f}, {4, 0x12}, {5, 0x17}, {6, 0x1F}, {10, 0x3F}
};

extern uint8_t length_temp;


typedef struct _ad_data {
    uint8_t type;
    uint8_t len;
    uint8_t data[31];
} ad_data;


extern int ad_data_covert(uint8_t *data, int16_t len, ad_data_t *ad_data);
static int check_tx_power_set_value(int set_value)
{
    for (int i = 0; i < sizeof(tx_power_table) / sizeof(tx_power_table[0]); i++) {

        if (set_value == tx_power_table[i].set_value) {
            return 0;
        }
    }

    return -1;
}

int at_ble_conf_load(at_conf_load *conf_load)
{
    if (conf_load == NULL) {
        return -1;
    }

    int ret = 0;
    int length = 1;
    uint8_t key_name[40];
    uint8_t num_flag;

    //role
    ret = aos_kv_get("BT_ROLE", &conf_load->role, &length);

    if (ret != 0 || (conf_load->role != MASTER && conf_load->role != SLAVE)) {
        conf_load->role = DEF_ROLE;
    }

    //name
    ret = aos_kv_getstring("BT_NAME", (char *)conf_load->bt_name, CONFIG_BT_DEVICE_NAME_MAX + 1);

    if (strlen((char *)conf_load->bt_name) == 0) {
        if (conf_load->role == SLAVE) {
            sprintf((char *)conf_load->bt_name, SLAVE_DEVICE_NAME_DEF);
        } else {
            sprintf((char *)conf_load->bt_name, MASTER_DEVICE_NAME_DEF);
        }
    }

    //mac
    ret = aos_kv_get("BT_ADDR_TYPE", &conf_load->addr.type, &length);

    if (conf_load->addr.type == DEV_ADDR_LE_RANDOM) {
        length = 6;
        ret = aos_kv_get("BT_MAC", conf_load->addr.val, &length);

        if (ret != 0 || length != 6) {
            conf_load->addr.type = 0;
        }
    }

    // TX POWER
    length = 1;
    ret = aos_kv_get("BT_TXPOWER", &conf_load->tx_power, &length);

    if (ret != 0 || check_tx_power_set_value(conf_load->tx_power)) {
        conf_load->tx_power = BT_TX_POWER_DEF;
    }

    // SLEEP MODE
    ret = aos_kv_get("BT_SLEEP_MODE", &conf_load->sleep_mode, &length);

    if (ret != 0 || (conf_load->sleep_mode != NO_SLEEP && conf_load->sleep_mode != SLEEP && conf_load->sleep_mode != STANDBY)) {
        conf_load->sleep_mode = DEF_LPM_MODE;
    }

    //BAUD
    ret = aos_kv_getint("BT_BAUD", &conf_load->baud);

    if (ret != 0 || (conf_load->baud != 9600 && conf_load->baud != 115200 && conf_load->baud != 250000 && conf_load->baud != 512000 && conf_load->baud != 600000)) {
        conf_load->baud = BT_TX_BAUD_DEF;
    }

    //CONN_UPDATE_FLAG
    ret = aos_kv_get("CONN_UPDATE_ON", &conf_load->uart_conf.slave_conf.conn_update_def_on, &length);

    if (ret != 0) {
        conf_load->uart_conf.slave_conf.conn_update_def_on = CONN_UPDATE_DEF_ON_OFF;
    }

    //CONN PRAM
    if (conf_load->uart_conf.slave_conf.conn_update_def_on) {
        length = sizeof(conn_param_t);
        ret  = aos_kv_get("CONN_PARAM", &conf_load->uart_conf.slave_conf.conn_param, &length);

        if (ret != 0) {
            conf_load->uart_conf.slave_conf.conn_update_def_on = CONN_UPDATE_DEF_ON_OFF;
        }
    }

    if (conf_load->role == SLAVE) {
        length = 1 ;

        //ADV DEF
        ret = aos_kv_get("BT_ADV_DEF", &conf_load->uart_conf.slave_conf.adv_def_on, &length);

        if (ret != 0 || (conf_load->uart_conf.slave_conf.adv_def_on != ADV_ON && conf_load->uart_conf.slave_conf.adv_def_on != ADV_OFF)) {
            conf_load->uart_conf.slave_conf.adv_def_on = ADV_DEF_ON_OFF;
        }

        //ADV type
        ret = aos_kv_get("BT_ADV_TYPE", &conf_load->uart_conf.slave_conf.param.type, &length);

        if (ret != 0 || (conf_load->uart_conf.slave_conf.param.type != ADV_NONCONN_IND && \
                         conf_load->uart_conf.slave_conf.param.type != ADV_IND && \
                         conf_load->uart_conf.slave_conf.param.type != ADV_DIRECT_IND)) {
            conf_load->uart_conf.slave_conf.param.type = AD_DEF_TYPE;
        }

        length = sizeof(conf_load->uart_conf.slave_conf.param.interval_min);

        //ADV INT MIN
        ret = aos_kv_get("BT_ADV_INT_MIN", &conf_load->uart_conf.slave_conf.param.interval_min, &length);

        if (ret != 0) {
            conf_load->uart_conf.slave_conf.param.interval_min = AD_INT_MIN_DEF;
        }

        length = sizeof(conf_load->uart_conf.slave_conf.param.interval_max);
        //ADV INT MAX
        ret = aos_kv_get("BT_ADV_INT_MAX", &conf_load->uart_conf.slave_conf.param.interval_max, &length);

        if (ret != 0) {
            conf_load->uart_conf.slave_conf.param.interval_max = AD_INT_MAX_DEF;
        }

        // ADV AD
        length = 1;

        if (conf_load->addr.type == DEV_ADDR_LE_RANDOM) {
            memcpy(&ad_def[1].data[10], conf_load->addr.val, 6);
        } else {
            dev_addr_t addr;
            extern int read_public_addr(dev_addr_t *addr);
            read_public_addr(&addr);
            memcpy(&ad_def[1].data[10], addr.val, 6);
        }


        ret = aos_kv_get("BT_ADV_AD_LEN", &conf_load->uart_conf.slave_conf.ad.ad_len, &length);

        uint8_t set_ad_faild = 0;

        if (ret != 0 || conf_load->uart_conf.slave_conf.ad.ad_len == 0) {
            conf_load->uart_conf.slave_conf.param.ad = ad_def;
            conf_load->uart_conf.slave_conf.param.ad_num = sizeof(ad_def) / sizeof(ad_def[0]);
        } else {
            length = conf_load->uart_conf.slave_conf.ad.ad_len;
            ret = aos_kv_get("BT_ADV_AD", conf_load->uart_conf.slave_conf.ad.ad, &length);
            conf_load->uart_conf.slave_conf.param.ad_num = ad_data_covert(conf_load->uart_conf.slave_conf.ad.ad, length, NULL);

            if (ret != 0 || conf_load->uart_conf.slave_conf.ad.ad_len != length || conf_load->uart_conf.slave_conf.param.ad_num == 0) {
                set_ad_faild = 1;
            } else {
                conf_load->uart_conf.slave_conf.param.ad = (ad_data_t *)malloc(sizeof(ad_data_t) * conf_load->uart_conf.slave_conf.param.ad_num);

                if (!conf_load->uart_conf.slave_conf.param.ad) {
                    set_ad_faild = 1;
                } else {
                    ad_data_covert(conf_load->uart_conf.slave_conf.ad.ad, length, conf_load->uart_conf.slave_conf.param.ad);
                }
            }

        }

        if (set_ad_faild) {
            conf_load->uart_conf.slave_conf.param.ad = ad_def;
            conf_load->uart_conf.slave_conf.param.ad_num = sizeof(ad_def) / sizeof(ad_def[0]);
        }


        //ADV SD
        sd_def[0].type = AD_DATA_TYPE_NAME_COMPLETE;
        sd_def[0].data = conf_load->bt_name;
        sd_def[0].len = strlen((char *)sd_def[0].data);
        set_ad_faild = 0;
        length = 1;
        ret = aos_kv_get("BT_ADV_SD_LEN", &conf_load->uart_conf.slave_conf.sd.ad_len, &length);

        if (ret != 0 || conf_load->uart_conf.slave_conf.sd.ad_len == 0) {

            conf_load->uart_conf.slave_conf.param.sd = sd_def;
            conf_load->uart_conf.slave_conf.param.sd_num = sizeof(sd_def) / sizeof(sd_def[0]);
        } else {
            length = conf_load->uart_conf.slave_conf.sd.ad_len;
            ret = aos_kv_get("BT_ADV_SD", conf_load->uart_conf.slave_conf.sd.ad, &length);
            conf_load->uart_conf.slave_conf.param.sd_num = ad_data_covert(conf_load->uart_conf.slave_conf.sd.ad, length, NULL);

            if (ret != 0 || conf_load->uart_conf.slave_conf.sd.ad_len != length || conf_load->uart_conf.slave_conf.param.sd_num == 0) {
                set_ad_faild = 1;
            } else {
                conf_load->uart_conf.slave_conf.param.sd = (ad_data_t *)malloc(sizeof(ad_data_t) * conf_load->uart_conf.slave_conf.param.sd_num);

                if (!conf_load->uart_conf.slave_conf.param.sd) {
                    set_ad_faild = 1;
                } else {
                    ad_data_covert(conf_load->uart_conf.slave_conf.sd.ad, length, conf_load->uart_conf.slave_conf.param.sd);
                }
            }
        }

        if (set_ad_faild) {
            conf_load->uart_conf.slave_conf.param.ad = sd_def;
            conf_load->uart_conf.slave_conf.param.ad_num = sizeof(sd_def) / sizeof(sd_def[0]);
        }

    } else {
        master_load *master_conf = &conf_load->uart_conf.master_conf;
        length = 1;
        ret = aos_kv_get("BT_CONNAUTO", &master_conf->conn_def_on, &length);

        if (ret != 0  || (master_conf->conn_def_on != CONN_AUTO_ON  && master_conf->conn_def_on != CONN_AUTO_OFF)) {
            master_conf->conn_def_on = CONN_AUTO_CONF;
        }

        ret = aos_kv_get("BT_CONN_NUM", &num_flag, &length);

        if (ret < 0) {
            return 0;
        }

        length_temp = num_flag;

        if (ret == 0 && num_flag > 0) {
            master_conf->auto_conn_num = num_flag < AUTO_CONN_BT_MAX ? num_flag : AUTO_CONN_BT_MAX;

            for (int i = 0 ; i < master_conf->auto_conn_num ; i++) {
                snprintf((char *)key_name, sizeof(key_name), "BT_CONN_%d", i);
                length = sizeof(dev_addr_t);
                ret = aos_kv_get((char *)key_name, &master_conf->auto_conn_info[i], &length);

                if (ret) {
                    return 0;
                }
            }
        }

    }

    return 0;
}



int at_ble_conf_erase()
{
    int ret;
    int length = 1;
    uint8_t conn_num_flag;
    uint8_t key_name[30];
    aos_kv_del("BT_ROLE");
    aos_kv_del("BT_NAME");
    aos_kv_del("BT_ADDR_TYPE");
    aos_kv_del("BT_MAC");
    aos_kv_del("BT_TXPOWER");
    aos_kv_del("BT_SLEEP_MODE");
    aos_kv_del("BT_BAUD");
    aos_kv_del("CONN_UPDATE_ON");
    aos_kv_del("CONN_PARAM");
    aos_kv_del("BT_ADV_DEF");
    aos_kv_del("BT_ADV_TYPE");
    aos_kv_del("BT_ADV_INT_MIN");
    aos_kv_del("BT_ADV_INT_MAX");
    aos_kv_del("BT_ADV_AD_LEN");
    aos_kv_del("BT_ADV_AD");
    aos_kv_del("BT_ADV_SD_LEN");
    aos_kv_del("BT_ADV_SD");
    aos_kv_del("BT_CONNAUTO");

    ret = aos_kv_get("BT_CONN_NUM", &conn_num_flag, &length);

    if (ret < 0 || conn_num_flag == 0) {
        return 0;
    } else {
        conn_num_flag = conn_num_flag < AUTO_CONN_BT_MAX ? conn_num_flag : AUTO_CONN_BT_MAX;

        for (int i = 0 ; i < conn_num_flag ; i++) {
            snprintf((char *)key_name, sizeof(key_name), "BT_CONN_%d", i);
            length = sizeof(dev_addr_t);
            aos_kv_del((char *)key_name);
        }
    }

    return 0;
}




