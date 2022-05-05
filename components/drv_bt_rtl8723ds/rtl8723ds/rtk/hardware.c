/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Realtek Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      hardware.c
 *
 *  Description:   Contains controller-specific functions, like
 *                      firmware patch download
 *                      low power mode operations
 *
 ******************************************************************************/

#define LOG_TAG "bt_hwcfg"
#define RTKBT_RELEASE_NAME "20171130_BT_ANDROID_7.0"

#include <ulog/ulog.h>
#include <aos/aos.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "bt_vendor_rtk.h"
#include "userial_vendor.h"

#ifdef CONFIG_BT_DRV_NO_ONESHOT
#include "rtl8723d_fw.c"
#else
#include "rtl8723d_fw_oneshot.c"
#endif
#include "rtl8723d_config.c"
#include "rtl8723ds_mp_fw.c"
#include "rtl8723ds_mp_config.c"
#include <devices/hci.h>

#define TAG "hci_vendor"

/******************************************************************************
**  Constants &  Macros
******************************************************************************/
#define RTK_VERSION "4.1.1"


#define cpu_to_le16(d)  (d)
#define cpu_to_le32(d)  (d)
#define le16_to_cpu(d)  (d)
#define le32_to_cpu(d)  (d)

#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}
#define UINT24_TO_STREAM(p, u24) {*(p)++ = (uint8_t)(u24); *(p)++ = (uint8_t)((u24) >> 8); *(p)++ = (uint8_t)((u24) >> 16);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}
#define INT8_TO_STREAM(p, u8)    {*(p)++ = (int8_t)(u8);}

#define STREAM_TO_UINT8(u8, p)   {u8 = (uint8_t)(*(p)); (p) += 1;}
#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define STREAM_TO_UINT24(u32, p) {u32 = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + ((((uint32_t)(*((p) + 2)))) << 16) ); (p) += 3;}
#define STREAM_TO_UINT32(u32, p) {u32 = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + ((((uint32_t)(*((p) + 2)))) << 16) + ((((uint32_t)(*((p) + 3)))) << 24)); (p) += 4;}

#define FIRMWARE_DIRECTORY  "/system/etc/firmware/%s"
#define BT_CONFIG_DIRECTORY "/system/etc/firmware/%s"
#define PATCH_DATA_FIELD_MAX_SIZE       252
#define RTK_VENDOR_CONFIG_MAGIC         0x8723ab55
#define MAX_PATCH_SIZE_24K            (1024*24)   //24K
#define MAX_PATCH_SIZE_40K            (1024*40)   //40K

#define MAX_ORG_CONFIG_SIZE     (0x100*14)
#define MAX_ALT_CONFIG_SIZE     (0x100*2)

struct rtk_bt_vendor_config_entry {
    uint16_t offset;
    uint8_t entry_len;
    uint8_t entry_data[0];
} __attribute__((packed));

struct rtk_bt_vendor_config {
    uint32_t signature;
    uint16_t data_len;
    struct rtk_bt_vendor_config_entry entry[0];
} __attribute__((packed));

#define HCI_CMD_MAX_LEN             258

#define HCI_RESET                               0x0C03
#define HCI_READ_LMP_VERSION                    0x1001
#define HCI_VSC_H5_INIT                         0xFCEE
#define HCI_VSC_UPDATE_BAUDRATE                 0xFC17
#define HCI_VSC_DOWNLOAD_FW_PATCH               0xFC20
#define HCI_VSC_READ_ROM_VERSION                0xFC6D
#define HCI_VSC_READ_CHIP_TYPE                  0xFC61

#define HCI_VERSION_MASK_10     (1<<0)     //Bluetooth Core Spec 1.0b
#define HCI_VERSION_MASK_11     (1<<1)     //Bluetooth Core Spec 1.1
#define HCI_VERSION_MASK_12     (1<<2)     //Bluetooth Core Spec 1.2
#define HCI_VERSION_MASK_20     (1<<3)     //Bluetooth Core Spec 2.0+EDR
#define HCI_VERSION_MASK_21     (1<<4)     //Bluetooth Core Spec 2.1+EDR
#define HCI_VERSION_MASK_30     (1<<5)     //Bluetooth Core Spec 3.0+HS
#define HCI_VERSION_MASK_40     (1<<6)     //Bluetooth Core Spec 4.0
#define HCI_VERSION_MASK_41     (1<<7)     //Bluetooth Core Spec 4.1
#define HCI_VERSION_MASK_42     (1<<8)     //Bluetooth Core Spec 4.2
#define HCI_VERSION_MASK_ALL    (0xFFFFFFFF)

