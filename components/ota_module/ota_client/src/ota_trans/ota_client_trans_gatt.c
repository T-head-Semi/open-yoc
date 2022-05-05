/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_OTA_CLIENT_TRANS_GATT)  && CONFIG_OTA_CLIENT_TRANS_GATT

#include "aos/kernel.h"
#include "errno.h"
#include "ulog/ulog.h"
#include "yoc/ais_server.h"
#include "ota_trans/ota_client_trans.h"
#include "ota_trans/ota_client_trans_gatt.h"


#define TAG "OTA_TRANS_GATT"


static ota_client_event_cb g_cb = NULL;


void ais_event_cb(uint8_t event,const void *event_data,uint32_t len)
{
  uint8_t trans_event = 0;
  switch(event) {
    case AIS_SERVER_GATT_EVENT_CONN: {
       trans_event = OTA_CLIENT_EVENT_CONN;
	} break;
    case AIS_SERVER_GATT_EVENT_DISCONN: {
       trans_event = OTA_CLIENT_EVENT_DISCONN;
	} break;
     case AIS_SERVER_GATT_EVENT_WRITE: {
       trans_event = OTA_CLIENT_EVENT_WRITE;
	} break;
	default:
	   return;
  }

  if(g_cb) {
     g_cb(trans_event,event_data,len);
  }
}


int ota_client_trans_gatt_init(void         (*cb)(uint8_t , const void* , uint8_t ))
{
    int ret = 0;
    ret = ble_ais_server_init(ais_event_cb);
    if(ret) {
      LOGE(TAG,"ble ais init faild %d",ret);
      return ret;
    }
    g_cb = (ota_client_event_cb)cb;
    return 0;
}


int ota_client_trans_gatt_disconnect()
{  
    return ble_ais_server_disconnect();
}

int ota_client_trans_gatt_send(uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len, uint8_t ack)
{
    if(!ack) {
       ble_ais_server_notify(msg_id, cmd, p_msg, len);
	} else {
       ble_ais_server_indicate(msg_id, cmd, p_msg, len);
	}
    return 0;
}

#endif
