/**
* Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
*/
#ifndef CONFIG_KEY_MGR_NO_PUB_PARTITION

#include "key_mgr_pub_key.h"
#include "key_mgr.h"
#include "key_mgr_port.h"
#include "key_mgr_log.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef CONFIG_CHIP_PANGU
#include "key_mgr_efuse.h"
#endif
#include "yoc/partition.h"
#include "verify_wrapper.h"
#if CONFIG_TB_KP > 0
#if (CONFIG_KEY_FROM_OTP > 0)
#include "tee_addr_map.h"
#endif
#endif
#include <soc.h>

// Key type. Fixme, this need be confirmed by server
#define KEY_TYPE_MNF_TB_V3_ADDR 12
#define KEY_TYPE_PUBK_TB_MOD 8
#define KEY_TYPE_LPMRECOVERY 7
#define KEY_TYPE_SOCCONFIG 6
#define KEY_TYPE_ID2PRVK 5
#define KEY_TYPE_ID2ID 4
#define KEY_TYPE_PUBK_TB 3
#define KEY_TYPE_JTAG_KEY 2
#define KEY_TYPE_MNF_TB_ADDR 1

#define KP_HASH_REGSION_SIZE 48

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t reserve;
    uint16_t key_num;
    uint16_t digest_scheme;
    uint32_t total_size; //exclude new adding publickey region
} manifest_idx_head_t;

/* Type of key or data */
typedef struct {
    uint16_t type : 1;      // Key or data
    uint16_t key_sche : 15; // Reserve if type is data
} key_flag;

/* Key meta data receive from hosttool */
typedef struct {
    uint16_t type;
    uint16_t size;
    key_flag flags;
    uint16_t reserve;
    uint32_t offset;
    uint32_t rsv2;
} key_meta_t;

/* The primary manifest.idx from hosttool */
typedef struct {
    manifest_idx_head_t head;
    key_meta_t          key_metas[0];
} manifest_idx_t;

/* Head of manifest.idx in flash */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t key_num;
} manifest_idx1_head_real;

/* Position info */
typedef struct {
    uint32_t info : 28;
    uint32_t type : 4;
} position_info_t;

/* Real key meta data in flash */
typedef struct {
    uint16_t        type;
    uint16_t        size;
    key_flag        flags;
    uint16_t        reserve;
    position_info_t position_info;
} key_meta1_real;

// pubkey region
typedef struct {
    uint32_t magic;
    uint32_t total_size;
} otp_pub_key_head_t;

// store in otp partition
typedef struct {
    char     name[PUBLIC_KEY_NAME_SIZE]; //公钥名字
    uint32_t type;                       //otp_pubkey_type_e
    uint8_t  data[0];                    // public key data
    // uint32_t data_size;     // public key data size
} otp_pub_key_store_t;

// key partition
typedef struct {
    uint32_t magic;
    uint32_t total_size; //pkey分区所有实际数据的大小
} pkey_head_t;

typedef struct {
    char     name[PUBLIC_KEY_NAME_SIZE];
    uint16_t type;
    uint16_t rsv;
    uint8_t  data[0];
    // uint32_t size;
} pkey_t;

typedef struct {
    char     name[PUBLIC_KEY_NAME_SIZE];
    uint8_t  key[MAX_PUB_KEY_SIZE];
    uint16_t key_size;
    uint16_t invalid: 1;
    uint16_t rsv: 15;
} pk_info_t;

typedef struct {
    uint32_t key_num;
    pk_info_t pk_info[MAX_PUB_KEY_NUM];
} pub_key_info_t;

#if defined(CONFIG_CHIP_CH2201)

uint32_t km_pub_key_init(void)
{
    return 0;
}

#else

static pub_key_info_t g_pub_key_info;

static int get_pub_key_from_pkey_part(const char *pub_key_name, unsigned long *key_addr, uint32_t *key_len, int *index)
{
    int i;

    if (!(pub_key_name && key_addr && key_len)) {
        return -EINVAL;
    }

    i = 0;
    while (i < g_pub_key_info.key_num) {
        pk_info_t *pk_info = &g_pub_key_info.pk_info[i];
        if (strncmp(pub_key_name, pk_info->name, sizeof(pk_info->name)) == 0
            && pk_info->invalid == 0) {
            *key_addr = (unsigned long)pk_info->key;
            *key_len = pk_info->key_size;
            if (index)
                *index = i;
            return 0;
        }
        i++;
    }

    return KM_ERR;
}

