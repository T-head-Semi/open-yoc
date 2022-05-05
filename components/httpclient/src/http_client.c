// Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "http_client.h"
#include "http_header.h"
#include "http_utils.h"
#include "http_parser.h"
#include "http_auth.h"
#include "transport/transport_tcp.h"
#include "transport/tperrors.h"
#include "ulog/ulog.h"
#include "aos/debug.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef CONFIG_USING_TLS
#include "transport/transport_ssl.h"
#endif

static const char *TAG = "HTTP_CLIENT";

#define ERR_HTTP_BASE               (0x3000)                /*!< Starting number of HTTP error codes */
#define ERR_HTTP_MAX_REDIRECT       (ERR_HTTP_BASE + 1)     /*!< The error exceeds the number of HTTP redirects */
#define ERR_HTTP_CONNECT            (ERR_HTTP_BASE + 2)     /*!< Error open the HTTP connection */
#define ERR_HTTP_WRITE_DATA         (ERR_HTTP_BASE + 3)     /*!< Error write HTTP data */
#define ERR_HTTP_FETCH_HEADER       (ERR_HTTP_BASE + 4)     /*!< Error read HTTP header from server */
#define ERR_HTTP_INVALID_TRANSPORT  (ERR_HTTP_BASE + 5)     /*!< There are no transport support for the input scheme */
#define ERR_HTTP_CONNECTING         (ERR_HTTP_BASE + 6)     /*!< HTTP connection hasn't been established yet */
#define ERR_HTTP_EAGAIN             EAGAIN//(ERR_HTTP_BASE + 7)     /*!< Mapping of errno EAGAIN to web_err_t */

/**
 * HTTP Buffer
 */
typedef struct {
    char *data;         /*!< The HTTP data received from the server */
    int len;            /*!< The HTTP data len received from the server */
    char *raw_data;     /*!< The HTTP data after decoding */
    int raw_len;        /*!< The HTTP data len after decoding */
    char *output_ptr;   /*!< The destination address of the data to be copied to after decoding */
} http_buffer_t;

/**
 * private HTTP Data structure
 */
typedef struct {
    http_header_handle_t headers;       /*!< http header */
    http_buffer_t       *buffer;        /*!< data buffer as linked list */
    int                 status_code;    /*!< status code (integer) */
    int                 content_length; /*!< data length */
    int                 data_offset;    /*!< offset to http data (Skip header) */
    int                 data_process;   /*!< data processed */
    int                 method;         /*!< http method */
    bool                is_chunked;
} http_data_t;

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

typedef enum {
    HTTP_STATE_UNINIT = 0,
    HTTP_STATE_INIT,
    HTTP_STATE_CONNECTED,
    HTTP_STATE_REQ_COMPLETE_HEADER,
    HTTP_STATE_REQ_COMPLETE_DATA,
    HTTP_STATE_RES_COMPLETE_HEADER,
    HTTP_STATE_RES_COMPLETE_DATA,
    HTTP_STATE_CLOSE
} http_state_t;
/**
 * HTTP client class
 */
struct http_client {
    int                         redirect_counter;
    int                         max_redirection_count;
    int                         process_again;
    struct http_parser          *parser;
    struct http_parser_settings *parser_settings;
    transport_list_handle_t     transport_list;
    transport_handle_t          transport;
    http_data_t                 *request;
    http_data_t                 *response;
    void                        *user_data;
    http_auth_data_t            *auth_data;
    char                        *post_data;
    char                        *location;
    char                        *auth_header;
    char                        *current_header_key;
    char                        *current_header_value;
    int                         post_len;
    connection_info_t           connection_info;
    bool                        is_chunk_complete;
    http_state_t                state;
    http_event_handle_cb        event_handler;
    int                         timeout_ms;
    int                         buffer_size;
    bool                        disable_auto_redirect;
    http_client_event_t         event;
    int                         data_written_index;
    int                         data_write_left;
    bool                        first_line_prepared;
    int                         header_index;
    bool                        is_async;
};

typedef struct http_client http_client_t;

static web_err_t _clear_connection_info(http_client_handle_t client);
/**
 * Default settings
 */
#define DEFAULT_HTTP_PORT (80)
#define DEFAULT_HTTPS_PORT (443)

#define ASYNC_TRANS_CONNECT_FAIL -1
#define ASYNC_TRANS_CONNECTING 0
#define ASYNC_TRANS_CONNECT_PASS 1

static const char *DEFAULT_HTTP_USER_AGENT = "CK HTTP Client/1.0";
static const char *DEFAULT_HTTP_PROTOCOL = "HTTP/1.1";
static const char *DEFAULT_HTTP_PATH = "/";
static int DEFAULT_MAX_REDIRECT = 10;
static int DEFAULT_TIMEOUT_MS = 5000;

static const char *HTTP_METHOD_MAPPING[] = {
    "GET",
    "POST",
    "PUT",
    "PATCH",
    "DELETE",
    "HEAD",
    "NOTIFY",
    "SUBSCRIBE",
    "UNSUBSCRIBE",
    "OPTIONS"
};

static web_err_t http_client_request_send(http_client_handle_t client, int write_len);
static web_err_t http_client_connect(http_client_handle_t client);
static web_err_t http_client_send_post_data(http_client_handle_t client);

static web_err_t http_dispatch_event(http_client_t *client, http_client_event_id_t event_id, void *data, int len)
{
    http_client_event_t *event = &client->event;

    if (client->event_handler) {
        event->event_id = event_id;
        event->user_data = client->user_data;
        event->data = data;
        event->data_len = len;
        return client->event_handler(event);
    }
    return WEB_OK;
}

static int http_on_message_begin(http_parser *parser)
{
    http_client_t *client = parser->data;
    LOGD(TAG, "on_message_begin");

    client->response->is_chunked = false;
    client->is_chunk_complete = false;
    return 0;
}

