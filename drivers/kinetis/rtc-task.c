
#include <generated/deconfig.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/iopoll.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/printk.h>

#include <kinetis/design_verification.h>
#include <kinetis/rtc-task.h>

static LIST_HEAD(rtc_task_head);
static LIST_HEAD(rtc_task_suspend_head);

static struct tm current_time = {.tm_year = 0};

static bool rtc_task_expired(struct rtc_task *rtc_task)
{
	if (current_time.tm_year != rtc_task->expired.tm_year) {
		return time_after32(current_time.tm_year, rtc_task->expired.tm_year);
	}

	if (current_time.tm_mon != rtc_task->expired.tm_mon) {
		return time_after32(current_time.tm_mon, rtc_task->expired.tm_mon);
	}

	if (current_time.tm_mday != rtc_task->expired.tm_mday) {
		return time_after32(current_time.tm_mday, rtc_task->expired.tm_mday);
	}

	if (current_time.tm_hour != rtc_task->expired.tm_hour) {
		return time_after32(current_time.tm_hour, rtc_task->expired.tm_hour);
	}

	if (current_time.tm_min != rtc_task->expired.tm_min) {
		return time_after32(current_time.tm_min, rtc_task->expired.tm_min);
	}

	if (current_time.tm_sec != rtc_task->expired.tm_sec) {
		return time_after32(current_time.tm_sec, rtc_task->expired.tm_sec);
	}

	return true;
}

static void rtc_task_special_add_days(struct tm *date_time)
{
	/* Is it February?,28,29 */
	if (date_time->tm_mon == 2) {
		/*
		 * The system will certainly not be in use until 2100,
		 * so it only judges whether it is divisible by 4.
		 * Leap year
		*/
		if (date_time->tm_year % 4 == 0) {
			//No more than 29 days
			if (date_time->tm_mday < 30) {
				return ;
			}

			//Minus the overflow of 29
			//month +1
			date_time->tm_mday -= 29;
			++date_time->tm_mon;
		} else {
			//No more than 28 days
			if (date_time->tm_mday < 29) {
				return ;
			}

			//Minus the overflow of 28
			//month +1
			date_time->tm_mday -= 28;
			++date_time->tm_mon;
		}
	}

	// Is it a 30-day month
	if (date_time->tm_mon == 4 || date_time->tm_mon == 6 ||
		date_time->tm_mon == 9 || date_time->tm_mon == 11) {
		//No more than 30 days
		if (date_time->tm_mday < 31) {
			return;
		}

		//Minus the overflow of 30
		//month+1
		date_time->tm_mday -= 30;
		++date_time->tm_mon;
	}

	// Is it a 31-day month
	if (date_time->tm_mon == 1 || date_time->tm_mon == 3 ||
		date_time->tm_mon == 5 || date_time->tm_mon == 7 ||
		date_time->tm_mon == 8 || date_time->tm_mon == 10 ||
		date_time->tm_mon == 12) {
		//No more than 31 days
		if (date_time->tm_mday < 32) {
			return;
		}

		//Minus the overflow of 30
		//month +1
		date_time->tm_mday -= 31;
		++date_time->tm_mon;
	}

	//No more than December

	if (date_time->tm_mon < 13) {
		return ;
	}

	//Minus 12 months of overflow
	//year +1
	date_time->tm_mon -= 12;
	++date_time->tm_year;

	return;
}

static void rtc_task_time_add_seconds(struct tm *date_time, u8 seconds)
{
	if (seconds > 60) {
		return ;
	}

	//Plus the number of seconds
	date_time->tm_sec += seconds;

	//Finished
	if (date_time->tm_sec < 60) {
		return ;
	}

	//Minute + 1
	date_time->tm_sec -= 60;
	++date_time->tm_min;

	//Finished
	if (date_time->tm_min < 60) {
		return ;
	}

	//tm_hour + 1
	date_time->tm_min -= 60;
	++date_time->tm_hour;

	//Finished
	if (date_time->tm_hour < 24) {
		return ;
	}

	//Days + 1;
	date_time->tm_hour -= 24;
	++date_time->tm_mday;

	rtc_task_special_add_days(date_time);
}

