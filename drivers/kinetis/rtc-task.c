#include "kinetis/rtc-task.h"
#include "kinetis/idebug.h"

#include <linux/slab.h>
#include <linux/errno.h>

#include "string.h"

//rtc_task rtc_task list head.
static LIST_HEAD(rtc_task_head);

//struct rtc_task ticks
static struct rtc_task_date_time current_time;

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in function rtc_task_Test, design the function you need and initialize it in the main function.
  * @step 3:  Call the time update function rtc_task_GetCurrentTime every second.
  * @step 4:  An infinite loop calls function rtc_task_Loop.
  */


/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

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

    rtc_task = kmalloc(sizeof(*rtc_task), GFP_KERNEL);

    if (!rtc_task)
        return -ENOMEM;

    rtc_task->callback = callback;

    rtc_task->interval.year = add_year;
    rtc_task->interval.month = add_month;
    rtc_task->interval.date = add_date;
    rtc_task->interval.hours = add_hours;
    rtc_task->interval.minutes = add_minutes;
    rtc_task->interval.seconds = add_seconds;

    rtc_task->expired_time.year = current_time.year;
    rtc_task->expired_time.month = current_time.month;
    rtc_task->expired_time.date = current_time.date;
    rtc_task->expired_time.hours = current_time.hours + rtc_task->interval.hours;
    rtc_task->expired_time.minutes = current_time.minutes + rtc_task->interval.minutes;
    rtc_task->expired_time.seconds = current_time.seconds + rtc_task->interval.seconds;

    rtc_task->auto_load = auto_load;

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
  * @brief  .
  * @param  None.
  * @retval None
  */
static bool rtc_task_expired(struct rtc_task *rtc_task)
{
    u8 time_cmp1[12];
    u8 time_cmp2[12];
    int ret = 0;

    snprintf((char *)time_cmp1, sizeof(time_cmp1), "%02d%02d%02d%02d%02d%02d",
        current_time.year, current_time.month, current_time.date,
        current_time.hours, current_time.minutes, current_time.seconds);

    snprintf((char *)time_cmp2, sizeof(time_cmp2), "%02d%02d%02d%02d%02d%02d",
        rtc_task->expired_time.year, rtc_task->expired_time.month, rtc_task->expired_time.date,
        rtc_task->expired_time.hours, rtc_task->expired_time.minutes, rtc_task->expired_time.seconds);

    ret = strncmp((char *)time_cmp1, (char *)time_cmp2, sizeof(time_cmp1));

    if (ret < 0)
        return false;
    else
        return true;
}

static void rtc_task_special_add_days(struct rtc_task_date_time *date_time)
{
    /* Is it February?,28,29 */
    if (date_time->month == 2) {
        /* The system will certainly not be in use until 2100, so it only judges whether it is divisible by 4 */ //Leap year
        if (date_time->year % 4 == 0) {
            //No more than 29 days
            if (date_time->date < 30)
                return ;

            //Minus the overflow of 29
            //month +1
            date_time->date -= 29;
            ++date_time->month;
        } else {
            //No more than 28 days
            if (date_time->date < 29)
                return ;

            //Minus the overflow of 28
            //month +1
            date_time->date -= 28;
            ++date_time->month;
        }
    }

    // Is it a 30-day month
    if (date_time->month == 4 || date_time->month == 6 ||
        date_time->month == 9 || date_time->month == 11) {
        //No more than 30 days
        if (date_time->date < 31)
            return;

        //Minus the overflow of 30
        //month+1
        date_time->date -= 30;
        ++date_time->month;
    }

    // Is it a 31-day month
    if (date_time->month == 1 || date_time->month == 3 ||
        date_time->month == 5 || date_time->month == 7 ||
        date_time->month == 8 || date_time->month == 10 ||
        date_time->month == 12) {
        //No more than 31 days
        if (date_time->date < 32)
            return;

        //Minus the overflow of 30
        //month +1
        date_time->date -= 31;
        ++date_time->month;
    }

    //No more than December

    if (date_time->month < 13)
        return ;

    //Minus 12 months of overflow
    //year +1
    date_time->month -= 12;
    ++date_time->year;

    return;
}

static void rtc_task_time_add_seconds(struct rtc_task_date_time *date_time, u8 seconds)
{
    if (seconds > 60)
        return ;

    //Plus the number of seconds
    date_time->seconds += seconds;

    //Finished
    if (date_time->seconds < 60)
        return ;

    //Minute + 1
    date_time->seconds -= 60;
    ++date_time->hours;

    //Finished
    if (date_time->minutes < 60)
        return ;

    //hours + 1
    date_time->minutes -= 60;
    ++date_time->hours;

    //Finished
    if (date_time->hours < 24)
        return ;

    //Days + 1;
    date_time->hours -= 24;
    ++date_time->date;

    rtc_task_special_add_days(date_time);
}

