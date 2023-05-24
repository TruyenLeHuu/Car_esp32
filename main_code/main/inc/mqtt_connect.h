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


void log_error_if_nonzero(const char *, int );
void mqtt_event_handler(void *, esp_event_base_t , int32_t , void *);
bool mqtt_client_publish(char* , char *);
void mqtt_app_start(void);
void mqtt_transmit_task(void *);
#endif /* MAIN_MQTT_APP_H_ */













