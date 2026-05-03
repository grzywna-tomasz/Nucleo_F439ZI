#include <vector>
#include <cstring>
#include "can.hpp"

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32f4xx_hal_can.h"
}

static void Can_RxMsgDispatcherWrapper(CAN_HandleTypeDef *hcan);

CanDriverInstance CanDriver;
CanListener CanListener_;

extern "C" {
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // Can_RxMsgDispatcherWrapper(hcan);
    CanDriver.rxMsgDispatcher(hcan);
}

void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan);
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan);
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan);
}

// /* TODO as a excercise - try to replace the structure with template to check out if this is possible to make this configurable (through the constructor) */
// struct CanDriverInstance::freeRTOSData {
//     static constexpr uint32_t CAN_STACK_SIZE {256U};
//     static constexpr uint32_t CAN_TASK_PRIORITY {tskIDLE_PRIORITY + 2};
//     StackType_t Can_Stack[CAN_STACK_SIZE];
//     StaticTask_t Can_TaskBuffer;
//     TaskHandle_t Can_TaskHandle = NULL;
    
//     static constexpr uint32_t CAN_TX_QUEUE_LENGTH {5U};
//     static constexpr uint32_t CAN_TX_QUEUE_DATA_SIZE {sizeof(CanData_t)};
//     static StaticQueue_t Can_TxQueue;
//     static uint8_t Can_TxQueueStorageArea[CAN_TX_QUEUE_LENGTH * CAN_TX_QUEUE_DATA_SIZE];
//     static QueueHandle_t Can_QueueHandle;
// };

// static std::vector<CanListener *> Can_Listeners;



static void Can_RxMsgDispatcherWrapper(CAN_HandleTypeDef *hcan)
{
    CanDriver.rxMsgDispatcher(hcan);
    // CAN_RxHeaderTypeDef header;
    // uint8_t data[8];

    // HAL_CAN_GetRxMessage(
    //     hcan,
    //     CAN_RX_FIFO0,
    //     &header,
    //     data
    // );

    // for (CanListener* listener : Can_Listeners)
    // {
    //     if (listener->msgForThisListener(header.StdId))
    //     {
    //         listener->storeMsgFromIsr(header.StdId, data, header.DLC);
    //         break;
    //     }
    // }
}

Std_ReturnType CanListener::init(uint16_t min_id, uint16_t max_id, CanDriverInstance* driver_instance)
{
    m_minId = min_id;
    m_maxId = max_id;
    return driver_instance->addListener(this);
}

bool CanListener::msgForThisListener(uint16_t id) const
{
    if ((m_minId <= id) && (m_maxId >= id))
    {
        return true;
    }
    return false;
}

void CanListener::storeMsgFromIsr(uint16_t id, const uint8_t *data, uint8_t data_len) const
{
    if ((m_data) && (data) && (data_len <= CAN_DATA_SIZE))
    {
        memcpy(m_data->data, data, data_len);
        m_data->id = id;
        m_data->data_len = data_len;
        /* TODO task notification */
    }
    else
    {
        /* todo add det here */
    }
}

Std_ReturnType CanListener::waitForMsg(CanData_t& msg)
{
    m_data = &msg;
    /* TODO add here waiting for the task */
}

// CanDriverInstance::CanDriverInstance(uint8_t max_listeners)
//     :m_can_listeners(3)
// {

// }

void CanDriverInstance::taskWrapper(void *pvParameters)
{
    CanDriverInstance *self = static_cast<CanDriverInstance *>(pvParameters);
    self->task();
}

void CanDriverInstance::task()
{
    while(1)
    {
        /* TODO handle sending from queue here */
        vTaskDelay(pdMS_TO_TICKS(1000u));
    }
}

Std_ReturnType CanDriverInstance::init()
{
    Std_ReturnType ret_val = E_NOT_OK;

    m_Can_TaskHandle = xTaskCreateStatic(taskWrapper, "CanTask", m_CAN_STACK_SIZE, (void *) 0, m_CAN_TASK_PRIORITY, m_Can_Stack, &m_Can_TaskBuffer);
    m_Can_QueueHandle = xQueueCreateStatic(m_CAN_TX_QUEUE_LENGTH, m_CAN_TX_QUEUE_DATA_SIZE, m_Can_TxQueueStorageArea, &m_Can_TxQueue);

    if ((m_Can_QueueHandle) && (m_Can_TaskHandle))
    {
        ret_val = E_OK;
    }

    return ret_val;
}

Std_ReturnType CanDriverInstance::addListener(CanListener *input_listener)
{
    Std_ReturnType ret_val = E_NOT_OK;

    for (CanListener * listener : m_can_listeners)
    {
        if (listener)
        {
            listener = input_listener;
            ret_val = E_OK;
        }
    }

    return ret_val;
}

void CanDriverInstance::rxMsgDispatcher(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef header;
    uint8_t data[8];

    HAL_CAN_GetRxMessage(
        hcan,
        CAN_RX_FIFO0,
        &header,
        data
    );

    for (CanListener* listener : m_can_listeners)
    {
        if ((listener) && (listener->msgForThisListener(header.StdId)))
        {
            listener->storeMsgFromIsr(header.StdId, data, header.DLC);
            break;
        }
    }
}

/* TODO - this is only temporary solution untill app is moved to be fully cpp */
extern "C" void CanClassInit(void)
{
    CanDriver.init();
    CanListener_.init(0, 0x100, &CanDriver);
}