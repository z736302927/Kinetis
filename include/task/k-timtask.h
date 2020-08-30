#ifndef __K_TIMTASK_H
#define __K_TIMTASK_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core/core_common.h"

typedef struct TimTask_TypeDef
{
    uint32_t Timeout;
    uint32_t Repeat;
    void (*Timeout_cb)(void);
    struct TimTask_TypeDef *Next;
} TimTask_TypeDef;

void TimTask_Deinit(struct TimTask_TypeDef *Handle);
void TimTask_Init(struct TimTask_TypeDef *Handle, void(*Timeout_cb)(), uint32_t Timeout, uint32_t Repeat);
int  TimTask_Start(struct TimTask_TypeDef *Handle);
void TimTask_Stop(struct TimTask_TypeDef *Handle);
void TimTask_Ticks(void);
void TimTask_Loop(void);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef __cplusplus
}
#endif

#endif /* __K_TIMTASK_H */
