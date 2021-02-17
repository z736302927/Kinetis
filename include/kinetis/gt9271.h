#ifndef __GT9271_H
#define __GT9271_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

#define GTP_MAX_HEIGHT                  800
#define GTP_MAX_WIDTH                   1280

int32_t GTP_Init_Panel(void);
void GTP_ReadCurrentTSCase(u8 *pdata);
void GTP_WriteCurrentTSCase(u8 Data);
void GTP_ReadCurrentTSPoint(u16 addr, u8 *pdata, u16 length);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __GT9271_H */
