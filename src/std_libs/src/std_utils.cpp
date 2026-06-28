#include "std_utils.hpp"

void StdUtils_BufferToValueDifferenEndian(uint8_t*& pointer_modified, int32_t& variable)
{
    variable = (pointer_modified[0] << 24) | (pointer_modified[1] << 16) | (pointer_modified[2] << 8) | pointer_modified[3];
    pointer_modified += 4;
}

void StdUtils_BufferToValueDifferenEndian(uint8_t*& pointer_modified, int16_t& variable)
{
    variable = (pointer_modified[0] << 8) | pointer_modified[1];
    pointer_modified += 2;
}