static int pkey_part_init(uint32_t part_start, uint32_t part_end)
{
    int i;
    int size, body_size;
    unsigned long offset;
    pkey_t * pkey;

    KM_LOGD("pk init start:0x%x,end:0x%x", part_start, part_end);
    if (part_start != INVALID_ADDR) {
        size = part_end - part_start;

#if defined(CONFIG_PKEY_IN_OTP) && (CONFIG_TB_KP > 0)
        uint8_t buf[CONFIG_OTP_BANK_SIZE];
        int32_t len;

        size = CONFIG_OTP_BANK_SIZE;
        if ((len = efusec_read_bytes(part_start, buf, size)) <= 0) {
            KM_LOGE("efuse read e..");
            goto err;
        }
        KM_LOGD("efuse read data len:%d", len);
        // dump_data(buf, len);
#else
        uint8_t buf[MAX_PUB_KEY_NUM * sizeof(pkey_t) + sizeof(pkey_head_t)];
        if (get_data_from_addr(part_start, buf, size)) {
            goto err;
        }
#endif
        pkey_head_t *head = (pkey_head_t *)buf;
        if (head->magic != PKEY_PART_MAGIC) {
            KM_LOGE("pkey part magic e[0x%08x]", head->magic);
            goto err;
        }
        if (head->total_size > size) {
            KM_LOGE("pkey data size e, %d", head->total_size);
            goto err;
        }
        size = head->total_size;
        body_size = size - sizeof(pkey_head_t);
        if (body_size < 0) {
            KM_LOGE("pkey body_size e, %d", body_size);
            goto err;
        }

        KM_LOGD("pkey real size:0x%x", size);
        pkey = (pkey_t *)((unsigned long)buf + sizeof(pkey_head_t));
        i = 0;
        while (i < body_size) {
            int data_len = get_length_with_signature_type(pkey->type);
            pk_info_t pk_info;
            KM_LOGD("%s,type:%d, data_len:%d", pkey->name, pkey->type, data_len);
            offset = (unsigned long)(pkey + 1);
            strncpy(pk_info.name, pkey->name, PUBLIC_KEY_NAME_SIZE);
            memcpy(pk_info.key, (uint8_t *)offset, MIN(data_len, sizeof(pk_info.key)));
            pk_info.key_size = data_len;
            g_pub_key_info.pk_info[g_pub_key_info.key_num++] = pk_info;
            if (g_pub_key_info.key_num >= MAX_PUB_KEY_NUM) {
                KM_LOGW("pubkey num > %d", MAX_PUB_KEY_NUM);
                break;
            }
            pkey = (pkey_t *)((uint8_t *)pkey + data_len + sizeof(pkey_t));
            i += data_len + sizeof(pkey_t);
        }
        return 0;
    }

err:
    return KM_ERR;
}

#if CONFIG_TB_KP > 0
static int get_length_with_type(otp_pubkey_type_e type)
{
    switch (type) {
        default:
            break;
        case OTP_HASH_SHA1:
            return 20;
        case OTP_HASH_MD5:
            return 16;
        case OTP_HASH_SHA224:
            return 28;
        case OTP_HASH_SHA256:
            return 32;
        case OTP_HASH_SHA384:
            return 48;
        case OTP_HASH_SHA512:
            return 64;
        case OTP_RSA1024:
            return 128;
        case OTP_RSA2048:
            return 256;
        case OTP_ECC256: // FIXME:
            return 0;
        case OTP_POINT:
            return 4;
    }
    return 0;
}

static int kp_check(uint8_t *kp_data)
{
    manifest_idx_t *kp          = (manifest_idx_t *)kp_data;
    int             total_size  = kp->head.total_size;
    int             key_num     = kp->head.key_num;
    int             ds          = kp->head.digest_scheme;
    int             hash_offset = key_num * sizeof(key_meta_t) + sizeof(manifest_idx_head_t);
    uint8_t         hash[32];
    uint8_t         buf[CONFIG_OTP_BANK_SIZE];
    uint32_t        olen;

    KM_LOGD("total_size:%d, key_num:%d, ds:%d, hash_offset:0x%x", total_size, key_num, ds, hash_offset);

    if (kp->head.magic != MANIFEST_IDX_MAGIC) {
        KM_LOGE("otp f maigc e");
        return KM_ERR;
    }
    if (0x03 != kp->head.version) {
        KM_LOGE("kp version is not 0x03");
        return KM_ERR;
    }
    if (total_size > CONFIG_OTP_BANK_SIZE) {
        KM_LOGE("kp size:%d is more than opt size:%d", total_size, CONFIG_OTP_BANK_SIZE);
        return KM_ERR;
    }
    KM_LOGD("kp size:%d is more than opt size:%d", total_size, CONFIG_OTP_BANK_SIZE);
    memcpy(buf, kp_data, total_size);
    // clear hash region
    memset(buf + hash_offset, 0, KP_HASH_REGSION_SIZE);
    if (hash_calc_start(ds, (const uint8_t *)buf, total_size, hash, &olen, 1) != 0) {
        KM_LOGE("kp hash calc e.");
        return KM_ERR;
    }
#if 0
    for (int i = 0; i < total_size; i++) {
        printf("%02x ", buf[i]);
        if (((i + 1) % 16) == 0) {
            printf("\n");
        }
    }
    printf("\n");
#endif
    if (memcmp(kp_data + hash_offset, hash, olen) != 0) {
        KM_LOGE("kp hash verify e.");
        return KM_ERR;
    }
    return 0;
}
#endif /* CONFIG_TB_KP */



