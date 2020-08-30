#ifndef __KRNG_H
#define __KRNG_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core/core_common.h"

#define RNG_8BITS                       8
#define RNG_32BITS                      16
#define RNG_16BITS                      32
#define RNG_64BITS                      64

uint8_t Random_Get8bit(void);
uint16_t Random_Get16bit(void);
uint32_t Random_Get32bit(void);
void Random_GetArray(void *pData, uint32_t Length, uint8_t Bits);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __KRNG_H */
