#include "std_types.h"

Std_ReturnType Flash_Init(void);
Std_ReturnType Flash_WriteBlock(uint32_t address, const uint32_t *data, uint32_t length);
Std_ReturnType Flash_EraseSector(uint32_t sector);
