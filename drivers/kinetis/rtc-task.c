
#include <generated/deconfig.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/iopoll.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/printk.h>

#include <kinetis/design_verification.h>
#include <kinetis/rtc-task.h>


//rtc_task rtc_task list head.
static LIST_HEAD(rtc_task_head);
static LIST_HEAD(rtc_task_suspend_head);

//struct rtc_task ticks
static struct tm current_time = {.tm_year = 0};

// Enhanced task management
static struct rtc_task_stats rtc_stats = {0};

// Performance monitoring
static bool performance_profiling_enabled = false;
static u32 task_execution_history[100];
static u8 history_index = 0;

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in function rtc_task_Test, design the function you need and initialize it in the main function.
  * @step 3:  Call the time update function rtc_task_GetCurrentTime every second.
  * @step 4:  An infinite loop calls function rtc_task_Loop.
  */


/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @brief  .
  * @param  None.
  * @retval None
  */
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
	/* Is it February?,28,29 */
	if (date_time->tm_mon == 2) {
		/*
		 * The system will certainly not be in use until 2100,
		 * so it only judges whether it is divisible by 4.
		 * Leap year
		*/
		if (date_time->tm_year % 4 == 0) {
			//No more than 29 days
			if (date_time->tm_mday < 30)
				return ;

			//Minus the overflow of 29
			//month +1
			date_time->tm_mday -= 29;
			++date_time->tm_mon;
		} else {
			//No more than 28 days
			if (date_time->tm_mday < 29)
				return ;

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
		if (date_time->tm_mday < 31)
			return;

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
		if (date_time->tm_mday < 32)
			return;

		//Minus the overflow of 30
		//month +1
		date_time->tm_mday -= 31;
		++date_time->tm_mon;
	}

	//No more than December

	if (date_time->tm_mon < 13)
		return ;

	//Minus 12 months of overflow
	//year +1
	date_time->tm_mon -= 12;
	++date_time->tm_year;

	return;
}

static void rtc_task_time_add_seconds(struct tm *date_time, u8 seconds)
{
	if (seconds > 60)
		return ;

	//Plus the number of seconds
	date_time->tm_sec += seconds;

	//Finished
	if (date_time->tm_sec < 60)
		return ;

	//Minute + 1
	date_time->tm_sec -= 60;
	++date_time->tm_min;

	//Finished
	if (date_time->tm_min < 60)
		return ;

	//tm_hour + 1
	date_time->tm_min -= 60;
	++date_time->tm_hour;

	//Finished
	if (date_time->tm_hour < 24)
		return ;

	//Days + 1;
	date_time->tm_hour -= 24;
	++date_time->tm_mday;

	rtc_task_special_add_days(date_time);
}

// minutes cannot be greater than 60 minutes
static void rtc_task_time_add_minutes(struct tm *date_time, u8 minutes)
{
	if (minutes > 60)
		return ;

	//Plus the number of minutes
	date_time->tm_min += minutes;

	//Finished
	if (date_time->tm_min < 60)
		return ;

	//hours + 1
	date_time->tm_min -= 60;
	++date_time->tm_hour;

	//Finished
	if (date_time->tm_hour < 24)
		return ;

	//days + 1;
	date_time->tm_hour -= 24;
	++date_time->tm_mday;

	rtc_task_special_add_days(date_time);
}

// An hour cannot be greater than 24
static void rtc_task_time_add_hours(struct tm *date_time, u8 hours)
{
	if (hours > 24)
		return;

	date_time->tm_hour += hours;

	//Finished
	if (date_time->tm_hour < 24)
		return ;

	//days + 1;
	date_time->tm_hour -= 24;
	++date_time->tm_mday;

	rtc_task_special_add_days(date_time);
}

static void rtc_task_time_add_days(struct tm *date_time, u8 days)
{
	//Make sure january doesn't jump to march
	if (days > 28)
		return;

	date_time->tm_mday += days;
	rtc_task_special_add_days(date_time);
}

static void rtc_task_time_add_months(struct tm *date_time, u8 months)
{
	if (months > 12)
		return;

	date_time->tm_mon += months;

	if (date_time->tm_mon < 13)
		return;

	date_time->tm_mon -= 12;
	++date_time->tm_year;
}

