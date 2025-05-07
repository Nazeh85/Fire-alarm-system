#ifndef __DHT_H__
#define __DHT_H__

#include "esp_err.h"
#include "driver/gpio.h"

// Typ av DHT sensor (DHT11 eller DHT22)
typedef enum {
    DHT_TYPE_DHT11 = 0,
    DHT_TYPE_DHT22
} dht_type_t;


esp_err_t dht_read_float_data(dht_type_t sensor_type, gpio_num_t pin, float *humidity, float *temperature);


#endif 
