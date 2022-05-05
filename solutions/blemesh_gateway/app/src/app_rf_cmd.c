#include <stdbool.h>
#include <aos/aos.h>
#include <ulog/ulog.h>
#include <aos/cli.h>

#include "wm_type_def.h"
#include "utils.h"
#include "wm_mem.h"
#include "wm_regs.h"
#include "wm_wifi.h"
#include "wm_efuse.h"
#include "litepoint.h"

#define TAG "wifi"

extern u8* ieee80211_get_tx_gain(void);

static void w800_lpinit(void)
{
    tls_wifi_disconnect();
    tls_wifi_softap_destroy();
    tls_litepoint_start();
}

static void w800_lprstr(int argc, char **argv)
{
    u8 channel;
    u8 bandwidth = 0;

    if (argc > 2 || argc < 1)
    {
        printf("Argument failed\n"
               "w800 &lprstr <channel> [bandwidth]\n");
        return;
    }

    channel = atoi(argv[0]);
    if (argc > 1)
        bandwidth = atoi(argv[1]);
    printf("set channel %hhu, bandwidth %hhu\n", channel, bandwidth);

    w800_lpinit();
    tls_rx_litepoint_test_start(channel, bandwidth);
}

static void w800_lprstp(int argc, char **argv)
{
    if (argc != 0)
    {
        printf("Argument failed\n"
               "w800 &lprstp\n");
        return;
    }

    printf("stop rx process\n");

    tls_txrx_litepoint_test_stop();
}

static void w800_lprstt(int argc, char **argv)
{
    u32 cnt_total = 0;
    u32 cnt_good = 0;
    u32 cnt_bad = 0;

    if (argc != 0)
    {
        printf("Argument failed\n"
               "w800 &lprstt\n");
        return;
    }

	tls_rx_litepoint_test_result(&cnt_total, &cnt_good, &cnt_bad);

	printf("rx reslut: total=%x, good=%x, bad=%x\n", cnt_total, cnt_good, cnt_bad);
}

static void w800_lpchl(int argc, char **argv)
{
    u8 channel;
    u8 bandwidth = 0;

    if (argc > 2 || argc < 1)
    {
        printf("Argument failed\n"
               "w800 &lpchl <channel> [bandwidth]\n");
        return;
    }

    channel = atoi(argv[0]);
    if (argc > 1)
        bandwidth = atoi(argv[1]);
    printf("set channel %hhu, bandwidth %hhu\n", channel, bandwidth);

    w800_lpinit();
    tls_set_test_channel(channel, bandwidth);
}

static void w800_lptpd(int argc, char **argv)
{
    u32 period;

    if (argc != 1)
    {
        printf("Argument failed\n"
               "w800 &lptpd <interval>\n");
        return;
    }

    period = atoi(argv[0]);
    printf("set tx interval %u\n", period);

    tls_set_tx_litepoint_period(period);
}

static void w800_lptstr(int argc, char **argv)
{
    u32 tempcomp;
    u32 packetcnt;
    u16 psdulen;
    u32 gain;
    u32 txrate;

    if (argc != 5)
    {
        printf("Argument failed\n"
               "w800 &lptstr <tempcomp> <count> <length> <gain> <rate>\n");
        return;
    }

    tempcomp = strtol(argv[0], NULL, 16);
    packetcnt = strtol(argv[1], NULL, 16);
    psdulen = strtol(argv[2], NULL, 16);
    gain = strtol(argv[3], NULL, 16);
    txrate = strtol(argv[4], NULL, 16);
    printf("set tx params: tempcomp %x, cnt %x, len %x, gain %x, rate %x\n", tempcomp, packetcnt, psdulen, gain, txrate);

    w800_lpinit();
    tls_tx_litepoint_test_start(tempcomp, packetcnt, psdulen, gain, txrate, 0, 0, 0);
}

static void w800_lptstp(int argc, char **argv)
{
    if (argc != 0)
    {
        printf("Argument failed\n"
               "w800 &lptstp\n");
        return;
    }

    printf("stop tx process\n");

    tls_txrx_litepoint_test_stop();
}