static void rtc_task_time_add_year(struct tm *date_time, u8 years)
{
	date_time->tm_year += years;
}

/**
  * @brief  Use round and align.
  * @param  None.
  * @retval None
  */
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
		rtc_task_time_add_days(&(rtc_task->expired), rtc_task->interval.tm_mon);
	}

	if (rtc_task->interval.tm_mday != 0) {
		rtc_task->expired.tm_sec = 0;
		rtc_task->expired.tm_min = 0;
		rtc_task->expired.tm_hour = 0;
		rtc_task->expired.tm_mday -=
			rtc_task->expired.tm_mday % rtc_task->interval.tm_mday;
		rtc_task_time_add_hours(&(rtc_task->expired), rtc_task->interval.tm_mday);
	}

	if (rtc_task->interval.tm_hour != 0) {
		rtc_task->expired.tm_sec = 0;
		rtc_task->expired.tm_min = 0;
		rtc_task->expired.tm_hour -=
			rtc_task->expired.tm_hour % rtc_task->interval.tm_hour;
		rtc_task_time_add_months(&(rtc_task->expired), rtc_task->interval.tm_hour);
	}

	if (rtc_task->interval.tm_min != 0) {
		rtc_task->expired.tm_sec = 0;
		rtc_task->expired.tm_min -=
			rtc_task->expired.tm_min % rtc_task->interval.tm_min;
		rtc_task_time_add_minutes(&(rtc_task->expired), rtc_task->interval.tm_min);
	}

	if (rtc_task->interval.tm_sec != 0)
		rtc_task_time_add_seconds(&(rtc_task->expired), rtc_task->interval.tm_sec);
}

/**
  * @brief  Initializes the rtc_task struct handle.
  * @param  rtc_task: the rtc_task handle strcut.
  * @param  callback: Timeout callback.
  * @param  Repeat: Repeat interval time.
  * @retval None
  */
int rtc_task_add(u16 add_year, u8 add_month, u8 add_date,
	u8 add_hours, u8 add_minutes, u8 add_seconds,
	bool auto_load, void(*callback)())
{
	struct rtc_task *rtc_task;
	int ret, val;

	// Enhanced validation
	if (add_year > 9999 || add_month > 12 || add_date > 31 ||
		add_hours > 23 || add_minutes > 59 || add_seconds > 59) {
		pr_err("Invalid time parameters\n");
		return -EINVAL;
	}

	ret = readl_poll_timeout_atomic(&current_time.tm_year, val, val, 1, 3000000);

	if (ret) {
		printk(KERN_ERR "Native time has not been updated.\n");
		return -ETIMEDOUT;
	}

	rtc_task = kmalloc(sizeof(*rtc_task), GFP_KERNEL);

	if (!rtc_task)
		return -ENOMEM;

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
	rtc_stats.total_tasks_created++;

	pr_info("RTC task created: %04d-%02d-%02d %02d:%02d:%02d interval\n",
		add_year, add_month, add_date, add_hours, add_minutes, add_seconds);

	return 0;
}

/**
  * @brief  Deinitializes the rtc_task struct handle.
  * @param  rtc_task: the rtc_task handle strcut.
  * @retval None
  */
int rtc_task_drop(void(*callback)())
{
	struct rtc_task *rtc_task, *tmp;

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_head, list) {
		if (rtc_task->callback == callback) {
			list_del(&rtc_task->list);
			kfree(rtc_task);
			return 0;
		}
	}

	return -EINVAL;
}

/**
  * @brief  add the rtc_task struct handle.
  * @param  rtc_task: the rtc_task handle strcut.
  * @param  timeout_cb: Timeout callback.
  * @param  timeout: time out time.
  * @param  repeat: repeat interval time.
  * @retval None
  */
