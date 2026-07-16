/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "ai_toy_ws_client.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AI_TOY_MSG_CLIENT_STATUS       "10001"
#define AI_TOY_MSG_CLIENT_AUDIO_BEGIN  "10003"
#define AI_TOY_MSG_CLIENT_AUDIO_END    "10004"

#define AI_TOY_MSG_SERVER_ACK          "20001"
#define AI_TOY_MSG_SERVER_OTA          "20002"
#define AI_TOY_MSG_SERVER_VOLUME       "20003"
#define AI_TOY_MSG_SERVER_PLAY         "20004"
#define AI_TOY_MSG_SERVER_PROCESSING   "20005"
#define AI_TOY_MSG_SERVER_UPGRADE      "20006"

esp_err_t ai_toy_ws_build_status_message(const char *device_sn, const ai_toy_ws_device_status_t *status, char **out);
esp_err_t ai_toy_ws_build_audio_begin_message(const char *device_sn, const ai_toy_ws_audio_config_t *audio, char **out);
esp_err_t ai_toy_ws_build_audio_end_message(const char *device_sn, char **out);
esp_err_t ai_toy_ws_parse_server_message(const char *json, size_t len, ai_toy_ws_event_cb_t cb, void *ctx);

#ifdef __cplusplus
}
#endif