#define HCI_REVISION_MASK_ALL   (0xFFFFFFFF)

#define LMP_SUBVERSION_NONE     (0x0)
#define LMPSUBVERSION_8723a     (0x1200)

#define CHIPTYPE_NONE           (0x1F)      //Chip Type's range: 0x0 ~ 0xF
#define CHIP_TYPE_MASK_ALL      (0xFFFFFFFF)

#define PROJECT_ID_MASK_ALL     (0xFFFFFFFF)    // temp used for unknow project id for a new chip

#define PATCH_OPTIONAL_MATCH_FLAG_CHIPTYPE   (0x1)

#define CONFIG_MAC_OFFSET_GEN_1_2       (0x3C)      //MAC's OFFSET in config/efuse for realtek generation 1~2 bluetooth chip
#define CONFIG_MAC_OFFSET_GEN_3PLUS     (0x44)      //MAC's OFFSET in config/efuse for rtk generation 3+ bluetooth chip

#define HCI_EVT_CMD_CMPL_OPCODE_OFFSET          (3)     //opcode's offset in COMMAND Completed Event
#define HCI_EVT_CMD_CMPL_STATUS_OFFSET          (5)     //status's offset in COMMAND Completed Event

#define HCI_EVT_CMD_CMPL_OP1001_HCI_VERSION_OFFSET     (6)     //HCI_Version's offset in COMMAND Completed Event for OpCode 0x1001(Read Local Version Information Command)
#define HCI_EVT_CMD_CMPL_OP1001_HCI_REVISION_OFFSET     (7)     //HCI_Revision's offset in COMMAND Completed Event for OpCode 0x1001(Read Local Version Information Command)
#define HCI_EVT_CMD_CMPL_OP1001_LMP_SUBVERSION_OFFSET  (12)    //LMP Subversion's offset in COMMAND Completed Event for OpCode 0x1001(Read Local Version Information Command)
#define HCI_EVT_CMD_CMPL_OP0C14_LOCAL_NAME_OFFSET      (6)     //Local Name's offset in COMMAND Completed Event for OpCode 0x0C14(Read Local Name Command)
#define HCI_EVT_CMD_CMPL_OP1009_BDADDR_OFFSET    (6)     //BD_ADDR's offset in COMMAND Completed Event for OpCode 0x1009(Read BD_ADDR Command)
#define HCI_EVT_CMD_CMPL_OPFC6D_EVERSION_OFFSET        (6)  //eversion's offset in COMMAND Completed Event for OpCode 0xfc6d(Read eVERSION Vendor Command)
#define HCI_EVT_CMD_CMPL_OPFC61_CHIPTYPE_OFFSET        (6)  //chip type's offset in COMMAND Completed Event for OpCode 0xfc61(Read ChipType Vendor Command)

//#define UPDATE_BAUDRATE_CMD_PARAM_SIZE          (6)
#define HCI_CMD_PREAMBLE_SIZE                   (3)
#define HCI_CMD_READ_CHIP_TYPE_SIZE             (5)
//#define HCD_REC_PAYLOAD_LEN_BYTE                (2)
//#define BD_ADDR_LEN                             (6)
//#define LOCAL_NAME_BUFFER_LEN                   (32)
//#define LOCAL_BDADDR_PATH_BUFFER_LEN            (256)

#define H5_SYNC_REQ_SIZE (2)
#define H5_SYNC_RYOC_SIZE (2)
#define H5_CONF_REQ_SIZE (3)
#define H5_CONF_RYOC_SIZE (2)

typedef struct _baudrate_ex {
    uint32_t rtk_speed;
    uint32_t uart_speed;
} baudrate_ex;

baudrate_ex baudrates[] = {
    {0x00006004, 921600},
    {0x05F75004, 921600},//RTL8723BS
    {0x00004003, 1500000},
    {0x04928002, 1500000},//RTL8723BS
    {0x00005002, 2000000},//same as RTL8723AS
    {0x00008001, 3000000},
    {0x04928001, 3000000},//RTL8723BS
    {0x06B58001, 3000000},//add RTL8703as
    {0x00007001, 3500000},
    {0x052A6001, 3500000},//RTL8723BS
    {0x00005001, 4000000},//same as RTL8723AS
    {0x0000701d, 115200},
    {0x0252C014, 115200}//RTL8723BS
};

