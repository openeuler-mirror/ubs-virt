/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 * enpu-manager is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include "rest_api.h"
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "../../core/include/allocation.h"
#include "../../core/include/common.h"
#include "../../core/include/enpu_config.h"
#include "../../core/include/enpu_manager.h"
#include "../../core/include/log.h"
#include "../../core/include/swap_buffer_manager.h"
#include "ratio_request.h"
#include "resource_format.h"
#include "securec.h"

#define REST_MAX_CONNECTIONS 10
#define REST_BUFFER_SIZE 4096
#define REST_RESPONSE_SIZE 65536
#define REST_PATH_SIZE 1024
#define DEVICE_JSON_BUF_SIZE 512
#define REST_RECV_TIMEOUT_SEC 5
#define HTTP_STATUS_OK 200
#define HTTP_STATUS_BAD_REQUEST 400
#define HTTP_STATUS_REQUEST_TIMEOUT 408
#define HTTP_STATUS_PAYLOAD_TOO_LARGE 413
#define HTTP_STATUS_CONFLICT 409
#define HTTP_STATUS_NOT_FOUND 404
#define HTTP_STATUS_INTERNAL_ERROR 500
#define HTTP_STATUS_METHOD_NOT_ALLOWED 405

typedef struct rest_api_server {
    char socket_path[sizeof(((struct sockaddr_un *)0)->sun_path)];
    int server_fd;
    int running;
    int socket_bound;
    pthread_t accept_thread;
    enpu_manager_t *manager;
    pthread_mutex_t lock;
} rest_api_server_t;

static int parse_http_request(char *buffer, char *method, size_t method_size, char *path, size_t path_size, char *body,
                              size_t body_size);
static int handle_request(rest_api_server_t *server, int client_fd);
static void *accept_thread_func(void *arg);
static int send_response(int client_fd, int status, const char *body);
static int send_typed_response(int client_fd, int status, const char *body, const char *content_type);
static int route_request(rest_api_server_t *server, const char *method, const char *path, const char *body,
                         char *response);
static int update_config_route(enpu_manager_t *mgr, const char *body, char *response);
static int get_config_route(char *response);

static char *format_devices_json(enpu_manager_t *mgr);
static char *format_device_json(enpu_manager_t *mgr, int phy_id);
static char *format_allocations_json(enpu_manager_t *mgr);
static char *format_alloc_response_json(const alloc_response_t *resp, const char *request_body);
static char *format_result_json(int result, const char *error_msg);
static int copy_json_response(char *response, char *json);
static int parse_json_alloc_request(const char *body, alloc_request_t *req, char *error, size_t error_size);

static int is_decimal_string(const char *s)
{
    if (s == NULL || *s == '\0') {
        return 0;
    }
    for (const char *p = s; *p != '\0'; p++) {
        if (*p < '0' || *p > '9') {
            return 0;
        }
    }
    return 1;
}

static void set_json_field_error(char *error, size_t error_size, const char *key, const char *reason)
{
    if (error == NULL || error_size == 0) {
        return;
    }
    if (snprintf_s(error, error_size, error_size - 1, "%s %s", key, reason) < 0) {
        error[0] = '\0';
    }
}

static int json_optional_string(const cJSON *root, const char *key, char *output, size_t output_size, char *error,
                                size_t error_size)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == NULL) {
        return ENPU_SUCCESS;
    }
    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        set_json_field_error(error, error_size, key, "must be a JSON string");
        return ENPU_INVALID_PARAM;
    }
    if (strnlen(item->valuestring, output_size) >= output_size) {
        if (snprintf_s(error, error_size, error_size - 1, "%s exceeds maximum length %zu", key, output_size - 1) < 0) {
            error[0] = '\0';
        }
        return ENPU_INVALID_PARAM;
    }
    if (strncpy_s(output, output_size, item->valuestring, strlen(item->valuestring)) != 0) {
        set_json_field_error(error, error_size, key, "could not be copied");
        return ENPU_INVALID_PARAM;
    }
    return ENPU_SUCCESS;
}

static int json_optional_int(const cJSON *root, const char *key, int *output, char *error, size_t error_size)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == NULL) {
        return ENPU_SUCCESS;
    }
    double value = item->valuedouble;
    if (!cJSON_IsNumber(item) || !isfinite(value) || value < INT_MIN || value > INT_MAX ||
        (double)(int)value != value) {
        set_json_field_error(error, error_size, key, "must be a JSON integer");
        return ENPU_INVALID_PARAM;
    }
    *output = (int)value;
    return ENPU_SUCCESS;
}

static int json_optional_uint64(const cJSON *root, const char *key, uint64_t *output, bool *present, char *error,
                                size_t error_size)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    *present = item != NULL;
    if (item == NULL) {
        return ENPU_SUCCESS;
    }
    double value = item->valuedouble;
    /* JSON numbers above 2^53 cannot be converted to uint64 without losing identity. */
    if (!cJSON_IsNumber(item) || !isfinite(value) || value < 0 || value > 9007199254740991.0 ||
        (double)(uint64_t)value != value) {
        set_json_field_error(error, error_size, key, "must be a non-negative JSON integer within the supported range");
        return ENPU_INVALID_PARAM;
    }
    *output = (uint64_t)value;
    return ENPU_SUCCESS;
}

static int json_add_uint64(cJSON *object, const char *key, uint64_t value)
{
    char number[32];
    if (snprintf_s(number, sizeof(number), sizeof(number) - 1, "%llu", (unsigned long long)value) < 0) {
        return ENPU_FAIL;
    }
    cJSON *raw = cJSON_CreateRaw(number);
    if (raw == NULL) {
        return ENPU_NO_RESOURCE;
    }
    cJSON_AddItemToObject(object, key, raw);
    return ENPU_SUCCESS;
}

static const char *http_reason_phrase(int status)
{
    switch (status) {
        case HTTP_STATUS_OK:
            return "OK";
        case HTTP_STATUS_BAD_REQUEST:
            return "Bad Request";
        case HTTP_STATUS_REQUEST_TIMEOUT:
            return "Request Timeout";
        case HTTP_STATUS_PAYLOAD_TOO_LARGE:
            return "Payload Too Large";
        case HTTP_STATUS_CONFLICT:
            return "Conflict";
        case HTTP_STATUS_NOT_FOUND:
            return "Not Found";
        case HTTP_STATUS_METHOD_NOT_ALLOWED:
            return "Method Not Allowed";
        case HTTP_STATUS_INTERNAL_ERROR:
            return "Internal Server Error";
        default:
            return "OK";
    }
}

rest_api_server_t *rest_api_create(const char *socket_path)
{
    if (socket_path == NULL || socket_path[0] != '/' ||
        strnlen(socket_path, sizeof(((struct sockaddr_un *)0)->sun_path)) >=
            sizeof(((struct sockaddr_un *)0)->sun_path)) {
        LOG_ERROR("[REST] Invalid Unix socket path");
        return NULL;
    }
    rest_api_server_t *server = calloc(1, sizeof(rest_api_server_t));
    if (!server)
        return NULL;
    int ret = strncpy_s(server->socket_path, sizeof(server->socket_path), socket_path, sizeof(server->socket_path) - 1);
    if (ret != 0) {
        LOG_ERROR("[REST] Failed to copy Unix socket path: %d", ret);
        free(server);
        return NULL;
    }
    server->server_fd = -1;
    server->running = 0;
    server->manager = NULL;
    pthread_mutex_init(&server->lock, NULL);

    return server;
}

