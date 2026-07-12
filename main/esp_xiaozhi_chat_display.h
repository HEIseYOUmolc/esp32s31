/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief 使用 Board Manager 提供的 LCD 句柄初始化显示界面
 *
 * @note 调用前必须先完成板级 LCD 初始化。当前显示模块会自行创建 LVGL display。
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE LCD 不可用。
 */
esp_err_t esp_xiaozhi_chat_display_init(void);

/**
 * @brief 获取显示宽度
 * @param[out] width 返回宽度（像素）
 * @return ESP_OK 成功；ESP_ERR_INVALID_ARG 参数为空。
 */
esp_err_t esp_xiaozhi_chat_display_get_width(int *width);

/**
 * @brief 获取显示高度
 * @param[out] height 返回高度（像素）
 * @return ESP_OK 成功；ESP_ERR_INVALID_ARG 参数为空。
 */
esp_err_t esp_xiaozhi_chat_display_get_height(int *height);

/**
 * @brief 更新状态栏文字
 * @param[in] status UTF-8 状态文本
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE 显示未初始化。
 */
esp_err_t esp_xiaozhi_chat_display_set_status(const char *status);

/**
 * @brief 更新副标题；当前界面将其显示在状态栏位置
 * @param[in] subtitle UTF-8 副标题文本
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE 显示未初始化。
 */
esp_err_t esp_xiaozhi_chat_display_set_subtitle(const char *subtitle);

/**
 * @brief 更新音量；当前实现同时用该值调整背光
 * @param[in] volume 音量百分比，范围 0～100
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE 显示未初始化。
 */
esp_err_t esp_xiaozhi_chat_display_set_volume(int volume);

/**
 * @brief 设置 LCD 背光亮度
 * @param[in] brightness 亮度百分比，范围 0～100
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE 显示未初始化。
 */
esp_err_t esp_xiaozhi_chat_display_set_brightness(int brightness);

/**
 * @brief 临时显示通知，超时后恢复状态文字
 * @param[in] notification UTF-8 通知文本
 * @param[in] duration_ms 显示时间（毫秒），非正数时使用默认值 3000
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE 显示未初始化。
 */
esp_err_t esp_xiaozhi_chat_display_set_notification(const char *notification, int duration_ms);

/**
 * @brief 根据情绪名称切换表情图标
 * @param[in] emotion 情绪名称，例如 happy、sad、neutral
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE 显示未初始化。
 */
esp_err_t esp_xiaozhi_chat_display_set_emotion(const char *emotion);

/**
 * @brief 更新聊天消息
 * @param[in] role 消息角色：user、assistant 或 system
 * @param[in] content UTF-8 消息内容
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE 显示未初始化。
 */
esp_err_t esp_xiaozhi_chat_display_set_chat_message(const char *role, const char *content);

/**
 * @brief 直接设置主区域图标
 * @param[in] icon 图标字体对应的 UTF-8 字符串
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE 显示未初始化。
 */
esp_err_t esp_xiaozhi_chat_display_set_icon(const char *icon);

/**
 * @brief 显示或隐藏预览图片
 * @param[in] image LVGL 图片描述符；传入 NULL 时隐藏图片并恢复表情
 * @return ESP_OK 成功；ESP_ERR_INVALID_STATE 显示未初始化。
 */
esp_err_t esp_xiaozhi_chat_display_set_image(const void *image);

/**
 * @brief 切换显示主题
 * @param[in] theme_name 主题名称：light 或 dark
 * @return ESP_OK 成功；ESP_ERR_INVALID_ARG 名称无效；ESP_ERR_INVALID_STATE 显示未初始化。
 */
esp_err_t esp_xiaozhi_chat_display_set_theme(const char *theme_name);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
