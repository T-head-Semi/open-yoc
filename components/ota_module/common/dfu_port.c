/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include "dfu_port.h"
#include "yoc/partition.h"
#include "ulog/ulog.h"
#include <stdio.h>
#include <string.h>
#include "aos/hal/flash.h"

#define TAG "DFU_PORT"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))


typedef struct {
    uint32_t magic;
    uint8_t ver;
    uint8_t image_sum;
    uint8_t digest_type;
    uint8_t signature_type;
    uint32_t mnft_off;
    uint8_t head_len;
    uint8_t pad_type;
    uint8_t rsvd[2];
    uint8_t app_version[CONFIG_MAX_FOTA_APP_VERSION_LENGTH];
} fota_head_info_t;

const char *hex_dump(const void *buf, size_t len)
{
    static const char hex[] = "0123456789abcdef";
    static char str[300];
    const uint8_t *b = buf;
    size_t i;

    len = MIN(len, (sizeof(str) - 1) / 2);

    for (i = 0; i < len; i++) {
        str[i * 2]     = hex[b[i] >> 4];
        str[i * 2 + 1] = hex[b[i] & 0xf];
    }

    str[i * 2] = '\0';

    return str;
}

int dfu_image_read(uint8_t* start_address,uint32_t offset,uint32_t length,uint8_t* buffer)
{
    int ret = 0;
    partition_t h = partition_open("misc");
    partition_info_t *info = partition_info_get(h);
    if(!info) {
        return -1;
    }
    uint32_t partion_abso_addr = info->base_addr + info->start_addr;
    if(partion_abso_addr > (size_t)start_address || (offset + length)>  info->length) {
        LOGE(TAG,"The ota device image is over OTA temporary!\r\n");
        return -1;
    }
    off_t read_offset = (size_t)start_address - partion_abso_addr + offset;
    ret = partition_read(h, read_offset, buffer, length);
    //TODO
    if(ret) {
        printf("read faild %d\r\n",ret);
    }
    static uint8_t  read_first = 0;
    if(read_first) {
        fota_head_info_t *info = (fota_head_info_t *)buffer;
        printf("%02x %04x %04x\r\n",info->head_len,info->ver,info->mnft_off);
    }
    //LOGD(TAG,"%d %p %s",length,buffer,hex_dump(buffer,length));
    partition_close(h);
    return ret;
}



extern uint16_t util_crc16_ccitt(uint8_t const *p_data, uint32_t size, uint16_t const *p_crc);
extern void drv_reboot(void);



#define ADDR_GRAN_MASK (0xFFF)
#define ADDR_ALIGN_UP(a) (((a) + ADDR_GRAN_MASK) & ~ADDR_GRAN_MASK)

static unsigned short image_crc16 = 0;

/**
 * @brief 写flash上锁
 *
 * @param[in]  -
 *
 * @return -
 */
void lock_flash()
{
    //TODO
    //hal_flash_enable_secure(HAL_PARTITION_OTA_TEMP, 0, 0);
}

/**
 * @brief 写flash解锁
 *
 * @param[in]  -
 *
 * @return -
 */
void unlock_flash_all()
{
    // TODO
    //hal_flash_dis_secure(HAL_PARTITION_OTA_TEMP, 0, 0);
}


/**
 * @brief 镜像更新
 *
 * @param[in]  signature    暂时不使用
 * @param[in]  offset       当前buf代表的内容，从镜像bin文件从offset位置开始，比如为100，表示当前buffer是bin文件的第100字节开始
 * @param[in]  length       本次buffer的长度
 * @param[in]  buf          本次写入的具体内容
 *
 * @return 0:success, otherwise is failed
 */
int dfu_image_update(short signature, int offset, int length, int *buf)
{
    int ret;
    partition_t h;
    partition_info_t *info = NULL;
    uint32_t wr_idx = offset;
    uint8_t *wr_buf = (uint8_t *)buf;


    if (offset == 0) {
        image_crc16 = util_crc16_ccitt(wr_buf, length, NULL);
    } else {
        image_crc16 = util_crc16_ccitt(wr_buf, length, &image_crc16);
    }

    ///get OTA temporary partition information
    h =    partition_open("misc");
    info = partition_info_get(h);
    if(!info) {
        LOGE(TAG,"get misc partition faild");
        return -1;
    }
    /* For bootloader upgrade, we will reserve two sectors, then save the image */
    wr_idx += (info->sector_size << 1);

    if (info->length < (offset + length + (info->sector_size << 1))) {
        LOGE(TAG,"The write range is over OTA temporary!\r\n");
        return -1;
    }

    ret = partition_write(h, wr_idx, wr_buf, length);
    if (ret < 0) {
        LOGE(TAG,"write flash error!!\r\n");
        return -1;
    }
#if 0
    uint8_t  r_buf[256];
    ret = partition_read(h, wr_idx, (void *)r_buf, length);
    if(ret < 0) {
        LOGE(TAG,"read flash error!!\r\n");
        return -1;
    }
    LOGD(TAG,"read: %d %d %s\r\n",wr_idx,length,bt_hex(r_buf,length));
#endif
    partition_close(h);
    return 0;
}

