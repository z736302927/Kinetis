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
    RED,
    GREEN,
    BLUE
};

enum led_status {
    ON,
    OFF,
    TOGGLE
};

struct led {
    u32 unique_id;
    enum led_color color;
    void *group;
    u32 num;
    struct list_head list;
};

int led_add(u32 unique_id, enum led_color color, void *group, u32 num);
void led_drop(u32 unique_id);
void led_operation(u32 unique_id, enum led_status status);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_LED_H */
