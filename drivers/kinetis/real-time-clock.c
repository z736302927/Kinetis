
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/string.h>

#include <kinetis/design_verification.h>
#include <kinetis/real-time-clock.h>

#include <time.h>

#define HOURS24                         0x00

bool rtc_is_leap_year(u16 year)
{
	if (year < 100) {
		year += 2000;    // Handle 2-digit years
	}
	return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

int rtc_get_days_in_month(u16 year, u8 month)
{
	switch (month) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		return 31;
	case 4:
	case 6:
	case 9:
	case 11:
		return 30;
	case 2:
		return rtc_is_leap_year(year) ? 29 : 28;
	default:
		return 0; // Invalid month
	}
}

void rtc_format_time_string(struct tm *rtc, char *buffer, size_t buffer_size)
{
	if (!rtc || !buffer || buffer_size < 20) {
		return;
	}

	snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d",
		rtc->tm_year, rtc->tm_mon, rtc->tm_mday,
		rtc->tm_hour, rtc->tm_min, rtc->tm_sec);
}

void rtc_backup_reg_write(struct rtc_device *dev)
{
	if (dev->backup_reg_write) {
		dev->backup_reg_write();
	}
}

void rtc_backup_reg_read(struct rtc_device *dev, u32 *tmp)
{
	if (dev->backup_reg_read) {
		dev->backup_reg_read(tmp);
	}
}

bool rtc_validate_time_components(struct tm *rtc)
{
	if (!rtc) {
		return false;
	}

	// Basic range checks
	if (rtc->tm_year < 1970 || rtc->tm_year > 2099 ||
		rtc->tm_mon < 1 || rtc->tm_mon > 12 ||
		rtc->tm_mday < 1 || rtc->tm_mday > 31 ||
		rtc->tm_hour > 23 || rtc->tm_min > 59 || rtc->tm_sec > 59 ||
		rtc->tm_wday > 7) {
		return false;
	}

	// February validation (leap year)
	if (rtc->tm_mon == 2) {
		bool is_leap = (rtc->tm_year % 4 == 0);
		if (rtc->tm_mday > (is_leap ? 29 : 28)) {
			return false;
		}
	}

	// 30-day months validation
	if ((rtc->tm_mon == 4 || rtc->tm_mon == 6 ||
		rtc->tm_mon == 9 || rtc->tm_mon == 11) &&
		rtc->tm_mday > 30) {
		return false;
	}

	return true;
}

int rtc_calendar_set(struct rtc_device *dev, struct tm *rtc, u8 format)
{
	bool valid_time;
	char rtc_string[20];

	valid_time = rtc_validate_time_components(rtc);
	if (!valid_time) {
		rtc_format_time_string(rtc, rtc_string, sizeof(rtc_string));
		pr_err("rtc: invalid time %s components provided", rtc_string);
		return -EINVAL;
	}

	rtc_format_time_string(rtc, rtc_string, sizeof(rtc_string));
	pr_info("setting rtc time: %s", rtc_string);

	if (dev->calendar_set) {
		dev->calendar_set(rtc, format);
	}

	return 0;
}

void rtc_calendar_get(struct rtc_device *dev, struct tm *rtc, u8 format)
{
	char rtc_string[20];

	dev->calendar_get(rtc, format);
	rtc_format_time_string(rtc, rtc_string, sizeof(rtc_string));
	pr_info("getting rtc time: %s", rtc_string);
}

void rtc_set_time_format(struct rtc_device *dev, u8 tmp)
{
	dev->set_time_format(tmp);
}

u8 rtc_get_time_format(struct rtc_device *dev)
{
	return dev->get_time_format();
}

#ifdef DESIGN_VERIFICATION_RTC
#include "kinetis/test-kinetis.h"

#ifdef KINETIS_FAKE_SIM

void fake_rtc_calendar_get(struct tm *rtc, u8 format)
{
	time_t current_time;
	struct tm *sys_time;

	current_time = time(NULL);
	sys_time = localtime(&current_time);

	if (sys_time) {
		/* Convert system time to RTC format */
		rtc->tm_year = sys_time->tm_year + 1900;  /* tm_year is years since 1900 */
		rtc->tm_mon = sys_time->tm_mon + 1;        /* tm_mon is 0-11 */
		rtc->tm_mday = sys_time->tm_mday;
		rtc->tm_wday = sys_time->tm_wday;
		rtc->tm_hour = sys_time->tm_hour;
		rtc->tm_min = sys_time->tm_min;
		rtc->tm_sec = sys_time->tm_sec;
		rtc->tm_yday = sys_time->tm_yday;
	} else {
		pr_warn("rtc: failed to get system time");
	}
}

