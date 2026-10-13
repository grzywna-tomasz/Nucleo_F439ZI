#pragma once

#include "std_types.h"

#define EEPROM_MAGIC_NUMBER (0xAA559966U)
#define MEMORY_ALIGNMENT (4U) /* 4 bytes alignment for STM32F439ZI */

typedef struct
{
    uint32_t magic_number;      /* Used to verify correctness of the record */
    uint32_t block_id;          /* Used to identify block */
    uint32_t sequence_version;  /* current version, increased with each update */
    uint32_t block_size;
    uint32_t data_crc;          /* CRC32 of the block */
} EepromDataHeader_t;

typedef struct
{
    uint32_t * const ram_ptr;
    const EepromDataHeader_t * const default_ptr;
} EepromDatainit_t;

extern const uint32_t EepromData_PointersSize;

/* Block 1 definition */
#define EEPROM_VESC_BLOCK_ID (0x00000000U)
typedef struct
{
    EepromDataHeader_t header_data;
    uint8_t can_id;
    uint32_t max_erpm;
    uint32_t max_current;
} VescBlockConfig_t;
extern VescBlockConfig_t VescDataConfig_RAM;