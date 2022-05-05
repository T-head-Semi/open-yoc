#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <http_client.h>
#include <aos/kernel.h>
#include <aos/debug.h>
#include <aos/cli.h>

#define TAG "example"

static char ca_crt_rsa[] = {
"-----BEGIN CERTIFICATE-----\r\n"
"MIIEdTCCA12gAwIBAgIJAKcOSkw0grd/MA0GCSqGSIb3DQEBCwUAMGgxCzAJBgNV\r\n"
"BAYTAlVTMSUwIwYDVQQKExxTdGFyZmllbGQgVGVjaG5vbG9naWVzLCBJbmMuMTIw\r\n"
"MAYDVQQLEylTdGFyZmllbGQgQ2xhc3MgMiBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0\r\n"
"eTAeFw0wOTA5MDIwMDAwMDBaFw0zNDA2MjgxNzM5MTZaMIGYMQswCQYDVQQGEwJV\r\n"
"UzEQMA4GA1UECBMHQXJpem9uYTETMBEGA1UEBxMKU2NvdHRzZGFsZTElMCMGA1UE\r\n"
"ChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjE7MDkGA1UEAxMyU3RhcmZp\r\n"
"ZWxkIFNlcnZpY2VzIFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5IC0gRzIwggEi\r\n"
"MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDVDDrEKvlO4vW+GZdfjohTsR8/\r\n"
"y8+fIBNtKTrID30892t2OGPZNmCom15cAICyL1l/9of5JUOG52kbUpqQ4XHj2C0N\r\n"
"Tm/2yEnZtvMaVq4rtnQU68/7JuMauh2WLmo7WJSJR1b/JaCTcFOD2oR0FMNnngRo\r\n"
"Ot+OQFodSk7PQ5E751bWAHDLUu57fa4657wx+UX2wmDPE1kCK4DMNEffud6QZW0C\r\n"
"zyyRpqbn3oUYSXxmTqM6bam17jQuug0DuDPfR+uxa40l2ZvOgdFFRjKWcIfeAg5J\r\n"
"Q4W2bHO7ZOphQazJ1FTfhy/HIrImzJ9ZVGif/L4qL8RVHHVAYBeFAlU5i38FAgMB\r\n"
"AAGjgfAwge0wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0O\r\n"
"BBYEFJxfAN+qAdcwKziIorhtSpzyEZGDMB8GA1UdIwQYMBaAFL9ft9HO3R+G9FtV\r\n"
"rNzXEMIOqYjnME8GCCsGAQUFBwEBBEMwQTAcBggrBgEFBQcwAYYQaHR0cDovL28u\r\n"
"c3MyLnVzLzAhBggrBgEFBQcwAoYVaHR0cDovL3guc3MyLnVzL3guY2VyMCYGA1Ud\r\n"
"HwQfMB0wG6AZoBeGFWh0dHA6Ly9zLnNzMi51cy9yLmNybDARBgNVHSAECjAIMAYG\r\n"
"BFUdIAAwDQYJKoZIhvcNAQELBQADggEBACMd44pXyn3pF3lM8R5V/cxTbj5HD9/G\r\n"
"VfKyBDbtgB9TxF00KGu+x1X8Z+rLP3+QsjPNG1gQggL4+C/1E2DUBc7xgQjB3ad1\r\n"
"l08YuW3e95ORCLp+QCztweq7dp4zBncdDQh/U90bZKuCJ/Fp1U1ervShw3WnWEQt\r\n"
"8jxwmKy6abaVd38PMV4s/KCHOkdp8Hlf9BRUpJVeEXgSYCfOn8J3/yNTd126/+pZ\r\n"
"59vPr5KW7ySaNRB6nJHGDn2Z9j8Z3/VyVOEVqQdZe4O/Ui5GjLIAZHYcSNPYeehu\r\n"
"VsyuLAOQ1xk4meTKCRlb/weWsKh/NEnfVqn3sF/tM+2MR7cwA130A4w=\r\n"
"-----END CERTIFICATE-----\r\n"
" "
};


#define MAX_HTTP_RECV_BUFFER 512


int _http_event_handler(http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!http_client_is_chunked_response(evt->client)) {
                // Write out data
                // printf("%.*s", evt->data_len, (char*)evt->data);
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return 0;
}

static void http_get_with_url(const char *url)
{
    if (url == NULL) {
        LOGE(TAG, "Please input the valid url.");
        return;
    }
    http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    if (!client) {
        return;
    }

    // GET
    http_errors_t err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
        LOGI(TAG, "++++++++++++++ HTTP GET TEST OK\n");
    } else {
        LOGE(TAG, "HTTP GET request failed: 0x%x @#@@@@@@", (err));
    }
}

