#include <vector>
#include <cstring>
#include "can.hpp"
#include "std_utils.h"

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32f4xx_hal_can.h"
#include "det.h"
extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
}

static constexpr uint32_t CAN_STACK_SIZE {256U};
static constexpr uint32_t CAN_TASK_PRIORITY {tskIDLE_PRIORITY + 2};
StackType_t Can_Stack[CAN_STACK_SIZE];
StaticTask_t Can_TaskBuffer;
TaskHandle_t Can_TaskHandle = NULL;

CanDriverInstance<5> CanDriver1 {3, hcan1};
CanDriverInstance<4> CanDriver2 {3, hcan2};

uint8_t Can_GetUniqueIdOfInstance(CAN_HandleTypeDef *hcan)
{
    uint8_t can_driver_id = 0xFF;

    if (hcan->Instance == CanDriver1.getHalHandle().Instance)
    {
        can_driver_id = CanDriver1.getUniqueId();
    }
    else if (hcan->Instance == CanDriver2.getHalHandle().Instance)
    {
        can_driver_id = CanDriver2.getUniqueId();
    }

    return can_driver_id;
}

extern "C" {
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CanDriver1.getHalHandle().Instance)
    {
        CanDriver1.rxMsgDispatcher(hcan);
    }
    else if (hcan->Instance == CanDriver2.getHalHandle().Instance)
    {
        CanDriver2.rxMsgDispatcher(hcan);
    }
}

void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan)
{
    uint8_t can_driver_id = Can_GetUniqueIdOfInstance(hcan);

    Det_ErrorWithData(DET_CAN_RX_FIFO_FULL_CALLBACK, DET_MULTIPLE_TIME_REPORT_ERROR, &can_driver_id, sizeof(can_driver_id));
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    /* Trigger transmission if there are more messages in queue */
    xTaskNotifyGive(Can_TaskHandle);
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    CanErrorCallback_t buffer = {.can_driver_id = Can_GetUniqueIdOfInstance(hcan), hcan->ErrorCode};

    Det_ErrorWithData(DET_CAN_ERROR_CALLBACK, DET_MULTIPLE_TIME_REPORT_ERROR, reinterpret_cast<uint8_t*>(&buffer), sizeof(buffer));
}
}

void Can_Task(void *pvParams)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        CanDriver1.sendFromQueue();
        CanDriver2.sendFromQueue();
    }
}

extern "C" Std_ReturnType Can_Init(void)
{
    Std_ReturnType ret_val = E_OK;
    Can_TaskHandle = xTaskCreateStatic(Can_Task, "CanTask", CAN_STACK_SIZE, (void *) 0, CAN_TASK_PRIORITY, Can_Stack, &Can_TaskBuffer);

    if (NULL == Can_TaskHandle)
    {
        CanFailedToCreateReason buffer = TaskHandle;
        Det_ErrorWithData(DET_CAN_FAILED_TO_CREATE, DET_MULTIPLE_TIME_REPORT_ERROR, reinterpret_cast<uint8_t*>(&buffer), sizeof(buffer));
        ret_val = E_NOT_OK;
    }

    ret_val |= CanDriver1.init();
    ret_val |= CanDriver2.init();
    
    return ret_val;
}

uint8_t ICanDriverInstance::m_CurrentId = 0;

ICanDriverInstance::ICanDriverInstance(uint8_t max_listeners, CAN_HandleTypeDef& hcan, uint8_t* tx_queue_storage_area, uint32_t tx_queue_size)
    :m_UniqueId {m_CurrentId++}, m_listeners(max_listeners, nullptr), m_TxQueueStorageArea {tx_queue_storage_area}, m_TxQueueStorageAreaSize {tx_queue_size}, m_hcan {hcan}
{
}

