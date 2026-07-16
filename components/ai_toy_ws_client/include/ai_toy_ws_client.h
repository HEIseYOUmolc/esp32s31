/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ai_toy_ws_client_s *ai_toy_ws_client_handle_t;

typedef enum {
    AI_TOY_WS_EVENT_CONNECTED,
    AI_TOY_WS_EVENT_DISCONNECTED,
    AI_TOY_WS_EVENT_ACK,
    AI_TOY_WS_EVENT_OTA,
    AI_TOY_WS_EVENT_SET_VOLUME,
    AI_TOY_WS_EVENT_PLAY_CONTROL,
    AI_TOY_WS_EVENT_PROCESSING,
    AI_TOY_WS_EVENT_UPGRADE_NOW,
    AI_TOY_WS_EVENT_AUDIO_DATA,
    AI_TOY_WS_EVENT_ERROR,
} ai_toy_ws_event_t;

typedef struct {
    int code;
    const char *message;
} ai_toy_ws_ack_t;

typedef struct {
    bool is_update;
    const char *url;
    const char *version;
} ai_toy_ws_ota_t;

typedef struct {
    int volume;
} ai_toy_ws_volume_t;

typedef struct {
    bool start;
} ai_toy_ws_play_control_t;

typedef struct {
    int type;
} ai_toy_ws_type_command_t;

typedef struct {
    const uint8_t *data;
    size_t len;
} ai_toy_ws_audio_data_t;

typedef struct {
    esp_err_t code;
    const char *message;
} ai_toy_ws_error_t;

typedef void (*ai_toy_ws_event_cb_t)(ai_toy_ws_event_t event, const void *event_data, void *ctx);

typedef struct {
    const char *url;
    const char *device_sn;
    const char *auth_token;
    int reconnect_timeout_ms;
    ai_toy_ws_event_cb_t event_cb;
    void *event_ctx;
} ai_toy_ws_client_config_t;

typedef struct {
    int electric_charge;
    int volume;
    const char *version;
    bool is_update;
    const char *update_version;
    int start;
    const char *dialog_id;
} ai_toy_ws_device_status_t;

typedef struct {
    const char *format;
    int sample_rate;
    int sample_bits;
    int channels;
} ai_toy_ws_audio_config_t;

esp_err_t ai_toy_ws_client_init(const ai_toy_ws_client_config_t *config, ai_toy_ws_client_handle_t *handle);
esp_err_t ai_toy_ws_client_deinit(ai_toy_ws_client_handle_t handle);
esp_err_t ai_toy_ws_client_start(ai_toy_ws_client_handle_t handle);
esp_err_t ai_toy_ws_client_stop(ai_toy_ws_client_handle_t handle);
esp_err_t ai_toy_ws_client_report_status(ai_toy_ws_client_handle_t handle, const ai_toy_ws_device_status_t *status);
esp_err_t ai_toy_ws_client_audio_begin(ai_toy_ws_client_handle_t handle, const ai_toy_ws_audio_config_t *audio);
esp_err_t ai_toy_ws_client_audio_send(ai_toy_ws_client_handle_t handle, const uint8_t *data, size_t len);
esp_err_t ai_toy_ws_client_audio_end(ai_toy_ws_client_handle_t handle);
bool ai_toy_ws_client_is_connected(ai_toy_ws_client_handle_t handle);

#ifdef __cplusplus
}
#endif
