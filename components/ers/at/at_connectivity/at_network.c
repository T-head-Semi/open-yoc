#include <yoc/atserver.h>
#include <at_cmd.h>
#include <http_client.h>
#include <devices/wifi.h>
#include <devices/netdrv.h>
#include <ulog/ulog.h>
#include <lwip/netdb.h>
#include <mbedtls/base64.h>
#include <at_internal.h>
#include <wm_fwup.h>
#include <wm_crypto_hard.h>
#include <Wm_flash_map.h>
#include <wm_internal_flash.h>
#include <wm_efuse.h>
#include <ntp.h>
#include <aos/kv.h>
#include "at_network.h"

#define TAG "at_network"
#define USER_VERSION   5

#define AT_RESP_CME_ERR(errno)    atserver_send("\r\n+CME ERROR:%d\r\n", errno)

extern int ping(int type, char *remote_ip);
extern void dumpBuffer(char *name, char* buffer, int len);

typedef enum {
	USER_AT_EVT_TYPE_HTTP = 0,
	USER_AT_EVT_TYPE_NTP,
	USER_AT_EVT_TYPE_OTA,
	USER_AT_EVT_TYPE_MAX,
}user_at_event_type;
typedef struct {
	user_at_event_type type;
	http_client_method_t method;
	uint32_t offset;
	char *url;
}event_user_at_param_t;

typedef struct {
    char                         *url;
    char                         *scheme;
    char                         *host;
    int                          port;
    char                         *username;
    char                         *password;
    char                         *path;
    char                         *query;
    char                         *cert_pem;
    http_client_method_t         method;
    http_client_auth_type_t      auth_type;
    http_client_transport_t      transport_type;
    int                          max_store_header_size;
} connection_info_t;

struct http_client_at {
    int                         redirect_counter;
    int                         max_redirection_count;
    int                         process_again;
    struct http_parser          *parser;
    struct http_parser_settings *parser_settings;
    void                        *transport_list;
    void                        *transport;
    void                        *request;
    void                        *response;
    void                        *user_data;
    void                        *auth_data;
    char                        *post_data;
    char                        *location;
    char                        *auth_header;
    char                        *current_header_key;
    char                        *current_header_value;
    int                         post_len;
    connection_info_t           connection_info;
    bool                        is_chunk_complete;
};

int wifi_getip(ip_addr_t *ipaddr)
{
    int ret = -1;
    ip_addr_t netmask;
    ip_addr_t gw;

    aos_dev_t *dev = device_open_id("wifi", 0);//netmgr_get_dev(app_netmgr_hdl);

    if (dev) {
        ret = hal_net_get_ipaddr(dev, ipaddr, &netmask, &gw);
    }
    return ret;
}

int wifi_scan(wifi_event_func *evt_cb)
{
	int ret = -1;
    aos_dev_t *dev = device_open_id("wifi", 0);//netmgr_get_dev(app_netmgr_hdl);
	LOGD(TAG, "wifi dev %x", dev);
    if (dev) {
		hal_wifi_install_event_cb(dev, evt_cb);
		ret = hal_wifi_start_scan(dev, NULL, 1);
    }
    return ret;
}