uint8_t* dfu_get_start_address()
{
    partition_t h;
    partition_info_t *info = NULL;
    ///get OTA temporary partition information
    h =    partition_open("misc");
    info = partition_info_get(h);
    if(!info) {
        LOGE(TAG,"get misc partition faild");
        return NULL;
    }

    return (uint8_t *)(size_t)(info->base_addr + info->start_addr  + (info->sector_size << 1));
}
/**
 * @brief 镜像更新
 *
 * @param[in]  signature    暂时不使用
 * @param[in]  offset       当前buf代表的内容，从镜像bin文件从offset位置开始，比如为100，表示当前buffer是bin文件的第100字节开始
 * @param[in]  length       本次buffer的长度
 * @param[in]  buf          本次写入的具体内容
 *
 * @return 0:success, otherwise is failed
 */
int dfu_image_update_no_crc(short signature, int offset, int length, uint8_t *buf)
{
    int ret;
    partition_t h;
    partition_info_t *info = NULL;
    uint32_t wr_idx = offset;
    uint8_t *wr_buf = buf;

    ///get OTA temporary partition information
    h =    partition_open("misc");
    info = partition_info_get(h);
    if(!info) {
        LOGE(TAG,"get misc partition faild");
        return -1;
    }
    /* For bootloader upgrade, we will reserve two sectors, then save the image */
    wr_idx += (info->sector_size << 1 );

#if 1
    if (info->length < (offset + length + (info->sector_size << 1))) {
        LOGE(TAG,"The write range is over OTA temporary!\r\n");
        return -1;
    }
#endif

    ret = partition_write(h, wr_idx, wr_buf, length);
    if (ret < 0) {
        LOGE(TAG,"write flash error!!\r\n");
        return -1;
    }
#if 0
    uint8_t  r_buf[256];
    ret = partition_read(h, wr_idx, (void *)r_buf, length);
    if(ret < 0) {
        LOGE(TAG,"read flash error!!\r\n");
        return -1;
    }
    LOGD(TAG,"read: %d %d %s\r\n",wr_idx,length,bt_hex_real(r_buf,length));
#endif
    partition_close(h);
    return 0;
}

/**
 * @brief 写入flash之前和之后checksum计算
 *
 * @param[in]  image_id 暂时不使用
 * @param[in]  crc16_output 计算出来的crc返回给调用者
 *
 * @return 1:success 0:failed
 */
unsigned char dfu_check_checksum(short image_id, unsigned short *crc16_output)
{
    *crc16_output = image_crc16;
    return 1;
}

/**
 * @brief 升级结束后重启
 *
 * @param[in]  -
 * @return -
 * @说明: 比如在此函数里面call 切换镜像分区的业务逻辑
 */
void dfu_reboot()
{
    drv_reboot();
}

#ifdef CONFIG_GENIE_OTA_PINGPONG
uint8_t ble_sal_ota_get_current_image_id(void)
{
    return 0;
}

uint8_t ble_sal_ota_change_image_id(uint8_t target_id)
{
    return 0;
}
#endif

int erase_dfu_flash(uint8_t force)
{
    int ret;
    partition_t h;
    partition_info_t *info = NULL;
    uint32_t offset;
    uint8_t cmp_buf[32] = {0xFF};
    uint8_t wr_buf[32] = {0};
    ///get OTA temporary partition information

    h =    partition_open("misc");
    info = partition_info_get(h);
    offset = info->sector_size << 1;
    if(!info) {
        LOGE(TAG,"get misc partition faild");
        return -1;
    }
    memset(cmp_buf, 0xFF, sizeof(cmp_buf));

    if(!force) {
        ret = partition_read(h, offset, (void *)wr_buf, sizeof(wr_buf));
        if (ret < 0) {
            LOGE(TAG,"read flash error!!\r\n");
            return -1;
        }
        if (memcmp(wr_buf, cmp_buf, sizeof(wr_buf)) == 0) {
            return 0;
        }
        LOG("OTA dirty\n");
    }

    /* For bootloader upgrade, we will reserve two sectors, then save the image */

    uint16_t  block_size = (info->length + info->sector_size - 1) / info->sector_size;
    if(block_size <= 2) {
        LOGE(TAG,"partition should larger than two sectors !!\r\n");
        return -1;
    }
    ret = partition_erase(h, offset, block_size - 2);
    if (ret < 0) {
        LOG("Erase flash error %d!!\r\n",ret);
        return -1;
    }

    partition_close(h);

    return 0;
}

