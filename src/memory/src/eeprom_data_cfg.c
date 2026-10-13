#include "eeprom_data_cfg.h"
#include "string.h"

const VescBlockConfig_t VescDataConfig_DefaultData = {
    .header_data = {
        .magic_number = EEPROM_MAGIC_NUMBER,
        .block_id = EEPROM_VESC_BLOCK_ID,
        .sequence_version = 0x00000001,
        .block_size = (sizeof(VescBlockConfig_t) + (MEMORY_ALIGNMENT - 1)) & ~(MEMORY_ALIGNMENT - 1)
    },
    .can_id = 32,
    .max_erpm = 10000,
    .max_current = 30000
};

VescBlockConfig_t VescDataConfig_RAM;

EepromDatainit_t EepromData_pointers[] = {
    {
        .ram_ptr = (uint32_t *)(&VescDataConfig_RAM),
        .default_ptr = (EepromDataHeader_t *)(&VescDataConfig_DefaultData)
    }
};

const uint32_t EepromData_PointersSize = sizeof(EepromData_pointers) / sizeof(EepromDatainit_t);