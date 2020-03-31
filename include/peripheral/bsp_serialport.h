#ifndef __BSP_SERIALPORT_H
#define __BSP_SERIALPORT_H

#ifdef __cplusplus
 extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
   
/* Includes ------------------------------------------------------------------*/
#include "core_common.h"
   
typedef struct SerialPort_TypeDef {
  uint16_t *RxBuffer;
  uint16_t RxBuffer_Size;
  uint32_t RxScanInterval;
  uint16_t Rx_pHead;
  uint16_t Rx_pTail;
  uint32_t RxBeginTime;
  uint32_t RxCurrentTime;
  uint32_t RxTimeDiff;
  uint8_t Tx_SendDone;
  
  struct SerialPort_TypeDef* Next;
}SerialPort_TypeDef;


uint8_t SerialPort_Open(SerialPort_TypeDef *Instance, uint32_t Interval, uint16_t Size);
void SerialPort_Close(SerialPort_TypeDef *Instance);
void SerialPort_Send(SerialPort_TypeDef *Instance, uint8_t* pData, uint16_t Len);
void SerialPort_Receive(SerialPort_TypeDef *Instance, uint8_t* pData, uint16_t* Len);
int SerialPort_ReadRxState(void);
void SerialPort_SetRxState(int state);

extern SerialPort_TypeDef SerialPort_1;
extern SerialPort_TypeDef SerialPort_2;
extern SerialPort_TypeDef SerialPort_3;

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef __cplusplus
} 
#endif

#endif /* __BSP_SERIALPORT_H */
