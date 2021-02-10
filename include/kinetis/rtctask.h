#ifndef __K_RTCTASK_H
#define __K_RTCTASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"

typedef struct RTCTask_DateTime_TypeDef {
    u8 Year;
    u8 Month;
    u8 Date;
    u8 Hours;
    u8 Minutes;
    u8 Seconds;
} RTCTask_DateTime_TypeDef;

typedef struct RTCTask_DeltaT_TypeDef {
    u8 Hours;
    u8 Minutes;
    u8 Seconds;
} RTCTask_DeltaT_TypeDef;

typedef struct RTCTask_TypeDef {
    RTCTask_DateTime_TypeDef ExpiredTime;
    RTCTask_DeltaT_TypeDef DeltaTime;
    void (*Timeout_cb)(void);
    struct RTCTask_TypeDef *Next;
} RTCTask_TypeDef;

void RTCTask_Deinit(struct RTCTask_TypeDef *Handle);
void RTCTask_Init(struct RTCTask_TypeDef *Handle, void(*Timeout_cb)(), u8 AddHours, u8 AddMinutes, u8 AddSeconds);
int  RTCTask_Start(struct RTCTask_TypeDef *Handle);
void RTCTask_Stop(struct RTCTask_TypeDef *Handle);
void RTCTask_GetCurrentTime(u8 TmpYear, u8 TmpMonth, u8 TmpDate, u8 TmpHours, u8 TmpMinutes, u8 TmpSeconds);
void RTCTask_Loop(void);
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_RTCTASK_H */
