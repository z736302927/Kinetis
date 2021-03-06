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

#include "kinetis/core_common.h"

struct rtc_task {
    struct tm expired;
    struct tm interval;
    void (*callback)();
    struct list_head list;
    bool auto_load;
    bool self_alloc;
};

int rtc_task_add(u8 add_year, u8 add_month, u8 add_date,
    u8 add_hours, u8 add_minutes, u8 add_seconds,
    bool auto_load, void(*callback)());
int rtc_task_drop(void(*callback)());
int rtc_task_enqueue(struct rtc_task *rtc_task,
    u8 add_year, u8 add_month, u8 add_date,
    u8 add_hours, u8 add_minutes, u8 add_seconds,
    bool auto_load, void(*callback)(struct rtc_task *));
void rtc_task_dequeue(struct rtc_task *rtc_task);
int rtc_task_suspend(void(*callback)());
int rtc_task_resume(void(*callback)());
void rtc_task_loop(void);
void rtc_task_get_current_time(u8 year, u8 month, u8 date, u8 hours, u8 minutes, u8 seconds);
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_RTCTASK_H */