//signature: realtech
const uint8_t RTK_EPATCH_SIGNATURE[8] = {0x52, 0x65, 0x61, 0x6C, 0x74, 0x65, 0x63, 0x68};
//Extension Section IGNATURE:0x77FD0451
const uint8_t EXTENSION_SECTION_SIGNATURE[4] = {0x51, 0x04, 0xFD, 0x77};

typedef struct {
    uint16_t    lmp_subversion;
    uint32_t     hci_version_mask;
    uint32_t     hci_revision_mask;
    uint32_t     chip_type_mask;
    uint32_t     project_id_mask;
    char        *patch_name;
    char        *config_name;
    uint16_t     mac_offset;
    uint32_t    max_patch_size;
} patch_info;

struct rtk_epatch_entry {
    uint16_t chip_id;
    uint16_t patch_length;
    uint32_t patch_offset;
    uint32_t svn_version;
    uint32_t coex_version;
} __attribute__((packed));

struct rtk_epatch {
    uint8_t signature[8];
    uint32_t fw_version;
    uint16_t number_of_patch;
    struct rtk_epatch_entry entry[0];
} __attribute__((packed));

/* h/w config control block */
typedef struct {
    uint32_t    max_patch_size;
    uint32_t    baudrate;
    uint16_t    lmp_subversion;
    uint8_t     state;          /* Hardware configuration state */
    uint8_t     eversion;
    uint32_t    project_id_mask;
    uint8_t     hci_version;
    uint8_t     hci_revision;
    uint8_t     chip_type;
    uint8_t     dl_fw_flag;
    int         fw_len;          /* FW patch file len */
    int         config_len;      /* Config patch file len */
    int         total_len;       /* FW & config extracted buf len */
    uint8_t     *fw_buf;         /* FW patch file buf */
    uint8_t     *config_buf;     /* Config patch file buf */
    uint8_t     *total_buf;      /* FW & config extracted buf */
    uint8_t     patch_frag_cnt;  /* Patch fragment count download */
    uint8_t     patch_frag_idx;  /* Current patch fragment index */
    uint8_t     patch_frag_len;  /* Patch fragment length */
    uint8_t     patch_frag_tail; /* Last patch fragment length */
    uint8_t     hw_flow_cntrl;   /* Uart flow control, bit7:set, bit0:enable */
} bt_hw_cfg_cb_t;
static hci_driver_send_cmd_t g_send_cmd;
static uint8_t resp_data[1024];
static uint8_t g_is_mp_mode = 0;
static bt_hw_cfg_cb_t hw_cfg_cb;

void set_mp_mode()
{
    g_is_mp_mode = 1;
}

static int getmacaddr(unsigned char *addr)
{
#if 0
    int i = 0;
    char data[256], *str;
    int addr_fd;

    if ((addr_fd = open("/data/misc/bluetooth/bdaddr", O_RDONLY)) != -1) {
        memset(data, 0, sizeof(data));
        read(addr_fd, data, 17);

        for (i = 0, str = data; i < 6; i++) {
            addr[5 - i] = (unsigned char)strtoul(str, &str, 16);
            str++;
        }

        close(addr_fd);
        return 0;
    }
#endif

    return -1;
}

static inline int getAltSettings(patch_info *patch_entry, unsigned short *offset, int max_group_cnt)
{
    int n = 0;

    if (patch_entry) {
        offset[n++] = patch_entry->mac_offset;
    }

    /*
    //sample code, add special settings

        offset[n++] = 0x15B;
    */
    return n;
}
static inline int getAltSettingVal(patch_info *patch_entry, unsigned short offset, unsigned char *val)
{
    int res = 0;

    switch (offset) {
        /*
        //sample code, add special settings
                case 0x15B:
                    val[0] = 0x0B;
                    val[1] = 0x0B;
                    val[2] = 0x0B;
                    val[3] = 0x0B;
                    res = 4;
                    break;
        */
        default:
            res = 0;
            break;
    }

    if ((patch_entry) && (offset == patch_entry->mac_offset) && (res == 0)) {
        if (getmacaddr(val) == 0) {
            HCI_LOGD(TAG,"MAC: %02x:%02x:%02x:%02x:%02x:%02x", val[5], val[4], val[3], val[2], val[1], val[0]);
            res = 6;
        }
    }

    return res;
}

