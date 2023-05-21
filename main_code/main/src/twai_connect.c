#include "twai_connect.h"
#include "esp_log.h"
static const char *TAG = "Twai connection";

void twai_install_start(Twai_Handler_Struct* Twai_s)
{
    ESP_ERROR_CHECK(twai_driver_install(&Twai_s->g_config, &Twai_s->t_config, &Twai_s->f_config));
    ESP_LOGI(TAG, "TWAI Driver installed");

    ESP_ERROR_CHECK(twai_start());

    Twai_s->tx_task_queue = xQueueCreate(1, sizeof(Twai_s->tx_task_action));
}

void twai_stop_uninstall(Twai_Handler_Struct* Twai_s)
{
    ESP_ERROR_CHECK(twai_stop());
    //Uninstall TWAI driver
    ESP_ERROR_CHECK(twai_driver_uninstall());
    ESP_LOGI(TAG, "Twai driver uninstalled");
    vQueueDelete(Twai_s->tx_task_queue);
}

void twai_receive_task(void *arg)
{   
    twai_message_t rx_msg;
    unsigned int msg_type;
    unsigned int target_node;
    unsigned int frame_type;
    // ESP_ERROR_CHECK(twai_start());
    while (1) {
        twai_receive(&rx_msg, portMAX_DELAY);
        unsigned int msg_type = (rx_msg.identifier >> 7) & 0b1111;
        unsigned int target_node = (rx_msg.identifier >> 3) & 0b1111;
        unsigned int frame_type = (rx_msg.identifier >> 0) & 0b111;
        if (target_node == ID_TARGET_MASTER_NODE) 
        {   
        } else if (target_node == ID_TARGET_MASTER_NODE) {
            //Listen for start command from master
            twai_message_t rx_msg;
            while (1) {
                twai_receive(&rx_msg, portMAX_DELAY);
                if (rx_msg.identifier == ID_TARGET_MASTER_NODE) {
                    break;
                }
            }
        } else if (target_node == ID_TARGET_MASTER_NODE) {
            //Listen for stop command from master
            twai_message_t rx_msg;
            while (1) {
                twai_receive(&rx_msg, portMAX_DELAY);
                if (rx_msg.identifier == ID_TARGET_MASTER_NODE) {
                    break;
                }
            }
        } else if (target_node == ID_TARGET_MASTER_NODE) {
            break;
        }
    }
    vTaskDelete(NULL);
}

void twai_transmit_task(void *arg)
{
    Twai_Handler_Struct* Twais = (Twai_Handler_Struct* )arg;
    // while (1) {
        // vTaskDelay(pdMS_TO_TICKS(10000));
    // }
    vTaskDelay(pdMS_TO_TICKS(10000));
    twai_stop_uninstall(Twais);
    vTaskDelete(NULL);
}