void wgip_handler(char *cmd, int type, char *data)
{
	int ret;
	ip_addr_t ipaddr;

    if (type == EXECUTE_CMD) {
        ret = wifi_getip(&ipaddr);
		if(ret)
		{
			AT_BACK_CME_ERR(AT_ERR_STATUS);
		}
		else
		{
        	AT_BACK_RET_OK(cmd, ipaddr_ntoa(&ipaddr));
		}
    }
}
#define SCAN_RESULTS_BUF_LEN  1024
static char g_scan_results[1024];
static void scan_compeleted(aos_dev_t *dev, uint16_t number, wifi_ap_record_t *ap_records)
{
	int pos = 0;
    wifi_ap_record_t wifiApRecord;
	
    // sort with rssi
    for (int j = 0; j < number; ++j) {
        for (int i = j + 1; i < number; ++i) {
            if (ap_records[i].rssi > ap_records[j].rssi) {
                wifiApRecord = ap_records[i];
                ap_records[i] = ap_records[j];
                ap_records[j] = wifiApRecord;
            }
        }
    }

	g_scan_results[pos] = '\0';
    for (int i = 0; i < number; i++) {
        if (ap_records[i].ssid[0] != '\0') {
            LOGD(TAG, "ssid: %s, rssi, %d", ap_records[i].ssid, ap_records[i].rssi);
			if(pos < 1024 - 70)
			{
				pos += sprintf(&g_scan_results[pos], "%s,%02x:%02x:%02x:%02x:%02x:%02x,%d,%d\r\n", 
					ap_records[i].ssid, 
					ap_records[i].bssid[0], ap_records[i].bssid[1], ap_records[i].bssid[2], 
					ap_records[i].bssid[3], ap_records[i].bssid[4],	ap_records[i].bssid[5], 
					ap_records[i].rssi, ap_records[i].channel);
			}
			else
			{
				break;
			}
        }
    }
	//remove last /r/n
	if(pos >= 2)
	{
		g_scan_results[pos - 2] = '\0';
	}
}
static wifi_event_func wifi_event = {
    NULL,
    NULL,
    scan_compeleted,
    NULL
};
void wscan_handler(char *cmd, int type, char *data)
{
	int ret;

    {
        ret = wifi_scan(&wifi_event);
		if(ret)
		{
			AT_BACK_CME_ERR(AT_ERR_STATUS);
		}
		else
		{
        	AT_BACK_RET_OK(cmd, g_scan_results);
		}
    }
}
static void event_ping_result_handler(uint32_t event_id, const void *param, void *context)
{
	int timeout;
	switch(event_id)
	{
		case EVENT_NET_PING_RESULT:
			timeout = (int)param;
			if(timeout > 0)
			{
				AT_BACK_RET_OK_INT("AT+PING", timeout);
			}
			else
			{
				AT_BACK_CME_ERR(AT_ERR);
			}
			break;
		default:
			break;
	}
}
void ping_handler(char *cmd, int type, char *data)
{
	int ret;

    {
        ret = ping(4, data);
		if(ret)
		{
			AT_BACK_CME_ERR(AT_ERR);
		}
		else
		{
        	//AT_BACK_RET_OK(cmd, g_scan_results);
		}
    }
}
#define FLASH_OTA_ADDR    0x8010000
static long g_data_len = 0;
static hsDigestContext_t md5_ctx;
static char md5_content[28];
static int ota_img_length = 0;
static int g_ota_addr = FLASH_OTA_ADDR;
static IMAGE_HEADER_PARAM_ST g_ota_booter;
static int g_ota_booter_len = 0;
static int g_ota_skip_size = 0;
static int _http_event_handler(http_client_event_t *evt)
{
#define PRINT_SIZE   (50 * 1024)
static struct timeval tv_start;
	int last = 0;
	char buf[16] = {0};
	char base64_buf[28];
	int err = 0;
	int len = 0;
	int type = (int)evt->user_data;
	size_t olen;
	struct timeval tv;
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
			if(type == USER_AT_EVT_TYPE_OTA)
			{
				g_ota_addr = FLASH_OTA_ADDR;
				g_ota_booter_len = 0;
				g_ota_skip_size = 0;
				tls_crypto_md5_init(&md5_ctx);
			}
            break;
        case HTTP_EVENT_HEADER_SENT:
            LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
			gettimeofday(&tv_start, NULL);
            LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			if(type == USER_AT_EVT_TYPE_OTA)
			{
				if(!strcmp("Content-MD5", evt->header_key))
				{
					strncpy(md5_content, evt->header_value, sizeof(md5_content));
					LOGD(TAG, "md5_content %s", md5_content);
				}
				else if(!strcmp("Content-Length", evt->header_key))
				{
					ota_img_length = atoi(evt->header_value);
					LOGD(TAG, "OTA img len %d", ota_img_length);
				}
			}
			else if(type == USER_AT_EVT_TYPE_HTTP)
			{
				err = atserver_send("+WH=%s: %s\r\n", evt->header_key, evt->header_value);
				if(err == -EINVAL)
				{
					err = 0;
				}
			}
            break;
        case HTTP_EVENT_ON_DATA:
            if (http_client_is_chunked_response(evt->client)) {
				//TODO: chunked data
            }
			else
			{
				if(type == USER_AT_EVT_TYPE_OTA)
				{
					if(g_ota_booter_len < sizeof(IMAGE_HEADER_PARAM_ST) && g_ota_skip_size == 0)
					{
						memcpy((char *)&g_ota_booter + g_ota_booter_len, evt->data, min(evt->data_len, sizeof(IMAGE_HEADER_PARAM_ST) - g_ota_booter_len));
						g_ota_booter_len += evt->data_len;
					}
					if(g_ota_booter_len >= sizeof(IMAGE_HEADER_PARAM_ST) && g_ota_booter.img_attr.b.img_type != 1)
					{
						g_ota_skip_size = g_ota_booter.img_len;
						g_ota_skip_size += sizeof(IMAGE_HEADER_PARAM_ST);
						if(g_ota_booter.img_attr.b.signature == 1)
						{
							g_ota_skip_size += 128;
						}
						g_ota_booter_len = 0;
					}
					if(g_ota_skip_size > 0)
					{
						if(g_ota_skip_size > evt->data_len)
						{
							g_ota_skip_size -= evt->data_len;
						}
						else
						{
							memcpy((char *)&g_ota_booter + g_ota_booter_len, (char *)evt->data + g_ota_skip_size,
								min((evt->data_len - g_ota_skip_size), sizeof(IMAGE_HEADER_PARAM_ST) - g_ota_booter_len));
							g_ota_booter_len += (evt->data_len - g_ota_skip_size);
							g_ota_skip_size = 0;
						}
					}
					tls_crypto_md5_update(&md5_ctx, evt->data, evt->data_len);
					tls_fls_write(g_ota_addr, evt->data, evt->data_len);
					//LOGD(TAG, "Write to flash addr 0x%x, len 0x%x", g_ota_addr, evt->data_len);
					g_ota_addr += evt->data_len;
					if(ota_img_length > 0)
					{
						last = g_data_len * 10 / ota_img_length;
						g_data_len += evt->data_len;
						if((g_data_len * 10 / ota_img_length) > last)
						{
							len = sprintf(buf, "+%d%%", (g_data_len * 10 / ota_img_length) * 10);
							err = atserver_write(buf, len);
							LOGD(TAG, "%s", buf);
						}
					}
				}
				else if(type == USER_AT_EVT_TYPE_HTTP)
				{
					last = g_data_len / PRINT_SIZE;
					g_data_len += evt->data_len;
					if(g_data_len / PRINT_SIZE > last)
					{
		            	LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", g_data_len);
					}
					sprintf(buf, "+WD=%d,", evt->data_len);
					atserver_write(buf, strlen(buf));
					err = atserver_write(evt->data, evt->data_len);
					//LOGD(TAG, "event: %s", buf);
				}
			}

            break;
        case HTTP_EVENT_ON_FINISH:
			gettimeofday(&tv, NULL);
            LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", g_data_len);
            LOGD(TAG, "HTTP_EVENT_ON_FINISH duration %d s", tv.tv_sec - tv_start.tv_sec);
			if(((struct http_client_at *)evt->client)->is_chunk_complete || 
				((struct http_client_at *)evt->client)->connection_info.method == HTTP_METHOD_HEAD)
			{
				if(type == USER_AT_EVT_TYPE_OTA)
				{
					tls_crypto_md5_final(&md5_ctx, buf);
					dumpBuffer("md5 buf",buf,16);
					mbedtls_base64_encode(base64_buf, sizeof(base64_buf), &olen, buf, 16);
					LOGD(TAG, "MD5: %s", base64_buf);
					if(!strcmp(base64_buf, md5_content))
					{
						LOGD(TAG, "Write OTA flag 0x%x to flash addr 0x%x", g_ota_booter.org_checksum, TLS_FLASH_OTA_FLAG_ADDR);
						tls_fls_write(TLS_FLASH_OTA_FLAG_ADDR, (u8 *)&g_ota_booter.org_checksum, sizeof(g_ota_booter.org_checksum));
					}
					else
					{
						AT_BACK_CME_ERR(AT_ERR);
						break;
					}
				}
				AT_BACK_OK();
			}
			else
			{
				AT_BACK_CME_ERR(AT_ERR);
			}
            break;
        case HTTP_EVENT_DISCONNECTED:
            LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
	if(err < 0)
	{
		LOGD(TAG, "ERR %d", err);
		http_client_close(evt->client);
    	return err;
	}
    return 0;
}
static void https_with_url(event_user_at_param_t *param)
{
    http_client_config_t config = {
        .url = param->url,
        .event_handler = _http_event_handler,
        .method = param->method,
        .buffer_size = 1460,
        .user_data = (void *)param->type,
//        .cert_pem = ca_crt_rsa,
    };
	g_data_len = 0;
    http_client_handle_t client = http_client_init(&config);
	LOGD(TAG, "param->method: %d, param->offset: %d\r\n", param->method, param->offset);
	if(param->method == HTTP_METHOD_GET && param->offset > 0)
	{
		char range_data[30];
		sprintf(range_data, "bytes=%d-", param->offset);
		LOGD(TAG, "Range: %s\r\n", range_data);
		if(http_client_set_header(client, "Range", range_data) != 0)
		{
			LOGE(TAG, "Error Set header Range: bytes=%d-", (param->offset));
			AT_BACK_CME_ERR(AT_ERR);
			http_client_cleanup(client);
			return;
		}
	}
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTPS Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
		AT_BACK_CME_ERR(AT_ERR);
    }
    http_client_cleanup(client);
}

