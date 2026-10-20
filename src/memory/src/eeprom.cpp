#include "eeprom.hpp"
#include <cstring>
extern "C" {
#include "flash.h"
#include "FreeRTOS.h"
#include "task.h"
}

/* TODOLater:
1. change this module to class
2. Add CRC to blocks */

/* TODOLater - This is only test purposes, in proper implementation there should be at least two pages used
this is implement4ed for STM32F439 with 2 MB Flash with usage of FLASH_SECTOR_23. If this is modified you need to modify EEPROM_START_ADDRESS, EEPROM_PAGE_SIZE */

extern EepromDatainit_t EepromData_pointers[];
constexpr uint32_t EEPROM_SECTOR = 23; /* FLASH_SECTOR_23 is used for EEPROM emulation */
constexpr uint32_t EEPROM_START_ADDRESS = 0x081E0000; /* Start address of EEPROM emulation area. */ 
constexpr uint32_t EEPROM_PAGE_SIZE = 0x20000;
static uint8_t * EepromData_FreeSpacePtr = reinterpret_cast<uint8_t *>(EEPROM_START_ADDRESS);

typedef struct
{
    uint8_t * free_space_ptr;
    uint32_t block_id;
} EepromData_WriteBlockParams_t;

extern "C" void EepromData_WriteBlockTask(void *pvParameters);
extern "C" void EepromData_ErasePageTask(void *pvParameters);

void EepromData_Init(void)
{
    uint8_t * recent_data_ptr[EepromData_PointersSize];
    memset(recent_data_ptr, 0, sizeof(recent_data_ptr));
    uint8_t * current_eeprom_ptr = reinterpret_cast<uint8_t *>(EEPROM_START_ADDRESS);

    while (1)
    {
        EepromDataHeader_t *current_data_ptr = reinterpret_cast<EepromDataHeader_t *>(current_eeprom_ptr);
        if (EEPROM_MAGIC_NUMBER == current_data_ptr->magic_number)
        {
            EepromDataHeader_t *recent_data_header = reinterpret_cast<EepromDataHeader_t *>(recent_data_ptr[current_data_ptr->block_id]);
            if ((recent_data_header == nullptr) || (current_data_ptr->sequence_version > recent_data_header->sequence_version))
            {
                recent_data_ptr[current_data_ptr->block_id] = current_eeprom_ptr;
            }
            current_eeprom_ptr += current_data_ptr->block_size;
            if (current_eeprom_ptr >= reinterpret_cast<uint8_t *>(EEPROM_START_ADDRESS + EEPROM_PAGE_SIZE))
            {
                /* TODOLater - here should we should move to next page, erase old page etc. */
                break;
            }
            EepromData_FreeSpacePtr = current_eeprom_ptr;
        }
        else
        {
            break;
        }
    }

    for (uint32_t i = 0; i < EepromData_PointersSize; i++)
    {
        if (recent_data_ptr[i] != nullptr)
        {
            /* TODONow - add CRC verification */
            memcpy(EepromData_pointers[i].ram_ptr, recent_data_ptr[i], EepromData_pointers[i].default_ptr->block_size);
        }
        else
        {
            memcpy(EepromData_pointers[i].ram_ptr, EepromData_pointers[i].default_ptr, EepromData_pointers[i].default_ptr->block_size);
        }
    }
}

Std_ReturnType EepromData_WriteBlock(uint32_t block_id)
{
    if ((block_id >= EepromData_PointersSize) || 
    (EepromData_FreeSpacePtr + EepromData_pointers[block_id].default_ptr->block_size > 
        reinterpret_cast<uint8_t *>(EEPROM_START_ADDRESS + EEPROM_PAGE_SIZE)))
    {
        return E_NOT_OK;
    }

    Std_ReturnType ret_val = E_OK;
    EepromData_WriteBlockParams_t *params = reinterpret_cast<EepromData_WriteBlockParams_t *>(pvPortMalloc(sizeof(EepromData_WriteBlockParams_t)));
    params->block_id = block_id;
    params->free_space_ptr = EepromData_FreeSpacePtr;

    if (pdPASS != xTaskCreate(EepromData_WriteBlockTask, "WriteBlock", 512, params, tskIDLE_PRIORITY + 1, NULL))
    {
        vPortFree(params);
        ret_val = E_NOT_OK;
    }
    else
    {
        EepromDataHeader_t *header_data = reinterpret_cast<EepromDataHeader_t *>(EepromData_pointers[block_id].ram_ptr);
        header_data->sequence_version += 1;
        EepromData_FreeSpacePtr += EepromData_pointers[block_id].default_ptr->block_size;
    }

    return ret_val;
}

extern "C" void EepromData_WriteBlockTask(void *pvParameters)
{
    EepromData_WriteBlockParams_t *params = reinterpret_cast<EepromData_WriteBlockParams_t *>(pvParameters);
    uint32_t block_id = params->block_id;

    /* TODONow - add CRC calculation */
    
    if (E_OK != Flash_WriteBlock(reinterpret_cast<uint32_t>(params->free_space_ptr), EepromData_pointers[block_id].ram_ptr, EepromData_pointers[block_id].default_ptr->block_size / MEMORY_ALIGNMENT))
    {
        /* TODOLater - handle write error */
    }

    vPortFree(params);
    vTaskDelete(nullptr);
}

/* TODOLater - this task should remove all EepromData_WriteBlockTask if any exists. */
extern "C" void EepromData_ErasePageTask(void *pvParameters)
{
    Flash_EraseSector(EEPROM_SECTOR);

    EepromData_FreeSpacePtr = reinterpret_cast<uint8_t *>(EEPROM_START_ADDRESS);
    vTaskDelete(nullptr);
}

void EepromData_ErasePage(void)
{
    if (pdPASS != xTaskCreate(EepromData_ErasePageTask, "ErasePage", 1024, nullptr, tskIDLE_PRIORITY + 1, nullptr))
    {
        while(1)
        {
            /* TODOLater - DET */
        }
        /* TODOLater - DET */
    }
}
