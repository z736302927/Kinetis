
#include <generated/deconfig.h>
#include <linux/kernel.h>

#include <kinetis/fmu.h>

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

	result->expected_d = (expect - result->expected_old) * hz;

	if (coef->feedback_d_mode == 0)
		result->feedback_d = (feedback - result->feedback_old) * hz;
	else
		result->feedback_d = result->feedback_d_ex;

	differential = coef->kd_ex * result->expected_d - coef->kd_fb * result->feedback_d;

	result->err = expect - feedback;
	result->err_i = result->err_i +
		coef->ki * clamp(result->err, -inte_d_lim, inte_d_lim) * dt_s;
	result->err_i = clamp(result->err_i, -inte_lim, inte_lim);

	result->out = coef->k_ff * feedforward +
		coef->kp * result->err + differential + result->err_i;

	result->feedback_old = feedback;
	result->expected_old = expect;

	return result->out;
}

void fmu_ctrl_pid_init(struct fmu_pid_ctrl *ctrl,
	struct fmu_parameter *para)
{
	ctrl->angel.rol.coef.kp = para->attitude.rol.p;
	ctrl->angel.rol.coef.ki = para->attitude.rol.i;
	ctrl->angel.rol.coef.kd_ex = para->attitude.rol.d;
	ctrl->angel.rol.coef.kd_fb = para->attitude.rol.d;
	ctrl->angel.rol.coef.k_ff = 0.0f;

	ctrl->angel.pit.coef.kp = para->attitude.pit.p;
	ctrl->angel.pit.coef.ki = para->attitude.pit.i;
	ctrl->angel.pit.coef.kd_ex = para->attitude.pit.d;
	ctrl->angel.pit.coef.kd_fb = para->attitude.pit.d;
	ctrl->angel.pit.coef.k_ff = 0.0f;

	ctrl->angel.yaw.coef.kp = para->attitude.yaw.p;
	ctrl->angel.yaw.coef.ki = para->attitude.yaw.i;
	ctrl->angel.yaw.coef.kd_ex = para->attitude.yaw.d;
	ctrl->angel.yaw.coef.kd_fb = para->attitude.yaw.d;
	ctrl->angel.yaw.coef.k_ff = 0.0f;

	/* Attitude angular velocity control parameters.
	 * P adjust the angular velocity response speed,
	 * and the higher the better without oscillation
	 * Vibration debugging can reduce P and increase D.
	 * If increasing D cannot suppress the oscillation,
	 * it is necessary to reduce P and D at the same time. */
	ctrl->angel_df.rol.coef.kp = para->attitude_df.rol.p;
	ctrl->angel_df.rol.coef.ki = para->attitude_df.rol.i;
	ctrl->angel_df.rol.coef.kd_ex = 0;//0.000f;
	ctrl->angel_df.rol.coef.kd_fb = para->attitude_df.rol.d;
	ctrl->angel_df.rol.coef.k_ff = 0.0f;

	ctrl->angel_df.pit.coef.kp = para->attitude_df.pit.p;
	ctrl->angel_df.pit.coef.ki = para->attitude_df.pit.i;
	ctrl->angel_df.pit.coef.kd_ex = 0;//0.000f;
	ctrl->angel_df.pit.coef.kd_fb = para->attitude_df.pit.d;
	ctrl->angel_df.pit.coef.k_ff = 0.0f;

	ctrl->angel_df.yaw.coef.kp = para->attitude_df.yaw.p;
	ctrl->angel_df.yaw.coef.ki = para->attitude_df.yaw.i;
	ctrl->angel_df.yaw.coef.kd_ex = 0;//0.00f;
	ctrl->angel_df.yaw.coef.kd_fb = para->attitude_df.yaw.d;
	ctrl->angel_df.yaw.coef.k_ff = 0.00f;

#if (MOTOR_ESC_TYPE == 2)
#define DIFF_GAIN 0.3f
//	ctrl->angel_df.rol.coef.kd_ex = ctrl->angel_df.rol.coef.kd_ex * DIFF_GAIN;
//	ctrl->angel_df.pit.coef.kd_ex = ctrl->angel_df.pit.coef.kd_ex * DIFF_GAIN;
	ctrl->angel_df.rol.coef.kd_fb = ctrl->angel_df.rol.coef.kd_fb * DIFF_GAIN;
	ctrl->angel_df.pit.coef.kd_fb = ctrl->angel_df.pit.coef.kd_fb * DIFF_GAIN;
#elif (MOTOR_ESC_TYPE == 1)
#define DIFF_GAIN 1.0f
//	ctrl->angel_df.rol.coef.kd_ex = ctrl->angel_df.rol.coef.kd_ex * DIFF_GAIN;
//	ctrl->angel_df.pit.coef.kd_ex = ctrl->angel_df.pit.coef.kd_ex * DIFF_GAIN;
	ctrl->angel_df.rol.coef.kd_fb = ctrl->angel_df.rol.coef.kd_fb * DIFF_GAIN;
	ctrl->angel_df.pit.coef.kd_fb = ctrl->angel_df.pit.coef.kd_fb * DIFF_GAIN;
#endif

