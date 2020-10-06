#ifndef __K_RTCTASK_H
#define __K_RTCTASK_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core/core_common.h"

typedef struct RTCTask_DateTime_TypeDef {
    uint8_t Year;
    uint8_t Month;
    uint8_t Date;
    uint8_t Hours;
    uint8_t Minutes;
    uint8_t Seconds;
} RTCTask_DateTime_TypeDef;

typedef struct RTCTask_DeltaT_TypeDef {
    uint8_t Hours;
    uint8_t Minutes;
    uint8_t Seconds;
} RTCTask_DeltaT_TypeDef;

typedef struct RTCTask_TypeDef {
    RTCTask_DateTime_TypeDef ExpiredTime;
    RTCTask_DeltaT_TypeDef DeltaTime;
    void (*Timeout_cb)(void);
    struct RTCTask_TypeDef *Next;
} RTCTask_TypeDef;

void RTCTask_Deinit(struct RTCTask_TypeDef *Handle);
void RTCTask_Init(struct RTCTask_TypeDef *Handle, void(*Timeout_cb)(), uint8_t AddHours, uint8_t AddMinutes, uint8_t AddSeconds);
int  RTCTask_Start(struct RTCTask_TypeDef *Handle);
void RTCTask_Stop(struct RTCTask_TypeDef *Handle);
void RTCTask_GetCurrentTime(uint8_t TmpYear, uint8_t TmpMonth, uint8_t TmpDate, uint8_t TmpHours, uint8_t TmpMinutes, uint8_t TmpSeconds);
void RTCTask_Loop(void);
/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __K_RTCTASK_H */
