#include "twai_connect.h"  
#include <string.h>
#include "esp_log.h"
#include "sys_config.h"
#include "stdint.h"
#include "string.h"

#include "cJSON.h"

static const char *TAG = "Twai connection";

uint32_t num_err = 0, num_correct = 0;

void twai_install_start(Twai_Handler_Struct* Twai_s)
{
    
    ESP_ERROR_CHECK(twai_driver_install(&Twai_s->g_config, &Twai_s->t_config, &Twai_s->f_config));
    ESP_LOGI(TAG, "TWAI Driver installed");

    ESP_ERROR_CHECK(twai_start());

    // esp_log_level_set(TAG, ESP_LOG_NONE);
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
void twai_to_mqtt_transmit(MQTT_Handler_Struct* mqtt_handler, uint8_t id_node, id_type_msg id, char* str_msg)
{
    if(mqtt_handler->state == MQTT_STATE_CONNECTED)
    {
        int idMSG, res;
        double X, Y, Z;
        char *rendered;
        
        cJSON *root;
        root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "node_id", id_node);
        cJSON_AddNumberToObject(root, "id_msg", id.msg_type);
        cJSON_AddNumberToObject(root, "id_target", id.target_type);
        // ESP_LOGI(TAG, "Data: %s", str_msg);
        char* token = strtok(str_msg, "="); 
        if (token == NULL)
        {
            ESP_LOGE(TAG, "Wrong Format 1\r\n");
            return;
        }
        sscanf(token + 1, "%d", &idMSG);
        switch (idMSG)
        {
        case TYPE_DATA_MSG_CAR_VELOCITY : 
            {
                double velocity; 
                token = strtok(NULL, "\n");
                if (token == NULL)
                {
                    ESP_LOGE(TAG, "Wrong Format 2\r\n");
                    return;
                }
                res = sscanf(token, "%lf", &velocity);
                if(res != 1)
                {
                    ESP_LOGE(TAG, "Wrong Format 3\r\n");
                    return;
                }
                cJSON_AddNumberToObject(root, "velocity", velocity);
                rendered=cJSON_Print(root);
                /* Transmit msg via mqtt */
                mqtt_client_publish(mqtt_handler, SPEED_TOPIC_PUB, rendered);
                
                free(rendered);
            }
            break;
        case TYPE_DATA_MSG_GPS_DATA : 
            {   
                double _long, _lat; 
                token = strtok(NULL, "\n");
                if (token == NULL)
                {
                    ESP_LOGE(TAG, "Wrong Format 2\r\n");
                    return;
                }
                res = sscanf(token, "%lf;%lf", &_long, &_lat);
                if(res != 2)
                {
                    ESP_LOGE(TAG, "Wrong Format 3\r\n");
                    return;
                }
                cJSON_AddNumberToObject(root, "long", _long);
                cJSON_AddNumberToObject(root, "lat", _lat);
                rendered=cJSON_Print(root);
                /* Transmit msg via mqtt */
                mqtt_client_publish(mqtt_handler, GPS_TOPIC_PUB, rendered);

                free(rendered);
            }
            break;
        case TYPE_DATA_MSG_POWER_MEASURE: 
        { 
                char* name;
                double power, voltage, current; 
                char* token = strtok(NULL, "="); 
                if (token == NULL)
                {
                    ESP_LOGE(TAG, "Wrong Format 2\r\n");
                    return;
                }
                name = strtok(token, ";");
                if (name == NULL)
                {
                    ESP_LOGE(TAG, "Wrong Format 3\r\n");
                    return;
                }
                token = strtok(NULL, "\n");
                if (token == NULL)
                {
                    ESP_LOGE(TAG, "Wrong Format 4\r\n");
                    return;
                }
                int res = sscanf(token, "%lf;%lf;%lf", &power, &voltage, &current);
                if(res != 3)
                {
                    ESP_LOGE(TAG, "Wrong Format 5\r\n");
                    return;
                }
                cJSON_AddStringToObject(root, "name", name);
                cJSON_AddNumberToObject(root, "power", power);
                cJSON_AddNumberToObject(root, "voltage", voltage);
                cJSON_AddNumberToObject(root, "current", current);
                rendered=cJSON_Print(root);

                /* Transmit msg via mqtt */
                mqtt_client_publish(mqtt_handler, POWER_TOPIC_PUB, rendered);

                free(rendered);
            }
            break;
        case TYPE_DATA_MSG_DISTANCE_SENSOR : 
            {   
                int isIRDetect;
                double laserRanging; 
                token = strtok(NULL, "\n");
                if (token == NULL)
                {
                    ESP_LOGE(TAG, "Wrong Format 2\r\n");
                    return;
                }
                res = sscanf(token, "%d;%lf", &isIRDetect, &laserRanging);
                if(res != 2)
                {
                    ESP_LOGE(TAG, "Wrong Format 3\r\n");
                    return;
                }
                cJSON_AddNumberToObject(root, "is_ir_detect", isIRDetect);
                cJSON_AddNumberToObject(root, "laser_ranging", laserRanging);
                rendered=cJSON_Print(root);

                /* Transmit msg via mqtt */
                mqtt_client_publish(mqtt_handler, SENSOR_TOPIC_PUB, rendered);

                free(rendered);
            }
            break;
        case TYPE_DATA_MSG_IMU_EULER_DATA : 
            {   
                double heading, roll, pitch; 
                token = strtok(NULL, "\n");
                if (token == NULL)
                {
                    ESP_LOGE(TAG, "Wrong Format 2\r\n");
                    return;
                }
                res = sscanf(token, "%lf;%lf;%lf", &heading, &roll, &pitch);
                if(res != 3)
                {
                    ESP_LOGE(TAG, "Wrong Format 3\r\n");
                    return;
                }
                cJSON_AddNumberToObject(root, "heading", heading);
                cJSON_AddNumberToObject(root, "roll", roll);
                cJSON_AddNumberToObject(root, "pitch", pitch);
                rendered=cJSON_Print(root);

                /* Transmit msg via mqtt */
                mqtt_client_publish(mqtt_handler, IMU_EULER_TOPIC_PUB, rendered);

                free(rendered);
            }
            break;
        case TYPE_DATA_MSG_IMU_ACCEL_DATA : 
            {   
                token = strtok(NULL, "\n");
                if (token == NULL)
                {
                    ESP_LOGE(TAG, "Wrong Format 2\r\n");
                    return;
                }
                res = sscanf(token, "%lf;%lf;%lf", &X, &Y, &Z);
                if(res != 3)
                {
                    ESP_LOGE(TAG, "Wrong Format 3\r\n");
                    return;
                }
                cJSON_AddNumberToObject(root, "X", X);
                cJSON_AddNumberToObject(root, "Y", Y);
                cJSON_AddNumberToObject(root, "Z", Z);
                rendered = cJSON_Print(root);

                /* Transmit msg via mqtt */
                mqtt_client_publish(mqtt_handler, IMU_ACCEL_TOPIC_PUB, rendered);

                free(rendered);
            }
            break;
        case TYPE_DATA_MSG_IMU_GYRO_DATA : 
            {
                token = strtok(NULL, "\n");
                if (token == NULL)
                {
                    ESP_LOGE(TAG, "Wrong Format 2\r\n");
                    return;
                }
                res = sscanf(token, "%lf;%lf;%lf", &X, &Y, &Z);
                if(res != 3)
                {
                    ESP_LOGE(TAG, "Wrong Format 3\r\n");
                    return;
                }
                cJSON_AddNumberToObject(root, "X", X);
                cJSON_AddNumberToObject(root, "Y", Y);
                cJSON_AddNumberToObject(root, "Z", Z);
                rendered=cJSON_Print(root);

                /* Transmit msg via mqtt */
                mqtt_client_publish(mqtt_handler, IMU_GYRO_TOPIC_PUB, rendered);
                free(rendered);
            }
            break;  
        default:
            break;
        }
        // ESP_LOGE(TAG, "==============%d==============\r\n", esp_mqtt_client_get_state(mqtt_handler->client));
        cJSON_Delete(root);  
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
        strcat(str_msg, (char*)rx_msg->rx_buffer_msg[i].data + num_extra_byte);
        #ifdef DEBUG
        ESP_LOGW(TAG, "Current message: %s.", str_msg);
        #endif
    }

    id_type_msg id = decode_id(rx_msg->rx_buffer_msg[0].identifier);
    #ifdef CHECK_MSG_CRC
    if ((uint8_t)str_msg[rx_msg->rx_buffer_msg[0].data[1] - 1] != crc_8((uint8_t*)str_msg, rx_msg->rx_buffer_msg[0].data[1] - 1))
    {
        num_err++;
        #if LOG_ENABLE_TWAI == 1

        ESP_LOGW(TAG, "From node ID: %d", rx_msg->rx_buffer_msg[0].data[0]);
        log_binary((uint16_t)rx_msg->rx_buffer_msg[0].identifier);
        ESP_LOGW(TAG, "Message length: %d", rx_msg->rx_buffer_msg[0].data[1]);
        ESP_LOGW(TAG, "Twai message receive: %s", str_msg);
        #endif
    } else 
    {
        str_msg[rx_msg->rx_buffer_msg[0].data[1] - 1] = '\0';
    #endif
        /* Log message */
        #if LOG_ENABLE_TWAI == 1
        ESP_LOGW(TAG, "From node ID: %d", rx_msg->rx_buffer_msg[0].data[0]);
        log_binary((uint16_t)rx_msg->rx_buffer_msg[0].identifier);
        ESP_LOGW(TAG, "Message length: %d", rx_msg->rx_buffer_msg[0].data[1]);
        ESP_LOGW(TAG, "Twai message receive: %s", str_msg);
        #endif

        twai_to_mqtt_transmit(rx_msg->mqtt_handler, rx_msg->rx_buffer_msg[0].data[0], id, str_msg);

        
    #ifdef CHECK_MSG_CRC
        num_correct++;
    }
    #if LOG_ENABLE_TWAI == 1

    ESP_LOGW(TAG, "Number error msg: %d, number correct msg: %d", num_err, num_correct);
    #endif
    #endif
    // vTaskDelete(NULL);
}
/* Using for log twai message */
void log_packet(twai_message_t rx_msg)
{
    #if LOG_ENABLE_TWAI == 1
    ESP_LOGW(TAG, "===============================================");
    ESP_LOGW(TAG, "From node ID: %d", rx_msg.data[0]);
    log_binary((uint16_t)rx_msg.identifier);
    ESP_LOGW(TAG, "Message length: %d", rx_msg.data[1]);
    ESP_LOGW(TAG, "Twai message receive: %s", (char*)rx_msg.data);
    ESP_LOGW(TAG, "===============================================");
    #endif
}

