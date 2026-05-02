#include "example_common.h"
#include <cstring>
#include "erpc_port.h"

/* Values defined for STM32F429ZI */
static constexpr int Memory_FlashStart = 0x08000000;
static constexpr int Memory_FlashEnd = 0x081FFFFF;
static constexpr int Memory_RamStart = 0x20000000;
static constexpr int Memory_RamEnd = 0x2002FFFF;

extern "C" {
Erpc_Status_t Memory_Read(uint32_t address, uint32_t size, list_uint8_1_t * out_data)
{
    int address_end = address + size;
    if ((((Memory_FlashStart <= address) && (Memory_FlashEnd >= address_end)) ||
        ((Memory_RamStart <= address) && (Memory_RamEnd >= address_end))) && out_data)
    {
        out_data->elements = reinterpret_cast<uint8_t *>(erpc_malloc(size));
        if (out_data->elements)
        {
            memcpy(out_data->elements, reinterpret_cast<uint8_t *>(address), size);
            out_data->elementsCount = size;
            return ERPC_OK;
        }
        return ERPC_NOT_OK;
    }
    else
    {
        return ERPC_NOT_OK;
    }
}

Erpc_Status_t Memory_Write(uint32_t address, const list_uint8_1_t * in_data)
{
    if (in_data && in_data->elements)
    {
        int address_end = address + in_data->elementsCount;
        if ((Memory_RamStart <= address) && (Memory_RamEnd >= address_end))
        {
            memcpy(reinterpret_cast<uint8_t *>(address), in_data->elements, in_data->elementsCount);
            return ERPC_OK;
        }
        else
        {
            return ERPC_NOT_OK;
        }
    }
    else
    {
        return ERPC_NOT_OK;
    }
}
} /* extern "C" */