static void w800_regr(int argc, char **argv)
{
    int i;
    u32 addr;
    u32 cnt;

    if (argc != 2)
    {
        printf("Argument failed\n"
               "w800 &regr <addr> <count>\n");
        return;
    }

    addr = strtol(argv[0], NULL, 16);
    cnt = strtol(argv[1], NULL, 16);

    printf("read register: addr %x, cnt %u\n", addr, cnt);

    for(i = 0; i < cnt; i++)
    {
        printf("%x ", tls_reg_read32(addr));
        addr += 4;
    }
    printf("\n");
}

static void w800_regw(int argc, char **argv)
{
    int i;
    u32 cnt;
    u32 addr;
    u32 value[8];

    if (argc < 2 && argc > 9)
    {
        printf("Argument failed\n"
               "w800 &regw <addr> <value1> [value2]...[value8]\n");
        return;
    }

    addr = strtol(argv[0], NULL, 16);
    cnt = argc - 1;
    for (i = 0; i < cnt; i++)
    {
        value[i] = strtol(argv[i + 1], NULL, 16);
    }

    printf("write register: addr %x, cnt %u\n", addr, cnt);

    for(i = 0; i < cnt; i++)
    {
        tls_reg_write32(addr, value[i]);
        addr += 4;
    }
}

static const char* rf_help_str = "w800 &lprstr:set rx channel\n"
                       "w800 &lprstp:set rx process\n"
                       "w800 &lprstt:get rx result\n"
                       "w800 &lpchl:set tx channel\n"
                       "w800 &lptpd:packet sending interval\n"
                       "w800 &lptstr:start tx process\n"
                       "w800 &lptstp:stop tx process\n"
                       "w800 &regr:read register\n"
                       "w800 &regw:write register\n\n>";

static void w800_rf_cli_cmd(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc < 2) {
        goto help;
    }
    if (strcmp(argv[1], "&lprstr") == 0)
        w800_lprstr(argc - 2, argv + 2);
    else if (strcmp(argv[1], "&lprstp") == 0)
        w800_lprstp(argc - 2, argv + 2);
    else if (strcmp(argv[1], "&lprstt") == 0)
        w800_lprstt(argc - 2, argv + 2);
    else if (strcmp(argv[1], "&lpchl") == 0)
        w800_lpchl(argc - 2, argv + 2);
    else if (strcmp(argv[1], "&lptpd") == 0) {
        w800_lptpd(argc - 2, argv + 2); 
    } else if (strcmp(argv[1], "&lptstr") == 0)
        w800_lptstr(argc - 2, argv + 2);
    else if (strcmp(argv[1], "&lptstp") == 0)
        w800_lptstp(argc - 2, argv + 2);
    else if (strcmp(argv[1], "&regr") == 0)
        w800_regr(argc - 2, argv + 2);
    else if (strcmp(argv[1], "&regw") == 0)
        w800_regw(argc - 2, argv + 2);
    else
        goto help;

    return;
help:

    printf("Argument failed\n%s", rf_help_str);
}


