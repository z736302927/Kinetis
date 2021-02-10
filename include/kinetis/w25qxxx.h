#ifndef __W25QXXX_H
#define __W25QXXX_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

#define W25Q128                         0x17
#define W25Q256                         0x18

u8 w25qxxx_ReadStatusRegister(u8 w25qxxx, u8 Number);
void w25qxxx_WriteStatusRegister(u8 w25qxxx, u8 Number, u8 Data);
void w25qxxx_ReadData(u8 w25qxxx, u32 Addr, u8 *pData, u32 Length);
void w25qxxx_WriteData(u8 w25qxxx, u32 Addr, u8 *pData, u16 Length);
void w25qxxx_SectorErase(u8 w25qxxx, u32 Addr);
u8 w25qxxx_ReleaseDeviceID(u8 w25qxxx);
void w25qxxx_Init(u8 w25qxxx);
void w25qxxx_ReadInfo(u8 w25qxxx);
void w25qxxx_Test(u8 w25qxxx);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __W25QXXX_H */