void fake_rtc_set_time_format(u8 tmp)
{

}

u8 fake_rtc_get_time_format(void)
{
	return 0;
}

struct rtc_device fake_rtc = {
	.backup_reg_write = NULL,
	.backup_reg_read = NULL,
	.calendar_set = NULL,
	.calendar_get = fake_rtc_calendar_get,
	.set_time_format = fake_rtc_set_time_format,
	.get_time_format = fake_rtc_get_time_format,
};
#endif

#ifdef USING_CHIP_RTC
void chip_rtc_backup_reg_write()
{
	/*##-3- Writes a data in a RTC Backup data Register1 #####################*/
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);
}

void chip_rtc_backup_reg_read(u32 *tmp)
{
	/*##-3- Read a data in a RTC Backup data Register1 #######################*/
	*tmp = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
}

void chip_rtc_calendar_set(struct tm *rtc, u8 format)
{
	RTC_DateTypeDef sdate;
	RTC_TimeTypeDef stime;

	/*##-1- Configure the Date ###############################################*/
	/* Set Date: Wednesday May 1th 2019 */
	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);
	}

	sdate.Year = rtc->tm_year;
	sdate.Month = rtc->tm_mon;
	sdate.Date = rtc->tm_mday;

	if (rtc->tm_wday != 0) {
		sdate.WeekDay = rtc->tm_wday;
	}

	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BCD);
	}

	/*##-2- Configure the Time ###############################################*/
	/* Set Time: 00:00:00 */
	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);
	}

	stime.Hours = rtc->tm_hour;
	stime.Minutes = rtc->tm_min;
	stime.Seconds = rtc->tm_sec;
	stime.TimeFormat = RTC_HOURFORMAT12_AM;
	stime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE ;
	stime.StoreOperation = RTC_STOREOPERATION_RESET;

	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BCD);
	}

	/*##-3- Writes a data in a RTC Backup data Register1 #####################*/
	rtc_backup_reg_write();
}

void chip_rtc_calendar_get(struct tm *rtc, u8 format)
{
	RTC_DateTypeDef sdate;
	RTC_TimeTypeDef stime;

	/* Get the RTC current Date */
	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);
	}

	rtc->tm_year = sdate.Year;
	rtc->tm_mon = sdate.Month;
	rtc->tm_mday = sdate.Date;

	if (rtc->tm_wday != 0) {
		rtc->tm_wday = sdate.WeekDay;
	}

	/* Get the RTC current Time */
	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);
	}

	rtc->tm_hour = stime.Hours;
	rtc->tm_min = stime.Minutes;
	rtc->tm_sec = stime.Seconds;
}

void chip_rtc_set_time_format(u8 tmp)
{
	/* Placeholder for chip RTC time format setting */
}

u8 chip_rtc_get_time_format(void)
{
	return 0;
}

struct rtc_device chip_rtc = {
	.backup_reg_write = chip_rtc_backup_reg_write,
	.backup_reg_read = chip_rtc_backup_reg_read,
	.calendar_set = chip_rtc_calendar_set,
	.calendar_get = chip_rtc_calendar_get,
	.set_time_format = chip_rtc_set_time_format,
	.get_time_format = chip_rtc_get_time_format,
};
#endif

#ifdef USING_DS3231

void ds3231_rtc_calendar_set(struct tm *rtc, u8 format)
{
	char time[13];

	/* Validate time components before formatting */
	if (rtc->tm_year > 2099 || rtc->tm_mon > 12 || rtc->tm_mday > 31 ||
		rtc->tm_hour > 23 || rtc->tm_min > 59 || rtc->tm_sec > 59) {
		pr_err("invalid time values");
		return;
	}

	snprintf(time, sizeof(time), "%02d%02d%02d%02d%02d%02d",
		rtc->tm_year, rtc->tm_mon, rtc->tm_mday,
		rtc->tm_hour, rtc->tm_min, rtc->tm_sec);
	ds3231_SetTimeWithString(time);

	if (rtc->tm_wday != 0) {
		ds3231_SetWeek(rtc->tm_wday);
	}
}

