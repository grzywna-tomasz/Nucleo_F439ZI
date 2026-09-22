#include "std_types.h"

Std_ReturnType CANOpen_ReadSDO(uint8_t nodeId, uint16_t index, uint8_t subIndex, uint8_t *buf, uint8_t bufSize, uint8_t *readSize);
Std_ReturnType CANOpen_WriteSDO(uint8_t nodeId, uint16_t index, uint8_t subIndex, uint8_t *data, uint8_t dataSize);
Std_ReturnType CANOpen_Init(void);
void CANopen_ResetNode(uint8_t node_id);