#ifndef __W25Q128_H
#define __W25Q128_H

#ifdef __cplusplus
 extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
   
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"


uint8_t w25q128_ReadStatusRegister(uint8_t Number);
void w25q128_WriteStatusRegister(uint8_t Number, uint8_t Data);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __W25Q128_H */
