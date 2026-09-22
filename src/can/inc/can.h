#ifndef CAN_H
#define CAN_H

#include "std_types.h"
#include "stm32f4xx_hal.h"

#define CAN_DATA_SIZE   (8U)

typedef enum
{
    FRAME_STANDARD = CAN_ID_STD,
    FRAME_EXTENDED = CAN_ID_EXT
} frame_id_type_t;

typedef struct
{
    uint16_t id;
    uint8_t data[CAN_DATA_SIZE];
    uint8_t data_len;
    frame_id_type_t frame_id_type;
} CanData_t;

Std_ReturnType Can_Init(void);
Std_ReturnType Can_SendMessage(CanData_t* msg);
void Can_WaitForMessage(CanData_t* msg);

#endif /* CAN_H */