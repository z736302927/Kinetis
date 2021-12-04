#include "Ano_AttCtrl.h"
#include "Ano_Imu.h"
#include "Drv_icm20602.h"
#include "Ano_MagProcess.h"
#include "Drv_spl06.h"
#include "Ano_MotionCal.h"
#include "Ano_FlightCtrl.h"
#include "Ano_LocCtrl.h"
#include "Ano_MotorCtrl.h"
#include "Ano_FlyCtrl.h"
#include "Ano_Sensor_Basic.h"
#include "Ano_ProgramCtrl_User.h"

/*角度环PID参数初始化*/
void angle_pid_init()
{
    angle_coe[ROL].kp = Ano_Parame.set.pid_att_2level[ROL][KP];
    angle_coe[ROL].ki = Ano_Parame.set.pid_att_2level[ROL][KI];
    angle_coe[ROL].kd_ex = Ano_Parame.set.pid_att_2level[ROL][KD];
    angle_coe[ROL].kd_fb = Ano_Parame.set.pid_att_2level[ROL][KD];
    angle_coe[ROL].k_ff = 0.0f;

    angle_coe[PIT].kp = Ano_Parame.set.pid_att_2level[PIT][KP];
    angle_coe[PIT].ki = Ano_Parame.set.pid_att_2level[PIT][KI];
    angle_coe[PIT].kd_ex = Ano_Parame.set.pid_att_2level[PIT][KD];
    angle_coe[PIT].kd_fb = Ano_Parame.set.pid_att_2level[PIT][KD];
    angle_coe[PIT].k_ff = 0.0f;

    angle_coe[YAW].kp = Ano_Parame.set.pid_att_2level[YAW][KP];
    angle_coe[YAW].ki = Ano_Parame.set.pid_att_2level[YAW][KI];
    angle_coe[YAW].kd_ex = Ano_Parame.set.pid_att_2level[YAW][KD];
    angle_coe[YAW].kd_fb = Ano_Parame.set.pid_att_2level[YAW][KD];
    angle_coe[YAW].k_ff = 0.0f;
}

/*
姿态角速率部分控制参数

arg_1_kp：调整角速度响应速度，不震荡的前提下，尽量越高越好。

震荡试，可以降低arg_1_kp，增大arg_1_kd。

若增大arg_1_kd已经不能抑制震荡，需要将kp和kd同时减小。
*/
#define CTRL_1_KI_START 0.f

/*角速度环PID参数初始化*/
void angle_df_pid_init()
{
    angle_df_coe[ROL].kp = Ano_Parame.set.pid_att_1level[ROL][KP];
    angle_df_coe[ROL].ki = Ano_Parame.set.pid_att_1level[ROL][KI];
    angle_df_coe[ROL].kd_ex = 0;//0.000f   ;
    angle_df_coe[ROL].kd_fb = Ano_Parame.set.pid_att_1level[ROL][KD];
    angle_df_coe[ROL].k_ff = 0.0f;

    angle_df_coe[PIT].kp = Ano_Parame.set.pid_att_1level[PIT][KP];
    angle_df_coe[PIT].ki = Ano_Parame.set.pid_att_1level[PIT][KI];
    angle_df_coe[PIT].kd_ex = 0;//0.000f   ;
    angle_df_coe[PIT].kd_fb = Ano_Parame.set.pid_att_1level[PIT][KD];
    angle_df_coe[PIT].k_ff = 0.0f;

    angle_df_coe[YAW].kp = Ano_Parame.set.pid_att_1level[YAW][KP];
    angle_df_coe[YAW].ki = Ano_Parame.set.pid_att_1level[YAW][KI];
    angle_df_coe[YAW].kd_ex = 0;//0.00f   ;
    angle_df_coe[YAW].kd_fb = Ano_Parame.set.pid_att_1level[YAW][KD];
    angle_df_coe[YAW].k_ff = 0.00f;

#if (MOTOR_ESC_TYPE == 2)
#define DIFF_GAIN 0.3f
//	angle_df_coe[ROL].kd_ex = angle_df_coe[ROL].kd_ex *DIFF_GAIN;
//	angle_df_coe[PIT].kd_ex = angle_df_coe[PIT].kd_ex *DIFF_GAIN;
    angle_df_coe[ROL].kd_fb = angle_df_coe[ROL].kd_fb * DIFF_GAIN;
    angle_df_coe[PIT].kd_fb = angle_df_coe[PIT].kd_fb * DIFF_GAIN;
#elif (MOTOR_ESC_TYPE == 1)
#define DIFF_GAIN 1.0f
//	angle_df_coe[ROL].kd_ex = angle_df_coe[ROL].kd_ex *DIFF_GAIN;
//	angle_df_coe[PIT].kd_ex = angle_df_coe[PIT].kd_ex *DIFF_GAIN;
    angle_df_coe[ROL].kd_fb = angle_df_coe[ROL].kd_fb * DIFF_GAIN;
    angle_df_coe[PIT].kd_fb = angle_df_coe[PIT].kd_fb * DIFF_GAIN;
#endif
}

