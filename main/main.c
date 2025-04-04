#include <stdio.h>
#include "string.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "wifi_handler.h"
#include "nvs_flash.h"
#include "color.h"

// Globala variabler
EventGroupHandle_t wifi_event_group;
wifi_init_param_t w_param = {
    .ssid = CONFIG_WIFI_SSID,
    .password = CONFIG_WIFI_PASSWORD,
};

static esp_err_t initialize_wifi() {
    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL) {
        PRINTFC_MAIN("Failed to create Wi-Fi event group");
        return ESP_FAIL;
    }
    w_param.wifi_event_group = wifi_event_group;
    wifi_handler_start(&w_param);
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI("Main", "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    PRINTFC_MAIN("Wi-Fi Initialization");
    if (initialize_wifi() != ESP_OK) {
        return;
    }

    PRINTFC_WIFI_HANDLER("Waiting for Wi-Fi connection...");
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group, WIFI_CONNECTED_BIT | WIFI_HAS_IP_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(30000));
    if (!(bits & WIFI_HAS_IP_BIT)) {
        PRINTFC_MAIN("Wi-Fi connection timeout. Exiting.");
        return;
    }
    PRINTFC_MAIN("Wi-Fi connected and IP acquired.");
    
}