#include <generated/deconfig.h>
#include <linux/errno.h>
#include <linux/ktime.h>
#include <linux/printk.h>
#include <linux/string.h>

#include <kinetis/design_verification.h>
#include <kinetis/rtc-task.h>

static LIST_HEAD(rtc_task_head);
static LIST_HEAD(rtc_task_suspend_head);
static struct tm current_time = {.tm_year = 0};

static bool rtc_task_expired(struct rtc_task *rtc_task)
{
	if (current_time.tm_year != rtc_task->expired.tm_year)
		return time_after32(current_time.tm_year, rtc_task->expired.tm_year);
	if (current_time.tm_mon != rtc_task->expired.tm_mon)
		return time_after32(current_time.tm_mon, rtc_task->expired.tm_mon);
	if (current_time.tm_mday != rtc_task->expired.tm_mday)
		return time_after32(current_time.tm_mday, rtc_task->expired.tm_mday);
	if (current_time.tm_hour != rtc_task->expired.tm_hour)
		return time_after32(current_time.tm_hour, rtc_task->expired.tm_hour);
	if (current_time.tm_min != rtc_task->expired.tm_min)
		return time_after32(current_time.tm_min, rtc_task->expired.tm_min);
	if (current_time.tm_sec != rtc_task->expired.tm_sec)
		return time_after32(current_time.tm_sec, rtc_task->expired.tm_sec);
	return true;
}

static void rtc_task_special_add_days(struct tm *date_time)
{
	if (date_time->tm_mon == 2) {
		if (date_time->tm_year % 4 == 0) {
			if (date_time->tm_mday < 30) return;
			date_time->tm_mday -= 29;
			++date_time->tm_mon;
		} else {
			if (date_time->tm_mday < 29) return;
			date_time->tm_mday -= 28;
			++date_time->tm_mon;
		}
	}
	if (date_time->tm_mon == 4 || date_time->tm_mon == 6 ||
		date_time->tm_mon == 9 || date_time->tm_mon == 11) {
		if (date_time->tm_mday < 31) return;
		date_time->tm_mday -= 30;
		++date_time->tm_mon;
	}
	if (date_time->tm_mon == 1 || date_time->tm_mon == 3 ||
		date_time->tm_mon == 5 || date_time->tm_mon == 7 ||
		date_time->tm_mon == 8 || date_time->tm_mon == 10 ||
		date_time->tm_mon == 12) {
		if (date_time->tm_mday < 32) return;
		date_time->tm_mday -= 31;
		++date_time->tm_mon;
	}
	if (date_time->tm_mon < 13) return;
	date_time->tm_mon -= 12;
	++date_time->tm_year;
}

static void rtc_task_time_add_seconds(struct tm *date_time, u8 seconds)
{
	if (seconds > 60) return;
	date_time->tm_sec += seconds;
	if (date_time->tm_sec < 60) return;
	date_time->tm_sec -= 60;
	++date_time->tm_min;
	if (date_time->tm_min < 60) return;
	date_time->tm_min -= 60;
	++date_time->tm_hour;
	if (date_time->tm_hour < 24) return;
	date_time->tm_hour -= 24;
	++date_time->tm_mday;
	rtc_task_special_add_days(date_time);
}

static void rtc_task_time_add_minutes(struct tm *date_time, u8 minutes)
{
	if (minutes > 60) return;
	date_time->tm_min += minutes;
	if (date_time->tm_min < 60) return;
	date_time->tm_min -= 60;
	++date_time->tm_hour;
	if (date_time->tm_hour < 24) return;
	date_time->tm_hour -= 24;
	++date_time->tm_mday;
	rtc_task_special_add_days(date_time);
}

static void rtc_task_time_add_hours(struct tm *date_time, u8 hours)
{
	if (hours > 24) return;
	date_time->tm_hour += hours;
	if (date_time->tm_hour < 24) return;
	date_time->tm_hour -= 24;
	++date_time->tm_mday;
	rtc_task_special_add_days(date_time);
}

static void rtc_task_time_add_days(struct tm *date_time, u8 days)
{
	while (days-- > 0) {
		++date_time->tm_mday;
		rtc_task_special_add_days(date_time);
	}
}

