
#include <generated/deconfig.h>
#include <linux/kernel.h>

#include "pid.h"
#include "fmu_math.h"


/**
 * function pid_calculate
 * @dt_s:	Scale factor
 * @feedforward:	Integral coefficient
 * @expect:	Differential coefficient
 * @feedback:	Differential first
 * @coef:	Incomplete differential low pass coefficient
 * @result:	Incomplete Differential Normalization (0,1)
 * @inte_d_lim:	Feedforward
 * @inte_lim:	Feedforward
 */
float pid_calculate(float dt_s,
	float feedforward, float expect, float feedback,
	struct pid_coefficient *coef, struct pid_op_result *result,
	float inte_d_lim, float inte_lim)
{
	float differential, hz;

	hz = div32(1.0f, dt_s, 0);

	result->exp_d = (expect - result->exp_old) * hz;

	if (coef->fb_d_mode == 0)
		result->fb_d = (feedback - result->feedback_old) * hz;
	else
		result->fb_d = result->fb_d_ex;

	differential = coef->kd_ex * result->exp_d - coef->kd_fb * result->fb_d;

	result->err = expect - feedback;
	result->err_i = result->err_i +
		coef->ki * clamp(result->err, -inte_d_lim, inte_d_lim) * dt_s;
	result->err_i = clamp(result->err_i, -inte_lim, inte_lim);

	result->out = coef->k_ff * feedforward +
		coef->kp * result->err + differential + result->err_i;

	result->feedback_old = feedback;
	result->exp_old = expect;

	return result->out;
}

//static s16 auto_taking_off_speed;

//#define AUTO_TAKE_OFF_KP 2.0f

//void fmu_auto_take_off_land(u8 dt_ms)
//{
//	static u16 take_off_ok_cnt;

//	one_key_take_off_task(dt_ms);

//	if (flag.unlock_sta) {
//		if (flag.taking_off) {
//			if (flag.auto_take_off_land == AUTO_TAKE_OFF_NULL)
//				flag.auto_take_off_land = AUTO_TAKE_OFF;
//		}
//	} else {
//		auto_taking_off_speed = 0;
//		flag.auto_take_off_land = AUTO_TAKE_OFF_NULL;
//	}

//	if (flag.auto_take_off_land == AUTO_TAKE_OFF) {
//		//设置最大起飞速度
//		s16 max_take_off_vel = clamp(Ano_Parame.set.auto_take_off_speed, 20, 200);

//		take_off_ok_cnt += dt_ms;
//		auto_taking_off_speed = AUTO_TAKE_OFF_KP * (Ano_Parame.set.auto_take_off_height - wcz_hei_fus.out);
//		//计算起飞速度
//		auto_taking_off_speed = clamp(auto_taking_off_speed, 0, max_take_off_vel);

//		//退出起飞流程条件1，满足高度或者流程时间大于5000毫秒。
//		if (take_off_ok_cnt >= 5000 || (Ano_Parame.set.auto_take_off_height - loc_ctrl_2.exp[Z] < 2)) //(auto_ref_height>AUTO_TAKE_OFF_HEIGHT)
//			flag.auto_take_off_land = AUTO_TAKE_OFF_FINISH;

//		//退出起飞流程条件2，2000毫秒后判断用户正在控制油门。
//		if (take_off_ok_cnt > 2000 && ABS(fs.speed_set_h_norm[Z]) > 0.1f) // 一定已经taking_off,如果还在推杆，退出起飞流程
//			flag.auto_take_off_land = AUTO_TAKE_OFF_FINISH;
//	} else {
//		take_off_ok_cnt = 0;

//		if (flag.auto_take_off_land == AUTO_TAKE_OFF_FINISH)
//			auto_taking_off_speed = 0;
//	}

//	//设置自动下降速度
//	if (flag.auto_take_off_land == AUTO_LAND)
//		auto_taking_off_speed = -(s16)clamp(Ano_Parame.set.auto_landing_speed, 20, 200);
//}

//_PID_arg_st alt_arg_2;
//_PID_val_st alt_val_2;

///*高度环PID参数初始化*/
//void Alt_2level_PID_Init()
//{
//	alt_arg_2.kp = Ano_Parame.set.pid_alt_2level[KP];
//	alt_arg_2.ki = Ano_Parame.set.pid_alt_2level[KI];
//	alt_arg_2.kd_ex = 0.00f;
//	alt_arg_2.kd_fb = Ano_Parame.set.pid_alt_2level[KD];
//	alt_arg_2.k_ff = 0.0f;
//}

