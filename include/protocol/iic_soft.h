#ifndef __IIC_SOFT_H
#define __IIC_SOFT_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core/core_common.h"

#define IIC_1                           1
#define IIC_2                           2
#define IIC_3                           3


void IIC_Soft_Init(void);
void IIC_Soft_delay(uint32_t ticks);
int IIC_Soft_Start(void);
void IIC_Soft_Stop(void);
int IIC_Soft_WaitAck(void);
void IIC_Soft_SendByte(uint8_t Data);
uint8_t IIC_Soft_ReadByte(uint8_t Ack);
void IIC_PortTransmmit(uint8_t IIC, uint8_t SlaveAddr, uint16_t RegAddr, uint8_t Regdata);
void IIC_PortReceive(uint8_t IIC, uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *Regdata);
void IIC_PortMultiTransmmit(uint8_t IIC, uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len);
void IIC_PortMultiReceive(uint8_t IIC, uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pData, uint8_t Len);

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#endif /* __IIC_SOFT_H */