	ctrl->height_df.coef.kp = para->height_df.p;
	ctrl->height_df.coef.ki = para->height_df.i;
	ctrl->height_df.coef.kd_ex = 0.00f;
	ctrl->height_df.coef.kd_fb = 0;//para->height_df.d;
	ctrl->height_df.coef.k_ff = 0.0f;

	ctrl->height.coef.kp = para->height.p;
	ctrl->height.coef.ki = para->height.i;
	ctrl->height.coef.kd_ex = 0.00f;
	ctrl->height.coef.kd_fb = para->height.d;
	ctrl->height.coef.k_ff = 0.0f;

	switch (ctrl->position_sensor) {
	case FMU_PS_OF:
		ctrl->position_df.x.coef.kp = para->position_df.p;
		ctrl->position_df.x.coef.ki = 0.0f;
		ctrl->position_df.x.coef.kd_ex = 0.00f;
		ctrl->position_df.x.coef.kd_fb = para->position_df.d;
		ctrl->position_df.x.coef.k_ff = 0.02f;

		ctrl->position_df.y.coef.kp = para->position_df.p;
		ctrl->position_df.y.coef.ki = 0.0f;
		ctrl->position_df.y.coef.kd_ex = 0.00f;
		ctrl->position_df.y.coef.kd_fb = para->position_df.d;
		ctrl->position_df.y.coef.k_ff = 0.02f;

		ctrl->position_df_fix.x.coef.kp = 0.0f;
		ctrl->position_df_fix.x.coef.ki = para->position_df.i;
		ctrl->position_df_fix.x.coef.kd_ex = 0.00f;
		ctrl->position_df_fix.x.coef.kd_fb = 0.00f;
		ctrl->position_df_fix.x.coef.k_ff = 0.0f;

		ctrl->position_df_fix.y.coef.kp = 0.0f;
		ctrl->position_df_fix.y.coef.ki = para->position_df.i;
		ctrl->position_df_fix.y.coef.kd_ex = 0.00f;
		ctrl->position_df_fix.y.coef.kd_fb = 0.00f;
		ctrl->position_df_fix.y.coef.k_ff = 0.0f;
		break;
	case FMU_PS_GPS:
		ctrl->position_df.x.coef.kp = para->gps_df.p;
		ctrl->position_df.x.coef.ki = 0;
		ctrl->position_df.x.coef.kd_ex = 0.00f;
		ctrl->position_df.x.coef.kd_fb = para->gps_df.d;
		ctrl->position_df.x.coef.k_ff = 0.02f;

		ctrl->position_df.y.coef.kp = para->gps_df.p;
		ctrl->position_df.y.coef.ki = 0;
		ctrl->position_df.y.coef.kd_ex = 0.00f;
		ctrl->position_df.y.coef.kd_fb = para->gps_df.d;
		ctrl->position_df.y.coef.k_ff = 0.02f;

		ctrl->position_df_fix.x.coef.kp = 0.0f;
		ctrl->position_df_fix.x.coef.ki = para->gps_df.i;
		ctrl->position_df_fix.x.coef.kd_ex = 0.00f;
		ctrl->position_df_fix.x.coef.kd_fb = 0.00f;
		ctrl->position_df_fix.x.coef.k_ff = 0.0f;

		ctrl->position_df_fix.y.coef.kp = 0.0f;
		ctrl->position_df_fix.y.coef.ki = para->gps_df.i;
		ctrl->position_df_fix.y.coef.kd_ex = 0.00f;
		ctrl->position_df_fix.y.coef.kd_fb = 0.00f;
		ctrl->position_df_fix.y.coef.k_ff = 0.0f;
		break;
	case FMU_PS_UWB:
	case FMU_PS_OF_UWB:
		ctrl->position_df.x.coef.kp = para->position_df.p;
		ctrl->position_df.x.coef.ki = 0.0f;
		ctrl->position_df.x.coef.kd_ex = 0.00f;
		ctrl->position_df.x.coef.kd_fb = para->position_df.d;
		ctrl->position_df.x.coef.k_ff = 0.02f;

		ctrl->position_df.y.coef.kp = para->position_df.p;
		ctrl->position_df.y.coef.ki = 0.0f;
		ctrl->position_df.y.coef.kd_ex = 0.00f;
		ctrl->position_df.y.coef.kd_fb = para->position_df.d;
		ctrl->position_df.y.coef.k_ff = 0.02f;

		ctrl->position_df_fix.x.coef.kp = 0.0f;
		ctrl->position_df_fix.x.coef.ki = para->position_df.i;
		ctrl->position_df_fix.x.coef.kd_ex = 0.00f;
		ctrl->position_df_fix.x.coef.kd_fb = 0.00f;
		ctrl->position_df_fix.x.coef.k_ff = 0.0f;

		ctrl->position_df_fix.y.coef.kp = 0.0f;
		ctrl->position_df_fix.y.coef.ki = para->position_df.i;
		ctrl->position_df_fix.y.coef.kd_ex = 0.00f;
		ctrl->position_df_fix.y.coef.kd_fb = 0.00f;
		ctrl->position_df_fix.y.coef.k_ff = 0.0f;
		break;
	}
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
//		if (take_off_ok_cnt > 2000 && imu_abs(fs.speed_set_h_norm[Z]) > 0.1f) // 一定已经taking_off,如果还在推杆，退出起飞流程
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

//void Alt_2level_Ctrl(float dt)
//{
//	Auto_Take_Off_Land_Task(1000 * dt);

//	fs.alt_ctrl_speed_set = fs.speed_set_h[Z] + auto_taking_off_speed;
//	//
//	loc_ctrl_2.exp[Z] += fs.alt_ctrl_speed_set * dt;
//	loc_ctrl_2.exp[Z] = clamp(loc_ctrl_2.exp[Z], loc_ctrl_2.fb[Z] - 200, loc_ctrl_2.fb[Z] + 200);
//	//
//	loc_ctrl_2.fb[Z] = (s32)wcz_hei_fus.out;/////////////

//	if (fs.alt_ctrl_speed_set != 0)
//		flag.ct_alt_hold = 0;
//	else {
//		if (imu_abs(loc_ctrl_1.exp[Z] - loc_ctrl_1.fb[Z]) < 20)
//			flag.ct_alt_hold = 1;
//	}

//	if (flag.taking_off == 1) {

//		PID_calculate(dt,             //周期（单位：秒）
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
//void Alt_1level_Ctrl(float dt)
//{
//	u8 out_en;
//	out_en = (flag.taking_off != 0) ? 1 : 0;

//	flag.thr_mode = THR_AUTO;//THR_MANUAL;

//	loc_ctrl_1.exp[Z] = 0.6f * fs.alt_ctrl_speed_set + alt_val_2.out; //速度前馈0.6f，直接给速度

//	w_acc_z_lpf += 0.2f * (imu_data.w_acc[Z] - w_acc_z_lpf); //低通滤波

//	loc_ctrl_1.fb[Z] = wcz_spe_fus.out + Ano_Parame.set.pid_alt_1level[KD] * w_acc_z_lpf; //微分先行，下边PID函数微分系数为0

//	PID_calculate(dt,             //周期（单位：秒）
//		0,				//前馈值
//		loc_ctrl_1.exp[Z],				//期望值（设定值）
//		loc_ctrl_1.fb[Z],			//反馈值（）
//		&alt_arg_1, //PID参数结构体
//		&alt_val_1,	//PID数据结构体
//		100,//积分误差限幅
//		(THR_INTE_LIM * 10 - err_i_comp)*out_en			//integration limit，积分限幅
//	);

//	if (flag.taking_off == 1) {
//		LPF_1_(1.0f, dt, THR_START * 10, err_i_comp); //err_i_comp = THR_START *10;
//	} else
//		err_i_comp = 0;

//	loc_ctrl_1.out[Z] = out_en * (alt_val_1.out + err_i_comp);

//	loc_ctrl_1.out[Z] = clamp(loc_ctrl_1.out[Z], 0, MAX_THR_SET * 10);

//	mc.ct_val_thr = loc_ctrl_1.out[Z];
//}

struct pid_ctrl_status {
	float expected_rol_adj;
	float expected_pit_adj;
	float yaw_error;

