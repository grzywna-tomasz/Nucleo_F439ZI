#include "can.hpp"

extern "C" {
#include "lwip.h"
};

extern CanDriverInstance CanDriver;
CanListener Test_Listener;

static constexpr uint32_t STACK_SIZE {256U};
static constexpr uint32_t TASK_PRIORITY {tskIDLE_PRIORITY + 2};
StackType_t Stack[STACK_SIZE];
StaticTask_t TaskBuffer;
TaskHandle_t TaskHandle = NULL;

void task(void *pvParams)
{
    CanData_t msg;
    while(1)
    {
        if (E_OK == Test_Listener.waitForMsg(msg))
        {
            Lwip_SendUdp(reinterpret_cast<const char*>(msg.data), msg.data_len);
        }
    }
}

extern "C" void TestInit()
{
    Test_Listener.init(0, 0x200, &CanDriver, &TaskHandle);

    TaskHandle = xTaskCreateStatic(task, "task", STACK_SIZE, (void *) 0, TASK_PRIORITY, Stack, &TaskBuffer);
}