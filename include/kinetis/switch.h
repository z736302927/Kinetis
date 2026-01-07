#ifndef __K_SWITCH_H
#define __K_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/list.h>

#include "kinetis/core_common.h"

typedef void (*switch_callback)(void *);

enum switch_event {
    SWITCH_DOWN = 0,
    SWITCH_UP,
    SWITCHEVENT_NBR,
    NONE_SWITCH
};

struct _switch {
    u32 unique_id;
    u8  event : 4;
    u8  state : 3;
    u8  debounce_cnt : 3;
    u8  active_level : 1;
    u8  switch_level : 1;
    u8(*hal_switch_level)(void);
    switch_callback callback[SWITCHEVENT_NBR];
    struct list_head list;
};

int switch_task_init(void);
void switch_task_exit(void);

int switch_add(u32 unique_id, u8(*pin_level)(void), u8 active_level,
    switch_callback callback);
void switch_drop(u32 unique_id);
void switch_ticks(void);
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_SWITCH_H */