static void rtc_task_time_add_months(struct tm *date_time, u8 months)
{
	if (months > 12) return;
	date_time->tm_mon += months;
	if (date_time->tm_mon < 13) return;
	date_time->tm_mon -= 12;
	++date_time->tm_year;
}

static void rtc_task_time_add_year(struct tm *date_time, u8 years)
{
	date_time->tm_year += years;
}

static void rtc_task_update_time(struct rtc_task *rtc_task)
{
	rtc_task->expired.tm_year = current_time.tm_year;
	rtc_task->expired.tm_mon = current_time.tm_mon;
	rtc_task->expired.tm_mday = current_time.tm_mday;
	rtc_task->expired.tm_hour = current_time.tm_hour;
	rtc_task->expired.tm_min = current_time.tm_min;
	rtc_task->expired.tm_sec = current_time.tm_sec;

	if (rtc_task->interval.tm_year != 0) {
		rtc_task->expired.tm_sec = 0; rtc_task->expired.tm_min = 0;
		rtc_task->expired.tm_hour = 0; rtc_task->expired.tm_mday = 0;
		rtc_task->expired.tm_mon = 0;
		rtc_task->expired.tm_year -= rtc_task->expired.tm_year % rtc_task->interval.tm_year;
		rtc_task_time_add_year(&rtc_task->expired, rtc_task->interval.tm_year);
	}
	if (rtc_task->interval.tm_mon != 0) {
		rtc_task->expired.tm_sec = 0; rtc_task->expired.tm_min = 0;
		rtc_task->expired.tm_hour = 0; rtc_task->expired.tm_mday = 0;
		rtc_task->expired.tm_mon -= rtc_task->expired.tm_mon % rtc_task->interval.tm_mon;
		rtc_task_time_add_months(&rtc_task->expired, rtc_task->interval.tm_mon);
	}
	if (rtc_task->interval.tm_mday != 0) {
		rtc_task->expired.tm_sec = 0; rtc_task->expired.tm_min = 0;
		rtc_task->expired.tm_hour = 0;
		rtc_task->expired.tm_mday -= rtc_task->expired.tm_mday % rtc_task->interval.tm_mday;
		rtc_task_time_add_days(&rtc_task->expired, rtc_task->interval.tm_mday);
	}
	if (rtc_task->interval.tm_hour != 0) {
		rtc_task->expired.tm_sec = 0; rtc_task->expired.tm_min = 0;
		rtc_task->expired.tm_hour -= rtc_task->expired.tm_hour % rtc_task->interval.tm_hour;
		rtc_task_time_add_hours(&rtc_task->expired, rtc_task->interval.tm_hour);
	}
	if (rtc_task->interval.tm_min != 0) {
		rtc_task->expired.tm_sec = 0;
		rtc_task->expired.tm_min -= rtc_task->expired.tm_min % rtc_task->interval.tm_min;
		rtc_task_time_add_minutes(&rtc_task->expired, rtc_task->interval.tm_min);
	}
	if (rtc_task->interval.tm_sec != 0)
		rtc_task_time_add_seconds(&rtc_task->expired, rtc_task->interval.tm_sec);
}

int rtc_task_add(struct rtc_task *rtc_task,
	u16 add_year, u8 add_month, u8 add_date,
	u8 add_hours, u8 add_minutes, u8 add_seconds,
	bool auto_load, void(*callback)(struct rtc_task *))
{
	if (!rtc_task)
		return -EINVAL;
	if (add_year > 9999 || add_month > 12 || add_date > 31 ||
		add_hours > 23 || add_minutes > 59 || add_seconds > 59)
		return -EINVAL;
	if (current_time.tm_year == 0)
		return -ETIMEDOUT;
	if (!callback)
		return -EINVAL;

	rtc_task->callback = callback;
	rtc_task->interval.tm_year  = add_year;
	rtc_task->interval.tm_mon   = add_month;
	rtc_task->interval.tm_mday  = add_date;
	rtc_task->interval.tm_hour  = add_hours;
	rtc_task->interval.tm_min   = add_minutes;
	rtc_task->interval.tm_sec   = add_seconds;
	rtc_task->auto_load = auto_load;
	INIT_LIST_HEAD(&rtc_task->list);
	rtc_task_update_time(rtc_task);
	list_add_tail(&rtc_task->list, &rtc_task_head);
	return 0;
}

