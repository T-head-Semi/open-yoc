#include <stdbool.h>
#include <aos/aos.h>
#include <ulog/ulog.h>
#include <aos/cli.h>

#include "key_mgr.h"

typedef struct {
    int key_type;
    int key_size;
    char name[20];
} cli_key_t;

static cli_key_t cli_key_tbl[] = {
    {KEY_ID_MAC, 6, "KEY_MAC"},
    {KEY_ID_MESSAGE_AUTH, 16, "KEY_MESSAGE_AUTH"},
    {KEY_ID_PRODUCT_KEY, 32, "KEY_PRODUCT_KEY"},
    {KEY_ID_PRODUCT_SECRET, 32, "KEY_PRODUCT_SECRET"},
    {KEY_ID_PRODUCT_ID, 32, "KEY_PRODUCT_ID"},
    {KEY_ID_DEVICE_NAME, 32, "KEY_DEVICE_NAME"},
    {KEY_ID_DEVICE_SECRET, 16, "KEY_DEVICE_SECRET"},
};

void kp_test(void *arg)
{
    km_init();
    key_handle key_addr;
    uint32_t len;
    uint8_t g_key_buf[50];

    for (int i = KEY_ID_MAC; i < (KEY_ID_DEVICE_SECRET+1); i ++) {
        memset(g_key_buf, 0x00, sizeof(g_key_buf));
        int ret = km_get_key(i, &key_addr, &len);
        
        if (ret != KM_OK) {
            printf("read(%s) err\r\n", cli_key_tbl[i-KEY_ID_MAC].name);
        } else {
            memcpy(g_key_buf, (uint8_t *)key_addr, cli_key_tbl[i-KEY_ID_MAC].key_size);

            printf("%s:", cli_key_tbl[i-KEY_ID_MAC].name);
            if (i == KEY_ID_MAC) {
                for (int j = 0; j < cli_key_tbl[i-KEY_ID_MAC].key_size; j++) {
                    printf("%02x ", g_key_buf[j]);
                }
            } else if (i == KEY_ID_PRODUCT_ID) {
                printf("%d", *(int*)g_key_buf);
            } else {
                printf("%s", g_key_buf);
            }
            printf("\r\n");
        }
    }

    printf("kp test\r\n");    
}

static void kp_cli_cmd(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int ret = aos_task_new("kp", kp_test, NULL, 4*1024);

    printf("kp task (%d)\r\n", ret);
}

void cli_reg_cmd_kp(void)
{
    static const struct cli_command cmd_info = {
        "kp",
        "kp  test command.",
        kp_cli_cmd
    };

    aos_cli_register_command(&cmd_info);

}