static void http_rest_with_url()
{
    http_client_config_t config = {
        .url = "http://httpbin.org/get",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);

    // GET
    http_errors_t err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
        LOGI(TAG, "++++++++++++++ HTTP GET TEST OK\n");
    } else {
        LOGE(TAG, "HTTP GET request failed: 0x%x @#@@@@@@", (err));
    }

    // POST
    const char *post_data = "field1=value1&field2=value2";
    http_client_set_url(client, "http://httpbin.org/post");
    http_client_set_method(client, HTTP_METHOD_POST);
    http_client_set_post_field(client, post_data, strlen(post_data));
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
        LOGI(TAG, "++++++++++++++ HTTP POST TEST OK\n");
    } else {
        LOGE(TAG, "HTTP POST request failed: 0x%x @#@@@@@@", (err));
    }

    //PUT
    http_client_set_url(client, "http://httpbin.org/put");
    http_client_set_method(client, HTTP_METHOD_PUT);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP PUT Status = %d, content_length = %d",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
        LOGI(TAG, "++++++++++++++ HTTP PUT TEST OK\n");
    } else {
        LOGE(TAG, "HTTP PUT request failed: 0x%x @#@@@@@@", (err));
    }

    //PATCH
    http_client_set_url(client, "http://httpbin.org/patch");
    http_client_set_method(client, HTTP_METHOD_PATCH);
    http_client_set_post_field(client, NULL, 0);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP PATCH Status = %d, content_length = %d",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
        LOGI(TAG, "++++++++++++++ HTTP PATCH TEST OK\n");
    } else {
        LOGE(TAG, "HTTP PATCH request failed: 0x%x @#@@@@@@", (err));
    }

    //DELETE
    http_client_set_url(client, "http://httpbin.org/delete");
    http_client_set_method(client, HTTP_METHOD_DELETE);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP DELETE Status = %d, content_length = %d",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
        LOGI(TAG, "++++++++++++++ HTTP DELETE TEST OK\n");
    } else {
        LOGE(TAG, "HTTP DELETE request failed: 0x%x @#@@@@@@", (err));
    }

    //HEAD
    http_client_set_url(client, "http://httpbin.org/head");
    http_client_set_method(client, HTTP_METHOD_HEAD);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP HEAD Status = %d, content_length = %d",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
        LOGI(TAG, "++++++++++++++ HTTP HEAD TEST OK\n");
    } else {
        LOGE(TAG, "HTTP HEAD request failed: 0x%x @#@@@@@@", (err));
    }

    http_client_cleanup(client);
}

static void https_async()
{
    http_client_config_t config = {
        .url = "https://postman-echo.com/post",
        .event_handler = _http_event_handler,
        .cert_pem = ca_crt_rsa,
        .is_async = true,
        .timeout_ms = 20000,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err;
    const char *post_data = "Using a Palantír requires a person with great strength of will and wisdom. The Palantíri were meant to "
                            "be used by the Dúnedain to communicate throughout the Realms in Exile. During the War of the Ring, "
                            "the Palantíri were used by many individuals. Sauron used the Ithil-stone to take advantage of the users "
                            "of the other two stones, the Orthanc-stone and Anor-stone, but was also susceptible to deception himself.";
    http_client_set_method(client, HTTP_METHOD_POST);
    http_client_set_post_field(client, post_data, strlen(post_data));
    while (1) {
        err = http_client_perform(client);
        if (err != EAGAIN) {
            break;
        }
    }
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTPS Status = %d, content_length = %d",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
        LOGI(TAG, "++++++++++++++ HTTPS TEST OK\n");
    } else {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
    }
    http_client_cleanup(client);
}

void show_help(void)
{
    aos_cli_printf("Usage: web http|https\n");
    aos_cli_printf("     : web http get <url>, such as web http get http://occ.t-head.cn \n");
}

static void webclient_main(int argc, char **argv)
{
    ca_crt_rsa[sizeof(ca_crt_rsa) - 1] = 0;

    if (argc < 2) {
        show_help();
        return;
    }
    if (strcmp(argv[1], "http") == 0) {
        if (argc == 4) {
            if (strcmp(argv[2], "get") == 0) {
                const char *url = argv[3];
                http_get_with_url(url);
                return;
            }
        } else if (argc == 2) {
            http_rest_with_url();
            return;
        }
    } else if (strcmp(argv[1], "https") == 0) {
        if (argc == 2) {
            https_async();
            return;
        }
    }
    show_help();
}

ALIOS_CLI_CMD_REGISTER(webclient_main, web, web cmd);
