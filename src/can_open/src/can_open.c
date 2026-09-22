#include "main.h"
#include "CANopen.h"
#include "CO_app_STM32.h"
#include "CO_SDOclient.h"
#include "FreeRTOS.h"
#include "task.h"
#include "can_open.h"

#define CAN_OPEN_STACK_SIZE     (256U)
#define CAN_OPEN_TASK_PRIORITY  (configMAX_PRIORITIES - 1)

static void CANopen_Task(void * pvParameters);
static void CANopen_Error(void);

static StaticTask_t CANOpen_TaskStruct;
static StackType_t CANOpen_StackBuffer[CAN_OPEN_STACK_SIZE];
static TaskHandle_t CANOpen_Handle = NULL;

extern CAN_HandleTypeDef hcan2;
extern TIM_HandleTypeDef htim4;
/* Global CAN Open object. Used for data handling */
extern CO_t *CO;

Std_ReturnType CANOpen_ReadSDO(uint8_t nodeId, uint16_t index, uint8_t subIndex, uint8_t *buf, uint8_t bufSize, uint8_t *readSize)
{
    if (CO_SDO_RT_ok_communicationEnd != CO_SDOclient_setup(CO->SDOclient, CO_CAN_ID_SDO_CLI + nodeId, CO_CAN_ID_SDO_SRV + nodeId, nodeId))
    {
        return E_NOT_OK;
    }

    /* TODO check and rework the delay */
    if (CO_SDO_RT_ok_communicationEnd != CO_SDOclientUploadInitiate(CO->SDOclient, index, subIndex, 1000, false))
    {
        return E_NOT_OK;
    }

    /* TODO rework this to have a timeout and simplify the logic. Check the timeDifference_us */
    CO_SDO_return_t communication_code;
    do 
    {
        uint32_t timeDifference_us = 10000;
        CO_SDO_abortCode_t abortCode = CO_SDO_AB_NONE;

        communication_code = CO_SDOclientUpload(CO->SDOclient, timeDifference_us, false, &abortCode, NULL, NULL, NULL);
        if (communication_code < 0) 
        {
            return E_NOT_OK;
        }

        /* TODO Verify this delay - maybe use some notification to handle this */
        vTaskDelay(pdMS_TO_TICKS(1));
    } while(communication_code > 0);

    /* TODO do it if needed */
    // copy data to the user buffer (for long data function must be called
    // several times inside the loop)
    *readSize = CO_SDOclientUploadBufRead(CO->SDOclient, buf, bufSize);

    return E_OK;
}

Std_ReturnType CANOpen_WriteSDO(uint8_t nodeId, uint16_t index, uint8_t subIndex, uint8_t *data, uint8_t dataSize)
{
    bool_t bufferPartial = false;

    if (CO_SDO_RT_ok_communicationEnd != CO_SDOclient_setup(CO->SDOclient, CO_CAN_ID_SDO_CLI + nodeId, CO_CAN_ID_SDO_SRV + nodeId, nodeId))
    {
        return E_NOT_OK;
    }

    /* TODO check and rework the delay */
    if (CO_SDO_RT_ok_communicationEnd != CO_SDOclientDownloadInitiate(CO->SDOclient, index, subIndex, dataSize, 1000, false))
    {
        return E_NOT_OK;
    }

    size_t nWritten = CO_SDOclientDownloadBufWrite(CO->SDOclient, data, dataSize);
    if (nWritten < dataSize) 
    {
        bufferPartial = true;
        // TODO - fix this if needed If SDO Fifo buffer is too small, data can be refilled in the loop.
    }

    /* TODO rework this to have a timeout and simplify the logic. Check the timeDifference_us */
    CO_SDO_return_t communication_code;
    do 
    {
        uint32_t timeDifference_us = 10000;
        CO_SDO_abortCode_t abortCode = CO_SDO_AB_NONE;

        communication_code = CO_SDOclientDownload(CO->SDOclient, timeDifference_us, false, bufferPartial, &abortCode, NULL, NULL);
        if (communication_code < 0)
        {
            return E_NOT_OK;
        }

        /* TODO Verify this delay - maybe use some notification to handle this */
        vTaskDelay(pdMS_TO_TICKS(1));
    } while(communication_code > 0);

    return E_OK;
}

static CO_SDO_return_t CANopen_InitializeNode(uint8_t node_id)
{
    return CO_NMT_sendCommand(CO->NMT, CO_NMT_ENTER_OPERATIONAL, node_id);
}

void CANopen_ResetNode(uint8_t node_id)
{
    CO_NMT_sendCommand(CO->NMT, CO_NMT_RESET_NODE, node_id);
}

Std_ReturnType CANOpen_Init(void)
{
    CANOpen_Handle = xTaskCreateStatic(CANopen_Task, "CanOpen", CAN_OPEN_STACK_SIZE, 0, CAN_OPEN_TASK_PRIORITY, CANOpen_StackBuffer, &CANOpen_TaskStruct);

    if (NULL_PTR == CANOpen_Handle)
    {
        return E_NOT_OK;
    }

    return E_OK;
}

static void CANopen_Task(void * pvParameters)
{
    CANopenNodeSTM32 server_node;
    server_node.CANHandle      = &hcan2;
    server_node.HWInitFunction = MX_CAN2_Init;
    server_node.timerHandle    = &htim4;
    server_node.desiredNodeID  = 120;
    server_node.baudrate       = 500;

    
    if (0 != canopen_app_init(&server_node))
    {
        CANopen_Error();
    }

    // HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    // HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    
    /* TODO do it smarter. This should get all available nodes */
    /* wait for all nodes to become alive */
    vTaskDelay(pdMS_TO_TICKS(2000));
    uint8_t temp_node = 0x01;

    __HAL_CAN_ENABLE_IT(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);

    if (CO_SDO_RT_ok_communicationEnd != CANopen_InitializeNode(temp_node))
    {
        CANopen_Error();
    }

    for(;;)
    {
        canopen_app_process(); // todo Propably could be removed with server
        // CO_SDOclient_process(&pCO->SDOclient[0], 1000, NULL); TODO check if this is needed for anything

        /* todo just to let other tasks work. can be removed in future with task notification */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void CANopen_Error(void)
{
    /* TODO - add DET instead of while */
    volatile uint8_t dummy_wait = 1;
    while(dummy_wait);
}

/* TODO - remove it or change it */
void CANopen_process(TIM_HandleTypeDef *htim)
{
    if (htim == canopenNodeSTM32->timerHandle) {
      canopen_app_interrupt();
  }
}

/* TODO - do it better */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4)
    {
        CANopen_process(htim);
    }
}