int rest_api_destroy(rest_api_server_t *server)
{
    if (!server)
        return ENPU_INVALID_PARAM;

    rest_api_stop(server);
    pthread_mutex_destroy(&server->lock);
    free(server);
    return ENPU_SUCCESS;
}
void rest_api_set_manager(rest_api_server_t *server, enpu_manager_t *mgr)
{
    if (!server)
        return;
    pthread_mutex_lock(&server->lock);
    server->manager = mgr;
    pthread_mutex_unlock(&server->lock);
}
int rest_api_start(rest_api_server_t *server)
{
    if (!server)
        return ENPU_INVALID_PARAM;

    pthread_mutex_lock(&server->lock);
    if (server->running) {
        pthread_mutex_unlock(&server->lock);
        return ENPU_ALREADY_EXISTS;
    }

    struct sockaddr_un addr;
    int ret = memset_s(&addr, sizeof(addr), 0, sizeof(addr));
    if (ret != 0) {
        pthread_mutex_unlock(&server->lock);
        LOG_ERROR("[REST] memset_s addr failed: %d", ret);
        return ENPU_FAIL;
    }
    addr.sun_family = AF_UNIX;
    ret = strncpy_s(addr.sun_path, sizeof(addr.sun_path), server->socket_path, sizeof(addr.sun_path) - 1);
    if (ret != 0) {
        pthread_mutex_unlock(&server->lock);
        LOG_ERROR("[REST] Failed to copy socket path: %d", ret);
        return ENPU_FAIL;
    }

    server->server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        pthread_mutex_unlock(&server->lock);
        LOG_ERROR("[REST] Failed to create socket: %s", strerror(errno));
        return ENPU_FAIL;
    }

    struct stat socket_stat;
    if (lstat(server->socket_path, &socket_stat) == 0) {
        if (!S_ISSOCK(socket_stat.st_mode)) {
            close(server->server_fd);
            server->server_fd = -1;
            pthread_mutex_unlock(&server->lock);
            LOG_ERROR("[REST] Socket path exists and is not a socket: %s", server->socket_path);
            return ENPU_FAIL;
        }
        int probe_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        int probe_ret = probe_fd < 0 ? -1 : connect(probe_fd, (struct sockaddr *)&addr, sizeof(addr));
        int probe_errno = errno;
        if (probe_fd >= 0) {
            close(probe_fd);
        }
        if (probe_ret == 0 || (probe_ret < 0 && probe_errno != ECONNREFUSED)) {
            close(server->server_fd);
            server->server_fd = -1;
            pthread_mutex_unlock(&server->lock);
            LOG_ERROR("[REST] Unix socket is already in use: %s", server->socket_path);
            return ENPU_ALREADY_EXISTS;
        }
        if (unlink(server->socket_path) != 0) {
            close(server->server_fd);
            server->server_fd = -1;
            pthread_mutex_unlock(&server->lock);
            LOG_ERROR("[REST] Failed to remove stale socket %s: %s", server->socket_path, strerror(errno));
            return ENPU_FAIL;
        }
        LOG_WARN("[REST] Removed stale Unix socket: %s", server->socket_path);
    } else if (errno != ENOENT) {
        close(server->server_fd);
        server->server_fd = -1;
        pthread_mutex_unlock(&server->lock);
        LOG_ERROR("[REST] Failed to inspect socket path %s: %s", server->socket_path, strerror(errno));
        return ENPU_FAIL;
    }

    if (bind(server->server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(server->server_fd);
        server->server_fd = -1;
        pthread_mutex_unlock(&server->lock);
        LOG_ERROR("[REST] Failed to bind Unix socket %s: %s", server->socket_path, strerror(errno));
        return ENPU_FAIL;
    }
    server->socket_bound = 1;
    if (chmod(server->socket_path, 0660) != 0) {
        close(server->server_fd);
        server->server_fd = -1;
        unlink(server->socket_path);
        server->socket_bound = 0;
        pthread_mutex_unlock(&server->lock);
        LOG_ERROR("[REST] Failed to set socket permissions for %s: %s", server->socket_path, strerror(errno));
        return ENPU_FAIL;
    }

    if (listen(server->server_fd, REST_MAX_CONNECTIONS) < 0) {
        close(server->server_fd);
        server->server_fd = -1;
        unlink(server->socket_path);
        server->socket_bound = 0;
        pthread_mutex_unlock(&server->lock);
        LOG_ERROR("[REST] Failed to listen socket: %s", strerror(errno));
        return ENPU_FAIL;
    }

    server->running = 1;
    int create_ret = pthread_create(&server->accept_thread, NULL, accept_thread_func, server);
    if (create_ret != 0) {
        server->running = 0;
        close(server->server_fd);
        server->server_fd = -1;
        unlink(server->socket_path);
        server->socket_bound = 0;
        pthread_mutex_unlock(&server->lock);
        LOG_ERROR("[REST] Failed to create accept thread: %d", create_ret);
        return ENPU_FAIL;
    }
    pthread_mutex_unlock(&server->lock);

    LOG_INFO("[REST] Server started on Unix socket %s", server->socket_path);
    return ENPU_SUCCESS;
}
int rest_api_stop(rest_api_server_t *server)
{
    if (!server)
        return ENPU_INVALID_PARAM;

    pthread_mutex_lock(&server->lock);
    if (!server->running) {
        pthread_mutex_unlock(&server->lock);
        return ENPU_SUCCESS;
    }

    server->running = 0;
    if (server->server_fd >= 0) {
        shutdown(server->server_fd, SHUT_RDWR);
        close(server->server_fd);
        server->server_fd = -1;
    }
    if (server->socket_bound) {
        if (unlink(server->socket_path) != 0 && errno != ENOENT) {
            LOG_WARN("[REST] Failed to remove Unix socket %s: %s", server->socket_path, strerror(errno));
        }
        server->socket_bound = 0;
    }

    pthread_mutex_unlock(&server->lock);

    pthread_join(server->accept_thread, NULL);
    LOG_INFO("[REST] Server stopped");
    return ENPU_SUCCESS;
}
static void *accept_thread_func(void *arg)
{
    rest_api_server_t *server = (rest_api_server_t *)arg;
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        LOG_ERROR("[REST] Failed to ignore SIGPIPE.");
    }
    while (server->running) {
        int client_fd = accept(server->server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR)
                continue;
            usleep(100000);
            continue;
        }
        handle_request(server, client_fd);
        close(client_fd);
    }
    return NULL;
}

static int resource_error_response(int client_fd, int status, int result, const char *message)
{
    char output[256];
    int n = snprintf_s(output, sizeof(output), sizeof(output) - 1, "{\"items\":[],\"result\":%d,\"error_msg\":\"%s\"}",
                       result, message);
    if (n < 0) {
        return ENPU_FAIL;
    }
    return send_response(client_fd, status, output);
}

static int handle_resource_request(rest_api_server_t *server, int client_fd, const char *method, const char *uri)
{
    if (strcmp(method, "GET") != 0) {
        return resource_error_response(client_fd, HTTP_STATUS_METHOD_NOT_ALLOWED, ENPU_INVALID_PARAM, "GET required");
    }
    enpu_resource_query_t query;
    bool metrics = false;
    if (resource_parse_uri(uri, &query, &metrics) != ENPU_SUCCESS) {
        LOG_WARN("[REST-RESOURCES] Invalid resource query parameters");
        return resource_error_response(client_fd, HTTP_STATUS_BAD_REQUEST, ENPU_INVALID_PARAM,
                                       "Invalid resource query");
    }
    pthread_mutex_lock(&server->lock);
    enpu_manager_t *mgr = server->manager;
    pthread_mutex_unlock(&server->lock);
    enpu_resource_response_t snapshot = {0};
    int ret = mgr == NULL ? ENPU_FAIL : enpu_query_resources(mgr, &query, &snapshot);
    char *output = NULL;
    const char *stage = "core_query";
    if (ret == ENPU_SUCCESS) {
        stage = "format";
        ret = resource_format(&snapshot, metrics, &output);
    }
    LOG_DEBUG("[REST-RESOURCES] Query: metrics=%d type=%d detail=%d resources=%zu ret=%d", metrics, query.type,
              query.detail, snapshot.count, ret);
    enpu_free_resource_response(&snapshot);
    if (ret != ENPU_SUCCESS) {
        free(output);
        if (ret == ENPU_NOT_FOUND) {
            LOG_DEBUG("[REST-RESOURCES] No matching resource: type=%d", query.type);
        } else {
            LOG_ERROR("[REST-RESOURCES] Query failed: stage=%s metrics=%d ret=%d", stage, metrics, ret);
        }
        int status = ret == ENPU_NOT_FOUND ? HTTP_STATUS_NOT_FOUND : HTTP_STATUS_INTERNAL_ERROR;
        return resource_error_response(client_fd, status, ret,
                                       ret == ENPU_NOT_FOUND ? "Resource not found" : "Resource query failed");
    }
    ret = send_typed_response(client_fd, HTTP_STATUS_OK, output,
                              metrics ? "text/plain; version=0.0.4; charset=utf-8" : "application/json");
    free(output);
    return ret;
}

/* One request per connection. Only Content-Length framing is supported. */
static int request_body_length(const char *buffer, const char *header_end, size_t capacity, size_t *length)
{
    const char *line = strstr(buffer, "\r\n");
    if (line == NULL || line > header_end) {
        return HTTP_STATUS_BAD_REQUEST;
    }
    bool seen_length = false;
    *length = 0;
    for (line += 2; line < header_end;) {
        const char *end = strstr(line, "\r\n");
        const char *colon = end == NULL ? NULL : memchr(line, ':', (size_t)(end - line));
        if (end == NULL || end > header_end || colon == NULL || colon == line) {
            return HTTP_STATUS_BAD_REQUEST;
        }
        size_t name_length = (size_t)(colon - line);
        if (line[0] == ' ' || line[0] == '\t' || colon[-1] == ' ' || colon[-1] == '\t') {
            return HTTP_STATUS_BAD_REQUEST;
        }
        if (name_length == strlen("Transfer-Encoding") && strncasecmp(line, "Transfer-Encoding", name_length) == 0) {
            return HTTP_STATUS_BAD_REQUEST;
        }
        if (name_length == strlen("Content-Length") && strncasecmp(line, "Content-Length", name_length) == 0) {
            if (seen_length) {
                return HTTP_STATUS_BAD_REQUEST;
            }
            seen_length = true;
            const char *value = colon + 1;
            while (value < end && (*value == ' ' || *value == '\t')) {
                ++value;
            }
            if (value == end || *value < '0' || *value > '9') {
                return HTTP_STATUS_BAD_REQUEST;
            }
            while (value < end && *value >= '0' && *value <= '9') {
                size_t digit = (size_t)(*value++ - '0');
                if (*length > (SIZE_MAX - digit) / 10) {
                    return HTTP_STATUS_PAYLOAD_TOO_LARGE;
                }
                *length = *length * 10 + digit;
            }
            while (value < end && (*value == ' ' || *value == '\t')) {
                ++value;
            }
            if (value != end) {
                return HTTP_STATUS_BAD_REQUEST;
            }
        }
        line = end + 2;
    }
    size_t header_length = (size_t)(header_end - buffer) + 4;
    return header_length >= capacity || *length > capacity - header_length - 1 ? HTTP_STATUS_PAYLOAD_TOO_LARGE :
                                                                                 HTTP_STATUS_OK;
}

