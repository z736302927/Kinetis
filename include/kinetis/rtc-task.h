#ifndef __K_RTCTASK_H
#define __K_RTCTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>
#include <linux/list.h>
#include <linux/time.h>

struct rtc_task {
	struct tm expired;
	struct tm interval;
	void (*callback)(struct rtc_task *);
	struct list_head list;
	bool auto_load;
};

int rtc_task_add(struct rtc_task *task,
	u16 add_year, u8 add_month, u8 add_date,
	u8 add_hours, u8 add_minutes, u8 add_seconds,
	bool auto_load, void(*callback)(struct rtc_task *));
int rtc_task_drop(void(*callback)(struct rtc_task *));
void rtc_task_dequeue(struct rtc_task *task);
int rtc_task_suspend(void(*callback)(struct rtc_task *));
int rtc_task_resume(void(*callback)(struct rtc_task *));
void rtc_task_loop(void);
void rtc_task_set_current_time(u16 year, u8 month, u8 date,
	u8 hours, u8 minutes, u8 seconds);
bool rtc_task_validate_time(u16 year, u8 month, u8 date,
	u8 hours, u8 minutes, u8 seconds);
void rtc_task_cleanup_all(void);
struct rtc_task *rtc_task_find_by_callback(void (*callback)(struct rtc_task *));

#ifdef __cplusplus
}
#endif

#endif
