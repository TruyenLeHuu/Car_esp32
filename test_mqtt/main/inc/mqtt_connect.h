/*
 * mqtt.h
 */
#include "mqtt_client.h"

#ifndef MAIN_MQTT_APP_H_
#define MAIN_MQTT_APP_H_

void log_error_if_nonzero(const char *, int );
void mqtt_event_handler(void *, esp_event_base_t , int32_t , void *);
bool mqtt_client_publish(char* , char *);
void mqtt_app_start(void);
#endif /* MAIN_MQTT_APP_H_ */