static void w800_wifi_debug(int argc, char **argv)
{
    u8* tx_gain;

    if (argc < 1)
    {
        goto error;
    }

    if (strcmp(argv[0], "log") == 0)
    {
        if (argc != 2)
            goto error;
        int value = atoi(argv[1]);
        printf("set wifi log %d\n", value);
        tls_wifi_enable_log(value ? TRUE : FALSE);
    }
    else if (strcmp(argv[0], "get_txpwr") == 0)
    {
        tx_gain = ieee80211_get_tx_gain();

        if (argc == 1)
        {
            printf("tx power: %02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x"
                             "%02x%02x%02x\n", 
                             tx_gain[0], tx_gain[1], tx_gain[2], \
                             tx_gain[3], tx_gain[4], tx_gain[5], \
                             tx_gain[6], tx_gain[7], tx_gain[8], \
                             tx_gain[9], tx_gain[10], tx_gain[11],\
                             tx_gain[12], tx_gain[13], tx_gain[14],\
                             tx_gain[15], tx_gain[16], tx_gain[17],\
                             tx_gain[18], tx_gain[19], tx_gain[20],\
                             tx_gain[21], tx_gain[22], tx_gain[23],\
                             tx_gain[24], tx_gain[25], tx_gain[26],\
                             tx_gain[27], tx_gain[28],      	
                             tx_gain[29], tx_gain[30], tx_gain[31], \
                             tx_gain[32], tx_gain[33], tx_gain[34],\
                             tx_gain[35], tx_gain[36], tx_gain[37],\
                             tx_gain[38], tx_gain[39], tx_gain[40], tx_gain[41],\
                             tx_gain[42], tx_gain[43], tx_gain[44],\
                             tx_gain[45], tx_gain[46], tx_gain[47],\
                             tx_gain[48], tx_gain[49], tx_gain[50],\
                             tx_gain[51], tx_gain[52], tx_gain[53],\
                             tx_gain[54], tx_gain[55], tx_gain[56],\
                             tx_gain[57], tx_gain[58],  
                             tx_gain[59], tx_gain[60], 
                             tx_gain[61], tx_gain[62], \
                             tx_gain[63], tx_gain[64], tx_gain[65], \
                             tx_gain[66], tx_gain[67], tx_gain[68], \
                             tx_gain[69], tx_gain[70], tx_gain[71],\
                             tx_gain[72], tx_gain[73], tx_gain[74],\
                             tx_gain[75], tx_gain[76], tx_gain[77],\
                             tx_gain[78], tx_gain[79], tx_gain[80],\
                             tx_gain[81], tx_gain[82], tx_gain[83]);
        }
        else if (argc == 2)
        {
            strtohexarray(tx_gain, TX_GAIN_LEN, argv[1]);
            tls_set_tx_gain(tx_gain);
        }
        else
        {
            goto error;
        }
    }
    else
    {
        goto error;
    }

    return;

error:
    printf("Argument failed\n"
           "iwpriv wifi_debug log <0|1>\n"
           "iwpriv wifi_debug get_txpwr [value]\n");
    return;
}

static char *scan_privacy_string(u8 privacy)
{
    char *sec;
    static char unknown[16];

    switch (privacy)
    {
    case WM_WIFI_AUTH_MODE_OPEN:
        sec = "NONE";
        break;
    case WM_WIFI_AUTH_MODE_WEP_AUTO:
        sec = "WEP/AUTO";
        break;
    case WM_WIFI_AUTH_MODE_WPA_PSK_TKIP:
        sec = "WPA_PSK/TKIP";
        break;
    case WM_WIFI_AUTH_MODE_WPA_PSK_CCMP:
        sec = "WPA_PSK/CCMP";
        break;
    case WM_WIFI_AUTH_MODE_WPA_PSK_AUTO:
        sec = "WPA_PSK/AUTO";
        break;
    case WM_WIFI_AUTH_MODE_WPA2_PSK_TKIP:
        sec = "WPA2_PSK/TKIP";
        break;
    case WM_WIFI_AUTH_MODE_WPA2_PSK_CCMP:
        sec = "WPA2_PSK/CCMP";
        break;
    case WM_WIFI_AUTH_MODE_WPA2_PSK_AUTO:
        sec = "WPA2_PSK/AUTO";
        break;
    case WM_WIFI_AUTH_MODE_WPA_WPA2_PSK_TKIP:
        sec = "WPA_PSK/WPA2_PSK/TKIP";
        break;
    case WM_WIFI_AUTH_MODE_WPA_WPA2_PSK_CCMP:
        sec = "WPA_PSK/WPA2_PSK/CCMP";
        break;
    case WM_WIFI_AUTH_MODE_WPA_WPA2_PSK_AUTO:
        sec = "WPA_PSK/WPA2_PSK/AUTO";
        break;

    default:
        sprintf(unknown, "Unknown(0x%02x)", privacy);
        sec = unknown;
        break;
    }
    return sec;
}

