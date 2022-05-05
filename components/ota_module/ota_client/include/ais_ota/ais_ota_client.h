#ifndef _BLE_AIS_CLIENT_H
#define _BLE_AIS_CLIENT_H
#include "ais_ota.h"
#include "aos/kernel.h"
#include "timer_port.h"

#define OTA_CRYPTO_UNIT_SIZE (16)

#ifndef CONFIG_AIS_TOTAL_FRAME
#define CONFIG_AIS_TOTAL_FRAME 16
#endif

#ifndef CONFIG_OTA_PAYLOAD_MAX
#define CONFIG_OTA_PAYLOAD_MAX (CONFIG_BT_L2CAP_RX_MTU - 3)
#endif

#define AIS_DISCONNECT_TIMEOUT 500 //ms
#define OTA_RECV_BUF_SIZE (CONFIG_AIS_TOTAL_FRAME * CONFIG_OTA_PAYLOAD_MAX)
#define OTA_RECV_MAX_ERR_COUNT (CONFIG_AIS_TOTAL_FRAME * 3)
#define BLE_OTA_IMAGE_ID_MAGIC (0xA5A5A5)
#define BLE_OTA_IMAGE_ID_A (BLE_OTA_IMAGE_ID_MAGIC << 8 | DFU_IMAGE_A)
#define BLE_OTA_IMAGE_ID_B (BLE_OTA_IMAGE_ID_MAGIC << 8 | DFU_IMAGE_B)

#define AIS_OTA_AUTH_TIMEOUT 10000    //10s
#define AIS_OTA_DISCONN_TIMEOUT 60000 //60s
#define AIS_OTA_REPORT_TIMEOUT (CONFIG_AIS_TOTAL_FRAME * 400)
#define AIS_OTA_REBOOT_TIMEOUT 800 //800ms



//For pingpong OTA
enum {
    DFU_IMAGE_A = 0,
    DFU_IMAGE_B,
    DFU_IMAGE_TOTAL = DFU_IMAGE_B,
    DFU_IMAGE_INVALID_ID,
};


typedef struct ais_ota_ctx_s {
    uint8_t init_flag;
    uint8_t state;
    uint8_t channel;
    uint8_t err_count;
    uint8_t last_seq;
    uint8_t total_frame;
    uint8_t except_seq;
    uint32_t rx_size;

    uint8_t image_type;
    uint32_t image_ver;
    uint32_t image_size;
    uint16_t image_crc16;
    uint8_t crc_check_flag;
    uint8_t ota_flag;
    uint8_t auto_reboot_flag;

    uint8_t flash_clean : 1;
    uint8_t ota_ready : 1;
    uint16_t rx_len;
    uint8_t recv_buf[OTA_RECV_BUF_SIZE];
    ota_timer_t state_update_timer;
    ota_timer_t disconnect_timer;
} ais_ota_ctx_t;


int ais_ota_client_init();
int ais_ota_report_version(void);
void ais_ota_crc_check_ctl(uint8_t flag);
void ais_ota_auto_reboot_ctl(uint8_t flag);

#endif