int rtc_task_drop(void(*callback)(struct rtc_task *))
{
	struct rtc_task *rtc_task, *tmp;

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_head, list) {
		if (rtc_task->callback == callback) {
			list_del(&rtc_task->list);
			return 0;
		}
	}
	return -EINVAL;
}

void rtc_task_dequeue(struct rtc_task *rtc_task)
{
	list_del(&rtc_task->list);
}

int rtc_task_suspend(void(*callback)(struct rtc_task *))
{
	struct rtc_task *rtc_task, *tmp;

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_head, list) {
		if (rtc_task->callback == callback) {
			list_move_tail(&rtc_task->list, &rtc_task_suspend_head);
			return 0;
		}
	}
	return -EINVAL;
}

int rtc_task_resume(void(*callback)(struct rtc_task *))
{
	struct rtc_task *rtc_task, *tmp;

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_suspend_head, list) {
		if (rtc_task->callback == callback) {
			list_move_tail(&rtc_task->list, &rtc_task_head);
			return 0;
		}
	}
	return -EINVAL;
}

void rtc_task_loop(void)
{
	struct rtc_task *rtc_task, *tmp;

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_head, list) {
		if (rtc_task_expired(rtc_task)) {
			rtc_task->callback(rtc_task);
			if (rtc_task->auto_load)
				rtc_task_update_time(rtc_task);
			else
				list_del(&rtc_task->list);
		}
	}
}

void rtc_task_set_current_time(u16 year, u8 month, u8 date,
	u8 hours, u8 minutes, u8 seconds)
{
	current_time.tm_year = year;
	current_time.tm_mon  = month;
	current_time.tm_mday = date;
	current_time.tm_hour = hours;
	current_time.tm_min  = minutes;
	current_time.tm_sec  = seconds;
}

bool rtc_task_validate_time(u16 year, u8 month, u8 date,
	u8 hours, u8 minutes, u8 seconds)
{
	if (year < 1970 || year > 9999 || month < 1 || month > 12 ||
		date < 1 || date > 31 || hours > 23 || minutes > 59 || seconds > 59)
		return false;
	if (month == 2) {
		bool is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
		if (date > (is_leap ? 29 : 28)) return false;
	}
	if ((month == 4 || month == 6 || month == 9 || month == 11) && date > 30)
		return false;
	return true;
}

struct rtc_task *rtc_task_find_by_callback(void (*callback)(struct rtc_task *))
{
	struct rtc_task *rtc_task;

	list_for_each_entry(rtc_task, &rtc_task_head, list) {
		if (rtc_task->callback == callback)
			return rtc_task;
	}
	return NULL;
}

void rtc_task_cleanup_all(void)
{
	struct rtc_task *rtc_task, *tmp;

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_head, list)
		list_del(&rtc_task->list);
	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_suspend_head, list)
		list_del(&rtc_task->list);
}

#ifdef DESIGN_VERIFICATION_RTCTASK
#include "kinetis/test-kinetis.h"

static int callback_count = 0;
static int callback_count2 = 0;
static int callback_count3 = 0;
static int callback_count4 = 0;
static int callback_count5 = 0;
static struct rtc_task test_task_1, test_task_2, test_task_3;
static struct rtc_task test_task_4, test_task_5;

void rtc_task_callback(struct rtc_task *task)
{
	callback_count++;
	pr_debug("rtc_task_callback called, count: %d\n", callback_count);
}

void rtc_task_callback2(struct rtc_task *task)
{
	callback_count2++;
	pr_debug("rtc_task_callback2 called, count: %d\n", callback_count2);
}

void rtc_task_callback3(struct rtc_task *task)
{
	callback_count3++;
	pr_debug("rtc_task_callback3 called, count: %d\n", callback_count3);
}

