/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include "mtb.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "mtb_log.h"
#include "yoc/partition_flash.h"
#include "yoc/partition.h"
#include "verify_wrapper.h"
#include "mtb_internal.h"
#include "misc.h"
#if (CONFIG_PARITION_NO_VERIFY == 0)
#include <key_mgr.h>
#endif
/****************************************************************************
* Definitions
****************************************************************************/
#define PUBLIC_KEY_VALUE_SIZE 128
#define PUBLIC_KEY_MODULE_SIZE 128
#define SCH_SIGNATURE_MAX_LEN 128

#define PADDING_SIZE(src_size,padd) (((src_size+padd-1)/padd)*padd)
#define DATA_TO_SIGN_SIZE           512

const scn_type_t g_scn_img_type[] = {
    {{SCN_TYPE_NORMAL, SCN_TYPE_IMG}, SCN_SUB_TYPE_IMG_NOR},
    {{SCN_TYPE_NORMAL, SCN_TYPE_IMG}, SCN_SUB_TYPE_IMG_SAFE}
};

const scn_type_t g_scn_key_type[] = {
    {{SCN_TYPE_NORMAL, SCN_TYPE_KEY}, SCN_SUB_TYPE_KEY_BOOT}
};

const scn_type_t g_scn_part_type[] = {
    {{SCN_TYPE_NORMAL, SCN_TYPE_PART}, SCN_SUB_TYPE_IMG_PART}
};

#define GET_SECTION_TYPE_ADDR(scn_addr) ((unsigned long)&(((scn_head_t *)scn_addr)->type))
#define GET_SECTION_TYPE_PTR(scn_addr)  ((scn_type_t *)(GET_SECTION_TYPE_ADDR(scn_addr)))
#define GET_SECTION_IMG_INFO_ADDR(scn_addr) ((unsigned long)((unsigned long)scn_addr+ sizeof(scn_head_t)))
#define GET_SECTION_IMG_INFO_PTR(scn_addr)  ((scn_img_sig_info_t *)(GET_SECTION_IMG_INFO_ADDR(scn_addr)))
/****************************************************************************
 * Functions
 ****************************************************************************/
static inline int padding_flash(unsigned long src)
{
    int flashid = 0;
    partition_flash_info_t flash_info;

#if CONFIG_MULTI_FLASH_SUPPORT
    flashid = get_flashid_by_abs_addr(src);
#endif
    void *handle = partition_flash_open(flashid);
    partition_flash_info_get(handle, &flash_info);
    partition_flash_close(handle);
    return PADDING_SIZE(src, flash_info.sector_size);
}

static inline int mtb_version(void)
{
    return mtb_get()->version;
}

static int veriy_signature_check(signature_sch_e sig_type, digest_sch_e digest_type , unsigned long key, uint32_t key_len,
                              unsigned long src, uint32_t src_size,
                              uint8_t* signature, uint32_t sig_size)
{
    int ret;
    uint32_t output_len;
    uint8_t output[SCH_SIGNATURE_MAX_LEN];

    ret = hash_calc_start(digest_type, (uint8_t *)src, src_size, output, &output_len, 0);
    if (ret != 0) {
        return -1;
    }
    ret = signature_verify_start(digest_type, sig_type, (uint8_t *)key, key_len, output, output_len, signature, sig_size);
    return ret;
}

/*
    get the first section offset
 * @param[in]    im_addr         bootrom manfest address
 * @param[out]     offset      offset
 * @return      0 on success; -1 on failure
 */
static int get_scn_offset(uint32_t im_addr, unsigned long *offset)
{
    unsigned long m_addr = mtb_get()->using_addr;
    uint32_t size_ver = 0;

    if (mtb_version() == 1) {
        size_ver = MTB_OS_VERSION_LEN;
    } else if (mtb_version() == 2) {
        size_ver = MTB_OS_VERSION_LEN_V2;
    } else if (mtb_version() == 4) {
        // there is no section in MTB V4
        MTB_LOGE("there is no section in MTB V4");
        return -1;
    } else {
        MTB_LOGE("ver e");
        return -1;
    }

    *offset = (unsigned long)(m_addr + sizeof(mhead_tb) + size_ver);
    return 0;
}