void ds3231_rtc_calendar_get(struct tm *rtc, u8 format)
{
	u8 time[6], week;

	if (format == KRTC_FORMAT_BIN) {
		ds3231_get_time(time, DS3231_FORMAT_BIN);
	} else {
		ds3231_get_time(time, DS3231_FORMAT_BCD);
	}

	ds3231_get_week(&week);
	rtc->tm_year = time[5];
	rtc->tm_mon = time[4];
	rtc->tm_mday = time[3];
	rtc->tm_wday = week;
	rtc->tm_hour = time[2];
	rtc->tm_min = time[1];
	rtc->tm_sec = time[0];
}

void ds3231_rtc_set_time_format(u8 tmp)
{
	/* Placeholder for DS3231 time format setting */
}

u8 ds3231_rtc_get_time_format(void)
{
	return 0;
}

struct rtc_device ds3231_rtc = {
	.backup_reg_write = NULL,
	.backup_reg_read = NULL,
	.calendar_set = ds3231_rtc_calendar_set,
	.calendar_get = ds3231_rtc_calendar_get,
	.set_time_format = ds3231_rtc_set_time_format,
	.get_time_format = ds3231_rtc_get_time_format,
};
#endif

int t_rtc_set_clock(int argc, char **argv)
{
	struct tm rtc;

	rtc.tm_year = get_random_u32() % 130 + 1970;
	rtc.tm_mon = get_random_u32() % 12 + 1;
	rtc.tm_mday = get_random_u32() % 28 + 1;
	rtc.tm_hour = get_random_u32() % 24;
	rtc.tm_min = get_random_u32() % 60;
	rtc.tm_sec = get_random_u32() % 60;
	rtc.tm_wday = get_random_u32() % 7;

	if (argc > 1) {
		rtc.tm_year = simple_strtoul(argv[1], &argv[1], 10);
	}

	if (argc > 2) {
		rtc.tm_mon = simple_strtoul(argv[2], &argv[2], 10);
	}

	if (argc > 3) {
		rtc.tm_mday = simple_strtoul(argv[3], &argv[3], 10);
	}

	if (argc > 4) {
		rtc.tm_hour = simple_strtoul(argv[4], &argv[4], 10);
	}

	if (argc > 5) {
		rtc.tm_min = simple_strtoul(argv[5], &argv[5], 10);
	}

	if (argc > 6) {
		rtc.tm_sec = simple_strtoul(argv[6], &argv[6], 10);
	}

	if (argc > 7) {
		rtc.tm_wday = simple_strtoul(argv[7], &argv[7], 10);
	}

	return rtc_calendar_set(&fake_rtc, &rtc, KRTC_FORMAT_BIN);
}

int t_rtc_get_clock(int argc, char **argv)
{
	struct tm rtc;

	rtc_calendar_get(&fake_rtc, &rtc, KRTC_FORMAT_BIN);

	return 0;
}