// minutes cannot be greater than 60 minutes
static void rtc_task_time_add_minutes(struct tm *date_time, u8 minutes)
{
	if (minutes > 60) {
		return ;
	}

	//Plus the number of minutes
	date_time->tm_min += minutes;

	//Finished
	if (date_time->tm_min < 60) {
		return ;
	}

	//hours + 1
	date_time->tm_min -= 60;
	++date_time->tm_hour;

	//Finished
	if (date_time->tm_hour < 24) {
		return ;
	}

	//days + 1;
	date_time->tm_hour -= 24;
	++date_time->tm_mday;

	rtc_task_special_add_days(date_time);
}

// An hour cannot be greater than 24
static void rtc_task_time_add_hours(struct tm *date_time, u8 hours)
{
	if (hours > 24) {
		return;
	}

	date_time->tm_hour += hours;

	//Finished
	if (date_time->tm_hour < 24) {
		return ;
	}

	//days + 1;
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
	if (months > 12) {
		return;
	}

	date_time->tm_mon += months;

	if (date_time->tm_mon < 13) {
		return;
	}

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
		rtc_task->expired.tm_sec = 0;
		rtc_task->expired.tm_min = 0;
		rtc_task->expired.tm_hour = 0;
		rtc_task->expired.tm_mday = 0;
		rtc_task->expired.tm_mon = 0;
		rtc_task->expired.tm_year -=
			rtc_task->expired.tm_year % rtc_task->interval.tm_year;
		rtc_task_time_add_year(&(rtc_task->expired), rtc_task->interval.tm_year);
	}

	if (rtc_task->interval.tm_mon != 0) {
		rtc_task->expired.tm_sec = 0;
		rtc_task->expired.tm_min = 0;
		rtc_task->expired.tm_hour = 0;
		rtc_task->expired.tm_mday = 0;
		rtc_task->expired.tm_mon -=
			rtc_task->expired.tm_mon % rtc_task->interval.tm_mon;
		rtc_task_time_add_months(&(rtc_task->expired), rtc_task->interval.tm_mon);
	}

	if (rtc_task->interval.tm_mday != 0) {
		rtc_task->expired.tm_sec = 0;
		rtc_task->expired.tm_min = 0;
		rtc_task->expired.tm_hour = 0;
		rtc_task->expired.tm_mday -=
			rtc_task->expired.tm_mday % rtc_task->interval.tm_mday;
		rtc_task_time_add_days(&(rtc_task->expired), rtc_task->interval.tm_mday);
	}

	if (rtc_task->interval.tm_hour != 0) {
		rtc_task->expired.tm_sec = 0;
		rtc_task->expired.tm_min = 0;
		rtc_task->expired.tm_hour -=
			rtc_task->expired.tm_hour % rtc_task->interval.tm_hour;
		rtc_task_time_add_hours(&(rtc_task->expired), rtc_task->interval.tm_hour);
	}

	if (rtc_task->interval.tm_min != 0) {
		rtc_task->expired.tm_sec = 0;
		rtc_task->expired.tm_min -=
			rtc_task->expired.tm_min % rtc_task->interval.tm_min;
		rtc_task_time_add_minutes(&(rtc_task->expired), rtc_task->interval.tm_min);
	}

	if (rtc_task->interval.tm_sec != 0) {
		rtc_task_time_add_seconds(&(rtc_task->expired), rtc_task->interval.tm_sec);
	}
}

int rtc_task_add(u16 add_year, u8 add_month, u8 add_date,
	u8 add_hours, u8 add_minutes, u8 add_seconds,
	bool auto_load, void(*callback)())
{
	struct rtc_task *rtc_task;

	// Enhanced validation
	if (add_year > 9999 || add_month > 12 || add_date > 31 ||
		add_hours > 23 || add_minutes > 59 || add_seconds > 59) {
		pr_err("invalid time parameters\n");
		return -EINVAL;
	}

	if (current_time.tm_year == 0) {
		pr_err("native time has not been updated.\n");
		return -ETIMEDOUT;
	}

	rtc_task = kmalloc(sizeof(*rtc_task), GFP_KERNEL);

	if (!rtc_task) {
		return -ENOMEM;
	}

	if (!callback) {
		kfree(rtc_task);
		return -EINVAL;
	}

	rtc_task->callback = callback;

	rtc_task->interval.tm_year  = add_year;
	rtc_task->interval.tm_mon = add_month;
	rtc_task->interval.tm_mday = add_date;
	rtc_task->interval.tm_hour = add_hours;
	rtc_task->interval.tm_min = add_minutes;
	rtc_task->interval.tm_sec = add_seconds;

	rtc_task_update_time(rtc_task);

	rtc_task->auto_load = auto_load;
	rtc_task->self_alloc = true;

	list_add_tail(&rtc_task->list, &rtc_task_head);

	pr_info("rtc task created: %04d-%02d-%02d %02d:%02d:%02d interval\n",
		add_year, add_month, add_date, add_hours, add_minutes, add_seconds);

	return 0;
}

