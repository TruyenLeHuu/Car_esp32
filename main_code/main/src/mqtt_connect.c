#include <stdio.h>

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "mqtt_connect.h"
#include "twai_connect.h"
/**
 * System Config;
 */
#include "sys_config.h"
#include "cJSON.h"

static const char *TAG = "mqtt connection";

QueueHandle_t tx_task_queue;
mqtt_data_t mqtt_data;

void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}
void mqtt_receive_task(void* arg)
{   
    while (1)
    {
        
        ESP_LOGW(TAG, "Waiting queue...");

        mqtt_data_t mqtt_data_queue;
        xQueueReceive(tx_task_queue, (mqtt_data_t*) &mqtt_data_queue, portMAX_DELAY);

        mqtt_data_t mqtt_data = mqtt_data_queue;

        ESP_LOGW(TAG, "Received queue...");
        #ifdef DEBUG
        ESP_LOGW(TAG,"TOPIC = %s",  mqtt_data.topic);
        ESP_LOGW(TAG,"TOPIC_LENGTH = %d",  mqtt_data.topic_len);
        ESP_LOGW(TAG,"DATA = %s",   mqtt_data.data);
        ESP_LOGW(TAG,"DATA_LENGTH = %d",  mqtt_data.data_len);
        #endif

        if (NODE_ID == 2)
        {
            char topic[MAX_LENGTH_TOPIC];
            char data[MAX_LENGTH_DATA];
            snprintf(topic, MAX_LENGTH_TOPIC, "%.*s", mqtt_data.topic_len, mqtt_data.topic);
            snprintf(data, MAX_LENGTH_DATA, "%.*s", mqtt_data.data_len, mqtt_data.topic + mqtt_data.topic_len);
            
            if (strcmp(topic, "/Car_Control/Angle") == 0)
            {
                twai_msg send_msg = {
                .type_id = {
                            .msg_type = ID_MSG_TYPE_CMD_FRAME,
                            .target_type = ID_TARGET_STEER_CTRL_NODE,
                            },
                .msg = data,
                .msg_len = mqtt_data.data_len,
                };
                twai_transmit_msg(&send_msg);
            }
            else if (strcmp(topic, "/Car_Control/Speed") == 0)
            {
                twai_msg send_msg = {
                .type_id = {
                            .msg_type = ID_MSG_TYPE_CMD_FRAME,
                            .target_type = ID_TARGET_EGN_CTRL_NODE,
                            },
                .msg = data,
                .msg_len = mqtt_data.data_len,
                };
                twai_transmit_msg(&send_msg);
            }
            else if (strcmp(topic, "/Car_Control/Msg") == 0)
            {   
                cJSON *root;
                root = cJSON_Parse(data);
                uint8_t msg_type = cJSON_GetObjectItem(root, "id_msg")->valueint;
                uint8_t target_type = cJSON_GetObjectItem(root, "id_target")->valueint;
                char* msg = cJSON_GetObjectItem(root,"msg")->valuestring;
                twai_msg send_msg = {
                .type_id = {
                            .msg_type = msg_type,
                            .target_type = target_type,
                            },
                .msg = msg,
                .msg_len = strlen(msg),
                };
                twai_transmit_msg(&send_msg);
            }
        }
    }
    vTaskDelete(NULL);
}
/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    MQTT_Handler_Struct *mqtt_t = (MQTT_Handler_Struct *)handler_args;
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, SPEED_TOPIC_SUB, 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        mqtt_client_publish(mqtt_t, CONNECT_TOPIC_PUB, "Esp32");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        mqtt_data.topic = event->topic;
        mqtt_data.data = event->data;
        mqtt_data.data_len = event->data_len;
        mqtt_data.topic_len = event->topic_len;
        xQueueSend(tx_task_queue, (mqtt_data_t*) &mqtt_data, portMAX_DELAY);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}
bool mqtt_client_publish(MQTT_Handler_Struct *mqtt_t, char *topic, char *publish_string)
{
    if (mqtt_t->client)
    {
        int msg_id = esp_mqtt_client_publish(mqtt_t->client, topic, publish_string, 0, 1, 0);
        ESP_LOGI(TAG, "Sent publish returned msg_id=%d", msg_id);
        return true;
    }
    return false;
}
void mqtt_init_start(MQTT_Handler_Struct *mqtt_t)
{
    mqtt_t->client = esp_mqtt_client_init(mqtt_t->mqtt_cfg);
    tx_task_queue = xQueueCreate(5, sizeof(mqtt_data_t));
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(mqtt_t->client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_t);
    esp_mqtt_client_start(mqtt_t->client);
}