static  char *scan_mode_string(u8 mode)
{
    char *ap_mode;

    switch (mode)
    {
    case 1:
        ap_mode = "IBSS";
        break;
    case 2:
        ap_mode = "ESS";
        break;

    default:
        ap_mode = "ESS";
        break;
    }
    return ap_mode;
}


static void wifi_scan_handler(void)
{
    char *buf = NULL;
    char *buf1 = NULL;
    u32 buflen;
    int i, j;
    int err;
    u8 ssid[33];
    struct tls_scan_bss_t *wsr;
    struct tls_bss_info_t *bss_info;

    buflen = 2000;
    buf = tls_mem_alloc(buflen);
    if (!buf)
    {
        goto end;
    }

    buf1 = tls_mem_alloc(300);
    if(!buf1)
    {
        goto end;
    }
    memset(buf1, 0, 300);

    err = tls_wifi_get_scan_rslt((u8 *)buf, buflen);
    if (err)
    {
        goto end;
    }

    wsr = (struct tls_scan_bss_t *)buf;
    bss_info = (struct tls_bss_info_t *)(buf + 8);

    printf("\n");

    for(i = 0; i < wsr->count; i++)
    {
        j = sprintf(buf1, "bssid:%02X%02X%02X%02X%02X%02X, ", bss_info->bssid[0], bss_info->bssid[1],
                    bss_info->bssid[2], bss_info->bssid[3], bss_info->bssid[4], bss_info->bssid[5]);
        j += sprintf(buf1 + j, "ch:%d, ", bss_info->channel);
        j += sprintf(buf1 + j, "rssi:%d, ", (signed char)bss_info->rssi);
        j += sprintf(buf1 + j, "wps:%d, ", bss_info->wps_support);
        j += sprintf(buf1 + j, "max_rate:%dMbps, ", bss_info->max_data_rate);
        j += sprintf(buf1 + j, "%s, ", scan_mode_string(bss_info->mode));
        j += sprintf(buf1 + j, "%s, ", scan_privacy_string(bss_info->privacy));
        memcpy(ssid, bss_info->ssid, bss_info->ssid_len);
        ssid[bss_info->ssid_len] = '\0';
        j += sprintf(buf1 + j, "%s", ssid);

        printf("%s\r\n", buf1);

        bss_info ++;
    }

end:
    if(buf)
    {
        tls_mem_free(buf);
    }
    if(buf1)
    {
        tls_mem_free(buf1);
    }
}

static void w800_wifi_scan(int argc, char **argv)
{
    if (argc != 0)
    {
        printf("Argument failed\n"
               "iwpriv wifi_scan\n");
        return;
    }

    tls_wifi_scan_result_cb_register(wifi_scan_handler);
    tls_wifi_scan();

    return;
}

static const char* wifi_help_str = "iwpriv wifi_debug:wifi debug command\n"
                       "iwpriv wifi_scan:scan and show result\n\n>";

static void w800_wifi_cli_cmd(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc < 2) {
        goto help;
    }
    if (strcmp(argv[1], "wifi_debug") == 0)
        w800_wifi_debug(argc - 2, argv + 2);
    else if (strcmp(argv[1], "wifi_scan") == 0)
        w800_wifi_scan(argc - 2, argv + 2);
    else
        goto help;

    return;
help:

    printf("Argument failed\n%s", wifi_help_str);
}

void cli_reg_cmd_w800(void)
{
    static const struct cli_command cmd_info = {
        "w800",
        "w800 rf test command.",
        w800_rf_cli_cmd
    };

    aos_cli_register_command(&cmd_info);


    static const struct cli_command cmd_info2 = {
        "iwpriv",
        "w800 wifi control command.",
        w800_wifi_cli_cmd
    };

    aos_cli_register_command(&cmd_info2);

    return;
}

