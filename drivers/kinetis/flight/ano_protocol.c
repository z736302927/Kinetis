
#include <generated/deconfig.h>
#include <linux/slab.h>

#include <kinetis/ano_protocol.h>
#include <kinetis/fmu_config.h>

struct ano_protocol {
	u8 head;
#define ANO_HEAD	0xAA
	u8 addr;
#define ANO_NO_TARGET		0xFF
#define ANO_GND_STATION		0xAF
#define ANO_TUOKONG_FMU		0x05
#define ANO_DATA_MOD		0x10
#define ANO_OPTICAL_FLOW		0x22
#define ANO_UWB		0x30
#define ANO_LINXIAO_IMU		0x60
#define ANO_LINXIAO_FMU		0x61

	u8 func_code;
#define CHECK_DATA_FRAME		0x00
#define INERTIAL_SENSOR_DATA		0x01
#define COMPASS_PRESSURE_TEMP		0x02
#define EULER_ANGLE_FORMAT		0x03
#define QUATERNION_FORMAT		0x04
#define HEIGHT_DATA		0x05
#define FMU_OP_MODE		0x06
#define FMU_SPEED		0x07
#define POS_OFFSET		0x08
#define WIND_SPEED		0x09
#define TARGET_POSE		0x0A
#define TARGET_SPEED		0x0B
#define RETURN_INFO		0x0C
#define VOLTAGE_CURRENT		0x0D
#define EXT_MOD_STATUS		0x0E
#define RGB_BRIGHTNESS		0x0F
#define LOG_STRING		0xA0
#define LOG_STRING_NUM		0xA1
#define PWM_OUTPUT		0x20
#define ATTITUDE_OUTPUT		0x21
#define GPS_INFO		0x30
#define RESERVED		0x31
#define UNIVERSAL_POSITION		0x32
#define UNIVERSAL_SPEED		0x33
#define UNIVERSAL_DISTANCE		0x34
#define REMOTE_CONTROL		0x40
#define REALTIME_CONTROL		0x41
#define OPTICAL_FLOW		0x51
#define WAYPOINT_READING		0x60
#define WAYPOINT_WRITING		0x61
#define CMD_FRAME		0xE0
#define PARAMETER_READ		0xE1
#define PARAMETER_WRITE		0xE2
#define FLEXIBLE_FRAME_1		0xF1
#define FLEXIBLE_FRAME_2		0xF2
#define FLEXIBLE_FRAME_3		0xF3
#define FLEXIBLE_FRAME_4		0xF4
#define FLEXIBLE_FRAME_5		0xF5
#define FLEXIBLE_FRAME_6		0xF6
#define FLEXIBLE_FRAME_7		0xF7
#define FLEXIBLE_FRAME_8		0xF8
#define FLEXIBLE_FRAME_9		0xF9
#define FLEXIBLE_FRAME_A		0xFA
#define FLEXIBLE_FRAME_B		0xFB
#define FLEXIBLE_FRAME_C		0xFC
#define FLEXIBLE_FRAME_D		0xFD
#define FLEXIBLE_FRAME_E		0xFE
#define FLEXIBLE_FRAME_F		0xFF

	u8 len;
	void *buffer;
	u8 check_sum;
	u8 check_add;
};

struct ano_ptc_ack {
	u8 func_code;
	u8 check_sum;
	u8 check_add;
};

struct ano_ptc_inertial_sensor {
	s16 acc_x;
	s16 acc_y;
	s16 acc_z;
	s16 gyr_x;
	s16 gyr_y;
	s16 gyr_z;
	u8 shock_sta;
};

struct ano_ptc_cpt {
	s16 mag_x;
	s16 mag_y;
	s16 mag_z;
	s32 alt_bar;
	s16 tmp;
	u8 bar_sta;
	u8 mag_sta;
};

struct ano_ptc_eaf {
	s16 rol;
	s16 pit;
	s16 yaw;
	u8 fusion_sta;
};

struct ano_ptc_qf {
	s16 v0;
	s16 v1;
	s16 v2;
	s16 v3;
	u8 fusion_sta;
};

struct ano_ptc_height {
	s32 alt_fu;
	s32 alt_add;
	u8 alt_sta;
};

struct ano_ptc_fmu_op_mode {
	u8 mode;
	u8 locked;
	u8 cid;
	u8 cmd0;
	u8 cmd1;
};

struct ano_ptc_fmu_speed {
	s16 speed_x;
	s16 speed_y;
	s16 speed_z;
};

struct ano_ptc_pos_offset {
	s32 pos_x;
	s32 pos_y;
};

struct ano_ptc_wind_speed {
	s16 wind_x;
	s16 wind_y;
};

struct ano_ptc_target_pose {
	s16 tar_rol;
	s16 tar_pit;
	s16 tar_yaw;
};

struct ano_ptc_target_speed {
	s16 tar_speed_x;
	s16 tar_speed_y;
	s16 tar_speed_z;
};

