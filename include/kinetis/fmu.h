/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _FMU_H_
#define _FMU_H_

#include <linux/types.h>

struct fmu_parameter;

#include <kinetis/fmu_config.h>
#include <kinetis/pid.h>
#include <kinetis/tim-task.h>

struct fmu_pid_para {
	float p;
	float i;
	float d;
};

struct fmu_axis_ctrl {
	struct fmu_pid_para rol;
	struct fmu_pid_para pit;
	struct fmu_pid_para yaw;
};

struct fmu_axis_data {
	float x;
	float y;
	float z;
};

struct fmu_parameter {
	/* fmu board position offset matrix */
	float iem[3][3];
	/* accelelerometer, magnetometer, gravity and gyro zero bias */
	struct fmu_axis_data accel_offset;
	struct fmu_axis_data gyro_offset;
	struct fmu_axis_data magnet_offset;
	struct fmu_axis_data gravity_offset;
	/* 1G */
	struct fmu_axis_data accel_sensitivity_ref;
	/* magnetometer calibration scale */
	struct fmu_axis_data magnet_gain;
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
	unsigned accel_calibrated: 1;
	unsigned magnet_calibrated: 1;
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

struct fmu_rc_sbus {
	u8 head;
	unsigned channel_0: 11;
	unsigned channel_1: 11;
	unsigned channel_2: 11;
	unsigned channel_3: 11;
	unsigned channel_4: 11;
	unsigned channel_5: 11;
	unsigned channel_6: 11;
	unsigned channel_7: 11;
	unsigned channel_8: 11;
	unsigned channel_9: 11;
	unsigned channel_10: 11;
	unsigned channel_11: 11;
	unsigned channel_12: 11;
	unsigned channel_13: 11;
	unsigned channel_14: 11;
	unsigned channel_15: 11;
	u8 flag;
#define SBUS_CH17	BIT(7)
#define SBUS_CH18	BIT(6)
#define SBUS_FRAME_LOST	BIT(5)
#define SBUS_FAILSAFE	BIT(4)
	u8 tail;
} __packed;

struct fmu_status_instruction {
	unsigned all_module_done: 1;
	unsigned low_voltage: 1;
	unsigned reset_imu: 1;

	unsigned calibration_gyro: 1;
	unsigned calibration_accel: 1;
	unsigned calibration_magnet: 1;
	unsigned calibration_hori: 1;

	unsigned error_mpu: 1;
	unsigned error_magnet: 1;
	unsigned error_baro: 1;
	unsigned error_onetime: 1;

	unsigned invalid_rc: 1;
	unsigned optical_flow_status: 1;
	unsigned gps_status: 1;
	unsigned saving_para: 1;

	float brightness[FMU_LED_NUM];
#define FMU_B_LED	1
#define FMU_R_LED	2
#define FMU_G_LED	3
#define FMU_BIT_LED		0x00
#define FMU_BIT_XLED 	0x01
#define FMU_BIT_BLED 	0x02
#define FMU_BIT_RLED 	0x04
#define FMU_BIT_GLED 	0x08
#define FMU_BIT_WLED 	0x0e
#define FMU_BIT_PLED 	0x06
#define FMU_BIT_YLED 	0x0c
};

struct fmu_rc {
	u8 mode;
#define FMU_RC_MODE_NULL	0
#define FMU_RC_MODE_PPM		1
#define FMU_RC_MODE_SBUS	2

	bool lost_signal;
	bool fail_safe;
	u16 get_data_cnt;

	u16 channel_rol;
	u16 channel_pit;
	u16 channel_thr;
	u16 channel_yaw;
	u16 channel_aux1;
	u16 channel_aux2;
	u16 channel_aux3;
	u16 channel_aux4;
	u16 channel_aux5;
	u16 channel_aux6;

	struct tim_task conv_data_task;
	struct tim_task detect_task;

	struct fmu_rc_sbus sbus;
};

struct fmu_ahrs_data {
	struct fmu_axis_data accel;
	struct fmu_axis_data gyro;
	struct fmu_axis_data magnet;
};

struct space_coordinates {
	float x;
	float y;
	float z;
};

struct plane_coordinates {
	float x;
	float y;
};

struct fmu_core {
#define FMU_DIMENSION	3

	struct fmu_status_instruction led;

	struct fmu_parameter para;

	struct fmu_pid_ctrl ctrl;

	struct fmu_rc rc;