int t_rtc_validation(int argc, char **argv)
{
	struct tm test_times[8];
	bool test_results[8];
	int i;

	// Test 1: Valid time components, Leap year
	test_times[0].tm_year = 2024;
	test_times[0].tm_mon = 2;
	test_times[0].tm_mday = 29;
	test_times[0].tm_hour = 23;
	test_times[0].tm_min = 59;
	test_times[0].tm_sec = 59;
	test_times[0].tm_wday = 5;
	test_results[0] = rtc_validate_time_components(&test_times[0]);

	// Test 2: Invalid leap year
	test_times[1].tm_year = 2023;
	test_times[1].tm_mon = 2;
	test_times[1].tm_mday = 29;
	test_times[1].tm_hour = 23;
	test_times[1].tm_min = 59;
	test_times[1].tm_sec = 59;
	test_times[1].tm_wday = 5;
	test_results[1] = !rtc_validate_time_components(&test_times[1]);

	// Test 3: Valid regular date
	test_times[2].tm_year = 2024;
	test_times[2].tm_mon = 7;
	test_times[2].tm_mday = 15;
	test_times[2].tm_hour = 12;
	test_times[2].tm_min = 30;
	test_times[2].tm_sec = 45;
	test_times[2].tm_wday = 1;
	test_results[2] = rtc_validate_time_components(&test_times[2]);

	// Test 4: Month with 30 days
	test_times[3].tm_year = 2024;
	test_times[3].tm_mon = 4;
	test_times[3].tm_mday = 30;
	test_times[3].tm_hour = 10;
	test_times[3].tm_min = 20;
	test_times[3].tm_sec = 30;
	test_times[3].tm_wday = 2;
	test_results[3] = rtc_validate_time_components(&test_times[3]);

	// Test 5: Invalid time values, Invalid minutes
	test_times[4].tm_year = 2024;
	test_times[4].tm_mon = 1;
	test_times[4].tm_mday = 15;
	test_times[4].tm_hour = 25;
	test_times[4].tm_min = 70;
	test_times[4].tm_sec = 30;
	test_times[4].tm_wday = 1;
	test_results[4] = !rtc_validate_time_components(&test_times[4]);

	// Test 6: Minimum year (1970)
	test_times[5].tm_year = 1970;
	test_times[5].tm_mon = 1;
	test_times[5].tm_mday = 1;
	test_times[5].tm_hour = 0;
	test_times[5].tm_min = 0;
	test_times[5].tm_sec = 0;
	test_times[5].tm_wday = 4;
	test_results[5] = rtc_validate_time_components(&test_times[5]);

	// Test 7: Maximum month (12), Maximum day (31)
	test_times[6].tm_year = 2024;
	test_times[6].tm_mon = 12;
	test_times[6].tm_mday = 31;
	test_times[6].tm_hour = 23;
	test_times[6].tm_min = 59;
	test_times[6].tm_sec = 59;
	test_times[6].tm_wday = 2;
	test_results[6] = rtc_validate_time_components(&test_times[6]);

	// Test 8: Invalid year (1969 - below 1970)
	test_times[7].tm_year = 1969;
	test_times[7].tm_mon = 1;
	test_times[7].tm_mday = 1;
	test_times[7].tm_hour = 0;
	test_times[7].tm_min = 0;
	test_times[7].tm_sec = 0;
	test_times[7].tm_wday = 1;
	test_results[7] = !rtc_validate_time_components(&test_times[7]);

	// Run validation tests
	for (i = 0; i < 8; i++) {
		char time_str[32];
		rtc_format_time_string(&test_times[i], time_str, sizeof(time_str));

		if (test_results[i]) {
			pr_info("test %d: %s", i + 1, time_str);
		} else {
			pr_err("test %d: %s", i + 1, time_str);
			return -EINVAL;
		}
	}

	// Test leap year function
	if (rtc_is_leap_year(2024) && !rtc_is_leap_year(2023)) {
		pr_info("leap year detection working");
	} else {
		pr_err("leap year detection failed");
		return -EINVAL;
	}
	// Test days in month function
	if (rtc_get_days_in_month(2024, 2) == 29 &&
		rtc_get_days_in_month(2023, 2) == 28 &&
		rtc_get_days_in_month(2024, 4) == 30) {
		pr_info("days in month calculation working");
	} else {
		pr_err("days in month calculation failed");
		return -EINVAL;
	}

	return 0;
}

int t_rtc_backup(int argc, char **argv)
{
	u32 test_values[] = {0x12345678, 0xABCDEF00, 0x11223344, 0xFFEEDDCC, 0x55AA5533};
	u32 read_values[5];
	int i, errors = 0;

	for (i = 0; i < 5; i++) {
		u32 backup_value;

		rtc_backup_reg_write(&fake_rtc);

		rtc_backup_reg_read(&fake_rtc, &read_values[i]);

		if (read_values[i] == 0x32F2) { // Default value + written value
			pr_info("backup test %d: 0x%08x", i + 1, read_values[i]);
		} else {
			pr_err("backup test %d: expected 0x32f2, got 0x%08x",
				i + 1, read_values[i]);
			errors++;
		}
	}

	if (errors == 0) {
		pr_info("all backup register tests passed");
	} else {
		pr_err("%d out of %d backup register tests failed", errors, 5);
	}

	return errors == 0 ? 0 : -1;
}

#endif
