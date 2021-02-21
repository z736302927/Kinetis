#ifndef __K_TOUCHSCREEN_H
#define __K_TOUCHSCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

typedef struct {
    u16 TouchDetected;
    u16 X;
    u16 Y;
    u16 Z;
} TS_StateTypeDef;

typedef struct {
    void (*Init)(u16);
    u16(*ReadID)(u16);
    void (*Reset)(u16);
    void (*Start)(u16);
    u8(*DetectTouch)(u16);
    void (*GetXY)(u16, u16 *, u16 *);
    void (*EnableIT)(u16);
    void (*ClearIT)(u16);
    u8(*GetITStatus)(u16);
    void (*DisableIT)(u16);
} TS_DrvTypeDef;

typedef enum {
    TS_OK       = 0x00,
    TS_ERROR    = 0x01,
    TS_TIMEOUT  = 0x02
} TS_StatusTypeDef;

u8 K_TS_Init(u16 XSize, u16 YSize);
void    K_TS_GetState(TS_StateTypeDef *TsState);
u8 K_TS_ITConfig(void);
u8 K_TS_ITGetStatus(void);
void    K_TS_ITClear(void);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_TOUCHSCREEN_H */
