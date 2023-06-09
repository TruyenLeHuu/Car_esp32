#ifndef _SYSCONFIG__H
#define _SYSCONFIG__H

/**
 * Debugger?
 */

// #define DEBUG

#define NODE_ID 2

#define MAX_PACKET_NUMBER 8

#define MAX_NODE_NUMBER 50

#define TWAI_TRANSMIT_WAIT 500
/**
 * GPIOs defs
 */
// #define LED_BUILDING         ( 2 ) 
// #define LED_BUILDING         ( 27 ) 
// #define GPIO_OUTPUT_PIN_SEL  ( 1ULL<<LED_BUILDING )

#define BUTTON_PIN              ( 0 )
#define GPIO_INPUT_PIN_SEL      ( 1ULL<<BUTTON )   
#define BUTTON_PIN_1              ( 32 )
#define GPIO_INPUT_PIN_SEL      ( 1ULL<<BUTTON )   
#define BUTTON_PIN_2             ( 33 )
#define GPIO_INPUT_PIN_SEL      ( 1ULL<<BUTTON )   

#define MAX_LENGTH_TOPIC 50
#define MAX_LENGTH_DATA 200

#define RX_TASK_PRIO                    3       //Receiving task priority
#define TX_TASK_PRIO                    4       //Sending task priority

#define TX_GPIO_NUM                     CONFIG_TX_GPIO_NUM
#define RX_GPIO_NUM                     CONFIG_RX_GPIO_NUM

#define ID_MSG_TYPE_ALL_NODE            0x0
#define ID_MSG_TYPE_CMD_FRAME           0x1     //Command Frame
#define ID_MSG_TYPE_ACK_CMD_FRAME       0x2
#define ID_MSG_TYPE_R_N_FRAME           0x3     //Remote/Notice frame
#define ID_MSG_TYPE_TEST_FRAME          0x4
#define ID_MSG_TYPE_ACK_TEST_FRAME      0x5

#define ID_TARGET_ALL_NODE              0x0
#define ID_TARGET_EGN_CTRL_NODE         0x1     //Engine Control Node
#define ID_TARGET_LIGHT_GPS_CTRL_NODE   0x2     //Light-Speaker and GPS Node
#define ID_TARGET_MASTER_NODE           0x3
#define ID_TARGET_STEER_CTRL_NODE       0x4     //Steering Control Node
#define ID_TARGET_SENSOR_NODE           0x5     //Obstacle Sensor Node Engine
#define ID_TARGET_PW_MANAGEMENT_NODE    0x6     //Power Management Node

#define ID_END_FRAME                    0x0
#define ID_FIRST_FRAME                  0x1
#define ID_SECOND_FRAME                 0x2
#define ID_THIRD_FRAME                  0x3
#define ID_FOURTH_FRAME                 0x4
#define ID_FIFTH_FRAME                  0x5
#define ID_SIX_FRAME                    0x6
#define ID_SEVEN_FRAME                  0x7

#define ID_ALL_NODE                     0x0
#define ID_EGN_CTRL_NODE                0x1     //Engine Control Node
#define ID_LIGHT_GPS_CTRL_NODE          0x2     //Light-Speaker and GPS Node
#define ID_MASTER_NODE                  0x3
#define ID_STEER_CTRL_NODE              0x4     //Steering Control Node
#define ID_SENSOR_NODE                  0x5     //Obstacle Sensor Node Engine
#define ID_PW_MANAGEMENT_NODE           0x6

#define FIRST_PACKET_SIZE 6
#define NORMAL_PACKET_SIZE 7
/**
 * Info wifi your ssid & passwd
 */
// #define WIFI_SSID      "CEEC_Tenda"
// #define WIFI_PASS      "1denmuoi1"
#define WIFI_SSID      "1111"
#define WIFI_PASS      "01245678"

/**
 * Mqtt config
 */
// #define MQTT_ADDRESS 		"192.168.137.1"
#define MQTT_ADDRESS 		"192.168.137.154"
#define MQTT_PORT 		1883

/**
 * Topic
 */
#define TEST_TOPIC_PUB 		    "/Car_Data/"
#define LIGHT_TOPIC_PUB 		"/Status/Light"
#define SPEED_TOPIC_PUB 		"/Status/Speed"
#define SENSOR_TOPIC_PUB 		"/Status/Sensor" 
#define POWER_TOPIC_PUB 		"/Status/Power"
#define CONNECT_TOPIC_PUB 		"/Status/Connected"
#define DISCONNECT_TOPIC_PUB 	"/Status/Disconnected"

#define TEST_TOPIC_SUB 		"/Car_Control/#"

/**
 * Globals defs
 */
#define TRUE  1
#define FALSE 0

#endif 