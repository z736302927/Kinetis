
#include <generated/deconfig.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/string.h>

#include <kinetis/design_verification.h>
#include <kinetis/real-time-clock.h>


/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

//#define USING_CHIP_RTC
//#define USING_DS3231

#define HOURS24                         0x00

// Enhanced RTC management
static bool rtc_performance_monitoring = false;
static struct rtc_stats {
	u32 total_time_reads;
	u32 total_time_sets;
	u32 total_backup_reads;
	u32 total_backup_writes;
	u64 total_read_time_ms;
	u64 total_write_time_ms;
	u32 max_read_time_ms;
	u32 max_write_time_ms;
	ktime_t system_start_time;
} rtc_statistics = {0};

// RTC validation and error tracking
static u32 rtc_validation_errors = 0;
static u32 rtc_last_error_code = 0;

void rtc_backup_reg_write(void)
{
	ktime_t start_time;

	if (rtc_performance_monitoring)
		start_time = ktime_get();

#ifdef USING_CHIP_RTC
	/*##-3- Writes a data in a RTC Backup data Register1 #####################*/
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);

	if (rtc_performance_monitoring) {
		u64 write_time = ktime_to_ms(ktime_sub(ktime_get(), start_time));
		rtc_statistics.total_backup_writes++;
		rtc_statistics.total_write_time_ms += write_time;
		if (write_time > rtc_statistics.max_write_time_ms)
			rtc_statistics.max_write_time_ms = write_time;

		pr_debug("RTC backup write in %llu ms", write_time);
	}
#endif
}

void rtc_backup_reg_read(u32 *tmp)
{
	ktime_t start_time;

	if (rtc_performance_monitoring)
		start_time = ktime_get();

#ifdef USING_CHIP_RTC
	/*##-3- Read a data in a RTC Backup data Register1 #######################*/
	*tmp = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);

	if (rtc_performance_monitoring) {
		u64 read_time = ktime_to_ms(ktime_sub(ktime_get(), start_time));
		rtc_statistics.total_backup_reads++;
		rtc_statistics.total_read_time_ms += read_time;
		if (read_time > rtc_statistics.max_read_time_ms)
			rtc_statistics.max_read_time_ms = read_time;

		pr_debug("RTC backup read in %llu ms", read_time);
	}
#endif
}

// Enhanced RTC utility functions
bool rtc_validate_time_components(struct tm *rtc)
{
	if (!rtc)
		return false;

	// Basic range checks
	if (rtc->tm_year < 1970 || rtc->tm_year > 99 ||
		rtc->tm_mon < 1 || rtc->tm_mon > 12 ||
		rtc->tm_mday < 1 || rtc->tm_mday > 31 ||
		rtc->tm_hour > 23 || rtc->tm_min > 59 || rtc->tm_sec > 59 ||
		rtc->tm_wday > 7)
		return false;

	// February validation (leap year)
	if (rtc->tm_mon == 2) {
		bool is_leap = (rtc->tm_year % 4 == 0);
		if (rtc->tm_mday > (is_leap ? 29 : 28))
			return false;
	}

	// 30-day months validation
	if ((rtc->tm_mon == 4 || rtc->tm_mon == 6 ||
			rtc->tm_mon == 9 || rtc->tm_mon == 11) &&
		rtc->tm_mday > 30)
		return false;

	return true;
}

void rtc_calendar_set(struct tm *rtc, u8 format)
{
	ktime_t start_time;
	bool valid_time;

	if (rtc_performance_monitoring)
		start_time = ktime_get();

	/* Enhanced time validation */
	valid_time = rtc_validate_time_components(rtc);
	if (!valid_time) {
		rtc_validation_errors++;
		rtc_last_error_code = 0x01; // Invalid time components
		pr_err("RTC: Invalid time components provided");
		return;
	}

#ifdef USING_CHIP_RTC
	RTC_DateTypeDef sdate;
	RTC_TimeTypeDef stime;

	/*##-1- Configure the Date ###############################################*/
	/* Set Date: Wednesday May 1th 2019 */
	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	else
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);

	sdate.Year = rtc->tm_year;
	sdate.Month = rtc->tm_mon;
	sdate.Date = rtc->tm_mday;

	if (rtc->tm_wday != 0)
		sdate.WeekDay = rtc->tm_wday;

	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	else
		HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BCD);

	/*##-2- Configure the Time ###############################################*/
	/* Set Time: 00:00:00 */
	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	else
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);

	stime.Hours = rtc->tm_hour;
	stime.Minutes = rtc->tm_min;
	stime.Seconds = rtc->tm_sec;
	stime.TimeFormat = RTC_HOURFORMAT12_AM;
	stime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE ;
	stime.StoreOperation = RTC_STOREOPERATION_RESET;

	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	else
		HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BCD);

	/*##-3- Writes a data in a RTC Backup data Register1 #####################*/
	rtc_backup_reg_write();

	if (rtc_performance_monitoring) {
		u64 write_time = ktime_to_ms(ktime_sub(ktime_get(), start_time));
		rtc_statistics.total_time_sets++;
		rtc_statistics.total_write_time_ms += write_time;
		if (write_time > rtc_statistics.max_write_time_ms)
			rtc_statistics.max_write_time_ms = write_time;

		pr_debug("RTC time set in %llu ms", write_time);
	}
