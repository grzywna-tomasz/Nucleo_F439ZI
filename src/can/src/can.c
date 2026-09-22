#include "can.h"
#include "std_utils.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32f4xx_hal_can.h"
#include "det.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

#define CAN_STACK_SIZE      (256U)
#define CAN_TASK_PRIORITY   (tskIDLE_PRIORITY + 2)
StackType_t Can_Stack[CAN_STACK_SIZE];
StaticTask_t Can_TaskBuffer;
TaskHandle_t Can_TaskHandle = NULL;

#define CAN_TX_QUEUE_SIZE   (12U)
static QueueHandle_t TxQueueHandle = NULL;
static StaticQueue_t TxQueue;
static uint8_t TxQueueStorageArea[CAN_TX_QUEUE_SIZE * sizeof(CanData_t)];

#define CAN_RX_QUEUE_SIZE   (12U)
static QueueHandle_t RxQueueHandle = NULL;
static StaticQueue_t RxQueue;
static uint8_t RxQueueStorageArea[CAN_RX_QUEUE_SIZE * sizeof(CanData_t)];

static uint8_t Can_GetNumberOfInstance(CAN_HandleTypeDef *hcan);
static void Can_Task(void *pvParams);

static void CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    CanData_t msg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, msg.data);
    if (CAN_ID_EXT == rx_header.IDE)
    {
        msg.id = rx_header.ExtId;
    }
    else
    {
        msg.id = rx_header.StdId;
    }
    msg.data_len = rx_header.DLC;

    xQueueSendFromISR(RxQueueHandle, &msg, &xHigherPriorityTaskWoken);
}

static void CAN_TxMailboxCompleteCallback(CAN_HandleTypeDef *hcan)
{
    /* Trigger transmission if there are more messages in queue */
    xTaskNotifyGive(Can_TaskHandle);
}

void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan)
{
    uint8_t can_driver_id = Can_GetNumberOfInstance(hcan);

    Det_ErrorWithData(DET_CAN_RX_FIFO_FULL_CALLBACK, DET_MULTIPLE_TIME_REPORT_ERROR, &can_driver_id, sizeof(can_driver_id));
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    CanErrorCallback_t buffer = {.can_driver_id = Can_GetNumberOfInstance(hcan), hcan->ErrorCode};

    Det_ErrorWithData(DET_CAN_ERROR_CALLBACK, DET_MULTIPLE_TIME_REPORT_ERROR, (uint8_t*)(&buffer), sizeof(buffer));
}

static uint8_t Can_GetNumberOfInstance(CAN_HandleTypeDef *hcan)
{
    uint8_t can_driver_id = 0xff;

    if (hcan == &hcan1)
    {
        can_driver_id = 1;
    }
    else if (hcan == &hcan2)
    {
        can_driver_id = 2;
    }

    return can_driver_id;
}

Std_ReturnType Can_Init(void)
{
    Std_ReturnType ret_val = E_NOT_OK;

    Can_TaskHandle = xTaskCreateStatic(Can_Task, "CanTask", CAN_STACK_SIZE, (void *) 0, CAN_TASK_PRIORITY, Can_Stack, &Can_TaskBuffer);
    TxQueueHandle = xQueueCreateStatic(CAN_TX_QUEUE_SIZE, sizeof(CanData_t), TxQueueStorageArea, &TxQueue);
    RxQueueHandle = xQueueCreateStatic(CAN_RX_QUEUE_SIZE, sizeof(CanData_t), RxQueueStorageArea, &RxQueue);
    HAL_CAN_RegisterCallback(&hcan1, HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID, CAN_RxFifo0MsgPendingCallback);
    HAL_CAN_RegisterCallback(&hcan1, HAL_CAN_TX_MAILBOX0_COMPLETE_CB_ID, CAN_TxMailboxCompleteCallback);
    HAL_CAN_RegisterCallback(&hcan1, HAL_CAN_TX_MAILBOX1_COMPLETE_CB_ID, CAN_TxMailboxCompleteCallback);
    HAL_CAN_RegisterCallback(&hcan1, HAL_CAN_TX_MAILBOX2_COMPLETE_CB_ID, CAN_TxMailboxCompleteCallback);

    if ((TxQueueHandle) && (Can_TaskHandle) && (RxQueueHandle))
    {
        ret_val = E_OK;
    }

    if (E_NOT_OK == ret_val)
    {
        uint8_t can_driver_id = Can_GetNumberOfInstance(&hcan1);
        Det_ErrorWithData(DET_CAN_FAILED_TO_CREATE, DET_MULTIPLE_TIME_REPORT_ERROR, &can_driver_id, sizeof(can_driver_id));
    }

    return ret_val;
}

static void Can_Task(void *pvParams)
{
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

    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_ConfigFilter(&hcan1, &filter);
    HAL_CAN_Start(&hcan1);

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        CanData_t msg;
        if ((HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0) && (0 < xQueueReceive(TxQueueHandle, &msg, 0)))
        {
            CAN_TxHeaderTypeDef header;
            uint32_t mailbox_not_used;

            /* It is faster to just fill those two values than checking what type of frame it is */
            header.StdId = msg.id;
            header.ExtId = msg.id;
            header.RTR = CAN_RTR_DATA;
            header.IDE = msg.frame_id_type;
            header.DLC = msg.data_len;
            header.TransmitGlobalTime = DISABLE;
            HAL_CAN_AddTxMessage(&hcan1, &header, msg.data, &mailbox_not_used);
        }
    }
}

Std_ReturnType Can_SendMessage(CanData_t* data)
{
    Std_ReturnType ret_val = E_NOT_OK;
    if (pdPASS == xQueueSend(TxQueueHandle, data, 0))
    {
        ret_val = E_OK;
    }
    xTaskNotifyGive(Can_TaskHandle);
    return ret_val;
}

void Can_WaitForMessage(CanData_t* msg)
{
    xQueueReceive(RxQueueHandle, msg, portMAX_DELAY);
}