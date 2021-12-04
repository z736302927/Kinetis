#ifndef __ANO_PID_H
#define __ANO_PID_H

#include "Ano_FcData.h"
#include <linux/types.h>

struct pid_coefficient {
	u8 fb_d_mode;
	float kp;			 //比例系数
	float ki;			 //积分系数
	float kd_ex;		 	 //微分系数
	float kd_fb; //previous_d 微分先行
	float inc_hz;  //不完全微分低通系数
	float k_inc_d_norm; //Incomplete 不完全微分 归一（0,1）
	float k_ff;		 //前馈

};

struct struct pid_op_result {
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

float pid_calculate(float T,             //周期
	float in_ff,				//前馈
	float expect,				//期望值（设定值）
	float feedback,			//反馈值
	struct pid_coefficient *pid_arg, //PID参数结构体
	struct pid_op_result *pid_val,	//PID数据结构体
	float inte_d_lim,
	float inte_lim			//integration limit，积分限幅
);			//输出



#endif

