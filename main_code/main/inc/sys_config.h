#ifndef _SYSCONFIG__H
#define _SYSCONFIG__H

/**
 * Debugger?
 */
#define DEBUG 1


#define NODE_ID 24

#define MAX_BUFFER_RX 20
/**
 * GPIOs defs
 */
// #define LED_BUILDING         ( 2 ) 
// #define LED_BUILDING         ( 27 ) 
// #define GPIO_OUTPUT_PIN_SEL  ( 1ULL<<LED_BUILDING )

#define BUTTON_PIN              ( 0 )
#define GPIO_INPUT_PIN_SEL      ( 1ULL<<BUTTON )   

/**
 * Info wifi your ssid & passwd
 */
// #define WIFI_SSID      "CEEC_Tenda"
// #define WIFI_PASS      "1denmuoi1"
#define WIFI_SSID      "1111"
#define WIFI_PASS      "01245678"

/**
 * Net config
 */
#define FIXED_IP 0
#define IP_ADDRESS 		"192.168.0.121"
#define GATEWAY_ADDRESS "192.168.0.1"
#define NETMASK_ADDRESS "255.255.255.0"

/**
 * Mqtt config
 */
#define MQTT_ADDRESS 		"192.168.137.1"
#define MQTT_PORT 		1883

/**
 * Topic
 */
#define TEST_TOPIC_PUB 		"/Car_Data/"
#define CONNECT_TOPIC_PUB 		"/Status/Connected"
#define DISCONNECT_TOPIC_PUB 		"/Status/Disconnected"

#define TEST_TOPIC_SUB 		"/Car_Control/#"

/**
 * Globals defs
 */
#define TRUE  1
#define FALSE 0

#endif 