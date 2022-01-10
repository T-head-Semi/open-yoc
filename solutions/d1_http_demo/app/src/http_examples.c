#include <http_client.h>
#include <aos/kernel.h>
#include <aos/debug.h>

#define TAG "https_example"

#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define CHUNKSIZE (256)

int vasprintf(char **ret, const char *fmt, va_list ap)
{
    size_t chunks;
    size_t buflen;
    char * buf;
    int    len;

    chunks = ((strlen(fmt) + 1) / CHUNKSIZE) + 1;
    buflen = chunks * CHUNKSIZE;

    *ret = NULL;
    for (;;) {
        buf = malloc(buflen);
        if (buf == NULL) {
            return -ENOMEM;
        }
        len = vsnprintf(buf, buflen, fmt, ap);

        if (len >= 0 && (size_t)len < buflen - 1) {
            break;
        }
        free(buf);
        buflen = (++chunks) * CHUNKSIZE;

        /*
        * len >= 0 is required for vsnprintf implementations that
        * return -1 for insufficient buffer
        */

        if (len >= 0 && buflen <= (size_t)len) {
            buflen = len + 1;
        }
    }

    if (len > 0 && buf != NULL) {
        // strip the string
        *ret = strdup(buf);
        if (*ret != NULL)
            free(buf);
        else
            *ret = buf;
    }

    return len;
}

int asprintf(char **buf, const char *fmt, ...)
{
    va_list args;
    int     i;

    va_start(args, fmt);
    i = vasprintf(buf, fmt, args);
    va_end(args);
    return i;
}

static char ca_crt_rsa[] = {
"-----BEGIN CERTIFICATE-----\r\n"
"MIIEkjCCA3qgAwIBAgIQCgFBQgAAAVOFc2oLheynCDANBgkqhkiG9w0BAQsFADA/\r\n"
"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\r\n"
"DkRTVCBSb290IENBIFgzMB4XDTE2MDMxNzE2NDA0NloXDTIxMDMxNzE2NDA0Nlow\r\n"
"SjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUxldCdzIEVuY3J5cHQxIzAhBgNVBAMT\r\n"
"GkxldCdzIEVuY3J5cHQgQXV0aG9yaXR5IFgzMIIBIjANBgkqhkiG9w0BAQEFAAOC\r\n"
"AQ8AMIIBCgKCAQEAnNMM8FrlLke3cl03g7NoYzDq1zUmGSXhvb418XCSL7e4S0EF\r\n"
"q6meNQhY7LEqxGiHC6PjdeTm86dicbp5gWAf15Gan/PQeGdxyGkOlZHP/uaZ6WA8\r\n"
"SMx+yk13EiSdRxta67nsHjcAHJyse6cF6s5K671B5TaYucv9bTyWaN8jKkKQDIZ0\r\n"
"Z8h/pZq4UmEUEz9l6YKHy9v6Dlb2honzhT+Xhq+w3Brvaw2VFn3EK6BlspkENnWA\r\n"
"a6xK8xuQSXgvopZPKiAlKQTGdMDQMc2PMTiVFrqoM7hD8bEfwzB/onkxEz0tNvjj\r\n"
"/PIzark5McWvxI0NHWQWM6r6hCm21AvA2H3DkwIDAQABo4IBfTCCAXkwEgYDVR0T\r\n"
"AQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwfwYIKwYBBQUHAQEEczBxMDIG\r\n"
"CCsGAQUFBzABhiZodHRwOi8vaXNyZy50cnVzdGlkLm9jc3AuaWRlbnRydXN0LmNv\r\n"
"bTA7BggrBgEFBQcwAoYvaHR0cDovL2FwcHMuaWRlbnRydXN0LmNvbS9yb290cy9k\r\n"
"c3Ryb290Y2F4My5wN2MwHwYDVR0jBBgwFoAUxKexpHsscfrb4UuQdf/EFWCFiRAw\r\n"
"VAYDVR0gBE0wSzAIBgZngQwBAgEwPwYLKwYBBAGC3xMBAQEwMDAuBggrBgEFBQcC\r\n"
"ARYiaHR0cDovL2Nwcy5yb290LXgxLmxldHNlbmNyeXB0Lm9yZzA8BgNVHR8ENTAz\r\n"
"MDGgL6AthitodHRwOi8vY3JsLmlkZW50cnVzdC5jb20vRFNUUk9PVENBWDNDUkwu\r\n"
"Y3JsMB0GA1UdDgQWBBSoSmpjBH3duubRObemRWXv86jsoTANBgkqhkiG9w0BAQsF\r\n"
"AAOCAQEA3TPXEfNjWDjdGBX7CVW+dla5cEilaUcne8IkCJLxWh9KEik3JHRRHGJo\r\n"
"uM2VcGfl96S8TihRzZvoroed6ti6WqEBmtzw3Wodatg+VyOeph4EYpr/1wXKtx8/\r\n"
"wApIvJSwtmVi4MFU5aMqrSDE6ea73Mj2tcMyo5jMd6jmeWUHK8so/joWUoHOUgwu\r\n"
"X4Po1QYz+3dszkDqMp4fklxBwXRsW10KXzPMTZ+sOPAveyxindmjkW8lGy+QsRlG\r\n"
"PfZ+G6Z6h7mjem0Y+iWlkYcV4PIWL1iwBi8saCbGS5jN2p8M+X+Q7UNKEkROb3N6\r\n"
"KOqkqm57TH2H3eDJAkSnh6/DNFu0Qg==\r\n"
"-----END CERTIFICATE-----\r\n"
" "
};


