/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_mn_speech_commands.h"
#include "sdkconfig.h"
#include "xiaozhi_custom_wake_word.h"

static const char *TAG = "xiaozhi_custom_wake_word";

bool xiaozhi_custom_wake_word_is_enabled(void)
{
#if CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_ENABLE
    return true;
#else
    return false;
#endif
}

esp_err_t xiaozhi_custom_wake_word_configure_afe(av_processor_afe_config_t *afe_config)
{
    ESP_RETURN_ON_FALSE(afe_config != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid AFE config");

#if CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_ENABLE
    afe_config->mode.afe.algo_mask |= AV_PROCESSOR_AFE_FLAG_VCMD_DETECT_ENABLE;
    afe_config->mode.afe.mn_language = "cn";
    afe_config->mode.afe.vcmd_timeout_ms = CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_VCMD_TIMEOUT_MS;
    ESP_LOGI(TAG, "Custom wake word enabled: command=\"%s\" display=\"%s\" id=%d",
             CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_COMMAND,
             CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_DISPLAY,
             CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_COMMAND_ID);
#endif

    return ESP_OK;
}

esp_err_t xiaozhi_custom_wake_word_register_command(void)
{
#if CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_ENABLE
    esp_err_t ret = esp_mn_commands_add(CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_COMMAND_ID,
                                        CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_COMMAND);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to add MultiNet command \"%s\": %s",
                 CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_COMMAND, esp_err_to_name(ret));
        return ret;
    }

    esp_mn_error_t *mn_error = esp_mn_commands_update();
    if (mn_error != NULL && mn_error->num > 0) {
        ESP_LOGW(TAG, "MultiNet rejected %d command phrase(s)", mn_error->num);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Registered custom wake word command: %s",
             CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_COMMAND);
#endif
    return ESP_OK;
}

esp_err_t xiaozhi_custom_wake_word_start_detection(audio_recorder_handle_t recorder)
{
#if CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_ENABLE
    ESP_RETURN_ON_FALSE(recorder != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid recorder");

    esp_gmf_element_handle_t afe_element = NULL;
    esp_err_t ret = audio_recorder_get_afe_element_handle(recorder, &afe_element);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to get AFE element");
    ESP_RETURN_ON_FALSE(afe_element != NULL, ESP_ERR_INVALID_STATE, TAG, "AFE element is null");

    esp_gmf_err_t gmf_ret = esp_gmf_afe_vcmd_detection_begin(afe_element);
    if (gmf_ret != ESP_GMF_ERR_OK) {
        ESP_LOGW(TAG, "Failed to start voice command detection: %d", (int)gmf_ret);
        return ESP_FAIL;
    }
#endif
    return ESP_OK;
}

bool xiaozhi_custom_wake_word_match(const esp_gmf_afe_vcmd_info_t *vcmd_info)
{
#if CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_ENABLE
    if (vcmd_info == NULL) {
        return false;
    }

    float min_prob = (float)CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_MIN_PROB_PERCENT / 100.0f;
    if (min_prob > 0.0f && vcmd_info->prob < min_prob) {
        ESP_LOGI(TAG, "Ignore command id=%d prob=%.3f below %.3f",
                 vcmd_info->phrase_id, vcmd_info->prob, min_prob);
        return false;
    }

    if (vcmd_info->phrase_id == CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_COMMAND_ID) {
        return true;
    }

    if (strcmp(vcmd_info->str, CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_COMMAND) == 0 ||
        strcmp(vcmd_info->str, CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_DISPLAY) == 0) {
        return true;
    }
#endif
    return false;
}

const char *xiaozhi_custom_wake_word_get_display_text(void)
{
#if CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_ENABLE
    return CONFIG_XIAOZHI_CUSTOM_WAKE_WORD_DISPLAY;
#else
    return "你好小智";
#endif
}
