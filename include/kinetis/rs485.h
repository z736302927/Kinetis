#ifndef __K_RS485_H
#define __K_RS485_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

void RS485_Master_Send(u8 Dev_addr, u8 Fun_code, u16 Reg_addr, u8 length);

int RS485_Master_Receive(u8 *pdata, u16 *length);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_RS485_H */