#define MAX_HTTP_RECV_BUFFER 512

static int e_count = 0;

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
        LOGI(TAG, "HTTP GET Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP GET request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    // POST
    const char *post_data = "field1=value1&field2=value2";
    http_client_set_url(client, "http://httpbin.org/post");
    http_client_set_method(client, HTTP_METHOD_POST);
    http_client_set_post_field(client, post_data, strlen(post_data));
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP POST Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP POST request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    //PUT
    http_client_set_url(client, "http://httpbin.org/put");
    http_client_set_method(client, HTTP_METHOD_PUT);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP PUT Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP PUT request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    //PATCH
    http_client_set_url(client, "http://httpbin.org/patch");
    http_client_set_method(client, HTTP_METHOD_PATCH);
    http_client_set_post_field(client, NULL, 0);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP PATCH Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP PATCH request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    //DELETE
    http_client_set_url(client, "http://httpbin.org/delete");
    http_client_set_method(client, HTTP_METHOD_DELETE);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP DELETE Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP DELETE request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    //HEAD
    http_client_set_url(client, "http://httpbin.org/head");
    http_client_set_method(client, HTTP_METHOD_HEAD);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP HEAD Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP HEAD request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    http_client_cleanup(client);
}

static void http_rest_with_hostname_path()
{
    http_client_config_t config = {
        .host = "httpbin.org",
        .path = "/get",
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);

    // GET
    http_errors_t err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP GET Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP GET request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    // POST
    const char *post_data = "field1=value1&field2=value2";
    http_client_set_url(client, "/post");
    http_client_set_method(client, HTTP_METHOD_POST);
    http_client_set_post_field(client, post_data, strlen(post_data));
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP POST Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP POST request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    //PUT
    http_client_set_url(client, "/put");
    http_client_set_method(client, HTTP_METHOD_PUT);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP PUT Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP PUT request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    //PATCH
    http_client_set_url(client, "/patch");
    http_client_set_method(client, HTTP_METHOD_PATCH);
    http_client_set_post_field(client, NULL, 0);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP PATCH Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP PATCH request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    //DELETE
    http_client_set_url(client, "/delete");
    http_client_set_method(client, HTTP_METHOD_DELETE);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP DELETE Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP DELETE request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    //HEAD
    http_client_set_url(client, "/get");
    http_client_set_method(client, HTTP_METHOD_HEAD);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP HEAD Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "HTTP HEAD request failed: 0x%x @#@@@@@@", (err));
        e_count ++;
    }

    http_client_cleanup(client);
}


static void http_auth_basic()
{
    http_client_config_t config = {
        .url = "http://user:passwd@httpbin.org/basic-auth/user/passwd",
        .event_handler = _http_event_handler,
        .auth_type = HTTP_AUTH_TYPE_BASIC,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP Basic Auth Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count ++;
    }
    http_client_cleanup(client);
}

static void http_auth_basic_redirect()
{
    http_client_config_t config = {
        .url = "http://user:passwd@httpbin.org/basic-auth/user/passwd",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP Basic Auth redirect Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count ++;
    }
    http_client_cleanup(client);
}

static void http_auth_digest()
{
    http_client_config_t config = {
        .url = "http://user:passwd@httpbin.org/digest-auth/auth/user/passwd/MD5/never",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP Digest Auth Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count ++;
    }
    http_client_cleanup(client);
}