int dfu_get_ota_partition_max_size(void)
{
    uint32_t reserve_size;
    partition_t h;
    partition_info_t *info = NULL;

    h =    partition_open("misc");
    info = partition_info_get(h);
    reserve_size = (info->sector_size << 1);

    if(!info) {
        LOGE(TAG,"get misc partition faild");
        return -1;
    }

    if (info->length > reserve_size) {
        return info->length - reserve_size;
    } else {
        LOGE(TAG,"misc partion size is %d ,it should be larger than %d",info->length,reserve_size);
        return -1;
    }
    return 0;
}


int  dfu_get_fota_file_size(uint8_t* start_addr, uint32_t pos)
{
    int ret = 0;
    fota_head_info_t *info;
    if(pos == 0x00) { //flash
        uint8_t buffer[sizeof(fota_head_info_t)];
        ret = dfu_image_read(start_addr,0,sizeof(fota_head_info_t),buffer);
        if(ret < 0) {
            return -1;
        }
        info = (fota_head_info_t *)buffer;
        //check fota image head magic "IMGE"
        if (info->magic != 0x45474d49) {
            return -1;
        }
        return info->mnft_off + 284;
    } else {
        info = (fota_head_info_t *)start_addr;
        //check fota image head magic "IMGE"
        if (info->magic != 0x45474d49) {
            return -1;
        }
        return info->mnft_off + 284;
    }
}

static int version_char_to_u32(char* ver_str,uint32_t *version)
{
    int v1 = 0,v2 = 0,v3 = 0;
    if(*ver_str == 'v') {
        sscanf(ver_str, "v%d.%d.%d", &v1,&v2, &v3);
    } else {
        sscanf(ver_str, "%d.%d.%d", &v1,&v2, &v3);
    }
    *version = (v1 << 16 ) | (v2 << 8) | (v3);
    return 0;
}

int  get_version_str(uint32_t version,char* ver_str)
{
    if(!ver_str) {
        return -1;
    }
    uint8_t v1 = 0,v2 = 0,v3 = 0;
    v1 = (version & 0xFF0000) >> 16;
    v2 = (version & 0x00FF00) >> 8;
    v3=  version & 0x0000FF;
    sprintf(ver_str, "%d.%d.%d",v1,v2,v3);
    return 0;
}
uint32_t  dfu_get_fota_file_app_version(uint8_t * start_addr, uint32_t pos)
{
    int ret = 0;
    fota_head_info_t *info;
    uint32_t version_u32 = 0xffffffff;
    if(pos == 0x00) { //flash
        uint8_t buffer[sizeof(fota_head_info_t)];
        ret = dfu_image_read(start_addr,0,sizeof(fota_head_info_t),buffer);
        if(ret < 0) {
            return version_u32;
        }
        info = (fota_head_info_t *)buffer;
    } else {
        info = (fota_head_info_t *)start_addr;
    }
    if (info->magic != 0x45474d49) {
        return version_u32;
    }

    version_char_to_u32((char*)info->app_version,&version_u32);
    return version_u32;
}


int dfu_image_get_crc(uint8_t* address,uint32_t size, uint16_t *crc16, uint8_t data_type)
{
    uint8_t  read_buffer[256];
    uint8_t* addrtemp = read_buffer;
    uint32_t offset = 0;
    int ret = 0;
    uint32_t read_size = sizeof(read_buffer);

    while(read_size == sizeof(read_buffer)) {
        read_size = (size - offset) < sizeof(read_buffer) ? size - offset : sizeof(read_buffer);
        if(data_type == POS_FLASH) {
            ret = dfu_image_read(address,offset,read_size,read_buffer);
            if(ret) {
                return -1;
            }
        } else {
            addrtemp =  address + offset;
        }
        if(ret < 0 ) {
            return -1;
        }
        if(!offset) {
            *crc16 = util_crc16_ccitt(addrtemp, read_size, NULL);
        } else {
            *crc16 = util_crc16_ccitt(addrtemp, read_size, crc16);
        }
        offset += read_size;
    }
    LOGD(TAG,"Ota image crc calu success %04x",*crc16);
    return 0;
}


int sal_ota_is_allow_reboot()
{
    return 1;
}




