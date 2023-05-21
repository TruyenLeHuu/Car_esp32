#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "mqtt_connect.h"
#include "wifi_connect.h"
#include "twai_connect.h"

#include "cJSON.h"

#include "sys_config.h"

static const char *TAG = "main";

Twai_Handler_Struct Twai_s =
{
    .f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(),
    .t_config = TWAI_TIMING_CONFIG_500KBITS(),
    .g_config = {   .mode = TWAI_MODE_NORMAL,
                    .tx_io = TX_GPIO_NUM, .rx_io = RX_GPIO_NUM,
                    .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED,
                    .tx_queue_len = 10, .rx_queue_len = 10,
                    .alerts_enabled = TWAI_ALERT_NONE,
                    .clkout_divider = 0}
};

static QueueHandle_t tx_mqtt_task_queue;

typedef enum {
    RX_RECEIVE_PING,
    RX_RECEIVE_START_CMD,
    RX_RECEIVE_STOP_CMD,
    RX_TASK_EXIT,
} tx_mqtt_task_action_t;

//taking millis function
unsigned long IRAM_ATTR millis(){return (unsigned long) (esp_timer_get_time() / 1000ULL);}

static void mqtt_transmit_task(void *arg)
{
     while (1) {
        tx_mqtt_task_action_t action;
        xQueueReceive(tx_mqtt_task_queue, &action, portMAX_DELAY);
        if (action == RX_RECEIVE_PING) {
            
        } else if (action == RX_TASK_EXIT) {
            break;
        }
    }
    vTaskDelete(NULL);
}

static void mqtt_receive_task(void *arg)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
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
    // twai
    twai_install_start(&Twai_s);

    tx_mqtt_task_queue = xQueueCreate(1, sizeof(tx_mqtt_task_action_t));
    // Tasks
    xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(twai_transmit_task, "TWAI_tx", 4096, &Twai_s, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(mqtt_receive_task, "MQTT_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(mqtt_transmit_task, "MQTT_tx", 4096, NULL, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
}
