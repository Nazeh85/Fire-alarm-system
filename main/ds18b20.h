#ifndef DS18B20_H
#define DS18B20_H

#include "driver/gpio.h" // Inkluderar stöd för GPIO-konfigurationer
// Initierar DS18B20-sensorn på angiven GPIO-pin.
// Måste anropas innan temperaturen kan läsas.
void ds18b20_init(gpio_num_t gpio);
// Läser aktuell temperatur från DS18B20-sensorn.
// Returnerar temperatur i grader Celsius (float).
// Om ingen sensor hittas returneras -100.0 som felvärde.
float ds18b20_get_temp(void);

#endif // DS18B20_H
