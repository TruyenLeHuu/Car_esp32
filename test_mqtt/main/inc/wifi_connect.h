/*
 * wifi_app.h
 *
 *  Created on: Oct 17, 2021
 *      Author: kjagu
 */
#include "freertos/event_groups.h"

#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#define EXAMPLE_ESP_WIFI_SSID      "CEEC_Tenda"
#define EXAMPLE_ESP_WIFI_PASS      "1denmuoi1"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";



static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
void wifi_init_sta(void);
#endif /* MAIN_WIFI_APP_H_ */




