static uint32_t rtk_parse_config_file(unsigned char **config_buf, size_t *filelen)
{
    struct rtk_bt_vendor_config *config = (struct rtk_bt_vendor_config *) *config_buf;
    uint16_t config_len = le16_to_cpu(config->data_len), temp = 0;
    struct rtk_bt_vendor_config_entry *entry = config->entry;
    unsigned int i = 0;
    uint32_t baudrate = 0;
    //uint32_t config_has_bdaddr = 0;
    uint8_t *p;

    if (le32_to_cpu(config->signature) != RTK_VENDOR_CONFIG_MAGIC) {
        LOGE(TAG,"config signature magic number(0x%x) is not set to RTK_VENDOR_CONFIG_MAGIC", config->signature);
        return 0;
    }

    if (config_len != *filelen - sizeof(struct rtk_bt_vendor_config)) {
        LOGE(TAG,"config len(0x%x) is not right(0x%x)", config_len, *filelen - sizeof(struct rtk_bt_vendor_config));
        return 0;
    }

    for (i = 0; i < config_len;) {
        switch (le16_to_cpu(entry->offset)) {
            case 0xc: {
                p = (uint8_t *)entry->entry_data;
                STREAM_TO_UINT32(baudrate, p);

                if (entry->entry_len >= 12) {
                    hw_cfg_cb.hw_flow_cntrl |= 0x80; /* bit7 set hw flow control */

                    if (entry->entry_data[12] & 0x04) { /* offset 0x18, bit2 */
                       hw_cfg_cb.hw_flow_cntrl |= 1;    /* bit0 enable hw flow control */
                    }
                }

                HCI_LOGD(TAG,"config baud rate to :0x%08x, hwflowcontrol:0x%x, 0x%x", baudrate, entry->entry_data[12], hw_cfg_cb.hw_flow_cntrl);
                break;
            }

            default:
                HCI_LOGD(TAG,"config offset(0x%x),length(0x%x)", entry->offset, entry->entry_len);
                break;
        }

        temp = entry->entry_len + sizeof(struct rtk_bt_vendor_config_entry);
        i += temp;
        entry = (struct rtk_bt_vendor_config_entry *)((uint8_t *)entry + temp);
    }

    return baudrate;
}

static uint32_t rtk_get_bt_config(unsigned char **config_buf,
                                  uint32_t *config_baud_rate)
{
    size_t filelen = 0;

    if ((*config_buf = malloc(MAX_ORG_CONFIG_SIZE + MAX_ALT_CONFIG_SIZE)) == NULL) {
        LOGE(TAG,"malloc buffer for config file fail(0x%x)\n", filelen);
        //close(fd);
        return -1;
    }

    if (g_is_mp_mode) {
        memcpy(*config_buf, rtl8723ds_mp_config, rtl8723ds_mp_config_len);

        filelen = rtl8723ds_mp_config_len;
    } else {
        memcpy(*config_buf, rtl8723d_config, rtl8723d_config_len);

        filelen = rtl8723d_config_len;
    }

    *config_baud_rate = rtk_parse_config_file(config_buf, &filelen);
    //*config_baud_rate = 0;
    HCI_LOGD(TAG,"Get config baud rate from config file:0x%x", *config_baud_rate);

    return filelen;
}

static int rtk_get_bt_firmware(uint8_t **fw_buf)
{
    //char filename[PATH_MAX] = {0};
    size_t buf_size = 0;

    if (g_is_mp_mode) {
        buf_size = rtl8723ds_mp_len;
    } else {
        buf_size = rtl8723d_fw_len;
    }

    if (!(*fw_buf = malloc(buf_size))) {
        LOGE(TAG,"Can't alloc memory for fw&config, errno:%d", errno);
        return -1;
    }

    if (g_is_mp_mode) {
        memcpy(*fw_buf, rtl8723ds_mp, buf_size);
    } else {
        memcpy(*fw_buf, rtl8723d_fw, buf_size);
    }

    HCI_LOGD(TAG,"Load FW OK");

    return buf_size;
}

static struct rtk_epatch_entry *rtk_get_patch_entry(bt_hw_cfg_cb_t *cfg_cb)
{
    uint16_t i;
    struct rtk_epatch *patch;
    struct rtk_epatch_entry *entry;
    uint8_t *p;
    uint16_t chip_id;

    patch = (struct rtk_epatch *)cfg_cb->fw_buf;
    entry = (struct rtk_epatch_entry *)malloc(sizeof(*entry));

    if (!entry) {
        LOGE(TAG,"rtk_get_patch_entry: failed to allocate mem for patch entry");
        return NULL;
    }

    patch->number_of_patch = le16_to_cpu(patch->number_of_patch);

    HCI_LOGD(TAG,"rtk_get_patch_entry: fw_ver 0x%08x, patch_num %d",
          le32_to_cpu(patch->fw_version), patch->number_of_patch);