static int init_param_with_url(event_user_at_param_t **param_p, char *url)
{
	event_user_at_param_t *param = NULL;
	int len = 0;
	if(url)
	{
		len = strlen(url);
		if(len == 0 || len > 512)
		{
			LOGD(TAG, "url len %d is invalid", len);
			return AT_ERR;
		}
	}
	param = aos_zalloc(sizeof(event_user_at_param_t));
	if(param == NULL)
	{
		return AT_ERR;
	}
	if(url)
	{
		param->url = aos_zalloc(len + 1);
		LOGD(TAG, "url %p is zalloc", param->url);
		if(param->url == NULL)
		{
			aos_free(param);
			return AT_ERR;
		}
		strcpy(param->url, url);
	}
	*param_p = param;
	return AT_OK;
}
void httpget_handler(char *cmd, int type, char *data)
{
	if(type == WRITE_CMD)
	{
		event_user_at_param_t *param = NULL;
		char *url = data;
		uint32_t offset = 0;
		char *token = strchr(data, ',');
        int ret = -1;
        if (token) {
            *token = '\0';
			token++;
			offset = atoi(data);
			url = token;
        }

		if(init_param_with_url(&param, url))
		{
			AT_BACK_CME_ERR(AT_ERR);
			return;
		}
		param->type = USER_AT_EVT_TYPE_HTTP;
		param->method = HTTP_METHOD_GET;
		param->offset = offset;
		LOGD(TAG, "offset is %d\r\n", param->offset);
		event_publish(EVENT_NETWORK_AT_CMD, param);
	}
	else
	{
		AT_BACK_CME_ERR(AT_ERR_INVAL);
	}
}
void httphead_handler(char *cmd, int type, char *data)
{
	if(type == WRITE_CMD)
	{
		event_user_at_param_t *param = NULL;
		if(init_param_with_url(&param, data))
		{
			AT_BACK_CME_ERR(AT_ERR);
			return;
		}
		param->type = USER_AT_EVT_TYPE_HTTP;
		param->method= HTTP_METHOD_HEAD;
		event_publish(EVENT_NETWORK_AT_CMD, param);
	}
	else
	{
		AT_BACK_CME_ERR(AT_ERR_INVAL);
	}
}