uint32_t pub_key_init_internal(uint32_t otp_part_start, uint32_t otp_part_end, 
                                      uint32_t pkey_part_start, uint32_t pkey_part_end)
{
    int ret;

    memset(&g_pub_key_info, 0, sizeof(g_pub_key_info));

    ret = pkey_part_init(pkey_part_start, pkey_part_end);

    if (otp_part_start == INVALID_ADDR) {
        return ret;
    }
#if CONFIG_TB_KP > 0
    int i;
    uint32_t otp_part_size;
	//uint8_t buf[CONFIG_OTP_BANK_SIZE];
	uint8_t* buf = (uint8_t*)malloc(CONFIG_OTP_BANK_SIZE);
    manifest_idx_head_t *idx_head;
    otp_pub_key_head_t * otp_pub_start;

    if (!buf) {
        KM_LOGE("malloc fail");
        goto fail;
    }

    KM_LOGD("otp init start:0x%x,end:0x%x", otp_part_start, otp_part_end);
    otp_part_size = otp_part_end - otp_part_start;
    if (otp_part_size > CONFIG_OTP_BANK_SIZE) {
        otp_part_size = CONFIG_OTP_BANK_SIZE;
    }

#ifdef CONFIG_CHIP_PANGU
    if (otp_part_start == CONFIG_OTP_BASE_ADDR) {
        ret = efusec_read_bytes(otp_part_start, buf, otp_part_size);
        KM_LOGD("efuse read len:%d", ret);        
    } else {
        // FIXME: for test,规定从0x8FFF000地址获取KP数据，为了测试使用
        if (get_data_from_addr(otp_part_start, buf, otp_part_size)) {
            goto fail;
        }
        ret = otp_part_size;
    }
    // dump_data(buf, otp_part_size);
    if (ret <= 0 || ret > otp_part_size) {
        KM_LOGE("efuse read e.., len:%d", ret);
        goto fail;
    }
#else
    if (get_data_from_addr(otp_part_start, buf, otp_part_size)) {
        goto fail;
    }
#endif /* CONFIG_CHIP_PANGU */

    if (kp_check(buf)) {
        KM_LOGE("kp check e...");
        goto fail;
    }

    manifest_idx_t *kp = (manifest_idx_t *)buf;
    key_meta_t *key_meta = kp->key_metas;
    int key_num = kp->head.key_num;
    int version = kp->head.version;

    if (version == 0x03) {
        for (i = 0; i < key_num; i++) {
            if (key_meta->type == KEY_TYPE_PUBK_TB) {
                KM_LOGD("find pubkey in kp");
                int num = g_pub_key_info.key_num;
                if (num >= MAX_PUB_KEY_NUM)
                    break;
                strcpy(g_pub_key_info.pk_info[num].name, DEFAULT_PUBLIC_KEY_NAME_IN_OTP);
                memcpy(g_pub_key_info.pk_info[num].key, key_meta->offset + (uint8_t *)buf, key_meta->size);
                g_pub_key_info.pk_info[num].key_size = key_meta->size;
                g_pub_key_info.key_num++;
                break;
            }
            key_meta++;
        }        
    }

    idx_head = (manifest_idx_head_t *)buf;
    otp_pub_start = (otp_pub_key_head_t *)((uint32_t)buf + idx_head->total_size);
    if (otp_pub_start->magic != OTP_PUB_REGION_MAGIC) {
        KM_LOGE("otp cant find pk region");
        goto fail;
    }

    int index;
    int key_count;
    uint8_t *p = (uint8_t *)otp_pub_start + sizeof(otp_pub_key_head_t);
    int pr_body_len = otp_pub_start->total_size - sizeof(otp_pub_key_head_t);

    i = 0;
    while (i < pr_body_len) {
        int data_len;
        unsigned long key_addr = INVALID_ADDR, key_len = 0;
        otp_pub_key_store_t *store = (otp_pub_key_store_t *)p;

        data_len = get_length_with_type(store->type);

        if (store->type == OTP_POINT) {
            // Point to an address
            key_addr = *(uint32_t *)(store + 1);
            key_len = data_len;
            KM_LOGD("OTP_POINT key_addr=0x%x", key_addr);
        } else if (store->type < OTP_RSA1024) {
            // HASH
            if (get_pub_key_from_pkey_part(store->name, &key_addr, &key_len, &index)) {
                KM_LOGW("cant f pk in pkey part");
                goto end;
            }
            if (store->type == OTP_HASH_SHA1) {
                uint32_t olen;
                uint8_t  sha1[20];
                uint8_t *o_sha = (uint8_t *)(store + 1);

                hash_calc_start(DIGEST_HASH_SHA1, (uint8_t *)key_addr, key_len, sha1, &olen, 0);
                if (memcmp(o_sha, sha1, 20)) {
                    KM_LOGE("otp pk sha1 cmp e");
                    g_pub_key_info.pk_info[index].invalid = 1;
                }
            } else if (store->type == OTP_HASH_SHA256) {
                // TODO:
            }
            goto end;
        } else if (store->type < OTP_POINT) {
            // RAW data
            // RSA...ECC
            key_addr = (uint32_t)(store + 1);
            key_len = data_len;
        }
        unsigned long a;
        uint32_t b;
        if (get_pub_key_from_pkey_part(store->name, &a, &b, &index) == 0) {
            // 替换原来的pubkey
            g_pub_key_info.pk_info[index].key_size = MIN(key_len, MAX_PUB_KEY_SIZE);
            get_data_from_addr(key_addr, (uint8_t *)&g_pub_key_info.pk_info[index].key, MIN(key_len, MAX_PUB_KEY_SIZE));                
        } else {
            // 新增一个
            key_count = g_pub_key_info.key_num;
            if (key_count >= MAX_PUB_KEY_NUM) {
                KM_LOGD("2pubkey num > %d", MAX_PUB_KEY_NUM);
                goto end;
            }
            strncpy(g_pub_key_info.pk_info[key_count].name, store->name, PUBLIC_KEY_NAME_SIZE);
            memcpy(g_pub_key_info.pk_info[key_count].key, (void *)key_addr, MIN(key_len, MAX_PUB_KEY_SIZE));
            g_pub_key_info.pk_info[key_count].key_size = MIN(key_len, MAX_PUB_KEY_SIZE);
            g_pub_key_info.pk_info[key_count].invalid = 0;
            g_pub_key_info.key_num ++;
        }
    end:
        i += data_len + sizeof(otp_pub_key_store_t);
        p += data_len + sizeof(otp_pub_key_store_t);
    }
    KM_LOGD("pub key init ok");
	free(buf);
    return 0;
fail:
#endif /* CONFIG_TB_KP */
#if CONFIG_TB_KP > 0
    if (buf) {
	    free(buf);
    }
#endif
    return ret;
}