static int http_on_url(http_parser *parser, const char *at, size_t length)
{
    LOGD(TAG, "http_on_url");
    return 0;
}

static int http_on_status(http_parser *parser, const char *at, size_t length)
{
    return 0;
}

static int http_on_header_field(http_parser *parser, const char *at, size_t length)
{
    http_client_t *client = parser->data;
    http_utils_assign_string(&client->current_header_key, at, length);

    return 0;
}

static int http_on_header_value(http_parser *parser, const char *at, size_t length)
{
    http_client_handle_t client = parser->data;
    if (client->current_header_key == NULL) {
        return 0;
    }
    if (strcasecmp(client->current_header_key, "Location") == 0) {
        http_utils_assign_string(&client->location, at, length);
    } else if (strcasecmp(client->current_header_key, "Transfer-Encoding") == 0
               && memcmp(at, "chunked", length) == 0) {
        client->response->is_chunked = true;
    } else if (strcasecmp(client->current_header_key, "WWW-Authenticate") == 0) {
        http_utils_assign_string(&client->auth_header, at, length);
    }
    http_utils_assign_string(&client->current_header_value, at, length);

    LOGD(TAG, "HEADER=%s:%s", client->current_header_key, client->current_header_value);
    client->event.header_key = client->current_header_key;
    client->event.header_value = client->current_header_value;
    http_dispatch_event(client, HTTP_EVENT_ON_HEADER, NULL, 0);
    free(client->current_header_key);
    free(client->current_header_value);
    client->current_header_key = NULL;
    client->current_header_value = NULL;
    return 0;
}

static int http_on_headers_complete(http_parser *parser)
{
    http_client_handle_t client = parser->data;
    client->response->status_code = parser->status_code;
    client->response->data_offset = parser->nread;
    client->response->content_length = parser->content_length;
    client->response->data_process = 0;
    LOGD(TAG, "http_on_headers_complete, status=%d, offset=%d, nread=%d", parser->status_code, client->response->data_offset, parser->nread);
    client->state = HTTP_STATE_RES_COMPLETE_HEADER;
    return 0;
}

static int http_on_body(http_parser *parser, const char *at, size_t length)
{
    http_client_t *client = parser->data;
    // LOGD(TAG, "http_on_body %d", length);
    client->response->buffer->raw_data = (char *)at;
    if (client->response->buffer->output_ptr) {
        memcpy(client->response->buffer->output_ptr, (char *)at, length);
        client->response->buffer->output_ptr += length;
    }

    client->response->data_process += length;
    client->response->buffer->raw_len += length;
    http_dispatch_event(client, HTTP_EVENT_ON_DATA, (void *)at, length);
    return 0;
}

static int http_on_message_complete(http_parser *parser)
{
    LOGD(TAG, "http_on_message_complete, parser=0x%lx", (unsigned long)parser);
    http_client_handle_t client = parser->data;
    client->is_chunk_complete = true;
    return 0;
}

static int http_on_chunk_complete(http_parser *parser)
{
    LOGD(TAG, "http_on_chunk_complete");
    return 0;
}

web_err_t http_client_set_header(http_client_handle_t client, const char *key, const char *value)
{
    return http_header_set(client->request->headers, key, value);
}

web_err_t http_client_get_header(http_client_handle_t client, const char *key, char **value)
{
    return http_header_get(client->request->headers, key, value);
}

web_err_t http_client_delete_header(http_client_handle_t client, const char *key)
{
    return http_header_delete(client->request->headers, key);
}

web_err_t http_client_get_username(http_client_handle_t client, char **value)
{
    if (client == NULL || value == NULL) {
        LOGE(TAG, "client or value must not be NULL");
        return WEB_ERR_INVALID_ARG;
    }
    *value = client->connection_info.username;
    return WEB_OK;
}

web_err_t http_client_get_password(http_client_handle_t client, char **value)
{
    if (client == NULL || value == NULL) {
        LOGE(TAG, "client or value must not be NULL");
        return WEB_ERR_INVALID_ARG;
    }
    *value = client->connection_info.password;
    return WEB_OK;
}

static web_err_t _set_config(http_client_handle_t client, const http_client_config_t *config)
{
    client->connection_info.method = config->method;
    client->connection_info.port = config->port;
    client->connection_info.auth_type = config->auth_type;
    client->event_handler = config->event_handler;
    client->timeout_ms = config->timeout_ms;
    client->max_redirection_count = config->max_redirection_count;
    client->user_data = config->user_data;
    client->buffer_size = config->buffer_size;
    client->disable_auto_redirect = config->disable_auto_redirect;

    if (config->buffer_size == 0) {
        client->buffer_size = DEFAULT_HTTP_BUF_SIZE;
    }

    if (client->max_redirection_count == 0) {
        client->max_redirection_count = DEFAULT_MAX_REDIRECT;
    }

    if (config->path) {
        client->connection_info.path = strdup(config->path);
    } else {
        client->connection_info.path = strdup(DEFAULT_HTTP_PATH);
    }

    HTTP_MEM_CHECK(TAG, client->connection_info.path, {
        return WEB_ERR_NO_MEM;
    });

    if (config->host) {
        client->connection_info.host = strdup(config->host);

        HTTP_MEM_CHECK(TAG, client->connection_info.host, {
            _clear_connection_info(client);
            return WEB_ERR_NO_MEM;
        });
    }

    if (config->query) {
        client->connection_info.query = strdup(config->query);
        HTTP_MEM_CHECK(TAG, client->connection_info.query, {
            _clear_connection_info(client);
            return WEB_ERR_NO_MEM;
        });
    }

    if (config->username) {
        client->connection_info.username = strdup(config->username);
        HTTP_MEM_CHECK(TAG, client->connection_info.username, {
            _clear_connection_info(client);
            return WEB_ERR_NO_MEM;
        });
    }

    if (config->password) {
        client->connection_info.password = strdup(config->password);
        HTTP_MEM_CHECK(TAG, client->connection_info.password, {
            _clear_connection_info(client);
            return WEB_ERR_NO_MEM;
        });
    }

    if (config->transport_type == HTTP_TRANSPORT_OVER_SSL) {
        http_utils_assign_string(&client->connection_info.scheme, "https", 0);
        if (client->connection_info.port == 0) {
            client->connection_info.port = DEFAULT_HTTPS_PORT;
        }
    } else {
        http_utils_assign_string(&client->connection_info.scheme, "http", 0);
        if (client->connection_info.port == 0) {
            client->connection_info.port = DEFAULT_HTTP_PORT;
        }
    }
    if (client->timeout_ms == 0) {
        client->timeout_ms = DEFAULT_TIMEOUT_MS;
    }
    if (config->is_async) {
        client->is_async = true;
    }

    return WEB_OK;
}

