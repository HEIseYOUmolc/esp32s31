#include "esp_err.h"
#include "esp_log.h"

#include <nvs.h>
#include <nvs_flash.h>

#include <esp_event.h>
#include "protocol_examples_common.h"
#include "esp_xiaozhi_chat_app.h"

esp_err_t application_start(void);

static const char *TAG = "main";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    esp_xiaozhi_chat_app();
}