void httpota_handler(char *cmd, int type, char *data)
{
	if(type == WRITE_CMD)
	{
		event_user_at_param_t *param = NULL;
		if(init_param_with_url(&param, data))
		{
			AT_BACK_CME_ERR(AT_ERR);
			return;
		}
		param->type = USER_AT_EVT_TYPE_OTA;
		param->method= HTTP_METHOD_GET;
		event_publish(EVENT_NETWORK_AT_CMD, param);
	}
	else
	{
		AT_BACK_CME_ERR(AT_ERR_INVAL);
	}
}

void ntp_handler(char *cmd, int type, char *data)
{
	if(type == WRITE_CMD || type == EXECUTE_CMD)
	{
		event_user_at_param_t *param = NULL;
		if(init_param_with_url(&param, type == EXECUTE_CMD ? NULL : data))
		{
			AT_BACK_CME_ERR(AT_ERR);
			return;
		}
		param->type = USER_AT_EVT_TYPE_NTP;
		event_publish(EVENT_NETWORK_AT_CMD, param);
	}
	else
	{
		AT_BACK_CME_ERR(AT_ERR_INVAL);
	}
}

void sta_mac_handler(char *cmd, int type, char *data)
{
	uint8_t combo_adv_mac[6] = {0};
	uint8_t mac_str[30] = {0};

	tls_get_mac_addr(combo_adv_mac);

	sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", combo_adv_mac[0], combo_adv_mac[1], 
			combo_adv_mac[2], combo_adv_mac[3], combo_adv_mac[4], combo_adv_mac[5]);
	if (type == EXECUTE_CMD) {
		//AT_BACK_RET_OK(cmd, mac_str);
		atserver_send("\r\n%s:%02x:%02x:%02x:%02x:%02x:%02x\r\nOK\r\n", cmd + 2, 
				combo_adv_mac[0], combo_adv_mac[1], combo_adv_mac[2], 
				combo_adv_mac[3], combo_adv_mac[4], combo_adv_mac[5]);
    }
	else
	{
		AT_BACK_CME_ERR(AT_ERR);
	}
}

