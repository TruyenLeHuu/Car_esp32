#ifndef TWAI_APP_H_
#define TWAI_APP_H_
#include "driver/twai.h"
#include "freertos/queue.h"
/**
 * Constant
 */
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

typedef enum {
    TX_SEND_PING_RESP,
    TX_SEND_DATA,
    TX_SEND_STOP_RESP,
    TX_TASK_EXIT,
} tx_twai_task_action_t;

typedef struct Twai_Handler_s
{
	twai_filter_config_t f_config;
    twai_timing_config_t t_config;
    twai_general_config_t g_config;
    QueueHandle_t tx_task_queue;
    tx_twai_task_action_t tx_task_action;

} Twai_Handler_Struct;

void twai_install_start(Twai_Handler_Struct* );
void twai_stop_uninstall(Twai_Handler_Struct* );
void twai_receive_task(void *);
void twai_transmit_task(void *);
#endif /* TWAI_APP_H_ */