int rtc_task_drop(void(*callback)())
{
	struct rtc_task *rtc_task, *tmp;

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_head, list) {
		if (rtc_task->callback == callback) {
			list_del(&rtc_task->list);
			if (rtc_task->self_alloc) {
				kfree(rtc_task);
			}
			return 0;
		}
	}

	return -EINVAL;
}

int rtc_task_enqueue(struct rtc_task *rtc_task,
	u16 add_year, u8 add_month, u8 add_date,
	u8 add_hours, u8 add_minutes, u8 add_seconds,
	void(*callback)(struct rtc_task *))
{
	if (current_time.tm_year == 0) {
		pr_err("native time has not been updated.\n");
		return -ETIMEDOUT;
	}

	if (!callback) {
		return -EINVAL;
	}

	rtc_task->callback = callback;

	rtc_task->interval.tm_year  = add_year;
	rtc_task->interval.tm_mon = add_month;
	rtc_task->interval.tm_mday = add_date;
	rtc_task->interval.tm_hour = add_hours;
	rtc_task->interval.tm_min = add_minutes;
	rtc_task->interval.tm_sec = add_seconds;

	rtc_task_update_time(rtc_task);

	rtc_task->auto_load = false;
	rtc_task->self_alloc = false;

	list_add_tail(&rtc_task->list, &rtc_task_head);

	return 0;
}

void rtc_task_dequeue(struct rtc_task *rtc_task)
{
	list_del(&rtc_task->list);
}

int rtc_task_suspend(void(*callback)())
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

int rtc_task_resume(void(*callback)())
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
	ktime_t loop_start_time;

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_head, list) {
		if (rtc_task_expired(rtc_task)) {
			pr_debug("%04d-%02d-%02d %02d:%02d:%02d - executing rtc task\n",
				current_time.tm_year, current_time.tm_mon, current_time.tm_mday,
				current_time.tm_hour, current_time.tm_min, current_time.tm_sec);

			rtc_task->callback();

			if (rtc_task->auto_load) {
				rtc_task_update_time(rtc_task);
			} else {
				list_del(&rtc_task->list);

				if (rtc_task->self_alloc == true) {
					kfree(rtc_task);
				}
			}
		}
	}
}

void rtc_task_set_current_time(u16 year, u8 month, u8 date,
	u8 hours, u8 minutes, u8 seconds)
{
	current_time.tm_year = year;
	current_time.tm_mon = month;
	current_time.tm_mday = date;
	current_time.tm_hour = hours;
	current_time.tm_min = minutes;
	current_time.tm_sec = seconds;
}

bool rtc_task_validate_time(u16 year, u8 month, u8 date,
	u8 hours, u8 minutes, u8 seconds)
{
	// Basic range checks
	if (year < 1970 || year > 9999 ||
		month < 1 || month > 12 ||
		date < 1 || date > 31 ||
		hours > 23 || minutes > 59 || seconds > 59) {
		return false;
	}

	// February validation
	if (month == 2) {
		bool is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
		if (date > (is_leap ? 29 : 28)) {
			return false;
		}
	}

	// Months with 30 days
	if ((month == 4 || month == 6 || month == 9 || month == 11) && date > 30) {
		return false;
	}

	return true;
}

struct rtc_task *rtc_task_find_by_callback(void (*callback)(void))
{
	struct rtc_task *rtc_task;

	list_for_each_entry(rtc_task, &rtc_task_head, list) {
		if (rtc_task->callback == callback) {
			return rtc_task;
		}
	}

	return NULL;
}

