#ifndef __W25QXXX_H
#define __W25QXXX_H

#ifdef __cplusplus
 extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
   
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"


uint8_t w25qxxx_ReadStatusRegister(uint8_t Number);
void w25qxxx_WriteStatusRegister(uint8_t Number, uint8_t Data);
void w25qxxx_ReadData(uint32_t Addr, uint8_t *pData, uint32_t Length);
void w25qxxx_WriteData(uint32_t Addr, uint8_t* pData, uint16_t Length);
void w25qxxx_SectorErase(uint32_t Addr);
void w25qxxx_Init(void);
void w25qxxx_ReadInfo(void);
void w25qxxx_Test(void);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __W25QXXX_H */
