#ifndef TWAI_APP_H_
#define TWAI_APP_H_
#include "driver/twai.h"
#include "freertos/queue.h"
#include "mqtt_connect.h"
/**
 * Constant
 */

typedef enum {
    TX_SEND_PING_RESP,
    TX_SEND_DATA,
    TX_SEND_STOP_RESP,
    TX_TASK_EXIT,
} tx_twai_task_action_t;

typedef struct id_type_msg_t{
    uint8_t msg_type;                
    uint8_t target_type;              
    uint8_t frame_type;    
} id_type_msg;

typedef struct twai_msg_t{
    id_type_msg type_id;                
    uint8_t msg_len;              
    char* msg;    
} twai_msg;

typedef struct twai_rx_msg_t{
    MQTT_Handler_Struct* mqtt_handler;
    twai_message_t rx_buffer_msg[8];                
    uint8_t current_buffer_len;    
    uint8_t graft_buffer_len;
} twai_rx_msg;

typedef struct Twai_Handler_s
{
	twai_filter_config_t f_config;
    twai_timing_config_t t_config;
    twai_general_config_t g_config;
    QueueHandle_t tx_task_queue;
    tx_twai_task_action_t tx_task_action;
    uint8_t id;
} Twai_Handler_Struct;

void twai_install_start(Twai_Handler_Struct* );
void twai_stop_uninstall(Twai_Handler_Struct* );
void twai_to_mqtt_transmit(MQTT_Handler_Struct* , uint8_t , char* );
void twai_receive_task(void *);
void twai_transmit_task(void *);
void twai_transmit_msg(void *);
void twai_transmit_multi_task(void *);
void twai_transmit_single_for_multi(void *);
void twai_transmit_single(void *);
void twai_graft_packet_task(void *);
uint32_t encode_id(id_type_msg );
id_type_msg decode_id(uint32_t );
void log_binary(uint16_t );
#endif /* TWAI_APP_H_ */