Std_ReturnType ICanDriverInstance::init()
{
    Std_ReturnType ret_val = E_NOT_OK;

    CAN_FilterTypeDef filter = {
        .FilterIdHigh = 0,
        .FilterIdLow = 0,
        .FilterMaskIdHigh = 0,
        .FilterMaskIdLow = 0,
        .FilterFIFOAssignment = CAN_FILTER_FIFO0,
        .FilterBank = 0,
        .FilterMode = CAN_FILTERMODE_IDMASK,
        .FilterScale = CAN_FILTERSCALE_32BIT,
        .FilterActivation = ENABLE,
    };

    HAL_CAN_ActivateNotification(&m_hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

    HAL_CAN_ConfigFilter(&m_hcan, &filter);
    HAL_CAN_Start(&m_hcan);

    m_TxQueueHandle = xQueueCreateStatic(m_TxQueueStorageAreaSize/sizeof(CanData_t), sizeof(CanData_t), m_TxQueueStorageArea, &m_TxQueue);

    if (m_TxQueueHandle)
    {
        ret_val = E_OK;
    }

    if (E_NOT_OK == ret_val)
    {
        CanFailedToCreateReason buffer = static_cast<CanFailedToCreateReason>(Can1 + getUniqueId());
        Det_ErrorWithData(DET_CAN_FAILED_TO_CREATE, DET_MULTIPLE_TIME_REPORT_ERROR, reinterpret_cast<uint8_t*>(&buffer), sizeof(buffer));
    }

    return ret_val;
}

Std_ReturnType ICanDriverInstance::addListener(ICanListener *input_listener)
{
    Std_ReturnType ret_val = E_NOT_OK;

    for (ICanListener *& listener : m_listeners)
    {
        if (!listener)
        {
            listener = input_listener;
            ret_val = E_OK;
            break;
        }
    }

    return ret_val;
}

void ICanDriverInstance::rxMsgDispatcher(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef header;
    CanData_t data;

    HAL_CAN_GetRxMessage(
        hcan,
        CAN_RX_FIFO0,
        &header,
        data.data
    );
    data.id = header.StdId;
    data.data_len = header.DLC;

    for (ICanListener*& listener : m_listeners)
    {
        if ((listener) && (listener->msgForThisListener(header.StdId)))
        {
            BaseType_t ret_val = xQueueSendFromISR(listener->getCanQueueHandle(), &data, pdFALSE);
            if (pdPASS != ret_val)
            {
                CanFailedToStoreError_t error_data {.queue_status = static_cast<uint32_t>(ret_val), .frame_id = data.id, .unique_id = listener->getUniqueId(), .rx_direction=true};
                Det_WarningWithData(DET_CAN_FAILED_TO_STORE_MESSAGE, DET_MULTIPLE_TIME_REPORT_ERROR, reinterpret_cast<uint8_t *>(&error_data), sizeof(CanFailedToStoreError_t));
            }
            break;
        }
    }
}

Std_ReturnType ICanDriverInstance::queueMessage(CanData_t& data)
{
    Std_ReturnType ret_val = E_NOT_OK;
    if (pdPASS == xQueueSend(m_TxQueueHandle, &data, 0))
    {
        ret_val = E_OK;
    }
    xTaskNotifyGive(Can_TaskHandle);
    return ret_val;
}

void ICanDriverInstance::sendFromQueue(void)
{
    CanData_t msg;
    if (0 < xQueueReceive(m_TxQueueHandle, &msg, 0))
    {
        CAN_TxHeaderTypeDef header;
        uint32_t mailbox_not_used;
    
        header.StdId = msg.id;
        header.ExtId = 0x00;
        header.RTR = CAN_RTR_DATA;
        header.IDE = CAN_ID_STD;
        header.DLC = msg.data_len;
        header.TransmitGlobalTime = DISABLE;
        HAL_CAN_AddTxMessage(&m_hcan, &header, msg.data, &mailbox_not_used);
    }
}

const CAN_HandleTypeDef& ICanDriverInstance::getHalHandle() const
{
    return m_hcan;
}

uint8_t ICanDriverInstance::getUniqueId() const
{
    return m_UniqueId;
}

uint8_t ICanListener::m_UniqueId = 0;

ICanListener::ICanListener(uint8_t* rx_queue_storage_area, uint32_t rx_queue_size)
    :m_RxQueueStorageArea {rx_queue_storage_area}, m_RxQueueStorageAreaSize {rx_queue_size}
{
    m_UniqueId++;
}

Std_ReturnType ICanListener::init(ICanDriverInstance* driver_instance)
{
    Std_ReturnType ret_val {E_NOT_OK};
    m_RxQueueHandle = xQueueCreateStatic(m_RxQueueStorageAreaSize/sizeof(CanData_t), sizeof(CanData_t), m_RxQueueStorageArea, &m_RxQueue);
    if ((m_RxQueueHandle))
    {
        m_assignedDriver = driver_instance;
        ret_val = driver_instance->addListener(this);
    }

    if (E_NOT_OK == ret_val)
    {
        CanFailedToCreateReason buffer = static_cast<CanFailedToCreateReason>(Can1 + getUniqueId());
        Det_ErrorWithData(DET_CAN_FAILED_TO_CREATE_LISTENER, DET_MULTIPLE_TIME_REPORT_ERROR, reinterpret_cast<uint8_t*>(&buffer), sizeof(buffer));
    }
    return ret_val;
}

bool ICanListener::msgForThisListener(uint16_t id) const
{
    return true;
}

Std_ReturnType ICanListener::waitForMsg(CanData_t& msg, uint32_t timeout_ms)
{
    if (0 < xQueueReceive(m_RxQueueHandle, &msg, pdMS_TO_TICKS(timeout_ms)))
    {
        return E_OK;
    }
    return E_NOT_OK;
}

QueueHandle_t ICanListener::getCanQueueHandle() const
{
    return m_RxQueueHandle;
}

uint8_t ICanListener::getUniqueId() const
{
    return m_UniqueId;
}

Std_ReturnType ICanListener::sendMessage(CanData_t& data)
{
    return m_assignedDriver->queueMessage(data);
}