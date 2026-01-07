#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/errno.h>

#include "kinetis/led.h"
#include "kinetis/design_verification.h"

//LED Handle list head.
static LIST_HEAD(led_head);

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

#if MCU_PLATFORM_STM32
#include "stm32f4xx_hal.h"
#endif

/* PWM control callbacks for RGB LED */
static void (*pwm_set_red_callback)(u8 duty) = NULL;
static void (*pwm_set_green_callback)(u8 duty) = NULL;
static void (*pwm_set_blue_callback)(u8 duty) = NULL;

/**
  * @brief  Register PWM control callbacks for RGB LED
  * @param  red_cb:   PWM callback for red channel
  * @param  green_cb: PWM callback for green channel
  * @param  blue_cb:  PWM callback for blue channel
  * @retval None
  */
void led_register_pwm_callbacks(void (*red_cb)(u8), void (*green_cb)(u8), void (*blue_cb)(u8))
{
	pwm_set_red_callback = red_cb;
	pwm_set_green_callback = green_cb;
	pwm_set_blue_callback = blue_cb;
}

/**
  * @brief  Update RGB LED PWM values
  * @param  led: Pointer to LED structure
  * @retval None
  */
static void led_update_rgb_pwm(struct led *led)
{
	if (led->type != LED_TYPE_RGB) {
		return;
	}

#ifdef KINETIS_FAKE_SIM
	pr_info("RGB LED[%d] - R:%d G:%d B:%d (SIM mode)\n",
		led->unique_id, led->rgb.red, led->rgb.green, led->rgb.blue);
#endif

	/* Call PWM callbacks to update hardware */
	if (pwm_set_red_callback) {
		pwm_set_red_callback(led->rgb.red);
	}
	if (pwm_set_green_callback) {
		pwm_set_green_callback(led->rgb.green);
	}
	if (pwm_set_blue_callback) {
		pwm_set_blue_callback(led->rgb.blue);
	}
}

/**
  * @brief  Get RGB values for preset color
  * @param  preset: Preset color enum
  * @param  rgb: Output RGB values
  * @retval None
  */
static void led_get_preset_rgb(enum preset_color preset, struct rgb_color *rgb)
{
	switch (preset) {
	case COLOR_OFF:
		rgb->red = 0;
		rgb->green = 0;
		rgb->blue = 0;
		break;
	case COLOR_RED:
		rgb->red = 255;
		rgb->green = 0;
		rgb->blue = 0;
		break;
	case COLOR_GREEN:
		rgb->red = 0;
		rgb->green = 255;
		rgb->blue = 0;
		break;
	case COLOR_BLUE:
		rgb->red = 0;
		rgb->green = 0;
		rgb->blue = 255;
		break;
	case COLOR_YELLOW:
		rgb->red = 255;
		rgb->green = 255;
		rgb->blue = 0;
		break;
	case COLOR_CYAN:
		rgb->red = 0;
		rgb->green = 255;
		rgb->blue = 255;
		break;
	case COLOR_MAGENTA:
		rgb->red = 255;
		rgb->green = 0;
		rgb->blue = 255;
		break;
	case COLOR_WHITE:
		rgb->red = 255;
		rgb->green = 255;
		rgb->blue = 255;
		break;
	case COLOR_PURPLE:
		rgb->red = 128;
		rgb->green = 0;
		rgb->blue = 128;
		break;
	case COLOR_ORANGE:
		rgb->red = 255;
		rgb->green = 128;
		rgb->blue = 0;
		break;
	case COLOR_PINK:
		rgb->red = 255;
		rgb->green = 192;
		rgb->blue = 203;
		break;
	case COLOR_DIM_RED:
		rgb->red = 128;
		rgb->green = 0;
		rgb->blue = 0;
		break;
	case COLOR_DIM_GREEN:
		rgb->red = 0;
		rgb->green = 128;
		rgb->blue = 0;
		break;
	case COLOR_DIM_BLUE:
		rgb->red = 0;
		rgb->green = 0;
		rgb->blue = 128;
		break;
	case COLOR_GRAY:
		rgb->red = 128;
		rgb->green = 128;
		rgb->blue = 128;
		break;
	case COLOR_CUSTOM:
	default:
		/* Keep current values */
		break;
	}
}

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/LEDn_Type/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

static inline void led_init(struct led *led)
{
#if MCU_PLATFORM_STM32
#else
#endif

#if MCU_PLATFORM_STM32
	GPIO_InitTypeDef  GPIO_InitStruct;

	/* Enable the GPIO_LED Clock */
	__HAL_RCC_GPIOC_CLK_ENABLE();

	/* Configure the GPIO_LED pin */
	GPIO_InitStruct.Pin = led->num;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	HAL_GPIO_Init(led->group, &GPIO_InitStruct);

	HAL_GPIO_WritePin(led->group, led->num, GPIO_PIN_RESET);
#elif KINETIS_FAKE_SIM
	pr_info("LED[%d] initialized (SIM mode)\n", led->unique_id);
#else
#endif
}