    for (i = 0; i < patch->number_of_patch; i++) {
        p = cfg_cb->fw_buf + 14 + 2 * i;
        STREAM_TO_UINT16(chip_id, p);

        if (chip_id == cfg_cb->eversion + 1) {
            entry->chip_id = chip_id;
            p = cfg_cb->fw_buf + 14 + 2 * patch->number_of_patch + 2 * i;
            STREAM_TO_UINT16(entry->patch_length, p);
            p = cfg_cb->fw_buf + 14 + 4 * patch->number_of_patch + 4 * i;
            STREAM_TO_UINT32(entry->patch_offset, p);
            HCI_LOGD(TAG,"rtk_get_patch_entry: chip_id %d, patch_len 0x%x, patch_offset 0x%x",
                  entry->chip_id, entry->patch_length, entry->patch_offset);
            break;
        }
    }

    if (i == patch->number_of_patch) {
        LOGE(TAG,"rtk_get_patch_entry: failed to get etnry");
        free(entry);
        entry = NULL;
    }

    return entry;
}

static void rtk_get_bt_final_patch(bt_hw_cfg_cb_t *cfg_cb)
{
    struct rtk_epatch_entry *entry = NULL;
    struct rtk_epatch *patch = (struct rtk_epatch *)cfg_cb->fw_buf;
    //int iBtCalLen = 0;

    if (cfg_cb->lmp_subversion == LMPSUBVERSION_8723a) {
        if (memcmp(cfg_cb->fw_buf, RTK_EPATCH_SIGNATURE, 8) == 0) {
            LOGE(TAG,"8723as check signature error!");
            cfg_cb->dl_fw_flag = 0;
            goto free_buf;
        } else {
            cfg_cb->total_len = cfg_cb->fw_len + cfg_cb->config_len;

            if (!(cfg_cb->total_buf = malloc(cfg_cb->total_len))) {
                LOGE(TAG,"can't alloc memory for fw&config, errno:%d", errno);
                cfg_cb->dl_fw_flag = 0;
                goto free_buf;
            } else {
                HCI_LOGD(TAG,"8723as, fw copy direct");
                memcpy(cfg_cb->total_buf, cfg_cb->fw_buf, cfg_cb->fw_len);
                memcpy(cfg_cb->total_buf + cfg_cb->fw_len, cfg_cb->config_buf, cfg_cb->config_len);
                cfg_cb->dl_fw_flag = 1;
                goto free_buf;
            }
        }
    }

    if (memcmp(cfg_cb->fw_buf, RTK_EPATCH_SIGNATURE, 8)) {
        LOGE(TAG,"check signature error");
        cfg_cb->dl_fw_flag = 0;
        goto free_buf;
    }

    /* check the extension section signature */
    if (memcmp(cfg_cb->fw_buf + cfg_cb->fw_len - 4, EXTENSION_SECTION_SIGNATURE, 4)) {
        LOGE(TAG,"check extension section signature error");
        cfg_cb->dl_fw_flag = 0;
        goto free_buf;
    }

    // proj_id = rtk_get_fw_project_id(cfg_cb->fw_buf + cfg_cb->fw_len - 5);

    // if ((hw_cfg_cb.project_id_mask != PROJECT_ID_MASK_ALL) && ((hw_cfg_cb.project_id_mask & (1 << proj_id)) == 0)) {
    //     LOGE(TAG,"hw_cfg_cb.project_id_mask is 0x%08x, fw project_id is %d, does not match!!!",
    //           hw_cfg_cb.project_id_mask, proj_id);
    //     cfg_cb->dl_fw_flag = 0;
    //     goto free_buf;
    // }

    entry = rtk_get_patch_entry(cfg_cb);

    if (entry) {
        cfg_cb->total_len = entry->patch_length + cfg_cb->config_len;
    } else {
        cfg_cb->dl_fw_flag = 0;
        goto free_buf;
    }

    HCI_LOGD(TAG,"total_len = 0x%x", cfg_cb->total_len);

    if (!(cfg_cb->total_buf = malloc(cfg_cb->total_len))) {
        LOGE(TAG,"Can't alloc memory for multi fw&config, errno:%d", errno);
        cfg_cb->dl_fw_flag = 0;
        goto free_buf;
    } else {
        memcpy(cfg_cb->total_buf, cfg_cb->fw_buf + entry->patch_offset, entry->patch_length);
        memcpy(cfg_cb->total_buf + entry->patch_length - 4, &patch->fw_version, 4);
        memcpy(&entry->svn_version, cfg_cb->total_buf + entry->patch_length - 8, 4);
        memcpy(&entry->coex_version, cfg_cb->total_buf + entry->patch_length - 12, 4);
        HCI_LOGD(TAG,"BTCOEX:20%06d-%04x svn_version:%d lmp_subversion:0x%x hci_version:0x%x hci_revision:0x%x chip_type:%d Cut:%d libbt-vendor_uart version:%s\n",
              ((entry->coex_version >> 16) & 0x7ff) + ((entry->coex_version >> 27) * 10000),
              (entry->coex_version & 0xffff), entry->svn_version, cfg_cb->lmp_subversion, cfg_cb->hci_version, cfg_cb->hci_revision, cfg_cb->chip_type, cfg_cb->eversion + 1, RTK_VERSION);
    }

    if (cfg_cb->config_len) {
        memcpy(cfg_cb->total_buf + entry->patch_length, cfg_cb->config_buf, cfg_cb->config_len);
    }

    cfg_cb->dl_fw_flag = 1;
    HCI_LOGD(TAG,"Fw:%s exists, config file:%s exists", (cfg_cb->fw_len > 0) ? "" : "not", (cfg_cb->config_len > 0) ? "" : "not");

free_buf:

    if (cfg_cb->fw_len > 0) {
        free(cfg_cb->fw_buf);
        cfg_cb->fw_len = 0;
    }

    if (cfg_cb->config_len > 0) {
        free(cfg_cb->config_buf);
        cfg_cb->config_len = 0;
    }

    if (entry) {
        free(entry);
    }
}