/*
    get the first type section sum
 * @param[in]    m_addr         bootrom manfest address
 * @param[in]     first_type      first_type
 * @return      count
 */
static uint32_t get_section_type_sum(unsigned long im_addr, uint8_t first_type)
{
    unsigned long m_addr = mtb_get()->using_addr;
    mhead_tb *mtb = (mhead_tb *)m_addr;
    scn_head_t *scn_head = NULL;

    if (get_scn_offset(m_addr, (unsigned long *)(&scn_head))) {
        return -1;
    }

    uint32_t sum_scn = mtb->scn_count;
    uint32_t i, count = 0;

    for (i = 0; i < sum_scn; i++) {
        if (scn_head->type.father_type.first_type == first_type) {
            count++;
        }

        scn_head = (scn_head_t *)(((unsigned long)scn_head) + scn_head->size);
    }

    return count;
}

/*
    get the  section address by type or name
 * @param[in]     seach_type      seach_type
 * @param[in]     seach_name      seach_name
 * @param[out]     scn_addr      scn_addr
 * @param[in]     mode          SEACH_MODE_FIRST_TYPE,SEACH_MODE_SON_TYPE or SEACH_MODE_EXTEND_TYPE
 * @return      0 on success; -1 on failure
 */
int get_section_addr(unsigned long im_addr, const scn_type_t *seach_type, uint8_t *seach_name, unsigned long *scn_addr, uint32_t mode)
{
    unsigned long m_addr = im_addr;
    mhead_tb *mtb = (mhead_tb *)m_addr;
    scn_head_t *scn_head = NULL;
    scn_img_sig_info_t *img_info_t;
    uint32_t i = 0;
    uint32_t sum_scn;
    uint8_t s_name[MTB_IMAGE_NAME_SIZE + 1];

    if (mtb == NULL) {
        return -EINVAL;
    }
    sum_scn = mtb->scn_count;
    memset(s_name, 0, sizeof(s_name));

    if (seach_name != NULL) {
        memcpy(s_name, seach_name, MTB_IMAGE_NAME_SIZE);
    }

    if (get_scn_offset(m_addr, (unsigned long *)(&scn_head))) {
        return -1;
    }

    if (NULL != seach_name) {
        MTB_LOGD("seach_name:%.*s,seach_type:%d,mode:%d", MTB_IMAGE_NAME_SIZE, seach_name, seach_type->father_type.first_type , mode);
    }

    for (; i < sum_scn ; i++) {
        //MTB_LOGD("get_section_addr:%x",(uint32_t)scn_t);
        if (scn_head->type.father_type.first_type == seach_type->father_type.first_type) {
            if (mode == SEACH_MODE_FIRST_TYPE || ((mode == SEACH_MODE_SON_TYPE) && scn_head->type.son_type == seach_type->son_type) ||
                (mode == SEACH_MODE_EXTEND_TYPE && scn_head->type.son_type == seach_type->son_type && scn_head->type.father_type.extend_type == seach_type->father_type.extend_type)) {
                if (seach_name == NULL) {
                    goto succ;
                } else {
                    img_info_t = GET_SECTION_IMG_INFO_PTR(scn_head);

                    if (!strncmp((char *)(img_info_t->image_name), (char *)s_name, MTB_IMAGE_NAME_SIZE)) {
                        goto succ;
                    }

                    MTB_LOGD("seach_type:%d,name:%.*s,s_name:%s", seach_type->father_type.first_type, MTB_IMAGE_NAME_SIZE, (char *)img_info_t->image_name, s_name);
                }
            }
        }

        scn_head = (scn_head_t *)(NEXT_SECTION(scn_head));
    }

    MTB_LOGD("not seach_type:%d", seach_type->father_type.first_type);
    return -1;
succ:
    MTB_LOGD("s scn_t:0x%lx", (unsigned long)scn_head);
    *scn_addr = (unsigned long)scn_head;
    return 0;
}

