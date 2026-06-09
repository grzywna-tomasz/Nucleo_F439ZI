#include "can.hpp"

extern "C" {
#include "lwip.h"
};

extern CanDriverInstance<5> CanDriver1;
extern CanDriverInstance<4> CanDriver2;
CanListener<5> Test_Listener;
CanListenerMinMax<5> Test_Listener2(0, 0x200);

static constexpr uint32_t STACK_SIZE {256U};
static constexpr uint32_t TASK_PRIORITY {tskIDLE_PRIORITY + 2};
StackType_t Stack[STACK_SIZE];
StaticTask_t TaskBuffer;
TaskHandle_t TestTaskHandle = NULL;

void task(void *pvParams)
{
    CanData_t msg;
    while(1)
    {
        CanData_t data {.id = 0x123, .data={0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}, .data_len = 8};

        Test_Listener.sendMessage(data);

        if (E_OK == Test_Listener.waitForMsg(msg, pdMS_TO_TICKS(1000)))
        {
            Lwip_SendUdp(reinterpret_cast<const char*>(msg.data), msg.data_len);
        }
        vTaskDelay(pdMS_TO_TICKS(1000u));

        uint8_t in_data = 0xFF;
        for (uint8_t index = 0; index < 5; index++)
        {
            data.data[index] = in_data;
            in_data -= 0x11;
        }

        data.data_len = 5;

        Test_Listener2.sendMessage(data);

        if (E_OK == Test_Listener2.waitForMsg(msg, pdMS_TO_TICKS(1000)))
        {
            Lwip_SendUdp(reinterpret_cast<const char*>(msg.data), msg.data_len);
        }
        vTaskDelay(pdMS_TO_TICKS(1000u));
    }
}

volatile uint32_t test_;

extern "C" void TestInit()
{
    Test_Listener.init(&CanDriver1);
    Test_Listener2.init(&CanDriver2);

    TestTaskHandle = xTaskCreateStatic(task, "task", STACK_SIZE, (void *) 0, TASK_PRIORITY, Stack, &TaskBuffer);
}