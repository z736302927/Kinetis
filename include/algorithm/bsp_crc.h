#ifndef __BSP_CRC_H
#define __BSP_CRC_H

#ifdef __cplusplus
 extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
   
/* Includes ------------------------------------------------------------------*/
#include "core_common.h"

short CRC16_Calculate(char* pchMsg,  int wDataLen);
int CRC16_Check(char* input, int inputlen);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __BSP_CRC_H */