int rtc_task_get_current_time_safe(u16 *year, u8 *month, u8 *date,
	u8 *hours, u8 *minutes, u8 *seconds)
{
	if (!year || !month || !date || !hours || !minutes || !seconds) {
		return -EINVAL;
	}

	*year = current_time.tm_year;
	*month = current_time.tm_mon;
	*date = current_time.tm_mday;
	*hours = current_time.tm_hour;
	*minutes = current_time.tm_min;
	*seconds = current_time.tm_sec;

	return 0;
}

void rtc_task_cleanup_all(void)
{
	struct rtc_task *rtc_task, *tmp;

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_head, list) {
		list_del(&rtc_task->list);
		if (rtc_task->self_alloc) {
			kfree(rtc_task);
		}
	}

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_suspend_head, list) {
		list_del(&rtc_task->list);
		if (rtc_task->self_alloc) {
			kfree(rtc_task);
		}
	}

	pr_info("all rtc tasks are cleaned up\n");
}

#ifdef DESIGN_VERIFICATION_RTCTASK
#include "kinetis/test-kinetis.h"

static int callback_count = 0;
static int callback_count2 = 0;
static int callback_count3 = 0;
static int callback_count4 = 0;
static int callback_count5 = 0;

void rtc_task_callback(void)
{
	callback_count++;
	pr_debug("rtc_task_callback called, count: %d\n", callback_count);
}

void rtc_task_callback2(void)
{
	callback_count2++;
	pr_debug("rtc_task_callback2 called, count: %d\n", callback_count2);
}

void rtc_task_callback3(void)
{
	callback_count3++;
	pr_debug("rtc_task_callback3 called, count: %d\n", callback_count3);
}

void rtc_task_callback4(void)
{
	callback_count4++;
	pr_debug("rtc_task_callback4 called, count: %d\n", callback_count4);
}

void rtc_task_callback5(void)
{
	callback_count5++;
	pr_debug("rtc_task_callback5 called, count: %d\n", callback_count5);
}

/**
 * @brief Test basic RTC task addition
 */
int t_rtc_task_add(int argc, char **argv)
{
	u16 year;
	u8 month, date, hours, minutes, seconds;
	bool auto_load;
	int ret;

	if (argc > 1) {
		year = simple_strtoul(argv[1], NULL, 10);
	}

	if (argc > 2) {
		month = simple_strtoul(argv[2], NULL, 10);
	}

	if (argc > 3) {
		date = simple_strtoul(argv[3], NULL, 10);
	}

	if (argc > 4) {
		hours = simple_strtoul(argv[4], NULL, 10);
	}

	if (argc > 5) {
		minutes = simple_strtoul(argv[5], NULL, 10);
	}

	if (argc > 6) {
		seconds = simple_strtoul(argv[6], NULL, 10);
	}

	if (argc > 7) {
		auto_load = simple_strtoul(argv[7], NULL, 10);
	}

	ret = rtc_task_add(year, month, date, hours, minutes, seconds, auto_load, rtc_task_callback);
	if (ret) {
		pr_err("rtc task creation failed: %d\n", ret);
		return ret;
	}

	pr_info("rtc basic task created successfully\n");
	return 0;
}

/**
 * @brief Test RTC task dropping
 */
int t_rtc_task_drop(int argc, char **argv)
{
	int ret;

	/* Add a task */
	ret = rtc_task_add(0, 0, 0, 0, 0, 5, true, rtc_task_callback);
	if (ret) {
		pr_err("rtc task creation failed: %d\n", ret);
		return ret;
	}

	/* Drop the task */
	ret = rtc_task_drop(rtc_task_callback);
	if (ret) {
		pr_err("rtc task drop failed: %d\n", ret);
		return ret;
	}

	pr_info("rtc task dropped successfully\n");
	return 0;
}

/**
 * @brief Test RTC task validation
 */
