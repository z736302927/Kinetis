#ifndef __K_RTC_H
#define __K_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define KRTC_FORMAT_BIN                     0x00000000U
#define KRTC_FORMAT_BCD                     0x00000001U

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/time.h>
#include <linux/kernel.h>

void rtc_calendar_set(struct tm *rtc, u8 format);
void rtc_calendar_get(struct tm *rtc, u8 format);

static char *get_rtc_string(void)
{
	struct tm rtc;
	static char buf[64];
	
	rtc_calendar_get(&rtc, KRTC_FORMAT_BIN);
	
	sprintf(buf, "%ld-%d-%d %d:%d:%d",
		rtc.tm_year, rtc.tm_mon, rtc.tm_mday,
		rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
	
	return buf;
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_RTC_H */
