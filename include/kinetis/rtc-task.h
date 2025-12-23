#ifndef __K_RTCTASK_H
#define __K_RTCTASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/list.h>
#include <linux/time.h>

struct rtc_task {
	struct tm expired;
	struct tm interval;
	void (*callback)();
	struct list_head list;
	bool auto_load;
	bool self_alloc;
};

struct rtc_task_stats {
	u32 total_tasks_created;
	u32 total_tasks_executed;
	u32 total_tasks_failed;
	u64 total_execution_time_ms;
	u32 max_execution_time_ms;
	u32 min_execution_time_ms;
	ktime_t system_start_time;
};

int rtc_task_add(u16 add_year, u8 add_month, u8 add_date,
	u8 add_hours, u8 add_minutes, u8 add_seconds,
	bool auto_load, void(*callback)());
int rtc_task_drop(void(*callback)());
int rtc_task_enqueue(struct rtc_task *rtc_task,
	u16 add_year, u8 add_month, u8 add_date,
	u8 add_hours, u8 add_minutes, u8 add_seconds,
	void(*callback)(struct rtc_task *));
void rtc_task_dequeue(struct rtc_task *rtc_task);
int rtc_task_suspend(void(*callback)());
int rtc_task_resume(void(*callback)());
void rtc_task_loop(void);
void rtc_task_get_current_time(u16 year, u8 month, u8 date, u8 hours, u8 minutes, u8 seconds);
bool rtc_task_validate_time(u16 year, u8 month, u8 date,
	u8 hours, u8 minutes, u8 seconds);
void rtc_task_get_stats(struct rtc_task_stats *stats);
void rtc_task_reset_stats(void);
void rtc_task_set_profiling(bool enable);
void rtc_task_print_performance_report(void);
struct rtc_task *rtc_task_find_by_callback(void (*callback)(void));
int rtc_task_get_current_time_safe(u16 *year, u8 *month, u8 *date,
	u8 *hours, u8 *minutes, u8 *seconds);
void rtc_task_cleanup_all(void);
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_RTCTASK_H */