int t_rtc_task_validation(int argc, char **argv)
{
	bool valid;

	/* Test 1: Valid time */
	valid = rtc_task_validate_time(2024, 1, 15, 12, 30, 45);
	if (!valid) {
		pr_err("test 1 failed: valid time rejected\n");
		return FAIL;
	}

	/* Test 2: Invalid month */
	valid = rtc_task_validate_time(2024, 13, 15, 12, 30, 45);
	if (valid) {
		pr_err("test 2 failed: invalid month accepted\n");
		return FAIL;
	}

	/* Test 3: Invalid day */
	valid = rtc_task_validate_time(2024, 2, 30, 12, 30, 45);
	if (valid) {
		pr_err("test 3 failed: invalid day accepted\n");
		return FAIL;
	}

	/* Test 4: Leap year valid day */
	valid = rtc_task_validate_time(2024, 2, 29, 12, 30, 45);
	if (!valid) {
		pr_err("test 4 failed: leap year day rejected\n");
		return FAIL;
	}

	/* Test 5: Invalid hour */
	valid = rtc_task_validate_time(2024, 1, 15, 24, 30, 45);
	if (valid) {
		pr_err("test 5 failed: invalid hour accepted\n");
		return FAIL;
	}

	/* Test 6: Invalid minute */
	valid = rtc_task_validate_time(2024, 1, 15, 12, 60, 45);
	if (valid) {
		pr_err("test 6 failed: invalid minute accepted\n");
		return FAIL;
	}

	/* Test 7: Invalid second */
	valid = rtc_task_validate_time(2024, 1, 15, 12, 30, 60);
	if (valid) {
		pr_err("test 7 failed: invalid second accepted\n");
		return FAIL;
	}

	pr_info("all validation tests passed\n");
	return 0;
}

/**
 * @brief Test RTC task with different intervals (priority-like)
 */
int t_rtc_task_priority(int argc, char **argv)
{
	int ret;

	/* Reset counters */
	callback_count = 0;
	callback_count2 = 0;
	callback_count3 = 0;

	/* Add tasks with different intervals (simulating priority) */
	ret = rtc_task_add(0, 0, 0, 0, 0, 5, true, rtc_task_callback);
	if (ret) {
		pr_err("rtc task 1 creation failed: %d\n", ret);
		return ret;
	}

	ret = rtc_task_add(0, 0, 0, 0, 0, 10, true, rtc_task_callback2);
	if (ret) {
		pr_err("rtc task 2 creation failed: %d\n", ret);
		return ret;
	}

	ret = rtc_task_add(0, 0, 0, 0, 0, 15, true, rtc_task_callback3);
	if (ret) {
		pr_err("rtc task 3 creation failed: %d\n", ret);
		return ret;
	}

	pr_info("three rtc tasks with different intervals created\n");
	return 0;
}

/**
 * @brief Test RTC task performance
 */
int t_rtc_task_performance(int argc, char **argv)
{
	int ret;
	int i;
	int num_tasks = 100;

	/* Add multiple tasks to test performance */
	for (i = 0; i < num_tasks; i++) {
		ret = rtc_task_add(0, 0, 0, 0, 0, (i % 10) + 1, true, rtc_task_callback);
		if (ret) {
			pr_err("rtc task %d creation failed: %d\n", i, ret);
			return ret;
		}
	}

	pr_info("created %d rtc tasks for performance test\n", num_tasks);
	return 0;
}

/**
 * @brief Test RTC task cleanup
 */
int t_rtc_task_cleanup(int argc, char **argv)
{
	int ret;
	int i;

	/* Add several tasks */
	for (i = 0; i < 10; i++) {
		ret = rtc_task_add(0, 0, 0, 0, 0, (i % 10) + 1, true, rtc_task_callback);
		if (ret) {
			pr_err("rtc task %d creation failed: %d\n", i, ret);
			return ret;
		}
	}

	/* Cleanup all tasks */
	rtc_task_cleanup_all();

	pr_info("all rtc tasks cleaned up successfully\n");
	return 0;
}

/**
 * @brief Test RTC task boundary conditions
 */