static web_err_t _clear_connection_info(http_client_handle_t client)
{
    free(client->connection_info.path);
    free(client->connection_info.host);
    free(client->connection_info.query);
    free(client->connection_info.username);
    if (client->connection_info.password) {
        memset(client->connection_info.password, 0, strlen(client->connection_info.password));
        free(client->connection_info.password);
    }
    free(client->connection_info.scheme);
    free(client->connection_info.url);
    memset(&client->connection_info, 0, sizeof(connection_info_t));
    return WEB_OK;
}

static web_err_t _clear_auth_data(http_client_handle_t client)
{
    if (client->auth_data == NULL) {
        return WEB_FAIL;
    }

    free(client->auth_data->method);
    free(client->auth_data->realm);
    free(client->auth_data->algorithm);
    free(client->auth_data->qop);
    free(client->auth_data->nonce);
    free(client->auth_data->opaque);
    memset(client->auth_data, 0, sizeof(http_auth_data_t));
    return WEB_OK;
}

static web_err_t http_client_prepare(http_client_handle_t client)
{
    client->process_again = 0;
    client->response->data_process = 0;
    client->first_line_prepared = false;
    http_parser_init(client->parser, HTTP_RESPONSE);
    if (client->connection_info.username) {
        char *auth_response = NULL;

        if (client->connection_info.auth_type == HTTP_AUTH_TYPE_BASIC) {
            auth_response = http_auth_basic(client->connection_info.username, client->connection_info.password);
        }
        else if (client->connection_info.auth_type == HTTP_AUTH_TYPE_DIGEST && client->auth_data) {
            client->auth_data->uri = client->connection_info.path;
            client->auth_data->cnonce = ((uint64_t)rand() << 32) + rand();
            auth_response = http_auth_digest(client->connection_info.username, client->connection_info.password, client->auth_data);
            client->auth_data->nc ++;
        }

        if (auth_response) {
            LOGD(TAG, "auth_response=%s", auth_response);
            http_client_set_header(client, "Authorization", auth_response);
            free(auth_response);
        }
    }
    return WEB_OK;
}

http_client_handle_t http_client_init(const http_client_config_t *config)
{

    http_client_handle_t client;
    transport_handle_t tcp;
    bool _success;

    _success = (
                   (client                         = calloc(1, sizeof(http_client_t)))           &&
                   (client->parser                 = calloc(1, sizeof(struct http_parser)))          &&
                   (client->parser_settings        = calloc(1, sizeof(struct http_parser_settings))) &&
                   (client->auth_data              = calloc(1, sizeof(http_auth_data_t)))        &&
                   (client->request                = calloc(1, sizeof(http_data_t)))             &&
                   (client->request->headers       = http_header_init())                             &&
                   (client->request->buffer        = calloc(1, sizeof(http_buffer_t)))           &&
                   (client->response               = calloc(1, sizeof(http_data_t)))             &&
                   (client->response->headers      = http_header_init())                             &&
                   (client->response->buffer       = calloc(1, sizeof(http_buffer_t)))
               );

    if (!_success) {
        LOGE(TAG, "Error allocate memory");
        goto error;
    }

    _success = (
                   (client->transport_list = transport_list_init()) &&
                   (tcp = transport_tcp_init()) &&
                   (transport_set_default_port(tcp, DEFAULT_HTTP_PORT) == WEB_OK) &&
                   (transport_list_add(client->transport_list, tcp, "http") == WEB_OK)
               );
    if (!_success) {
        LOGE(TAG, "Error initialize transport");
        goto error;
    }
#ifdef CONFIG_USING_TLS
    transport_handle_t ssl;
    _success = (
                   (ssl = transport_ssl_init()) &&
                   (transport_set_default_port(ssl, DEFAULT_HTTPS_PORT) == WEB_OK) &&
                   (transport_list_add(client->transport_list, ssl, "https") == WEB_OK)
               );

    if (!_success) {
        LOGE(TAG, "Error initialize SSL Transport");
        goto error;
    }

    if (config->use_global_ca_store == true) {
        transport_ssl_enable_global_ca_store(ssl);
    } else if (config->cert_pem) {
        transport_ssl_set_cert_data(ssl, config->cert_pem, strlen(config->cert_pem));
    }

    if (config->client_cert_pem) {
        transport_ssl_set_client_cert_data(ssl, config->client_cert_pem, strlen(config->client_cert_pem));
    }

    if (config->client_key_pem) {
        transport_ssl_set_client_key_data(ssl, config->client_key_pem, strlen(config->client_key_pem));
    }
#endif

    if (_set_config(client, config) != WEB_OK) {
        LOGE(TAG, "Error set configurations");
        goto error;
    }
    _success = (
                   (client->request->buffer->data  = malloc(client->buffer_size))  &&
                   (client->response->buffer->data = malloc(client->buffer_size))
               );

    if (!_success) {
        LOGE(TAG, "Allocation failed");
        goto error;
    }

    if (config->host != NULL && config->path != NULL) {
        _success = (
            (http_client_set_header(client, "User-Agent", DEFAULT_HTTP_USER_AGENT) == WEB_OK) &&
            (http_client_set_header(client, "Host", client->connection_info.host) == WEB_OK)
        );

        if (!_success) {
            LOGE(TAG, "Error while setting default configurations");
            goto error;
        }
    } else if (config->url != NULL) {
        _success = (
                    (http_client_set_url(client, config->url) == WEB_OK) &&
                    (http_client_set_header(client, "User-Agent", DEFAULT_HTTP_USER_AGENT) == WEB_OK) &&
                    (http_client_set_header(client, "Host", client->connection_info.host) == WEB_OK)
                );

        if (!_success) {
            LOGE(TAG, "Error while setting default configurations");
            goto error;
        }
    } else {
        LOGE(TAG, "config should have either URL or host & path");
        goto error;
    }

    client->parser_settings->on_message_begin = http_on_message_begin;
    client->parser_settings->on_url = http_on_url;
    client->parser_settings->on_status = http_on_status;
    client->parser_settings->on_header_field = http_on_header_field;
    client->parser_settings->on_header_value = http_on_header_value;
    client->parser_settings->on_headers_complete = http_on_headers_complete;
    client->parser_settings->on_body = http_on_body;
    client->parser_settings->on_message_complete = http_on_message_complete;
    client->parser_settings->on_chunk_complete = http_on_chunk_complete;
    client->parser->data = client;
    client->event.client = client;

    client->state = HTTP_STATE_INIT;
    return client;
error:
    http_client_cleanup(client);
    return NULL;
}

