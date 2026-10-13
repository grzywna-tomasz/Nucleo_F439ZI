#pragma once

#include "eeprom_data_cfg.h"

void EepromData_Init(void);
void EepromData_ErasePage(void);
Std_ReturnType EepromData_WriteBlock(uint32_t block_id);