#endif

#ifdef USING_DS3231
	char time[13];

	/* Validate time components before formatting */
	if (rtc->tm_year > 99 || rtc->tm_mon > 12 || rtc->tm_mday > 31 ||
		rtc->tm_hour > 23 || rtc->tm_min > 59 || rtc->tm_sec > 59) {
		pr_err("Invalid time values");
		return;
	}

	snprintf(time, sizeof(time), "%02d%02d%02d%02d%02d%02d",
		rtc->tm_year, rtc->tm_mon, rtc->tm_mday,
		rtc->tm_hour, rtc->tm_min, rtc->tm_sec);
	ds3231_SetTimeWithString(time);

	if (rtc->tm_wday != 0)
		ds3231_SetWeek(rtc->tm_wday);

#endif
}

void rtc_calendar_get(struct tm *rtc, u8 format)
{
	ktime_t start_time;

	if (rtc_performance_monitoring)
		start_time = ktime_get();

#ifdef USING_CHIP_RTC
	RTC_DateTypeDef sdate;
	RTC_TimeTypeDef stime;

	/* Get the RTC current Date */
	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	else
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);

	rtc->tm_year = sdate.Year;
	rtc->tm_mon = sdate.Month;
	rtc->tm_mday = sdate.Date;

	if (rtc->tm_wday != 0)
		rtc->tm_wday = sdate.WeekDay;

	/* Get the RTC current Time */
	if (format == KRTC_FORMAT_BIN)
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	else
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);

	rtc->tm_hour = stime.Hours;
	rtc->tm_min = stime.Minutes;
	rtc->tm_sec = stime.Seconds;

	if (rtc_performance_monitoring) {
		u64 read_time = ktime_to_ms(ktime_sub(ktime_get(), start_time));
		rtc_statistics.total_time_reads++;
		rtc_statistics.total_read_time_ms += read_time;
		if (read_time > rtc_statistics.max_read_time_ms)
			rtc_statistics.max_read_time_ms = read_time;

		pr_debug("RTC time read in %llu ms", read_time);
	}
#endif

#ifdef USING_DS3231
	u8 time[6], week;

	if (format == KRTC_FORMAT_BIN)
		ds3231_ReadTime(time, DS3231_FORMAT_BIN);
	else
		ds3231_ReadTime(time, DS3231_FORMAT_BCD);

	ds3231_ReadWeek(&week);
	rtc->tm_year = time[5];
	rtc->tm_mon = time[4];
	rtc->tm_mday = time[3];
	rtc->tm_wday = week;
	rtc->tm_hour = time[2];
	rtc->tm_min = time[1];
	rtc->tm_sec = time[0];
#endif
}

/**
  * @brief  Configure the current time and date.
  * @param  None
  * @retval None
  */
void rtc_set_time_format(u8 tmp)
{
#ifdef USING_CHIP_RTC

#endif

#ifdef USING_DS3231

#endif
}

/**
  * @brief  Configure the current time and date.
  * @param  None
  * @retval None
  */
u8 rtc_get_time_format(void)
{
#ifdef USING_CHIP_RTC

#endif

#ifdef USING_DS3231

#endif
	return 0;
}

void rtc_enable_performance_monitoring(bool enable)
{
	rtc_performance_monitoring = enable;
	if (enable) {
		rtc_statistics.system_start_time = ktime_get();
		pr_info("RTC performance monitoring enabled");
	}
}

void rtc_get_statistics(struct rtc_stats *stats)
{
	if (stats)
		*stats = rtc_statistics;
}

void rtc_reset_statistics(void)
{
	memset(&rtc_statistics, 0, sizeof(rtc_statistics));
	rtc_statistics.system_start_time = ktime_get();
	rtc_validation_errors = 0;
	rtc_last_error_code = 0;
	pr_info("RTC statistics reset");
}

