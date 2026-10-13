#include "flash.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

SemaphoreHandle_t Flash_MutexHandle = NULL;
StaticSemaphore_t Flash_MutexBuffer;
TaskHandle_t Flash_TaskToNotify = NULL;

static Std_ReturnType Flash_WriteWord(uint32_t address, uint32_t data)
{
    Std_ReturnType ret_val = E_OK;

    HAL_FLASH_Unlock();

    /* TODOLater - here interrupts could and should be used. However reading Flash when erase is performed (even by debugger) trigger hardfault. To be considered in future
    HAL_FLASH_Program_IT(FLASH_TYPEPROGRAM_WORD, address, data);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (HAL_FLASH_ERROR_NONE != HAL_FLASH_GetError())
    {
        ret_val = E_NOT_OK;
    } */
    if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data))
    {
        ret_val = E_NOT_OK;
    }

    HAL_FLASH_Lock();

    return ret_val;
}

static void Flash_InitializeOperation(void)
{
    xSemaphoreTake(Flash_MutexHandle, portMAX_DELAY);
    /* Clean notification */
    ulTaskNotifyTake(pdTRUE, 0);
    Flash_TaskToNotify = xTaskGetCurrentTaskHandle();
}

static void Flash_CloseOperation(void)
{
    Flash_TaskToNotify = NULL;
    xSemaphoreGive(Flash_MutexHandle);
}

Std_ReturnType Flash_EraseSector(uint32_t sector)
{
    if ((sector > FLASH_SECTOR_23) || (sector < FLASH_SECTOR_0))
    {
        return E_NOT_OK;
    }

    Flash_InitializeOperation();

    Std_ReturnType ret_val = E_OK;
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Sector       = sector,
        .NbSectors    = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
    };

    uint32_t sector_error = 0;
    if (HAL_OK != HAL_FLASHEx_Erase(&erase, &sector_error))
    {
        ret_val = E_NOT_OK;
    }
    /* TODOLater - here interrupts could and should be used. However reading Flash when erase is performed (even by debugger) trigger hardfault. To be considered in future
    HAL_FLASHEx_Erase_IT(&erase);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (HAL_FLASH_ERROR_NONE != HAL_FLASH_GetError())
    {
        ret_val = E_NOT_OK;
    } */

    HAL_FLASH_Lock();

    Flash_CloseOperation();

    return ret_val;
}

Std_ReturnType Flash_WriteBlock(uint32_t address, const uint32_t *data, uint32_t length)
{
    Flash_InitializeOperation();
    
    Std_ReturnType ret_val = E_OK;

    for (uint32_t i = 0; i < length; i++)
    {
        if (E_OK != Flash_WriteWord(address + (i * sizeof(uint32_t)), data[i]))
        {
            ret_val = E_NOT_OK;
            break;
        }
    }

    Flash_CloseOperation();
    return ret_val;
}

Std_ReturnType Flash_Init(void)
{
    Std_ReturnType ret_val = E_OK;
    Flash_MutexHandle = xSemaphoreCreateMutexStatic(&Flash_MutexBuffer);

    if (Flash_MutexHandle == NULL)
    {
        ret_val = E_NOT_OK;
    }

    /* Enable End of FLASH Operation interrupt */
  __HAL_FLASH_ENABLE_IT(FLASH_IT_EOP);
  
  /* Enable Error source interrupt */
  __HAL_FLASH_ENABLE_IT(FLASH_IT_ERR);

    return ret_val;
}

void HAL_FLASH_OperationErrorCallback(uint32_t ReturnValue)
{
    /* TODOLater - DET */
    HAL_FLASH_EndOfOperationCallback(ReturnValue);
}

void HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (Flash_TaskToNotify != NULL)
    {
        vTaskNotifyGiveFromISR(Flash_TaskToNotify, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
