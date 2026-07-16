/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_xiaozhi_chat.h"
#include "esp_gmf_oal_thread.h"
#if CONFIG_XIAOZHI_CHAT_APP_SERVER_AI_TOY
#include "ai_toy_ws_client.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define ESP_XIAOZHI_CHAT_REC_READ_SIZE CONFIG_XIAOZHI_UDP_MAX_AUDIO_PAYLOAD_SIZE
#define ESP_XIAOZHI_CHAT_APP_ONLINE (1 << 1)
#define ESP_XIAOZHI_CHAT_APP_OFFLINE (1 << 2)

/**
 * @brief 小智聊天应用运行上下文
 *
 * 保存聊天句柄、音频格式、工作线程和事件同步对象。该结构由应用入口创建，
 * 并在聊天、录音和音频通道线程之间共享。
 */
typedef struct esp_xiaozhi_chat_app_s {
    bool                       wakeuped;          /*!< 已唤醒且允许上传录音数据 */
    bool                       tts_playing;       /*!< 正在播放服务端语音，暂停上传麦克风数据避免自回声 */
    int                        audio_send_errors;/*!< 连续临时发送失败次数 */
    EventGroupHandle_t         data_evt_group;   /*!< 控制音频通道开关的事件组 */
    esp_xiaozhi_chat_handle_t  chat;             /*!< 小智聊天客户端句柄 */
#if CONFIG_XIAOZHI_CHAT_APP_SERVER_AI_TOY
    ai_toy_ws_client_handle_t  ai_toy;           /*!< AI 玩具 WebSocket 客户端句柄 */
#endif
    esp_xiaozhi_chat_audio_t   audio;            /*!< 上传音频的编码和采样参数 */
    esp_gmf_oal_thread_t       read_thread;      /*!< 录音读取与上传线程 */
    esp_gmf_oal_thread_t       audio_channel;    /*!< 音频通道控制线程 */
} esp_xiaozhi_chat_app_t;

/**
 * @brief 启动小智聊天应用
 *
 * @return ESP_OK 启动成功；ESP_ERR_NO_MEM 内存分配失败；其他值为对应初始化错误。
 */
esp_err_t esp_xiaozhi_chat_app(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
