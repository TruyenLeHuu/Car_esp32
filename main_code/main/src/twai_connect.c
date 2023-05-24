#include "twai_connect.h"
#include "esp_log.h"
#include "sys_config.h"
#include <string.h>
static const char *TAG = "Twai connection";

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
void graft_packet(void *arg){
    twai_rx_msg* rx_msg = (twai_rx_msg*) arg;
    char str_msg[100] = "";
    uint8_t length_msg = rx_msg->rx_buffer_msg[0].data[1];
    int num_packets = (length_msg - FIRST_PACKET_SIZE + NORMAL_PACKET_SIZE - 1) / NORMAL_PACKET_SIZE + 1;  
    if (num_packets != rx_msg->graft_buffer_msg)
    {
        ESP_LOGE(TAG, "num_packets = %d not equal length_buffer_msg = %d!", num_packets, rx_msg->graft_buffer_msg);
        vTaskDelete(NULL);
    }
    uint8_t length_id;
    for (int i = 0; i < num_packets; i++)
    {       
        length_id = (i == 0 ? 2 : 1); 
        strcat(str_msg, (char*)rx_msg->rx_buffer_msg[i].data + length_id);
        ESP_LOGI(TAG, "Msg curren: %s.", str_msg);
    }
    ESP_LOGI(TAG, "Msg ID: %s.", convert_binary((uint16_t)rx_msg->rx_buffer_msg[0].identifier));
    ESP_LOGI(TAG, "Node ID: %d.", rx_msg->rx_buffer_msg[0].data[0]);
    ESP_LOGI(TAG, "Msg Length: %d.", rx_msg->rx_buffer_msg[0].data[1]);
    ESP_LOGI(TAG, "Twai receive msg: %s.", str_msg);
    vTaskDelay(pdMS_TO_TICKS(50));
    vTaskDelete(NULL);
}
void log_packet(twai_message_t rx_msg)
{
    ESP_LOGI(TAG, "Id: %s.", convert_binary((uint16_t)rx_msg.identifier));
    ESP_LOGI(TAG, "Node ID: %d.", rx_msg.data[0]);
    ESP_LOGI(TAG, "Msg Length: %d.", rx_msg.data[1]);
    ESP_LOGI(TAG, "Twai receive msg: %s.", (char*)rx_msg.data);
}
void twai_receive_task(void *arg)
{   
    twai_rx_msg* twai_rx_buf = (twai_rx_msg*)malloc(sizeof(twai_rx_msg));
    twai_message_t rx_msg = {.identifier = 0x000000, .data_length_code = 0, .data = {0, 0 , 0 , 0 ,0 ,0 ,0 ,0}}; 
    if (twai_rx_buf == NULL) {
        ESP_LOGE(TAG, "Can't malloc struct array.");
        vTaskDelete(NULL);
    }
    while (1) {
        twai_receive(&rx_msg, portMAX_DELAY);
        id_type_msg type_id = decode_id(rx_msg.identifier);
        uint8_t id_node_transmit = rx_msg.data[0];
        if (type_id.frame_type == ID_END_FRAME && twai_rx_buf[id_node_transmit].current_buffer_msg == 0)
        {   
            log_packet(rx_msg);
        }
        else 
        {   
            log_packet(rx_msg);
            if (type_id.frame_type == ID_FIRST_FRAME && twai_rx_buf[id_node_transmit].current_buffer_msg != 0) 
                twai_rx_buf[id_node_transmit].current_buffer_msg = 0;
            ESP_LOGI(TAG, "Add packet into buffer: %d.", id_node_transmit);
            twai_rx_buf[id_node_transmit].rx_buffer_msg[twai_rx_buf[id_node_transmit].current_buffer_msg] = rx_msg;
            twai_rx_buf[id_node_transmit].current_buffer_msg++;
            if (type_id.frame_type == ID_END_FRAME)
            {
                ESP_LOGI(TAG, "Start graft packet: %d.", id_node_transmit);
                twai_rx_msg graft_buffer_rx_msg = twai_rx_buf[id_node_transmit];
                twai_rx_buf[id_node_transmit].graft_buffer_msg = twai_rx_buf[id_node_transmit].current_buffer_msg;
                twai_rx_buf[id_node_transmit].current_buffer_msg = 0;
                xTaskCreatePinnedToCore(graft_packet, "TWAI_tx_single", 4096, &twai_rx_buf[id_node_transmit], RX_TASK_PRIO, NULL, tskNO_AFFINITY);
                
            }

        }
    }
    vTaskDelete(NULL);
}
char* convert_binary(uint16_t number) {
    int bits = sizeof(number) * 8;  // Số bit trong số nguyên

    // Cấp phát bộ nhớ cho chuỗi kết quả
    char* binary = (char*)malloc(bits + 1);
    binary[bits] = '\0';  // Kết thúc chuỗi

    // Chuyển đổi số nguyên thành chuỗi binary
    for (int i = bits - 1; i >= 0; i--) {
        binary[i] = (number & 1) ? '1' : '0';
        number >>= 1;
    }
    return binary;
}
void twai_transmit_multi(void * arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;
    char send_str_msg[50]; 
    strcpy(send_str_msg, send_msg->msg);
    int num_packets = (send_msg->msg_len - FIRST_PACKET_SIZE + NORMAL_PACKET_SIZE - 1) / NORMAL_PACKET_SIZE + 1;  
    int remaining_bytes = send_msg->msg_len - FIRST_PACKET_SIZE;

    send_msg->type_id.frame_type = ID_FIRST_FRAME;
    ESP_LOGI(TAG, "Twai transmited packet: %d.", 0);
    twai_transmit_single(send_msg);
    for (int i = 1; i < num_packets; i++) {
        ESP_LOGI(TAG, "Twai transmited packet: %d.", i);
        if (i == num_packets - 1) 
        {
            send_msg->type_id.frame_type = ID_END_FRAME;
        }
        else
        {
            send_msg->type_id.frame_type = i + 1;
        }
        uint8_t packet_size = (remaining_bytes >= NORMAL_PACKET_SIZE) ? NORMAL_PACKET_SIZE : remaining_bytes;
        send_msg->msg = strndup(send_str_msg + ((i - 1) * NORMAL_PACKET_SIZE + 6), packet_size);
        send_msg->msg_len = packet_size;
        twai_transmit_single_for_multi(send_msg);
        remaining_bytes -= packet_size;
    }
    vTaskDelete(NULL);
}
void twai_transmit_single_for_multi(void * arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;
    twai_message_t twai_tx_msg = {  .identifier = encode_id(send_msg->type_id), 
                                    .data_length_code = send_msg->msg_len + 1,
                                    .data = {NODE_ID, 0, 0, 0, 0, 0, 0, 0}};
    memcpy((char*)&twai_tx_msg.data[1], send_msg->msg, send_msg->msg_len);
    twai_transmit(&twai_tx_msg, portMAX_DELAY);
    ESP_LOGI(TAG, "Transmitted msg %d - %s", twai_tx_msg.data_length_code, (char*) twai_tx_msg.data);
}
void twai_transmit_single(void * arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;
    twai_message_t twai_tx_msg = {  .identifier = encode_id(send_msg->type_id), 
                                    .data_length_code = send_msg->msg_len <= 6 ? send_msg->msg_len + 2 : 8,
                                    .data = {NODE_ID, send_msg->msg_len, 0, 0, 0, 0, 0, 0}};
    memcpy((char*)&twai_tx_msg.data[2], send_msg->msg, send_msg->msg_len <= 6 ? send_msg->msg_len : 6);
    twai_transmit(&twai_tx_msg, portMAX_DELAY);
    ESP_LOGI(TAG, "Transmitted msg %d - %s", twai_tx_msg.data_length_code, (char*) twai_tx_msg.data);
}
void twai_transmit_msg(id_type_msg _type_id, uint8_t _msg_len, char* _str_msg)
{   
    twai_msg send_msg = {
        .type_id = _type_id,
        .msg_len = _msg_len,
        .msg = _str_msg
    };
    if(_msg_len > 55)
    {
        ESP_LOGE(TAG, "Msg too long to send!");
    }
    if(_msg_len > 6)
    {   
        xTaskCreatePinnedToCore(twai_transmit_multi, "TWAI_tx_single", 4096, &send_msg, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    }
    else 
    {
        send_msg.type_id.frame_type = ID_END_FRAME;
        xTaskCreatePinnedToCore(twai_transmit_single, "TWAI_tx_single", 4096, &send_msg, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
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

void twai_transmit_task(void *arg)
{
    Twai_Handler_Struct* Twais = (Twai_Handler_Struct* )arg;
    while (1) {
        if (!gpio_get_level(BUTTON_PIN)){
            id_type_msg type_id = {
                        .msg_type = ID_MSG_TYPE_CMD_FRAME,
                        .target_type = ID_TARGET_ALL_NODE,
                        };
            twai_transmit_msg(type_id, strlen("Alo con ga kho!!"), "Alo con ga kho!!");
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        else
            vTaskDelay(pdMS_TO_TICKS(50));
    }
    // vTaskDelay(pdMS_TO_TICKS(10000));
    twai_stop_uninstall(Twais);
    vTaskDelete(NULL);
}