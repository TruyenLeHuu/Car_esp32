/*
 * mqtt.h
 */
#ifndef MAIN_MQTT_APP_H_
#define MAIN_MQTT_APP_H_
#include "mqtt_client.h"

typedef enum {
    RX_RECEIVE_PING,
    RX_RECEIVE_START_CMD,
    RX_RECEIVE_STOP_CMD,
    RX_TASK_EXIT,
} tx_mqtt_task_action_t;

typedef enum {
    MQTT_STATE_INIT = 0,
    MQTT_STATE_DISCONNECTED,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_WAIT_RECONNECT,
} mqtt_client_state_st;

typedef struct mqtt_data_t{   
    char* topic;
    int topic_len;
    char* data;
    int data_len;
} mqtt_data_t;

typedef struct MQTT_Handler_Struct{   
    esp_mqtt_client_handle_t client;
    esp_mqtt_client_config_t* mqtt_cfg;    
    QueueHandle_t tx_mqtt_task_queue;
    mqtt_client_state_st state;
} MQTT_Handler_Struct;

void log_error_if_nonzero(const char *, int );
void mqtt_event_handler(void *, esp_event_base_t , int32_t , void *);
bool mqtt_client_publish(MQTT_Handler_Struct* , char* , char *);
void mqtt_init_start(MQTT_Handler_Struct* );
void mqtt_to_twai_transmit(void*);
void mqtt_receive_task(void*);
#endif /* MAIN_MQTT_APP_H_ */













