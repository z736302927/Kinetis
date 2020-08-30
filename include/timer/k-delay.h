#ifndef __K_DELAY_H
#define __K_DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core_common.h"

#define DELAY_32BIT_TIMER

#ifdef DELAY_32BIT_TIMER
#define DELAY_TIMER_UNIT              (uint32_t)4294967295uL
#else
#define DELAY_TIMER_UNIT              (uint16_t)65535u
#endif

void Delay_EnableTimer(void);
void Delay_DisableTimer(void);
void Delay_Init(void);
void Delay_SetFlag(void);
void Delay_ClearFlag(void);
void Delay_us(uint32_t Delay);
void Delay_ms(uint32_t Delay);
void Delay_s(uint32_t Delay);
/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __K_DELAY_H */
