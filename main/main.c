#include "esp_err.h"
#include "esp_log.h"

#include <nvs.h>
#include <nvs_flash.h>

#include "protocol_examples_common.h"
#include "esp_xiaozhi_chat_app.h"

esp_err_t application_start(void);

static const char *TAG = "main";

/*
 * 固件入口：先准备持久化存储和系统网络栈，再连接网络并启动小智应用。
 * 这里不负责具体的音频、显示和聊天协议初始化，这些工作由应用层统一完成。
 */
void app_main(void)
{
    // NVS 保存 Wi-Fi、设备标识等数据；版本变化或空间不足时需要擦除后重建。
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    // 初始化 TCP/IP 适配层和默认事件循环，供 Wi-Fi 与聊天协议共用。
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 使用 protocol_examples_common 完成配网并等待网络连接成功。
    ESP_ERROR_CHECK(example_connect());

    // 进入小智应用，后续初始化显示、音频处理和聊天客户端。
    esp_xiaozhi_chat_app();
}