/**
 * get key info by key type
 *
 * @param[in]       address     mtb address
 * @param[in]       key_type    key name
 * @param[out]      key_addr    key store address
 * @param[out]      key_size    key size
 * @return      0 suc ; -1 on failure
 */
static int get_key_info(unsigned long im_addr, uint8_t key_type, unsigned long *key_addr, uint32_t *key_size)
{
    unsigned long m_addr = mtb_get()->using_addr;
    mhead_tb *mtb = (mhead_tb *)m_addr;

    if (!(key_addr && key_size)) {
        MTB_LOGE("get key info e.");
        return -EINVAL;
    }
    *key_size = 0;

    MTB_LOGD("seach key_type:%d", key_type);
    if (mtb->version == 1) {
        // scn_type_t *section_type;
        scn_head_t *scn_keyinfo_addr;
        if (key_type > SCN_SUB_TYPE_KEY_BOOT) {
            MTB_LOGE("e key_type");
            return -1;
        }

        if (get_section_addr(m_addr, (const scn_type_t *)&g_scn_key_type[key_type], NULL,
                             (unsigned long *)(&scn_keyinfo_addr), SEACH_MODE_EXTEND_TYPE)) {
            MTB_LOGE("not find %d key", key_type);
            return -1;
        }

        // section_type = GET_SECTION_TYPE_PTR(scn_keyinfo_addr);

        // key_info->key_type =  section_type->son_type;
        *key_addr = (unsigned long)(scn_keyinfo_addr) + sizeof(scn_head_t);
        *key_size =  scn_keyinfo_addr->size - sizeof(scn_head_t);
    }
    else if (mtb->version == 2) {

    }
    else if (mtb->version == 4) {
        // TODO:
    }
    else {
        MTB_LOGE("e m v");
        return -1;
    }

    return 0;
}

/**
 * get part info by scn_partion and name
 *
 * @param[in]   scn_partion      partion address
 * @param[in]   seach_name         partion name
 * @param[out]   addr           partion  size
 * @param[out]   size           partion  size
 * @return      0 suc ; -1 on failure
 */
static int get_part_info_sub(unsigned long scn_partion, uint8_t *seach_name, uint32_t *addr, uint32_t *size)
{
    uint32_t part_num;
    uint32_t i = 0;

    uint32_t name_len;
    uint8_t s_name[MTB_IMAGE_NAME_SIZE];
    name_len = strlen((const char *)seach_name);

    if (name_len > MTB_IMAGE_NAME_SIZE) {
        name_len = MTB_IMAGE_NAME_SIZE;
    }

    memset(s_name, 0, sizeof(s_name));
    memcpy(s_name, seach_name, name_len);

    MTB_LOGD("seach scn_partion_name:%.*s", MTB_IMAGE_NAME_SIZE, seach_name);
    scn_partion_info_t *part_info = (scn_partion_info_t *)(scn_partion + sizeof(scn_head_t));
    part_num = (((scn_head_t *)scn_partion)->size - sizeof(scn_head_t)) / sizeof(scn_partion_info_t);

    for (; i < part_num; i++) {
        if (!strncmp((char *)(part_info[i].image_name), (char *)(s_name), MTB_IMAGE_NAME_SIZE)) {
            if (mtb_version() == 1) {
                *size = (uint32_t)(part_info[i].part_end);
            } else if (mtb_version() == 2) {
                *size = (uint32_t)(part_info[i].part_end - part_info[i].part_addr);
            } else if (mtb_version() == 4) {
                // TODO:
            }
            *addr = (uint32_t)(part_info[i].part_addr);
            return 0;
        }
    }

    return -1;
}

/**
 * get part info by scn_partion and name
 *
 * @param[in]   name         partion name
 * @param[out]   addr           partion  size
 * @param[out]   size           partion  size
 * @return      0 suc ; -1 on failure
 */
