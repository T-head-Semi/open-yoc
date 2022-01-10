/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#if defined(CONFIG_TCPIP) || defined(CONFIG_SAL)

#include <aos/cli.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <yoc/netmgr.h>

#include <devices/netdrv.h>

#include <lwip/netdb.h>
#include <arpa/inet.h>



#if defined(CONFIG_TCPIP) || defined(CONFIG_TCPIP_NBIOT)

#define USAGE_INFO    "\n\tifconfig ipaddr netmask gateway\n\tifconfig ipv6 ipaddr\n\tifconfig dhcpstart/dhcpstop\n"

#elif defined(CONFIG_SAL)

#define USAGE_INFO    "\n\tifconfig status/ip\n\tOnly wifi support:\n\tifconfig ap ssid psw\n\tifconfig apoff\n\tifconfig uartcfg 115200 8 1 0 3 1\n"

#else

#define USAGE_INFO    "\n\tcmd is useless\n"

#endif

#define USAGE_INFO_ETH  "\n\tifconfig ip netmask gateway  e.g. ifconfig 192.168.0.1 255.255.255.0 0.0.0.0"  \
                        "\n\tifconfig dhcpstart/dhcpstop\n"

static void cmd_ifconfig_eth(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int item_count = argc;

    aos_dev_t *eth_dev = device_find("eth", 0);
    if (eth_dev == NULL) {
        printf("eth not found\n");
        printf(USAGE_INFO);
        return;
    }

    netmgr_hdl_t hdl_eth = netmgr_get_handle("eth");

    if (item_count == 1) {
        netmgr_get_info(hdl_eth);
    } else if (item_count == 2) {
#if LWIP_DHCP
        if (strcmp(argv[1], "dhcpstart") == 0) {
            printf("start dhcp\n");
            netmgr_ipconfig(hdl_eth, 1, NULL, NULL, NULL);
        } else if (strcmp(argv[1], "dhcpstop") == 0) {
            printf("stop dhcp\n");
            netmgr_ipconfig(hdl_eth, 0, NULL, NULL, NULL);
        } else {
            printf(USAGE_INFO_ETH);
        }
#endif
    } else if (item_count == 4) {
        netmgr_ipconfig(hdl_eth, 0, argv[1], argv[2], argv[3]);
    } else {
        printf(USAGE_INFO_ETH);
    }
}

static void cmd_ifconfig_wifi(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int item_count = argc;
    aos_dev_t *wifi_dev = device_find("wifi", 0);

    if (wifi_dev != NULL) {
        netmgr_hdl_t hdl = netmgr_get_handle("wifi");

        if (hdl == NULL) {
            return;
        }

        if (item_count > 1) {
            if (strcmp(argv[1], "dns") == 0) {
                ip_addr_t dns_svr[2];
                if (item_count == 3) {
                    if (ipaddr_aton(argv[2], &dns_svr[0])) {
                        hal_net_set_dns_server(wifi_dev, &dns_svr[0], 1);
                        return;
                    }
                } else if (item_count == 4) {
                    if (ipaddr_aton(argv[2], &dns_svr[0]) && ipaddr_aton(argv[3], &dns_svr[1])) {
                        hal_net_set_dns_server(wifi_dev, &dns_svr[0], 2);
                        return;
                    }
                }
            } else if (strcmp(argv[1], "apoff") == 0) {
                netmgr_stop(hdl);
                printf("apoff\n");
                return;
            } else if (strcmp(argv[1], "ap") == 0) {
                if (item_count == 4) {
                    printf("apconfig ssid:%s, psw:%s\n", argv[2], argv[3]);

                    netmgr_config_wifi(hdl, argv[2], strlen(argv[2]), argv[3], strlen(argv[3]));
                    netmgr_start(hdl);
                    return;
                }
            } else {
                if (item_count == 4) {
                    printf("ipaddr:%s netmask:%s, gateway:%s\n", argv[1], argv[2], argv[3]);
                    netmgr_ipconfig(hdl, 0, argv[1], argv[2], argv[3]);
                    return;
                }
            }
        } else {
            netmgr_get_info(hdl);
            return;
        }
    }
    printf(USAGE_INFO);
}

