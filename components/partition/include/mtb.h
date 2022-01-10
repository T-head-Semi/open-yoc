/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef __MTB_H__
#define __MTB_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BMTB_MAGIC  0x74427254
#define MTB_MAGIC   0x74427251

#define INVALID_ADDR (~0)

#define PUBLIC_KEY_NAME_SIZE 8
#define MTB_MISC_NAME           "misc"
#define MTB_IMAGE_NAME_IMTB     "imtb"
#define MTB_IMAGE_NAME_PKEY     "pkey"
#define MTB_IMAGE_NAME_OTP      "otp"
#ifndef CONFIG_MTB_NAME_KP
#define MTB_IMAGE_NAME_KP       "kp"
#else
#define MTB_IMAGE_NAME_KP       CONFIG_MTB_NAME_KP
#endif

#define MAX_MANTB_BYTE_SIZE (1024 * 4)
#define MAX_MANTB_WORD_SIZE ((MAX_MANTB_BYTE_SIZE) >> 2)

#define PATH_BOOT_PRIM 0x1u
#define PATH_BOOT_RCVY 0x2u
#define PATH_BOOT_OTHE 0x3u

#define MTB_IMAGE_NAME_SIZE     8 /* image name length*/
#define MTB_OS_VERSION_LEN     32 /* for version 1.0, os version length is 32*/
#define MTB_OS_VERSION_LEN_V2  64 /* for version 2.0, os version length is 64*/
#define MTB_OS_VERSION_LEN_V4  64 /* for version 4.0, os version length is 64*/
#define MTB_IMAGE_VERSION_SIZE 12

#define MTB_KEY_BIT            1024
#define MTB_KEY_BYTE           (MTB_KEY_BIT >> 3)
#define MTB_KEY_N_SIZE         MTB_KEY_BYTE
#define MTB_KEY_E_SIZE         MTB_KEY_BYTE
#define MTB_KEY_D_SIZE         (MTB_KEY_BYTE * 2)
#define MTB_KEY_SIZE           (MTB_KEY_N_SIZE + MTB_KEY_E_SIZE)
#define MTB_ID2KEY_BYTE        16

#define SEACH_MODE_FIRST_TYPE 0
#define SEACH_MODE_SON_TYPE 1
#define SEACH_MODE_EXTEND_TYPE 2

#define V2_MANIFEST_TRUSKEY_VALUE 0
#define V2_MANIFEST_TRUSKEY_MODULS 1

#define UPDATE_TYPE_IMG_INFO 0
#define UPDATE_TYPE_OS_VERSIN 1

#define PARTITION_HEAD_MAGIC "CSKY"
#define PARTITION_TAIL_MAGIC "XBSR"

typedef enum {
    SCN_TYPE_IMG  = 0,
    SCN_TYPE_KEY  = 4,
    SCN_TYPE_PART = 5,
} scn_type_first_e;

typedef enum {
    SCN_TYPE_NORMAL = 0,
} scn_type_extend_e;

typedef enum { SCN_SUB_TYPE_IMG_NOR = 0, SCN_SUB_TYPE_IMG_SAFE = 1 } scn_sub_type_img_e;

typedef enum {
    SCN_SUB_TYPE_KEY_BOOT = 0,
} scn_sub_type_key_e;

typedef enum {
    SCN_SUB_TYPE_IMG_PART = 0,
} scn_sub_type_part_e;

typedef struct {
    uint16_t encrypted : 1;
    uint16_t reserve : 7;
    uint16_t update_flag : 1;
    uint16_t reserve2 : 7;
} tb_flag;

typedef struct {
    uint32_t magic;
    uint16_t version;
    tb_flag  flag;
    uint16_t scn_count;
    uint16_t digest_sch;
    uint16_t signature_sch;
    uint16_t size;
} mhead_tb;

typedef struct {
    uint8_t value[MTB_OS_VERSION_LEN];
} mtb_os_version_t;

typedef struct {
    uint8_t value[MTB_OS_VERSION_LEN_V2];
} mtb_os_version2_t;

typedef struct {
    uint8_t v[MTB_OS_VERSION_LEN_V4];
} mtb_os_version4_t;

