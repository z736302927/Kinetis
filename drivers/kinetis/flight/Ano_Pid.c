/******************** (C) COPYRIGHT 2016 ANO Tech ***************************
 * 作�?	 ：匿名科�?
 * 文件�? ：ANO_PID.c
 * 描述    ：PID函数
 * 官网    ：www.anotc.com
 * 淘宝    ：anotc.taobao.com
 * 技术Q�?�?90169595
*****************************************************************************/
#include "Ano_Pid.h"
#include "Ano_Math.h"
#include "Ano_Filter.h"


float PID_calculate(float dT_s,             //周期（单位：秒）
    float in_ff,				//前馈�?
    float expect,				//期望值（设定值）
    float feedback,			//反馈值（�?
    _PID_arg_st *pid_arg, //PID参数结构�?
    _PID_val_st *pid_val,	//PID数据结构�?
    float inte_d_lim,//积分误差限幅
    float inte_lim			//integration limit，积分限�?
)
{
    float differential, hz;
    hz = safe_div(1.0f, dT_s, 0);

//	pid_arg->k_inc_d_norm = clamp(pid_arg->k_inc_d_norm,0,1);



    pid_val->exp_d = (expect - pid_val->exp_old) * hz;

    if (pid_arg->fb_d_mode == 0)
        pid_val->fb_d = (feedback - pid_val->feedback_old) * hz;
    else
        pid_val->fb_d = pid_val->fb_d_ex;

    differential = (pid_arg->kd_ex * pid_val->exp_d - pid_arg->kd_fb * pid_val->fb_d);

    pid_val->err = (expect - feedback);

    pid_val->err_i += pid_arg->ki * clamp((pid_val->err), -inte_d_lim, inte_d_lim) * dT_s; //)*T;//+ differential/pid_arg->kp
    //pid_val->err_i += pid_arg->ki *(pid_val->err )*T;//)*T;//+ pid_arg->k_pre_d *pid_val->feedback_d
    pid_val->err_i = clamp(pid_val->err_i, -inte_lim, inte_lim);



    pid_val->out = pid_arg->k_ff * in_ff
        + pid_arg->kp * pid_val->err
        +	differential
//	    + pid_arg->k_inc_d_norm *pid_val->err_d_lpf + (1.0f-pid_arg->k_inc_d_norm) *differential
        + pid_val->err_i;

    pid_val->feedback_old = feedback;
    pid_val->exp_old = expect;

    return (pid_val->out);
}





/******************* (C) COPYRIGHT 2016 ANO TECH *****END OF FILE************/


