
#include <generated/deconfig.h>
#include <linux/slab.h>

struct fmu_pid_para {
	float p;
	float i;
	float d;
};

struct fmu_axis_ctrl {
	struct fmu_pid_para x;
	struct fmu_pid_para y;
	struct fmu_pid_para z;
};

struct fmu_para {
	/* fmu board position offset matrix */
	float iem[3][3];
	/* accelerometer, magnetometer, gravity and gyro zero bias */
	struct {
		float acc_x;
		float acc_y;
		float acc_z;
		float gyro_x;
		float gyro_y;
		float gyro_z;
		float mag_x;
		float mag_y;
		float mag_z;
		float gravity_x;
		float gravity_y;
		float gravity_z;
	} offset;
	/* 1G */
	float acc_sensitivity_ref[3];
	/* magnetometer calibration scale */
	float mag_gain_x;
	float mag_gain_y;
	float mag_gain_z;
	/* constant and derivative control */
	struct fmu_axis_ctrl attitude;
	struct fmu_axis_ctrl attitude_df;
	struct fmu_pid_para height;
	struct fmu_pid_para height_df;
	struct fmu_pid_para position;
	struct fmu_pid_para position_df;
	struct fmu_pid_para gps;
	struct fmu_pid_para gps_df;

	struct {
		float warn_voltage;
		float lowest_voltage;
		s32 cell;
	} battery;

	float auto_take_off_height;
	float auto_take_off_speed;
	float auto_landing_speed;
	float idle_speed_pwm;

	/* Receiver mode, PWM type/PPM type */
	unsigned pwm_in_mode: 1;
	unsigned heat_switch: 1;
	unsigned acc_calibrated: 1;
	unsigned mag_calibrated: 1;
	u8 reserve1;
	u8 reserve2;

	/*
	 * When the controller is initialized for the first time,
	 * some special work needs to be done,
	 * such as clearing the flash
	 */
	u8 frist_init;

	struct {
		u8 save_en;
		u8 save_trig;
		u16 time_delay;
	} state;
};

static int fmu_reset_parameter(void)
{
	return 0;
}

int fmu_init(void)
{
	struct fmu_para *para;
	int ret;

	para = kzalloc(sizeof(*para), GFP_KERNEL);
	if (para)
		return -ENOMEM;

	para->iem[0][0] = 1;
	para->iem[0][1] = 0;
	para->iem[0][2] = 0;
	para->iem[1][0] = 0;
	para->iem[1][1] = 1;
	para->iem[1][2] = 0;
	para->iem[2][0] = 0;
	para->iem[2][1] = 0;
	para->iem[2][2] = 1;

	para->acc_sensitivity_ref[0] = 1368;
	para->acc_sensitivity_ref[1] = 1368;
	para->acc_sensitivity_ref[2] = 1368;
	para->mag_gain_x = 200;
	para->mag_gain_y = 200;
	para->mag_gain_z = 200;

	para->heat_switch = false;

	para->battery.cell = 3;
	para->battery.warn_voltage = 3.50f;
	para->battery.lowest_voltage = 3.40f;

	para->auto_take_off_height = 0;
	para->auto_take_off_speed = 150;
	para->auto_landing_speed = 60;
	para->idle_speed_pwm = 20;
	
	ret = rc_check_sbus();
	if (ret)
		return 0;
	
	ret = fmu_check_esc();
	if (ret)
		return 0;
	
	ret = sensor_check_battery();
	if (ret)
		return 0;
	
	ret = sensor_check_magnetometer();
	if (ret)
		return 0;
	
	ret = sensor_check_pressure();
	if (ret)
		return 0;
	
	ret = sensor_check_imu();
	if (ret)
		return 0;
	
	ret = sensor_check_laser();
	if (ret)
		return 0;
	
	ret = sensor_check_optical_flow();
	if (ret)
		return 0;
	
	ret = sensor_check_gps();
	if (ret)
		return 0;
	
	ret = sensor_check_heat_res();
	if (ret)
		return 0;
	
	ret = sensor_check_ano_dt();
	if (ret)
		return 0;
	
	pr_info("All checker pass, you can fly now!");

	return 0;
}












