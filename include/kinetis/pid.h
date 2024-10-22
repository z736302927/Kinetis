#ifndef _FMU_PID_H
#define _FMU_PID_H

#include <linux/types.h>

#include "fmu.h"

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
	u8 feedback_d_mode;
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
	float expected_old;
	float feedback_old;

	float feedback_d;
	float feedback_d_ex;
	float expected_d;
	float err_d_lpf;
	float err_i;
	float ff;
	float pre_d;

	float out;
};

struct pid_coef_op {
	struct pid_coefficient coef;
	struct pid_op_result op;
};

struct fmu_pid_ctrl {
	struct {
		struct pid_coef_op rol;
		struct pid_coef_op pit;
		struct pid_coef_op yaw;
	} angel;

	struct {
		struct pid_coef_op rol;
		struct pid_coef_op pit;
		struct pid_coef_op yaw;
	} angel_df;
	
	struct pid_coef_op height;
	struct pid_coef_op height_df;

	struct {
		struct pid_coef_op x;
		struct pid_coef_op y;
	} position_df;

	struct {
		struct pid_coef_op x;
		struct pid_coef_op y;
	} position_df_fix;
	
	u8 position_sensor;
#define FMU_PS_OF	1
#define FMU_PS_GPS	2
#define FMU_PS_UWB	3
#define FMU_PS_OF_UWB	4
};

void fmu_ctrl_pid_init(struct fmu_pid_ctrl *ctrl,
	struct fmu_parameter *para);

#endif /* _FMU_PID_H */