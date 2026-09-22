#include "vesc.hpp"

extern "C" {
#include "can.h"
#include "lwip.h"
#include "can_open.h"
#include "FreeRTOS.h"
#include "task.h"
};

static constexpr uint32_t STACK_SIZE {256U};
static constexpr uint32_t TASK_PRIORITY {tskIDLE_PRIORITY + 2};
StackType_t Stack[STACK_SIZE];
StaticTask_t TaskBuffer;
TaskHandle_t TestTaskHandle = NULL;


volatile uint32_t position_value;

void task(void *pvParams)
{
    vTaskDelay(pdMS_TO_TICKS(3000u));
    // CanData_t msg;
    while(1)
    {
        // CanData_t data {.id = 0x123, .data={0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}, .data_len = 8};

        // Test_Listener.sendMessage(data);

        // if (E_OK == Test_Listener.waitForMsg(msg, pdMS_TO_TICKS(1000)))
        // if (E_OK == Test_Listener.waitForMsg(msg, portMAX_DELAY))
        // {
        //     Lwip_SendUdp(reinterpret_cast<const char*>(msg.data), msg.data_len);
        // }
        // vTaskDelay(pdMS_TO_TICKS(1000u));

        // uint8_t in_data = 0xFF;
        // for (uint8_t index = 0; index < 5; index++)
        // {
        //     data.data[index] = in_data;
        //     in_data -= 0x11;
        // }

        // data.data_len = 5;

        // Test_Listener2.sendMessage(data);

        // if (E_OK == Test_Listener2.waitForMsg(msg, pdMS_TO_TICKS(1000)))
        // {
        //     Lwip_SendUdp(reinterpret_cast<const char*>(msg.data), msg.data_len);
        // }

        /* This reads current value */
        
        uint8_t unused;
        CANOpen_ReadSDO(0x01, 0x6004, 0, const_cast<uint8_t *>(reinterpret_cast<volatile uint8_t *>(&position_value)), 4, &unused);
        vTaskDelay(pdMS_TO_TICKS(1u));
    }
}

volatile uint32_t test_;
extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

extern "C" void TestInit()
{

    TestTaskHandle = xTaskCreateStatic(task, "task", STACK_SIZE, (void *) 0, TASK_PRIORITY, Stack, &TaskBuffer);
    Vesc_Init();
    CANOpen_Init();
}