void rtc_task_callback4(struct rtc_task *task)
{
	callback_count4++;
	pr_debug("rtc_task_callback4 called, count: %d\n", callback_count4);
}

void rtc_task_callback5(struct rtc_task *task)
{
	callback_count5++;
	pr_debug("rtc_task_callback5 called, count: %d\n", callback_count5);
}

int t_rtc_task_add(int argc, char **argv)
{
	u16 year = 0; u8 month = 0, date = 0, hours = 0, minutes = 0, seconds = 0;
	bool auto_load = true;

	if (argc > 1) year = simple_strtoul(argv[1], NULL, 10);
	if (argc > 2) month = simple_strtoul(argv[2], NULL, 10);
	if (argc > 3) date = simple_strtoul(argv[3], NULL, 10);
	if (argc > 4) hours = simple_strtoul(argv[4], NULL, 10);
	if (argc > 5) minutes = simple_strtoul(argv[5], NULL, 10);
	if (argc > 6) seconds = simple_strtoul(argv[6], NULL, 10);
	if (argc > 7) auto_load = simple_strtoul(argv[7], NULL, 10);

	int ret = rtc_task_add(&test_task_1, year, month, date, hours, minutes, seconds, auto_load, rtc_task_callback);
	if (ret) return ret;
	pr_info("rtc basic task created successfully\n");
	return 0;
}

int t_rtc_task_drop(int argc, char **argv)
{
	int ret = rtc_task_add(&test_task_1, 0, 0, 0, 0, 0, 5, true, rtc_task_callback);
	if (ret) return ret;

	ret = rtc_task_drop(rtc_task_callback);
	if (ret) return ret;
	pr_info("rtc task dropped successfully\n");
	return 0;
}

int t_rtc_task_validation(int argc, char **argv)
{
	bool valid;

	valid = rtc_task_validate_time(2024, 1, 15, 12, 30, 45);
	if (!valid) { pr_err("test 1 failed\n"); return FAIL; }
	valid = rtc_task_validate_time(2024, 13, 15, 12, 30, 45);
	if (valid) { pr_err("test 2 failed\n"); return FAIL; }
	valid = rtc_task_validate_time(2024, 2, 30, 12, 30, 45);
	if (valid) { pr_err("test 3 failed\n"); return FAIL; }
	valid = rtc_task_validate_time(2024, 2, 29, 12, 30, 45);
	if (!valid) { pr_err("test 4 failed\n"); return FAIL; }
	valid = rtc_task_validate_time(2024, 1, 15, 24, 30, 45);
	if (valid) { pr_err("test 5 failed\n"); return FAIL; }
	valid = rtc_task_validate_time(2024, 1, 15, 12, 60, 45);
	if (valid) { pr_err("test 6 failed\n"); return FAIL; }
	valid = rtc_task_validate_time(2024, 1, 15, 12, 30, 60);
	if (valid) { pr_err("test 7 failed\n"); return FAIL; }
	pr_info("all validation tests passed\n");
	return 0;
}

int t_rtc_task_priority(int argc, char **argv)
{
	callback_count = callback_count2 = callback_count3 = 0;
	int ret;

	ret = rtc_task_add(&test_task_1, 0, 0, 0, 0, 0, 5, true, rtc_task_callback);
	if (ret) return ret;
	ret = rtc_task_add(&test_task_2, 0, 0, 0, 0, 0, 10, true, rtc_task_callback2);
	if (ret) return ret;
	ret = rtc_task_add(&test_task_3, 0, 0, 0, 0, 0, 15, true, rtc_task_callback3);
	if (ret) return ret;

	pr_info("three rtc tasks with different intervals created\n");
	return 0;
}

int t_rtc_task_performance(int argc, char **argv)
{
	struct rtc_task tasks[100];
	int ret;

	for (int i = 0; i < 100; i++) {
		ret = rtc_task_add(&tasks[i], 0, 0, 0, 0, 0, (i % 10) + 1, true, rtc_task_callback);
		if (ret) return ret;
	}
	pr_info("created 100 rtc tasks for performance test\n");
	return 0;
}