void rtc_print_performance_report(void)
{
	u64 uptime = ktime_to_ms(ktime_sub(ktime_get(), rtc_statistics.system_start_time));
	u64 avg_read_time = rtc_statistics.total_time_reads > 0 ?
		rtc_statistics.total_read_time_ms / rtc_statistics.total_time_reads : 0;
	u64 avg_write_time = rtc_statistics.total_time_sets > 0 ?
		rtc_statistics.total_write_time_ms / rtc_statistics.total_time_sets : 0;

	pr_info("=== RTC Performance Report ===");
	pr_info("System uptime: %llu ms", uptime);
	pr_info("Time reads: %u", rtc_statistics.total_time_reads);
	pr_info("Time sets: %u", rtc_statistics.total_time_sets);
	pr_info("Backup reads: %u", rtc_statistics.total_backup_reads);
	pr_info("Backup writes: %u", rtc_statistics.total_backup_writes);
	pr_info("Validation errors: %u", rtc_validation_errors);
	pr_info("Avg read time: %llu ms", avg_read_time);
	pr_info("Avg write time: %llu ms", avg_write_time);
	pr_info("Max read time: %u ms", rtc_statistics.max_read_time_ms);
	pr_info("Max write time: %u ms", rtc_statistics.max_write_time_ms);
	pr_info("===============================");
}

bool rtc_is_leap_year(u16 year)
{
	if (year < 100)
		year += 2000; // Handle 2-digit years
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
	if (!rtc || !buffer || buffer_size < 20)
		return;

	snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d",
		rtc->tm_year + 2000, rtc->tm_mon, rtc->tm_mday,
		rtc->tm_hour, rtc->tm_min, rtc->tm_sec);
}

#ifdef DESIGN_VERIFICATION_RTC
#include "kinetis/test-kinetis.h"

#include <linux/printk.h>
#include <linux/random.h>

static int rtc_test_operation_count = 0;

void rtc_test_callback(void)
{
	rtc_test_operation_count++;
	pr_debug("RTC test operation #%d completed", rtc_test_operation_count);
}

int t_rtc_set_clock(int argc, char **argv)
{
	struct tm rtc;
	ktime_t start_time;

	pr_info("=== RTC Set Clock Basic Test ===");

	if (rtc_performance_monitoring)
		start_time = ktime_get();

	rtc.tm_year = get_random_u32() % 100;
	rtc.tm_mon = get_random_u32() % 12;
	rtc.tm_mday = get_random_u32() % 28;
	rtc.tm_hour = get_random_u32() % 24;
	rtc.tm_min = get_random_u32() % 60;
	rtc.tm_sec = get_random_u32() % 60;
	rtc.tm_wday = get_random_u32() % 7;

	if (argc > 1)
		rtc.tm_year = simple_strtoul(argv[1], &argv[1], 10);

	if (argc > 2)
		rtc.tm_mon = simple_strtoul(argv[2], &argv[2], 10);

	if (argc > 3)
		rtc.tm_mday = simple_strtoul(argv[3], &argv[3], 10);

	if (argc > 4)
		rtc.tm_hour = simple_strtoul(argv[4], &argv[4], 10);

	if (argc > 5)
		rtc.tm_min = simple_strtoul(argv[5], &argv[5], 10);

	if (argc > 6)
		rtc.tm_sec = simple_strtoul(argv[6], &argv[6], 10);

	if (argc > 7)
		rtc.tm_wday = simple_strtoul(argv[7], &argv[7], 10);

	pr_info("Setting RTC time: %s", get_rtc_string());
	rtc_calendar_set(&rtc, KRTC_FORMAT_BIN);

	if (rtc_performance_monitoring) {
		u64 set_time = ktime_to_ms(ktime_sub(ktime_get(), start_time));
		pr_info("RTC set operation completed in %llu ms", set_time);
	}

	return PASS;
}

int t_rtc_get_clock(int argc, char **argv)
{
	struct tm rtc;
	ktime_t start_time;

	pr_info("=== RTC Get Clock Basic Test ===");

	if (rtc_performance_monitoring)
		start_time = ktime_get();

	rtc_calendar_get(&rtc, KRTC_FORMAT_BIN);
	pr_info("Getting RTC time: %s", get_rtc_string());

	if (rtc_performance_monitoring) {
		u64 get_time = ktime_to_ms(ktime_sub(ktime_get(), start_time));
		pr_info("RTC get operation completed in %llu ms", get_time);
	}

	return PASS;
}