static inline void led_set_status(struct led *led, enum led_status status)
{
	switch (status) {
	case ON:
		led->status = ON;
#if MCU_PLATFORM_STM32
		HAL_GPIO_WritePin(led->group, led->num, GPIO_PIN_SET);
#elif KINETIS_FAKE_SIM
		pr_info("LED[%d] turned ON (SIM mode)\n", led->unique_id);
#else
#endif
		break;

	case OFF:
		led->status = OFF;
#if MCU_PLATFORM_STM32
		HAL_GPIO_WritePin(led->group, led->num, GPIO_PIN_RESET);
#elif KINETIS_FAKE_SIM
		pr_info("LED[%d] turned OFF (SIM mode)\n", led->unique_id);
#else
#endif
		break;

	case TOGGLE:
		if (led->status == ON) {
			led->status = OFF;
		} else {
			led->status = ON;
		}
#if MCU_PLATFORM_STM32
		HAL_GPIO_TogglePin(led->group, led->num);
#elif KINETIS_FAKE_SIM
		pr_info("LED[%d] toggled to %s (SIM mode)\n",
			led->unique_id,
			led->status == ON ? "ON" : "OFF");
#else
#endif
		break;

	default:
		;
	}
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @brief  Start the LED work, add the handle into work list.
  * @param  btn: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int led_add(u32 unique_id, enum led_color color, void *group, u32 num)
{
	struct led *led;

	led = kmalloc(sizeof(*led), GFP_KERNEL);

	if (!led) {
		return -ENOMEM;
	}

	led->unique_id = unique_id;
	led->color = color;
	led->type = LED_TYPE_SINGLE;
	led->status = OFF;
	led->rgb.red = 0;
	led->rgb.green = 0;
	led->rgb.blue = 0;
	led->group = group;
	led->num = num;
	led_init(led);

	list_add_tail(&led->list, &led_head);

	return 0;
}

int led_add_rgb(u32 unique_id, enum led_type type, void *group, u32 num)
{
	struct led *led;

	led = kmalloc(sizeof(*led), GFP_KERNEL);

	if (!led) {
		return -ENOMEM;
	}

	led->unique_id = unique_id;
	led->color = RED;  /* Default color for RGB LED */
	led->type = type;
	led->status = OFF;
	led->rgb.red = 0;
	led->rgb.green = 0;
	led->rgb.blue = 0;
	led->group = group;
	led->num = num;
	led_init(led);

	list_add_tail(&led->list, &led_head);

	return 0;
}

/**
  * @brief  Stop the LED work, remove the handle off work list.
  * @param  Handle: target handle strcut.
  * @retval None
  */
void led_drop(u32 unique_id)
{
	struct led *led, *tmp;

	list_for_each_entry_safe(led, tmp, &led_head, list) {
		if (led->unique_id == unique_id) {
			list_del(&led->list);
			kfree(led);
			break;
		}
	}
}

/**
  * @brief  Turns selected LED On.
  * @param  LED: Specifies the LED to be set on.
  *   This parameter can be one of following parameters:
  *     @arg LEDx
  */
void led_operation(u32 unique_id, enum led_status status)
{
	struct led *led;

	list_for_each_entry(led, &led_head, list) {
		if (led->unique_id == unique_id) {
			led_set_status(led, status);
			break;
		}
	}
}

void led_set_rgb(u32 unique_id, u8 red, u8 green, u8 blue)
{
	struct led *led;

	list_for_each_entry(led, &led_head, list) {
		if (led->unique_id == unique_id) {
			led->rgb.red = red;
			led->rgb.green = green;
			led->rgb.blue = blue;
			led_update_rgb_pwm(led);
			break;
		}
	}
}

void led_get_rgb(u32 unique_id, struct rgb_color *rgb)
{
	struct led *led;

	list_for_each_entry(led, &led_head, list) {
		if (led->unique_id == unique_id) {
			rgb->red = led->rgb.red;
			rgb->green = led->rgb.green;
			rgb->blue = led->rgb.blue;
			break;
		}
	}
}

void led_set_rgb_preset(u32 unique_id, enum preset_color preset)
{
	struct led *led;
	struct rgb_color rgb;

	list_for_each_entry(led, &led_head, list) {
		if (led->unique_id == unique_id) {
			if (led->type != LED_TYPE_RGB) {
				return;
			}

			led_get_preset_rgb(preset, &rgb);
			led->rgb.red = rgb.red;
			led->rgb.green = rgb.green;
			led->rgb.blue = rgb.blue;
			led_update_rgb_pwm(led);
			break;
		}
	}
}

const char *led_get_color_name(enum preset_color preset)
{
	switch (preset) {
	case COLOR_OFF:
		return "OFF";
	case COLOR_RED:
		return "RED";
	case COLOR_GREEN:
		return "GREEN";
	case COLOR_BLUE:
		return "BLUE";
	case COLOR_YELLOW:
		return "YELLOW";
	case COLOR_CYAN:
		return "CYAN";
	case COLOR_MAGENTA:
		return "MAGENTA";
	case COLOR_WHITE:
		return "WHITE";
	case COLOR_PURPLE:
		return "PURPLE";
	case COLOR_ORANGE:
		return "ORANGE";
	case COLOR_PINK:
		return "PINK";
	case COLOR_DIM_RED:
		return "DIM_RED";
	case COLOR_DIM_GREEN:
		return "DIM_GREEN";
	case COLOR_DIM_BLUE:
		return "DIM_BLUE";
	case COLOR_GRAY:
		return "GRAY";
	case COLOR_CUSTOM:
		return "CUSTOM";
	default:
		return "UNKNOWN";
	}
}

#ifdef DESIGN_VERIFICATION_LED
#include "kinetis/test-kinetis.h"
#include "kinetis/tim-task.h"

static struct tim_task led_task;
static struct tim_task rgb_led_task;
static int rgb_test_step = 0;

/* Mock PWM callbacks for RGB LED testing */
static void mock_pwm_set_red(u8 duty)
{
#ifdef KINETIS_FAKE_SIM
	pr_info("  [PWM RED] Duty: %d/255\n", duty);
#endif
}

static void mock_pwm_set_green(u8 duty)
{
#ifdef KINETIS_FAKE_SIM
	pr_info("  [PWM GREEN] Duty: %d/255\n", duty);
#endif
}

static void mock_pwm_set_blue(u8 duty)
{
#ifdef KINETIS_FAKE_SIM
	pr_info("  [PWM BLUE] Duty: %d/255\n", duty);
#endif
}

static void led_task_callback(struct tim_task *task)
{
	led_operation(1, TOGGLE);
	led_operation(2, TOGGLE);
}

static void rgb_led_test_callback(struct tim_task *task)
{
	/* RGB LED color cycle test using preset colors */
	enum preset_color colors[] = {
		COLOR_RED,      COLOR_GREEN,    COLOR_BLUE,
		COLOR_YELLOW,   COLOR_CYAN,     COLOR_MAGENTA,
		COLOR_WHITE,    COLOR_PURPLE,   COLOR_ORANGE,
		COLOR_PINK,     COLOR_DIM_RED,  COLOR_OFF
	};
	const int num_colors = sizeof(colors) / sizeof(colors[0]);

	if (rgb_test_step >= 0 && rgb_test_step < num_colors) {
		enum preset_color color = colors[rgb_test_step];
		pr_info("=== RGB Test: %s ===\n", led_get_color_name(color));
		led_set_rgb_preset(10, color);
		rgb_test_step++;
	} else {
		rgb_test_step = 0;
	}
}

int t_led_add(int argc, char **argv)
{
	int ret;

	/* Add single color LEDs */
	ret = led_add(1, RED, NULL, 4);
	if (ret) {
		return FAIL;
	}

	ret = led_add(2, GREEN, NULL, 5);
	if (ret) {
		return FAIL;
	}

	/* Start single LED toggle test */
	ret = tim_task_add(&led_task, "led_task",
			1000, true, false, led_task_callback);
	if (ret) {
		return FAIL;
	}

	/* Register PWM callbacks for RGB LED */
	led_register_pwm_callbacks(mock_pwm_set_red, mock_pwm_set_green, mock_pwm_set_blue);

	/* Add RGB LED */
	ret = led_add_rgb(10, LED_TYPE_RGB, NULL, 0);
	if (ret) {
		return FAIL;
	}

	/* Start RGB LED test */
	ret = tim_task_add(&rgb_led_task, "rgb_led_task",
			500, true, false, rgb_led_test_callback);
	if (ret) {
		return FAIL;
	}

	pr_info("LED test started:\n");
	pr_info("  - Single LEDs (1,2) toggling every 1000ms\n");
	pr_info("  - RGB LED (10) cycling colors every 500ms\n");

	return PASS;
}

int t_led_drop(int argc, char **argv)
{
	tim_task_drop(&led_task);
	tim_task_drop(&rgb_led_task);

	led_drop(1);
	led_drop(2);
	led_drop(10);

	return PASS;
}
#endif
