#include "app.h"
#include "std_utils.h"
#include "example_common.h"

uint32_t Det_GetCommonErrorDataLenght(void)
{
    return sizeof(CommonErrorData_t);
}

uint32_t Det_CommonErrorData(uint8_t* buffer)
{
    StdUtils_Uint16ToBuffer(buffer, APP_VERSION);
    StdUtils_Uint16ToBuffer(buffer + APP_VERSION_SIZEOF, ERPC_INTERFACE_VERSION);
    return Det_GetCommonErrorDataLenght();
}