static uint8_t line_speed_to_userial_baud(uint32_t line_speed)
{
    uint8_t baud;

    if (line_speed == 4000000) {
        baud = USERIAL_BAUD_4M;
    } else if (line_speed == 3000000) {
        baud = USERIAL_BAUD_3M;
    } else if (line_speed == 2000000) {
        baud = USERIAL_BAUD_2M;
    } else if (line_speed == 1500000) {
        baud = USERIAL_BAUD_1_5M;
    } else if (line_speed == 1000000) {
        baud = USERIAL_BAUD_1M;
    } else if (line_speed == 921600) {
        baud = USERIAL_BAUD_921600;
    } else if (line_speed == 460800) {
        baud = USERIAL_BAUD_460800;
    } else if (line_speed == 230400) {
        baud = USERIAL_BAUD_230400;
    } else if (line_speed == 115200) {
        baud = USERIAL_BAUD_115200;
    } else if (line_speed == 57600) {
        baud = USERIAL_BAUD_57600;
    } else if (line_speed == 19200) {
        baud = USERIAL_BAUD_19200;
    } else if (line_speed == 9600) {
        baud = USERIAL_BAUD_9600;
    } else if (line_speed == 1200) {
        baud = USERIAL_BAUD_1200;
    } else if (line_speed == 600) {
        baud = USERIAL_BAUD_600;
    } else {
        LOGE(TAG,"userial vendor: unsupported baud speed %d", line_speed);
        baud = USERIAL_BAUD_115200;
    }

    return baud;
}

static void rtk_speed_to_uart_speed(uint32_t rtk_speed, uint32_t *uart_speed)
{
    *uart_speed = 115200;

    uint8_t i;

    for (i = 0; i < sizeof(baudrates) / sizeof(baudrate_ex); i++) {
        if (baudrates[i].rtk_speed == rtk_speed) {
            *uart_speed = baudrates[i].uart_speed;
            return;
        }
    }

    return;
}

static void hw_h5_init()
{
    int ret = 0;

    if (g_send_cmd) {
        ret = g_send_cmd(HCI_VSC_H5_INIT, NULL, 0, NULL, NULL);
    }

    aos_check(!ret, EIO);
}