	struct device *bmi088_accel_dev;
	struct device *bmi088_gyro_dev;
	struct device *ak09911_dev;
	struct device *spl06_dev;

	struct fmu_axis_data accel_scale;
	struct fmu_axis_data gyro_scale;

	struct fmu_axis_data accel_cmpss;
	struct fmu_axis_data gyro_degree;
	struct fmu_axis_data accel_cmpss_nb;
	struct fmu_axis_data gyro_degree_nb;
	struct fmu_axis_data gyro_radian_nb;
	struct fmu_axis_data magnet;
	float height;

	struct fmu_ahrs_data ahrs;
#define AHRS_AVERAGE_CNT	1000
};

static inline void vector_x_matrix_t(float *in, float matrix[3][3], float *out)
{
	float temp;
	int i, j;

	for (i = 0; i < FMU_DIMENSION; i++) {
		for (j = 0, temp = 0; j < FMU_DIMENSION; j++) {
			temp += in[j] * matrix[i][j];
		}

		out[i] = temp;
	}
}

static inline void vector_x_matrix(float *in, float matrix[3][3], float *out)
{
	float temp;
	int i, j;

	for (i = 0; i < FMU_DIMENSION; i++) {
		for (j = 0, temp = 0; j < FMU_DIMENSION; j++) {
			temp += in[j] * matrix[j][i];
		}

		out[i] = temp;
	}
}

struct fmu_core *get_fmu_core(void);

int fmu_init(void);

void sbus_get_byte(struct fmu_rc *rc, u8 data);
int rc_module_init(struct fmu_rc *rc);
void rc_module_exit(struct fmu_rc *rc);

int imu_static_detect(float dt_s,
	float *accel_cmss_in, float *gyro_dps_in);
int imu_calculate_offset(struct fmu_axis_data *accel_offset,
	struct fmu_axis_data *gyro_offset);

int ahrs_init_container(u32 fifo_size);
void ahrs_fini_container(void);
int ahrs_container_size(void);
int ahrs_container_out(struct fmu_ahrs_data *output, u32 size);
int ahrs_container_in(struct fmu_ahrs_data *input, u32 size);

int bmi088_accel_get_all_axis(void);
int bmi088_gyro_get_all_axis(void);
int ak8975_get_all_axis(void);
int spl06_get_temp_and_pressure(void);

typedef struct {
	float out;
	float last_out;
	float a;
	float b;
	float a_c2;
	float b_c2;
	float c_c2;

	float k;
} _ano_filter_1_st;

typedef struct {
	float in_est;    //Estimator
	float in_obs;    //Observation

	float fix_ki;
	float ei_limit;

	float e;
	float ei;

	float out;
} _inte_fix_filter_st;

typedef struct {
	float in_est_d;   //Estimator
	float in_obs;    //Observation

	float fix_kp;
	float e_limit;

	float e;

	float out;
} _fix_inte_filter_st;

typedef struct {
	float lpf_1;

	float out;
} _lf_t;

typedef struct {
	float lpf_1;
	float lpf_2;
	float in_old;
	float out;
} _jldf_t;

typedef struct {
	float a;//q
	float b;

	s16 limit;
	float e_nr;

	float err_v;
	float out;
	float ei;
	float out_f;
} _filter_1_st;

typedef struct {
	u8 cnt;

	s32 lst_pow_sum;

	s32 now_out;
	s32 lst_out;
	s32 now_velocity_xdt;
} _steepest_st;

typedef struct Filter {
	float _cutoff_freq;
	float _a1;
	float _a2;
	float _b0;
	float _b1;
	float _b2;
	float _delay_element_1;        // buffered sample -1
	float _delay_element_2;        // buffered sample -2
} filter_s;

void inte_fix_filter(float dT, _inte_fix_filter_st *data);
void fix_inte_filter(float dT, _fix_inte_filter_st *data);

void filter_1(float, float, float, float, _filter_1_st *);

void limit_filter(float T, float hz, _lf_t *data, float in);
void limit_filter_2(float T, float hz, _lf_t *data, float in);
void limit_filter_3(float T, float hz, _lf_t *data, float in);

void init_low_pass2_fliter(float sample_freq, float cutoff_freq, filter_s *imu_filter);
float low_pass2_filter(float sample, filter_s *imu_filter);

void steepest_descend(s32 arr[], u8 len, _steepest_st *steepest, u8 step_num, s32 in);

void filter_1(float, float, float, float, _filter_1_st *);

void jyoun_limit_deadzone_filter(float T, float hz1, float hz2, _jldf_t *data, float in); //?????

void jyoun_filter(float dT, float hz, float ref_value, float exp, float fb, float *out);
//float Moving_Average(u8 item,u8 width_num,float in);
void Moving_Average(float moavarray[],//???? ????:len+1
	u16 len, //??????
	u16 *fil_cnt,//????????(??,????)
	float in,//??
	float *out //??
);

void step_filter(float step, float in, float *out);

void fir_arrange_filter(float *arr, u16 len, u8 *fil_cnt, float in, float *arr_out); //len<=255 len >= 3

#define LOW_PASS_FILTER(hz, t, in, out)	\
	((out) += (1 / (1 + 1 / ((hz) * 6.28f * (t)))) * ((in) - (out)))

#define S_LOW_PASS_FILTER(a, in, out)	\
	((out) += (a) * ((in) - (out)))

void LPF_1_db(float hz, float time, double in, double *out); //????,2hz??0.5???????0.7?,??1????90%

void LPF_I(float raw_a, float raw_b, float time, float in, float *out, float *intera);

float my_deadzone_3(float T, float hz, float x, float, float zoom, float range_x, float *zoom_adj); //range_x   0 ----- 1  *****

/*============ ???? ===============
?????
					x
					|
			y---z

???????,x?????,z??????

======================================*/
//void vec_3dh_transition(float ref[VEC_XYZ], float in[VEC_XYZ], float out[VEC_XYZ]);
//void vec_3dh_transition_matrix(float ref[VEC_XYZ], float wh_matrix[VEC_XYZ][VEC_XYZ]);

#define TAN_MAP_RES		0.003921569f     /* (smallest non-zero value in table) */
#define RAD_PER_DEG		0.017453293f
#define DEG_PER_RAD		57.29577951f
#define TAN_MAP_SIZE	256
#define MY_PPPIII		3.14159f
#define MY_PPPIII_HALF	1.570796f

#define my_sign(x) (((x) > 1e-6f) ? 1 : (((x) < -1e-6f) ? -1 : 0))

#define NORMALIZATION_3(x, y, z)	\
	float_sqrt(float_pow(x, 2) + float_pow(y, 2) + float_pow(z, 2))
#define NORMALIZATION_2(x, y)	\
	float_sqrt(float_pow(x, 2) + float_pow(y, 2))

#define SAFE_DIV(numerator, denominator, safe_value)	\
	((denominator == 0) ? (safe_value) : ((numerator) / (denominator)))

#define POW_2_CURVE(in, a, max)	\
	(((1.0f - (a)) + (a) * clamp(abs((in) / (max)), 0, 1)) * in)

#define RANGE_TO_180_DEGREES(a)	\
	((a) > 180 ? (a - 360) : ((a) < -180 ? (a + 360) : (a)))
#define TO_180_DEGREES	RANGE_TO_180_DEGREES

static inline float imu_abs(float f)
{
	return f >= 0.0f ? f : -f;
}

float fast_atan(float y, float x);

static inline float fast_tan(float x, float y)
{
	return fast_atan(y, x);
}

float float_pow(float base, unsigned int exp);
float float_sqrt_reciprocal(float number);

/**
 * float_sqrt - computes the integer square root
 * @x: integer of which to calculate the sqrt
 *
 * Computes: floor(sqrt(x))
 */
static inline float float_sqrt(float x)
{
	return x * float_sqrt_reciprocal(x);
}

double sinx(double rad);
double siny(double rad);
double cosinx(double rad);

float deadzone_1(float x, float, float zoom);
float deadzone_2(float x, float, float zoom);
float deadzone_p(float x, float zone);
float deadzone_n(float x, float zone);

double to_180_degrees_db(double x);
void length_limit(float in1, float in2, float limit, float *out);
float fifo(u8 arr_num, u8 *cnt, float *arr, float in);
float linear_interpolation(float *range, float *interpolation, float in);

void plane_vector_roate(float sinx,
	struct plane_coordinates *in, struct plane_coordinates *out);
float plane_vector_cross_product(struct plane_coordinates *in1,
	struct plane_coordinates *in2);
float plane_vector_dot_product(struct plane_coordinates *in1,
	struct plane_coordinates *in2);
void space_vector_cross_product_err_sinx(
	struct space_coordinates *in1, struct space_coordinates *in2,
	struct space_coordinates *out);
float space_vector_dot_product(struct space_coordinates *in1,
	struct space_coordinates *in2);

#endif /* _FMU_H_ */
