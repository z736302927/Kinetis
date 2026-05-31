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

struct rtc_device {
	void (*backup_reg_write)(void);
	void (*backup_reg_read)(u32 *tmp);
	void (*calendar_set)(struct tm *rtc, u8 format);
	void (*calendar_get)(struct tm *rtc, u8 format);
	void (*set_time_format)(u8 tmp);
	u8(*get_time_format)(void);
};

int rtc_calendar_set(struct rtc_device *dev, struct tm *rtc, u8 format);
void rtc_calendar_get(struct rtc_device *dev, struct tm *rtc, u8 format);
void rtc_set_time_format(struct rtc_device *dev, u8 tmp);
u8 rtc_get_time_format(struct rtc_device *dev);

#ifdef USING_CHIP_RTC
extern struct rtc_device chip_rtc;
#endif

#ifdef USING_DS3231
extern struct rtc_device ds3231_rtc;
#endif

extern struct rtc_device general_rtc;

static inline char *get_rtc_string(struct rtc_device *dev)
{
	struct tm rtc;
	static char buf[64];

	rtc_calendar_get(dev, &rtc, KRTC_FORMAT_BIN);

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
