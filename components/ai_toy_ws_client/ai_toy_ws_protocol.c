/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_check.h"
#include "esp_log.h"

#include "ai_toy_ws_protocol.h"

static const char *TAG = "AI_TOY_WS_PROTO";

static esp_err_t ai_toy_ws_print_json(cJSON *root, char **out)
{
    ESP_RETURN_ON_FALSE(root != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid JSON root");
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid output");

    char *text = cJSON_PrintUnformatted(root);
    ESP_RETURN_ON_FALSE(text != NULL, ESP_ERR_NO_MEM, TAG, "Failed to print JSON");
    *out = text;
    return ESP_OK;
}

static cJSON *ai_toy_ws_create_base_message(const char *device_sn, const char *msg_id)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }
    if (!cJSON_AddStringToObject(root, "deviceSn", device_sn != NULL ? device_sn : "") ||
            !cJSON_AddStringToObject(root, "msgId", msg_id)) {
        cJSON_Delete(root);
        return NULL;
    }
    return root;
}

esp_err_t ai_toy_ws_build_status_message(const char *device_sn, const ai_toy_ws_device_status_t *status, char **out)
{
    ESP_RETURN_ON_FALSE(status != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid status");
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid output");

    cJSON *root = ai_toy_ws_create_base_message(device_sn, AI_TOY_MSG_CLIENT_STATUS);
    ESP_RETURN_ON_FALSE(root != NULL, ESP_ERR_NO_MEM, TAG, "Failed to create status message");

    cJSON *data = cJSON_CreateObject();
    if (data == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddNumberToObject(data, "electricCharge", status->electric_charge);
    cJSON_AddNumberToObject(data, "volume", status->volume);
    cJSON_AddStringToObject(data, "version", status->version != NULL ? status->version : "");
    cJSON_AddBoolToObject(data, "isUpdate", status->is_update);
    cJSON_AddStringToObject(data, "updateVersion", status->update_version != NULL ? status->update_version : "");
    cJSON_AddNumberToObject(data, "start", status->start);
    if (status->dialog_id != NULL) {
        cJSON_AddStringToObject(data, "dialogId", status->dialog_id);
    }

    esp_err_t ret = ai_toy_ws_print_json(root, out);
    cJSON_Delete(root);
    return ret;
}

esp_err_t ai_toy_ws_build_audio_begin_message(const char *device_sn, const ai_toy_ws_audio_config_t *audio, char **out)
{
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid output");

    cJSON *root = ai_toy_ws_create_base_message(device_sn, AI_TOY_MSG_CLIENT_AUDIO_BEGIN);
    ESP_RETURN_ON_FALSE(root != NULL, ESP_ERR_NO_MEM, TAG, "Failed to create audio begin message");

    if (audio != NULL) {
        cJSON *data = cJSON_CreateObject();
        if (data == NULL) {
            cJSON_Delete(root);
            return ESP_ERR_NO_MEM;
        }
        cJSON_AddItemToObject(root, "data", data);
        cJSON_AddStringToObject(data, "format", audio->format != NULL ? audio->format : "pcm");
        cJSON_AddNumberToObject(data, "sampleRate", audio->sample_rate);
        cJSON_AddNumberToObject(data, "sampleBits", audio->sample_bits);
        cJSON_AddNumberToObject(data, "channels", audio->channels);
    }

    esp_err_t ret = ai_toy_ws_print_json(root, out);
    cJSON_Delete(root);
    return ret;
}

esp_err_t ai_toy_ws_build_audio_end_message(const char *device_sn, char **out)
{
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid output");

    cJSON *root = ai_toy_ws_create_base_message(device_sn, AI_TOY_MSG_CLIENT_AUDIO_END);
    ESP_RETURN_ON_FALSE(root != NULL, ESP_ERR_NO_MEM, TAG, "Failed to create audio end message");

    esp_err_t ret = ai_toy_ws_print_json(root, out);
    cJSON_Delete(root);
    return ret;
}

static int ai_toy_ws_get_int(const cJSON *object, const char *name, int default_value)
{
    const cJSON *item = cJSON_GetObjectItem(object, name);
    return cJSON_IsNumber(item) ? item->valueint : default_value;
}

static const char *ai_toy_ws_get_string(const cJSON *object, const char *name)
{
    const cJSON *item = cJSON_GetObjectItem(object, name);
    return cJSON_IsString(item) ? item->valuestring : NULL;
}

esp_err_t ai_toy_ws_parse_server_message(const char *json, size_t len, ai_toy_ws_event_cb_t cb, void *ctx)
{
    ESP_RETURN_ON_FALSE(json != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid JSON");
    ESP_RETURN_ON_FALSE(cb != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid callback");

    cJSON *root = cJSON_ParseWithLength(json, len);
    ESP_RETURN_ON_FALSE(root != NULL, ESP_ERR_INVALID_RESPONSE, TAG, "Failed to parse server JSON");

    const char *msg_id = ai_toy_ws_get_string(root, "msgId");
    const cJSON *data = cJSON_GetObjectItem(root, "data");
    if (msg_id == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (strcmp(msg_id, AI_TOY_MSG_SERVER_ACK) == 0) {
        ai_toy_ws_ack_t ack = {
            .code = ai_toy_ws_get_int(data, "code", 0),
            .message = ai_toy_ws_get_string(data, "msg"),
        };
        cb(AI_TOY_WS_EVENT_ACK, &ack, ctx);
    } else if (strcmp(msg_id, AI_TOY_MSG_SERVER_OTA) == 0) {
        ai_toy_ws_ota_t ota = {
            .is_update = ai_toy_ws_get_int(data, "isUpdate", 0) != 0,
            .url = ai_toy_ws_get_string(data, "url"),
            .version = ai_toy_ws_get_string(data, "version"),
        };
        cb(AI_TOY_WS_EVENT_OTA, &ota, ctx);
    } else if (strcmp(msg_id, AI_TOY_MSG_SERVER_VOLUME) == 0) {
        ai_toy_ws_volume_t volume = {
            .volume = ai_toy_ws_get_int(data, "volume", 0),
        };
        cb(AI_TOY_WS_EVENT_SET_VOLUME, &volume, ctx);
    } else if (strcmp(msg_id, AI_TOY_MSG_SERVER_PLAY) == 0) {
        ai_toy_ws_play_control_t play = {
            .start = ai_toy_ws_get_int(data, "start", 0) != 0,
        };
        cb(AI_TOY_WS_EVENT_PLAY_CONTROL, &play, ctx);
    } else if (strcmp(msg_id, AI_TOY_MSG_SERVER_PROCESSING) == 0) {
        ai_toy_ws_type_command_t processing = {
            .type = ai_toy_ws_get_int(data, "type", 0),
        };
        cb(AI_TOY_WS_EVENT_PROCESSING, &processing, ctx);
    } else if (strcmp(msg_id, AI_TOY_MSG_SERVER_UPGRADE) == 0) {
        ai_toy_ws_type_command_t upgrade = {
            .type = ai_toy_ws_get_int(data, "type", 1),
        };
        cb(AI_TOY_WS_EVENT_UPGRADE_NOW, &upgrade, ctx);
    } else {
        ESP_LOGW(TAG, "Unknown server msgId: %s", msg_id);
    }

    cJSON_Delete(root);
    return ESP_OK;
}
