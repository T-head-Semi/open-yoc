
#ifndef _EXT_ADV_H_
#define _EXT_ADV_H_

typedef struct {
    struct net_buf *head_node;
    struct bt_le_scan_recv_info info;
    bt_addr_le_t addr;
    u8_t flag;
    int  start_time;
} adv_buf_frag_head;


#ifndef CONFIG_EXT_ADV_BUF_MAX_FRAG_SIZE
#define CONFIG_EXT_ADV_BUF_MAX_FRAG_SIZE 10
#endif
#define INVALID_NET_IF 0xFF
#define MAX_AD_LEN 255
#define MAX_TOTAL_AD_DATA_LEN 510
#define HEAD_BUF_INDEX_NOT_FOUND 0xFF
#define EXT_ADV_GATHER_TIMEOUT (1000) //ms


typedef void (*recv_data_cb_t)(struct net_buf_simple *buf);

int bt_mesh_ext_adv_init();
int bt_mesh_ext_adv_start(const struct bt_le_adv_param *param, int32_t duration,
                          const struct bt_data *ad, size_t ad_len,
                          const struct bt_data *sd, size_t sd_len);
int bt_mesh_ext_adv_stop(void);
int bt_mesh_ext_adv_enable(void);
int bt_mesh_ext_adv_disable(void);
void bt_mesh_ext_adv_scan(recv_data_cb_t cb);

#endif /* End of _EXT_ADV_H_  */
