#ifndef __IIC_SOFT_H
#define __IIC_SOFT_H

#ifdef __cplusplus
 extern "C" {
#endif
   
/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
                                              
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"


void IIC_Soft_Init(void);
int IIC_Soft_Start(void);
void IIC_Soft_Stop(void);
int IIC_Soft_WaitAck(void);
void IIC_Soft_SendByte(uint8_t Data);
uint8_t IIC_Soft_ReadByte(uint8_t Ack);
uint8_t IIC_Soft_WriteSingleByteWithAddr(uint8_t SlaveAddr,uint16_t RegAddr,uint8_t Regdata);
uint8_t IIC_Soft_ReadSingleByteWithAddr(uint8_t SlaveAddr,uint16_t RegAddr,uint8_t *Regdata);
uint8_t IIC_Soft_WriteMultiByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len);
uint8_t IIC_Soft_ReadMultiByteWithAddr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len);

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#endif /* __IIC_SOFT_H */