int mtbv_get_part_info(uint8_t *name, uint32_t *part_addr, uint32_t *part_size)
{
    uint32_t im_addr;
    unsigned long scn_partion;

    im_addr = mtb_get()->using_addr;
    if (GET_PARTION_SCN_ADDR(im_addr, &scn_partion)) {
        MTB_LOGE("no scnpart");
        return -1;
    }

    if (get_part_info_sub(scn_partion, name, part_addr, part_size)) {
        MTB_LOGE("e seach scn_partion_name:%.*s", MTB_IMAGE_NAME_SIZE, name);
        return -1;
    }

    return 0;
}

int mtbv_get_img_info(const char *name, img_info_t *img_info)
{
    // FIXME: NON_ADDRESS_FLASH
    scn_type_t *section_type;
    scn_img_sig_info_t *scn_img_info_t;
    unsigned long scn_imginfo_addr;
    uint32_t part_size = 0;
    uint32_t part_addr = 0;
    MTB_LOGD("seach imgname:%.*s", MTB_IMAGE_NAME_SIZE, name);

    if (mtbv_get_part_info((uint8_t *)name, &part_addr, &part_size)) {
        return -1;
    }

    if (get_section_addr(mtb_get()->using_addr, (const scn_type_t *)&g_scn_img_type[SCN_TYPE_IMG], (uint8_t *)name, &scn_imginfo_addr, SEACH_MODE_FIRST_TYPE)) {
        MTB_LOGE("not find %.*s imfo", MTB_IMAGE_NAME_SIZE, name);
        return -1;
    }

    section_type = GET_SECTION_TYPE_PTR(scn_imginfo_addr);

    scn_img_info_t = GET_SECTION_IMG_INFO_PTR(scn_imginfo_addr);
    strncpy((char *)(img_info->img_name), (char *)(scn_img_info_t->image_name), sizeof((img_info->img_name)));
    img_info->img_name[sizeof((img_info->img_name)) - 1] = '\0';
    img_info->img_addr =  scn_img_info_t->loading_addr;
    img_info->img_size =  scn_img_info_t->image_size;
    img_info->img_type =  section_type->son_type;

    img_info->img_part_size = (uint32_t)part_size - ((uint32_t)(scn_img_info_t->static_addr) - (uint32_t)part_addr);
    return 0;
}

/**
 * check img siginature
 *
 * @param[in]   scn_img_addr       img section address
 * @param[in]   scn_key_addr         key section address
 * @param[in]   sig_type      sig_type
 * @param[in]   dig_type         dig_type
 * @return      0 suc ; -1 on failure
 */
int get_img_check_result(unsigned long scn_img_addr, unsigned long key_addr, uint32_t key_size, signature_sch_e sig_type, digest_sch_e dig_type)
{
    uint8_t section_type = ((scn_head_t *)scn_img_addr)->type.father_type.first_type;
    uint8_t section_extend = ((scn_head_t *)scn_img_addr)->type.father_type.extend_type;
    uint8_t data_to_sign[DATA_TO_SIGN_SIZE];

    if (section_type != SCN_TYPE_IMG && section_extend != SCN_TYPE_NORMAL) {
        return -1;
    }

    scn_img_sig_info_t *img_info_t = GET_SECTION_IMG_INFO_PTR(scn_img_addr);
    unsigned long img_buf = img_info_t->static_addr;
    uint32_t img_len = img_info_t->image_size;

    uint8_t *img_info_buf = (uint8_t *)scn_img_addr;
    uint32_t img_info_len = sizeof(scn_head_t) + sizeof(scn_img_sig_info_t);

    uint8_t *sig_buf = (uint8_t *)img_info_t + sizeof(scn_img_sig_info_t);
    uint32_t sig_len = ((scn_head_t *)scn_img_addr)->size - img_info_len;

    uint32_t data_to_sign_len = 0;
    memset(data_to_sign, 0, DATA_TO_SIGN_SIZE);
    memcpy(data_to_sign, img_info_buf, img_info_len);

    hash_calc_start(dig_type, (uint8_t *)img_buf, img_len, data_to_sign + img_info_len, &data_to_sign_len, 0);

    data_to_sign_len += img_info_len;


    return veriy_signature_check(sig_type, dig_type, key_addr, key_size,
                                (unsigned long)data_to_sign, data_to_sign_len, sig_buf, sig_len);
}

