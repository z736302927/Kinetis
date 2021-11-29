#ifndef __K_TIMTASK_H
#define __K_TIMTASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <generated/deconfig.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/ktime.h>

struct tim_task {
    ktime_t timeout;
    u32 interval;
    void (*callback)(struct tim_task *);
    struct list_head list;
    bool auto_load;
    bool self_alloc;
};

int tim_task_add(u32 interval, bool auto_load, void(*callback)());
int tim_task_drop(void(*callback)());
int tim_task_enqueue(struct tim_task *tim_task,
    u32 interval, void(*callback)(struct tim_task *));
void tim_task_dequeue(struct tim_task *tim_task);
int tim_task_suspend(void(*callback)());
int tim_task_resume(void(*callback)());
void tim_task_loop(void);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_TIMTASK_H */