static void cmd_ifconfig_gprs(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int item_count = argc;
    aos_dev_t *gprs_dev = device_find("gprs", 0);

    if (gprs_dev != NULL) {

        netmgr_hdl_t hdl = netmgr_get_handle("gprs");

        if (hdl == NULL)
            return;

        if (item_count == 1) {

            netmgr_get_info(hdl);

            return;
        }
    }
    printf(USAGE_INFO);
}

static void cmd_ifconfig_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int item_count = argc;

    aos_dev_t *eth_dev = device_find("eth", 0);

    if (eth_dev != NULL) {

        netmgr_hdl_t hdl_eth = netmgr_get_handle("eth");

        if (hdl_eth == NULL)
            return;

        if (item_count == 2) {
#if LWIP_DHCP
            if (strcmp(argv[1], "dhcpstart") == 0) {
                printf("start dhcp\n");
                netmgr_ipconfig(hdl_eth, 1, NULL, NULL, NULL);
            } else if (strcmp(argv[1], "dhcpstop") == 0) {
                printf("stop dhcp\n");
                netmgr_ipconfig(hdl_eth, 0, NULL, NULL, NULL);
            }
#endif

            return;
        } else {
            netmgr_get_info(hdl_eth);
            return;
        }
    }

    aos_dev_t *wifi_dev = device_find("wifi", 0);

    if (wifi_dev != NULL) {
        netmgr_hdl_t hdl = netmgr_get_handle("wifi");

        if (hdl == NULL)
            return;

        if (item_count == 2) {
            if (strcmp(argv[1], "apoff") == 0) {
                netmgr_stop(hdl);
                printf("apoff\n");
                return;
            }
        } else if (item_count == 4) {
            if (strcmp(argv[1], "ap") == 0) {
                printf("apconfig ssid:%s, psw:%s\n", argv[2], argv[3]);

                netmgr_config_wifi(hdl, argv[2], strlen(argv[2]), argv[3], strlen(argv[3]));
                netmgr_start(hdl);
            }

            return;
        } else {
            netmgr_get_info(hdl);

            return;
        }
    }

    aos_dev_t *gprs_dev = device_find("gprs", 0);

    if (gprs_dev != NULL) {

        netmgr_hdl_t hdl = netmgr_get_handle("gprs");

        if (hdl == NULL)
            return;

        if (item_count == 1) {

            netmgr_get_info(hdl);

            return;
        }
    }

    aos_dev_t *nbiot_dev = device_find("nbiot", 0);

    if (nbiot_dev != NULL) {

        netmgr_hdl_t hdl = netmgr_get_handle("nbiot");

        if (hdl == NULL)
            return;

        if (item_count == 1) {

            netmgr_get_info(hdl);

            return;
        }
    }

    printf(USAGE_INFO);
}

void cli_reg_cmd_ifconfig(void)
{
    static const struct cli_command cmd_info = {
        "ifconfig",
        "network config",
        cmd_ifconfig_func,
    };

    aos_cli_register_command(&cmd_info);
}

void cli_reg_cmd_ifconfig_eth(void)
{
    static const struct cli_command cmd_info = {
        "ifconfig",
        "network config",
        cmd_ifconfig_eth,
    };

    aos_cli_register_command(&cmd_info);
}

void cli_reg_cmd_ifconfig_wifi(void)
{
    static const struct cli_command cmd_info = {
        "ifconfig",
        "network config",
        cmd_ifconfig_wifi,
    };

    aos_cli_register_command(&cmd_info);
}

void cli_reg_cmd_ifconfig_gprs(void)
{
    static const struct cli_command cmd_info = {
        "ifconfig",
        "network config",
        cmd_ifconfig_gprs,
    };

    aos_cli_register_command(&cmd_info);
}
#endif
