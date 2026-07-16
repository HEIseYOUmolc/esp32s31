/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "cJSON.h"

#include "ai_toy_ws_client.h"
#include "ai_toy_ws_protocol.h"

struct ai_toy_ws_client_s {
    char url[256];
    char device_sn[64];
    char auth_token[256];
    char headers[384];
    bool connected;
    int reconnect_timeout_ms;
    esp_websocket_client_handle_t client;
    ai_toy_ws_event_cb_t event_cb;
    void *event_ctx;
    uint8_t *text_buf;
    size_t text_len;
    size_t text_cap;
};

static const char *TAG = "AI_TOY_WS";

static void ai_toy_ws_emit(ai_toy_ws_client_handle_t handle, ai_toy_ws_event_t event, const void *event_data)
{
    if (handle->event_cb != NULL) {
        handle->event_cb(event, event_data, handle->event_ctx);
    }
}

static void ai_toy_ws_emit_error(ai_toy_ws_client_handle_t handle, esp_err_t code, const char *message)
{
    ai_toy_ws_error_t error = {
        .code = code,
        .message = message,
    };
    ai_toy_ws_emit(handle, AI_TOY_WS_EVENT_ERROR, &error);
}

static void ai_toy_ws_clear_text(ai_toy_ws_client_handle_t handle)
{
    free(handle->text_buf);
    handle->text_buf = NULL;
    handle->text_len = 0;
    handle->text_cap = 0;
}

static esp_err_t ai_toy_ws_append_text(ai_toy_ws_client_handle_t handle, const char *data, int data_len,
                                       int payload_offset, int payload_len)
{
    ESP_RETURN_ON_FALSE(data != NULL && data_len > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid text chunk");
    ESP_RETURN_ON_FALSE(payload_offset >= 0, ESP_ERR_INVALID_ARG, TAG, "Invalid text offset");
    ESP_RETURN_ON_FALSE((size_t)payload_offset + (size_t)data_len <= CONFIG_AI_TOY_WS_MAX_TEXT_PAYLOAD,
                        ESP_ERR_INVALID_SIZE, TAG, "Text payload too large");

    size_t need = (size_t)payload_offset + (size_t)data_len;
    if (handle->text_buf == NULL) {
        size_t alloc = need + 1;
        if (payload_len > 0 && (size_t)payload_len < CONFIG_AI_TOY_WS_MAX_TEXT_PAYLOAD) {
            alloc = (size_t)payload_len + 1;
        }
        handle->text_buf = calloc(1, alloc);
        ESP_RETURN_ON_FALSE(handle->text_buf != NULL, ESP_ERR_NO_MEM, TAG, "No memory for text payload");
        handle->text_cap = alloc;
    }
    if (need + 1 > handle->text_cap) {
        uint8_t *new_buf = realloc(handle->text_buf, need + 1);
        ESP_RETURN_ON_FALSE(new_buf != NULL, ESP_ERR_NO_MEM, TAG, "No memory to grow text payload");
        handle->text_buf = new_buf;
        handle->text_cap = need + 1;
    }

    memcpy(handle->text_buf + payload_offset, data, (size_t)data_len);
    handle->text_buf[need] = '\0';
    if (need > handle->text_len) {
        handle->text_len = need;
    }
    return ESP_OK;
}

static void ai_toy_ws_handle_text(ai_toy_ws_client_handle_t handle, const char *data, size_t len)
{
    esp_err_t ret = ai_toy_ws_parse_server_message(data, len, handle->event_cb, handle->event_ctx);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to parse server message: %s", esp_err_to_name(ret));
        ai_toy_ws_emit_error(handle, ret, "invalid server JSON");
    }
}

static void ai_toy_ws_event(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)base;
    ai_toy_ws_client_handle_t handle = (ai_toy_ws_client_handle_t)handler_args;
    esp_websocket_event_data_t *event = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        handle->connected = true;
        ai_toy_ws_emit(handle, AI_TOY_WS_EVENT_CONNECTED, NULL);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
    case WEBSOCKET_EVENT_CLOSED:
    case WEBSOCKET_EVENT_FINISH:
        handle->connected = false;
        ai_toy_ws_clear_text(handle);
        ai_toy_ws_emit(handle, AI_TOY_WS_EVENT_DISCONNECTED, NULL);
        break;
    case WEBSOCKET_EVENT_DATA:
        if (event->op_code == 0x1) {
            if (event->fin && handle->text_buf == NULL) {
                ai_toy_ws_handle_text(handle, event->data_ptr, (size_t)event->data_len);
                break;
            }
            esp_err_t ret = ai_toy_ws_append_text(handle, event->data_ptr, event->data_len,
                                                  event->payload_offset, event->payload_len);
            if (ret != ESP_OK) {
                ai_toy_ws_clear_text(handle);
                ai_toy_ws_emit_error(handle, ret, "text reassembly failed");
                break;
            }
            if (event->fin) {
                ai_toy_ws_handle_text(handle, (const char *)handle->text_buf, handle->text_len);
                ai_toy_ws_clear_text(handle);
            }
        } else if (event->op_code == 0x2 && event->data_ptr != NULL && event->data_len > 0) {
            ai_toy_ws_audio_data_t audio = {
                .data = (const uint8_t *)event->data_ptr,
                .len = (size_t)event->data_len,
            };
            ai_toy_ws_emit(handle, AI_TOY_WS_EVENT_AUDIO_DATA, &audio);
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ai_toy_ws_emit_error(handle, ESP_FAIL, "websocket error");
        break;
    default:
        break;
    }
}

