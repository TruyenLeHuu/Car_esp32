// #include <stdio.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <ina219.h>
// #include <string.h>
// #include <esp_log.h>
// #include "driver/uart.h"
// #include <math.h>

// #define I2C_PORT 0
// #define I2C_ADDR INA219_ADDR_GND_GND
// #define SDA_GPIO 21
// #define SCL_GPIO 22

// const static char *TAG = "INA219";

// void task(void *pvParameters)
// {
//     ina219_t dev;
//     memset(&dev, 0, sizeof(ina219_t));

//     ESP_ERROR_CHECK(ina219_init_desc(&dev, I2C_ADDR, I2C_PORT, SDA_GPIO, SCL_GPIO));
//     ESP_LOGI(TAG, "Initializing INA219");
//     ESP_ERROR_CHECK(ina219_init(&dev));

//     ESP_LOGI(TAG, "Configuring INA219");
//     //ESP_ERROR_CHECK(ina219_reset(&dev));
//     //ESP_ERROR_CHECK(ina219_configure(&dev, INA219_BUS_RANGE_32V, INA219_GAIN_0_125,
//       //     INA219_RES_12BIT_1S, INA219_RES_12BIT_1S, INA219_MODE_CONT_SHUNT_BUS));

//     ESP_LOGI(TAG, "Calibrating INA219");
//     ESP_ERROR_CHECK(ina219_calibrate(&dev, 40.0, 0.002)); // max current,  Ohm shunt resistance

//     float bus_voltage, shunt_voltage, current, power;

//     ESP_LOGI(TAG, "Starting the loop");
//     while (1)
//     {
//         //long t = esp_timer_get_time();

//         //ESP_ERROR_CHECK(ina219_get_bus_voltage(&dev, &bus_voltage));
//         //ESP_ERROR_CHECK(ina219_get_current(&dev, &current));
//         //ESP_ERROR_CHECK(ina219_get_power(&dev, &power));

//         ESP_ERROR_CHECK(ina219_getVCP(&dev, &bus_voltage,&current, &power));

//         //t = esp_timer_get_time() - t;    

//         /* Using float in printf() requires non-default configuration in
//          * sdkconfig. See sdkconfig.defaults.esp32 and
//          * sdkconfig.defaults.esp8266  */
//         //ESP_ERROR_CHECK(ina219_get_shunt_voltage(&dev, &shunt_voltage));
//         //printf("VBUS: %.04f mV, VSHUNT: %.04f mV, IBUS: %4.04f mA , PBUS: %.04f mW, time: %ld\n",
//         //        bus_voltage, shunt_voltage *1, current * 1 , power * 1000, t); 

//         /*Raw : */
//         //printf("%ld  ,%f,%f,%f \n",t , bus_voltage,current,power);

//         /*Metric : */
//         printf("%f %f %f %f\n",esp_timer_get_time()/1000000.0,bus_voltage/1000.0,current,power);


//         vTaskDelay(100 / portTICK_PERIOD_MS);

//     }
// }

// void app_main()
// {
//     ESP_ERROR_CHECK(i2cdev_init());
//     xTaskCreate(task, "test", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
// }
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO    22       // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO    21       // GPIO number for I2C master data
#define I2C_MASTER_NUM       I2C_NUM_0 // I2C port number for master dev
#define I2C_MASTER_FREQ_HZ   100000   // I2C master clock frequency
#define INA219_I2C_ADDR      0x40     // I2C address of INA219

void i2c_master_init()
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

uint16_t read_register(uint8_t reg)
{
    uint8_t data[2];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (INA219_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);  // Configuring the register address
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (INA219_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &data[0], I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &data[1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    i2c_cmd_link_delete(cmd);

    uint16_t value = (data[0] << 8) | data[1];
    return value;
}

float read_shunt_voltage()
{
    uint16_t value = read_register(0x01);  // Shunt Voltage Register (0x01)
    return (float)value * 0.01;  // Assuming LSB value is 10μV
}

float read_bus_voltage()
{
    uint16_t value = read_register(0x02);  // Bus Voltage Register (0x02)
    value >>= 3;  // Shift out the lower 3 bits
    return (float)value * 0.004;  // Assuming LSB value is 4mV
}

float read_current()
{
    uint16_t value = read_register(0x04);  // Current Register (0x04)
    return (float)value * 0.1;  // Assuming LSB value is 100μA
}

void app_main()
{
    i2c_master_init();

    while (1)
    {
        float shunt_voltage = read_shunt_voltage();
        float bus_voltage = read_bus_voltage();
        float current = read_current();

        printf("Shunt Voltage: %.2f mV\n", shunt_voltage);
        printf("Bus Voltage: %.2f V\n", bus_voltage);
        printf("Current: %.2f mA\n", current);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}