/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "audio_processor.h"
#include "esp_gmf_afe.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure av_processor AFE for the custom wake word path.
 *
 * This enables ESP-SR MultiNet voice command detection while keeping the
 * existing AFE recorder pipeline.
 */
esp_err_t xiaozhi_custom_wake_word_configure_afe(av_processor_afe_config_t *afe_config);

/**
 * @brief Register the configured command with ESP-SR MultiNet.
 *
 * Call this after audio_recorder_open(), because GMF AFE initializes the
 * MultiNet command list during recorder startup.
 */
esp_err_t xiaozhi_custom_wake_word_register_command(void);

/**
 * @brief Begin or restart voice command detection on the recorder AFE element.
 */
esp_err_t xiaozhi_custom_wake_word_start_detection(audio_recorder_handle_t recorder);

/**
 * @brief Return true when a GMF AFE voice command event matches the wake word.
 */
bool xiaozhi_custom_wake_word_match(const esp_gmf_afe_vcmd_info_t *vcmd_info);

/**
 * @brief Text reported to the chat server as the wake word.
 */
const char *xiaozhi_custom_wake_word_get_display_text(void);

/**
 * @brief Return whether the custom wake word path is enabled.
 */
bool xiaozhi_custom_wake_word_is_enabled(void);

#ifdef __cplusplus
}
#endif
