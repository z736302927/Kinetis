#ifndef __BSP_SERIALPORT_H
#define __BSP_SERIALPORT_H

#ifdef __cplusplus
 extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
   
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

typedef struct SerialPort_TypeDef {
  uint16_t RxBuffer[256];
  uint16_t RxBuffer_Size;
  uint16_t Rx_pHead;
  uint16_t Rx_pTail;
  uint32_t RxBeginTime;
  uint32_t RxCurrentTime;
  uint32_t RxTimeDiff;
  uint32_t RxScanInterval;
  uint8_t Tx_SendDone;
  
  struct SerialPort_TypeDef* Next;
}SerialPort_TypeDef;


void SerialPort_Open(void);
void SerialPort_Close(void);
void SerialPort_Receive(void);


/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef __cplusplus
} 
#endif

#endif /* __BSP_SERIALPORT_H */
