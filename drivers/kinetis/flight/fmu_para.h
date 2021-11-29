#ifndef __FMU_PARAMETER_H
#define	__FMU_PARAMETER_H

#include <linux/types.h>

#include "fmu_fc_data.h"

enum fmu_para_type	{
	PARA_NULL           = 0,
	PARA_HW_TYPE        = 1,
	PARA_HW_VER         = 2,
	PARA_SW_VER         = 3,
	PARA_BL_VER         = 4,
	PARA_INFO5          = 5,
	PARA_INFO6          = 6,
	PARA_INFO7          = 7,
	PARA_INFO8          = 8,
	PARA_INFO9          = 9,
	PARA_INFO10         = 10,
	PARA_PID_1_P        = 11,
	PARA_PID_1_I        = 12,
	PARA_PID_1_D        = 13,
	PARA_PID_2_P        = 14,
	PARA_PID_2_I        = 15,
	PARA_PID_2_D        = 16,
	PARA_PID_3_P        = 17,
	PARA_PID_3_I        = 18,
	PARA_PID_3_D        = 19,
	PARA_PID_4_P        = 20,
	PARA_PID_4_I        = 21,
	PARA_PID_4_D        = 22,
	PARA_PID_5_P        = 23,
	PARA_PID_5_I        = 24,
	PARA_PID_5_D        = 25,
	PARA_PID_6_P        = 26,
	PARA_PID_6_I        = 27,
	PARA_PID_6_D        = 28,
	PARA_PID_7_P        = 29,
	PARA_PID_7_I        = 30,
	PARA_PID_7_D        = 31,
	PARA_PID_8_P        = 32,
	PARA_PID_8_I        = 33,
	PARA_PID_8_D        = 34,
	PARA_PID_9_P        = 35,
	PARA_PID_9_I        = 36,
	PARA_PID_9_D        = 37,
	PARA_PID_10_P       = 38,
	PARA_PID_10_I       = 39,
	PARA_PID_10_D       = 40,
	PARA_PID_11_P       = 41,
	PARA_PID_11_I       = 42,
	PARA_PID_11_D       = 43,
	PARA_PID_12_P       = 44,
	PARA_PID_12_I       = 45,
	PARA_PID_12_D       = 46,
	PARA_PID_13_P       = 47,
	PARA_PID_13_I       = 48,
	PARA_PID_13_D       = 49,
	PARA_PID_14_P       = 50,
	PARA_PID_14_I       = 51,
	PARA_PID_14_D       = 52,
	PARA_PID_15_P       = 53,
	PARA_PID_15_I       = 54,
	PARA_PID_15_D       = 55,
	PARA_PID_16_P       = 56,
	PARA_PID_16_I       = 57,
	PARA_PID_16_D       = 58,
	PARA_PID_17_P       = 59,
	PARA_PID_17_I       = 60,
	PARA_PID_17_D       = 61,
	PARA_PID_18_P       = 62,
	PARA_PID_18_I       = 63,
	PARA_PID_18_D       = 64,
	PARA_PID_ONE_1      = 65,
	PARA_PID_ONE_2      = 66,
	PARA_PID_ONE_3      = 67,
	PARA_PID_MODE       = 68,
	PARA_RCINMODE       = 69,
	PARA_UNLOCKPWM      = 70,
	PARA_UNLOCK_OO      = 71,
	PARA_AUTO_GYR_CAL   = 72,
	PARA_BAT_CELLS      = 73,
	PARA_LV_WARN_100    = 74,
	PARA_LV_RETN_100    = 75,
	PARA_LV_LAND_100    = 76,
	PARA_CENPOS_X       = 77,
	PARA_CENPOS_Y       = 78,
	PARA_CENPOS_Z       = 79,
	PARA_TAKEOFFHIGH    = 80,
	PARA_TAKEOFFSPEED   = 81,
	PARA_LANDSPEED      = 82,
	PARA_LANDSPEED_MAX  = 83,
	PARA_AUTO_LANDING   = 84,
	PARA_HEATSWITCH     = 85,
	PARA_HEAT_TMPER     = 86,
	PARA_MAX_SPEED_HOR  = 87,
	PARA_MAX_SPEED_PRC  = 88,
	PARA_MAX_SPEED_UP   = 89,
	PARA_MAX_SPEED_DW   = 90,
	PARA_MAX_SPEED_YAW  = 91,
	PARA_SAFE_ATT       = 92,
	PARA_MAGMODE        = 93,
	PARA_ACANGVELMAX    = 94,
	PARA_YCANGVELMAX    = 95,
	PARA_YCANGACCMAX    = 96,
	PARA_RH_ALT         = 97,
	PARA_RH_VEL         = 98,
	PARA_GYR_FILTER     = 99,
	PARA_ACC_FILTER     = 100,
	PARA_ATT_FUSION     = 101,
	PARA_MAG_FUSION     = 102,
	PARA_HOR_FUSION     = 103,
	PARA_VER_FUSION     = 104,
	PARA_FENCE_MODE     = 105,
	PARA_FENCE_RADIUS   = 106,
	PARA_FENCE_WIDTH_X  = 107,
	PARA_FENCE_WIDTH_Y  = 108,
	PARA_FENCE_HEIGHT   = 109,
	PARA_COM1_BAUD      = 110,
	PARA_COM2_BAUD      = 111,
	PARA_COM2_MODE      = 112,
	PARA_RGBOUT_ENA     = 113,
	PARA_DataOutTime_01 = 114,
	PARA_DataOutTime_02 = 115,
	PARA_DataOutTime_03 = 116,
	PARA_DataOutTime_04 = 117,
	PARA_DataOutTime_05 = 118,
	PARA_DataOutTime_06 = 119,
	PARA_DataOutTime_07 = 120,
	PARA_DataOutTime_08 = 121,
	PARA_DataOutTime_09 = 122,
	PARA_DataOutTime_0A = 123,
	PARA_DataOutTime_0B = 124,
	PARA_DataOutTime_0C = 125,
	PARA_DataOutTime_0D = 126,
	PARA_DataOutTime_0E = 127,
	PARA_DataOutTime_20 = 128,
	PARA_DataOutTime_21 = 129,
	PARA_DataOutTime_30 = 130,
	PARA_DataOutTime_32 = 131,
	PARA_DataOutTime_33 = 132,
	PARA_DataOutTime_34 = 133,
	PARA_DataOutTime_40 = 134,
	PARA_DataOutTime_41 = 135,
	PARA_DATA_NUM       = 136,
	CAL_ACC_OFFSET_X    = 137,
	CAL_ACC_OFFSET_Y    = 138,
	CAL_ACC_OFFSET_Z    = 139,
	CAL_ACC_SENSIV_X    = 140,
	CAL_ACC_SENSIV_Y    = 141,
	CAL_ACC_SENSIV_Z    = 142,
	CAL_ACC_IEM_00      = 143,
	CAL_ACC_IEM_01      = 144,
	CAL_ACC_IEM_02      = 145,
	CAL_ACC_IEM_10      = 146,
	CAL_ACC_IEM_11      = 147,
	CAL_ACC_IEM_12      = 148,
	CAL_ACC_IEM_20      = 149,
	CAL_ACC_IEM_21      = 150,
	CAL_ACC_IEM_22      = 151,
	CAL_MAG_OFFSET_X    = 152,
	CAL_MAG_OFFSET_Y    = 153,
	CAL_MAG_OFFSET_Z    = 154,
	CAL_MAG_SENSIV_X    = 155,
	CAL_MAG_SENSIV_Y    = 156,
	CAL_MAG_SENSIV_Z    = 157,
};

