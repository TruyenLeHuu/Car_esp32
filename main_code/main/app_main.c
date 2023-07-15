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

#include "runStats.h"

#include "sys_config.h"

static const char *TAG = "main";

esp_mqtt_client_config_t mqtt_cfg_t = {
            .host = MQTT_ADDRESS,
            .port = MQTT_PORT,
            .lwt_topic = DISCONNECT_TOPIC_PUB,
            .lwt_msg = "Esp32",
            .lwt_msg_len = 0,
            .lwt_qos = 1,
            .lwt_retain = 0
};
MQTT_Handler_Struct mqtt_h =
{
    .mqtt_cfg = &mqtt_cfg_t,
};
Twai_Handler_Struct twai_h =
{
    .f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(),
    .t_config = TWAI_TIMING_CONFIG_500KBITS(),
    .g_config = {   .mode = TWAI_MODE_NORMAL,
                    .tx_io = TX_GPIO_NUM, .rx_io = RX_GPIO_NUM,
                    .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED,      
                    .tx_queue_len = 5, .rx_queue_len = 5,                           
                    .alerts_enabled = TWAI_ALERT_NONE,  .clkout_divider = 0,        
                    .intr_flags = ESP_INTR_FLAG_LEVEL1}
};

//Taking millis function
unsigned long IRAM_ATTR millis(){return (unsigned long) (esp_timer_get_time() / 1000ULL);}

void app_main(void)
{   
    ESP_LOGI(TAG, "[APP] Startup..");
    // Nvs init
    ESP_ERROR_CHECK(nvs_flash_init());
    // Gpio setup
    gpio_pad_select_gpio(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(BUTTON_PIN_1);
    gpio_set_direction(BUTTON_PIN_1, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(BUTTON_PIN_2);
    gpio_set_direction(BUTTON_PIN_2, GPIO_MODE_INPUT);
    // Wifi start
    wifi_init();
    // Mqtt start
    mqtt_init_start(&mqtt_h);
    // Twai start
    twai_install_start(&twai_h);
    // Tasks
    xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, &mqtt_h, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(twai_transmit_task, "TWAI_tx", 4096, &twai_h, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(mqtt_receive_task, "MQTT_tx", 4096, NULL, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
    // xTaskCreatePinnedToCore(stats_task, "RunStats", 4096, NULL, 5, NULL, tskNO_AFFINITY);
}