/**
 * check mtb siginature and set safe region
 *
 * @param[in]   im_addr       bootrom manifest address
 * @param[in]   mode         MTB_MODE_CHECK_RCVY,MTB_MODE_CHECK_OTHER or MTB_MODE_SAFET_SAT
 * @param[in]   cb           safe flash section set call back function
 * @param[in]   dig_type         dig_type
 * @return      0 suc ; -1 on failure
 */
int mtbv_check_imginfo(unsigned long im_addr, uint32_t mode)
{
    unsigned long m_addr = mtb_get()->using_addr;
    uint32_t image_sum = get_section_type_sum(im_addr, SCN_TYPE_IMG);
    uint32_t i;
    unsigned long scn_img_addr;
    unsigned long key_addr = 0;
    uint32_t key_size;

    signature_sch_e sig_type = ((mhead_tb *)m_addr)->signature_sch;
    digest_sch_e dig_type = ((mhead_tb *)m_addr)->digest_sch;
    MTB_LOGD("check_ mtb_imginfo:%d,image_sum:%d", mode, image_sum);

    if (((mhead_tb *)m_addr)->magic != MTB_MAGIC) {
        MTB_LOGE("e magic");
        return -1;
    }

    MTB_LOGD("seach key");

    if (get_key_info(im_addr, SCN_SUB_TYPE_KEY_BOOT, &key_addr, &key_size)) {
        MTB_LOGE("e got key");
        return -1;
    }

    MTB_LOGD("seach first scn");

    if (GET_FIRST_IMG_SCN_ADDR(im_addr, &scn_img_addr)) {
        return -1;
    }

    for (i = 0; i < image_sum; i++) {
        MTB_LOGD("scn_img_addr:%lx", scn_img_addr);
		MTB_LOGD("son_type:%d", (uint8_t)(GET_SECTION_TYPE_PTR(scn_img_addr)->son_type));

        if (mode == MTB_MODE_CHECK_OTHER) {
            MTB_LOGD("check imgname:%.*s,image_size:%d,static_addr:%x",
                        MTB_IMAGE_NAME_SIZE, (char *)(GET_SECTION_IMG_INFO_PTR(scn_img_addr)->image_name),
                        GET_SECTION_IMG_INFO_PTR(scn_img_addr)->image_size,
                        (uint32_t)(GET_SECTION_IMG_INFO_PTR(scn_img_addr)->static_addr)
                        );

            if (get_img_check_result(scn_img_addr, key_addr, key_size, sig_type, dig_type)) {
                MTB_LOGE("e check img:%.*s", MTB_IMAGE_NAME_SIZE, (char *)(GET_SECTION_IMG_INFO_PTR(scn_img_addr)->image_name));
                return -1;
            }
        }
#if 0 // FIXME:
        else if (mode == MTB_MODE_SAFET_SAT
			&& GET_SECTION_TYPE_PTR(scn_img_addr)->father_type.first_type == SCN_TYPE_IMG
			&& GET_SECTION_TYPE_PTR(scn_img_addr)->son_type != SCN_SUB_TYPE_IMG_NOR
			&& cb) {
            MTB_LOGD("set safe imgname:%.*s,image_size:%d,static_addr:%x", \
                      MTB_IMAGE_NAME_SIZE, (char *)(GET_SECTION_IMG_INFO_PTR(scn_img_addr)->image_name), \
                      GET_SECTION_IMG_INFO_PTR(scn_img_addr)->image_size, \
                      (uint32_t)(GET_SECTION_IMG_INFO_PTR(scn_img_addr)->static_addr) \
                     );

            if (cb(GET_SECTION_IMG_INFO_PTR(scn_img_addr)->static_addr, GET_SECTION_IMG_INFO_PTR(scn_img_addr)->image_size)) {
                MTB_LOGE("e set safe img:%.*s", MTB_IMAGE_NAME_SIZE, (char *)(GET_SECTION_IMG_INFO_PTR(scn_img_addr)->image_name));
                return -1;
            }
        }
#endif
        scn_img_addr = NEXT_SECTION(scn_img_addr);
    }
#if 0
    // FIXME:
	// set MTB partition to safe region
    if (mode == MTB_MODE_SAFET_SAT && cb) {
        if (cb(mtb_get_addr(1), get_manifest_size(mtb_get_addr(1), MANIFEST_SIZE_MODE()))) {
            MTB_LOGE("e set safe img.manfest:%x", scn_img_addr);
            return -1;
        }

        if (cb(mtb_get_addr(0), get_manifest_size(mtb_get_addr(1), MANIFEST_SIZE_MODE()))) {
            MTB_LOGE("e set safe img.manfest:%x", scn_img_addr);
            return -1;
        }
    }
#endif
    return 0;
}