struct ano_ptc_return_info {
	s16 r_a;
	u16 r_d;
};

struct ano_ptc_voltage_current {
	u16 votage;
	u16 current;
};

struct ano_ptc_ext_mod {
	u8 sta_g_vel;
	u8 sta_g_pos;
	u8 sta_gps;
	u8 sta_alt_add;
};

struct ano_ptc_rgb {
	u8 bri_r;
	u8 bri_g;
	u8 bri_b;
	u8 bri_a;
};

struct ano_ptc_pwm {
	u16 pwm1;
	u16 pwm2;
	u16 pwm3;
	u16 pwm4;
	u16 pwm5;
	u16 pwm6;
	u16 pwm7;
	u16 pwm8;
};

struct ano_ptc_attitude {
	s16 ctrl_rol;
	s16 ctrl_pit;
	s16 ctrl_thr;
	s16 ctrl_yaw;
};

struct ano_ptc_gps_info {
	u8 fix_sta;
	u8 s_num;
	s32 lng;
	s32 lat;
	s32 alt_gps;
	s16 n_spe;
	s16 e_spe;
	s16 d_spe;
	u8 pdop;
	u8 sacc;
	u8 vacc;
};

struct ano_ptc_up {
	s32 pos_x;
	s32 pos_y;
	s32 pos_z;
};

struct ano_ptc_us {
	s16 speed_x;
	s16 speed_y;
	s16 speed_z;
};

struct ano_ptc_ud {
	u8 direction;
	u16 angle;
	u32 dist;
};

struct ano_ptc_remote_ctrl {
	s16 rol;
	s16 pit;
	s16 thr;
	s16 yaw;
	s16 aux1;
	s16 aux2;
	s16 aux3;
	s16 aux4;
	s16 aux5;
	s16 aux6;
};

struct ano_ptc_realtime_ctrl {
	s16 ctrl_rol;
	s16 ctrl_pit;
	s16 ctrl_thr;
	s16 ctrl_yawdps;
	s16 ctrl_spd_x;
	s16 ctrl_spd_y;
	s16 ctrl_spd_z;
};

enum ptical_flow_mode {
	PFM_RAW,
	PFM_FUSION,
	PFM_ISF
};

struct ano_ptc_optical_flow {
	u8 mode;
	u8 state;
	s8 dx_0;
	s8 dy_0;
	u8 quality;
};

struct ano_ptc_optical_flow_f {
	u8 mode;
	u8 state;
	s16 dx_1;
	s16 dy_1;
	u8 quality;
};

struct ano_ptc_optical_flow_isf {
	u8 mode;
	u8 state;
	s16 dx_2;
	s16 dy_2;
	s16 dx_fix;
	s16 dy_fix;
	s16 integ_x;
	s16 integ_y;
	u8 quality;
};

struct ano_ptc_waypoint {
	u8 num;
	s32 lat;
	s32 lng;
	s32 alt;
	u16 spd;
	u16 yaw;
	u8 fun;
	u8 cmd1;
	u8 cmd2;
	u8 cmd3;
	u8 cmd4;
};

struct ano_ptc_cmd_frame {
	u8 cid;
	u8 cmd1;
	u8 cmd2;
	u8 cmd3;
	u8 cmd4;
	u8 cmd5;
	u8 cmd6;
	u8 cmd7;
	u8 cmd8;
	u8 cmd9;
};

struct ano_ptc_parameter_read {
	u16 par_id;
};

struct ano_ptc_parameter_write {
	u16 par_id;
	s32 par_val;
};

