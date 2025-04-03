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

