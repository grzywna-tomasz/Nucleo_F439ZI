#ifndef STD_TYPES_H
#define STD_TYPES_H

/**********************************INCLUDE*************************************/
#include "stdint.h"
#include "stdbool.h"

/**********************************DEFINES*************************************/
#define NULL_PTR    ((void*)0)

/**********************************TYPEDEFS************************************/
// typedef enum 
// {
//     E_OK = 0U,
//     E_NOT_OK = 1U
// } Std_ReturnType;

#define E_OK        (0u)
#define E_NOT_OK    (1u)

typedef uint8_t Std_ReturnType;

/**********************************PROTOTYPES**********************************/

/**********************************OBJECTS*************************************/

/**********************************DEFINITIONS*********************************/

#endif /* STD_TYPES_H */