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
#include "ds18b20.h"
#include "driver/ledc.h"

#define DHT_GPIO 18
#define DHT_TYPE DHT_TYPE_DHT22

#define RED_LED_GPIO 25
#define GREEN_LED_GPIO 26
#define BUZZER_GPIO 27
#define DS18B20_GPIO 19
#define BUTTON_GPIO 4
#define POTENTIOMETER_ADC_CHANNEL ADC_CHANNEL_6 // GPIO34

#define BUZZER_PWM_FREQ 3000
#define BUZZER_DUTY 512
#define ALARM_COOLDOWN_MS 15000
// Globala variabler
EventGroupHandle_t wifi_event_group;
wifi_init_param_t w_param = {
    .ssid = CONFIG_WIFI_SSID,
    .password = CONFIG_WIFI_PASSWORD,
};

static char system_status[5] = "OK";
static bool alarm_active = false;

static void configure_gpio()
{
    gpio_reset_pin(RED_LED_GPIO);
    gpio_set_direction(RED_LED_GPIO, GPIO_MODE_OUTPUT);
    

    gpio_reset_pin(GREEN_LED_GPIO);
    gpio_set_direction(GREEN_LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(POTENTIOMETER_ADC_CHANNEL, ADC_ATTEN_DB_11);

    ds18b20_init(DS18B20_GPIO);
}
static void configure_buzzer_pwm() {
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = BUZZER_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = BUZZER_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0,
    };
    ledc_channel_config(&channel);
}

static void buzzer_on() {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, BUZZER_DUTY);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void buzzer_off() {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static float get_poti_temp() {
    int raw = adc1_get_raw(POTENTIOMETER_ADC_CHANNEL);
    PRINTFC_MAIN("Poti råvärde: %d", raw);
    if (raw < ADC_THRESHOLD_MIN) return -1.0f;
    return TEMP_MIN + ((float)raw / 4095.0f) * (TEMP_MAX - TEMP_MIN);
}

static float get_ds18b20_temp() {
    float temperature = ds18b20_get_temp();
    PRINTFC_DS18B20I("DS18B20 Temperatur: %.2f C", temperature);
    return temperature;
}

static void fire_alarm(const char *reason) {
    static TickType_t last_alarm = 0;
    if (xTaskGetTickCount() - last_alarm < pdMS_TO_TICKS(ALARM_COOLDOWN_MS)) return;
    last_alarm = xTaskGetTickCount();

    PRINTFC_DHT("LARM AKTIVERAT: %s", reason);
    strcpy(system_status, "LARM");
    alarm_active = true;

    for (int i = 0; i < 50; i++) {
        if (gpio_get_level(BUTTON_GPIO) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BUTTON_GPIO) == 0) {
                PRINTFC_DHT("Larm stoppat manuellt!");
                alarm_active = false;
                strcpy(system_status, "OK");
                break;
            }
        }
        buzzer_on();
        gpio_set_level(RED_LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        buzzer_off();
        gpio_set_level(RED_LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    buzzer_off();
    gpio_set_level(RED_LED_GPIO, 0);
}

static void init_peripherals() {
    configure_gpio();
    configure_buzzer_pwm();
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