#include "ds18b20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "color.h"


// Global variabel för vald GPIO
static gpio_num_t ds_pin;
// Fördröjning i mikrosekunder, används för att styra timing enligt DS18B20-protokollet
static void ds18b20_delay_us(int us) {
    esp_rom_delay_us(us);  
}
