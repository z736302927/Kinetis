#ifndef POV_PID_H
#define POV_PID_H

#include <linux/types.h>

/**
 * @brief Q8.8 fixed-point scaling for PID gains
 *
 * All gains (kp, ki, kd) are stored as Q8.8 fixed-point values.
 * Actual gain = stored_value / 256.
 * Example: kp = 512 means Kp = 2.0
 */
#define PID_SCALE_BITS    8
#define PID_SCALE         (1 << PID_SCALE_BITS)

/**
 * @brief PID controller state
 *
 * Implements positional PID with conditional integration anti-windup.
 * Integral accumulates only when output is not saturated, or when
 * the saturation direction opposes the error direction.
 */
struct pid_controller {
	s32 kp;              /* Proportional gain (Q8.8) */
	s32 ki;              /* Integral gain (Q8.8) */
	s32 kd;              /* Derivative gain (Q8.8) */

	s32 integral;        /* Integral accumulator */
	s32 prev_err;        /* Previous error for derivative term */
	s8  saturated;       /* 1=positive sat, -1=negative sat, 0=normal */

	s32 out_min;         /* Output lower limit */
	s32 out_max;         /* Output upper limit */
	s32 int_min;         /* Integral lower limit */
	s32 int_max;         /* Integral upper limit */
};

void pid_init(struct pid_controller *ctrl, s32 kp, s32 ki, s32 kd);
void pid_reset(struct pid_controller *ctrl);
s32  pid_update(struct pid_controller *ctrl, s32 err);

#endif