web_err_t http_client_cleanup(http_client_handle_t client)
{
    if (client == NULL) {
        return WEB_FAIL;
    }
    http_client_close(client);
    transport_list_destroy(client->transport_list);
    http_header_destroy(client->request->headers);
    free(client->request->buffer->data);
    free(client->request->buffer);
    free(client->request);
    http_header_destroy(client->response->headers);
    free(client->response->buffer->data);
    free(client->response->buffer);
    free(client->response);

    free(client->parser);
    free(client->parser_settings);
    _clear_connection_info(client);
    _clear_auth_data(client);
    free(client->auth_data);
    free(client->current_header_key);
    free(client->location);
    free(client->auth_header);
    free(client);
    return WEB_OK;
}

http_errors_t http_client_set_redirection(http_client_handle_t client)
{
    if (client == NULL) {
        return HTTP_CLI_ERR_INVALID_ARG;
    }
    if (client->location == NULL) {
        return HTTP_CLI_ERR_INVALID_ARG;
    }
    LOGD(TAG, "Redirect to %s", client->location);
    return http_client_set_url(client, client->location);
}

static web_err_t http_check_response(http_client_handle_t client)
{
    char *auth_header = NULL;

    if (client->redirect_counter >= client->max_redirection_count || client->disable_auto_redirect) {
        LOGE(TAG, "Error, reach max_redirection_count count=%d", client->redirect_counter);
        return ERR_HTTP_MAX_REDIRECT;
    }
    switch (client->response->status_code) {
        case HttpStatus_MovedPermanently:
        case HttpStatus_Found:
        case HttpStatus_TemporaryRedirect:
            LOGI(TAG, "Redirect to %s", client->location);
            http_client_set_url(client, client->location);
            client->redirect_counter ++;
            client->process_again = 1;
            break;
        case HttpStatus_Unauthorized:
            auth_header = client->auth_header;
            if (auth_header) {
                http_utils_trim_whitespace(&auth_header);
                LOGD(TAG, "UNAUTHORIZED: %s", auth_header);
                client->redirect_counter ++;
                if (http_utils_str_starts_with(auth_header, "Digest") == 0) {
                    LOGD(TAG, "type = Digest");
                    client->connection_info.auth_type = HTTP_AUTH_TYPE_DIGEST;
                } else if (http_utils_str_starts_with(auth_header, "Basic") == 0) {
                    LOGD(TAG, "type = Basic");
                    client->connection_info.auth_type = HTTP_AUTH_TYPE_BASIC;
                } else {
                    client->connection_info.auth_type = HTTP_AUTH_TYPE_NONE;
                    LOGE(TAG, "This authentication method is not supported: %s", auth_header);
                    break;
                }

                _clear_auth_data(client);

                client->auth_data->method = strdup(HTTP_METHOD_MAPPING[client->connection_info.method]);

                client->auth_data->nc = 1;
                client->auth_data->realm = http_utils_get_string_between(auth_header, "realm=\"", "\"");
                client->auth_data->algorithm = http_utils_get_string_between(auth_header, "algorithm=", ",");
                client->auth_data->qop = http_utils_get_string_between(auth_header, "qop=\"", "\"");
                client->auth_data->nonce = http_utils_get_string_between(auth_header, "nonce=\"", "\"");
                client->auth_data->opaque = http_utils_get_string_between(auth_header, "opaque=\"", "\"");
                client->process_again = 1;
            } else {
                client->connection_info.auth_type = HTTP_AUTH_TYPE_NONE;
                LOGW(TAG, "This request requires authentication, but does not provide header information for that");
            }
    }
    return WEB_OK;
}

