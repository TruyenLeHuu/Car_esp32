/*
 * wifi_app.h
 */


#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#define EXAMPLE_ESP_MAXIMUM_RETRY  5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void event_handler(void* , esp_event_base_t , int32_t , void* );
void wifi_init(void);
#endif /* MAIN_WIFI_APP_H_ */













