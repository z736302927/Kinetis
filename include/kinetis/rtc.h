#ifndef __K_RTC_H
#define __K_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

#include "kinetis/core_common.h"

void RTC_CalendarConfig(u8 Year, u8 Month, u8 Date,
    u8 Hours, u8 Minutes, u8 Seconds, u8 WeekDay, u8 Format);
void RTC_CalendarShow(u8 *Year, u8 *Month, u8 *Date,
    u8 *Hours, u8 *Minutes, u8 *Seconds, u8 *WeekDay, u8 Format);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define KRTC_FORMAT_BIN                     0x00000000U
#define KRTC_FORMAT_BCD                     0x00000001U

#ifdef __cplusplus
}
#endif

#endif /* __K_RTC_H */