struct fmu_para {
	/* 安装误差矩阵 */
	float iem[3][3];
	/* 加速度计零偏 */
	float acc_zero_offset[3];
	/* 1G */
	float acc_sensitivity_ref[3];
	/* 陀螺仪零偏 */
	float gyr_zero_offset[3];
	/* 重心相对传感器位置偏移量 */
	float body_central_pos_cm[VEC_XYZ];
	/* 磁力计零偏 */
	float mag_offset[VEC_XYZ];
	/* 磁力计校正比例 */
	float mag_gain[VEC_XYZ];
	/* 姿态控制角速度环PID参数 */
	float pid_att_1level[VEC_RPY][PID];
	/* 姿态控制角度环PID参数 */
	float pid_att_2level[VEC_RPY][PID];
	/* 高度控制高度速度环PID参数 */
	float pid_alt_1level[PID];
	/* 高度控制高度环PID参数 */
	float pid_alt_2level[PID];
	/* 位置控制位置速度环PID参数 */
	float pid_loc_1level[PID];
	/* 位置控制位置环PID参数 */
	float pid_loc_2level[PID];
	/* 位置控制位置速度环PID参数 */
	float pid_gps_loc_1level[PID];
	/* 位置控制位置环PID参数 */
	float pid_gps_loc_2level[PID];

	float warn_power_voltage;
	s32 bat_cell;
	float lowest_power_voltage;

	float auto_take_off_height;
	float auto_take_off_speed;
	float auto_landing_speed;
	float idle_speed_pwm;

	/* 接收机模式，分别为PWM型PPM型 */
	u8 pwm_in_mode;
	u8 heat_switch;

	u8 acc_calibrated;
	u8 mag_calibrated;
	u8 reserve1;
	u8 reserve2;

	/* 飞控第一次初始化，需要做一些特殊工作，比如清空flash */
	u16 frist_init;

	struct {
		u8 save_en;
		u8 save_trig;
		u16 time_delay;
	} state;
};



#endif

