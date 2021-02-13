#ifndef __K_TIMTASK_H
#define __K_TIMTASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/list.h>

#include "kinetis/core_common.h"

struct tim_task {
    u64 timeout;
    u32 interval;
    void (*callback)(void);
    struct list_head list;
    bool auto_load;
};

int tim_task_add(u32 interval, bool auto_load, void(*callback)());
int tim_task_drop(void(*callback)());
void tim_task_loop(void);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_TIMTASK_H */
