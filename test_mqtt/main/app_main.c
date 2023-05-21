#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqtt_connect.h"
#include "wifi_connect.h"
/**
* Json
*/
#include "cJSON.h"
/**
 * System Config;
 */
#include "sys_config.h"

//taking millis function
unsigned long IRAM_ATTR millis(){return (unsigned long) (esp_timer_get_time() / 1000ULL);}

static const char *TAG = "main";
void task_mqtt_trans ( void *pvParameter )
{
    for (;;)
    {
        cJSON *root;
        root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "Timestamp", millis());
        cJSON_AddStringToObject(root, "Data", "Data from Car Hardware");
        char *data = cJSON_Print(root);
        mqtt_client_publish(TEST_TOPIC_PUB, data);
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
    }
}
void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    // nvs init
    ESP_ERROR_CHECK(nvs_flash_init());
    // wifi connect
    wifi_init_sta();
    // mqtt connect
    mqtt_app_start();
    // mqtt task
    if( xTaskCreate( task_mqtt_trans, "task_mqtt_transmit", 1024 * 2, NULL, 2, NULL) != pdPASS )
    {
        #if DEBUG
        ESP_LOGI( TAG, "ERROR - sensor_task NOT ALLOCATED :/\r\n" );  
        #endif
        return;   
    }    
}
