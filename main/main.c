#include <stdio.h>
#include "string.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "wifi_handler.h"
#include "nvs_flash.h"
#include "color.h"
#include "esp_system.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "dht.h"

#define DHT_GPIO 18
#define DHT_TYPE DHT_TYPE_DHT22

#define RED_LED_GPIO 25
#define GREEN_LED_GPIO 26
#define BUZZER_GPIO 27

// Globala variabler
EventGroupHandle_t wifi_event_group;
wifi_init_param_t w_param = {
    .ssid = CONFIG_WIFI_SSID,
    .password = CONFIG_WIFI_PASSWORD,
};

static void configure_gpio()
{
    gpio_reset_pin(RED_LED_GPIO);
    gpio_set_direction(RED_LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_reset_pin(GREEN_LED_GPIO);
    gpio_set_direction(GREEN_LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_reset_pin(BUZZER_GPIO);
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
}

static esp_err_t initialize_wifi() {
    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL) {
        PRINTFC_WIFI_HANDLER("Failed to create Wi-Fi event group");
        return ESP_FAIL;
    }
    w_param.wifi_event_group = wifi_event_group;
    wifi_handler_start(&w_param);
    return ESP_OK;
}

void app_main(void)
{
    PRINTFC_MAIN("Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    PRINTFC_WIFI_HANDLER("Wi-Fi Initialization");
    if (initialize_wifi() != ESP_OK) {
        return;
    }

    PRINTFC_WIFI_HANDLER("Waiting for Wi-Fi connection...");
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group, WIFI_CONNECTED_BIT | WIFI_HAS_IP_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(30000));
    if (!(bits & WIFI_HAS_IP_BIT)) {
        PRINTFC_WIFI_HANDLER("Wi-Fi connection timeout. Exiting.");
        return;
    }
    PRINTFC_WIFI_HANDLER("Wi-Fi connected and IP acquired.");
    configure_gpio();

    while (1)
    {
        float temperature = 0;
        float humidity = 0;
        esp_err_t result;
        int retries = 3;

        float temp_threshold = 25.0f;

        while (retries-- > 0)
        {
            result = dht_read_float_data(DHT_TYPE, DHT_GPIO, &humidity, &temperature);
            if (result == ESP_OK)
                break;
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (result == ESP_OK)
        {
            PRINTFC_DHT("Temperatur: %.1f°C | Luftfuktighet: %.1f%% | Tröskel: %.1f°C", temperature, humidity, temp_threshold);

            if (temperature >= temp_threshold)
            {
                gpio_set_level(RED_LED_GPIO, 1);
                gpio_set_level(BUZZER_GPIO, 1);
                gpio_set_level(GREEN_LED_GPIO, 0);

                PRINTFC_DHT("LARM TRIGGAT! Temperatur: %.1f°C >= Tröskel: %.1f°C", temperature, temp_threshold);
            }
            else
            {
                gpio_set_level(RED_LED_GPIO, 0);
                gpio_set_level(BUZZER_GPIO, 0);

                gpio_set_level(GREEN_LED_GPIO, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level(GREEN_LED_GPIO, 0);
                vTaskDelay(pdMS_TO_TICKS(500));

                PRINTFC_DHT("Temperatur OK (%.1f°C < %.1f°C) – Larm AV", temperature, temp_threshold);
            }
        }
        else
        {
            PRINTFC_DHT("Sensor read failed after retries: %s", esp_err_to_name(result));
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
        
    }