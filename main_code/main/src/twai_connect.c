#include "twai_connect.h"  
#include <string.h>
#include "esp_log.h"
#include "sys_config.h"
#include "stdint.h"
#include "time.h"
#include "cJSON.h"

static const char *TAG = "Twai connection";

uint32_t num_err = 0, num_correct = 0;

void twai_install_start(Twai_Handler_Struct* Twai_s)
{
    ESP_ERROR_CHECK(twai_driver_install(&Twai_s->g_config, &Twai_s->t_config, &Twai_s->f_config));
    ESP_LOGI(TAG, "TWAI Driver installed");

    ESP_ERROR_CHECK(twai_start());

    Twai_s->tx_task_queue = xQueueCreate(1, sizeof(Twai_s->tx_task_action));
}

void twai_stop_uninstall(Twai_Handler_Struct* Twai_s)
{
    ESP_ERROR_CHECK(twai_stop());
    //Uninstall TWAI driver
    ESP_ERROR_CHECK(twai_driver_uninstall());
    ESP_LOGI(TAG, "Twai driver uninstalled");
    vQueueDelete(Twai_s->tx_task_queue);
}
/* Transmit to server via mqtt */
void twai_to_mqtt_transmit(MQTT_Handler_Struct* mqtt_handler, uint8_t id_node, char* data)
{
    switch (id_node)
    {
    case ID_EGN_CTRL_NODE:
        mqtt_client_publish(mqtt_handler, SPEED_TOPIC_PUB, data);
        break;
    case ID_SENSOR_NODE:
        mqtt_client_publish(mqtt_handler, SENSOR_TOPIC_PUB, data);
        break;
    case ID_PW_MANAGEMENT_NODE:
        mqtt_client_publish(mqtt_handler, POWER_TOPIC_PUB, data);
        break;
    default:
        ESP_LOGE(TAG, "Not register for this node!");
        mqtt_client_publish(mqtt_handler, "/Status/Unknown", data);
        break;
    }
}
/* Concatenate multiple frames together */
void twai_graft_packet_task(void *arg)
{
    /* Array of frame msg */
    twai_rx_msg* rx_msg = (twai_rx_msg*) arg;
    char str_msg[100] = "";
    /* Number of extra byte that not relate to string msg */
    uint8_t num_extra_byte;
    /* Length of string msg */
    uint8_t length_msg = rx_msg->rx_buffer_msg[0].data[1];
    #ifdef CHECK_FRAME_CRC
    uint8_t remain_msg = length_msg; 
    #endif
    /* Number of frame (packet) */
    int num_packets = (length_msg - FIRST_PACKET_SIZE + NORMAL_PACKET_SIZE - 1) / NORMAL_PACKET_SIZE + 1;  
    /* Checksum for number of frame */
    if (num_packets != rx_msg->graft_buffer_len)
    {
        ESP_LOGE(TAG, "num_packets = %d not equal rx_msg->graft_buffer_len = %d!", num_packets, rx_msg->graft_buffer_len);
    }
    /* Concat msg */
    for (int i = 0; i < num_packets; i++)
    {  
        num_extra_byte = (i == 0 ? 2 : 1);
        #ifdef CHECK_FRAME_CRC
        uint8_t msg_length = (i == 0 ? FIRST_PACKET_SIZE : (i == (num_packets - 1) ? remain_msg : NORMAL_PACKET_SIZE));
        remain_msg = remain_msg - (i == 0 ? FIRST_PACKET_SIZE : (i == (num_packets - 1) ? 0 : NORMAL_PACKET_SIZE));
        // ESP_LOGW(TAG, "remain message: %d.", remain_msg);
        // ESP_LOGW(TAG, "remain message: %d.", msg_length);
        // ESP_LOGW(TAG, "crc: %d.", rx_msg->rx_buffer_msg[i].data[msg_length + num_extra_byte]);
        ESP_LOGW(TAG, "Crc of frame %d: %d.", i + 1, crc_8(&rx_msg->rx_buffer_msg[i].data[num_extra_byte], msg_length));   
        
        if (rx_msg->rx_buffer_msg[i].data[msg_length + num_extra_byte] != crc_8(&rx_msg->rx_buffer_msg[i].data[num_extra_byte], msg_length))
        {
                num_err++;
                vTaskDelete(NULL);
        } 
        else {
            rx_msg->rx_buffer_msg[i].data[msg_length + num_extra_byte] = (uint8_t) '\0';
        #endif
            strcat(str_msg, (char*)rx_msg->rx_buffer_msg[i].data + num_extra_byte);
            #ifdef DEBUG
            ESP_LOGW(TAG, "Current message: %s.", str_msg);
            #endif
        #ifdef CHECK_FRAME_CRC
        }
        #endif
    }

    id_type_msg id = decode_id(rx_msg->rx_buffer_msg[0].identifier);
    #ifdef CHECK_MSG_CRC
    if ((uint8_t)str_msg[rx_msg->rx_buffer_msg[0].data[1]-1] != crc_8((uint8_t*)str_msg, rx_msg->rx_buffer_msg[0].data[1] - 1))
    {
        num_err++;
    } else 
    {
        str_msg[rx_msg->rx_buffer_msg[0].data[1]-1] = '\0';
    #endif
        cJSON *root;
        root=cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "node_id", rx_msg->rx_buffer_msg[0].data[0]);
        cJSON_AddNumberToObject(root, "id_msg", id.msg_type);
        cJSON_AddNumberToObject(root, "id_target", id.target_type);
        cJSON_AddStringToObject(root, "msg", str_msg);
        char *rendered=cJSON_Print(root);
        /* Transmit msg via mqtt */
        twai_to_mqtt_transmit(rx_msg->mqtt_handler, rx_msg->rx_buffer_msg[0].data[0], rendered);

        /* Log message */
        ESP_LOGW(TAG, "From node ID: %d.", rx_msg->rx_buffer_msg[0].data[0]);
        log_binary((uint16_t)rx_msg->rx_buffer_msg[0].identifier);
        ESP_LOGW(TAG, "Message length: %d.", rx_msg->rx_buffer_msg[0].data[1]);
        ESP_LOGW(TAG, "Twai message receive: %s.", str_msg);
    #ifdef CHECK_MSG_CRC
        num_correct++;
    }
    ESP_LOGW(TAG, "Number error msg: %d, number correct msg: %d", num_err, num_correct);
    #endif
    vTaskDelete(NULL);
}
/* Using for log twai message */
void log_packet(twai_message_t rx_msg)
{
    ESP_LOGW(TAG, "===============================================");
    ESP_LOGW(TAG, "From node ID: %d.", rx_msg.data[0]);
    log_binary((uint16_t)rx_msg.identifier);
    ESP_LOGW(TAG, "Message length: %d.", rx_msg.data[1]);
    ESP_LOGW(TAG, "Twai message receive: %s.    ", (char*)rx_msg.data);
    ESP_LOGW(TAG, "===============================================");
}

