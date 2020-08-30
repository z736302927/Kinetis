#ifndef __K_SERIALPORT_H
#define __K_SERIALPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core_common.h"

typedef struct SerialPort_TypeDef
{
    uint16_t TempBuffer_Size;
    uint16_t *TempBuffer;
    uint16_t RxBuffer_Size;
    char *RxBuffer;
    uint32_t RxScanInterval;
    uint16_t Rx_pHead;
    uint16_t Rx_pTail;
    uint8_t *TxBuffer;
    uint16_t TxBuffer_Size;
    uint8_t PortNbr;
    uint8_t *Endchar;
    uint8_t Endchar_Size;
    uint8_t *CurrentBuffer;

    struct SerialPort_TypeDef *Next;
} SerialPort_TypeDef;

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

uint8_t SerialPort_Open(SerialPort_TypeDef *Instance);
void SerialPort_Close(SerialPort_TypeDef *Instance);
void SerialPort_Send(SerialPort_TypeDef *Instance);
uint8_t SerialPort_Receive(SerialPort_TypeDef *Instance);



#ifdef __cplusplus
}
#endif

#endif /* __K_SERIALPORT_H */
