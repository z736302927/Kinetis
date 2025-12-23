#ifndef _FMU_FC_H
#define _FMU_FC_H

#include <linux/types.h>

struct fmu_state {
	//Basic status/sensor
	u8 start_ok;
	u8 sensor_imu_ok;
	u8 mems_temperature_ok;

	u8 motionless;
	u8 power_state;
	u8 wifi_ch_en;

	u8 rc_loss_back_home;
	u8 gps_ok;

	//Control state
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

	//Flight status
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

#endif /* _FMU_FC_H */