int t_rtc_task_boundary(int argc, char **argv)
{
	int ret;

	/* Test 1: Invalid year (too large) */
	ret = rtc_task_add(10000, 0, 0, 0, 0, 0, true, rtc_task_callback);
	if (ret != -EINVAL) {
		pr_err("test 1 failed: invalid year should be rejected\n");
		return FAIL;
	}

	/* Test 2: Invalid month */
	ret = rtc_task_add(0, 13, 0, 0, 0, 0, true, rtc_task_callback);
	if (ret != -EINVAL) {
		pr_err("test 2 failed: invalid month should be rejected\n");
		return FAIL;
	}

	/* Test 3: Invalid day */
	ret = rtc_task_add(0, 0, 32, 0, 0, 0, true, rtc_task_callback);
	if (ret != -EINVAL) {
		pr_err("test 3 failed: invalid day should be rejected\n");
		return FAIL;
	}

	/* Test 4: Invalid hour */
	ret = rtc_task_add(0, 0, 0, 24, 0, 0, true, rtc_task_callback);
	if (ret != -EINVAL) {
		pr_err("test 4 failed: invalid hour should be rejected\n");
		return FAIL;
	}

	/* Test 5: Invalid minute */
	ret = rtc_task_add(0, 0, 0, 0, 60, 0, true, rtc_task_callback);
	if (ret != -EINVAL) {
		pr_err("test 5 failed: invalid minute should be rejected\n");
		return FAIL;
	}

	/* Test 6: Invalid second */
	ret = rtc_task_add(0, 0, 0, 0, 0, 60, true, rtc_task_callback);
	if (ret != -EINVAL) {
		pr_err("test 6 failed: invalid second should be rejected\n");
		return FAIL;
	}

	/* Test 7: NULL callback */
	ret = rtc_task_add(0, 0, 0, 0, 0, 5, true, NULL);
	if (ret != -EINVAL) {
		pr_err("test 7 failed: NULL callback should be rejected\n");
		return FAIL;
	}

	pr_info("all boundary tests passed\n");
	return 0;
}

/**
 * @brief Test RTC task concurrent operations
 */
int t_rtc_task_concurrent(int argc, char **argv)
{
	int ret;

	/* Reset counters */
	callback_count = 0;
	callback_count2 = 0;
	callback_count3 = 0;
	callback_count4 = 0;
	callback_count5 = 0;

	/* Add multiple tasks with same interval */
	ret = rtc_task_add(0, 0, 0, 0, 0, 5, true, rtc_task_callback);
	if (ret) {
		pr_err("rtc task 1 creation failed: %d\n", ret);
		return ret;
	}

	ret = rtc_task_add(0, 0, 0, 0, 0, 5, true, rtc_task_callback2);
	if (ret) {
		pr_err("rtc task 2 creation failed: %d\n", ret);
		return ret;
	}

	ret = rtc_task_add(0, 0, 0, 0, 0, 5, true, rtc_task_callback3);
	if (ret) {
		pr_err("rtc task 3 creation failed: %d\n", ret);
		return ret;
	}

	ret = rtc_task_add(0, 0, 0, 0, 0, 5, true, rtc_task_callback4);
	if (ret) {
		pr_err("rtc task 4 creation failed: %d\n", ret);
		return ret;
	}

	ret = rtc_task_add(0, 0, 0, 0, 0, 5, true, rtc_task_callback5);
	if (ret) {
		pr_err("rtc task 5 creation failed: %d\n", ret);
		return ret;
	}

	pr_info("five concurrent rtc tasks created with same interval\n");
	return 0;
}

/**
 * @brief Test RTC task with short intervals
 */
int t_rtc_task_short_interval(int argc, char **argv)
{
	int ret;

	/* Add task with 1 second interval (shortest valid) */
	ret = rtc_task_add(0, 0, 0, 0, 0, 1, true, rtc_task_callback);
	if (ret) {
		pr_err("rtc task with 1s interval creation failed: %d\n", ret);
		return ret;
	}

	/* Add task with 2 second interval */
	ret = rtc_task_add(0, 0, 0, 0, 0, 2, true, rtc_task_callback2);
	if (ret) {
		pr_err("rtc task with 2s interval creation failed: %d\n", ret);
		return ret;
	}

	pr_info("rtc tasks with short intervals created\n");
	return 0;
}

/**
 * @brief Test RTC task suspend and resume
 */
int t_rtc_task_suspend_resume(int argc, char **argv)
{
	int ret;

	/* Add a task */
	ret = rtc_task_add(0, 0, 0, 0, 0, 5, true, rtc_task_callback);
	if (ret) {
		pr_err("rtc task creation failed: %d\n", ret);
		return ret;
	}

	/* Suspend the task */
	ret = rtc_task_suspend(rtc_task_callback);
	if (ret) {
		pr_err("rtc task suspend failed: %d\n", ret);
		return ret;
	}

	pr_info("rtc task suspended\n");

	/* Resume the task */
	ret = rtc_task_resume(rtc_task_callback);
	if (ret) {
		pr_err("rtc task resume failed: %d\n", ret);
		return ret;
	}

	pr_info("rtc task resumed successfully\n");
	return 0;
}

#endif
