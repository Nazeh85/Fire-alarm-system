#include "mqtt.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>
#include "color.h"

// MQTT-klienthandtag, används för att hålla referens till klienten
static esp_mqtt_client_handle_t mqtt_client = NULL;

// Händelsehanterare för MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            // När anslutning till MQTT-broker är etablerad
            PRINTFC_MQTT("MQTT connected!");
            esp_mqtt_client_subscribe(mqtt_client, "brandlarm/command", 0); // Prenumerera på ämnet "brandlarm/command"
            break;

        case MQTT_EVENT_DISCONNECTED:
            // När anslutningen till MQTT-brokern bryts
            PRINTFC_MQTT("MQTT disconnected.");
            break;

        case MQTT_EVENT_DATA:
            // När data tas emot från MQTT
            PRINTFC_MQTT("Received data:");
            PRINTFC_MQTT("Topic: %.*s\n", event->topic_len, event->topic); // Skriv ut ämnet
            PRINTFC_MQTT("Data: %.*s\n", event->data_len, event->data);   // Skriv ut innehållet
            break;

        default:
            // För alla andra händelser
            PRINTFC_MQTT("Other MQTT event: %d", event->event_id);
            break;
    }
}

// Funktion för att starta MQTT-klienten
esp_err_t mqtt_start() {
    // MQTT-konfiguration med brokerns URI
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://192.168.0.30:1883",  // IP och port till MQTT-broker
        .network.disable_auto_reconnect = false           // Tillåt automatisk återanslutning
    };

    // Initiera MQTT-klienten
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client) {
        PRINTFC_MQTT("Failed to init MQTT client"); // Fel vid initiering
        return ESP_FAIL;
    }

    // Registrera händelsehanteraren
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    // Starta MQTT-klienten
    esp_err_t err = esp_mqtt_client_start(mqtt_client);
    if (err != ESP_OK) {
        PRINTFC_MQTT("Failed to start MQTT client"); // Fel vid start
    }

    return err;
}

// Funktion för att publicera ett meddelande till ett visst ämne
void mqtt_publish(const char *topic, const char *payload) {
    if (!mqtt_client) {
        PRINTFC_MQTT("MQTT client not initialized"); // Om klienten inte är initierad
        return;
    }

    // Publicera meddelandet
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
    if (msg_id < 0) {
        PRINTFC_MQTT("Failed to publish to topic: %s", topic); // Fel vid publicering
    } else {
        PRINTFC_MQTT("Published to %s: %s", topic, payload);   // Lyckad publicering
    }
}