void twai_receive_task(void *arg)
{   
    /* Mqtt handler */
    MQTT_Handler_Struct* mqtt_h = (MQTT_Handler_Struct*) arg;
    /* Twai receive buffer for message multiple frame */
    twai_rx_msg* twai_rx_buf = pvPortMalloc(MAX_NODE_NUMBER*sizeof(twai_rx_msg));
    
    id_type_msg type_id;
    uint8_t node_transmit_id; 
    if (twai_rx_buf == NULL) 
    {
        vTaskDelete(NULL);
    }
    /* Init for buffer */
    memset(twai_rx_buf, 0, MAX_NODE_NUMBER*sizeof(twai_rx_msg));
    twai_message_t rx_msg = {0}; 

    while (1) 
    {
        twai_receive(&rx_msg, portMAX_DELAY);
        // log_packet(rx_msg);
        node_transmit_id = rx_msg.data[0];
        type_id = decode_id(rx_msg.identifier);
        if (type_id.frame_type == ID_END_FRAME && twai_rx_buf[node_transmit_id].current_buffer_len == 0)
        {   
            #ifdef CHECK_MSG_CRC
            if (rx_msg.data[rx_msg.data[1] - 1] != crc_8(&rx_msg.data[2], rx_msg.data[1] - 1))
            {
                num_err++;
            } 
            else 
            {
                rx_msg.data[rx_msg.data[1] - 1] = (uint8_t)'\0';
            #endif
                cJSON *root;
                root=cJSON_CreateObject();
                cJSON_AddNumberToObject(root, "node_id", node_transmit_id);
                cJSON_AddNumberToObject(root, "id_msg", type_id.msg_type);
                cJSON_AddNumberToObject(root, "id_target", type_id.target_type);
                cJSON_AddStringToObject(root, "msg", (char*)&rx_msg.data[2]);
                char *rendered=cJSON_Print(root);

                /* This frame is the single frame */
                // twai_to_mqtt_transmit(mqtt_h, rx_msg.data[0], rendered);
                cJSON_Delete(root);
                free(rendered);
                log_packet(rx_msg);
            #ifdef CHECK_MSG_CRC
                num_correct++;
            }
            #if LOG_ENABLE_TWAI == 1
            ESP_LOGW(TAG, "Number error msg: %d, number correct msg: %d", num_err, num_correct);
            #endif
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
            if (type_id.frame_type - 1 == twai_rx_buf[node_transmit_id].current_buffer_len || type_id.frame_type == ID_END_FRAME){
            twai_rx_buf[node_transmit_id].rx_buffer_msg[twai_rx_buf[node_transmit_id].current_buffer_len] = rx_msg;
            twai_rx_buf[node_transmit_id].current_buffer_len++;
            }
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
                // xTaskCreatePinnedToCore(twai_graft_packet_task, "TWAI_tx_single", 4096, &graft_buffer_rx_msg, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
                twai_graft_packet_task(&graft_buffer_rx_msg);
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
    #if LOG_ENABLE_TWAI == 1
    ESP_LOGW(TAG, "Message ID: %s", binary);
    #endif
}

void twai_transmit_multi_task(void * arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;
    /* Backup string message */
    char send_str_msg[100];
    uint8_t packet_size;
    // sprintf(send_str_msg, "%s", send_msg->msg); 
    memcpy(send_str_msg, send_msg->msg, send_msg->msg_len);
    int num_packets = (send_msg->msg_len - FIRST_PACKET_SIZE + NORMAL_PACKET_SIZE - 1) / NORMAL_PACKET_SIZE + 1;  
    int remaining_bytes = send_msg->msg_len - FIRST_PACKET_SIZE;
    #if LOG_ENABLE_TWAI == 1
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
        send_msg->msg = strndup(send_str_msg + ((i - 1) * NORMAL_PACKET_SIZE + 6), packet_size);
        send_msg->msg_len = packet_size;

        twai_transmit_single_for_multi(send_msg);

        remaining_bytes -= packet_size;
        #if LOG_ENABLE_TWAI == 1
        ESP_LOGI(TAG, "Twai transmited packet: %d.", i);
        #endif
    }
    // vTaskDelete(NULL);
}
/* Function that transmit single frame for multil-msg */ 
void twai_transmit_single_for_multi(void * arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;
    twai_message_t twai_tx_msg = {  .identifier = encode_id(send_msg->type_id), 
                                    .data_length_code = send_msg->msg_len + 1,
                                    .data = {NODE_ID, 0, 0, 0, 0, 0, 0, 0}};
    memcpy((char*)&twai_tx_msg.data[1], send_msg->msg, send_msg->msg_len);
    twai_transmit(&twai_tx_msg, pdMS_TO_TICKS(TWAI_TRANSMIT_WAIT));
    #if LOG_ENABLE_TWAI == 1

    ESP_LOGI(TAG, "Transmitted msg %d -%s", twai_tx_msg.data_length_code, (char*) twai_tx_msg.data);
    #endif
}
/* Function that transmit single frame*/ 
void twai_transmit_single(void * arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;

    twai_message_t twai_tx_msg = {  
        .identifier = encode_id(send_msg->type_id), 
        .data_length_code = send_msg->msg_len <= 6 ? send_msg->msg_len + 2 : 8,
        .data = {NODE_ID, send_msg->msg_len, 0, 0, 0, 0, 0, 0}};

    memcpy((char*)&twai_tx_msg.data[2], send_msg->msg, send_msg->msg_len <= 6 ? send_msg->msg_len : 6);

    twai_transmit(&twai_tx_msg, pdMS_TO_TICKS(TWAI_TRANSMIT_WAIT));
    #if LOG_ENABLE_TWAI == 1
    ESP_LOGI(TAG, "Transmitted msg %d - %s", twai_tx_msg.data_length_code, (char*) twai_tx_msg.data);
    #endif
}
void twai_transmit_msg(void* arg)
{
    twai_msg* send_msg = (twai_msg*) arg;
    #ifdef CHECK_MSG_CRC
    send_msg->msg[send_msg->msg_len] = (char)crc_8((uint8_t*)send_msg->msg, send_msg->msg_len);
    // ESP_LOGE(TAG, "CRC: %d", crc_8((uint8_t*)send_msg->msg, send_msg->msg_len));
    send_msg->msg_len++;
    #endif
    if(send_msg->msg_len > 6)
    {   /* Multi-msg */
        
        if(send_msg->msg_len > 54)
        {
            ESP_LOGE(TAG, "Msg too long to send!");
        }
        #if LOG_ENABLE_TWAI == 0
        ESP_LOGI(TAG, "============= Transmit Multi========================");
        #endif

        // xTaskCreatePinnedToCore(twai_transmit_multi_task, "TWAI_tx_multi", 4096, send_msg, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
        twai_transmit_multi_task(send_msg);
    }
    else 
    {   /* Single msg */
        ESP_LOGI(TAG, "================== Transmit Single===================");
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
    Twai_Handler_Struct* Twais = (Twai_Handler_Struct* )arg;

    char str[50];
    while (1) {
        // if (!gpio_get_level(BUTTON_PIN) || !gpio_get_level(BUTTON_PIN_1) || !gpio_get_level(BUTTON_PIN_2)){
        if (!gpio_get_level(BUTTON_PIN)){
            // sprintf(str, "#%d=%s;%f;%f;%f\r\n", 12, "Esp32", 147.11456446, 157.11456446, 1456.11456466);
            // sprintf(str, "#%d=%f\r\n", 10, 147.56464646);
            sprintf(str, "#%d=%f;%f\r\n", 11, 147.56464646, 157.11456446);
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
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        else
            vTaskDelay(pdMS_TO_TICKS(50));
    }
    twai_stop_uninstall(Twais);
    vTaskDelete(NULL);
}