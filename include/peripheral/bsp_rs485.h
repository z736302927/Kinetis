#ifndef __BSP_RS485_H
#define __BSP_RS485_H

#ifdef __cplusplus
 extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
   
/* Includes ------------------------------------------------------------------*/
#include "core_common.h"

void RS485_Master_Send(uint8_t Dev_addr, uint8_t Fun_code, uint16_t Reg_addr, uint8_t Len);

int RS485_Master_Receive(uint8_t* pData, uint16_t *Len);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __BSP_RS485_H */
