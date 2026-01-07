#ifndef __NB_TIMER_H
#define __NB_TIMER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include "kinetis/core_common.h"
#include "nb_bc95.h"

void NB_IOT_SetTim(NB_TimeoutCallback cb);

void NB_IOT_StartTim(u32 ms);

void NB_IOT_StopTim(void);

void NB_IOT_PollTim(void);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif   /* __NB_TIMER_H */