static int receive_http_request(int fd, char *buffer, size_t capacity)
{
    buffer[0] = '\0';
    struct timespec started;
    if (clock_gettime(CLOCK_MONOTONIC, &started) != 0) {
        return HTTP_STATUS_INTERNAL_ERROR;
    }
    size_t used = 0, expected = 0;
    bool headers_complete = false;
    while (true) {
        if (used == capacity - 1) {
            return HTTP_STATUS_PAYLOAD_TOO_LARGE;
        }
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            return HTTP_STATUS_INTERNAL_ERROR;
        }
        int64_t elapsed_ns =
            (int64_t)(now.tv_sec - started.tv_sec) * 1000000000LL + (int64_t)now.tv_nsec - (int64_t)started.tv_nsec;
        int64_t remaining_ns = REST_RECV_TIMEOUT_SEC * 1000000000LL - elapsed_ns;
        if (remaining_ns <= 0) {
            return HTTP_STATUS_REQUEST_TIMEOUT;
        }
        struct pollfd pending = {.fd = fd, .events = POLLIN};
        int ready = poll(&pending, 1, (int)((remaining_ns + 999999LL) / 1000000LL));
        if (ready < 0 && errno == EINTR) {
            continue;
        }
        if (ready == 0) {
            return HTTP_STATUS_REQUEST_TIMEOUT;
        }
        if (ready < 0) {
            return HTTP_STATUS_INTERNAL_ERROR;
        }
        ssize_t received = recv(fd, buffer + used, capacity - used - 1, MSG_DONTWAIT);
        if (received < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
            continue;
        }
        if (received <= 0) {
            return HTTP_STATUS_BAD_REQUEST;
        }
        if (memchr(buffer + used, '\0', (size_t)received) != NULL) {
            return HTTP_STATUS_BAD_REQUEST;
        }
        used += (size_t)received;
        buffer[used] = '\0';
        if (!headers_complete) {
            const char *end = strstr(buffer, "\r\n\r\n");
            if (end == NULL) {
                continue;
            }
            size_t body_length = 0;
            int status = request_body_length(buffer, end, capacity, &body_length);
            if (status != HTTP_STATUS_OK) {
                return status;
            }
            expected = (size_t)(end - buffer) + 4 + body_length;
            headers_complete = true;
        }
        if (used > expected) {
            return HTTP_STATUS_BAD_REQUEST;
        }
        if (used == expected) {
            return HTTP_STATUS_OK;
        }
    }
}

static int handle_request(rest_api_server_t *server, int client_fd)
{
    char buffer[REST_BUFFER_SIZE];
    char method[16], path[REST_PATH_SIZE], body[REST_BUFFER_SIZE];
    int receive_status = receive_http_request(client_fd, buffer, sizeof(buffer));
    if (receive_status != HTTP_STATUS_OK) {
        LOG_WARN("[REST] Request receive failed: status=%d received_bytes=%zu", receive_status, strlen(buffer));
        send_response(client_fd, receive_status, "{\"error\":\"Invalid or incomplete HTTP request\"}");
        return ENPU_FAIL;
    }

    if (parse_http_request(buffer, method, sizeof(method), path, sizeof(path), body, sizeof(body)) != 0) {
        send_response(client_fd, HTTP_STATUS_BAD_REQUEST, "{\"error\":\"Invalid HTTP request\"}");
        return ENPU_FAIL;
    }

    /* Resource outputs grow independently of the legacy fixed-size routes. */
    size_t resource_path_len = strlen("/api/v1/resources");
    if ((strncmp(path, "/api/v1/resources", resource_path_len) == 0 &&
         (path[resource_path_len] == '\0' || path[resource_path_len] == '?')) ||
        strcmp(path, "/metrics") == 0 || strncmp(path, "/metrics?", 9) == 0) {
        return handle_resource_request(server, client_fd, method, path);
    }
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/allocations") == 0) {
        pthread_mutex_lock(&server->lock);
        enpu_manager_t *mgr = server->manager;
        pthread_mutex_unlock(&server->lock);
        char *json = format_allocations_json(mgr);
        if (json == NULL) {
            return send_response(client_fd, HTTP_STATUS_INTERNAL_ERROR,
                                 "{\"error\":\"Failed to serialize allocations\",\"result\":-1}");
        }
        int send_ret = send_response(client_fd, HTTP_STATUS_OK, json);
        free(json);
        return send_ret;
    }
    char *response = (char *)malloc(REST_RESPONSE_SIZE);
    if (response == NULL) {
        send_response(client_fd, HTTP_STATUS_INTERNAL_ERROR, "{\"error\":\"OOM\"}");
        return ENPU_FAIL;
    }

    int result = route_request(server, method, path, body, response);

    send_response(client_fd, result, response);
    free(response);
    return ENPU_SUCCESS;
}
static int parse_http_request(char *buffer, char *method, size_t method_size, char *path, size_t path_size, char *body,
                              size_t body_size)
{
    char *headers_end = strstr(buffer, "\r\n\r\n");
    if (headers_end == NULL) {
        return -1;
    }
    char *line_end = strstr(buffer, "\r\n");
    if (!line_end)
        return -1;
    *line_end = '\0';
    char *space = strchr(buffer, ' ');
    if (!space)
        return -1;
    size_t method_len = (size_t)(space - buffer);
    if (method_len >= method_size)
        return -1;
    int copy_ret = strncpy_s(method, method_size, buffer, method_len);
    CHECK_COND_RETURN_(copy_ret != 0, -1, "strncpy_s method failed.");
    char *path_start = space + 1;
    char *path_end = strchr(path_start, ' ');
    if (!path_end)
        return -1;
    size_t path_len = (size_t)(path_end - path_start);
    if (path_len >= path_size)
        return -1;
    copy_ret = strncpy_s(path, path_size, path_start, path_len);
    CHECK_COND_RETURN_(copy_ret != 0, -1, "strncpy_s path failed.");

    const char *body_start = headers_end + 4;
    copy_ret = strncpy_s(body, body_size, body_start, strlen(body_start));
    CHECK_COND_RETURN_(copy_ret != 0, -1, "strncpy_s body failed.");
    return 0;
}
static int send_response(int client_fd, int status, const char *body)
{
    return send_typed_response(client_fd, status, body, "application/json");
}

static int send_typed_response(int client_fd, int status, const char *body, const char *content_type)
{
    char header[512];
    size_t body_len = strlen(body);

    int header_len = snprintf_s(header, sizeof(header), sizeof(header) - 1,
                                "HTTP/1.1 %d %s\r\n"
                                "Content-Type: %s\r\n"
                                "Content-Length: %zu\r\n"
                                "%s"
                                "Connection: close\r\n"
                                "\r\n",
                                status, http_reason_phrase(status), content_type, body_len,
                                status == HTTP_STATUS_METHOD_NOT_ALLOWED ? "Allow: GET\r\n" : "");
    CHECK_COND_RETURN_(header_len < 0, ENPU_FAIL, "snprintf_s header failed.");

    ssize_t written = 0;
    while (written < header_len) {
        ssize_t n = write(client_fd, header + written, header_len - written);
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n <= 0) {
            return ENPU_FAIL;
        }
        written += n;
    }

    written = 0;
    while (written < (ssize_t)body_len) {
        ssize_t n = write(client_fd, body + written, body_len - written);
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n <= 0) {
            return ENPU_FAIL;
        }
        written += n;
    }

    return 0;
}
static int parse_release_query(const char *query, char *pod, size_t pod_size, char *container, size_t container_size)
{
    pod[0] = container[0] = '\0';
    while (query && *query) {
        char *out;
        size_t capacity;
        if (strncmp(query, "pod_uid=", 8) == 0 && !pod[0]) {
            out = pod;
            capacity = pod_size;
            query += 8;
        } else if (strncmp(query, "container_name=", 15) == 0 && !container[0]) {
            out = container;
            capacity = container_size;
            query += 15;
        } else {
            return -1;
        }
        size_t length = 0;
        while (*query && *query != '&') {
            unsigned char value = (unsigned char)*query++;
            if (value == '%') {
                unsigned int decoded;
                if (!query[0] || !query[1] || strspn(query, "0123456789abcdefABCDEF") < 2 ||
                    sscanf(query, "%2x", &decoded) != 1) {
                    return -1;
                }
                value = (unsigned char)decoded;
                query += 2;
            } else if (value == '+') {
                value = ' ';
            }
            if (value < 0x20 || value == 0x7f || length + 1 >= capacity) {
                return -1;
            }
            out[length++] = (char)value;
        }
        out[length] = '\0';
        if (!length || (*query == '&' && !*++query)) {
            return -1;
        }
    }
    return pod[0] && container[0] ? 0 : -1;
}

static int config_error_response(char *response, int status, const char *message)
{
    const char *format = "{\"error\":\"%s\",\"result\":%d}";
    (void)snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1, format, message, ENPU_FAIL);
    return status;
}

