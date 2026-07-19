#pragma once
#include "std_types.h"

void StdUtils_BufferToValueDifferenEndian(uint8_t*& pointer_modified, int32_t& variable);
void StdUtils_BufferToValueDifferenEndian(uint8_t*& pointer_modified, int16_t& variable);
void StdUtils_ValueToBufferDifferenEndian(uint8_t*& pointer_modified, int32_t& variable);
void StdUtils_ValueToBufferDifferenEndian(uint8_t*& pointer_modified, int16_t& variable);