web_err_t http_client_set_url(http_client_handle_t client, const char *url)
{
    char *old_host = NULL;
    char *old_path = NULL;
    struct http_parser_url purl;
    int old_port;

    if (client == NULL || url == NULL) {
        LOGE(TAG, "client or url must not NULL");
        return WEB_ERR_INVALID_ARG;
    }

    http_parser_url_init(&purl);

    int parser_status = http_parser_parse_url(url, strlen(url), 0, &purl);

    if (parser_status != 0) {
        LOGE(TAG, "Error parse url %s", url);
        return WEB_ERR_INVALID_ARG;
    }
    if (client->connection_info.host) {
        old_host = strdup(client->connection_info.host);
    }
    old_port = client->connection_info.port;

    // Whether the passed url is absolute or is just a path
    bool is_absolute_url = (bool) purl.field_data[UF_HOST].len;

    if (is_absolute_url) {
        http_utils_assign_string(&client->connection_info.host, url + purl.field_data[UF_HOST].off, purl.field_data[UF_HOST].len);
        HTTP_MEM_CHECK(TAG, client->connection_info.host, {
            if (old_host) free(old_host);
            return WEB_ERR_NO_MEM;
        });
    }
    // Close the connection if host was changed
    if (old_host && client->connection_info.host
            && strcasecmp(old_host, (const void *)client->connection_info.host) != 0) {
        LOGD(TAG, "New host assign = %s", client->connection_info.host);
        if (http_client_set_header(client, "Host", client->connection_info.host) != WEB_OK) {
            free(old_host);
            return WEB_ERR_NO_MEM;
        }
        http_client_close(client);
    }

    if (old_host) {
        free(old_host);
        old_host = NULL;
    }

    if (purl.field_data[UF_SCHEMA].len) {
        http_utils_assign_string(&client->connection_info.scheme, url + purl.field_data[UF_SCHEMA].off, purl.field_data[UF_SCHEMA].len);
        HTTP_MEM_CHECK(TAG, client->connection_info.scheme, return WEB_ERR_NO_MEM);

        if (strcasecmp(client->connection_info.scheme, "http") == 0) {
            client->connection_info.port = DEFAULT_HTTP_PORT;
        } else if (strcasecmp(client->connection_info.scheme, "https") == 0) {
            client->connection_info.port = DEFAULT_HTTPS_PORT;
        }
    }

    if (purl.field_data[UF_PORT].len) {
        client->connection_info.port = strtol((const char*)(url + purl.field_data[UF_PORT].off), NULL, 10);
    }

    if (old_port != client->connection_info.port) {
        http_client_close(client);
    }

    if (purl.field_data[UF_USERINFO].len) {
        char *user_info = NULL;
        http_utils_assign_string(&user_info, url + purl.field_data[UF_USERINFO].off, purl.field_data[UF_USERINFO].len);
        if (user_info) {
            char *username = user_info;
            char *password = strchr(user_info, ':');
            if (password) {
                *password = 0;
                password ++;
                http_utils_assign_string(&client->connection_info.password, password, 0);
                HTTP_MEM_CHECK(TAG, client->connection_info.password, return WEB_ERR_NO_MEM);
            }
            http_utils_assign_string(&client->connection_info.username, username, 0);
            HTTP_MEM_CHECK(TAG, client->connection_info.username, return WEB_ERR_NO_MEM);
            free(user_info);
        } else {
            return WEB_ERR_NO_MEM;
        }
    } else if (is_absolute_url) {
        // Only reset authentication info if the passed URL is full
        free(client->connection_info.username);
        free(client->connection_info.password);
        client->connection_info.username = NULL;
        client->connection_info.password = NULL;
    }

    if (client->connection_info.path) {
        old_path = strdup(client->connection_info.path);
    }
    //Reset path and query if there are no information
    if (purl.field_data[UF_PATH].len) {
        http_utils_assign_string(&client->connection_info.path, url + purl.field_data[UF_PATH].off, purl.field_data[UF_PATH].len);
    } else {
        http_utils_assign_string(&client->connection_info.path, "/", 0);
    }
    LOGD(TAG, "###path:%s", client->connection_info.path);
    HTTP_MEM_CHECK(TAG, client->connection_info.path, {
        if (old_path) free(old_path);
        return WEB_ERR_NO_MEM;
    });

    // Close the connection if path was changed
    if (old_path && client->connection_info.path
            && strcasecmp(old_path, (const void *)client->connection_info.path) != 0) {
        LOGD(TAG, "New path assign = %s", client->connection_info.path);
        http_client_close(client);
    }
    if (old_path) {
        free(old_path);
        old_path = NULL;
    }

    if (purl.field_data[UF_QUERY].len) {
        http_utils_assign_string(&client->connection_info.query, url + purl.field_data[UF_QUERY].off, purl.field_data[UF_QUERY].len);
        HTTP_MEM_CHECK(TAG, client->connection_info.query, return WEB_ERR_NO_MEM);
    } else if (client->connection_info.query) {
        free(client->connection_info.query);
        client->connection_info.query = NULL;
    }

    return WEB_OK;
}

web_err_t http_client_set_method(http_client_handle_t client, http_client_method_t method)
{
    client->connection_info.method = method;
    return WEB_OK;
}

static int http_client_get_data(http_client_handle_t client)
{
    if (client->state < HTTP_STATE_RES_COMPLETE_HEADER) {
        return WEB_FAIL;
    }

    if (client->connection_info.method == HTTP_METHOD_HEAD) {
        return 0;
    }

    http_buffer_t *res_buffer = client->response->buffer;

    LOGD(TAG, "data_process=%d, content_length=%d", client->response->data_process, client->response->content_length);

    int rlen = transport_read(client->transport, res_buffer->data, client->buffer_size, client->timeout_ms);
    if (rlen >= 0) {
        http_parser_execute(client->parser, client->parser_settings, res_buffer->data, rlen);
    }
    return rlen;
}

