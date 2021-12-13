#ifndef _FMU_PID_H
#define _FMU_PID_H

#include <linux/types.h>

/**
 * struct pid_coefficient -  structure
 * @kp:	Scale factor
 * @ki:	Integral coefficient
 * @kd_ex:	Differential coefficient
 * @kd_fb:	Differential first
 * @inc_hz:	Incomplete differential low pass coefficient
 * @k_inc_d_norm:	Incomplete Differential Normalization (0,1)
 * @k_ff:	Feedforward
 */
struct pid_coefficient {
	u8 fb_d_mode;
	float kp;
	float ki;
	float kd_ex;
	float kd_fb;
	float inc_hz;
	float k_inc_d_norm;
	float k_ff;
};

struct pid_op_result {
	float err;
	float exp_old;
	float feedback_old;

	float fb_d;
	float fb_d_ex;
	float exp_d;
	float err_d_lpf;
	float err_i;
	float ff;
	float pre_d;

	float out;
};




#endif /* _FMU_PID_H */