void Set_Att_1level_Ki(u8 mode)
{
    if (mode == 0)
        angle_df_coe[ROL].ki = angle_df_coe[PIT].ki = 0;
    else if (mode == 1) {
        angle_df_coe[ROL].ki = Ano_Parame.set.pid_att_1level[ROL][KI];
        angle_df_coe[PIT].ki = Ano_Parame.set.pid_att_1level[PIT][KI];
    } else
        angle_df_coe[ROL].ki = angle_df_coe[PIT].ki = CTRL_1_KI_START;
}

void Set_Att_2level_Ki(u8 mode)
{
    if (mode == 0)
        angle_coe[ROL].ki = angle_coe[PIT].ki = 0;
    else {
        angle_coe[ROL].ki = Ano_Parame.set.pid_att_2level[ROL][KI];
        angle_coe[PIT].ki = Ano_Parame.set.pid_att_2level[PIT][KI];
    }
}

/*角度环控制*/
void Att_2level_Ctrl(float dT_s, s16 *CH_N)
{
    /*积分微调*/
    exp_rol_tmp = - loc_ctrl_1.out[Y];
    exp_pit_tmp = - loc_ctrl_1.out[X];

    if (flag.flight_mode == ATT_STAB) {
        if (ABS(exp_rol_tmp + att_2l_ct.exp_rol_adj) < 5) {
            att_2l_ct.exp_rol_adj += 0.2f * exp_rol_tmp * dT_s;
            att_2l_ct.exp_rol_adj = clamp(att_2l_ct.exp_rol_adj, -1, 1);
        }

        if (ABS(exp_pit_tmp + att_2l_ct.exp_pit_adj) < 5) {
            att_2l_ct.exp_pit_adj += 0.2f * exp_pit_tmp * dT_s;
            att_2l_ct.exp_pit_adj = clamp(att_2l_ct.exp_pit_adj, -1, 1);
        }
    } else {
        att_2l_ct.exp_rol_adj =
            att_2l_ct.exp_pit_adj = 0;
    }

    /*正负参考ANO坐标参考方向*/
    att_2l_ct.exp_rol = exp_rol_tmp + att_2l_ct.exp_rol_adj;// + POS_V_DAMPING *imu_data.h_acc[Y];
    att_2l_ct.exp_pit = exp_pit_tmp + att_2l_ct.exp_pit_adj;// + POS_V_DAMPING *imu_data.h_acc[X];

    /*期望角度限幅*/
    att_2l_ct.exp_rol = clamp(att_2l_ct.exp_rol, -MAX_ANGLE, MAX_ANGLE);
    att_2l_ct.exp_pit = clamp(att_2l_ct.exp_pit, -MAX_ANGLE, MAX_ANGLE);

    if (flag.speed_mode == 3)
        max_yaw_speed = MAX_SPEED_YAW;
    else if (flag.speed_mode == 2)
        max_yaw_speed = 220;
    else
        max_yaw_speed = 200;

    fc_stv.yaw_pal_limit = max_yaw_speed;
    /*摇杆量转换为YAW期望角速度 + 程控期望角速度*/
    set_yaw_av_tmp = (s32)(0.0023f * my_deadzone(CH_N[CH_YAW], 0, 65) * max_yaw_speed) + (-program_ctrl.yaw_pal_dps) + pc_user.pal_dps_set;

    /*最大YAW角速度限幅*/
    set_yaw_av_tmp = clamp(set_yaw_av_tmp, -max_yaw_speed, max_yaw_speed);

    /*没有起飞，复位*/
    if (flag.taking_off == 0 || (flag.locking)) {
        att_2l_ct.exp_rol = att_2l_ct.exp_pit = set_yaw_av_tmp = 0;
        att_2l_ct.exp_yaw = att_2l_ct.fb_yaw;
    }

    /*限制误差增大*/
    if (att_2l_ct.yaw_err > 90) {
        if (set_yaw_av_tmp > 0)
            set_yaw_av_tmp = 0;
    } else if (att_2l_ct.yaw_err < -90) {
        if (set_yaw_av_tmp < 0)
            set_yaw_av_tmp = 0;
    }

    //增量限幅
    att_1l_ct.set_yaw_speed += clamp((set_yaw_av_tmp - att_1l_ct.set_yaw_speed), -30, 30);
    /*设置期望YAW角度*/
    att_2l_ct.exp_yaw += att_1l_ct.set_yaw_speed * dT_s;

    /*限制为+-180度*/
    if (att_2l_ct.exp_yaw < -180)
        att_2l_ct.exp_yaw += 360;
    else if (att_2l_ct.exp_yaw > 180)
        att_2l_ct.exp_yaw -= 360;

    /*计算YAW角度误差*/
    att_2l_ct.yaw_err = (att_2l_ct.exp_yaw - att_2l_ct.fb_yaw);

    /*限制为+-180度*/
    if (att_2l_ct.yaw_err < -180)
        att_2l_ct.yaw_err += 360;
    else if (att_2l_ct.yaw_err > 180)
        att_2l_ct.yaw_err -= 360;

    /*赋值反馈角度值*/
    att_2l_ct.fb_yaw = imu_data.yaw ;

    att_2l_ct.fb_rol = (imu_data.rol) ;
    att_2l_ct.fb_pit = (imu_data.pit) ;

    pid_calculate(dT_s,             //周期（单位：秒）
        0,				//前馈值
        att_2l_ct.exp_rol,				//期望值（设定值）
        att_2l_ct.fb_rol,			//反馈值（）
        &angle_coe[ROL], //PID参数结构体
        &angle_res[ROL],	//PID数据结构体
        5,//积分误差限幅
        5 * flag.taking_off			//integration limit，积分限幅
    );

    pid_calculate(dT_s,             //周期（单位：秒）
        0,				//前馈值
        att_2l_ct.exp_pit,				//期望值（设定值）
        att_2l_ct.fb_pit,			//反馈值（）
        &angle_coe[PIT], //PID参数结构体
        &angle_res[PIT],	//PID数据结构体
        5,//积分误差限幅
        5 * flag.taking_off		//integration limit，积分限幅
    );

    pid_calculate(dT_s,             //周期（单位：秒）
        0,				//前馈值
        att_2l_ct.yaw_err,				//期望值（设定值）
        0,			//反馈值（）
        &angle_coe[YAW], //PID参数结构体
        &angle_res[YAW],	//PID数据结构体
        5,//积分误差限幅
        5 * flag.taking_off			//integration limit，积分限幅
    );
}

