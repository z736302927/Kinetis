#ifndef __K_RTC_H
#define __K_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core/core_common.h"

void RTC_CalendarConfig(uint8_t Year, uint8_t Month, uint8_t Date,
    uint8_t Hours, uint8_t Minutes, uint8_t Seconds, uint8_t WeekDay, uint8_t Format);
void RTC_CalendarShow(uint8_t *Year, uint8_t *Month, uint8_t *Date,
    uint8_t *Hours, uint8_t *Minutes, uint8_t *Seconds, uint8_t *WeekDay, uint8_t Format);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define KRTC_FORMAT_BIN                     0x00000000U
#define KRTC_FORMAT_BCD                     0x00000001U

#ifdef __cplusplus
}
#endif

#endif /* __K_RTC_H */