typedef struct {
    uint8_t  image_name[MTB_IMAGE_NAME_SIZE];
    uint64_t static_addr;
    uint64_t loading_addr;
    uint32_t image_size;
    uint8_t  image_version[MTB_IMAGE_VERSION_SIZE];
} scn_img_sig_info_t;

typedef struct {
    uint8_t  image_name[MTB_IMAGE_NAME_SIZE];
    uint64_t part_addr;
    uint64_t part_end;
} scn_partion_info_t;

typedef struct {
    uint8_t n[MTB_KEY_N_SIZE];
    uint8_t e[MTB_KEY_E_SIZE];
} scn_key_info_t;

typedef struct {
    uint8_t extend_type : 4; //扩展标志
    uint8_t first_type : 4;  //type表述
} scn_father_type_t;

typedef struct {
    scn_father_type_t father_type;  //父类型
    uint8_t son_type;               //描述段子类型
} scn_type_t;

typedef struct {
    scn_type_t type;
    uint16_t   index;
    uint32_t   size;
} scn_head_t;

typedef struct {
    scn_head_t scn_head;
    union {
        scn_img_sig_info_t img[0];
        scn_partion_info_t partition[0];
        scn_key_info_t key[0];
    };
} scn_t;

typedef struct {
    scn_head_t head;                //8 BYTES
    scn_img_sig_info_t imginfo;     // 40 BYTES
    uint8_t signature[0];
} scn_img_t;

#define VALID_MANIFEST(mtb)                                                                        \
    (((mhead_tb *)mtb)->magic == MTB_MAGIC || ((mhead_tb *)mtb)->magic == BMTB_MAGIC)

#define MTB_MODE_CHECK_OTHER    1
#define MTB_MODE_SAFET_SAT      2

extern const scn_type_t g_scn_img_type[];
extern const scn_type_t g_scn_key_type[];
extern const scn_type_t g_scn_part_type[];

// imtb v4
typedef struct {
    uint32_t magic;
    uint16_t version;
    tb_flag  flag;
    uint16_t digest_sch;
    uint16_t signature_sch;
    char     pub_key_name[PUBLIC_KEY_NAME_SIZE];
    uint16_t partition_count;
    uint16_t size;
} imtb_head_v4_t;

typedef struct {
    char name[MTB_IMAGE_NAME_SIZE];
    char pub_key_name[PUBLIC_KEY_NAME_SIZE];
    scn_type_t  partition_type;
    uint16_t    block_count;
    uint32_t    block_offset;
    uint32_t    load_address;
    uint32_t    img_size;
} imtb_partition_info_v4_t;

typedef struct {
    char name[MTB_IMAGE_NAME_SIZE];
    char pub_key_name[PUBLIC_KEY_NAME_SIZE];
    uint32_t start_addr;
    uint32_t end_addr;
    uint32_t load_addr;
    uint32_t img_size;
    scn_type_t part_type;
} mtb_partition_info_t;

typedef struct {
    union {
        mhead_tb head;
        imtb_head_v4_t headv4;
    };
} mtb_head_t;

typedef struct {
    uint32_t jump;
    uint8_t magic[4];
    uint32_t size;
} partition_header_t;

typedef struct {
    uint8_t magic[4];
    uint8_t digestType;
    uint8_t signatureType;
    uint16_t rsv;
    uint8_t digest_data[0];
} partition_tail_head_t;

typedef struct {
    partition_tail_head_t tail_head;
    uint8_t *hash;
    uint8_t *signature;
} partition_tail_t;

typedef struct {
    unsigned long using_addr;    // 当前使用的imtb地址(RAM、FLASH)
    unsigned long prim_addr;     // 当前使用的imtb地址(FLASH)
    unsigned long backup_addr;   // 备份的imtb地址(FLASH)
    uint32_t one_size: 16;  // 一份imtb表(或者bmtb+imtb)占用的分区size
    uint32_t i_offset: 16;  // imtb表的偏移地址，当存在bmtb时才有值，否则为0
    uint16_t version;       // imtb表的版本号
    uint16_t cur_piece: 1;  // 当前使用的主imtb表还是备份imtb表，0：主表，1：备份表
    uint16_t rsv: 15;
} mtb_t;

#ifndef CONFIG_MULTI_FLASH_SUPPORT
#define CONFIG_MULTI_FLASH_SUPPORT 0
#endif

