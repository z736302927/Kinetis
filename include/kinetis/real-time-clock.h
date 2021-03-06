#ifndef __K_RTC_H
#define __K_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/time.h>

#include "kinetis/core_common.h"

void rtc_calendar_set(struct tm *rtc, u8 format);
void rtc_calendar_get(struct tm *rtc, u8 format);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define KRTC_FORMAT_BIN                     0x00000000U
#define KRTC_FORMAT_BCD                     0x00000001U

#ifdef __cplusplus
}
#endif

#endif /* __K_RTC_H */