uint32_t km_get_pub_key_with_name(const char *pub_key_name, key_handle *key_addr, uint32_t *key_len)
{
    int i;

    if (get_pub_key_from_pkey_part(pub_key_name, (unsigned long *)key_addr, key_len, &i))
        return KM_ERR;
    return 0;
}

void km_show_pub_key_info(void)
{
    int i;
    KM_LOGD("--------------------------");
    for (i = 0; i < g_pub_key_info.key_num; i++) {
        KM_LOGD("%s", g_pub_key_info.pk_info[i].name);
        KM_LOGD("%d", g_pub_key_info.pk_info[i].invalid);
        KM_LOGD("0x%lx", (unsigned long)g_pub_key_info.pk_info[i].key);
        KM_LOGD("%d", g_pub_key_info.pk_info[i].key_size);
    }
    KM_LOGD("--------------------------");
}

uint32_t km_pub_key_init(void)
{
    mtb_partition_info_t part_info;
    uint32_t part_start, part_end, ppart_start, ppart_end;

    part_start = INVALID_ADDR;
    part_end = INVALID_ADDR;
    if (!mtb_get_partition_info(MTB_IMAGE_NAME_OTP, &part_info)) {
        part_start = part_info.start_addr;
        part_end = part_info.end_addr;
    } else {
        KM_LOGI("mtb cant f `otp`");
        if (!mtb_get_partition_info(MTB_IMAGE_NAME_KP, &part_info)) {
            part_start = part_info.start_addr;
            part_end = part_info.end_addr;
        } else {
            KM_LOGI("mtb cant f `kp`");
        }
    }

    ppart_start = INVALID_ADDR;
    ppart_end = INVALID_ADDR;
    if (!mtb_get_partition_info(MTB_IMAGE_NAME_PKEY, &part_info)) {
        ppart_start = part_info.start_addr;
        ppart_end = part_info.end_addr;
    } else {
        KM_LOGI("mtb cant f `pkey`");
    }

    if (pub_key_init_internal(part_start, part_end, ppart_start, ppart_end)) {
        KM_LOGI("pub key init e");
    }
    KM_LOGD("km_pub_key_init over");

#ifdef KM_DEBUG
    km_show_pub_key_info();
#endif
    return 0;
}

#endif /* defined(CONFIG_CHIP_CH2201) */
#endif