int t_rtc_task_cleanup(int argc, char **argv)
{
	struct rtc_task tasks[10];
	int ret;

	for (int i = 0; i < 10; i++) {
		ret = rtc_task_add(&tasks[i], 0, 0, 0, 0, 0, (i % 10) + 1, true, rtc_task_callback);
		if (ret) return ret;
	}
	rtc_task_cleanup_all();
	pr_info("all rtc tasks cleaned up successfully\n");
	return 0;
}

int t_rtc_task_boundary(int argc, char **argv)
{
	struct rtc_task bt;
	int ret;

	ret = rtc_task_add(&bt, 10000, 0, 0, 0, 0, 0, true, rtc_task_callback);
	if (ret != -EINVAL) { pr_err("test 1 failed\n"); return FAIL; }
	ret = rtc_task_add(&bt, 0, 13, 0, 0, 0, 0, true, rtc_task_callback);
	if (ret != -EINVAL) { pr_err("test 2 failed\n"); return FAIL; }
	ret = rtc_task_add(&bt, 0, 0, 32, 0, 0, 0, true, rtc_task_callback);
	if (ret != -EINVAL) { pr_err("test 3 failed\n"); return FAIL; }
	ret = rtc_task_add(&bt, 0, 0, 0, 24, 0, 0, true, rtc_task_callback);
	if (ret != -EINVAL) { pr_err("test 4 failed\n"); return FAIL; }
	ret = rtc_task_add(&bt, 0, 0, 0, 0, 60, 0, true, rtc_task_callback);
	if (ret != -EINVAL) { pr_err("test 5 failed\n"); return FAIL; }
	ret = rtc_task_add(&bt, 0, 0, 0, 0, 0, 60, true, rtc_task_callback);
	if (ret != -EINVAL) { pr_err("test 6 failed\n"); return FAIL; }
	ret = rtc_task_add(&bt, 0, 0, 0, 0, 0, 5, true, NULL);
	if (ret != -EINVAL) { pr_err("test 7 failed\n"); return FAIL; }
	pr_info("all boundary tests passed\n");
	return 0;
}

int t_rtc_task_concurrent(int argc, char **argv)
{
	callback_count = callback_count2 = callback_count3 = 0;
	callback_count4 = callback_count5 = 0;
	int ret;

	ret = rtc_task_add(&test_task_1, 0, 0, 0, 0, 0, 5, true, rtc_task_callback);
	if (ret) return ret;
	ret = rtc_task_add(&test_task_2, 0, 0, 0, 0, 0, 5, true, rtc_task_callback2);
	if (ret) return ret;
	ret = rtc_task_add(&test_task_3, 0, 0, 0, 0, 0, 5, true, rtc_task_callback3);
	if (ret) return ret;
	ret = rtc_task_add(&test_task_4, 0, 0, 0, 0, 0, 5, true, rtc_task_callback4);
	if (ret) return ret;
	ret = rtc_task_add(&test_task_5, 0, 0, 0, 0, 0, 5, true, rtc_task_callback5);
	if (ret) return ret;
	pr_info("five concurrent rtc tasks created\n");
	return 0;
}

int t_rtc_task_short_interval(int argc, char **argv)
{
	int ret;
	ret = rtc_task_add(&test_task_1, 0, 0, 0, 0, 0, 1, true, rtc_task_callback);
	if (ret) return ret;
	ret = rtc_task_add(&test_task_2, 0, 0, 0, 0, 0, 2, true, rtc_task_callback2);
	if (ret) return ret;
	pr_info("rtc tasks with short intervals created\n");
	return 0;
}

int t_rtc_task_suspend_resume(int argc, char **argv)
{
	int ret;

	ret = rtc_task_add(&test_task_1, 0, 0, 0, 0, 0, 5, true, rtc_task_callback);
	if (ret) return ret;

	ret = rtc_task_suspend(rtc_task_callback);
	if (ret) return ret;
	pr_info("rtc task suspended\n");

	ret = rtc_task_resume(rtc_task_callback);
	if (ret) return ret;
	pr_info("rtc task resumed successfully\n");
	return 0;
}

#endif
