#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

#include "dht.h"

#define DHT_DATA_BITS 40
static const char *TAG = "dht";

#define WAIT_FOR_LEVEL(pin, level, timeout_us)                     \
    ({                                                             \
        int64_t _start = esp_timer_get_time();                     \
        while (gpio_get_level(pin) == (level)) {                   \
            if (esp_timer_get_time() - _start > (timeout_us))      \
                return ESP_ERR_TIMEOUT;                            \
        }                                                          \
    })

esp_err_t dht_read_float_data(dht_type_t sensor_type, gpio_num_t pin, float *humidity, float *temperature)
{
    int data[DHT_DATA_BITS] = {0};
    uint8_t dht_data[5] = {0};

    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);

    if (sensor_type == DHT_TYPE_DHT11) {
        vTaskDelay(pdMS_TO_TICKS(3000));
    } else {
        esp_rom_delay_us(1000);
    }

    gpio_set_level(pin, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    WAIT_FOR_LEVEL(pin, 1, 80);
    WAIT_FOR_LEVEL(pin, 0, 80);
    WAIT_FOR_LEVEL(pin, 1, 80);

    for (int i = 0; i < DHT_DATA_BITS; i++) {
        WAIT_FOR_LEVEL(pin, 0, 80);

        int64_t start = esp_timer_get_time();
        WAIT_FOR_LEVEL(pin, 1, 100);

        int64_t pulse_time = esp_timer_get_time() - start;
        data[i] = (pulse_time > 40) ? 1 : 0;
    }

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 8; j++) {
            dht_data[i] <<= 1;
            dht_data[i] |= data[i * 8 + j];
        }
    }

    if (((dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) & 0xFF) != dht_data[4]) {
        return ESP_ERR_INVALID_CRC;
    }

    if (sensor_type == DHT_TYPE_DHT11) {
        if (humidity) *humidity = dht_data[0];
        if (temperature) *temperature = dht_data[2];
    } else {
        if (humidity)
            *humidity = ((dht_data[0] << 8) + dht_data[1]) / 10.0;
        if (temperature) {
            *temperature = (((dht_data[2] & 0x7F) << 8) + dht_data[3]) / 10.0;
            if (dht_data[2] & 0x80) {
                *temperature = -*temperature;
            }
        }
    }

    return ESP_OK;
}
