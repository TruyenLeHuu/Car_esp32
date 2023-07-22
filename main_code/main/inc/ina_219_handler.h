#ifndef __INA_219_HANDLER_H__
#define __INA_219_HANDLER_H__

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ina_219.h>
#include <string.h>
#include <esp_log.h>
#include <math.h>

typedef struct INA_Handler_Struct{   
    ina219_t* ina_dev;
} INA_Handler_Struct;

void init_ina(void*);
void read_power_task(void*);

#endif //__INA_219_HANDLER_H__