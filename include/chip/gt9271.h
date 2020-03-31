#ifndef __GT9271_H
#define __GT9271_H

#ifdef __cplusplus
 extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/
   
/* Includes ------------------------------------------------------------------*/
#include "core_common.h"

#define GTP_MAX_HEIGHT                  800
#define GTP_MAX_WIDTH                   1280
   
int32_t GTP_Init_Panel(void);
void GTP_ReadCurrentTSCase(uint8_t *pData);
void GTP_WriteCurrentTSCase(uint8_t Data);
void GTP_ReadCurrentTSPoint(uint16_t Addr, uint8_t *pData, uint16_t Len);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __GT9271_H */
