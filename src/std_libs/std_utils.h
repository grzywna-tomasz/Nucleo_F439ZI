#include "std_types.h"

#define StdUtils_BufferToUint16(buf) (uint16_t)(((buf)[1] << 8) | (buf)[0])
#define StdUtils_BufferToInt32(buf) (int32_t)(((buf)[3] << 24) | ((buf)[2] << 16) | ((buf)[1] << 8) | (buf)[0])

#define StdUtils_Uint16ToBuffer(buf, value)             \
    do {                                                \
        (buf)[0] = (uint8_t)((value) & 0xFF);           \
        (buf)[1] = (uint8_t)(((value) >> 8) & 0xFF);    \
    } while(0)
