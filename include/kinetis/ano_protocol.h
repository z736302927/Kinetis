#ifndef _ANO_PROTOCOL_H
#define _ANO_PROTOCOL_H

#include <linux/types.h>

#undef current

enum ANO_LOG_COLOR {
	ANO_LOG_COLOR_BLACK = 0,
	ANO_LOG_COLOR_RED = 1,
	ANO_LOG_COLOR_GREEN = 2,
};

int ano_send_flexible_frame(void *buffer, u32 len);
int ano_send_ack(u8 func_code, u8 check_sum, u8 check_add);
int ano_send_inertial_sensor(s16 acc_x, s16 acc_y, s16 acc_z,
	s16 gyr_x, s16 gyr_y, s16 gyr_z,
	u8 shock_sta);
int ano_send_compass_pressure_temp(s16 mag_x, s16 mag_y, s16 mag_z,
	s32 alt_bar, s16 tmp, u8 bar_sta, u8 mag_sta);
int ano_send_euler_angle_format(float rol, float pit, float yaw, u8 fusion_sta);
int ano_send_quaternion_format(float v0, float v1, float v2, float v3,
	u8 fusion_sta);
int ano_send_height(s32 alt_fu, s32 alt_add, u8 alt_sta);
int ano_send_fmu_op_mode(u8 mode, u8 locked, u8 cid, u8 cmd0, u8 cmd1);
int ano_send_fmu_speed(s16 speed_x, s16 speed_y, s16 speed_z);
int ano_send_pos_offset(s32 pos_x, s32 pos_y);
int ano_send_wind_speed(s16 wind_x, s16 wind_y);
int ano_send_target_pose(s16 tar_rol, s16 tar_pit, s16 tar_yaw);
int ano_send_target_speed(s16 tar_speed_x, s16 tar_speed_y, s16 tar_speed_z);
int ano_send_return_info(float r_a, u16 r_d);
int ano_send_voltage_current(float votage, float current);
int ano_send_ext_modt(u8 sta_g_vel, u8 sta_g_pos,
	u8 sta_gps, u8 sta_alt_add);
int ano_send_rgb(u8 bri_r, u8 bri_g, u8 bri_b, u8 bri_a);
int ano_send_log_string(u8 color, char *buffer);
int ano_send_log_string_num(s32 val, char *buffer);
int ano_send_pwm(u16 pwm1, u16 pwm2, u16 pwm3, u16 pwm4,
	u16 pwm5, u16 pwm6, u16 pwm7, u16 pwm8);
int ano_send_attitude(s16 ctrl_rol, s16 ctrl_pit, s16 ctrl_thr, s16 ctrl_yaw);
int ano_ptc_gps_info(u8 fix_sta, u8 s_num, float lng, float lat, s32 alt_gps,
	s16 n_spe, s16 e_spe, s16 d_spe, float pdop, float sacc, float vacc);
int ano_send_up(s32 pos_x, s32 pos_y, s32 pos_z);
int ano_send_us(s16 speed_x, s16 speed_y, s16 speed_z);
int ano_send_ud(u8 direction, u16 angle, u32 dist);
int ano_send_remote_ctrl(s16 rol, s16 pit, s16 thr, s16 yaw,
	s16 aux1, s16 aux2, s16 aux3, s16 aux4, s16 aux5, s16 aux6);
int ano_send_realtime_ctrl(s16 ctrl_rol, s16 ctrl_pit, s16 ctrl_thr,
	s16 ctrl_yawdps, s16 ctrl_spd_x, s16 ctrl_spd_y, s16 ctrl_spd_z);
int ano_send_optical_flow(u8 mode, u8 state, s8 dx_0, s8 dy_0, u8 quality);
int ano_send_optical_flow_f(u8 mode, u8 state, s16 dx_1, s16 dy_1, u8 quality);
int ano_send_optical_flow_isf(u8 mode, u8 state,
	s16 dx_2, s16 dy_2, s16 dx_fix, s16 dy_fix, s16 integ_x, s16 integ_y,
	u8 quality);
int ano_read_waypoint(u8 num);
int ano_send_waypoint(u8 num, float lat, float lng,
	s32 alt, u16 spd, u16 yaw, u8 fun,
	u8 cmd1, u8 cmd2, u8 cmd3, u8 cmd4);
int ano_send_cmd_frame(u8 cid,
	u8 cmd1, u8 cmd2, u8 cmd3, u8 cmd4,
	u8 cmd5, u8 cmd6, u8 cmd7, u8 cmd8, u8 cmd9);
int ano_send_parameter_read(u16 par_id);
int ano_send_parameter_write(u16 par_id, s32 par_val);



#endif /* _ANO_PROTOCOL_H */