/*角速度环控制*/
void Att_1level_Ctrl(float dT_s)
{
    ////////////////改变控制参数任务（最小控制周期内）////////////////////////
    ctrl_parameter_change_task();

    /*目标角速度赋值*/
    for (u8 i = 0; i < 3; i++) {
        att_1l_ct.exp_angular_velocity[i] = angle_res[i].out;
    }

    /*目标角速度限幅*/
    att_1l_ct.exp_angular_velocity[ROL] = clamp(att_1l_ct.exp_angular_velocity[ROL], -MAX_ROLLING_SPEED, MAX_ROLLING_SPEED);
    att_1l_ct.exp_angular_velocity[PIT] = clamp(att_1l_ct.exp_angular_velocity[PIT], -MAX_ROLLING_SPEED, MAX_ROLLING_SPEED);

    /*反馈角速度赋值*/
    att_1l_ct.fb_angular_velocity[ROL] += 0.25f * ((sensor.Gyro_deg[X]) - att_1l_ct.fb_angular_velocity[ROL]);
    att_1l_ct.fb_angular_velocity[PIT] += 0.25f * ((-sensor.Gyro_deg[Y]) - att_1l_ct.fb_angular_velocity[PIT]);
    att_1l_ct.fb_angular_velocity[YAW] += 0.25f * ((-sensor.Gyro_deg[Z]) - att_1l_ct.fb_angular_velocity[YAW]);

    /*PID计算*/
    for (u8 i = 0; i < 3; i++) {
        pid_calculate(dT_s,             //周期（单位：秒）
            0,				//前馈值
            att_1l_ct.exp_angular_velocity[i],				//期望值（设定值）
            att_1l_ct.fb_angular_velocity[i],			//反馈值（）
            &angle_df_coe[i], //PID参数结构体
            &angle_df_res[i],	//PID数据结构体
            200,//积分误差限幅
            CTRL_1_INTE_LIM * flag.taking_off			//integration limit，积分幅度限幅
        );

        ct_val[i] = (angle_df_res[i].out);
    }

    /*赋值，最终比例调节*/
    mc.ct_val_rol = FINAL_P * ct_val[ROL];
    mc.ct_val_pit = X_PROPORTION_X_Y * FINAL_P * ct_val[PIT];
    mc.ct_val_yaw = FINAL_P * ct_val[YAW];
    /*输出量限幅*/
    mc.ct_val_rol = clamp(mc.ct_val_rol, -1000, 1000);
    mc.ct_val_pit = clamp(mc.ct_val_pit, -1000, 1000);
    mc.ct_val_yaw = clamp(mc.ct_val_yaw, -400, 400);
}


