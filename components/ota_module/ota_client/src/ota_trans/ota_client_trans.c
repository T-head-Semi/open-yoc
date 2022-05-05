#include "ota_trans/ota_client_trans.h"
#include "ota_module.h"
#include "errno.h"
#include "ulog/ulog.h"
#include "stdint.h"

#if defined(CONFIG_OTA_CLIENT_TRANS_GATT)  && CONFIG_OTA_CLIENT_TRANS_GATT
#include "ota_trans/ota_client_trans_gatt.h"
#endif
#if defined(CONFIG_OTA_CLIENT_TRANS_UART)  && CONFIG_OTA_CLIENT_TRANS_UART
#include "ota_trans/ota_client_trans_uart.h"
#endif
#if defined(CONFIG_OTA_CLIENT_TRANS_HCI_UART)  && CONFIG_OTA_CLIENT_TRANS_HCI_UART
#include "ota_trans/ota_client_trans_hci_uart.h"
#endif


#define TAG "OTA_CLIENT_TRANS"

int  ota_client_trans_init(ota_client_event_cb cb)
{
    uint8_t channel_flag = 0;
    uint8_t channel  = 0;
    int ret = 0;
    if(!cb) {
        return -EINVAL;
    }

#if defined(CONFIG_OTA_CLIENT_TRANS_GATT)  && CONFIG_OTA_CLIENT_TRANS_GATT
    ret = ota_client_trans_gatt_init(cb);
    if (ret && ret != -EALREADY) {
        LOGE(TAG, "init ota trans gatt faild\r\n");
        return ret;
    }
    channel_flag ++;
    channel = OTA_BY_GATT;
#endif

#if defined(CONFIG_OTA_CLIENT_TRANS_UART)  && CONFIG_OTA_CLIENT_TRANS_UART
    ret = ota_client_trans_uart_init(cb);
    if (ret && ret != -EALREADY) {
        LOGE(TAG, "init ota trans uart faild\r\n");
        return ret;
    }
    channel_flag ++;
    channel = OTA_BY_UART;
#endif


#if defined(CONFIG_OTA_CLIENT_TRANS_HCI_UART)  && CONFIG_OTA_CLIENT_TRANS_HCI_UART
    ret = ota_client_trans_hci_uart_init(cb);
    if (ret && ret != -EALREADY) {
        LOGE(TAG, "init ota trans HCI uart faild\r\n");
        return ret;
    }
    channel_flag ++;
    channel = OTA_BY_HCI_UART;
#endif

    if(!channel_flag) {
        LOGE(TAG, "No tran channel select");
        return -EINVAL;
    } else if(channel_flag > 1) {
        LOGE(TAG, "Multi tran channel not support");
        return -EINVAL;
    }

    (void)ret;

    return channel;
}


int ota_client_trans_send(uint8_t channel, uint8_t msg_id, uint8_t cmd, uint8_t *p_msg, uint16_t len, uint8_t ack)
{
    if(channel == OTA_BY_GATT) {
#if defined(CONFIG_OTA_CLIENT_TRANS_GATT)  && CONFIG_OTA_CLIENT_TRANS_GATT
        return ota_client_trans_gatt_send(msg_id,cmd,p_msg,len,ack);
#else
        return -EOPNOTSUPP;
#endif

    } else if (channel == OTA_BY_UART) {
#if defined(CONFIG_OTA_CLIENT_TRANS_UART)  && CONFIG_OTA_CLIENT_TRANS_UART
        return ota_client_trans_uart_send(msg_id,cmd,p_msg,len,ack);
#else
        return -EOPNOTSUPP;
#endif

    } else if(channel == OTA_BY_HCI_UART) {
#if defined(CONFIG_OTA_CLIENT_TRANS_HCI_UART)  && CONFIG_OTA_CLIENT_TRANS_HCI_UART
        return ota_client_trans_hci_uart_send(msg_id,cmd,p_msg,len,ack);
#else
        return -EOPNOTSUPP;
#endif
    }

    return -EINVAL;
}

int ota_client_trans_disconnect(uint8_t channel)
{
    if(channel == OTA_BY_GATT) {
#if defined(CONFIG_OTA_CLIENT_TRANS_GATT)  && CONFIG_OTA_CLIENT_TRANS_GATT
        return ota_client_trans_gatt_disconnect();
#else
        return -EOPNOTSUPP;
#endif

    } else if(channel == OTA_BY_UART) {
#if defined(CONFIG_OTA_CLIENT_TRANS_UART)  && CONFIG_OTA_CLIENT_TRANS_UART
        return ota_client_trans_uart_disconnect();
#else
        return -EOPNOTSUPP;
#endif
    } else if(channel == OTA_BY_HCI_UART) {
#if defined(CONFIG_OTA_CLIENT_TRANS_HCI_UART)  && CONFIG_OTA_CLIENT_TRANS_HCI_UART
        return ota_client_trans_hci_uart_disconnect();
#else
        return -EOPNOTSUPP;
#endif
    }
    return -EINVAL;
}



