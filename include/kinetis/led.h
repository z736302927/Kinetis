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

enum led_type {
    LED_TYPE_SINGLE,   /* 单色LED (普通LED) */
    LED_TYPE_RGB       /* RGB LED (可调色) */
};

struct rgb_color {
    u8 red;   /* 红色分量 (0-255) */
    u8 green; /* 绿色分量 (0-255) */
    u8 blue;  /* 蓝色分量 (0-255) */
};

/* 预定义颜色常量 */
enum preset_color {
    COLOR_OFF,          /* 关闭 (0, 0, 0) */
    COLOR_RED,          /* 纯红色 (255, 0, 0) */
    COLOR_GREEN,        /* 纯绿色 (0, 255, 0) */
    COLOR_BLUE,         /* 纯蓝色 (0, 0, 255) */
    COLOR_YELLOW,       /* 黄色 (255, 255, 0) */
    COLOR_CYAN,         /* 青色 (0, 255, 255) */
    COLOR_MAGENTA,      /* 洋红色 (255, 0, 255) */
    COLOR_WHITE,        /* 白色 (255, 255, 255) */
    COLOR_PURPLE,       /* 紫色 (128, 0, 128) */
    COLOR_ORANGE,       /* 橙色 (255, 128, 0) */
    COLOR_PINK,         /* 粉色 (255, 192, 203) */
    COLOR_DIM_RED,      /* 暗红色 50% (128, 0, 0) */
    COLOR_DIM_GREEN,    /* 暗绿色 50% (0, 128, 0) */
    COLOR_DIM_BLUE,     /* 暗蓝色 50% (0, 0, 128) */
    COLOR_GRAY,         /* 灰色 (128, 128, 128) */
    COLOR_CUSTOM        /* 自定义颜色 */
};

struct led {
    u32 unique_id;
    enum led_color color;
    enum led_status status;
    enum led_type type;       /* LED类型：单色或RGB */
    struct rgb_color rgb;      /* RGB颜色值 (仅RGB LED使用) */
    void *group;
    u32 num;
    struct list_head list;
};

int led_add(u32 unique_id, enum led_color color, void *group, u32 num);
int led_add_rgb(u32 unique_id, enum led_type type, void *group, u32 num);
void led_drop(u32 unique_id);
void led_operation(u32 unique_id, enum led_status status);
void led_set_rgb(u32 unique_id, u8 red, u8 green, u8 blue);
void led_set_rgb_preset(u32 unique_id, enum preset_color preset);
void led_get_rgb(u32 unique_id, struct rgb_color *rgb);
const char *led_get_color_name(enum preset_color preset);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


#ifdef __cplusplus
}
#endif

#endif /* __K_LED_H */