void rst_handler(char *cmd, int type, char *data)
{
	if (type == EXECUTE_CMD) {
		AT_BACK_OK();
		aos_msleep(100);
    	aos_reboot();
    }
	else
	{
		AT_BACK_CME_ERR(AT_ERR);
	}
}

void dns_handler(char *cmd, int type, char *data)
{
	uint32_t u_ipaddr = 0;
	char str[30] = {0};

	struct hostent *host = gethostbyname((const char *)data);
	if (host == NULL) {
		printf("host is null\r\n");
		AT_BACK_CME_ERR(AT_ERR);
		return;
	}

	printf("\taddress: %s\n",
                        inet_ntop(host->h_addrtype, host->h_addr, str, sizeof(str)));

	if ((type == EXECUTE_CMD) || (type == WRITE_CMD)) {
		AT_BACK_RET_OK(cmd, inet_ntop(host->h_addrtype, host->h_addr, str, sizeof(str)));
    }
	else
	{
		AT_BACK_CME_ERR(AT_ERR);
	}
}

static aos_dev_t *get_wifi_dev()
{
    static aos_dev_t *wifi_dev = NULL;
    if (wifi_dev == NULL) {
        wifi_dev = device_open_id("wifi", 0);
    }

    return wifi_dev;
}

void link_info_handler(char *cmd, int type, char *data)
{
	ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;

	aos_dev_t *wifi_dev = get_wifi_dev();
    if (wifi_dev == NULL) {
        return;
    }

	/** ifconfig */
    hal_net_get_ipaddr(wifi_dev, &ipaddr, &netmask, &gw);
	if (type == EXECUTE_CMD) {
		atserver_send("\r\n%s:ip:%s\r\n\r\n", cmd + 2, ipaddr_ntoa(&ipaddr));
		atserver_send("\r\n%s:gateway:%s\r\n\r\n", cmd + 2, ipaddr_ntoa(&gw));
		atserver_send("\r\n%s:netmask:%s\r\n\r\n", cmd + 2, ipaddr_ntoa(&netmask));
		AT_BACK_OK();
	} else{
		AT_BACK_CME_ERR(AT_ERR);
	}
	
}

void link_status_handler(char *cmd, int type, char *data)
{
	wifi_ap_record_t ap_info = {0};

	aos_dev_t *wifi_dev = get_wifi_dev();
    if (wifi_dev == NULL) {
        return;
    }

	/** iw dev wlan0 link */
    hal_wifi_sta_get_link_status(wifi_dev, &ap_info);
	if (type == EXECUTE_CMD) {
		AT_BACK_RET_OK_INT(cmd, ap_info.link_status);
	} else {
		AT_BACK_CME_ERR(AT_ERR);
	}
	
}

