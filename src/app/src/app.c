#include "app.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "std_types.h"
#include "lwip.h"

#define DESTINATION_IP_BYTE_1       (198U)
#define DESTINATION_IP_BYTE_2       (19U)
#define DESTINATION_IP_BYTE_3       (60U)
#define DESTINATION_IP_BYTE_4       (1U)
#define DEST_PORT                   (1234U)

volatile uint8_t dummy_wait = 1;

#define ETH_STACK_SIZE (256)
#define ETH_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
StackType_t Eth_Stack[ETH_STACK_SIZE];
StaticTask_t Eth_TaskBuffer;
TaskHandle_t Eth_Handle = NULL;

#define LED_STACK_SIZE (256)
#define LED_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
StackType_t Led_Stack[LED_STACK_SIZE];
StaticTask_t Led_TaskBuffer;
TaskHandle_t Led_Handle = NULL;

#define LWIP_STACK_SIZE (256)
#define LWIP_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
StackType_t Lwip_Stack[LWIP_STACK_SIZE];
StaticTask_t Lwip_TaskBuffer;
TaskHandle_t Lwip_Handle = NULL;

void EthTask(void *pvParameters)
{
    /* Delay to let lwip perform initialization */
    vTaskDelay(pdMS_TO_TICKS(1000));

    ip_addr_t dest_ip;
    IP4_ADDR(&dest_ip, DESTINATION_IP_BYTE_1, DESTINATION_IP_BYTE_2, DESTINATION_IP_BYTE_3, DESTINATION_IP_BYTE_4);

    Lwip_CreateUdpConnection(dest_ip, DEST_PORT);

    while(1)
    {
        char msg[] = "Hello from STM32";
        if (ERR_OK != Lwip_SendUdp(msg, sizeof(msg)))
        {
            while(1);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void LedTask(void *pvParameters)
{
    while(1)
    {

        HAL_GPIO_TogglePin(GPIOB, LD1_Pin);
        HAL_GPIO_TogglePin(GPIOB, LD2_Pin);
        HAL_GPIO_TogglePin(GPIOB, LD3_Pin);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void App_main(void)
{
    Lwip_Init();

    Eth_Handle = xTaskCreateStatic(EthTask, "EthTask", ETH_STACK_SIZE, (void *) 0, ETH_TASK_PRIORITY, Eth_Stack, &Eth_TaskBuffer);
    Led_Handle = xTaskCreateStatic(LedTask, "LedTask", LED_STACK_SIZE, (void *) 0, LED_TASK_PRIORITY, Led_Stack, &Led_TaskBuffer);
    Lwip_Handle = xTaskCreateStatic(Lwip_Task, "LwipTask", LWIP_STACK_SIZE, (void *) 0, LWIP_TASK_PRIORITY, Lwip_Stack, &Lwip_TaskBuffer);

    vTaskStartScheduler();

    while(1);
}
