#include "twai_connect.h"
#include "mqtt_connect.h"   
#include <string.h>
#include "esp_log.h"
#include "sys_config.h"
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
void twai_to_mqtt_transmit(void *arg)
{

}
void twai_graft_packet_task(void *arg)
{
    twai_rx_msg* rx_msg = (twai_rx_msg*) arg;
    char str_msg[100] = "";
    uint8_t length_msg = rx_msg->rx_buffer_msg[0].data[1];
    int num_packets = (length_msg - FIRST_PACKET_SIZE + NORMAL_PACKET_SIZE - 1) / NORMAL_PACKET_SIZE + 1;  
    if (num_packets != rx_msg->graft_buffer_msg)
    {
        ESP_LOGE(TAG, "num_packets = %d not equal rx_msg->graft_buffer_msg = %d!", num_packets, rx_msg->graft_buffer_msg);
    }
    uint8_t length_id;
    for (int i = 0; i < num_packets; i++)
    {       
        length_id = (i == 0 ? 2 : 1); 
        strcat(str_msg, (char*)rx_msg->rx_buffer_msg[i].data + length_id);
        #ifdef DEBUG
        ESP_LOGW(TAG, "Current message: %s.", str_msg);
        #endif
    }
    ESP_LOGW(TAG, "From node ID: %d.", rx_msg->rx_buffer_msg[0].data[0]);
    log_binary((uint16_t)rx_msg->rx_buffer_msg[0].identifier);
    ESP_LOGW(TAG, "Message length: %d.", rx_msg->rx_buffer_msg[0].data[1]);
    ESP_LOGW(TAG, "Twai message receive: %s.", str_msg);
    vTaskDelete(NULL);
}
void log_packet(twai_message_t rx_msg)
{
    ESP_LOGW(TAG, "===============================================");
    ESP_LOGW(TAG, "From node ID: %d.", rx_msg.data[0]);
    log_binary((uint16_t)rx_msg.identifier);
    ESP_LOGW(TAG, "Message length: %d.", rx_msg.data[1]);
    ESP_LOGW(TAG, "Twai message receive: %s.", (char*)rx_msg.data);
    ESP_LOGW(TAG, "===============================================");
}
void twai_receive_task(void *arg)
{   
    MQTT_Handler_Struct* mqtt_s = (MQTT_Handler_Struct*) arg;
    twai_rx_msg* twai_rx_buf = (twai_rx_msg*)malloc(sizeof(twai_rx_msg));
    twai_message_t rx_msg = {.identifier = 0x0000, .data_length_code = 0, .data = {0, 0 , 0 , 0 ,0 ,0 ,0 ,0}}; 
    for(int i = 0; i < MAX_NODE_NUMBER; i++){
        twai_rx_buf[i].current_buffer_msg = 0;
        twai_rx_buf[i].graft_buffer_msg = 0;
    }
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
            #ifdef DEBUG
            log_packet(rx_msg);
            #endif
            if (type_id.frame_type == ID_FIRST_FRAME && twai_rx_buf[id_node_transmit].current_buffer_msg != 0) 
                twai_rx_buf[id_node_transmit].current_buffer_msg = 0;
            #ifdef DEBUG
            ESP_LOGW(TAG, "Add packet into buffer, from node: %d.", id_node_transmit);
            #endif
            twai_rx_buf[id_node_transmit].rx_buffer_msg[twai_rx_buf[id_node_transmit].current_buffer_msg] = rx_msg;
            twai_rx_buf[id_node_transmit].current_buffer_msg++;
            if (type_id.frame_type == ID_END_FRAME)
            {
                #ifdef DEBUG
                ESP_LOGW(TAG, "Start graft packet: %d.", id_node_transmit);
                #endif
                twai_rx_buf[id_node_transmit].graft_buffer_msg = twai_rx_buf[id_node_transmit].current_buffer_msg;
                twai_rx_msg graft_buffer_rx_msg = twai_rx_buf[id_node_transmit];
                graft_buffer_rx_msg.mqtt_client = mqtt_s->client;
                twai_rx_buf[id_node_transmit].current_buffer_msg = 0;
                xTaskCreatePinnedToCore(twai_graft_packet_task, "TWAI_tx_single", 4096, &graft_buffer_rx_msg, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
                
            }

        }
    }
    vTaskDelete(NULL);
}
void log_binary(uint16_t number) {
    int bits = sizeof(number) * 8;  // Số bit trong số nguyên

    // Cấp phát bộ nhớ cho chuỗi kết quả
    char binary[16*8+1];
    binary[bits] = '\0';  // Kết thúc chuỗi

    // Chuyển đổi số nguyên thành chuỗi binary
    for (int i = bits - 1; i >= 0; i--) {
        binary[i] = (number & 1) ? '1' : '0';
        number >>= 1;
    }
    ESP_LOGW(TAG, "Message ID: %s.", binary);
}
void twai_transmit_multi_task(void * arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;
    
    char send_str_msg[100];
    sprintf(send_str_msg, "%s", send_msg->msg); 
    // strcpy(send_str_msg, send_msg->msg);

    int num_packets = (send_msg->msg_len - FIRST_PACKET_SIZE + NORMAL_PACKET_SIZE - 1) / NORMAL_PACKET_SIZE + 1;  
    int remaining_bytes = send_msg->msg_len - FIRST_PACKET_SIZE;
    #ifdef DEBUG
    ESP_LOGI(TAG, "Twai transmited packet: %d.", 0);
    #endif
    send_msg->type_id.frame_type = ID_FIRST_FRAME;
    twai_transmit_single(send_msg);
    for (int i = 1; i < num_packets; i++) {
        #ifdef DEBUG
        ESP_LOGI(TAG, "Twai transmited packet: %d.", i);
        #endif
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
    twai_transmit(&twai_tx_msg, pdMS_TO_TICKS(TWAI_TRANSMIT_WAIT));
    ESP_LOGI(TAG, "Transmitted msg %d - %s", twai_tx_msg.data_length_code, (char*) twai_tx_msg.data);
}
void twai_transmit_single(void * arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;
    twai_message_t twai_tx_msg = {  .identifier = encode_id(send_msg->type_id), 
                                    .data_length_code = send_msg->msg_len <= 6 ? send_msg->msg_len + 2 : 8,
                                    .data = {NODE_ID, send_msg->msg_len, 0, 0, 0, 0, 0, 0}};
    memcpy((char*)&twai_tx_msg.data[2], send_msg->msg, send_msg->msg_len <= 6 ? send_msg->msg_len : 6);
    twai_transmit(&twai_tx_msg, pdMS_TO_TICKS(TWAI_TRANSMIT_WAIT));
    ESP_LOGI(TAG, "Transmitted msg %d - %s", twai_tx_msg.data_length_code, (char*) twai_tx_msg.data);
}
void twai_transmit_msg(void* arg)
{   
    twai_msg* send_msg = (twai_msg*) arg;
    if(send_msg->msg_len > 6)
    {   
        if(send_msg->msg_len > 55)
        {
            ESP_LOGE(TAG, "Msg too long to send!");
        }
        xTaskCreatePinnedToCore(twai_transmit_multi_task, "TWAI_tx_multi", 4096, send_msg, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
    }
    else 
    {
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

void twai_transmit_task(void *arg)
{
    Twai_Handler_Struct* Twais = (Twai_Handler_Struct* )arg;
    while (1) {
        if (!gpio_get_level(BUTTON_PIN)){
            twai_msg send_msg = {
                .type_id = {
                            .msg_type = ID_MSG_TYPE_CMD_FRAME,
                            .target_type = ID_TARGET_ALL_NODE,
                            },
                .msg = "Alo con ga kho!!",
                .msg_len = strlen("Alo con ga kho!!"),
            };
            twai_transmit_msg(&send_msg);
            #ifdef DEBUG
            vTaskDelay(pdMS_TO_TICKS(200));
            #endif
            vTaskDelay(pdMS_TO_TICKS(100));
           
        }
        else
            vTaskDelay(pdMS_TO_TICKS(50));
    }
    // vTaskDelay(pdMS_TO_TICKS(10000));
    twai_stop_uninstall(Twais);
    vTaskDelete(NULL);
}