int rtc_task_enqueue(struct rtc_task *rtc_task,
	u16 add_year, u8 add_month, u8 add_date,
	u8 add_hours, u8 add_minutes, u8 add_seconds,
	void(*callback)(struct rtc_task *))
{
	int ret, val;

	ret = readl_poll_timeout_atomic(&current_time.tm_year, val, val, 1, 3000000);

	if (ret) {
		printk(KERN_ERR "Native time has not been updated.\n");
		return -ETIMEDOUT;
	}

	if (!callback)
		return -EINVAL;

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

/**
  * @brief  drop the rtc_task struct handle.
  * @param  rtc_task: the rtc_task rtc_task strcut.
  * @retval None
  */
void rtc_task_dequeue(struct rtc_task *rtc_task)
{
	list_del(&rtc_task->list);
}

/**
  * @brief  suspend the rtc_task struct handle.
  * @param  rtc_task: the rtc_task rtc_task strcut.
  * @retval None
  */
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

/**
  * @brief  resume the rtc_task struct handle.
  * @param  rtc_task: the rtc_task rtc_task strcut.
  * @retval None
  */
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

/**
  * @brief  Task loop with performance monitoring.
  * @param  None.
  * @retval None
  */
void rtc_task_loop(void)
{
	struct rtc_task *rtc_task, *tmp;
	ktime_t loop_start_time;

	if (performance_profiling_enabled)
		loop_start_time = ktime_get();

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_head, list) {
		if (rtc_task_expired(rtc_task)) {
			ktime_t task_start_time, task_end_time;
			u32 execution_time_ms;

			if (performance_profiling_enabled)
				task_start_time = ktime_get();

			pr_debug("%04d-%02d-%02d %02d:%02d:%02d - Executing RTC task\n",
				current_time.tm_year, current_time.tm_mon, current_time.tm_mday,
				current_time.tm_hour, current_time.tm_min, current_time.tm_sec);

			rtc_task->callback();
			rtc_stats.total_tasks_executed++;

			if (performance_profiling_enabled) {
				task_end_time = ktime_get();
				execution_time_ms = ktime_to_ms(ktime_sub(task_end_time, task_start_time));

				// Update statistics
				rtc_stats.total_execution_time_ms += execution_time_ms;
				if (execution_time_ms > rtc_stats.max_execution_time_ms)
					rtc_stats.max_execution_time_ms = execution_time_ms;
				if (rtc_stats.min_execution_time_ms == 0 || execution_time_ms < rtc_stats.min_execution_time_ms)
					rtc_stats.min_execution_time_ms = execution_time_ms;

				// Store in history
				task_execution_history[history_index] = execution_time_ms;
				history_index = (history_index + 1) % 100;

				// Performance warning
				if (execution_time_ms > 50)
					pr_warn("RTC task took %u ms (threshold: 50ms)\n", execution_time_ms);
			}

			if (rtc_task->auto_load)
				rtc_task_update_time(rtc_task);
			else {
				list_del(&rtc_task->list);

				if (rtc_task->self_alloc == true)
					kfree(rtc_task);
			}
		}
	}

	if (performance_profiling_enabled) {
		u64 loop_time = ktime_to_ms(ktime_sub(ktime_get(), loop_start_time));
		if (loop_time > 10)
			pr_warn("RTC task loop took %llu ms (should be < 10ms)\n", loop_time);
	}
}

/**
  * @brief  background ticks, rtc_task repeat invoking interval 1s.
  * @param  None.
  * @retval None.
  */
void rtc_task_get_current_time(u16 year, u8 month, u8 date,
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
		hours > 23 || minutes > 59 || seconds > 59)
		return false;

	// February validation
	if (month == 2) {
		bool is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
		if (date > (is_leap ? 29 : 28))
			return false;
	}

	// Months with 30 days
	if ((month == 4 || month == 6 || month == 9 || month == 11) && date > 30)
		return false;

	return true;
}

void rtc_task_get_stats(struct rtc_task_stats *stats)
{
	*stats = rtc_stats;
}

void rtc_task_reset_stats(void)
{
	memset(&rtc_stats, 0, sizeof(rtc_stats));
	rtc_stats.min_execution_time_ms = UINT_MAX;
	memset(task_execution_history, 0, sizeof(task_execution_history));
	history_index = 0;
}

void rtc_task_set_profiling(bool enable)
{
	performance_profiling_enabled = enable;
	if (enable) {
		rtc_stats.system_start_time = ktime_get();
		pr_info("RTC task performance profiling enabled\n");
	}
}