void wifi_info_handler(char *cmd, int type, char *data)
{
	wifi_ap_record_t ap_info = {0};

	aos_dev_t *wifi_dev = get_wifi_dev();
    if (wifi_dev == NULL) {
        return;
    }

	/** iw dev wlan0 link */
    hal_wifi_sta_get_link_status(wifi_dev, &ap_info);
	if (type == EXECUTE_CMD) {
		atserver_send("\r\n%s:bssid:%02x:%02x:%02x:%02x:%02x:%02x\r\n\r\n", cmd + 2, 
				ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2], 
				ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
		atserver_send("\r\n%s:ssid:%s\r\n\r\n", cmd + 2, ap_info.ssid);
		atserver_send("\r\n%s:channel:%d\r\n\r\n", cmd + 2, ap_info.channel);
		atserver_send("\r\n%s:signal:%d\r\n\r\n", cmd + 2, ap_info.rssi);
		AT_BACK_OK();
	} else {
		AT_BACK_CME_ERR(AT_ERR);
	}
	
}

void wjap_handler(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        char *token = strchr(data, ',');
        int ret = -1;
        if (token) {
            *token = '\0';
            int len = token - data;
            if (len <= 32 && len > 0) {
                ++token;
                if (strlen(token) <= 64) {
                    aos_kv_setstring("wifi_ssid", data);
                    aos_kv_setstring("wifi_psk", token);
                    ret = 0;
					event_publish(EVENT_APP_AT_CMD, (void*)APP_AT_CMD_WJAP);
                }
            }
        }

        if (ret) {
            AT_RESP_CME_ERR(50);
        } else {
            AT_BACK_OK();
        }
    }
}

void wjapd_handler(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        aos_kv_del("wifi_ssid");
        aos_kv_del("wifi_psk");
        AT_BACK_OK();
    }
}

void wjapq_handler(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        char ssid[33];
        char psk[65];
        int rst = aos_kv_getstring("wifi_ssid", ssid, sizeof(ssid));
        if (rst >= 0) {
            rst = aos_kv_getstring("wifi_psk", psk, sizeof(psk));
        }
        if (rst < 0) {
            AT_BACK_ERR();
        } else {
            AT_BACK_RET_OK2(cmd, ssid, psk);
        }
    }
}

void wdap_handler(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
		event_publish(EVENT_APP_AT_CMD, (void*)APP_AT_CMD_WDAP);
    }
	AT_BACK_OK();
}
static void EVENT_NETWORK_AT_CMD_handler(uint32_t event_id, const void *p, void *context)
{
    if (event_id == EVENT_NETWORK_AT_CMD) 
	{
		event_user_at_param_t *param = (event_user_at_param_t *)p;
		if(param->url)
		{
			LOGD(TAG, "url: %s", param->url);
		}
		switch(param->type)
		{
			case USER_AT_EVT_TYPE_HTTP:
			case USER_AT_EVT_TYPE_OTA:
				https_with_url(param);
				break;
			case USER_AT_EVT_TYPE_NTP:
				if (ntp_sync_time(param->url) == 0) {
					struct timeval tv;
					gettimeofday(&tv, NULL);
					AT_BACK_RET_OK_INT("AT+NTP", tv.tv_sec);
				}
				else
				{
					AT_BACK_CME_ERR(AT_ERR);
				}
				break;
			default:
				break;
		}
		if(param->url)
		{
			aos_free(param->url);
			LOGD(TAG, "url %p is free", param->url);
			param->url = NULL;
		}
		aos_free(param);
	}
}
void ver_handler(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
		AT_BACK_RET_OK_INT(cmd, USER_VERSION);
    }
	else
	{
		AT_BACK_CME_ERR(AT_ERR);
	}
}

int at_network_evt_sub(void)
{
    event_subscribe(EVENT_NET_PING_RESULT, event_ping_result_handler, NULL);
    event_subscribe(EVENT_NETWORK_AT_CMD, EVENT_NETWORK_AT_CMD_handler, NULL);
    return 0;
}