//void Alt_2level_Ctrl(float dT_s)
//{
//	Auto_Take_Off_Land_Task(1000 * dT_s);

//	fs.alt_ctrl_speed_set = fs.speed_set_h[Z] + auto_taking_off_speed;
//	//
//	loc_ctrl_2.exp[Z] += fs.alt_ctrl_speed_set * dT_s;
//	loc_ctrl_2.exp[Z] = clamp(loc_ctrl_2.exp[Z], loc_ctrl_2.fb[Z] - 200, loc_ctrl_2.fb[Z] + 200);
//	//
//	loc_ctrl_2.fb[Z] = (s32)wcz_hei_fus.out;/////////////

//	if (fs.alt_ctrl_speed_set != 0)
//		flag.ct_alt_hold = 0;
//	else {
//		if (ABS(loc_ctrl_1.exp[Z] - loc_ctrl_1.fb[Z]) < 20)
//			flag.ct_alt_hold = 1;
//	}

//	if (flag.taking_off == 1) {

//		PID_calculate(dT_s,             //周期（单位：秒）
//			0,				//前馈值
//			loc_ctrl_2.exp[Z],				//期望值（设定值）
//			loc_ctrl_2.fb[Z],			//反馈值（）
//			&alt_arg_2, //PID参数结构体
//			&alt_val_2,	//PID数据结构体
//			100,//积分误差限幅
//			0			//integration limit，积分限幅
//		);
//	} else {
//		loc_ctrl_2.exp[Z] = loc_ctrl_2.fb[Z];
//		alt_val_2.out = 0;
//	}

//	alt_val_2.out  = clamp(alt_val_2.out, -150, 150);
//}

//_PID_arg_st alt_arg_1;
//_PID_val_st alt_val_1;

///*高度速度环PID参数初始化*/
//void Alt_1level_PID_Init()
//{
//	alt_arg_1.kp = Ano_Parame.set.pid_alt_1level[KP];
//	alt_arg_1.ki = Ano_Parame.set.pid_alt_1level[KI];
//	alt_arg_1.kd_ex = 0.00f;
//	alt_arg_1.kd_fb = 0;//Ano_Parame.set.pid_alt_1level[KD];
//	alt_arg_1.k_ff = 0.0f;
//}

////static u8 thr_start_ok;
//static float err_i_comp;
//static float w_acc_z_lpf;
//void Alt_1level_Ctrl(float dT_s)
//{
//	u8 out_en;
//	out_en = (flag.taking_off != 0) ? 1 : 0;

//	flag.thr_mode = THR_AUTO;//THR_MANUAL;

//	loc_ctrl_1.exp[Z] = 0.6f * fs.alt_ctrl_speed_set + alt_val_2.out; //速度前馈0.6f，直接给速度

//	w_acc_z_lpf += 0.2f * (imu_data.w_acc[Z] - w_acc_z_lpf); //低通滤波

//	loc_ctrl_1.fb[Z] = wcz_spe_fus.out + Ano_Parame.set.pid_alt_1level[KD] * w_acc_z_lpf; //微分先行，下边PID函数微分系数为0

//	PID_calculate(dT_s,             //周期（单位：秒）
//		0,				//前馈值
//		loc_ctrl_1.exp[Z],				//期望值（设定值）
//		loc_ctrl_1.fb[Z],			//反馈值（）
//		&alt_arg_1, //PID参数结构体
//		&alt_val_1,	//PID数据结构体
//		100,//积分误差限幅
//		(THR_INTE_LIM * 10 - err_i_comp)*out_en			//integration limit，积分限幅
//	);

//	if (flag.taking_off == 1) {
//		LPF_1_(1.0f, dT_s, THR_START * 10, err_i_comp); //err_i_comp = THR_START *10;
//	} else
//		err_i_comp = 0;

//	loc_ctrl_1.out[Z] = out_en * (alt_val_1.out + err_i_comp);

//	loc_ctrl_1.out[Z] = clamp(loc_ctrl_1.out[Z], 0, MAX_THR_SET * 10);

//	mc.ct_val_thr = loc_ctrl_1.out[Z];
//}
