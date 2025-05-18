#ifndef MQTT_H
#define MQTT_H

#include "esp_err.h"
// Starts the MQTT client
esp_err_t mqtt_start();
// Publishes a message to a given topic
void mqtt_publish(const char *topic, const char *payload);

#endif
