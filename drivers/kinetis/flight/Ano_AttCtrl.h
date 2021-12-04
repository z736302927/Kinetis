/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ATT_CTRL_H
#define __ATT_CTRL_H
/* Includes ------------------------------------------------------------------*/
#include "Ano_FcData.h"
#include "Ano_Filter.h"
#include "Ano_Math.h"
#include "Ano_Pid.h"
/* Exported types ------------------------------------------------------------*/

struct rolling_state {
    u8 roll_mode;
#define ROLL_END		0
#define ROLL_UP		    1
#define ROLL_ROLLING	2
#define ROLL_KEEP		3

    u8 rolling_step;
    u8 roll_thr_step;

    float rol_angle[2];

    s16 ref_height;
    s16 up_height;

    u16 keep_cnt;
    s16 roll_up_speed;
    u8 roll_height_ok;
	s16 roll_acc_fix;
};

struct att_1l_ct {
    float set_yaw_speed;

    float exp_angular_velocity[VEC_RPY];

    float fb_angular_velocity[VEC_RPY];
};

struct att_2l_ct {
    float yaw_err;
    float exp_rol_adj;
    float exp_pit_adj;

    float exp_rol, exp_pit, exp_yaw;
    float fb_rol, fb_pit, fb_yaw;

};

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void angle_pid_init(void);
void angle_df_pid_init(void);
void Set_Att_1level_Ki(u8 mode);
void Set_Att_2level_Ki(u8 mode);

void Att_2level_Ctrl(float dT, s16 *CH_N);
void Att_1level_Ctrl(float dT);

#endif
