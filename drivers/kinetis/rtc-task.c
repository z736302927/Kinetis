
#include <generated/deconfig.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/iopoll.h>

#include "rtc-task.h"


//rtc_task rtc_task list head.
static LIST_HEAD(rtc_task_head);
static LIST_HEAD(rtc_task_suspend_head);

//struct rtc_task ticks
static struct tm current_time = {.tm_year = 0};

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
		return time_after32(current_time.tm_year, rtc_task->expired.tm_year);

	if (current_time.tm_mday != rtc_task->expired.tm_mday)
		return time_after32(current_time.tm_year, rtc_task->expired.tm_year);

	if (current_time.tm_hour != rtc_task->expired.tm_hour)
		return time_after32(current_time.tm_year, rtc_task->expired.tm_year);

	if (current_time.tm_min != rtc_task->expired.tm_min)
		return time_after32(current_time.tm_year, rtc_task->expired.tm_year);

	if (current_time.tm_sec != rtc_task->expired.tm_sec)
		return time_after32(current_time.tm_year, rtc_task->expired.tm_year);

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
	++date_time->tm_hour;

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
int rtc_task_add(u8 add_year, u8 add_month, u8 add_date,
	u8 add_hours, u8 add_minutes, u8 add_seconds,
	bool auto_load, void(*callback)())
{
	struct rtc_task *rtc_task;
	int ret, val;

	ret = readl_poll_timeout_atomic(&current_time.tm_year, val, val, 1, 3000000);

	if (ret) {
		printk(KERN_ERR "Native time has not been updated.\n");
		return -ETIMEDOUT;
	}

	rtc_task = kmalloc(sizeof(*rtc_task), GFP_KERNEL);

	if (!rtc_task)
		return -ENOMEM;

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

	rtc_task->auto_load = auto_load;
	rtc_task->self_alloc = true;

	list_add_tail(&rtc_task->list, &rtc_task_head);

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
	u8 add_year, u8 add_month, u8 add_date,
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
  * @brief  Task loop.
  * @param  None.
  * @retval None
  */
void rtc_task_loop(void)
{
	struct rtc_task *rtc_task, *tmp;

	list_for_each_entry_safe(rtc_task, tmp, &rtc_task_head, list) {
		if (rtc_task_expired(rtc_task)) {
			rtc_task->callback();

			if (rtc_task->auto_load)
				rtc_task_update_time(rtc_task);
			else {
				list_del(&rtc_task->list);

				if (rtc_task->self_alloc == true)
					kfree(rtc_task);
			}
		}
	}
}

/**
  * @brief  background ticks, rtc_task repeat invoking interval 1s.
  * @param  None.
  * @retval None.
  */
void rtc_task_get_current_time(u8 year, u8 month, u8 date,
	u8 hours, u8 minutes, u8 seconds)
{
	current_time.tm_year = year;
	current_time.tm_mon = month;
	current_time.tm_mday = date;
	current_time.tm_hour = hours;
	current_time.tm_min = minutes;
	current_time.tm_sec = seconds;
}

#ifdef DESIGN_VERIFICATION_RTCTASK
#include "kinetis/test-kinetis.h"
#include "kinetis/timeout.h"

#include <linux/printk.h>
#include <linux/iopoll.h>

static u64 time_stamp;

void rtc_task_callback(void)
{
	time_stamp = basic_timer_get_ms() - time_stamp;
	printk(KERN_DEBUG "timeout! rtc_task elapse time = %llu ms.\n", time_stamp);

	if (time_stamp >= 59000 && time_stamp <= 61000)
		printk(KERN_DEBUG "PASS\n");
	else
		printk(KERN_DEBUG "FAIL\n");
}

int t_rtc_task_add(int argc, char **argv)
{
	int ret;

	time_stamp = basic_timer_get_ms();

	ret = rtc_task_add(0, 0, 0, 0, 1, 0, false, rtc_task_callback); //60s loop

	if (ret)
		return FAIL;

	return PASS;

err:
	printk(KERN_ERR "Failed to execute %s(), error code: %d\n",
		__func__, ret);
	return FAIL;
}

#endif