static esp_err_t ai_toy_ws_send_text(ai_toy_ws_client_handle_t handle, const char *text)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(text != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid text");
    ESP_RETURN_ON_FALSE(handle->client != NULL, ESP_ERR_INVALID_STATE, TAG, "WebSocket is not initialized");
    ESP_RETURN_ON_FALSE(handle->connected, ESP_ERR_INVALID_STATE, TAG, "WebSocket is not connected");

    int sent = esp_websocket_client_send_text(handle->client, text, strlen(text), portMAX_DELAY);
    if (sent < 0) {
        handle->connected = false;
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t ai_toy_ws_client_init(const ai_toy_ws_client_config_t *config, ai_toy_ws_client_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid config");
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(config->url != NULL && config->url[0] != '\0', ESP_ERR_INVALID_ARG, TAG, "Invalid URL");

    ai_toy_ws_client_handle_t client = calloc(1, sizeof(*client));
    ESP_RETURN_ON_FALSE(client != NULL, ESP_ERR_NO_MEM, TAG, "No memory for client");

    strlcpy(client->url, config->url, sizeof(client->url));
    strlcpy(client->device_sn, config->device_sn != NULL ? config->device_sn : "", sizeof(client->device_sn));
    strlcpy(client->auth_token, config->auth_token != NULL ? config->auth_token : "", sizeof(client->auth_token));
    client->reconnect_timeout_ms = config->reconnect_timeout_ms > 0 ? config->reconnect_timeout_ms : CONFIG_AI_TOY_WS_RECONNECT_MS;
    client->event_cb = config->event_cb;
    client->event_ctx = config->event_ctx;

    *handle = client;
    return ESP_OK;
}

esp_err_t ai_toy_ws_client_deinit(ai_toy_ws_client_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ai_toy_ws_client_stop(handle);
    ai_toy_ws_clear_text(handle);
    free(handle);
    return ESP_OK;
}

esp_err_t ai_toy_ws_client_start(ai_toy_ws_client_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");

    handle->headers[0] = '\0';
    if (handle->auth_token[0] != '\0') {
        snprintf(handle->headers, sizeof(handle->headers), "Authorization: %s\r\n", handle->auth_token);
    }

    esp_websocket_client_config_t websocket_cfg = {
        .uri = handle->url,
        .headers = handle->headers[0] != '\0' ? handle->headers : NULL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .disable_auto_reconnect = false,
        .enable_close_reconnect = true,
        .reconnect_timeout_ms = handle->reconnect_timeout_ms,
    };

    if (handle->client != NULL) {
        ESP_RETURN_ON_ERROR(ai_toy_ws_client_stop(handle), TAG, "Failed to stop previous client");
    }

    handle->client = esp_websocket_client_init(&websocket_cfg);
    ESP_RETURN_ON_FALSE(handle->client != NULL, ESP_ERR_NO_MEM, TAG, "Failed to create WebSocket client");

    esp_err_t ret = esp_websocket_register_events(handle->client, WEBSOCKET_EVENT_ANY, ai_toy_ws_event, handle);
    if (ret != ESP_OK) {
        esp_websocket_client_destroy(handle->client);
        handle->client = NULL;
        return ret;
    }

    ret = esp_websocket_client_start(handle->client);
    if (ret != ESP_OK) {
        esp_websocket_client_destroy(handle->client);
        handle->client = NULL;
        return ret;
    }
    return ESP_OK;
}

esp_err_t ai_toy_ws_client_stop(ai_toy_ws_client_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    if (handle->client == NULL) {
        return ESP_OK;
    }

    esp_websocket_client_stop(handle->client);
    esp_websocket_client_destroy(handle->client);
    handle->client = NULL;
    handle->connected = false;
    ai_toy_ws_clear_text(handle);
    return ESP_OK;
}

esp_err_t ai_toy_ws_client_report_status(ai_toy_ws_client_handle_t handle, const ai_toy_ws_device_status_t *status)
{
    char *message = NULL;
    ESP_RETURN_ON_ERROR(ai_toy_ws_build_status_message(handle->device_sn, status, &message), TAG, "Failed to build status");
    esp_err_t ret = ai_toy_ws_send_text(handle, message);
    cJSON_free(message);
    return ret;
}

esp_err_t ai_toy_ws_client_audio_begin(ai_toy_ws_client_handle_t handle, const ai_toy_ws_audio_config_t *audio)
{
    char *message = NULL;
    ESP_RETURN_ON_ERROR(ai_toy_ws_build_audio_begin_message(handle->device_sn, audio, &message), TAG, "Failed to build audio begin");
    esp_err_t ret = ai_toy_ws_send_text(handle, message);
    cJSON_free(message);
    return ret;
}

esp_err_t ai_toy_ws_client_audio_send(ai_toy_ws_client_handle_t handle, const uint8_t *data, size_t len)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");
    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid data");
    ESP_RETURN_ON_FALSE(len > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid data length");
    ESP_RETURN_ON_FALSE(handle->client != NULL, ESP_ERR_INVALID_STATE, TAG, "WebSocket is not initialized");
    ESP_RETURN_ON_FALSE(handle->connected, ESP_ERR_INVALID_STATE, TAG, "WebSocket is not connected");

    int sent = esp_websocket_client_send_bin(handle->client, (const char *)data, len, portMAX_DELAY);
    if (sent < 0) {
        handle->connected = false;
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t ai_toy_ws_client_audio_end(ai_toy_ws_client_handle_t handle)
{
    char *message = NULL;
    ESP_RETURN_ON_ERROR(ai_toy_ws_build_audio_end_message(handle->device_sn, &message), TAG, "Failed to build audio end");
    esp_err_t ret = ai_toy_ws_send_text(handle, message);
    cJSON_free(message);
    return ret;
}

bool ai_toy_ws_client_is_connected(ai_toy_ws_client_handle_t handle)
{
    return handle != NULL && handle->connected;
}