// static void https_with_url()
// {
//     http_client_config_t config = {
//         .url = "https://postman-echo.com/post",
//         .event_handler = _http_event_handler,
//         // .cert_pem = ca_crt_rsa,
//         .timeout_ms = 20000,
//         .is_async = true,
//     };
//     http_client_handle_t client = http_client_init(&config);
//     http_errors_t err = http_client_perform(client);

//     if (err == HTTP_CLI_OK) {
//         LOGI(TAG, "HTTPS Status = %d, content_length = %d \r\n",
//                 http_client_get_status_code(client),
//                 http_client_get_content_length(client));
//     } else {
//         LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
//         e_count ++;
//     }
//     http_client_cleanup(client);
// }

// static void https_with_hostname_path()
// {
//     http_client_config_t config = {
//         .host = "www.howsmyssl.com",
//         .path = "/",
//         .transport_type = HTTP_TRANSPORT_OVER_SSL,
//         .event_handler = _http_event_handler,
//         .cert_pem = ca_crt_rsa,
//     };
//     http_client_handle_t client = http_client_init(&config);
//     http_errors_t err = http_client_perform(client);

//     if (err == HTTP_CLI_OK) {
//         LOGI(TAG, "HTTPS Status = %d, content_length = %d \r\n",
//                 http_client_get_status_code(client),
//                 http_client_get_content_length(client));
//     } else {
//         LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
//         e_count ++;
//     }
//     http_client_cleanup(client);
// }

static void http_relative_redirect()
{
    http_client_config_t config = {
        .url = "http://httpbin.org/relative-redirect/3",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP Relative path redirect Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count ++;
    }
    http_client_cleanup(client);
}

static void http_absolute_redirect()
{
    http_client_config_t config = {
        .url = "http://httpbin.org/absolute-redirect/3",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP Absolute path redirect Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count ++;
    }
    http_client_cleanup(client);
}

static void http_redirect_to_https()
{
    http_client_config_t config = {
        .url = "http://httpbin.org/redirect-to?url=https%3A%2F%2Fwww.howsmyssl.com",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP redirect to HTTPS Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count ++;
    }
    http_client_cleanup(client);
}


static void http_download_chunk()
{
    http_client_config_t config = {
        .url = "http://httpbin.org/stream-bytes/8912",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK) {
        LOGI(TAG, "HTTP chunk encoding Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count ++;
    }
    http_client_cleanup(client);
}

static void http_perform_as_stream_reader()
{
    char *buffer = aos_malloc(MAX_HTTP_RECV_BUFFER + 1);
    if (buffer == NULL) {
        LOGE(TAG, "Cannot aos_malloc http receive buffer");
        return;
    }
    http_client_config_t config = {
        .url = "http://httpbin.org/get",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err;
    if ((err = http_client_open(client, 0)) != HTTP_CLI_OK) {
        LOGE(TAG, "Failed to open HTTP connection: 0x%x @#@@@@@@", (err));
        aos_free(buffer);
        return;
    }
    int content_length =  http_client_fetch_headers(client);
    int total_read_len = 0, read_len;
    if (total_read_len < content_length && content_length <= MAX_HTTP_RECV_BUFFER) {
        read_len = http_client_read(client, buffer, content_length);
        if (read_len <= 0) {
            LOGE(TAG, "Error read data");
        }
        buffer[read_len] = 0;
        LOGD(TAG, "read_len = %d", read_len);
    }
    LOGI(TAG, "HTTP Stream reader Status = %d, content_length = %d \r\n",
                    http_client_get_status_code(client),
                    http_client_get_content_length(client));
    http_client_close(client);
    http_client_cleanup(client);
    aos_free(buffer);
}

static void https_async()
{
    http_client_config_t config = {
        .url = "https://postman-echo.com/post",
        .event_handler = _http_event_handler,
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
        LOGI(TAG, "HTTPS Status = %d, content_length = %d \r\n",
                http_client_get_status_code(client),
                http_client_get_content_length(client));
    } else {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count ++;
    }
    http_client_cleanup(client);
}

static void http_task(void *priv)
{
    ca_crt_rsa[sizeof(ca_crt_rsa) - 1] = 0;
    http_rest_with_url();
    http_rest_with_hostname_path();
    http_auth_basic();
    http_auth_basic_redirect();
    http_auth_digest();
    http_relative_redirect();
    http_absolute_redirect();
    // https_with_url();
    // https_with_hostname_path();
    http_redirect_to_https();
    http_download_chunk();
    http_perform_as_stream_reader();
    https_async();

    LOGI(TAG, "Finish http example [%d]", e_count);
}

void test_https(void)
{
    aos_task_t task;

    aos_task_new_ext(&task, "http_test", http_task, NULL, 10240, 32);
}