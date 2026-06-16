
#include <linux/printk.h>
#include <linux/slab.h>

#include "config.h"
#include "hall.h"

#define EXTERN_INLINE_MATRIX static INLINE
#define EXTERN_INLINE_KALMAN static INLINE

#define KALMAN_NAME hall_period
#define KALMAN_NUM_STATES 			1
#define KALMAN_NUM_INPUTS 			1
#include "kalman_factory_filter.h"

#define KALMAN_MEASUREMENT_NAME 	period
#define KALMAN_NUM_MEASUREMENTS 	1
#include "kalman_factory_measurement.h"
#include "kalman_factory_cleanup.h"

static void hall_kalman_init(void)
{
	kalman_t *kf = kalman_filter_hall_period_init();
	kalman_measurement_t *kfm = kalman_filter_hall_period_measurement_period_init();

	/* A = [1] — period stays constant between predictions */
	matrix_t *A = kalman_get_state_transition(kf);
	matrix_set(A, 0, 0, 1.0f);

	/* B = [1] — inject process noise via B*Q*B' */
	matrix_t *B = kalman_get_input_transition(kf);
	matrix_set(B, 0, 0, 1.0f);

	/* Q = [25] — process noise variance (~5 ticks std dev) */
	matrix_t *Q = kalman_get_input_covariance(kf);
	matrix_set(Q, 0, 0, 25.0f);

	/* x0 = 66667 — initial guess: 900 RPM theoretical period */
	matrix_t *x = kalman_get_state_vector(kf);
	x->data[0] = 66667.0f;

	/* P0 = 1e6 — large initial uncertainty, fast convergence */
	matrix_t *P = kalman_get_system_covariance(kf);
	matrix_set(P, 0, 0, 1000000.0f);

	/* H = [1] — direct measurement model */
	matrix_t *H = kalman_get_measurement_transformation(kfm);
	matrix_set(H, 0, 0, 1.0f);

	/* R = [400] — measurement noise (~20 ticks std dev) */
	matrix_t *R = kalman_get_process_noise(kfm);
	matrix_set(R, 0, 0, 400.0f);
}

u32 hall_get_angle(struct hall_device *dev)
{
	u32 column;

	if (dev->period_ticks == 0) {
		return 0;
	}

	/* Map to column number */
	column = (dev->read_rotated_time(dev) * dev->angular_resolution) / dev->period_ticks;
	if (column >= dev->angular_resolution) {
		column = dev->angular_resolution - 1;
	}

	return column;
}

s32 hall_get_rpm(struct hall_device *dev)
{
	if (dev->period_ticks == 0) {
		return 0;
	}

	return 60000000 / dev->period_ticks;
}

void hall_hall_isr(struct hall_device *dev, u32 capture_value)
{
	kalman_t *kf = &kalman_filter_hall_period;
	kalman_measurement_t *kfm = &kalman_filter_hall_period_measurement_period;
	matrix_t *z = kalman_get_measurement_vector(kfm);
	matrix_t *x = kalman_get_state_vector(kf);
	static u32 pulse_cnt;
	u32 raw_ticks;
	u32 filter_ticks;
	s32 delta;

	/* Time update: predict state and covariance                */
	/*   x = A*x        (period unchanged in prediction)        */
	/*   P = A*P*A' + B*Q*B'  (add process noise)               */
	kalman_predict(kf);

	/* Measurement update: inject the raw hall pulse interval   */
	matrix_set(z, 0, 0, (matrix_data_t)capture_value);

	/* Correct: fuse prediction with measurement                */
	kalman_correct(kf, kfm);

	/* Store filtered period_ticks (round to nearest integer)   */
	dev->period_ticks = (u32)(x->data[0] + 0.5f);

	/* Log comparison: raw vs filtered (every 5 pulses)         */
	raw_ticks = capture_value;
	filter_ticks = dev->period_ticks;
	delta = (s32)raw_ticks - (s32)filter_ticks;
	if ((pulse_cnt++ & 7) == 0) {
		pr_info("kalman: #%-4u raw: %u(%dRPM) filter: %u(%dRPM) delta: %+d\n",
			pulse_cnt,
			raw_ticks, 60000000 / raw_ticks,
			filter_ticks, 60000000 / filter_ticks,
			delta);
	}
}

struct hall_device *hall_alloc_dev(u32 angular_resolution, u32(*read_rotated_time)(struct hall_device *dev))
{
	struct hall_device *dev;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		return NULL;
	}

	/*
	 * TIM2 configuration for input capture:
	 * - Clock: 200MHz / (199+1) = 1MHz (1us tick)
	 * - Channel 1: Input capture on hall sensor GPIO
	 * - Interrupt on capture event
	 *
	 * This would be done via STM32 HAL:
	 *   htim2.Init.Prescaler = POV_TIM_PRESCALER;
	 *   htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	 *   htim2.Init.Period = 0xFFFFFFFF;
	 *   HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
	 */

	dev->angular_resolution = angular_resolution;
	dev->read_rotated_time = read_rotated_time;

	/* Initialize Kalman filter before first hall pulse */
	hall_kalman_init();

#if KINETIS_FAKE_SIM
	/* ~66.7ms per revolution — feeds initial guess through filter */
	hall_hall_isr(dev, 60000000 / POV_TARGET_RPM);
	pr_info("hall was initialized in simulation mode\n");
#endif

	return dev;
}

#if KINETIS_FAKE_SIM

#include <kinetis/basic-timer.h>

u32 hall_read_rotated_time(struct hall_device *dev)
{
	return basic_timer_get_us() % dev->period_ticks;
}
#endif /* KINETIS_FAKE_SIM */