bool http_client_is_complete_data_received(http_client_handle_t client)
{
    if (client->response->is_chunked) {
        if (!client->is_chunk_complete) {
            LOGD(TAG, "Chunks were not completely read");
            return false;
        }
    } else {
        if (client->response->data_process != client->response->content_length) {
            LOGD(TAG, "Data processed %d != Data specified in content length %d", client->response->data_process, client->response->content_length);
            return false;
        }
    }
    return true;
}

int http_client_read(http_client_handle_t client, char *buffer, int len)
{
    http_buffer_t *res_buffer = client->response->buffer;

    int rlen = WEB_FAIL, ridx = 0;
    if (res_buffer->raw_len) {
        int remain_len = client->response->buffer->raw_len;
        if (remain_len > len) {
            remain_len = len;
        }
        memcpy(buffer, res_buffer->raw_data, remain_len);
        res_buffer->raw_len -= remain_len;
        res_buffer->raw_data += remain_len;
        ridx = remain_len;
    }
    int need_read = len - ridx;
    bool is_data_remain = true;
    while (need_read > 0 && is_data_remain) {
        if (client->response->is_chunked) {
            is_data_remain = !client->is_chunk_complete;
        } else {
            is_data_remain = client->response->data_process < client->response->content_length;
        }
        //LOGD(TAG, "is_data_remain=%d, is_chunked=%d, content_length=%d", is_data_remain, client->response->is_chunked, client->response->content_length);
        if (!is_data_remain) {
            break;
        }
        int byte_to_read = need_read;
        if (byte_to_read > client->buffer_size) {
            byte_to_read = client->buffer_size;
        }
        errno = 0;
        rlen = transport_read(client->transport, res_buffer->data, byte_to_read, client->timeout_ms);
        //LOGD(TAG, "need_read=%d, byte_to_read=%d, rlen=%d, ridx=%d", need_read, byte_to_read, rlen, ridx);

        if (rlen <= 0) {
            if (errno != 0) {
                /* On connection close from server, recv should ideally return 0 but we have error conversion
                 * in `tcp_transport` SSL layer which translates it `-1` and hence below additional checks */
                if (rlen == -1 && errno == ENOTCONN && client->response->is_chunked) {
                    /* Explicit call to parser for invoking `message_complete` callback */
                    http_parser_execute(client->parser, client->parser_settings, res_buffer->data, 0);
                    /* ...and lowering the message severity, as closed connection from server side is expected in chunked transport */
                }
                LOGW(TAG, "transport_read returned:%d and errno:%d ", rlen, errno);
            }
            if (rlen < 0 && ridx == 0 && !http_client_is_complete_data_received(client)) {
                return -1;
            } else {
                return ridx;
            }
        }
        res_buffer->output_ptr = buffer + ridx;
        http_parser_execute(client->parser, client->parser_settings, res_buffer->data, rlen);
        ridx += res_buffer->raw_len;
        need_read -= res_buffer->raw_len;

        res_buffer->raw_len = 0; //clear
        res_buffer->output_ptr = NULL;
    }

    return ridx;
}

web_err_t http_client_perform(http_client_handle_t client)
{
    web_err_t err;
    do {
        LOGD(TAG, "client->state: %d, client->process_again: %d", client->state, client->process_again);
        if (client->process_again) {
            http_client_prepare(client);
        }
        switch (client->state) {
        /* In case of blocking http_client_perform(), the following states will fall through one after the after;
           in case of non-blocking http_client_perform(), if there is an error condition, like EINPROGRESS or EAGAIN,
           then the http_client_perform() API will return WEB_ERR_HTTP_EAGAIN error. The user may call
           http_client_perform API again, and for this reason, we maintain the states */
            case HTTP_STATE_INIT:
                if ((err = http_client_connect(client)) != WEB_OK) {
                    if (client->is_async && err == ERR_HTTP_CONNECTING) {
                        return ERR_HTTP_EAGAIN;
                    }
                    return err;
                }
                LOGD(TAG, "HTTP_STATE_INIT");
                /* falls through */
            case HTTP_STATE_CONNECTED:
                if ((err = http_client_request_send(client, client->post_len)) != WEB_OK) {
                    if (client->is_async && errno == EAGAIN) {
                        return ERR_HTTP_EAGAIN;
                    }
                    return err;
                }
                LOGD(TAG, "HTTP_STATE_CONNECTED");
                /* falls through */
            case HTTP_STATE_REQ_COMPLETE_HEADER:
                if ((err = http_client_send_post_data(client)) != WEB_OK) {
                    if (client->is_async && errno == EAGAIN) {
                        return ERR_HTTP_EAGAIN;
                    }
                    return err;
                }
                LOGD(TAG, "HTTP_STATE_REQ_COMPLETE_HEADER");
                /* falls through */
            case HTTP_STATE_REQ_COMPLETE_DATA:
                if (http_client_fetch_headers(client) < 0) {
                    if (client->is_async && errno == EAGAIN) {
                        return ERR_HTTP_EAGAIN;
                    }
                    return ERR_HTTP_FETCH_HEADER;
                }
                LOGD(TAG, "HTTP_STATE_REQ_COMPLETE_DATA");
                /* falls through */
            case HTTP_STATE_RES_COMPLETE_HEADER:
                if ((err = http_check_response(client)) != WEB_OK) {
                    LOGE(TAG, "Error response");
                    return err;
                }
                while (client->response->is_chunked && !client->is_chunk_complete) {
                    if (http_client_get_data(client) <= 0) {
                        if (client->is_async && errno == EAGAIN) {
                            return ERR_HTTP_EAGAIN;
                        }
                        LOGD(TAG, "Read finish or server requests close");
                        break;
                    }
                }
                while (client->response->data_process < client->response->content_length) {
                    if (http_client_get_data(client) <= 0) {
                        if (client->is_async && errno == EAGAIN) {
                            return ERR_HTTP_EAGAIN;
                        }
                        LOGD(TAG, "Read finish or server requests close");
                        break;
                    }
                }
                http_dispatch_event(client, HTTP_EVENT_ON_FINISH, NULL, 0);

                if (!http_should_keep_alive(client->parser)) {
                    LOGD(TAG, "Close connection");
                    http_client_close(client);
                } else {
                    if (client->state > HTTP_STATE_CONNECTED) {
                        client->state = HTTP_STATE_CONNECTED;
                        client->first_line_prepared = false;
                    }
                }
                LOGD(TAG, "HTTP_STATE_RES_COMPLETE_HEADER");
                break;
                default:
                break;
        }
    } while (client->process_again);
    return WEB_OK;
}