void rtc_task_print_performance_report(void)
{
	struct rtc_task_stats stats;
	u64 uptime_ms;
	u32 avg_execution_time;

	rtc_task_get_stats(&stats);

	if (stats.total_tasks_executed == 0) {
		pr_info("No RTC tasks have been executed yet\n");
		return;
	}

	uptime_ms = ktime_to_ms(ktime_sub(ktime_get(), stats.system_start_time));
	avg_execution_time = stats.total_execution_time_ms / stats.total_tasks_executed;

	pr_info("=== RTC Task Performance Report ===\n");
	pr_info("System uptime: %llu ms\n", uptime_ms);
	pr_info("Total tasks created: %u\n", stats.total_tasks_created);
	pr_info("Total tasks executed: %u\n", stats.total_tasks_executed);
	pr_info("Total tasks failed: %u\n", stats.total_tasks_failed);
	pr_info("Average execution time: %u ms\n", avg_execution_time);
	pr_info("Min execution time: %u ms\n", stats.min_execution_time_ms);
	pr_info("Max execution time: %u ms\n", stats.max_execution_time_ms);
	pr_info("Tasks per second: %u\n",
		uptime_ms > 0 ? (stats.total_tasks_executed * 1000) / uptime_ms : 0);
	pr_info("===================================\n");
}

struct rtc_task *rtc_task_find_by_callback(void (*callback)(void))
{
	struct rtc_task *rtc_task;

	list_for_each_entry(rtc_task, &rtc_task_head, list) {
		if (rtc_task->callback == callback)
			return rtc_task;
	}

	return NULL;
}

int rtc_task_get_current_time_safe(u16 *year, u8 *month, u8 *date,
	u8 *hours, u8 *minutes, u8 *seconds)
{
	if (!year || !month || !date || !hours || !minutes || !seconds)
		return -EINVAL;

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
		if (rtc_task->self_alloc)
			kfree(rtc_task);
	}

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_suspend_head, list) {
		list_del(&rtc_task->list);
		if (rtc_task->self_alloc)
			kfree(rtc_task);
	}

	pr_info("All RTC tasks cleaned up\n");
}

#ifdef DESIGN_VERIFICATION_RTCTASK
#include "kinetis/test-kinetis.h"

#include <linux/printk.h>
#include <linux/iopoll.h>

static u64 time_stamp;
static int test_callback_count = 0;

void rtc_task_callback(void)
{
	s64 delta;
	test_callback_count++;

	delta = ktime_ms_delta(ktime_get(),  time_stamp);
	pr_info("RTC task callback #%d executed after %llu ms.\n", test_callback_count, delta);

	if (delta >= 59000 && delta <= 61000)
		pr_info("PASS - RTC task timing correct\n");
	else
		pr_info("FAIL - RTC task timing incorrect (expected: 59000-61000ms, actual: %llums)\n", delta);
	time_stamp = ktime_get();
}

void rtc_test_validation_callback(void)
{
	test_callback_count++;
	pr_info("RTC validation task executed - count: %d\n", test_callback_count);
}

void rtc_test_cleanup_callback(void)
{
	pr_info("RTC cleanup task executed\n");
}

int t_rtc_task_add(int argc, char **argv)
{
	int ret;

	pr_info("=== RTC Task Basic Test ===\n");
	time_stamp = ktime_get();

	ret = rtc_task_add(0, 0, 0, 0, 0, 5, true, rtc_task_callback);

	if (ret) {
		pr_err("RTC task creation failed: %d\n", ret);
		return FAIL;
	}

	pr_info("RTC basic task created successfully\n");
	return PASS;
}

