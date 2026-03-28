#include "app.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

extern ETH_HandleTypeDef heth;
volatile uint8_t dummy_wait = 1;

#define ETH_STACK_SIZE (256)
#define ETH_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
StackType_t EthStack[ETH_STACK_SIZE];
StaticTask_t EthTaskBuffer;
TaskHandle_t EthHandle = NULL;


void EthTask(void *pvParameters)
{
    MX_ETH_Init();
    HAL_ETH_Start(&heth);

    while(1)
    {
        uint8_t frame[60] = {
            // Destination MAC (broadcast)
            0xff,0xff,0xff,0xff,0xff,0xff,
            // Source MAC
            0x00,0x80,0xE1,0x00,0x00,0x00,
            // Ethertype (dummy)
            0x08,0x00,
            // Payload
            'H','E','L','L','O'
        };

        // --- TX config ---
        ETH_TxPacketConfigTypeDef TxConfig;
        ETH_BufferTypeDef TxBufferStruct;

        memset(&TxConfig, 0, sizeof(TxConfig));
        memset(&TxBufferStruct, 0, sizeof(TxBufferStruct));

        // Link buffer
        TxBufferStruct.buffer = frame;
        TxBufferStruct.len    = 60;
        TxBufferStruct.next   = NULL;

        // Configure packet
        TxConfig.Length   = 60;
        TxConfig.TxBuffer = &TxBufferStruct;

        // Optional but recommended
        TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
        TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;

        HAL_ETH_Transmit(&heth, &TxConfig, HAL_MAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

#define LED_STACK_SIZE (256)
#define LED_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
StackType_t LedStack[LED_STACK_SIZE];
StaticTask_t LedTaskBuffer;
TaskHandle_t LedHandle = NULL;

void LedTask(void *pvParameters)
{
    while(1)
    {
        HAL_GPIO_TogglePin(GPIOB, LD1_Pin);
        HAL_GPIO_TogglePin(GPIOB, LD2_Pin);
        HAL_GPIO_TogglePin(GPIOB, LD3_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void App_main(void)
{
    EthHandle = xTaskCreateStatic(EthTask, "EthTask", ETH_STACK_SIZE, (void *) 0, ETH_TASK_PRIORITY, EthStack, &EthTaskBuffer);
    LedHandle = xTaskCreateStatic(LedTask, "LedTask", LED_STACK_SIZE, (void *) 0, LED_TASK_PRIORITY, LedStack, &LedTaskBuffer);

    vTaskStartScheduler();

    while(1);
}

