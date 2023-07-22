#include "ina_219_handler.h"
#include "sys_config.h"

const static char *TAG = "INA219";

void init_ina(void * arg)
{
    INA_Handler_Struct* ina_handler = (INA_Handler_Struct*) arg;
    ina219_t dev;
    memset(&dev, 0, sizeof(ina219_t));

    ESP_ERROR_CHECK(ina219_init_desc(&dev, I2C_ADDR, I2C_PORT, SDA_GPIO, SCL_GPIO));
    ESP_LOGI(TAG, "Initializing INA219");
    ESP_ERROR_CHECK(ina219_init(&dev));

    ESP_LOGI(TAG, "Configuring INA219");


    ESP_LOGI(TAG, "Calibrating INA219");
    ESP_ERROR_CHECK(ina219_calibrate(&dev, 40.0, 0.002)); // max current,  Ohm shunt resistance

    ina_handler->ina_dev = &dev;
}
void read_power_task(void * arg)
{
    INA_Handler_Struct* ina_handler = (INA_Handler_Struct*) arg;

    float bus_voltage, current, power;

    ESP_LOGI(TAG, "Starting the loop");
    while (1)
    {
        ESP_ERROR_CHECK(ina219_getVCP(ina_handler->ina_dev, &bus_voltage, &current, &power));

        printf("%f %f %f %f\n",esp_timer_get_time()/1000000.0,bus_voltage/1000.0,current,power);

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}