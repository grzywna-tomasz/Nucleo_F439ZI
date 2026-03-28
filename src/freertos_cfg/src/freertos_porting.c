#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"

void xPortSysTickHandler(void);

void SysTick_Handler(void)
{
    HAL_IncTick();
    xPortSysTickHandler();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    while(1);
}