typedef struct {
    char image_name[MTB_IMAGE_NAME_SIZE];
    uint64_t part_addr;
    uint64_t part_size;
#if !defined(CONFIG_MANTB_VERSION) || (CONFIG_MANTB_VERSION > 3)
    uint64_t load_addr;
    uint32_t image_size;
#endif
#if CONFIG_MULTI_FLASH_SUPPORT
    scn_type_t type;        // Parameter reuse
    uint16_t rsv;
#endif
} sys_partition_info_t;

typedef struct {
    uint8_t img_name[MTB_IMAGE_NAME_SIZE];
    uint8_t *img_ver;
    uint32_t img_addr;
    uint32_t img_size;
    uint32_t img_part_size;
    uint8_t img_type;
} img_info_t;


/**
 * Init the manifest table
 *
 * @return  0: On success， otherwise is error
 */
int mtb_init(void);

/**
 * Get mtb_t handle
 *
 * @return  NULL: On failed, otherwise is mtb_t handle
 */
mtb_t *mtb_get(void);

/**
 * Get manifest table prim or backup address in flash
 *
 * @param[in]  is_prim         Indicate prim or backup, 0: backup addr, 1: prim addr
 *
 * @return  address in flash
 */
uint32_t mtb_get_addr(uint8_t is_prim);

/**
 * Get bmtb size
 *
 * @param[in]  data         The point to manifest table buffer
 *
 * @return  bmtb size
 */
uint32_t mtb_get_bmtb_size(const uint8_t *data);

/**
 * Get manifest size (imtb size or bmtb+imtb size), not partition size
 *
 * @return      size
 */
uint32_t mtb_get_size(void);

/**
 * Verify image by image name
 *
 * @param[in]  name         The point to image name buffer
 *
 * @return  0: On success， otherwise is error
 */
int mtb_image_verify(const char *name);

/**
 * Get partition information by name
 *
 * @param[in]  name         The point to image name buffer
 *
 * @param[in]  part_info    The point to partition information buffer
 *
 * @return  0: On success， otherwise is error
 */
int mtb_get_partition_info(const char *name, mtb_partition_info_t *part_info);

/**
 * Get image information by name
 *
 * @param[in]  name         The point to image name buffer
 *
 * @param[in]  img_info     The point to image information buffer
 *
 * @return  0: On success， otherwise is error
 */
int mtb_get_img_info(const char *name, img_info_t *img_info);


/**
 *  when mtb version < 4
 */
int get_section_buf(uint32_t addr, uint32_t img_len, scn_type_t *scn_type, uint32_t *scn_size);

/**
 *  when mtb version < 4
 */
int mtb_get_img_scn_addr(uint8_t *mtb_buf, const char *name, unsigned long *scn_addr);

/**
 * Read data from an area on a Flash to data buffer in RAM
 *
 * @param[in]  out          Pointer to partition buffer, see `sys_partition_info_t`
 * @param[in]  out_len      Partition buffer length,if info obtain successfully,
 *                          out_len is updated to actual partition buffer sizes
 *
 * @return  0 : On success, <0 If an error occurred with any step
 */
int get_sys_partition(uint8_t *out, uint32_t *out_len);

/**
 * Get the app version in manifest table
 *
 * @param[out]  out         The point to version buffer
 * @param[out]  out_len     The length of version string
 *
 * @return  0: On success， otherwise is error
 */
int get_app_version(uint8_t *out, uint32_t *out_len);


//// for tee
typedef struct {
    uint8_t *n;
    uint8_t *e;
    uint8_t *d;
    uint32_t size;
} key_info_t;

#define MTB_KEY_BIT            1024
#define MTB_KEY_BYTE           (MTB_KEY_BIT >> 3)
#define MTB_KEY_N_SIZE         MTB_KEY_BYTE
#define MTB_KEY_E_SIZE         MTB_KEY_BYTE
#define MTB_KEY_D_SIZE         (MTB_KEY_BYTE * 2)
#define MTB_KEY_SIZE           (MTB_KEY_N_SIZE + MTB_KEY_E_SIZE)

int mtb_get_img_info_buf(uint8_t *buf, uint32_t *size, char *name);
int mtb_get_key_info(key_info_t *info);
int mtb_get_pubkey_info(key_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* __MTB_H__ */