int http_client_fetch_headers(http_client_handle_t client)
{
    if (client->state < HTTP_STATE_REQ_COMPLETE_HEADER) {
        return WEB_FAIL;
    }

    client->state = HTTP_STATE_REQ_COMPLETE_DATA;
    http_buffer_t *buffer = client->response->buffer;
    client->response->status_code = -1;

    while (client->state < HTTP_STATE_RES_COMPLETE_HEADER) {
        buffer->len = transport_read(client->transport, buffer->data, client->buffer_size, client->timeout_ms);
        if (buffer->len <= 0) {
            return WEB_FAIL;
        }
        http_parser_execute(client->parser, client->parser_settings, buffer->data, buffer->len);
    }
    LOGD(TAG, "content_length = %d", client->response->content_length);
    if (client->response->content_length <= 0) {
        client->response->is_chunked = true;
        return 0;
    }
    return client->response->content_length;
}

static web_err_t http_client_connect(http_client_handle_t client)
{
    web_err_t err;

    if (client->state == HTTP_STATE_UNINIT) {
        LOGE(TAG, "Client has not been initialized");
        return WEB_ERR_INVALID_STATE;
    }

    if ((err = http_client_prepare(client)) != WEB_OK) {
        LOGE(TAG, "Failed to initialize request data");
        http_client_close(client);
        return err;
    }

    if (client->state < HTTP_STATE_CONNECTED) {
        LOGD(TAG, "Begin connect to: %s://%s:%d", client->connection_info.scheme, client->connection_info.host, client->connection_info.port);
        client->transport = transport_list_get_transport(client->transport_list, client->connection_info.scheme);
        if (client->transport == NULL) {
            LOGE(TAG, "No transport found");
#ifndef CONFIG_USING_TLS
            if (strcasecmp(client->connection_info.scheme, "https") == 0) {
                LOGE(TAG, "Please enable HTTPS at menuconfig to allow requesting via https");
            }
#endif
            return ERR_HTTP_INVALID_TRANSPORT;
        }
        if (!client->is_async) {
            if (transport_connect(client->transport, client->connection_info.host, client->connection_info.port, client->timeout_ms) < 0) {
                LOGE(TAG, "Connection failed, sock < 0");
                return ERR_HTTP_CONNECT;
            }
        } else {
            int ret = transport_connect_async(client->transport, client->connection_info.host, client->connection_info.port, client->timeout_ms);
            if (ret == ASYNC_TRANS_CONNECT_FAIL) {
                LOGE(TAG, "Connection failed");
                if (strcasecmp(client->connection_info.scheme, "http") == 0) {
                    LOGE(TAG, "Asynchronous mode doesn't work for HTTP based connection");
                    return WEB_ERR_INVALID_ARG;
                }
                return ERR_HTTP_CONNECT;
            } else if (ret == ASYNC_TRANS_CONNECTING) {
                LOGD(TAG, "Connection not yet established");
                return ERR_HTTP_CONNECTING;
            }
        }
        client->state = HTTP_STATE_CONNECTED;
        http_dispatch_event(client, HTTP_EVENT_ON_CONNECTED, NULL, 0);
    }
    return WEB_OK;
}

static int http_client_prepare_first_line(http_client_handle_t client, int write_len)
{
    if (write_len >= 0) {
        http_header_set_format(client->request->headers, "Content-Length", "%d", write_len);
    } else {
        http_client_set_header(client, "Transfer-Encoding", "chunked");
        http_client_set_method(client, HTTP_METHOD_POST);
    }

    const char *method = HTTP_METHOD_MAPPING[client->connection_info.method];

    int first_line_len = snprintf(client->request->buffer->data,
                                  client->buffer_size, "%s %s",
                                  method,
                                  client->connection_info.path);
    if (first_line_len >= client->buffer_size) {
        LOGE(TAG, "Out of buffer");
        return -1;
    }

    if (client->connection_info.query) {
        first_line_len += snprintf(client->request->buffer->data + first_line_len,
                                   client->buffer_size - first_line_len, "?%s", client->connection_info.query);
        if (first_line_len >= client->buffer_size) {
            LOGE(TAG, "Out of buffer");
            return -1;

        }
    }
    first_line_len += snprintf(client->request->buffer->data + first_line_len,
                               client->buffer_size - first_line_len, " %s\r\n", DEFAULT_HTTP_PROTOCOL);
    if (first_line_len >= client->buffer_size) {
        LOGE(TAG, "Out of buffer");
        return -1;
    }
    return first_line_len;
}

