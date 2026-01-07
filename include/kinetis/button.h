#ifndef __K_KEY_H
#define __K_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/list.h>

#include "kinetis/core_common.h"
#include "kinetis/tim-task.h"

struct button;

typedef void (*button_callback)(struct button *);

enum press_button_event {
    NONE_PRESS = 0,
    PRESS_DEBOUNCE,
    PRESS_DOWN,
    PRESS_UP,
    SINGLE_CLICK,
    DOUBLE_CLICK,
    LONG_RRESS,
    PRESS_REPEAT,
    PRESSEVENT_NBR,
};

struct button {
    u32 unique_id;
    u64 valid_ticks;
    u8 cnt : 2;
    u8 repeat : 4;
    u8 event : 4;
    u8 state : 3;
    u8 debounce_cnt : 3;
    u8 active_level : 1;
	DECLARE_BITMAP(button_level, 3);
    u8(*hal_button_level)(struct button *);
    struct tim_task task;
    button_callback callback;
    struct list_head list;
};

int button_task_init(void);
void button_task_exit(void);

int button_add(u32 unique_id, u8(*pin_level)(struct button *), u8 active_level,
    button_callback callback);
void button_drop(u32 unique_id);
void button_polling(void);


/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_KEY_H */
