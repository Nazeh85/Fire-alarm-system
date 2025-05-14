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
// Initierar ett reset-puls till sensorn och läser närvaro från DS18B20
static int ds_reset() {
    gpio_set_direction(ds_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(ds_pin, 0); // Skicka reset-puls (low i 480 µs)
    ds18b20_delay_us(480);
    gpio_set_direction(ds_pin, GPIO_MODE_INPUT); // Släpp linan
    ds18b20_delay_us(70);
    int presence = gpio_get_level(ds_pin); // Sensor svarar genom att hålla linan låg
    ds18b20_delay_us(410);
    return presence == 0; // Returnera true om sensor svarar korrekt
}
// Skriver en bit (1 eller 0) till sensorn enligt timing-krav
static void ds_write_bit(int bit) {
    gpio_set_direction(ds_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(ds_pin, 0);
    ds18b20_delay_us(bit ? 6 : 60); // Kort puls för '1', lång för '0'
    gpio_set_level(ds_pin, 1);
    ds18b20_delay_us(bit ? 64 : 10);
}
// Läser en bit från sensorn
static int ds_read_bit() {
    int bit;
    gpio_set_direction(ds_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(ds_pin, 0);
    ds18b20_delay_us(6);
    gpio_set_direction(ds_pin, GPIO_MODE_INPUT); // Släpp linan
    ds18b20_delay_us(9);
    bit = gpio_get_level(ds_pin); // Läs bit från sensorn
    ds18b20_delay_us(55);
    return bit;
}
