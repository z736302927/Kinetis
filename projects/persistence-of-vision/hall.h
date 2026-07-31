
#ifndef POV_HALL_H
#define POV_HALL_H

#include <linux/types.h>

#include <kinetis/tim-task.h>

#define EXTERN_INLINE_MATRIX    static INLINE
#define EXTERN_INLINE_KALMAN    static INLINE
#include "kalman.h"

struct hall_kf_backing {
	matrix_data_t A, x, B, u, P, Q;
	matrix_data_t aux;
	matrix_data_t predictedX;
	matrix_data_t temp_P, temp_BQ;
};

struct hall_kfm_backing {
	matrix_data_t H, z, R, y, S, K;
	matrix_data_t aux;
	matrix_data_t S_inv;
	matrix_data_t temp_HP, temp_PHt, temp_KHP;
};

struct hall_device {
	u32 angular_resolution;  /*< Columns per revolution (e.g. 720) */
	u32 period_ticks;        /*< Ticks between consecutive hall pulses */

	u32(*read_rotated_time)(struct hall_device *dev);
	/*< Platform-specific time read function (simulation or hardware TIM) */

	struct tim_task fake_pulse_task;
	void *priv;

	/* Per-instance Kalman filter state */
	kalman_t                kf;
	kalman_measurement_t    kfm;
	struct hall_kf_backing  kf_backing;
	struct hall_kfm_backing kfm_backing;
};

u32 hall_get_angle(struct hall_device *dev);
s32 hall_get_rpm(struct hall_device *dev);
void hall_hall_isr(struct hall_device *dev, u32 capture_value);
struct hall_device *hall_alloc_dev(u32 angular_resolution,
	u32(*read_rotated_time)(struct hall_device *dev));
u32 hall_read_rotated_time(struct hall_device *dev);

#endif /* POV_HALL_H */