static void hw_h5_read_lpm_version()
{
    uint32_t resp_len = 1024;
    uint8_t *p;
    int ret = 0;

    if (g_send_cmd) {
        ret = g_send_cmd(HCI_READ_LMP_VERSION, NULL, 0, resp_data, &resp_len);
    }

    aos_check(!ret, EIO);

    // LOG_HEXDUMP(TAG, resp_data, resp_len);

    p = ((uint8_t *)(resp_data + 1) + HCI_EVT_CMD_CMPL_OP1001_HCI_VERSION_OFFSET - 6);
    STREAM_TO_UINT16(hw_cfg_cb.hci_version, p);
    p = ((uint8_t *)(resp_data + 1) + HCI_EVT_CMD_CMPL_OP1001_HCI_REVISION_OFFSET - 6);
    STREAM_TO_UINT16(hw_cfg_cb.hci_revision, p);
    p = (uint8_t *)(resp_data + 1) + HCI_EVT_CMD_CMPL_OP1001_LMP_SUBVERSION_OFFSET - 6;
    STREAM_TO_UINT16(hw_cfg_cb.lmp_subversion, p);

    HCI_LOGD(TAG,"lmp_subversion = 0x%x hw_cfg_cb.hci_version = 0x%x hw_cfg_cb.hci_revision = 0x%x", hw_cfg_cb.lmp_subversion, hw_cfg_cb.hci_version, hw_cfg_cb.hci_revision);
}

static void hw_h5_read_rom_version()
{
    uint32_t resp_len = 1024;
    int ret = 0;

    if (g_send_cmd) {
        ret = g_send_cmd(HCI_VSC_READ_ROM_VERSION, NULL, 0, resp_data, &resp_len);
    }

    aos_check(!ret, EIO);

    hw_cfg_cb.eversion = *((uint8_t *)(resp_data + 1) + HCI_EVT_CMD_CMPL_OPFC6D_EVERSION_OFFSET - 6);
    HCI_LOGD(TAG,"hw_config_cback chip_id of the IC:%d", hw_cfg_cb.eversion + 1);
}

static void hw_h5_read_chip_type()
{
    uint32_t resp_len = 1024;
    int ret = 0;
    uint8_t send_data[6];

    uint8_t *p = send_data;

    // *p++ = 5;
    UINT8_TO_STREAM(p, 0x00);
    UINT32_TO_STREAM(p, 0xB000A094);

    if (g_send_cmd) {
        ret = g_send_cmd(HCI_VSC_READ_CHIP_TYPE, send_data, 5, resp_data, &resp_len);
    }

    aos_check(!ret, EIO);
    hw_cfg_cb.chip_type = ((*((uint8_t *)(resp_data + 1) + HCI_EVT_CMD_CMPL_OPFC61_CHIPTYPE_OFFSET - 6)) & 0x0F);

    HCI_LOGD(TAG,"READ_CHIP_TYPE hw_cfg_cb.chip_type = 0x%x", hw_cfg_cb.chip_type);
}

static void hw_h5_config_set_controller_baudrate(uint32_t baudrate)
{
    uint32_t resp_len = 1024;
    int ret = 0;
    uint8_t send_data[6];

    uint8_t *p = send_data;

    // *p++ = 4;
    UINT32_TO_STREAM(p, baudrate);

    HCI_LOGD(TAG,"%s band %x", __func__, baudrate);

    if (g_send_cmd) {
        ret = g_send_cmd(HCI_VSC_UPDATE_BAUDRATE, send_data, 4, resp_data, &resp_len);
    }

    HCI_LOGD(TAG,"%s  resp_len %ld, resp_data %x", __func__, resp_len, resp_data[0]);

    aos_check(!ret, EIO);
}

static int hw_h5_download_patch_cmd(int index, uint8_t *data, int len)
{
    uint32_t resp_len = 1024;
    uint8_t *p;
    int ret = 0;
    uint8_t *send_data = malloc(len + 2);
    uint8_t     iIndexRx = 0;

    if (send_data == NULL) {
        return -1;
    }

    p = send_data;

    // *p++ = 1 + len;  /* parameter length */
    *p++ = index;
    memcpy(p, data, len);

    if (g_send_cmd) {
        ret = g_send_cmd(HCI_VSC_DOWNLOAD_FW_PATCH, send_data, len + 1, resp_data, &resp_len);
    }

    aos_check(!ret, EIO);

    iIndexRx = *((uint8_t *)(resp_data + 1) + HCI_EVT_CMD_CMPL_STATUS_OFFSET + 1 - 6);
    // HCI_LOGD(TAG,"bt vendor lib: HW_CFG_DL_FW_PATCH, iIndexRx:%i", iIndexRx);
    hw_cfg_cb.patch_frag_idx++;

    free(send_data);
    
    if (iIndexRx & 0x80) {
        HCI_LOGD(TAG,"vendor lib fwcfg completed");
        free(hw_cfg_cb.total_buf);
        hw_cfg_cb.total_len = 0;

        hw_cfg_cb.state = 0;
        return 1;
    }

    return 0;
}

