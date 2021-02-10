#ifndef __K_SERIALPORT_H
#define __K_SERIALPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

typedef struct SerialPort_TypeDef {
    u16 TempBuffer_Size;
    u16 *TempBuffer;
    u16 RxBuffer_Size;
    char *RxBuffer;
    u32 RxScanInterval;
    u16 Rx_pHead;
    u16 Rx_pTail;
    u8 *TxBuffer;
    u16 TxBuffer_Size;
    u8 PortNbr;
    u8 *Endchar;
    u8 Endchar_Size;
    u8 *CurrentBuffer;

    struct SerialPort_TypeDef *Next;
} SerialPort_TypeDef;

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

u8 SerialPort_Open(SerialPort_TypeDef *Instance);
void SerialPort_Close(SerialPort_TypeDef *Instance);
void SerialPort_Send(SerialPort_TypeDef *Instance);
u8 SerialPort_Receive(SerialPort_TypeDef *Instance);



#ifdef __cplusplus
}
#endif

#endif /* __K_SERIALPORT_H */