int ano_send_flexible_frame(void *buffer, u32 len)
{
	struct ano_protocol ano;
	u8 check_len, *check_buff;
	u8 i;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = FLEXIBLE_FRAME_1;
	ano.len = len;
	ano.buffer = buffer;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

static int process_fun_code(u8 func_code, void *buffer, u32 len)
{
	int ret;

	switch (func_code) {
	case CHECK_DATA_FRAME:
		break;
	case INERTIAL_SENSOR_DATA:
		break;
	case COMPASS_PRESSURE_TEMP:
		break;
	case EULER_ANGLE_FORMAT:
		break;
	case QUATERNION_FORMAT:
		break;
	case HEIGHT_DATA:
		break;
	case FMU_OP_MODE:
		break;
	case FMU_SPEED:
		break;
	case POS_OFFSET:
		break;
	case WIND_SPEED:
		break;
	case TARGET_POSE:
		break;
	case TARGET_SPEED:
		break;
	case RETURN_INFO:
		break;
	case VOLTAGE_CURRENT:
		break;
	case EXT_MOD_STATUS:
		break;
	case RGB_BRIGHTNESS:
		break;
	case LOG_STRING:
		break;
	case LOG_STRING_NUM:
		break;
	case PWM_OUTPUT:
		break;
	case ATTITUDE_OUTPUT:
		break;
	case GPS_INFO:
		break;
	case RESERVED:
		break;
	case UNIVERSAL_POSITION:
		break;
	case UNIVERSAL_SPEED:
		break;
	case UNIVERSAL_DISTANCE:
		break;
	case REMOTE_CONTROL:
		break;
	case REALTIME_CONTROL:
		break;
	case OPTICAL_FLOW:
		break;
	case WAYPOINT_READING:
		break;
	case WAYPOINT_WRITING:
		break;
	case CMD_FRAME:
		break;
	case PARAMETER_READ:
		break;
	case PARAMETER_WRITE:
		break;
	case FLEXIBLE_FRAME_1:
		break;
	case FLEXIBLE_FRAME_2:
		break;
	case FLEXIBLE_FRAME_3:
		break;
	case FLEXIBLE_FRAME_4:
		break;
	case FLEXIBLE_FRAME_5:
		break;
	case FLEXIBLE_FRAME_6:
		break;
	case FLEXIBLE_FRAME_7:
		break;
	case FLEXIBLE_FRAME_8:
		break;
	case FLEXIBLE_FRAME_9:
		break;
	case FLEXIBLE_FRAME_A:
		break;
	case FLEXIBLE_FRAME_B:
		break;
	case FLEXIBLE_FRAME_C:
		break;
	case FLEXIBLE_FRAME_D:
		break;
	case FLEXIBLE_FRAME_E:
		break;
	case FLEXIBLE_FRAME_F:
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

int ano_decode_link_partner(void *buffer, u32 len)
{
	u8 *check_buff = buffer;
	u8 check_sum = 0;
	u8 check_add = 0;
	u8 i;
	int ret;

	for (i = 0; i < len - 2; i++) {
		check_sum += check_buff[i];
		check_add += check_sum;
	}

	if (check_sum != check_buff[len - 2] || check_add != check_buff[len - 1]) {
		return -EPIPE;
	}

	if (0xAA != check_buff[0]) {
		return -EPIPE;
	}

	switch (check_buff[1]) {
	case ANO_NO_TARGET:
		ret = process_fun_code(check_buff[2], &check_buff[4], check_buff[3]);
		break;
	case ANO_GND_STATION:
		break;
	case ANO_TUOKONG_FMU:
		break;
	case ANO_DATA_MOD:
		break;
	case ANO_OPTICAL_FLOW:
		break;
	case ANO_UWB:
		break;
	case ANO_LINXIAO_IMU:
		break;
	case ANO_LINXIAO_FMU:
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

void ano_receive_frame(u8 data, void *buffer, u8 *done)
{
	u8 *buf = buffer;
	static u8 cnt;
	static ktime_t time;
	u16 i, len;
	int ret;

	if (ktime_us_delta(ktime_get(), time) > 2500 && time) {
		cnt = 0;
	}
	time = ktime_get();

	buf[cnt++] = data;

	if (buf[0] != ANO_HEAD) {
		cnt = 0;
	}

	if (cnt == 4) {
		len = buf[3];
	} else if (cnt == len + 4 + 2) {
		*done = true;
	} else {
		*done = false;
	}
}

int ano_send_ack(u8 func_code, u8 check_sum, u8 check_add)
{
	struct ano_protocol ano;
	struct ano_ptc_ack ack;
	u8 check_len, *check_buff;
	u8 i;

	ack.func_code = func_code;
	ack.check_sum = check_sum;
	ack.check_add = check_add;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = CHECK_DATA_FRAME;
	ano.len = sizeof(ack);
	ano.buffer = &ack;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_inertial_sensor(s16 acc_x, s16 acc_y, s16 acc_z,
	s16 gyr_x, s16 gyr_y, s16 gyr_z,
	u8 shock_sta)
{
	struct ano_protocol ano;
	struct ano_ptc_inertial_sensor is;
	u8 check_len, *check_buff;
	u8 i;

	is.acc_x = acc_x;
	is.acc_y = acc_y;
	is.acc_z = acc_z;
	is.gyr_x = gyr_x;
	is.gyr_y = gyr_y;
	is.gyr_z = gyr_z;
	is.shock_sta = shock_sta;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = INERTIAL_SENSOR_DATA;
	ano.len = sizeof(is);
	ano.buffer = &is;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_compass_pressure_temp(s16 mag_x, s16 mag_y, s16 mag_z,
	s32 alt_bar, s16 tmp, u8 bar_sta, u8 mag_sta)
{
	struct ano_protocol ano;
	struct ano_ptc_cpt cpt;
	u8 check_len, *check_buff;
	u8 i;

	cpt.mag_x = mag_x;
	cpt.mag_y = mag_y;
	cpt.mag_z = mag_z;
	cpt.alt_bar = alt_bar;
	cpt.tmp = tmp;
	cpt.bar_sta = bar_sta;
	cpt.mag_sta = mag_sta;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = COMPASS_PRESSURE_TEMP;
	ano.len = sizeof(cpt);
	ano.buffer = &cpt;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_euler_angle_format(float rol, float pit, float yaw, u8 fusion_sta)
{
	struct ano_protocol ano;
	struct ano_ptc_eaf eaf;
	u8 check_len, *check_buff;
	u8 i;

	eaf.rol = rol * 100;
	eaf.pit = pit * 100;
	eaf.yaw = yaw * 100;
	eaf.fusion_sta = fusion_sta;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = EULER_ANGLE_FORMAT;
	ano.len = sizeof(eaf);
	ano.buffer = &eaf;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_quaternion_format(float v0, float v1, float v2, float v3,
	u8 fusion_sta)
{
	struct ano_protocol ano;
	struct ano_ptc_qf qf;
	u8 check_len, *check_buff;
	u8 i;

	qf.v0 = v0 * 10000;
	qf.v1 = v1 * 10000;
	qf.v2 = v2 * 10000;
	qf.v3 = v3 * 10000;
	qf.fusion_sta = fusion_sta;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = QUATERNION_FORMAT;
	ano.len = sizeof(qf);
	ano.buffer = &qf;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_height(s32 alt_fu, s32 alt_add, u8 alt_sta)
{
	struct ano_protocol ano;
	struct ano_ptc_height height;
	u8 check_len, *check_buff;
	u8 i;

	height.alt_fu = alt_fu;
	height.alt_add = alt_add;
	height.alt_sta = alt_sta;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = HEIGHT_DATA;
	ano.len = sizeof(height);
	ano.buffer = &height;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_fmu_op_mode(u8 mode, u8 locked, u8 cid, u8 cmd0, u8 cmd1)
{
	struct ano_protocol ano;
	struct ano_ptc_fmu_op_mode fmu_op_mode;
	u8 check_len, *check_buff;
	u8 i;

	fmu_op_mode.mode = mode;
	fmu_op_mode.locked = locked;
	fmu_op_mode.cid = cid;
	fmu_op_mode.cmd0 = cmd0;
	fmu_op_mode.cmd1 = cmd1;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = FMU_OP_MODE;
	ano.len = sizeof(fmu_op_mode);
	ano.buffer = &fmu_op_mode;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_fmu_speed(s16 speed_x, s16 speed_y, s16 speed_z)
{
	struct ano_protocol ano;
	struct ano_ptc_fmu_speed fmu_speed;
	u8 check_len, *check_buff;
	u8 i;

	fmu_speed.speed_x = speed_x;
	fmu_speed.speed_y = speed_y;
	fmu_speed.speed_z = speed_z;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = FMU_SPEED;
	ano.len = sizeof(fmu_speed);
	ano.buffer = &fmu_speed;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_pos_offset(s32 pos_x, s32 pos_y)
{
	struct ano_protocol ano;
	struct ano_ptc_pos_offset pos_offset;
	u8 check_len, *check_buff;
	u8 i;

	pos_offset.pos_x = pos_x;
	pos_offset.pos_y = pos_y;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = POS_OFFSET;
	ano.len = sizeof(pos_offset);
	ano.buffer = &pos_offset;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_wind_speed(s16 wind_x, s16 wind_y)
{
	struct ano_protocol ano;
	struct ano_ptc_wind_speed wind_speed;
	u8 check_len, *check_buff;
	u8 i;

	wind_speed.wind_x = wind_x;
	wind_speed.wind_y = wind_y;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = WIND_SPEED;
	ano.len = sizeof(wind_speed);
	ano.buffer = &wind_speed;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_target_pose(s16 tar_rol, s16 tar_pit, s16 tar_yaw)
{
	struct ano_protocol ano;
	struct ano_ptc_target_pose target_pose;
	u8 check_len, *check_buff;
	u8 i;

	target_pose.tar_rol = tar_rol;
	target_pose.tar_pit = tar_pit;
	target_pose.tar_yaw = tar_yaw;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = TARGET_POSE;
	ano.len = sizeof(target_pose);
	ano.buffer = &target_pose;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_target_speed(s16 tar_speed_x, s16 tar_speed_y, s16 tar_speed_z)
{
	struct ano_protocol ano;
	struct ano_ptc_target_speed target_speed;
	u8 check_len, *check_buff;
	u8 i;

	target_speed.tar_speed_x = tar_speed_x;
	target_speed.tar_speed_y = tar_speed_y;
	target_speed.tar_speed_z = tar_speed_z;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = TARGET_SPEED;
	ano.len = sizeof(target_speed);
	ano.buffer = &target_speed;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_return_info(float r_a, u16 r_d)
{
	struct ano_protocol ano;
	struct ano_ptc_return_info return_info;
	u8 check_len, *check_buff;
	u8 i;

	return_info.r_a = r_a * 10;
	return_info.r_d = r_d;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = TARGET_SPEED;
	ano.len = sizeof(return_info);
	ano.buffer = &return_info;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_voltage_current(float votage, float current)
{
	struct ano_protocol ano;
	struct ano_ptc_voltage_current voltage_current;
	u8 check_len, *check_buff;
	u8 i;

	voltage_current.votage = votage * 100;
	voltage_current.current = current * 100;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = VOLTAGE_CURRENT;
	ano.len = sizeof(voltage_current);
	ano.buffer = &voltage_current;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_ext_modt(u8 sta_g_vel, u8 sta_g_pos,
	u8 sta_gps, u8 sta_alt_add)
{
	struct ano_protocol ano;
	struct ano_ptc_ext_mod ext_mod;
	u8 check_len, *check_buff;
	u8 i;

	ext_mod.sta_g_vel = sta_g_vel;
	ext_mod.sta_g_pos = sta_g_pos;
	ext_mod.sta_gps = sta_gps;
	ext_mod.sta_alt_add = sta_alt_add;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = EXT_MOD_STATUS;
	ano.len = sizeof(ext_mod);
	ano.buffer = &ext_mod;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_rgb(u8 bri_r, u8 bri_g, u8 bri_b, u8 bri_a)
{
	struct ano_protocol ano;
	struct ano_ptc_rgb rgb;
	u8 check_len, *check_buff;
	u8 i;

	rgb.bri_r = bri_r;
	rgb.bri_g = bri_g;
	rgb.bri_b = bri_b;
	rgb.bri_a = bri_a;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = RGB_BRIGHTNESS;
	ano.len = sizeof(rgb);
	ano.buffer = &rgb;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_log_string(u8 color, char *buffer)
{
	struct ano_protocol ano;
	u8 check_len, *check_buff;
	u8 i;
	u32 len = strlen(buffer);
	int ret;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = LOG_STRING;
	ano.len = len + sizeof(color);
	ano.buffer = kmalloc(ano.len, GFP_KERNEL);
	if (ano.buffer) {
		return -ENOMEM;
	}
	memcpy(ano.buffer, &color, sizeof(color));
	memcpy(ano.buffer + sizeof(color), &buffer, len);
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	ret = ano_transfer_module(ano.buffer,
			ano.len + sizeof(ano) - sizeof(ano.buffer));
	if (ret) {
		return ret;
	}

	kfree(ano.buffer);

	return 0;
}

int ano_send_log_string_num(s32 val, char *buffer)
{
	struct ano_protocol ano;
	u8 check_len, *check_buff;
	u8 i;
	u32 len = strlen(buffer);

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = LOG_STRING_NUM;
	ano.len = len + sizeof(val);
	ano.buffer = kmalloc(ano.len, GFP_KERNEL);
	if (ano.buffer) {
		return -ENOMEM;
	}
	memcpy(ano.buffer, &val, sizeof(val));
	memcpy(ano.buffer + sizeof(val), &buffer, len);
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	kfree(ano.buffer);
	return 0;
}

int ano_send_pwm(u16 pwm1, u16 pwm2, u16 pwm3, u16 pwm4,
	u16 pwm5, u16 pwm6, u16 pwm7, u16 pwm8)
{
	struct ano_protocol ano;
	struct ano_ptc_pwm pwm;
	u8 check_len, *check_buff;
	u8 i;

	pwm.pwm1 = pwm1;
	pwm.pwm2 = pwm2;
	pwm.pwm3 = pwm3;
	pwm.pwm4 = pwm4;
	pwm.pwm5 = pwm5;
	pwm.pwm6 = pwm6;
	pwm.pwm7 = pwm7;
	pwm.pwm8 = pwm8;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = PWM_OUTPUT;
	ano.len = sizeof(pwm);
	ano.buffer = &pwm;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_attitude(s16 ctrl_rol, s16 ctrl_pit, s16 ctrl_thr, s16 ctrl_yaw)
{
	struct ano_protocol ano;
	struct ano_ptc_attitude attitude;
	u8 check_len, *check_buff;
	u8 i;

	attitude.ctrl_rol = ctrl_rol;
	attitude.ctrl_pit = ctrl_pit;
	attitude.ctrl_thr = ctrl_thr;
	attitude.ctrl_yaw = ctrl_yaw;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = ATTITUDE_OUTPUT;
	ano.len = sizeof(attitude);
	ano.buffer = &attitude;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_ptc_gps_info(u8 fix_sta, u8 s_num, float lng, float lat, s32 alt_gps,
	s16 n_spe, s16 e_spe, s16 d_spe, float pdop, float sacc, float vacc)
{
	struct ano_protocol ano;
	struct ano_ptc_gps_info gps_info;
	u8 check_len, *check_buff;
	u8 i;

	gps_info.fix_sta = fix_sta;
	gps_info.s_num = s_num;
	gps_info.lng = lng * 10000000;
	gps_info.lat = lat * 10000000;
	gps_info.alt_gps = alt_gps;
	gps_info.n_spe = n_spe;
	gps_info.e_spe = e_spe;
	gps_info.d_spe = d_spe;
	gps_info.pdop = pdop / 100;
	gps_info.sacc = sacc / 100;
	gps_info.vacc = vacc / 100;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = GPS_INFO;
	ano.len = sizeof(gps_info);
	ano.buffer = &gps_info;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_up(s32 pos_x, s32 pos_y, s32 pos_z)
{
	struct ano_protocol ano;
	struct ano_ptc_up up;
	u8 check_len, *check_buff;
	u8 i;

	up.pos_x = pos_x;
	up.pos_y = pos_y;
	up.pos_z = pos_z;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = UNIVERSAL_POSITION;
	ano.len = sizeof(up);
	ano.buffer = &up;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_us(s16 speed_x, s16 speed_y, s16 speed_z)
{
	struct ano_protocol ano;
	struct ano_ptc_us us;
	u8 check_len, *check_buff;
	u8 i;

	us.speed_x = speed_x;
	us.speed_y = speed_y;
	us.speed_z = speed_z;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = UNIVERSAL_SPEED;
	ano.len = sizeof(us);
	ano.buffer = &us;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_ud(u8 direction, u16 angle, u32 dist)
{
	struct ano_protocol ano;
	struct ano_ptc_ud ud;
	u8 check_len, *check_buff;
	u8 i;

	ud.direction = direction;
	ud.angle = angle;
	ud.dist = dist;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = UNIVERSAL_DISTANCE;
	ano.len = sizeof(ud);
	ano.buffer = &ud;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_remote_ctrl(s16 rol, s16 pit, s16 thr, s16 yaw,
	s16 aux1, s16 aux2, s16 aux3, s16 aux4, s16 aux5, s16 aux6)
{
	struct ano_protocol ano;
	struct ano_ptc_remote_ctrl remote_ctrl;
	u8 check_len, *check_buff;
	u8 i;

	remote_ctrl.rol = rol;
	remote_ctrl.pit = pit;
	remote_ctrl.thr = thr;
	remote_ctrl.yaw = yaw;
	remote_ctrl.aux1 = aux1;
	remote_ctrl.aux2 = aux2;
	remote_ctrl.aux3 = aux3;
	remote_ctrl.aux4 = aux4;
	remote_ctrl.aux5 = aux5;
	remote_ctrl.aux6 = aux6;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = REMOTE_CONTROL;
	ano.len = sizeof(remote_ctrl);
	ano.buffer = &remote_ctrl;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_realtime_ctrl(s16 ctrl_rol, s16 ctrl_pit, s16 ctrl_thr,
	s16 ctrl_yawdps, s16 ctrl_spd_x, s16 ctrl_spd_y, s16 ctrl_spd_z)
{
	struct ano_protocol ano;
	struct ano_ptc_realtime_ctrl realtime_ctrl;
	u8 check_len, *check_buff;
	u8 i;

	realtime_ctrl.ctrl_rol = ctrl_rol;
	realtime_ctrl.ctrl_pit = ctrl_pit;
	realtime_ctrl.ctrl_thr = ctrl_thr;
	realtime_ctrl.ctrl_yawdps = ctrl_yawdps;
	realtime_ctrl.ctrl_spd_x = ctrl_spd_x;
	realtime_ctrl.ctrl_spd_y = ctrl_spd_y;
	realtime_ctrl.ctrl_spd_z = ctrl_spd_z;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = REALTIME_CONTROL;
	ano.len = sizeof(realtime_ctrl);
	ano.buffer = &realtime_ctrl;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_optical_flow(u8 mode, u8 state, s8 dx_0, s8 dy_0, u8 quality)
{
	struct ano_protocol ano;
	struct ano_ptc_optical_flow optical_flow;
	u8 check_len, *check_buff;
	u8 i;

	optical_flow.mode = mode;
	optical_flow.state = state;
	optical_flow.dx_0 = dx_0;
	optical_flow.dy_0 = dy_0;
	optical_flow.quality = quality;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = OPTICAL_FLOW;
	ano.len = sizeof(optical_flow);
	ano.buffer = &optical_flow;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_optical_flow_f(u8 mode, u8 state, s16 dx_1, s16 dy_1, u8 quality)
{
	struct ano_protocol ano;
	struct ano_ptc_optical_flow_f optical_flow_f;
	u8 check_len, *check_buff;
	u8 i;

	optical_flow_f.mode = mode;
	optical_flow_f.state = state;
	optical_flow_f.dx_1 = dx_1;
	optical_flow_f.dy_1 = dy_1;
	optical_flow_f.quality = quality;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = OPTICAL_FLOW;
	ano.len = sizeof(optical_flow_f);
	ano.buffer = &optical_flow_f;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_optical_flow_isf(u8 mode, u8 state,
	s16 dx_2, s16 dy_2, s16 dx_fix, s16 dy_fix, s16 integ_x, s16 integ_y,
	u8 quality)
{
	struct ano_protocol ano;
	struct ano_ptc_optical_flow_isf optical_flow_isf;
	u8 check_len, *check_buff;
	u8 i;

	optical_flow_isf.mode = mode;
	optical_flow_isf.state = state;
	optical_flow_isf.dx_2 = dx_2;
	optical_flow_isf.dy_2 = dy_2;
	optical_flow_isf.dx_fix = dx_fix;
	optical_flow_isf.dy_fix = dy_fix;
	optical_flow_isf.integ_x = integ_x;
	optical_flow_isf.integ_y = integ_y;
	optical_flow_isf.quality = quality;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = OPTICAL_FLOW;
	ano.len = sizeof(optical_flow_isf);
	ano.buffer = &optical_flow_isf;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_read_waypoint(u8 num)
{
	struct ano_protocol ano;
	u8 check_len, *check_buff;
	u8 i;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = WAYPOINT_READING;
	ano.len = sizeof(num);
	ano.buffer = &num;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_waypoint(u8 num, float lat, float lng,
	s32 alt, u16 spd, u16 yaw, u8 fun,
	u8 cmd1, u8 cmd2, u8 cmd3, u8 cmd4)
{
	struct ano_protocol ano;
	struct ano_ptc_waypoint waypoint;
	u8 check_len, *check_buff;
	u8 i;

	waypoint.num = num;
	waypoint.lat = lat * 10000000;
	waypoint.lng = lng * 10000000;
	waypoint.alt = alt;
	waypoint.spd = spd;
	waypoint.yaw = yaw;
	waypoint.fun = fun;
	waypoint.cmd1 = cmd1;
	waypoint.cmd2 = cmd2;
	waypoint.cmd3 = cmd3;
	waypoint.cmd4 = cmd4;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = WAYPOINT_WRITING;
	ano.len = sizeof(waypoint);
	ano.buffer = &waypoint;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_cmd_frame(u8 cid,
	u8 cmd1, u8 cmd2, u8 cmd3, u8 cmd4,
	u8 cmd5, u8 cmd6, u8 cmd7, u8 cmd8, u8 cmd9)
{
	struct ano_protocol ano;
	struct ano_ptc_cmd_frame cmd_frame;
	u8 check_len, *check_buff;
	u8 i;

	cmd_frame.cid = cid;
	cmd_frame.cmd1 = cmd1;
	cmd_frame.cmd2 = cmd2;
	cmd_frame.cmd3 = cmd3;
	cmd_frame.cmd4 = cmd4;
	cmd_frame.cmd5 = cmd5;
	cmd_frame.cmd6 = cmd6;
	cmd_frame.cmd7 = cmd7;
	cmd_frame.cmd8 = cmd8;
	cmd_frame.cmd9 = cmd9;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = CMD_FRAME;
	ano.len = sizeof(cmd_frame);
	ano.buffer = &cmd_frame;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

enum ANO_PARA {
	ANO_NULL = 0,
	ANO_HW_TYPE,
	ANO_HW_VER,
	ANO_SW_VER,
	ANO_BL_VER = 4,
	ANO_INFO5,
	ANO_INFO6,
	ANO_INFO7,
	ANO_INFO8,
	ANO_INFO9,
	ANO_INFO10,
	ANO_PID_1_P =	11,
	ANO_PID_1_I,
	ANO_PID_1_D,
	ANO_PID_2_P,
	ANO_PID_2_I,
	ANO_PID_2_D,
	ANO_PID_3_P,
	ANO_PID_3_I,
	ANO_PID_3_D,
	ANO_PID_4_P,
	ANO_PID_4_I,
	ANO_PID_4_D,
	ANO_PID_5_P,
	ANO_PID_5_I,
	ANO_PID_5_D,
	ANO_PID_6_P,
	ANO_PID_6_I,
	ANO_PID_6_D,
	ANO_PID_7_P,
	ANO_PID_7_I,
	ANO_PID_7_D,
	ANO_PID_8_P,
	ANO_PID_8_I,
	ANO_PID_8_D,
	ANO_PID_9_P,
	ANO_PID_9_I,
	ANO_PID_9_D,
	ANO_PID_10_P,
	ANO_PID_10_I,
	ANO_PID_10_D,
	ANO_PID_11_P = 41,
	ANO_PID_11_I,
	ANO_PID_11_D,
	ANO_PID_12_P,
	ANO_PID_12_I,
	ANO_PID_12_D,
	ANO_PID_13_P,
	ANO_PID_13_I,
	ANO_PID_13_D,
	ANO_PID_14_P,
	ANO_PID_14_I,
	ANO_PID_14_D,
	ANO_PID_15_P,
	ANO_PID_15_I,
	ANO_PID_15_D,
	ANO_PID_16_P,
	ANO_PID_16_I,
	ANO_PID_16_D,
	ANO_PID_17_P,
	ANO_PID_17_I,
	ANO_PID_17_D,
	ANO_PID_18_P,
	ANO_PID_18_I,
	ANO_PID_18_D,
	ANO_PID_ONE_1,
	ANO_PID_ONE_2,
	ANO_PID_ONE_3,
	ANO_PID_MODE,
	ANO_RCINMODE = 71,
	ANO_UNLOCKPWM,
	ANO_UNLOCK_OO,
	ANO_AUTO_GYR_CAL,
	ANO_BAT_CELLS,
	ANO_LV_WARN_100,
	ANO_LV_RETN_100,
	ANO_LV_LAND_100,
	ANO_CENPOS_X,
	ANO_CENPOS_Y,
	ANO_CENPOS_Z,
	ANO_TAKEOFFHIGH,
	ANO_TAKEOFFSPEED,
	ANO_LANDSPEED,
	ANO_LANDSPEED_MAX,
	ANO_AUTO_LANDING,
	ANO_HEATSWITCH,
	ANO_HEAT_TMPER,
	ANO_MAX_SPEED_HOR,
	ANO_MAX_SPEED_PRC,
	ANO_MAX_SPEED_UP,
	ANO_MAX_SPEED_DW,
	ANO_MAX_SPEED_YAW,
	ANO_SAFE_ATT = 94,
	ANO_MAGMODE,
	ANO_ACANGVELMAX,
	ANO_YCANGVELMAX,
	ANO_YCANGACCMAX,
	ANO_RH_ALT,
	ANO_RH_VEL,
	ANO_GYR_FILTER = 151,
	ANO_ACC_FILTER,
	ANO_ATT_FUSION,
	ANO_MAG_FUSION,
	ANO_HOR_FUSION,
	ANO_VER_FUSION,
	ANO_FENCE_MODE,
	ANO_FENCE_RADIUS,
	ANO_FENCE_WIDTH_X,
	ANO_FENCE_WIDTH_Y,
	ANO_FENCE_HEIGHT,
	ANO_COM1_BAUD,
	ANO_COM2_BAUD,
	ANO_COM2_MODE,
	ANO_RGBOUT_ENA,
	ANO_DATAOUTTIME_01 = 300,
	ANO_DATAOUTTIME_02,
	ANO_DATAOUTTIME_03,
	ANO_DATAOUTTIME_04,
	ANO_DATAOUTTIME_05,
	ANO_DATAOUTTIME_06,
	ANO_DATAOUTTIME_07,
	ANO_DATAOUTTIME_08,
	ANO_DATAOUTTIME_09,
	ANO_DATAOUTTIME_0A,
	ANO_DATAOUTTIME_0B,
	ANO_DATAOUTTIME_0C,
	ANO_DATAOUTTIME_0D,
	ANO_DATAOUTTIME_0E,
	ANO_DATAOUTTIME_20,
	ANO_DATAOUTTIME_21,
	ANO_DATAOUTTIME_30,
	ANO_DATAOUTTIME_32,
	ANO_DATAOUTTIME_33,
	ANO_DATAOUTTIME_34,
	ANO_DATAOUTTIME_40,
	ANO_DATAOUTTIME_41,
	ANO_DATA_NUM,
	CAL_ACC_OFFSET_X = 1000,
	CAL_ACC_OFFSET_Y,
	CAL_ACC_OFFSET_Z,
	CAL_ACC_SENSIV_X,
	CAL_ACC_SENSIV_Y,
	CAL_ACC_SENSIV_Z,
	CAL_ACC_IEM_00,
	CAL_ACC_IEM_01,
	CAL_ACC_IEM_02,
	CAL_ACC_IEM_10,
	CAL_ACC_IEM_11,
	CAL_ACC_IEM_12,
	CAL_ACC_IEM_20,
	CAL_ACC_IEM_21,
	CAL_ACC_IEM_22,
	CAL_MAG_OFFSET_X,
	CAL_MAG_OFFSET_Y,
	CAL_MAG_OFFSET_Z,
	CAL_MAG_SENSIV_X,
	CAL_MAG_SENSIV_Y,
	CAL_MAG_SENSIV_Z,
};

int ano_send_parameter_read(u16 par_id)
{
	struct ano_protocol ano;
	struct ano_ptc_parameter_read parameter_read;
	u8 check_len, *check_buff;
	u8 i;

	parameter_read.par_id = par_id;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = PARAMETER_READ;
	ano.len = sizeof(parameter_read);
	ano.buffer = &parameter_read;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}

int ano_send_parameter_write(u16 par_id, s32 par_val)
{
	struct ano_protocol ano;
	struct ano_ptc_parameter_write parameter_write;
	u8 check_len, *check_buff;
	u8 i;

	parameter_write.par_id = par_id;
	parameter_write.par_val = par_val;

	ano.head = ANO_HEAD;
	ano.addr = ANO_NO_TARGET;
	ano.func_code = PARAMETER_WRITE;
	ano.len = sizeof(parameter_write);
	ano.buffer = &parameter_write;
	ano.check_sum = 0;
	ano.check_add = 0;

	check_len = sizeof(ano) -
		sizeof(ano.check_sum) - sizeof(ano.check_add) -
		sizeof(ano.buffer) + ano.len;
	check_buff = (u8 *)&ano;

	for (i = 0; i < check_len; i++) {
		ano.check_sum += check_buff[i];
		ano.check_add += ano.check_sum;
	}

	return 0;
}