static void hw_h5_download_patch()
{
    uint8_t     iIndexRx = 0;
    int ret = 0;

    while (ret == 0) {
        iIndexRx = hw_cfg_cb.patch_frag_idx ? ((hw_cfg_cb.patch_frag_idx - 1) % 0x7f + 1) : 0;

        if (hw_cfg_cb.patch_frag_idx == hw_cfg_cb.patch_frag_cnt - 1) {
            HCI_LOGD(TAG,"HW_CFG_DL_FW_PATCH: send last fw fragment");
            iIndexRx |= 0x80;
            hw_cfg_cb.patch_frag_len = hw_cfg_cb.patch_frag_tail;
        } else {
            iIndexRx &= 0x7F;
            hw_cfg_cb.patch_frag_len = PATCH_DATA_FIELD_MAX_SIZE;
        }

        HCI_LOGD(TAG,"HW_CFG_DL_FW_PATCH: send %d fragment", iIndexRx);

        ret = hw_h5_download_patch_cmd(iIndexRx,
                            hw_cfg_cb.total_buf + (hw_cfg_cb.patch_frag_idx * PATCH_DATA_FIELD_MAX_SIZE),
                            hw_cfg_cb.patch_frag_len);
    }
}

static void hw_h5_pre_download_patch()
{
    hw_cfg_cb.config_len = rtk_get_bt_config(&hw_cfg_cb.config_buf, &hw_cfg_cb.baudrate);

    if (hw_cfg_cb.config_len < 0) {
        LOGE(TAG,"Get Config file fail, just use efuse settings");
        hw_cfg_cb.config_len = 0;
        return;
    }

    // rtk_update_altsettings(hw_cfg_cb.config_buf, (size_t*)&(hw_cfg_cb.config_len));

    hw_cfg_cb.fw_len = rtk_get_bt_firmware(&hw_cfg_cb.fw_buf);

    if (hw_cfg_cb.fw_len < 0) {
        LOGE(TAG,"Get BT firmware fail");
        hw_cfg_cb.fw_len = 0;
        return;
    } else {
        // hw_cfg_cb.project_id_mask = prtk_patch_file_info->project_id_mask;
        rtk_get_bt_final_patch(&hw_cfg_cb);
    }

    HCI_LOGD(TAG,"Check total_len(0x%08x)", hw_cfg_cb.total_len);

    if ((hw_cfg_cb.total_len > 0) && hw_cfg_cb.dl_fw_flag) {
        hw_cfg_cb.patch_frag_cnt = hw_cfg_cb.total_len / PATCH_DATA_FIELD_MAX_SIZE;
        hw_cfg_cb.patch_frag_tail = hw_cfg_cb.total_len % PATCH_DATA_FIELD_MAX_SIZE;

        if (hw_cfg_cb.patch_frag_tail) {
            hw_cfg_cb.patch_frag_cnt += 1;
        } else {
            hw_cfg_cb.patch_frag_tail = PATCH_DATA_FIELD_MAX_SIZE;
        }

        HCI_LOGD(TAG,"patch fragment count %d, tail len %d", hw_cfg_cb.patch_frag_cnt, hw_cfg_cb.patch_frag_tail);
    }
}

static void hw_h5_change_uart_baudrate()
{
    uint32_t    host_baudrate = 0;

    rtk_speed_to_uart_speed(hw_cfg_cb.baudrate, &host_baudrate);
    HCI_LOGD(TAG,"bt vendor lib: set HOST UART baud %i", host_baudrate);
    userial_vendor_set_baud(line_speed_to_userial_baud(host_baudrate));
}

/*****************************************************************************
**   Hardware Configuration Interface Functions
*****************************************************************************/


/*******************************************************************************
**
** Function        hw_config_start
**
** Description     Kick off controller initialization process
**
** Returns         None
**
*******************************************************************************/
void hw_config_start(hci_driver_send_cmd_t send_cmd)
{
    g_send_cmd = send_cmd;

    HCI_LOGD(TAG,"hw_config_start\n");

    hw_h5_init();
    hw_h5_read_lpm_version();
    hw_h5_read_rom_version();
    hw_h5_read_chip_type();
    hw_h5_pre_download_patch();
    hw_h5_config_set_controller_baudrate(hw_cfg_cb.baudrate);
    // wait for controller baudrate ready
    aos_msleep(10);
    hw_h5_change_uart_baudrate();

    if (hw_cfg_cb.hw_flow_cntrl & 0x01) {
        userial_vendor_set_hw_fctrl(1);
    } else {
        userial_vendor_set_hw_fctrl(0);
    }

    hw_h5_download_patch();
}