// minutes cannot be greater than 60 minutes
static void rtc_task_time_add_minutes(struct rtc_task_date_time *date_time, u8 minutes)
{
    if (minutes > 60)
        return ;

    //Plus the number of minutes
    date_time->minutes += minutes;

    //Finished
    if (date_time->minutes < 60)
        return ;

    //hours + 1
    date_time->minutes -= 60;
    ++date_time->hours;

    //Finished
    if (date_time->hours < 24)
        return ;

    //days + 1;
    date_time->hours -= 24;
    ++date_time->date;

    rtc_task_special_add_days(date_time);
}

// An hour cannot be greater than 24
static void rtc_task_time_add_hours(struct rtc_task_date_time *date_time, u8 hours)
{
    if (hours > 24)
        return;

    date_time->hours += hours;

    //Finished
    if (date_time->hours < 24)
        return ;

    //days + 1;
    date_time->hours -= 24;
    ++date_time->date;

    rtc_task_special_add_days(date_time);
}

static void rtc_task_time_add_days(struct rtc_task_date_time *date_time, u8 days)
{
    //Make sure january doesn't jump to march
    if (days > 28)
        return;

    date_time->date += days;
    rtc_task_special_add_days(date_time);
}

static void rtc_task_time_add_months(struct rtc_task_date_time *date_time, u8 months)
{
    if (months > 12)
        return;

    date_time->month += months;

    if (date_time->month < 13)
        return;

    date_time->month -= 12;
    ++date_time->year;
}

/**
  * @brief  Use round and align.
  * @param  None.
  * @retval None
  */
static void rtc_task_update_time(struct rtc_task *rtc_task)
{
    rtc_task->expired_time.year = current_time.year;
    rtc_task->expired_time.month = current_time.month;
    rtc_task->expired_time.date = current_time.date;
    rtc_task->expired_time.hours = current_time.hours;
    rtc_task->expired_time.minutes = current_time.minutes;
    rtc_task->expired_time.seconds = current_time.seconds;

//    if (rtc_task->interval.year != 0) {
//        rtc_task->expired_time.seconds = 0;
//        rtc_task->expired_time.minutes = 0;
//        rtc_task->expired_time.hours -=
//            rtc_task->expired_time.hours % rtc_task->interval.hours;
//        rtc_task_time_add_hours(&(rtc_task->expired_time), rtc_task->interval.hours);
//    }

    if (rtc_task->interval.month != 0) {
        rtc_task->expired_time.seconds = 0;
        rtc_task->expired_time.minutes = 0;
        rtc_task->expired_time.hours = 0;
        rtc_task->expired_time.date = 0;
        rtc_task->expired_time.year -=
            rtc_task->expired_time.year % rtc_task->interval.year;
        rtc_task_time_add_days(&(rtc_task->expired_time), rtc_task->interval.month);
    }

    if (rtc_task->interval.date != 0) {
        rtc_task->expired_time.seconds = 0;
        rtc_task->expired_time.minutes = 0;
        rtc_task->expired_time.hours = 0;
        rtc_task->expired_time.date -=
            rtc_task->expired_time.date % rtc_task->interval.date;
        rtc_task_time_add_hours(&(rtc_task->expired_time), rtc_task->interval.date);
    }

    if (rtc_task->interval.hours != 0) {
        rtc_task->expired_time.seconds = 0;
        rtc_task->expired_time.minutes = 0;
        rtc_task->expired_time.hours -=
            rtc_task->expired_time.hours % rtc_task->interval.hours;
        rtc_task_time_add_months(&(rtc_task->expired_time), rtc_task->interval.hours);
    }

    if (rtc_task->interval.minutes != 0) {
        rtc_task->expired_time.seconds = 0;
        rtc_task->expired_time.minutes -=
            rtc_task->expired_time.minutes % rtc_task->interval.minutes;
        rtc_task_time_add_minutes(&(rtc_task->expired_time), rtc_task->interval.minutes);
    }

    if (rtc_task->interval.seconds != 0)
        rtc_task_time_add_seconds(&(rtc_task->expired_time), rtc_task->interval.seconds);
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
void rtc_task_get_current_time(u8 year, u8 month, u8 date, u8 hours, u8 minutes, u8 seconds)
{
    current_time.year = year;
    current_time.month = month;
    current_time.date = date;
    current_time.hours = hours;
    current_time.minutes = minutes;
    current_time.seconds = seconds;
}

#ifdef DESIGN_VERIFICATION_RTCTASK
#include "kinetis/test-kinetis.h"
#include "kinetis/timeout.h"

#include <linux/printk.h>
#include <linux/iopoll.h>

static u8 rtc_task_flag = 0;

void rtc_task_callback(void)
{
    rtc_task_flag = true;
    printk(KERN_DEBUG "rtc_task timeout!");
}

int t_rtc_task_add(int argc, char **argv)
{
    u32 time_stamp = 0;
    bool val;

    time_stamp = basic_timer_get_ms();

    rtc_task_add(0, 0, 0, 0, 1, 0, false, rtc_task_callback); //60s loop

    readl_poll_timeout_atomic(&rtc_task_flag, val, val == true, 1, 2000);

    time_stamp = basic_timer_get_ms() - time_stamp;
    printk(KERN_DEBUG "rtc_task elapse time = %u ms.", time_stamp);

    if (time_stamp > 1100)
        return FAIL;
    else
        return PASS;
}

#endif