int t_rtc_task_validation(int argc, char **argv)
{
	int ret;
	struct rtc_task *found_task;
	struct rtc_task_stats stats;
	u16 year;
	u8 month, date, hour, min, sec;

	pr_info("=== RTC Task Validation Test ===\n");

	// Test 1: Time validation
	if (!rtc_task_validate_time(2024, 2, 29, 23, 59, 59)) {
		pr_err("FAIL - Leap year validation failed\n");
		return FAIL;
	}

	if (rtc_task_validate_time(2023, 2, 29, 23, 59, 59)) {
		pr_err("FAIL - Non-leap year validation failed\n");
		return FAIL;
	}

	pr_info("PASS - Time validation working correctly\n");

	// Test 2: Task creation and lookup
	ret = rtc_task_add(0, 0, 0, 0, 0, 2, true, rtc_test_validation_callback);
	if (ret) {
		pr_err("FAIL - Test task creation failed: %d\n", ret);
		return FAIL;
	}

	found_task = rtc_task_find_by_callback(rtc_test_validation_callback);
	if (!found_task) {
		pr_err("FAIL - Task lookup failed\n");
		return FAIL;
	}

	pr_info("PASS - Task creation and lookup working\n");

	// Test 3: Safe time retrieval
	ret = rtc_task_get_current_time_safe(&year, &month, &date, &hour, &min, &sec);
	if (ret) {
		pr_err("FAIL - Safe time retrieval failed: %d\n", ret);
		return FAIL;
	}

	pr_info("PASS - Safe time retrieval working (time: %04d-%02d-%02d %02d:%02d:%02d)\n",
		year, month, date, hour, min, sec);

	// Test 4: Statistics
	rtc_task_get_stats(&stats);
	if (stats.total_tasks_created == 0) {
		pr_err("FAIL - Statistics not working\n");
		return FAIL;
	}

	pr_info("PASS - Statistics working (created: %u)\n", stats.total_tasks_created);

	// Test 5: Performance profiling
	rtc_task_set_profiling(true);
	rtc_task_print_performance_report();

	// Test 6: Cleanup
	ret = rtc_task_add(0, 0, 0, 0, 0, 1, false, rtc_test_cleanup_callback);
	if (ret) {
		pr_err("FAIL - Cleanup task creation failed: %d\n", ret);
		return FAIL;
	}

	pr_info("PASS - All RTC validation tests completed\n");
	return PASS;
}

int t_rtc_task_performance(int argc, char **argv)
{
	int ret, i;
	u64 start_time, end_time;
	struct rtc_task_stats stats;

	pr_info("=== RTC Task Performance Test ===\n");

	// Reset statistics
	rtc_task_reset_stats();
	rtc_task_set_profiling(true);

	start_time = ktime_get();

	// Create multiple tasks
	for (i = 0; i < 10; i++) {
		ret = rtc_task_add(0, 0, 0, 0, 0, 1, true, rtc_test_validation_callback);
		if (ret) {
			pr_err("FAIL - Performance task %d creation failed: %d\n", i, ret);
			return FAIL;
		}
	}

	end_time = ktime_get();
	pr_info("PASS - 10 RTC tasks created in %llu ms\n",
		ktime_to_ms(ktime_sub(end_time, start_time)));

	// Run tasks briefly
	ktime_t test_end = ktime_add_ms(ktime_get(), 3000);
	while (ktime_compare(ktime_get(), test_end) < 0)
		rtc_task_loop();
	// Get final stats
	rtc_task_get_stats(&stats);
	pr_info("Performance stats:\n");
	pr_info("  Tasks created: %u\n", stats.total_tasks_created);
	pr_info("  Tasks executed: %u\n", stats.total_tasks_executed);
	pr_info("  Max execution time: %u ms\n", stats.max_execution_time_ms);

	return PASS;
}

int t_rtc_task_cleanup(int argc, char **argv)
{
	pr_info("=== RTC Task Cleanup Test ===\n");

	// Reset callback count
	test_callback_count = 0;

	// Create a cleanup task
	int ret = rtc_task_add(0, 0, 0, 0, 0, 1, false, rtc_test_cleanup_callback);
	if (ret) {
		pr_err("FAIL - Cleanup test task creation failed: %d\n", ret);
		return FAIL;
	}

	// Set current time to trigger task
	rtc_task_get_current_time(2024, 1, 1, 0, 0, 0);

	// Run loop to execute task
	rtc_task_loop();

	// Cleanup all tasks
	rtc_task_cleanup_all();

	// Verify cleanup
	struct rtc_task_stats stats;
	rtc_task_get_stats(&stats);

	pr_info("PASS - Cleanup completed (final stats: created=%u, executed=%u)\n",
		stats.total_tasks_created, stats.total_tasks_executed);

	return PASS;
}

#endif