void twai_receive_task(void *arg)
{   
    /* Mqtt handler */
    MQTT_Handler_Struct* mqtt_h = (MQTT_Handler_Struct*) arg;
    /* Twai receive buffer for message multiple frame */
    twai_rx_msg* twai_rx_buf = (twai_rx_msg*)malloc(sizeof(twai_rx_msg));
    
    id_type_msg type_id;
    uint8_t node_transmit_id; 
    if (twai_rx_buf == NULL) 
    {
        ESP_LOGE(TAG, "Can't malloc struct array.");
        vTaskDelete(NULL);
    }
    /* Init for buffer */
    ESP_LOGW(TAG, "Time for init buffer: ");
    for(int i = 0; i < MAX_NODE_NUMBER; i++)
    {
        ESP_LOGW(TAG, "Buffer %d", i);
        twai_rx_buf[i].current_buffer_len = 0;
        twai_rx_buf[i].graft_buffer_len = 0;
    }
    
    twai_message_t rx_msg = {.identifier = 0x0000, .data_length_code = 0, .data = {0, 0 , 0 , 0 ,0 ,0 ,0 ,0}}; 

    while (1) 
    {
        twai_receive(&rx_msg, portMAX_DELAY);

        node_transmit_id = rx_msg.data[0];
        type_id = decode_id(rx_msg.identifier);
        if (type_id.frame_type == ID_END_FRAME && twai_rx_buf[node_transmit_id].current_buffer_len == 0)
        {   
            #ifdef CHECK_MSG_CRC
            #ifdef CHECK_FRAME_CRC
            if (rx_msg.data[rx_msg.data[1] + 2] != crc_8(&rx_msg.data[2], rx_msg.data[1]))
            #endif
            if (rx_msg.data[rx_msg.data[1]] != crc_8(&rx_msg.data[2], rx_msg.data[1] - 1))
            {
                num_err++;
            } 
            else 
            {
            #ifdef CHECK_FRAME_CRC
                rx_msg.data[rx_msg.data[1] + 2] = (uint8_t)'\0';
            #else
                rx_msg.data[rx_msg.data[1]] = (uint8_t)'\0';
            #endif
            #endif
                cJSON *root;
                root=cJSON_CreateObject();
                cJSON_AddNumberToObject(root, "node_id", node_transmit_id);
                cJSON_AddNumberToObject(root, "id_msg", type_id.msg_type);
                cJSON_AddNumberToObject(root, "id_target", type_id.target_type);
                cJSON_AddStringToObject(root, "msg", (char*)&rx_msg.data[2]);
                char *rendered=cJSON_Print(root);
                /* This frame is the single frame */
                twai_to_mqtt_transmit(mqtt_h, rx_msg.data[0], rendered);
                log_packet(rx_msg);
            #ifdef CHECK_MSG_CRC
                num_correct++;
            }
            ESP_LOGW(TAG, "Number error msg: %d, number correct msg: %d", num_err, num_correct);
            #endif
        }
        else 
        {   
            /* This frame is the multi frame */
            #ifdef DEBUG
            log_packet(rx_msg);
            #endif
            /* Disengage all redundant frame */
            if (type_id.frame_type == ID_FIRST_FRAME && twai_rx_buf[node_transmit_id].current_buffer_len != 0) 
                twai_rx_buf[node_transmit_id].current_buffer_len = 0;
            #ifdef DEBUG
            ESP_LOGW(TAG, "Add packet into buffer, from node: %d.", node_transmit_id);
            #endif
            /* Push frame to buffer */
            twai_rx_buf[node_transmit_id].rx_buffer_msg[twai_rx_buf[node_transmit_id].current_buffer_len] = rx_msg;
            twai_rx_buf[node_transmit_id].current_buffer_len++;
            if (type_id.frame_type == ID_END_FRAME)
            {
                /* This is last frame of message */
                #ifdef DEBUG
                ESP_LOGW(TAG, "Start graft packet: %d.", node_transmit_id);
                #endif
                
                twai_rx_buf[node_transmit_id].graft_buffer_len = twai_rx_buf[node_transmit_id].current_buffer_len;
                twai_rx_msg graft_buffer_rx_msg = twai_rx_buf[node_transmit_id];
                graft_buffer_rx_msg.mqtt_handler = mqtt_h;
                twai_rx_buf[node_transmit_id].current_buffer_len = 0;
                xTaskCreatePinnedToCore(twai_graft_packet_task, "TWAI_tx_single", 4096, &graft_buffer_rx_msg, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
            }
        }
    }
    vTaskDelete(NULL);
}
void log_binary(uint16_t number) 
{
    int bits = sizeof(number) * 8; 
    char binary[16*8+1];
    binary[bits] = '\0';  
    for (int i = bits - 1; i >= 0; i--) {
        binary[i] = (number & 1) ? '1' : '0';
        number >>= 1;
    }
    ESP_LOGW(TAG, "Message ID: %s.", binary);
}
void twai_transmit_multi_task(void * arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;
    /* Backup string message */
    char send_str_msg[100];
    uint8_t packet_size;
    sprintf(send_str_msg, "%s", send_msg->msg); 

    int num_packets = (send_msg->msg_len - FIRST_PACKET_SIZE + NORMAL_PACKET_SIZE - 1) / NORMAL_PACKET_SIZE + 1;  
    int remaining_bytes = send_msg->msg_len - FIRST_PACKET_SIZE;
    #ifdef DEBUG
    ESP_LOGI(TAG, "Twai transmited packet: %d.", 0);
    #endif
    /* Transmit first frame */
    send_msg->type_id.frame_type = ID_FIRST_FRAME;

    twai_transmit_single(send_msg);
    /* Transmit first-after frame */
    for (int i = 1; i < num_packets; i++) 
    {     
        if (i == num_packets - 1) 
        {
            send_msg->type_id.frame_type = ID_END_FRAME;
        }
        else
        {
            send_msg->type_id.frame_type = i + 1;
        }
        packet_size = (remaining_bytes >= NORMAL_PACKET_SIZE) ? NORMAL_PACKET_SIZE : remaining_bytes;
        #ifdef CHECK_FRAME_CRC
        send_msg->msg = strndup(send_str_msg + ((i - 1) * NORMAL_PACKET_SIZE + 5), packet_size);
        #else
        send_msg->msg = strndup(send_str_msg + ((i - 1) * NORMAL_PACKET_SIZE + 6), packet_size);
        #endif
        send_msg->msg_len = packet_size;

        twai_transmit_single_for_multi(send_msg);

        remaining_bytes -= packet_size;
         #ifdef DEBUG
        ESP_LOGI(TAG, "Twai transmited packet: %d.", i);
        #endif
    }
    vTaskDelete(NULL);
}
/* Function that transmit single frame for multil-msg */ 
void twai_transmit_single_for_multi(void * arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;
    twai_message_t twai_tx_msg = {  .identifier = encode_id(send_msg->type_id), 
    #ifdef CHECK_FRAME_CRC
                                    .data_length_code = send_msg->msg_len + 2,
                                    .data = {NODE_ID, 0, 0, 0, 0, 0, 0, crc_8((uint8_t*)send_msg->msg, send_msg->msg_len)}};
    #else
                                    .data_length_code = send_msg->msg_len + 1,
                                    .data = {NODE_ID, 0, 0, 0, 0, 0, 0, 0}};
    #endif
    memcpy((char*)&twai_tx_msg.data[1], send_msg->msg, send_msg->msg_len);
    twai_transmit(&twai_tx_msg, pdMS_TO_TICKS(TWAI_TRANSMIT_WAIT));

    ESP_LOGI(TAG, "Transmitted msg %d -%s", twai_tx_msg.data_length_code, (char*) twai_tx_msg.data);
}
/* Function that transmit single frame*/ 
void twai_transmit_single(void * arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;

    twai_message_t twai_tx_msg = {  
        .identifier = encode_id(send_msg->type_id), 
    #ifdef CHECK_FRAME_CRC
        .data_length_code = send_msg->msg_len <= 5 ? send_msg->msg_len + 3 : 8,
        .data = {NODE_ID, send_msg->msg_len, 0, 0, 0, 0, 0, crc_8((uint8_t*)send_msg->msg, send_msg->msg_len <= 5 ? send_msg->msg_len : 5)}};

    memcpy((char*)&twai_tx_msg.data[2], send_msg->msg, send_msg->msg_len <= 5 ? send_msg->msg_len : 5);
    #else
        .data_length_code = send_msg->msg_len <= 6 ? send_msg->msg_len + 2 : 8,
        .data = {NODE_ID, send_msg->msg_len, 0, 0, 0, 0, 0, 0}};

    memcpy((char*)&twai_tx_msg.data[2], send_msg->msg, send_msg->msg_len <= 6 ? send_msg->msg_len : 6);
    #endif
    twai_transmit(&twai_tx_msg, pdMS_TO_TICKS(TWAI_TRANSMIT_WAIT));

    ESP_LOGI(TAG, "Transmitted msg %d - %s", twai_tx_msg.data_length_code, (char*) twai_tx_msg.data);
}
void twai_transmit_msg(void* arg)
{
    twai_msg* send_msg = (twai_msg*) arg;
    #ifdef CHECK_MSG_CRC
    send_msg->msg[send_msg->msg_len] = (char)crc_8((uint8_t*)send_msg->msg, send_msg->msg_len);
    // ESP_LOGE(TAG, "CRC: %d", crc_8((uint8_t*)send_msg->msg, send_msg->msg_len));
    send_msg->msg_len++;
    #endif
    #ifdef CHECK_FRAME_CRC
    if(send_msg->msg_len > 5)
    #else
    if(send_msg->msg_len > 6)
    #endif
    {   /* Multi-msg */
        
        if(send_msg->msg_len > 55)
        {
            ESP_LOGE(TAG, "Msg too long to send!");
        }
        ESP_LOGI(TAG, "=====================================");
        xTaskCreatePinnedToCore(twai_transmit_multi_task, "TWAI_tx_multi", 4096, send_msg, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
    }
    else 
    {   /* Single msg */
        ESP_LOGI(TAG, "=====================================");
        send_msg->type_id.frame_type = ID_END_FRAME;
        twai_transmit_single(send_msg);
    }
}
uint32_t encode_id(id_type_msg type_id)
{
    return (uint32_t) (type_id.msg_type << 7) | (type_id.target_type << 3) | type_id.frame_type;
}
id_type_msg decode_id(uint32_t id)
{
    id_type_msg type_id = {
                        .msg_type = (id >> 7) & 0b1111,
                        .target_type = (id >> 3) & 0b1111,
                        .frame_type = (id >> 0) & 0b111,
                            };
    return type_id;
}
uint8_t crc_8(uint8_t* data, uint8_t len) {
// ESP_LOGE(TAG, "Data:%s-len:%d", (char*)data, len);
  uint32_t crc = 0;
  int i, j;
  for (j = len; j; j--, data++) {
    crc ^= (*data << 8);
    //   ESP_LOGE(TAG, "data: %d", *data);
    for(i = 8; i; i--) {
      if (crc & 0x8000)
        crc ^= (0x1070 << 3);
      crc <<= 1;
    //   ESP_LOGW(TAG, "Crc of frame: %d", (crc >> 8));
    }
  }
  return (uint8_t)(crc >> 8);
}
void twai_transmit_task(void *arg)
{
    srand(time(NULL)); 
    Twai_Handler_Struct* Twais = (Twai_Handler_Struct* )arg;
    uint8_t speed;
    char str[50];
    while (1) {
        // if (!gpio_get_level(BUTTON_PIN) || !gpio_get_level(BUTTON_PIN_1) || !gpio_get_level(BUTTON_PIN_2)){
        if (!gpio_get_level(BUTTON_PIN)){
            speed = rand() % 100 + 1;
            sprintf(str, "%dAlo d %dhel %d:", speed, speed, speed);
            // sprintf(str, "=%dA=", speed);
            twai_msg send_msg = {
                .type_id = {
                            .msg_type = ID_MSG_TYPE_CMD_FRAME,
                            .target_type = ID_TARGET_ALL_NODE,
                            },
                // .msg = "Alo and hello world!!",
                // .msg_len = strlen("Alo and hello world!!"),
                .msg = str,
                .msg_len = strlen(str),
                // .msg = "Alo !",
                // .msg_len = strlen("Alo !"),
            };
            twai_transmit_msg(&send_msg);
            
            #ifdef DEBUG
            vTaskDelay(pdMS_TO_TICKS(200));
            #endif
            vTaskDelay(pdMS_TO_TICKS(1000));
           
        }
        else
            vTaskDelay(pdMS_TO_TICKS(50));
    }
    twai_stop_uninstall(Twais);
    vTaskDelete(NULL);
}