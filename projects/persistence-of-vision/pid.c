#include <linux/types.h>
#include <linux/printk.h>

#include "config.h"
#include "pid.h"

void pid_init(struct pid_controller *ctrl, s32 kp, s32 ki, s32 kd)
{
	ctrl->kp = kp;
	ctrl->ki = ki;
	ctrl->kd = kd;
	ctrl->integral = 0;
	ctrl->prev_err = 0;
	ctrl->saturated = 0;
	ctrl->int_min = SLAVE_PID_INTEGRAL_MIN;
	ctrl->int_max = SLAVE_PID_INTEGRAL_MAX;
	ctrl->out_min = SLAVE_PID_OUTPUT_MIN;
	ctrl->out_max = SLAVE_PID_OUTPUT_MAX;
}

void pid_reset(struct pid_controller *ctrl)
{
	ctrl->integral = 0;
	ctrl->prev_err = 0;
	ctrl->saturated = 0;
}

s32 pid_update(struct pid_controller *ctrl, s32 err)
{
	s32 p = (ctrl->kp * err) >> PID_SCALE_BITS;
	s32 d = (ctrl->kd * (err - ctrl->prev_err)) >> PID_SCALE_BITS;
	s32 output;

	ctrl->prev_err = err;

	if (!ctrl->saturated || (ctrl->saturated > 0 && err < 0) || (ctrl->saturated < 0 && err > 0)) {
		ctrl->integral += (ctrl->ki * err) >> PID_SCALE_BITS;
		if (ctrl->integral > ctrl->int_max) {
			ctrl->integral = ctrl->int_max;
		}
		if (ctrl->integral < ctrl->int_min) {
			ctrl->integral = ctrl->int_min;
		}
	}

	output = p + ctrl->integral + d;

	if (output > ctrl->out_max) {
		output = ctrl->out_max;
		ctrl->saturated = 1;
	} else if (output < ctrl->out_min) {
		output = ctrl->out_min;
		ctrl->saturated = -1;
	} else {
		ctrl->saturated = 0;
	}

// 	pr_debug("pid: err=%d p=%d i=%d d=%d out=%d sat=%d\n",
// 		 err, p, ctrl->integral, d, output, ctrl->saturated);

	return output;
}
