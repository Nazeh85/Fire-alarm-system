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
// Skriver en hel byte till sensorn, bit för bit
static void ds_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        ds_write_bit(byte & (1 << i)); // Skicka varje bit individuellt (LSB först)
    }
}
// Läser en hel byte från sensorn, bit för bit
static uint8_t ds_read_byte() {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        if (ds_read_bit()) {
            byte |= (1 << i);
        }
    }
    return byte;
}
// Initierar sensorn med angiven GPIO
void ds18b20_init(gpio_num_t gpio) {
    ds_pin = gpio;
    gpio_reset_pin(ds_pin);   // Säkerställ att pinnen är i grundläge
    gpio_set_pull_mode(ds_pin, GPIO_PULLUP_ONLY); // DS18B20 kräver pull-up motstånd
}
// Huvudfunktion för att läsa temperatur från sensor
float ds18b20_get_temp() {
    if (!ds_reset()) { 
        PRINTFC_DS18B20("Ingen sensor hittad"); // Sensor svarade inte
        return -100.0f; // Returnera felvärde
    }

    ds_write_byte(0xCC);  // Skip ROM – adressera alla sensorer
    ds_write_byte(0x44);  // Starta temperaturkonvertering
    vTaskDelay(pdMS_TO_TICKS(750));  // Vänta på konvertering (~750 ms)

    if (!ds_reset()) {
        PRINTFC_DS18B20("Ingen sensor efter konvertering");
        return -100.0f;
    }

    ds_write_byte(0xCC);  // Skip ROM igen
    ds_write_byte(0xBE);  // Läs scratchpad (där temperaturen lagras)

    uint8_t temp_lsb = ds_read_byte(); // Lägsta byte av temperaturen
    uint8_t temp_msb = ds_read_byte(); // Högsta byte
    int16_t temp_raw = (temp_msb << 8) | temp_lsb; // Kombinera till 16-bit värde

    return (float)temp_raw / 16.0; // Omvandla till grader Celsius
}