int mtbv_init(void)
{
    mtb_t *mtb = mtb_get();
    uint32_t part_addr, part_size;

    if (mtbv_get_part_info((uint8_t *)MTB_IMAGE_NAME_IMTB, &part_addr, &part_size)) {
        return -1;
    }

    mtb->one_size = part_size >> 1;
    if (mtb->prim_addr == part_addr) {
        mtb->backup_addr = part_addr + mtb->one_size;
    } else {
        mtb->backup_addr = part_addr;
    }
    return 0;
}

int mtbv_image_verify(const char *name)
{
    mtb_t *mtb = mtb_get();

    // FIXME:
    mtbv_check_imginfo(mtb->using_addr, MTB_MODE_CHECK_OTHER);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
typedef enum {
    S_FARTHER_TYPE = 0,
    S_SON_TYPE,
} search_mode_e;

#define IMG_SCN_HEAD_TYPE       (&(scn_type_t){{SCN_TYPE_NORMAL, SCN_TYPE_IMG }, SCN_SUB_TYPE_IMG_NOR })
#define KEY_SCN_HEAD_TYPE       (&(scn_type_t){{SCN_TYPE_NORMAL, SCN_TYPE_KEY }, SCN_SUB_TYPE_KEY_BOOT})
#define SCN_CONTENT_SIZE(scn) (scn->scn_head.size - sizeof(scn_head_t))
#define NEXT_SCN(scn)   (scn = (scn_t *)((uint8_t *)scn + scn->scn_head.size))
#define IMG_SCN(name)   _get_section(S_SON_TYPE, IMG_SCN_HEAD_TYPE, name)
#define KEY_SCN         _get_section(S_SON_TYPE, KEY_SCN_HEAD_TYPE, NULL)
#define PART_SCN        _get_section(S_SON_TYPE, PARTITION_SCN_HEAD_TYPE, NULL)

static scn_t *_get_section(search_mode_e mode, scn_type_t *type, const char *img_name)
{
    int i;
    scn_t *scn;
    mtb_head_t *head;
    unsigned long mtb_head = mtb_get()->using_addr;

    if (mtb_version() == 0x01) {
        scn = (scn_t *)(mtb_head + sizeof(mtb_head_t) + sizeof(mtb_os_version_t));
    } else if (mtb_version() == 0x02) {
        scn = (scn_t *)(mtb_head + sizeof(mtb_head_t) + sizeof(mtb_os_version2_t));
    } else if (mtb_version() == 0x04) {
        return NULL;
    } else {
        MTB_LOGE("mtb ver err, mtb_addr %lx\n", mtb_head);
        return NULL;
    }
    head = (mtb_head_t*)mtb_head;
    for (i = 0; i < head->head.scn_count; i++) {
        if ((mode == S_FARTHER_TYPE && type->father_type.first_type == scn->scn_head.type.father_type.first_type)
            || (mode == S_SON_TYPE && type->father_type.first_type == scn->scn_head.type.father_type.first_type
                && type->son_type == scn->scn_head.type.son_type)) {
            if (NULL != img_name) {
                if (!strncmp(img_name, (const char *)scn->img->image_name, MTB_IMAGE_NAME_SIZE)) {
                    MTB_LOGI("find scn 0x%lx\n", (unsigned long)scn);
                    return scn;
                }
            } else {
                MTB_LOGI("find scn 0x%lx\n", (unsigned long)scn);
                return scn;
            }
        }

        NEXT_SCN(scn);
    }

    return NULL;
}

#if (CONFIG_PARITION_NO_VERIFY == 0)
#if (CONFIG_TB_KP == 0)
const uint8_t test_key_n[] = {
    0xB9, 0xC2, 0x6A, 0x88, 0x90, 0xB0, 0xE1, 0x1B,
    0xD3, 0x3F, 0x93, 0x99, 0xF8, 0xCB, 0xC4, 0xF2,
    0x88, 0xB8, 0x56, 0x9A, 0x93, 0x24, 0x0B, 0x4A,
    0x75, 0xB4, 0x8D, 0x6C, 0x32, 0x40, 0x52, 0xA7,
    0x3D, 0x2B, 0x37, 0xCE, 0x57, 0x80, 0xFD, 0xA5,
    0x08, 0xC7, 0x4A, 0x77, 0x94, 0x34, 0xBA, 0x56,
    0x9F, 0xBA, 0x2E, 0xBA, 0x20, 0x1E, 0x80, 0x4A,
    0x09, 0x40, 0x07, 0xF5, 0x1D, 0xB6, 0xFC, 0x8E,
    0x5C, 0x4E, 0x02, 0x01, 0xD5, 0x97, 0x14, 0x83,
    0x2A, 0x32, 0x90, 0x4E, 0xED, 0xDC, 0x6F, 0xD6,
    0x0A, 0x1F, 0x44, 0x6A, 0xB1, 0xFE, 0x42, 0xBD,
    0xBF, 0xD7, 0x6F, 0xB5, 0x9B, 0x78, 0x5F, 0x19,
    0x07, 0x15, 0x4E, 0x65, 0xD2, 0x72, 0x09, 0x0C,
    0x14, 0xAD, 0xE8, 0xC7, 0x76, 0x25, 0xE8, 0x94,
    0xF8, 0xEE, 0x19, 0x93, 0xDF, 0xC9, 0x34, 0xEE,
    0x42, 0x1A, 0xA6, 0xA8, 0x70, 0xE2, 0xC0, 0x99,
};

const uint8_t test_key_d[] = {
    0x0D, 0x59, 0xA1, 0x9A, 0x6A, 0xFB, 0xAB, 0x51,
    0xFA, 0x9E, 0xA9, 0x16, 0xF9, 0xC0, 0x7B, 0x49,
    0x0A, 0xF6, 0x64, 0x92, 0xFB, 0x5C, 0x2D, 0xCB,
    0x8E, 0x26, 0x09, 0xBB, 0xEA, 0xE8, 0xA0, 0x88,
    0x10, 0x67, 0x58, 0xBD, 0x65, 0xA0, 0x89, 0x9A,
    0x20, 0xC5, 0x61, 0xDE, 0xB2, 0x32, 0x0F, 0xEA,
    0x45, 0xD4, 0x75, 0xA3, 0x3E, 0xC7, 0xB2, 0x64,
    0x9F, 0xC1, 0x90, 0x50, 0xD7, 0x6F, 0xDD, 0x3A,
    0xA6, 0xA3, 0x52, 0xED, 0xB6, 0xFC, 0x2B, 0x61,
    0xA5, 0xDB, 0xE4, 0xC7, 0xF8, 0x21, 0x30, 0xEF,
    0x7F, 0x21, 0x1E, 0x5B, 0x96, 0x45, 0x43, 0x4B,
    0x74, 0x98, 0xC6, 0x1B, 0xD9, 0x66, 0x0A, 0x64,
    0x41, 0x5F, 0xF6, 0xE0, 0x87, 0xD6, 0xFA, 0x00,
    0x4D, 0x76, 0x9B, 0x4E, 0x36, 0xC5, 0x8A, 0x10,
    0x8C, 0x61, 0xA1, 0x41, 0xB0, 0x8B, 0x25, 0xEC,
    0x19, 0x25, 0x89, 0x45, 0x8E, 0xFB, 0xCF, 0xB9,
};
#endif
const uint8_t test_key_e[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
};

int mtb_get_key_info(key_info_t *info)
{
    int ret;
    uint32_t key_addr;
    uint32_t size;

    if (info == NULL) {
        return -EINVAL;
    }

    memset(info, 0, sizeof(key_info_t));
    ret = km_get_key(KEY_ID_PUBK_TB, &key_addr, &size);
    if (ret != 0) {
        return -1;
    }

    if (size >= MTB_KEY_N_SIZE) {
        info->d = (uint8_t *)key_addr;
        km_get_key(KEY_ID_PUBK_TB, &key_addr, &size);
        info->n = (uint8_t *)key_addr;
        info->e = (uint8_t *)test_key_e;
        info->size = MTB_KEY_SIZE;
    } else if (size >= MTB_ID2KEY_BYTE) {
        info->d = (uint8_t *)key_addr;
        info->size = size;
    } else {
        scn_t *scn = KEY_SCN;

        if (scn) {
            info->d = NULL;
            info->n = scn->key->n;
            info->e = scn->key->e;
            info->size = SCN_CONTENT_SIZE(scn);
        } else {
            return -1;
        }
    }

    return 0;
}

int mtb_get_pubkey_info(key_info_t *info)
{
    int ret;
    uint32_t key_addr;
    uint32_t size;

    if (info == NULL) {
        return -EINVAL;
    }

    memset(info, 0, sizeof(key_info_t));
    ret = km_get_key(KEY_ID_PUBK_TB, &key_addr, &size);
    if (ret != KM_OK) {
        return -1;
    }

    if (size >= MTB_KEY_N_SIZE) {
        info->n = (uint8_t *)key_addr;
        info->e = (uint8_t *)test_key_e;
        info->size = MTB_KEY_SIZE;
    } else {
        scn_t *scn = KEY_SCN;

        if (scn) {
            info->n = scn->key->n;
            info->e = scn->key->e;
            info->size = SCN_CONTENT_SIZE(scn);
        } else {
            return -1;
        }
    }

    return 0;
}
#endif /* CONFIG_PARITION_NO_VERIFY */

int mtb_get_img_info_buf(uint8_t *buf, uint32_t *size, char *name)
{
    if (!(buf && size && name)) {
        return -EINVAL;
    }

    *size = 0;

    if (mtb_version() < 4) {
        scn_t *scn = IMG_SCN(name);
        if (scn) {
            memcpy(buf, (uint8_t *)scn->img, SCN_CONTENT_SIZE(scn));
            *size = SCN_CONTENT_SIZE(scn);
            return 0;
        }
    } else {
        mtb_partition_info_t info;
        scn_img_sig_info_t out_info;

        memset(&out_info, 0, sizeof(scn_img_sig_info_t));
        if (mtb_get_partition_info(name, &info) == 0) {
            memcpy(out_info.image_name, info.name, MTB_IMAGE_NAME_SIZE);
            out_info.static_addr = info.start_addr;
            out_info.loading_addr = info.load_addr;
            out_info.image_size = info.img_size;
            memcpy(buf, &out_info, sizeof(scn_img_sig_info_t));
            *size = sizeof(scn_img_sig_info_t);
            return 0;
        }
    }
    return -1;
}
