#ifndef __W25QXXX_H
#define __W25QXXX_H

#ifdef __cplusplus
 extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
   
/* Includes ------------------------------------------------------------------*/
#include "core_common.h"

#define W25Q128                         0x17
#define W25Q256                         0x18

uint8_t w25qxxx_ReadStatusRegister(uint8_t w25qxxx, uint8_t Number);
void w25qxxx_WriteStatusRegister(uint8_t w25qxxx, uint8_t Number, uint8_t Data);
void w25qxxx_ReadData(uint8_t w25qxxx, uint32_t Addr, uint8_t *pData, uint32_t Length);
void w25qxxx_WriteData(uint8_t w25qxxx, uint32_t Addr, uint8_t* pData, uint16_t Length);
void w25qxxx_SectorErase(uint8_t w25qxxx, uint32_t Addr);
uint8_t w25qxxx_ReleaseDeviceID(uint8_t w25qxxx);
void w25qxxx_Init(uint8_t w25qxxx);
void w25qxxx_ReadInfo(uint8_t w25qxxx);
void w25qxxx_Test(uint8_t w25qxxx);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __W25QXXX_H */
