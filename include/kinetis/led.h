#ifndef __K_LED_H
#define __K_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/list.h>

#include "kinetis/core_common.h"

enum led_color {
	LED_COLOR_NONE,
	LED_COLOR_RED,
	LED_COLOR_GREEN,
	LED_COLOR_BLUE
};

enum led_status {
	LED_STATUS_ON,
	LED_STATUS_OFF,
	LED_STATUS_TOGGLE
};

struct rgb_tricolor {
	u8 red;
	u8 green;
	u8 blue;
};

struct led_device {
	u16 unique_id;
	enum led_color color;
	struct rgb_tricolor rgb;
	void *group;
	struct list_head list;

    void (*config_color)(u8 red, u8 green, u8 blue);
};

int led_config_color(u32 unique_id, enum led_color color);
int led_add(u32 unique_id, void (*config_color)(u8 red, u8 green, u8 blue), void *group);
void led_drop(u32 unique_id);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_LED_H */