static char *trim_config_value(char *value)
{
    while (isspace((unsigned char)*value)) {
        value++;
    }
    char *end = value + strlen(value);
    while (end > value && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return value;
}

static bool config_integer_key(const char *key)
{
    return strcmp(key, "swap-pre-watermark") == 0 || strcmp(key, "log_max_size") == 0 ||
           strcmp(key, "log_max_backups") == 0 || strcmp(key, "log_max_age") == 0 || strcmp(key, "log_console") == 0;
}

static int get_config_route(char *response)
{
    struct stat file_stat;
    if (lstat(ENPU_CONFIG_DEFAULT_PATH, &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
        return config_error_response(response, HTTP_STATUS_INTERNAL_ERROR, "Unable to read config file");
    }
    FILE *file = fopen(ENPU_CONFIG_DEFAULT_PATH, "r");
    if (file == NULL) {
        return config_error_response(response, HTTP_STATUS_INTERNAL_ERROR, "Unable to read config file");
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *config = cJSON_CreateObject();
    bool ok = root != NULL && config != NULL && cJSON_AddNumberToObject(root, "result", 0) != NULL;
    if (ok) {
        ok = cJSON_AddItemToObject(root, "config", config);
        if (ok) {
            config = NULL;
        }
    }

    char *line = NULL;
    size_t capacity = 0;
    while (ok && getline(&line, &capacity, file) >= 0) {
        char *key = trim_config_value(line);
        if (*key == '\0' || *key == '#') {
            continue;
        }
        char *separator = strchr(key, '=');
        if (separator == NULL) {
            ok = false;
            break;
        }
        *separator = '\0';
        key = trim_config_value(key);
        char *value = separator + 1;
        char *comment = strchr(value, '#');
        if (comment != NULL) {
            *comment = '\0';
        }
        value = trim_config_value(value);
        if (*key == '\0') {
            ok = false;
            break;
        }

        cJSON *settings = cJSON_GetObjectItemCaseSensitive(root, "config");
        cJSON_DeleteItemFromObjectCaseSensitive(settings, key);
        if (config_integer_key(key)) {
            char *end = NULL;
            errno = 0;
            long number = strtol(value, &end, 10);
            if (errno != 0 || end == value || *end != '\0' || number < INT_MIN || number > INT_MAX) {
                ok = false;
                break;
            }
            ok = cJSON_AddNumberToObject(settings, key, (double)number) != NULL;
        } else {
            ok = cJSON_AddStringToObject(settings, key, value) != NULL;
        }
    }
    bool read_failed = ferror(file) != 0;
    if (fclose(file) != 0 || read_failed) {
        ok = false;
    }
    free(line);

    char *json = ok ? cJSON_PrintUnformatted(root) : NULL;
    cJSON_Delete(config);
    cJSON_Delete(root);
    if (copy_json_response(response, json) != ENPU_SUCCESS) {
        return config_error_response(response, HTTP_STATUS_INTERNAL_ERROR, "Unable to read config file");
    }
    return HTTP_STATUS_OK;
}

static int update_config_route(enpu_manager_t *mgr, const char *body, char *response)
{
    if (body == NULL) {
        return config_error_response(response, HTTP_STATUS_BAD_REQUEST, "Invalid JSON body");
    }
    const char *end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(body, strlen(body), &end, 0);
    if (root == NULL) {
        return config_error_response(response, HTTP_STATUS_BAD_REQUEST, "Invalid JSON body");
    }
    while (end != NULL && isspace((unsigned char)*end)) {
        end++;
    }
    if (end == NULL || *end != '\0' || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return config_error_response(response, HTTP_STATUS_BAD_REQUEST, "Invalid JSON body");
    }

    cJSON *key_item = cJSON_GetObjectItemCaseSensitive(root, "key");
    if (!cJSON_IsString(key_item) || key_item->valuestring == NULL) {
        cJSON_Delete(root);
        return config_error_response(response, HTTP_STATUS_BAD_REQUEST, "key must be a supported config name");
    }
    const char *requested_key = key_item->valuestring;
    const char *key = NULL;
    if (strcmp(requested_key, "swap-pre-watermark") == 0) {
        key = "swap-pre-watermark";
    } else if (strcmp(requested_key, "share-strategy") == 0) {
        key = "share-strategy";
    } else if (strcmp(requested_key, "log_level") == 0) {
        key = "log_level";
    } else {
        cJSON_Delete(root);
        return config_error_response(response, HTTP_STATUS_BAD_REQUEST, "Unsupported config key");
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "value");
    char value[MAX_NAME_LEN] = {0};
    bool immediate = strcmp(key, "log_level") != 0;
    if (strcmp(key, "swap-pre-watermark") == 0) {
        bool invalid_number = !cJSON_IsNumber(item) || !isfinite(item->valuedouble);
        bool invalid_range = !invalid_number && (item->valuedouble < 1 || item->valuedouble > 100);
        bool invalid_integer = !invalid_number && item->valuedouble != (double)item->valueint;
        if (invalid_number || invalid_range || invalid_integer) {
            cJSON_Delete(root);
            const char *error = "Watermark must be an integer from 1 to 100";
            return config_error_response(response, HTTP_STATUS_BAD_REQUEST, error);
        }
        (void)snprintf_s(value, sizeof(value), sizeof(value) - 1, "%d", item->valueint);
    } else {
        if (!cJSON_IsString(item) || item->valuestring == NULL || strlen(item->valuestring) >= sizeof(value)) {
            cJSON_Delete(root);
            return config_error_response(response, HTTP_STATUS_BAD_REQUEST, "value must be a supported string");
        }
        if (strcmp(key, "share-strategy") == 0 && strcmp(item->valuestring, "compact") != 0 &&
            strcmp(item->valuestring, "anti-fragment") != 0) {
            cJSON_Delete(root);
            return config_error_response(response, HTTP_STATUS_BAD_REQUEST, "Unsupported share strategy");
        }
        if (strcmp(key, "log_level") == 0 && strcmp(item->valuestring, "debug") != 0 &&
            strcmp(item->valuestring, "info") != 0 && strcmp(item->valuestring, "warn") != 0 &&
            strcmp(item->valuestring, "error") != 0 && strcmp(item->valuestring, "fatal") != 0) {
            cJSON_Delete(root);
            return config_error_response(response, HTTP_STATUS_BAD_REQUEST, "Unsupported log level");
        }
        (void)strncpy_s(value, sizeof(value), item->valuestring, strlen(item->valuestring));
    }
    cJSON_Delete(root);

    if (enpu_config_update_value(ENPU_CONFIG_DEFAULT_PATH, key, value) != ENPU_SUCCESS) {
        return config_error_response(response, HTTP_STATUS_INTERNAL_ERROR, "Unable to update config file");
    }
    int result = ENPU_SUCCESS;
    if (strcmp(key, "swap-pre-watermark") == 0) {
        result = enpu_manager_set_swap_pre_watermark(mgr, atoi(value));
    } else if (strcmp(key, "share-strategy") == 0) {
        result = enpu_manager_set_share_strategy(mgr, value);
    }
    if (result != ENPU_SUCCESS) {
        return config_error_response(response, HTTP_STATUS_INTERNAL_ERROR,
                                     "Config saved but runtime update failed; restart required");
    }
    const char *format = "{\"result\":0,\"restart_required\":%s}";
    (void)snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1, format, immediate ? "false" : "true");
    return HTTP_STATUS_OK;
}

static int route_request(rest_api_server_t *server, const char *method, const char *path, const char *body,
                         char *response)
{
    /* 配置更新与比例调整共用文件，串行覆盖保存、应用和失败恢复全过程。 */
    static pthread_mutex_t config_update_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&server->lock);
    enpu_manager_t *mgr = server->manager;
    pthread_mutex_unlock(&server->lock);

    int ret = 0;

    if (!mgr) {
        ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                         "{\"error\":\"Manager not initialized\",\"result\":%d}", ENPU_FAIL);
        CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
        return HTTP_STATUS_INTERNAL_ERROR;
    }

    if (strcmp(path, "/api/v1/config") == 0) {
        if (strcmp(method, "GET") == 0) {
            return get_config_route(response);
        }
        if (strcmp(method, "PUT") != 0) {
            return config_error_response(response, HTTP_STATUS_METHOD_NOT_ALLOWED, "GET or PUT required");
        }
        pthread_mutex_lock(&config_update_lock);
        int status = update_config_route(mgr, body, response);
        pthread_mutex_unlock(&config_update_lock);
        return status;
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/devices") == 0) {
        char *json = format_devices_json(mgr);
        if (json) {
            ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1, "%s", json);
            CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
            free(json);
            return HTTP_STATUS_OK;
        }
        ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                         "{\"error\":\"Failed to list devices\",\"result\":%d}", ENPU_FAIL);
        CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
        return HTTP_STATUS_INTERNAL_ERROR;
    }

    if (strcmp(method, "GET") == 0 && strncmp(path, "/api/v1/devices/", 16) == 0) {
        long phy_id = is_decimal_string(path + 16) ? strtol(path + 16, NULL, 10) : -1;
        if (phy_id < 0 || phy_id >= MAX_NPU_PER_NODE) {
            ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                             "{\"error\":\"Invalid device id\",\"result\":%d}", ENPU_INVALID_PARAM);
            CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
            return HTTP_STATUS_BAD_REQUEST;
        }
        char *json = format_device_json(mgr, (int)phy_id);
        if (json) {
            ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1, "%s", json);
            CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
            free(json);
            return HTTP_STATUS_OK;
        }
        ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                         "{\"error\":\"Device not found\",\"result\":%d}", ENPU_NOT_FOUND);
        CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
        return HTTP_STATUS_NOT_FOUND;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/config/oversub-ratio") == 0) {
        pthread_mutex_lock(&config_update_lock);
        /* body: {"phy_id":3,"ratio":2.0} */
        int phy_id = -1;
        double ratio = -1.0;
        if (parse_ratio_request(body, MAX_NPU_PER_NODE, &phy_id, &ratio) != 0) {
            snprintf_s(response, REST_RESPONSE_SIZE, (REST_RESPONSE_SIZE)-1,
                       "{\"error\":\"Invalid phy_id or ratio\",\"result\":%d}", ENPU_INVALID_PARAM);
            pthread_mutex_unlock(&config_update_lock);
            return HTTP_STATUS_BAD_REQUEST;
        }

        /* CSV 按 phy_id 索引，仅写到实际最大 phy_id；稀疏编号保留占位。 */
        device_info_t devices[MAX_NPU_PER_NODE];
        int device_count = MAX_NPU_PER_NODE;
        if (enpu_manager_query_devices(mgr, NULL, devices, &device_count) != ENPU_SUCCESS || device_count <= 0 ||
            device_count > MAX_NPU_PER_NODE) {
            snprintf_s(response, REST_RESPONSE_SIZE, (REST_RESPONSE_SIZE)-1,
                       "{\"error\":\"Failed to read physical devices\",\"result\":%d}", ENPU_FAIL);
            pthread_mutex_unlock(&config_update_lock);
            return HTTP_STATUS_INTERNAL_ERROR;
        }
        int phy_id_span = 0;
        bool phy_exists = false;
        for (int i = 0; i < device_count; i++) {
            int id = devices[i].phy_id;
            if (id < 0 || id >= MAX_NPU_PER_NODE) {
                snprintf_s(response, REST_RESPONSE_SIZE, (REST_RESPONSE_SIZE)-1,
                           "{\"error\":\"Invalid physical device topology\",\"result\":%d}", ENPU_FAIL);
                pthread_mutex_unlock(&config_update_lock);
                return HTTP_STATUS_INTERNAL_ERROR;
            }
            if (id + 1 > phy_id_span)
                phy_id_span = id + 1;
            if (id == phy_id)
                phy_exists = true;
        }
        if (!phy_exists) {
            snprintf_s(response, REST_RESPONSE_SIZE, (REST_RESPONSE_SIZE)-1,
                       "{\"error\":\"Invalid phy_id or ratio\",\"result\":%d}", ENPU_INVALID_PARAM);
            pthread_mutex_unlock(&config_update_lock);
            return HTTP_STATUS_BAD_REQUEST;
        }

        /* 读当前全量 ratio 数组，组新数组 new[phy_id]=ratio。
         * 必须判返回值：失败时 ratios 全 0，直接落盘会把整节点 ratio 清零。 */
        double ratios[MAX_NPU_PER_NODE] = {0};
        if (enpu_manager_get_oversub_ratio(mgr, ratios) != ENPU_SUCCESS) {
            snprintf_s(response, REST_RESPONSE_SIZE, (REST_RESPONSE_SIZE)-1,
                       "{\"error\":\"Failed to read current oversub ratios\",\"result\":%d}", ENPU_FAIL);
            pthread_mutex_unlock(&config_update_lock);
            return HTTP_STATUS_INTERNAL_ERROR;
        }
        ratios[phy_id] = ratio;

        bool cfg_existed = (access(ENPU_CONFIG_DEFAULT_PATH, F_OK) == 0);

        /* 先落盘，再内存重建 */
        int wret = enpu_config_write_oversub_ratio(ENPU_CONFIG_DEFAULT_PATH, ratios, phy_id_span);
        if (wret != ENPU_SUCCESS) {
            snprintf_s(response, REST_RESPONSE_SIZE, (REST_RESPONSE_SIZE)-1,
                       "{\"error\":\"Failed to write config\",\"result\":%d}", wret);
            pthread_mutex_unlock(&config_update_lock);
            return HTTP_STATUS_INTERNAL_ERROR;
        }

        int result = enpu_manager_set_oversub_ratio(mgr, phy_id, ratio);
        if (result != ENPU_SUCCESS) {
            /* 内存重建失败：还原文件（原文件不存在时直接删掉新建的） */
            char bak[MAX_PATH_LEN];
            snprintf_s(bak, sizeof(bak), (sizeof(bak)) - 1, "%s.bak", ENPU_CONFIG_DEFAULT_PATH);
            if (cfg_existed) {
                if (rename(bak, ENPU_CONFIG_DEFAULT_PATH) != 0) {
                    LOG_ERROR("[REST] failed to restore config from %s", bak);
                }
            } else {
                unlink(ENPU_CONFIG_DEFAULT_PATH);
            }
            /* 区分失败类别，避免任何失败都显示同一句 "Failed to apply ratio"；
             * 具体原因（活跃分配数/哪个 vnpu 仍换出/哪条命令在途）见日志 set_oversub_ratio busy */
            const char *err;
            switch (result) {
                case ENPU_BUSY:
                    err = "Die busy: active allocation, swapped data, or in-flight swap command";
                    break;
                case ENPU_INVALID_PARAM:
                    err = "Invalid phy_id or ratio";
                    break;
                case ENPU_NO_RESOURCE:
                    err = "Node swap buffer exceeds host MemFree";
                    break;
                default:
                    err = "Failed to rebuild swap pool; see enpu-manager log";
                    break;
            }
            snprintf_s(response, REST_RESPONSE_SIZE, (REST_RESPONSE_SIZE)-1, "{\"error\":\"%s\",\"result\":%d}", err,
                       result);
            pthread_mutex_unlock(&config_update_lock);
            return (result == ENPU_BUSY || result == ENPU_INVALID_PARAM) ? HTTP_STATUS_BAD_REQUEST :
                                                                           HTTP_STATUS_INTERNAL_ERROR;
        }

        snprintf_s(response, REST_RESPONSE_SIZE, (REST_RESPONSE_SIZE)-1, "{\"result\":0}");
        pthread_mutex_unlock(&config_update_lock);
        return HTTP_STATUS_OK;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/allocate") == 0) {
        alloc_request_t req;
        alloc_response_t resp = {0};
        char parse_error[128] = {0};
        int parse_result = parse_json_alloc_request(body, &req, parse_error, sizeof(parse_error));
        if (parse_result != 0) {
            ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1, "{\"error\":\"%s\",\"result\":%d}",
                             parse_error[0] == '\0' ? "Invalid request body" : parse_error, ENPU_INVALID_PARAM);
            CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
            return HTTP_STATUS_BAD_REQUEST;
        }
        int result = enpu_manager_allocate(mgr, &req, &resp);
        if (result != ENPU_SUCCESS) {
            resp.result = result; /* 用返回值覆盖 resp.result，避免 rollback 后 resp.result 仍为 SUCCESS */
        }
        LOG_DEBUG("[REST-ALLOC] result=%d allocation_id=%s phy=%d vnpu=%d", result, resp.allocation_id, resp.phy_id,
                  resp.vnpu_id);
        if (copy_json_response(response, format_alloc_response_json(&resp, body)) != ENPU_SUCCESS) {
            LOG_ERROR("[REST-ALLOC] Failed to serialize response");
            return HTTP_STATUS_INTERNAL_ERROR;
        }
        return result == ENPU_SUCCESS        ? HTTP_STATUS_OK :
               result == ENPU_ALREADY_EXISTS ? HTTP_STATUS_CONFLICT :
                                               HTTP_STATUS_BAD_REQUEST;
    }

    if (strcmp(method, "DELETE") == 0 && strncmp(path, "/api/v1/allocations/", 20) == 0) {
        if (!strchr(path + 20, '/')) {
            int result = enpu_manager_release_by_id(mgr, path + 20);
            LOG_DEBUG("[REST-RELEASE] by=allocation_id allocation_id=%s result=%d", path + 20, result);
            const char *error_msg = result == ENPU_SUCCESS       ? "" :
                                    result == ENPU_INVALID_PARAM ? "Invalid allocation_id" :
                                    result == ENPU_NOT_FOUND     ? "Allocation not found" :
                                                                   "Failed to release allocation";
            if (copy_json_response(response, format_result_json(result, error_msg)) != ENPU_SUCCESS) {
                return HTTP_STATUS_INTERNAL_ERROR;
            }
            if (result == ENPU_INVALID_PARAM) {
                return HTTP_STATUS_BAD_REQUEST;
            }
            return result == ENPU_SUCCESS   ? HTTP_STATUS_OK :
                   result == ENPU_NOT_FOUND ? HTTP_STATUS_NOT_FOUND :
                                              HTTP_STATUS_INTERNAL_ERROR;
        }
    }

    if (strcmp(method, "DELETE") == 0 &&
        (strcmp(path, "/api/v1/allocations") == 0 || strncmp(path, "/api/v1/allocations?", 20) == 0)) {
        char pod_uid[MAX_UUID_LEN], container[MAX_NAME_LEN];
        if (parse_release_query(strchr(path, '?') ? path + 20 : NULL, pod_uid, sizeof(pod_uid), container,
                                sizeof(container)) != 0) {
            ret =
                snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                           "{\"error\":\"Require nonempty pod_uid and container_name query parameters\",\"result\":%d}",
                           ENPU_INVALID_PARAM);
            CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
            return HTTP_STATUS_BAD_REQUEST;
        }
        int result = enpu_manager_release(mgr, pod_uid, container);
        LOG_DEBUG("[REST-RELEASE] by=metadata pod=%s container=%s result=%d", pod_uid, container, result);
        const char *error_msg = "Failed to release allocation";
        if (result == ENPU_SUCCESS) {
            error_msg = "";
        } else if (result == ENPU_ALREADY_EXISTS) {
            error_msg = "Multiple allocations match metadata; use allocation_id";
        } else if (result == ENPU_NOT_FOUND) {
            error_msg = "Allocation not found";
        } else if (result == ENPU_INVALID_PARAM) {
            error_msg = "Invalid pod_uid or container_name";
        }
        if (copy_json_response(response, format_result_json(result, error_msg)) != ENPU_SUCCESS) {
            return HTTP_STATUS_INTERNAL_ERROR;
        }
        if (result == ENPU_ALREADY_EXISTS) {
            return HTTP_STATUS_CONFLICT;
        }
        return result == ENPU_SUCCESS       ? HTTP_STATUS_OK :
               result == ENPU_NOT_FOUND     ? HTTP_STATUS_NOT_FOUND :
               result == ENPU_INVALID_PARAM ? HTTP_STATUS_BAD_REQUEST :
                                              HTTP_STATUS_INTERNAL_ERROR;
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/allocations") == 0) {
        char *json = format_allocations_json(mgr);
        if (copy_json_response(response, json) == ENPU_SUCCESS) {
            return HTTP_STATUS_OK;
        }
        ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                         "{\"error\":\"Failed to list allocations\",\"result\":%d}", ENPU_FAIL);
        CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
        return HTTP_STATUS_INTERNAL_ERROR;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/checkpoint") == 0) {
        int result = enpu_manager_control(mgr, CONTROL_OP_CHECKPOINT);
        ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1, "{\"result\":%d,\"error_msg\":\"\"}",
                         result);
        CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
        return result == ENPU_SUCCESS ? HTTP_STATUS_OK : HTTP_STATUS_INTERNAL_ERROR;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/recover") == 0) {
        int result = enpu_manager_control(mgr, CONTROL_OP_RECOVER);
        ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1, "{\"result\":%d,\"error_msg\":\"\"}",
                         result);
        CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
        return result == ENPU_SUCCESS ? HTTP_STATUS_OK : HTTP_STATUS_INTERNAL_ERROR;
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/swap/status") == 0) {
        swap_status_response_t swap_resp;
        int result = enpu_manager_swap_status(mgr, &swap_resp);
        if (result == ENPU_SUCCESS) {
            int offset = 0;
            int fmt_ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                                     "{\"buffer\":{\"used\":%lu,\"free\":%lu,\"total\":%lu},\"swapped_models\":[",
                                     swap_resp.buffer_status.used, swap_resp.buffer_status.free,
                                     swap_resp.buffer_status.total);
            CHECK_COND_LOG_(fmt_ret < 0, "snprintf_s swap status failed.");
            offset = (int)strlen(response);

            for (int i = 0; i < swap_resp.swapped_count && offset < REST_RESPONSE_SIZE - 200; i++) {
                if (i > 0) {
                    fmt_ret = snprintf_s(response + offset, REST_RESPONSE_SIZE - offset,
                                         REST_RESPONSE_SIZE - offset - 1, ",");
                    if (fmt_ret < 0) {
                        break;
                    }
                    offset += (int)strlen(response + offset);
                }
                fmt_ret = snprintf_s(response + offset, REST_RESPONSE_SIZE - offset, REST_RESPONSE_SIZE - offset - 1,
                                     "{\"pod_uid\":\"%s\",\"vnpu_id\":%d,\"die_id\":\"%s\",\"swap_size\":%lu}",
                                     swap_resp.swapped_models[i].pod_uid, swap_resp.swapped_models[i].vnpu_id,
                                     swap_resp.swapped_models[i].die_id, swap_resp.swapped_models[i].swap_size);
                if (fmt_ret < 0) {
                    break;
                }
                offset += (int)strlen(response + offset);
            }
            fmt_ret = snprintf_s(response + offset, REST_RESPONSE_SIZE - offset, REST_RESPONSE_SIZE - offset - 1,
                                 "],\"result\":%d}", result);
            CHECK_COND_LOG_(fmt_ret < 0, "snprintf_s swap status failed.");
            return HTTP_STATUS_OK;
        }
        ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                         "{\"error\":\"Failed to get swap status\",\"result\":%d}", result);
        CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
        return HTTP_STATUS_INTERNAL_ERROR;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/swap/clean") == 0) {
        char pod_uid[MAX_UUID_LEN] = "";
        int copy_ret = 0;
        char *p = strstr(body, "\"pod_uid\"");
        if (p) {
            char *v = strchr(p + 9, '"');
            if (v) {
                char *e = strchr(v + 1, '"');
                if (e)
                    copy_ret = strncpy_s(pod_uid, sizeof(pod_uid), v + 1, e - (v + 1));
                CHECK_COND_LOG_(copy_ret != 0, "strncpy_s pod_uid failed.");
            }
        }
        if (pod_uid[0] == '\0') {
            ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                             "{\"error\":\"Missing pod_uid\",\"result\":%d}", ENPU_INVALID_PARAM);
            CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
            return HTTP_STATUS_BAD_REQUEST;
        }
        int result = enpu_manager_swap_control(mgr, pod_uid, SWAP_ACTION_CLEAN);
        ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1, "{\"result\":%d,\"error_msg\":\"\"}",
                         result);
        CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
        return result == ENPU_SUCCESS ? HTTP_STATUS_OK : HTTP_STATUS_NOT_FOUND;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/swap/force-out") == 0) {
        char pod_uid[MAX_UUID_LEN] = "";
        int copy_ret = 0;
        char *p = strstr(body, "\"pod_uid\"");
        if (p) {
            char *v = strchr(p + 9, '"');
            if (v) {
                char *e = strchr(v + 1, '"');
                if (e)
                    copy_ret = strncpy_s(pod_uid, sizeof(pod_uid), v + 1, e - (v + 1));
                CHECK_COND_LOG_(copy_ret != 0, "strncpy_s pod_uid failed.");
            }
        }
        if (pod_uid[0] == '\0') {
            ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                             "{\"error\":\"Missing pod_uid\",\"result\":%d}", ENPU_INVALID_PARAM);
            CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
            return HTTP_STATUS_BAD_REQUEST;
        }
        int result = enpu_manager_swap_control(mgr, pod_uid, SWAP_ACTION_FORCE_OUT);
        ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1, "{\"result\":%d,\"error_msg\":\"\"}",
                         result);
        CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
        return result == ENPU_SUCCESS ? HTTP_STATUS_OK : HTTP_STATUS_NOT_FOUND;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/swap/force-in") == 0) {
        char pod_uid[MAX_UUID_LEN] = "";
        int copy_ret = 0;
        char *p = strstr(body, "\"pod_uid\"");
        if (p) {
            char *v = strchr(p + 9, '"');
            if (v) {
                char *e = strchr(v + 1, '"');
                if (e)
                    copy_ret = strncpy_s(pod_uid, sizeof(pod_uid), v + 1, e - (v + 1));
                CHECK_COND_LOG_(copy_ret != 0, "strncpy_s pod_uid failed.");
            }
        }
        if (pod_uid[0] == '\0') {
            ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                             "{\"error\":\"Missing pod_uid\",\"result\":%d}", ENPU_INVALID_PARAM);
            CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
            return HTTP_STATUS_BAD_REQUEST;
        }
        int result = enpu_manager_swap_control(mgr, pod_uid, SWAP_ACTION_FORCE_IN);
        ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1, "{\"result\":%d,\"error_msg\":\"\"}",
                         result);
        CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
        return result == ENPU_SUCCESS ? HTTP_STATUS_OK : HTTP_STATUS_NOT_FOUND;
    }

    if (strcmp(method, "DELETE") == 0 && strcmp(path, "/api/v1/clean") == 0) {
        int released = 0;
        int result = enpu_manager_release_all(mgr, &released);
        ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                         "{\"released_count\":%d,\"result\":%d,\"error_msg\":\"\"}", released, result);
        CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
        return result == ENPU_SUCCESS ? HTTP_STATUS_OK : HTTP_STATUS_INTERNAL_ERROR;
    }

    ret = snprintf_s(response, REST_RESPONSE_SIZE, REST_RESPONSE_SIZE - 1,
                     "{\"error\":\"Unknown endpoint\",\"result\":%d}", ENPU_NOT_FOUND);
    CHECK_COND_LOG_(ret < 0, "snprintf_s response failed.");
    return HTTP_STATUS_NOT_FOUND;
}
static int parse_json_alloc_request(const char *body, alloc_request_t *req, char *error, size_t error_size)
{
    if (body == NULL || req == NULL || error == NULL || error_size == 0) {
        return ENPU_INVALID_PARAM;
    }
    error[0] = '\0';
    int ret = memset_s(req, sizeof(alloc_request_t), 0, sizeof(alloc_request_t));
    CHECK_COND_RETURN_ERROR_CODE_LOG(ret != 0, "memset_s req failed.");
    req->predicate_phy_id = -1;
    req->swap_order = SWAP_ORDER_DEFAULT;

    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithOpts(body, &parse_end, true);
    if (root == NULL || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        LOG_DEBUG("[REST-ALLOC] Invalid JSON request body near offset=%td", parse_end == NULL ? 0 : parse_end - body);
        (void)strncpy_s(error, error_size, "Request body must be a valid JSON object",
                        strlen("Request body must be a valid JSON object"));
        return ENPU_INVALID_PARAM;
    }

    bool hbm_request_present = false, hbm_quota_present = false, hbm_limit_present = false;
    uint64_t hbm_request = 0, hbm_quota = 0;
    ret =
        json_optional_string(root, "allocation_id", req->allocation_id, sizeof(req->allocation_id), error, error_size);
    if (ret == ENPU_SUCCESS && cJSON_HasObjectItem(root, "allocation_id") &&
        !allocation_id_is_valid(req->allocation_id)) {
        set_json_field_error(error, error_size, "allocation_id", "must be a valid UUID");
        ret = ENPU_INVALID_PARAM;
    }
    if (ret == ENPU_SUCCESS) {
        ret = json_optional_string(root, "pod_uid", req->pod_uid, sizeof(req->pod_uid), error, error_size);
    }
    if (ret == ENPU_SUCCESS) {
        ret = json_optional_string(root, "container_name", req->container_name, sizeof(req->container_name), error,
                                   error_size);
    }
    if (ret == ENPU_SUCCESS) {
        ret = json_optional_int(root, "aicore_quota", &req->aicore_quota, error, error_size);
    }
    if (ret == ENPU_SUCCESS) {
        ret = json_optional_uint64(root, "hbm_request", &hbm_request, &hbm_request_present, error, error_size);
    }
    if (ret == ENPU_SUCCESS) {
        ret = json_optional_uint64(root, "hbm_quota", &hbm_quota, &hbm_quota_present, error, error_size);
    }
    if (ret == ENPU_SUCCESS) {
        ret = json_optional_uint64(root, "hbm_limit", &req->hbm_limit, &hbm_limit_present, error, error_size);
    }
    if (ret == ENPU_SUCCESS) {
        ret = json_optional_int(root, "sched_policy", &req->sched_policy, error, error_size);
    }
    if (ret == ENPU_SUCCESS) {
        ret = json_optional_int(root, "vnpu_priority", &req->vnpu_priority, error, error_size);
        if (ret == ENPU_SUCCESS && req->vnpu_priority != VNPU_PRIORITY_NORMAL &&
            req->vnpu_priority != VNPU_PRIORITY_HIGH) {
            set_json_field_error(error, error_size, "vnpu_priority", "must be 0 or 1");
            ret = ENPU_INVALID_PARAM;
        }
    }
    if (ret == ENPU_SUCCESS) {
        ret = json_optional_int(root, "swap_order", &req->swap_order, error, error_size);
    }
    if (ret == ENPU_SUCCESS && (req->swap_order < SWAP_ORDER_MIN || req->swap_order > SWAP_ORDER_MAX)) {
        LOG_DEBUG("[REST-ALLOC] Invalid swap_order=%d; expected 0, 1 or 2", req->swap_order);
        set_json_field_error(error, error_size, "swap_order", "must be 0, 1 or 2");
        ret = ENPU_INVALID_PARAM;
    }
    if (ret == ENPU_SUCCESS) {
        ret = json_optional_int(root, "phy_id", &req->predicate_phy_id, error, error_size);
    }
    if (ret == ENPU_SUCCESS && req->predicate_phy_id < -1) {
        set_json_field_error(error, error_size, "phy_id", "must be -1 or a non-negative integer");
        ret = ENPU_INVALID_PARAM;
    }
    if (ret == ENPU_SUCCESS) {
        req->hbm_quota = hbm_request_present ? hbm_request : hbm_quota_present ? hbm_quota : 0;
        if (!hbm_limit_present) {
            req->hbm_limit = req->hbm_quota;
        }
    }
    cJSON_Delete(root);
    if (ret != ENPU_SUCCESS) {
        LOG_DEBUG("[REST-ALLOC] Invalid JSON field type, range or length");
    }
    return ret;
}

