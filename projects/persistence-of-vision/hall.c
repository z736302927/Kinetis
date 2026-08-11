
#include <linux/printk.h>
#include <linux/slab.h>

#include "config.h"
#include "hall.h"

/* Adaptive Q threshold and amplification factor */
#define HALL_DELTA_THRESHOLD    2000 /* |delta| > 2000 ticks → speed change */
#define HALL_Q_STEADY           25.0f		  /* process noise ~5  ticks std dev     */
#define HALL_Q_TRANSIENT        2500.0f  /* process noise ~50 ticks std dev     */

static void hall_kalman_init_dev(struct hall_device *dev)
{
	kalman_t *kf = &dev->kf;
	kalman_measurement_t *kfm = &dev->kfm;
	struct hall_kf_backing *fb = &dev->kf_backing;
	struct hall_kfm_backing *mb = &dev->kfm_backing;
	matrix_t *mat;

	kalman_filter_initialize(kf, 1, 1,
		&fb->A, &fb->x,
		&fb->B, &fb->u,
		&fb->P, &fb->Q,
		&fb->aux,
		&fb->predictedX,
		&fb->temp_P, &fb->temp_BQ);

	kalman_measurement_initialize(kfm, 1, 1,
		&mb->H, &mb->z,
		&mb->R, &mb->y,
		&mb->S, &mb->K,
		&mb->aux,
		&mb->S_inv,
		&mb->temp_HP, &mb->temp_PHt, &mb->temp_KHP);

	/* A = [1] ? period stays constant between predictions */
	mat = kalman_get_state_transition(kf);
	matrix_set(mat, 0, 0, 1.0f);

	/* B = [1] ? inject process noise via B*Q*B' */
	mat = kalman_get_input_transition(kf);
	matrix_set(mat, 0, 0, 1.0f);

	/* x0 = 66667 ? initial guess: 900 RPM theoretical period */
	mat = kalman_get_state_vector(kf);
	mat->data[0] = 66667.0f;

	/* P0 = 1e6 ? large initial uncertainty, fast convergence */
	mat = kalman_get_system_covariance(kf);
	matrix_set(mat, 0, 0, 1000000.0f);

	/* H = [1] ? direct measurement model */
	mat = kalman_get_measurement_transformation(kfm);
	matrix_set(mat, 0, 0, 1.0f);

	/* R = [400] ? measurement noise (~20 ticks std dev) */
	mat = kalman_get_process_noise(kfm);
	matrix_set(mat, 0, 0, 400.0f);
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
	kalman_t *kf = &dev->kf;
	kalman_measurement_t *kfm = &dev->kfm;
	matrix_t *z = kalman_get_measurement_vector(kfm);
	matrix_t *x = kalman_get_state_vector(kf);
	matrix_t *Q = kalman_get_input_covariance(kf);
	static u32 pulse_cnt;
	s32 delta;
	s32 raw_rpm;
	s32 filter_rpm;

	/*
	 * Adaptive Q: detect speed transient before prediction.
	 * When |delta| > HALL_DELTA_THRESHOLD ticks, amplify process
	 * noise so the gain rises and the filter tracks the change faster.
	 */
	delta = (s32)capture_value - (s32)x->data[0];
	if (abs(delta) > HALL_DELTA_THRESHOLD) {
		matrix_set(Q, 0, 0, HALL_Q_TRANSIENT);
	} else {
		matrix_set(Q, 0, 0, HALL_Q_STEADY);
	}

	/* Time update: predict state and covariance                */
	/*   x = A*x        (period unchanged in prediction)        */
	/*   P = A*P*A' + B*Q*B'  (add process noise, now adaptive) */
	kalman_predict(kf);

	/* Measurement update: inject the raw hall pulse interval   */
	matrix_set(z, 0, 0, (matrix_data_t)capture_value);

	/* Correct: fuse prediction with measurement                */
	kalman_correct(kf, kfm);

	/* Store filtered period_ticks (round to nearest integer)   */
	dev->period_ticks = (u32)(x->data[0] + 0.5f);

#if KINETIS_FAKE_SIM
// 	/* Log comparison: raw vs filtered (every 8 pulses)         */
// 	if ((pulse_cnt++ & 7) == 0) {
// 		raw_rpm = capture_value ? (s32)(60000000 / capture_value) : 0;
// 		filter_rpm = dev->period_ticks ? (s32)(60000000 / dev->period_ticks) : 0;
// 		delta = (s32)capture_value - (s32)dev->period_ticks;
// 		pr_info("kalman: %-4u raw: %u(%dRPM) filter: %u(%dRPM) delta: %+d\n",
// 			pulse_cnt,
// 			capture_value, raw_rpm,
// 			dev->period_ticks, filter_rpm,
// 			delta);
// 	}
#endif
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

	/* Initialize per-instance Kalman filter before first hall pulse */
	hall_kalman_init_dev(dev);

	return dev;
}

#if KINETIS_FAKE_SIM

#include <kinetis/basic-timer.h>

u32 hall_read_rotated_time(struct hall_device *dev)
{
	if (dev->period_ticks == 0)
		return 0;

	return basic_timer_get_us() % dev->period_ticks;
}
#endif /* KINETIS_FAKE_SIM */
