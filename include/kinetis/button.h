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
#include "kinetis/design_verification.h"

struct button;

typedef void (*button_callback)(struct button *);

enum button_state_machine {
	PRESS_IDLE = 0,
	PRESS_FIRST_EDGE,
	PRESS_DEBOUNCE,
	PRESS_DOWN,
	PRESS_SECOND_EDGE,
	RELEASE_DEBOUNCE,
	PRESS_UP
};

static inline const char *button_state_to_string(enum button_state_machine state)
{
	switch (state) {
	case PRESS_IDLE:
		return "PRESS_IDLE";
	case PRESS_FIRST_EDGE:
		return "PRESS_FIRST_EDGE";
	case PRESS_DEBOUNCE:
		return "PRESS_DEBOUNCE";
	case PRESS_DOWN:
		return "PRESS_DOWN";
	case PRESS_SECOND_EDGE:
		return "PRESS_SECOND_EDGE";
	case RELEASE_DEBOUNCE:
		return "RELEASE_DEBOUNCE";
	case PRESS_UP:
		return "PRESS_UP";
	default:
		return "UNKNOWN";
	}
}

enum button_event {
	NONE_PRESS = 0,
	SINGLE_CLICK,
	DOUBLE_CLICK,
	LONG_PRESS,
	REPEAT_PRESS,
	PRESS_EVENT_NBR
};

static inline const char *button_event_to_string(enum button_event event)
{
	switch (event) {
	case NONE_PRESS:
		return "NONE_PRESS";
	case SINGLE_CLICK:
		return "SINGLE_CLICK";
	case DOUBLE_CLICK:
		return "DOUBLE_CLICK";
	case LONG_PRESS:
		return "LONG_PRESS";
	case REPEAT_PRESS:
		return "REPEAT_PRESS";
	default:
		return "UNKNOWN";
	}
}

#ifdef KINETIS_FAKE_SIM
/* button simulation state for fake mode */
struct button_sim_state {
	u32 unique_id;
	u64 press_time;    /* When button was pressed */
	u64 release_time;        /* When button was released */
	u8 current_level;        /* Current GPIO level (0=low, 1=high) */
	u8 active_level;         /* Active level (0=low, 1=high) */
	u8 debounce_counter;      /* Debounce counter */
	u8 click_count;
	bool start;
	enum button_state_machine next_state;
	enum button_event next_event;
};
#endif

/* According to your need to modify the constants. */
#define TICKS_INTERVAL    5 //ms
#define DEBOUNCE_CNT      3 //MAX 8
#define SHORT_TICKS       (300  / TICKS_INTERVAL)
#define LONG_TICKS        (1000 / TICKS_INTERVAL)

struct button {
	u32 unique_id;
	u64 valid_ticks;
	u8 cnt;
	u8 repeat;
	u8 debounce_cnt;
	u8 active_level;
	enum button_event event;
	enum button_state_machine state;
	DECLARE_BITMAP(button_level, DEBOUNCE_CNT);
	u8(*hal_button_level)(struct button *);
	struct tim_task task;
	button_callback callback;
	struct list_head list;

#ifdef KINETIS_FAKE_SIM
	struct button_sim_state sim_state;
#endif
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
