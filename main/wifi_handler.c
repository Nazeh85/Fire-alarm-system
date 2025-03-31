#include "wifi_handler.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "color.h"

static int reconnect_counter = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    wifi_init_param_t *param = (wifi_init_param_t *)arg;
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        PRINTFC_WIFI_HANDLER("WIFI started now connecting");
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_CONNECTED:
        PRINTFC_WIFI_HANDLER("WIFI Connected");
        xEventGroupSetBits(param->wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        PRINTFC_WIFI_HANDLER("WIFI Disconnected");
        xEventGroupClearBits(param->wifi_event_group, WIFI_CONNECTED_BIT);
        if (reconnect_counter < WIFI_RECONNECT_MAX_ATTEMPT){
            reconnect_counter++;
            esp_wifi_connect();
        }
        break;
    
    default:
        break;
    }
}
