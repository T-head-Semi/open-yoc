/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef __MTB_INTERNAL_H__
#define __MTB_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NEXT_SECTION(scn_addr) ((unsigned long)scn_addr + ((scn_head_t *)scn_addr)->size)
#define NEXT_SECTION_IMGINFO(scn_addr) (NEXT_SECTION(scn_addr) + sizeof(scn_head_t))
#define GET_FIRST_IMG_SCN_ADDR(bm_addr, scn_addr)                                                  \
    get_section_addr((unsigned long)bm_addr, &(g_scn_img_type[SCN_TYPE_IMG]), NULL,                     \
                     (unsigned long *)scn_addr, SEACH_MODE_FIRST_TYPE)
#define GET_PUBLICKEY_SCN_ADDR(bm_addr, scn_addr)                                                  \
    get_section_addr((unsigned long)bm_addr, &(g_scn_key_type[SCN_SUB_TYPE_KEY_BOOT]), NULL,            \
                     (unsigned long *)scn_addr, SEACH_MODE_EXTEND_TYPE)
#define GET_PARTION_SCN_ADDR(bm_addr, scn_addr)                                                    \
    get_section_addr((unsigned long)bm_addr, &(g_scn_part_type[SCN_SUB_TYPE_IMG_PART]), NULL,           \
                     (unsigned long *)scn_addr, SEACH_MODE_EXTEND_TYPE)

// mtb old
int mtbv_init(void);
int mtbv_get_part_info(uint8_t *name, uint32_t *part_addr, uint32_t *part_size);
int mtbv_get_img_info(const char *name, img_info_t *img_info);
int get_section_addr(unsigned long bm_addr, const scn_type_t *seach_type, uint8_t *seach_name, unsigned long *scn_addr, uint32_t mode);
int mtbv_image_verify(const char *name);

// mtbv4
int mtbv4_init(void);
int mtbv4_crc_check(void);
int mtbv4_verify(void);
int mtbv4_image_verify(const char *name);
int mtbv4_partition_count(void);
int mtbv4_get_partition_info(const char *name, mtb_partition_info_t *part_info);
int mtbv4_get_partition_info_with_index(int index, mtb_partition_info_t *part_info);

// common
int get_data_from_faddr(unsigned long addr, void *data, size_t data_len);

#ifdef __cplusplus
}
#endif
#endif /* __MTB_INTERNAL_H__ */