int t_rtc_validation(int argc, char **argv)
{
	struct tm test_times[5];
	bool test_results[5];
	int i;

	pr_info("=== RTC Validation Test ===");

	// Reset statistics
	rtc_reset_statistics();
	rtc_enable_performance_monitoring(true);

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

	// Run validation tests
	for (i = 0; i < 5; i++) {
		char time_str[32];
		rtc_format_time_string(&test_times[i], time_str, sizeof(time_str));

		if (test_results[i])
			pr_info("PASS - Test %d: %s", i + 1, time_str);
		else
			pr_err("FAIL - Test %d: %s", i + 1, time_str);
	}

	// Test leap year function
	if (rtc_is_leap_year(2024) && !rtc_is_leap_year(2023))
		pr_info("PASS - Leap year detection working");
	else
		pr_err("FAIL - Leap year detection failed");
	// Test days in month function
	if (rtc_get_days_in_month(2024, 2) == 29 &&
		rtc_get_days_in_month(2023, 2) == 28 &&
		rtc_get_days_in_month(2024, 4) == 30)
		pr_info("PASS - Days in month calculation working");
	else
		pr_err("FAIL - Days in month calculation failed");
}

int t_rtc_performance(int argc, char **argv)
{
	int i, ret;
	u64 start_time, end_time;
	struct tm test_time;

	pr_info("=== RTC Performance Test ===");

	// Reset and enable monitoring
	rtc_reset_statistics();
	rtc_enable_performance_monitoring(true);

	start_time = ktime_get();

	// Test backup register performance
	for (i = 0; i < 100; i++) {
		u32 backup_value;
		rtc_backup_reg_write();
		rtc_backup_reg_read(&backup_value);
	}

	end_time = ktime_get();
	pr_info("100 backup operations completed in %llu ms",
		ktime_to_ms(ktime_sub(end_time, start_time)));

	// Test time setting performance
	start_time = ktime_get();
	test_time.tm_year = 2024;
	test_time.tm_mon = 1;
	test_time.tm_mday = 1;
	test_time.tm_hour = 0;
	test_time.tm_min = 0;
	test_time.tm_sec = 0;
	test_time.tm_wday = 1;

	for (i = 0; i < 50; i++) {
		test_time.tm_sec = i % 60;
		rtc_calendar_set(&test_time, KRTC_FORMAT_BIN);
	}

	end_time = ktime_get();
	pr_info("50 time set operations completed in %llu ms",
		ktime_to_ms(ktime_sub(end_time, start_time)));

	// Test time getting performance
	start_time = ktime_get();
	for (i = 0; i < 50; i++)
		rtc_calendar_get(&test_time, KRTC_FORMAT_BIN);
	end_time = ktime_get();
	pr_info("50 time get operations completed in %llu ms",
		ktime_to_ms(ktime_sub(end_time, start_time)));

	rtc_print_performance_report();
	return PASS;
}

int t_rtc_backup(int argc, char **argv)
{
	u32 test_values[] = {0x12345678, 0xABCDEF00, 0x11223344, 0xFFEEDDCC, 0x55AA5533};
	u32 read_values[5];
	int i, errors = 0;

	pr_info("=== RTC Backup Register Test ===");

	// Reset statistics
	rtc_reset_statistics();
	rtc_enable_performance_monitoring(true);

	// Test backup write and read
	for (i = 0; i < 5; i++) {
		u32 backup_value;

		// Write test value
		rtc_backup_reg_write();

		// Read back the value
		rtc_backup_reg_read(&read_values[i]);

		if (read_values[i] == 0x32F2)   // Default value + written value
			pr_info("PASS - Backup test %d: 0x%08X", i + 1, read_values[i]);
		else {
			pr_err("FAIL - Backup test %d: expected 0x32F2, got 0x%08X",
				i + 1, read_values[i]);
			errors++;
		}
	}

	if (errors == 0)
		pr_info("PASS - All backup register tests passed");
	else
		pr_err("FAIL - %d out of %d backup register tests failed", errors, 5);
	rtc_print_performance_report();
	return errors == 0 ? PASS : FAIL;
}

int t_rtc_cleanup(int argc, char **argv)
{
	struct rtc_stats stats;

	pr_info("=== RTC Cleanup Test ===");

	// Get final statistics
	rtc_get_statistics(&stats);

	pr_info("RTC cleanup summary:");
	pr_info("  Total operations: %u", stats.total_time_reads + stats.total_time_sets);
	pr_info("  Validation errors: %u", rtc_validation_errors);
	pr_info("  Last error code: 0x%02X", rtc_last_error_code);

	// Reset all statistics
	rtc_reset_statistics();
	rtc_enable_performance_monitoring(false);

	pr_info("PASS - RTC cleanup completed");
	return PASS;
}

#endif
