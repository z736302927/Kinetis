/**
 * @file host_angle.h
 * @brief Hall sensor angle detection for host (rotating end)
 * @note Uses TIM2 input capture to measure rotation period and
 *       compute current column number (0~719) and actual RPM
 */

#ifndef POV_HOST_ANGLE_H
#define POV_HOST_ANGLE_H

#include <linux/types.h>
#include "host_pov_config.h"

/*********************************************************************
 * Host Angle Device Structure
 *********************************************************************/

struct host_angle_device {
	u32 last_capture;        /* Last TIM2 capture value */
	u32 period_ticks;        /* Ticks between hall pulses (1 full revolution) */
	u32 current_tick;        /* Current TIM2 counter value */
	u16 current_column;      /* Current display column (0~719) */
	s32 measured_rpm;        /* Measured RPM from hall sensor */
	u8 initialized;

#if KINETIS_FAKE_SIM
	/* Simulation state */
	u64 sim_last_hall_ms;    /* Simulated last hall pulse time */
	u32 sim_period_ms;       /* Simulated revolution period */
	u8 sim_running;          /* Simulation thread running flag */
#endif
};

/*********************************************************************
 * Public API
 *********************************************************************/

/**
 * @brief Initialize host angle detection
 * @param dev: Device pointer
 * @return 0 on success, negative error code on failure
 * @note Configures TIM2 for input capture on hall sensor pin
 */
int host_angle_init(struct host_angle_device *dev);

/**
 * @brief Get current display column number
 * @param dev: Device pointer
 * @return Column number 0~719, or -1 if not initialized
 */
int host_angle_get_column(struct host_angle_device *dev);

/**
 * @brief Get measured RPM from hall sensor
 * @param dev: Device pointer
 * @return Measured RPM, or 0 if not available
 */
s32 host_angle_get_rpm(struct host_angle_device *dev);

/**
 * @brief Check if a new column has been reached since last check
 * @param dev: Device pointer
 * @return 1 if new column, 0 if same column
 * @note This is the main trigger for display updates
 */
int host_angle_is_new_column(struct host_angle_device *dev);

/**
 * @brief TIM2 input capture interrupt callback (hardware mode)
 * @param dev: Device pointer
 * @note Called from TIM2 ISR when hall pulse is captured
 */
void host_angle_hall_isr(struct host_angle_device *dev);

/**
 * @brief Start angle simulation (for KINETIS_FAKE_SIM mode)
 * @param dev: Device pointer
 * @param target_rpm: Simulated RPM
 * @return 0 on success
 */
int host_angle_sim_start(struct host_angle_device *dev, s32 target_rpm);

/**
 * @brief Stop angle simulation
 * @param dev: Device pointer
 */
void host_angle_sim_stop(struct host_angle_device *dev);

#endif /* POV_HOST_ANGLE_H */