static web_err_t http_client_request_send(http_client_handle_t client, int write_len)
{
    int first_line_len = 0;
    if (!client->first_line_prepared) {
        if ((first_line_len = http_client_prepare_first_line(client, write_len)) < 0) {
            return first_line_len;
        }
        client->first_line_prepared = true;
        client->header_index = 0;
        client->data_written_index = 0;
        client->data_write_left = 0;
    }

    if (client->data_write_left > 0) {
        /* sending leftover data from previous call to http_client_request_send() API */
        int wret = 0;
        if (((wret = http_client_write(client, client->request->buffer->data + client->data_written_index, client->data_write_left)) < 0)) {
            LOGE(TAG, "Error write request");
            return ERR_HTTP_WRITE_DATA;
        }
        client->data_write_left -= wret;
        client->data_written_index += wret;
        if (client->is_async && client->data_write_left > 0) {
            return ERR_HTTP_WRITE_DATA;      /* In case of EAGAIN error, we return WEB_ERR_HTTP_WRITE_DATA,
                                                 and the handling of EAGAIN should be done in the higher level APIs. */
        }
    }

    int wlen = client->buffer_size - first_line_len;
    while ((client->header_index = http_header_generate_string(client->request->headers, client->header_index, client->request->buffer->data + first_line_len, &wlen))) {
        if (wlen <= 0) {
            break;
        }
        if (first_line_len) {
            wlen += first_line_len;
            first_line_len = 0;
        }
        client->request->buffer->data[wlen] = 0;
        LOGD(TAG, "Write header[%d]: %s", client->header_index, client->request->buffer->data);

        client->data_write_left = wlen;
        client->data_written_index = 0;
        while (client->data_write_left > 0) {
            int wret = transport_write(client->transport, client->request->buffer->data + client->data_written_index, client->data_write_left, client->timeout_ms);
            if (wret <= 0) {
                LOGE(TAG, "Error write request");
                http_client_close(client);
                return ERR_HTTP_WRITE_DATA;
            }
            client->data_write_left -= wret;
            client->data_written_index += wret;
        }
        wlen = client->buffer_size;
    }

    client->data_written_index = 0;
    client->data_write_left = client->post_len;
    http_dispatch_event(client, HTTP_EVENT_HEADER_SENT, NULL, 0);
    client->state = HTTP_STATE_REQ_COMPLETE_HEADER;
    return WEB_OK;
}

static web_err_t http_client_send_post_data(http_client_handle_t client)
{
    if (client->state != HTTP_STATE_REQ_COMPLETE_HEADER) {
        LOGE(TAG, "Invalid state");
        return WEB_ERR_INVALID_STATE;
    }
    if (!(client->post_data && client->post_len)) {
        goto success;
    }

    int wret = http_client_write(client, client->post_data + client->data_written_index, client->data_write_left);
    if (wret < 0) {
        return wret;
    }
    client->data_write_left -= wret;
    client->data_written_index += wret;

    if (client->data_write_left <= 0) {
        goto success;
    } else {
        return ERR_HTTP_WRITE_DATA;
    }

success:
    client->state = HTTP_STATE_REQ_COMPLETE_DATA;
    return WEB_OK;
}

web_err_t http_client_open(http_client_handle_t client, int write_len)
{
    client->post_len = write_len;
    web_err_t err;
    if ((err = http_client_connect(client)) != WEB_OK) {
        return err;
    }
    if ((err = http_client_request_send(client, write_len)) != WEB_OK) {
        return err;
    }
    return WEB_OK;
}

int http_client_write(http_client_handle_t client, const char *buffer, int len)
{
    if (client->state < HTTP_STATE_REQ_COMPLETE_HEADER) {
        return WEB_FAIL;
    }

    int wlen = 0, widx = 0;
    while (len > 0) {
        wlen = transport_write(client->transport, buffer + widx, len, client->timeout_ms);
        /* client->async_block is initialised in case of non-blocking IO, and in this case we return how
           much ever data was written by the transport_write() API. */
        if (client->is_async || wlen <= 0) {
            return wlen;
        }
        widx += wlen;
        len -= wlen;
    }
    return widx;
}

web_err_t http_client_close(http_client_handle_t client)
{
    if (client->state >= HTTP_STATE_INIT) {
        http_dispatch_event(client, HTTP_EVENT_DISCONNECTED, NULL, 0);
        client->state = HTTP_STATE_INIT;
        return transport_close(client->transport);
    }
    return WEB_OK;
}

web_err_t http_client_set_post_field(http_client_handle_t client, const char *data, int len)
{
    web_err_t err = WEB_OK;
    client->post_data = (char *)data;
    client->post_len = len;
    LOGD(TAG, "set post file length = %d", len);
    if (client->post_data) {
        char *value = NULL;
        if ((err = http_client_get_header(client, "Content-Type", &value)) != WEB_OK) {
            return err;
        }
        if (value == NULL) {
            err = http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
        }
    } else {
        client->post_len = 0;
        err = http_client_set_header(client, "Content-Type", NULL);
    }
    return err;
}

int http_client_get_post_field(http_client_handle_t client, char **data)
{
    if (client->post_data) {
        *data = client->post_data;
        return client->post_len;
    }
    return 0;
}

int http_client_get_status_code(http_client_handle_t client)
{
    return client->response->status_code;
}

int http_client_get_content_length(http_client_handle_t client)
{
    return client->response->content_length;
}

bool http_client_is_chunked_response(http_client_handle_t client)
{
    return client->response->is_chunked;
}

http_client_transport_t http_client_get_transport_type(http_client_handle_t client)
{
    if (!strcasecmp(client->connection_info.scheme, "https") ) {
        return HTTP_TRANSPORT_OVER_SSL;
    } else if (!strcasecmp(client->connection_info.scheme, "http")) {
        return HTTP_TRANSPORT_OVER_TCP;
    } else {
        return HTTP_TRANSPORT_UNKNOWN;
    }
}

int http_client_read_response(http_client_handle_t client, char *buffer, int len)
{
    int read_len = 0;
    while (read_len < len) {
        int data_read = http_client_read(client, buffer + read_len, len - read_len);
        if (data_read <= 0) {
            return read_len;
        }
        read_len += data_read;
    }
    return read_len;
}