	float expected_rol;
	float expected_pit;
	float expected_yaw;
	float feedback_rol;
	float feedback_pit;
	float feedback_yaw;
};

#define POS_V_DAMPING 0.02f

/*角度环控制*/
void pid_angle_control(struct fmu_pid_ctrl *ctrl, struct pid_ctrl_status *status,
	u8 flight_mode, u8 speed_mode, float dt, s16 *CH_N)
{
	float expected_rol, expected_pit;
	s32 max_yaw_speed, yaw_angle_df;

	/*积分微调*/
//    expected_rol = - loc_ctrl_1.out[Y];
//    expected_pit = - loc_ctrl_1.out[X];

//	if (flight_mode == ATT_STAB) {
//		if (imu_abs(expected_rol + status->expected_rol_adj) < 5) {
//			status->expected_rol_adj += 0.2f * expected_rol * dt;
//			status->expected_rol_adj = clamp(status->expected_rol_adj, -1.0f, 1.0f);
//		}

//		if (imu_abs(expected_pit + status->expected_pit_adj) < 5) {
//			status->expected_pit_adj += 0.2f * expected_pit * dt;
//			status->expected_pit_adj = clamp(status->expected_pit_adj, -1.0f, 1.0f);
//		}
//	} else {
//		status->expected_rol_adj = 0;
//		status->expected_pit_adj = 0;
//	}

	/*正负参考ANO坐标参考方向*/
	status->expected_rol = expected_rol + status->expected_rol_adj;
	status->expected_pit = expected_pit + status->expected_pit_adj;

	/*期望角度限幅*/
//	status->expected_rol = clamp(status->expected_rol, -MAX_ANGLE, MAX_ANGLE);
//	status->expected_pit = clamp(status->expected_pit, -MAX_ANGLE, MAX_ANGLE);

//	if (speed_mode == 3)
//		max_yaw_speed = MAX_SPEED_YAW;
//	else if (speed_mode == 2)
//		max_yaw_speed = 220;
//	else
//		max_yaw_speed = 200;

//	fc_stv.yaw_pal_limit = max_yaw_speed;
//	/*摇杆量转换为YAW期望角速度 + 程控期望角速度*/
//	yaw_angle_df = (s32)(0.0023f * my_deadzone(CH_N[CH_YAW], 0, 65) * max_yaw_speed) +
//		(-program_ctrl.yaw_pal_dps) + pc_user.pal_dps_set;

	/*最大YAW角速度限幅*/
	yaw_angle_df = clamp(yaw_angle_df, -max_yaw_speed, max_yaw_speed);

//	/*没有起飞，复位*/
//	if (flag.taking_off == 0 || (flag.locking)) {
//		status->expected_rol = status->expected_pit = yaw_angle_df = 0;
//		status->expected_yaw = status->feedback_yaw;
//	}

	/*限制误差增大*/
	if (status->yaw_error > 90) {
		if (yaw_angle_df > 0)
			yaw_angle_df = 0;
	} else if (status->yaw_error < -90) {
		if (yaw_angle_df < 0)
			yaw_angle_df = 0;
	}

//	//增量限幅
//	att_1l_ct.set_yaw_speed += clamp(yaw_angle_df - att_1l_ct.set_yaw_speed, -30, 30);
//	/*设置期望YAW角度*/
//	status->expected_yaw += att_1l_ct.set_yaw_speed * dt;

	/*限制为+-180度*/
	if (status->expected_yaw < -180)
		status->expected_yaw += 360;
	else if (status->expected_yaw > 180)
		status->expected_yaw -= 360;

	/*计算YAW角度误差*/
	status->yaw_error = status->expected_yaw - status->feedback_yaw;

	/*限制为+-180度*/
	if (status->yaw_error < -180)
		status->yaw_error += 360;
	else if (status->yaw_error > 180)
		status->yaw_error -= 360;

	/*赋值反馈角度值*/
//	status->feedback_yaw = imu_data.yaw;
//	status->feedback_rol = imu_data.rol;
//	status->feedback_pit = imu_data.pit;

//	pid_calculate(dt, 0,
//		status->expected_rol, status->feedback_rol,
//		&ctrl->angel.rol.coef, &ctrl->angel.rol.op,
//		5, 5 * flag.taking_off);

//	pid_calculate(dt,
//		0,
//		status->expected_pit, status->feedback_pit,
//		&ctrl->angel.pit.coef, &ctrl->angel.pit.op,
//		5, 5 * flag.taking_off);

//	pid_calculate(dt,
//		0,
//		status->yaw_error,
//		0,
//		&ctrl->angel.yaw.coef, &ctrl->angel.yaw.op,
//		5, 5 * flag.taking_off);
}