/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ANO_FC_DATA_H
#define __ANO_FC_DATA_H

#include <linux/types.h>

enum {
	AUTO_TAKE_OFF_NULL = 0,
	AUTO_TAKE_OFF = 1,
	AUTO_TAKE_OFF_FINISH,
	AUTO_LAND,
};

enum pwminmode_e {
	PWM = 0,
	PPM,
	SBUS,
};

enum {
	A_X = 0,
	A_Y,
	A_Z,
	G_X,
	G_Y,
	G_Z,
	TEM,
	MPU_ITEMS,
};

enum {
	CH_ROL = 0,
	CH_PIT,
	CH_THR,
	CH_YAW,
	AUX1,
	AUX2,
	AUX3,
	AUX4,
	CH_NUM,
};

enum {
	m1 = 0,
	m2,
	m3,
	m4,
	m5,
	m6,
	m7,
	m8,
};

enum {
	MPU_6050_0 = 0,
	MPU_6050_1,
};

enum {
	X = 0,
	Y = 1,
	Z = 2,
	VEC_XYZ,
};

enum {
	ROL = 0,
	PIT = 1,
	YAW = 2,
	VEC_RPY,
};

enum {
	KP = 0,
	KI = 1,
	KD = 2,
	PID,
};

enum _power_alarm {
	HIGH_POWER = 0,
	HALF_POWER,
	LOW_POWER,
	LOWEST_POWER,
};

enum _flight_mode {
	ATT_STAB = 0,//Attitude stabilization
	LOC_HOLD,
	RETURN_HOME,
};

enum {
	THR_MANUAL = 0,
	THR_AUTO,
};

struct fmu_state {
	//基本状态/传感器
	u8 start_ok;
	u8 sensor_imu_ok;
	u8 mems_temperature_ok;

	u8 motionless;
	u8 power_state;
	u8 wifi_ch_en;

	u8 rc_loss_back_home;
	u8 gps_ok;

	//控制状态
	u8 manual_locked;
	u8 unlock_err;
	u8 unlock_cmd;
	u8 unlock_sta;
	u8 thr_low;
	u8 locking;
	u8 taking_off;
	u8 set_yaw;
	u8 ct_loc_hold;
	u8 ct_alt_hold;

	//飞行状态
	u8 flying;
	u8 auto_take_off_land;
	u8 home_location_ok;
	u8 speed_mode;
	u8 thr_mode;
	u8 flight_mode;
	u8 flight_mode2;
	u8 gps_mode_en;
	u8 motor_preparation;
	u8 locked_rotor;
};

struct fc_state {
	float vel_limit_xy;
	float vel_limit_z_p;
	float vel_limit_z_n;
	float yaw_pal_limit;
};

struct fmu_sensor_state {
	unsigned gyro_ok : 1;
	unsigned acc_ok : 1;
	unsigned mag_ok : 1;
	unsigned baro_ok : 1;
	unsigned gps_ok : 1;
	unsigned sonar_ok : 1;
	unsigned tof_ok : 1;
	unsigned of_ok : 1;
	unsigned of_df_ok : 1;

	unsigned sonar_on : 1;
	unsigned tof_on : 1;
	unsigned of_flow_on : 1;
	unsigned of_tof_on : 1;
	unsigned baro_on : 1;
	unsigned gps_on : 1;
	unsigned uwb_on : 1;
	unsigned opmv_on : 1;
};

void data_save(void);
void Para_Data_Init(void);

#endif