static char *json_print_unformatted(cJSON *root)
{
    if (root == NULL) {
        return NULL;
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static char *format_result_json(int result, const char *error_msg)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL || cJSON_AddNumberToObject(root, "result", result) == NULL ||
        cJSON_AddStringToObject(root, "error_msg", error_msg == NULL ? "" : error_msg) == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    return json_print_unformatted(root);
}

static int copy_json_response(char *response, char *json)
{
    if (response == NULL || json == NULL) {
        free(json);
        return ENPU_FAIL;
    }
    size_t length = strlen(json);
    int ret = length < REST_RESPONSE_SIZE ? strncpy_s(response, REST_RESPONSE_SIZE, json, length) : ERANGE;
    free(json);
    return ret == 0 ? ENPU_SUCCESS : ENPU_FAIL;
}

static char *format_alloc_response_json(const alloc_response_t *resp, const char *request_body)
{
    if (resp == NULL) {
        return NULL;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *metadata = cJSON_CreateObject();
    if (root == NULL || metadata == NULL ||
        cJSON_AddStringToObject(root, "allocation_id", resp->allocation_id) == NULL ||
        cJSON_AddStringToObject(root, "config_path", resp->config_path) == NULL ||
        cJSON_AddNumberToObject(root, "phy_id", resp->phy_id) == NULL ||
        cJSON_AddNumberToObject(root, "vnpu_id", resp->vnpu_id) == NULL ||
        cJSON_AddStringToObject(metadata, "pod_uid", resp->pod_uid) == NULL ||
        cJSON_AddStringToObject(metadata, "container_name", resp->container_name) == NULL) {
        cJSON_Delete(metadata);
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddItemToObject(root, "metadata", metadata);
    if (cJSON_AddStringToObject(root, "die_id", resp->die_id) == NULL ||
        cJSON_AddStringToObject(root, "shm_id", resp->shm_id) == NULL ||
        cJSON_AddNumberToObject(root, "aicore_quota", resp->aicore_quota) == NULL ||
        json_add_uint64(root, "hbm_quota", resp->hbm_quota) != ENPU_SUCCESS ||
        json_add_uint64(root, "hbm_limit", resp->hbm_limit) != ENPU_SUCCESS ||
        cJSON_AddNumberToObject(root, "sched_policy", resp->sched_policy) == NULL ||
        cJSON_AddNumberToObject(root, "vnpu_priority", resp->vnpu_priority) == NULL ||
        cJSON_AddStringToObject(root, "minor_name", resp->minor_name) == NULL ||
        cJSON_AddNumberToObject(root, "result", resp->result) == NULL ||
        cJSON_AddStringToObject(root, "error_msg", resp->error_msg) == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    if (resp->result == ENPU_SUCCESS && resp->vnpu_priority == VNPU_PRIORITY_HIGH) {
        /* Read the original fields: the allocator normalizes quota and policy in place. */
        cJSON *request = cJSON_Parse(request_body);
        cJSON *warnings = cJSON_CreateArray();
        if (request == NULL || warnings == NULL) {
            cJSON_Delete(request);
            cJSON_Delete(warnings);
            cJSON_Delete(root);
            return NULL;
        }
        const cJSON *quota = cJSON_GetObjectItemCaseSensitive(request, "aicore_quota");
        const cJSON *policy = cJSON_GetObjectItemCaseSensitive(request, "sched_policy");
        const char *messages[] = {
            quota != NULL && quota->valueint != 0 ?
                "aicore_quota is ignored when vnpu_priority=1; the effective value is 0." :
                NULL,
            policy != NULL && policy->valueint != SCHED_POLICY_BEST_EFFORT ?
                "sched_policy is ignored when vnpu_priority=1; the effective value is 3 (best-effort)." :
                NULL};
        for (size_t i = 0; i < sizeof(messages) / sizeof(messages[0]); ++i) {
            if (messages[i] == NULL) {
                continue;
            }
            cJSON *message = cJSON_CreateString(messages[i]);
            if (message == NULL || !cJSON_AddItemToArray(warnings, message)) {
                cJSON_Delete(message);
                cJSON_Delete(request);
                cJSON_Delete(warnings);
                cJSON_Delete(root);
                return NULL;
            }
        }
        cJSON_Delete(request);
        if (cJSON_GetArraySize(warnings) > 0) {
            if (!cJSON_AddItemToObject(root, "warnings", warnings)) {
                cJSON_Delete(warnings);
                cJSON_Delete(root);
                return NULL;
            }
        } else {
            cJSON_Delete(warnings);
        }
    }
    return json_print_unformatted(root);
}

static char *format_devices_json(enpu_manager_t *mgr)
{
    if (!mgr) {
        return strdup("{\"devices\":[],\"error_msg\":\"manager is null\"}");
    }

    device_info_t devices[MAX_NPU_PER_NODE] = {0};
    int count = 0;
    int ret = enpu_manager_query_devices(mgr, NULL, devices, &count);
    if (ret != ENPU_SUCCESS) {
        char err[128];
        int fmt_ret = snprintf_s(err, sizeof(err), sizeof(err) - 1,
                                 "{\"devices\":[],\"error_msg\":\"query failed: ret=%d\"}", ret);
        CHECK_COND_LOG_(fmt_ret < 0, "snprintf_s err failed.");
        return strdup(err);
    }
    if (count == 0) {
        return strdup("{\"devices\":[]}");
    }

    const size_t JSON_BUF_SIZE = 16384;
    char *json = (char *)malloc(JSON_BUF_SIZE);
    if (!json) {
        return strdup("{\"devices\":[],\"error_msg\":\"OOM: failed to alloc json buffer\"}");
    }

    int offset = 0;
    int i = 0;
    int fmt_ret = snprintf_s(json, JSON_BUF_SIZE, JSON_BUF_SIZE - 1, "{\"devices\":[");
    if (fmt_ret < 0) {
        goto overflow;
    }
    offset = (int)strlen(json);

    for (i = 0; i < count; i++) {
        if (i > 0) {
            fmt_ret = snprintf_s(json + offset, JSON_BUF_SIZE - offset, JSON_BUF_SIZE - offset - 1, ",");
            if (fmt_ret < 0) {
                goto overflow;
            }
            offset += (int)strlen(json + offset);
        }

        int aicore = atomic_load(&devices[i].allocatable.aicore_quota);
        uint64_t hbm = atomic_load(&devices[i].allocatable.hbm_quota);

        fmt_ret = snprintf_s(json + offset, JSON_BUF_SIZE - offset, JSON_BUF_SIZE - offset - 1,
                             "{\"phy_id\":%d,\"minor_name\":\"%s\",\"uuid\":\"%s\","
                             "\"aicore_available\":%d,\"hbm_available\":%lu,"
                             "\"vnpu_available\":%d,\"oversub_ratio\":%.2f}",
                             devices[i].meta.phy_id, devices[i].meta.minor_name, devices[i].meta.uuid, aicore, hbm,
                             MAX_VNPU_PER_DIE - devices[i].allocatable.vnpu_count, devices[i].oversub_ratio);
        if (fmt_ret < 0) {
            goto overflow;
        }
        offset += (int)strlen(json + offset);
    }

    fmt_ret = snprintf_s(json + offset, JSON_BUF_SIZE - offset, JSON_BUF_SIZE - offset - 1, "]}");
    CHECK_COND_LOG_(fmt_ret < 0, "snprintf_s devices tail failed.");
    return json;

overflow:
    if (offset >= 0 && (size_t)offset < JSON_BUF_SIZE) {
        fmt_ret =
            snprintf_s(json + offset, JSON_BUF_SIZE - offset, JSON_BUF_SIZE - offset - 1,
                       "],\"error_msg\":\"json buffer overflow at item %d (total %d), only first %d items returned\"",
                       i, count, i);
        CHECK_COND_LOG_(fmt_ret < 0, "snprintf_s overflow msg failed.");
    }
    return json;
}
static char *format_device_json(enpu_manager_t *mgr, int phy_id)
{
    if (!mgr) {
        return strdup("{\"error_msg\":\"manager is null\"}");
    }

    /* 复用 enpu_manager_query_devices 的单卡过滤 */
    device_query_t query = {0};
    query.phy_id = phy_id;

    device_info_t dev;
    int count = 0;
    int ret = enpu_manager_query_devices(mgr, &query, &dev, &count);
    if (ret != ENPU_SUCCESS) {
        char err[128];
        int fmt_ret = snprintf_s(err, sizeof(err), sizeof(err) - 1, "{\"error_msg\":\"query failed: ret=%d\"}", ret);
        CHECK_COND_LOG_(fmt_ret < 0, "snprintf_s err failed.");
        return strdup(err);
    }
    if (count == 0) {
        return NULL; // 404
    }

    int aicore = atomic_load(&dev.allocatable.aicore_quota);
    uint64_t hbm = atomic_load(&dev.allocatable.hbm_quota);

    char *json = (char *)malloc(DEVICE_JSON_BUF_SIZE);
    if (!json) {
        return strdup("{\"error_msg\":\"OOM: failed to alloc json buffer\"}");
    }

    int fmt_ret = snprintf_s(json, DEVICE_JSON_BUF_SIZE, DEVICE_JSON_BUF_SIZE - 1,
                             "{\"phy_id\":%d,\"die_id\":\"%s\","
                             "\"aicore_available\":%d,\"hbm_available\":%lu,"
                             "\"vnpu_available\":%d,\"total_memory\":%lu,"
                             "\"oversub_ratio\":%.2f}",
                             dev.meta.phy_id, dev.meta.die_id, aicore, hbm,
                             MAX_VNPU_PER_DIE - dev.allocatable.vnpu_count, (unsigned long)dev.meta.total_memory,
                             dev.oversub_ratio);
    CHECK_COND_LOG_(fmt_ret < 0, "snprintf_s device json failed.");
    return json;
}
static char *format_allocations_error(const char *error_msg)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *items = cJSON_CreateArray();
    if (root == NULL || items == NULL) {
        cJSON_Delete(items);
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddItemToObject(root, "allocations", items);
    if (error_msg != NULL && cJSON_AddStringToObject(root, "error_msg", error_msg) == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    return json_print_unformatted(root);
}

static char *format_allocations_json(enpu_manager_t *mgr)
{
    if (mgr == NULL) {
        return format_allocations_error("manager is null");
    }
    allocation_t *allocations = calloc(MAX_ALLOC_COUNT, sizeof(*allocations));
    if (allocations == NULL) {
        return format_allocations_error("OOM: failed to allocate registry buffer");
    }

    int count = 0;
    allocation_query_t query = {.phy_id = -1};
    int ret = enpu_manager_query_allocations(mgr, &query, allocations, &count);
    LOG_DEBUG("[REST-ALLOCATIONS] list result=%d count=%d", ret, count);
    if (ret != ENPU_SUCCESS || count < 0 || count > MAX_ALLOC_COUNT) {
        char error_msg[128];
        if (snprintf_s(error_msg, sizeof(error_msg), sizeof(error_msg) - 1, "query failed: ret=%d count=%d", ret,
                       count) < 0) {
            free(allocations);
            return NULL;
        }
        free(allocations);
        return format_allocations_error(error_msg);
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *items = cJSON_CreateArray();
    if (root == NULL || items == NULL) {
        cJSON_Delete(items);
        cJSON_Delete(root);
        free(allocations);
        return NULL;
    }
    cJSON_AddItemToObject(root, "allocations", items);

    for (int i = 0; i < count; ++i) {
        const allocation_t *alloc = &allocations[i];
        char config_path[MAX_PATH_LEN];
        if (enpu_manager_get_config_path(mgr, alloc->allocation_id, config_path, sizeof(config_path)) == NULL) {
            LOG_ERROR("[REST-ALLOCATIONS] Failed to resolve config path: allocation_id=%s", alloc->allocation_id);
            cJSON_Delete(root);
            free(allocations);
            return NULL;
        }
        cJSON *item = cJSON_CreateObject();
        cJSON *metadata = cJSON_CreateObject();
        if (item == NULL || metadata == NULL ||
            cJSON_AddStringToObject(item, "allocation_id", alloc->allocation_id) == NULL ||
            cJSON_AddStringToObject(item, "config_path", config_path) == NULL ||
            cJSON_AddStringToObject(metadata, "pod_uid", alloc->pod_uid) == NULL ||
            cJSON_AddStringToObject(metadata, "container_name", alloc->container_name) == NULL) {
            cJSON_Delete(metadata);
            cJSON_Delete(item);
            cJSON_Delete(root);
            free(allocations);
            return NULL;
        }
        cJSON_AddItemToObject(item, "metadata", metadata);
        if (cJSON_AddNumberToObject(item, "phy_id", alloc->phy_id) == NULL ||
            cJSON_AddNumberToObject(item, "vnpu_id", alloc->vnpu_id) == NULL ||
            cJSON_AddStringToObject(item, "die_id", alloc->die_id) == NULL ||
            cJSON_AddNumberToObject(item, "aicore_quota", alloc->aicore_quota) == NULL ||
            json_add_uint64(item, "hbm_quota", alloc->hbm_quota) != ENPU_SUCCESS ||
            json_add_uint64(item, "hbm_limit", alloc->hbm_limit) != ENPU_SUCCESS ||
            cJSON_AddNumberToObject(item, "sched_policy", alloc->sched_policy) == NULL ||
            cJSON_AddNumberToObject(item, "swap_order", alloc->swap_order) == NULL ||
            cJSON_AddNumberToObject(item, "vnpu_priority", alloc->vnpu_priority) == NULL) {
            cJSON_Delete(item);
            cJSON_Delete(root);
            free(allocations);
            return NULL;
        }
        cJSON_AddItemToArray(items, item);
    }
    free(allocations);
    return json_print_unformatted(root);
}
