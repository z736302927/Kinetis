
#include <generated/deconfig.h>
#include <linux/kfifo.h>

#include <kinetis/fmu.h>

#define ACCEL_STATIC_THRESHOLD	25.0f
#define GYRO_STATIC_THRESHOLD	3.0f

int imu_static_detect(float dt_s,
	float *accel_cmss_in, float *gyro_dps_in)
{
	float accel[3], gyro[3];
	float accel_delta[3], gyro_delta[3];
	float accel_delta_length, gyro_delta_length;
	float hold_time_ms[2];
	int i, ret = 0;

	for (i = 0; i < 3; i++) {
		LOW_PASS_FILTER(2.0f, dt_s, accel_cmss_in[i], accel[i]);
		LOW_PASS_FILTER(2.0f, dt_s, gyro_dps_in[i], gyro[i]);

		accel_delta[i] = accel_cmss_in[i] - accel[i];
		gyro_delta[i] = gyro_dps_in[i]  - gyro[i];
	}

	accel_delta_length = NORMALIZATION_3(accel_delta[0], accel_delta[1], accel_delta[2]);
	gyro_delta_length = NORMALIZATION_3(gyro_delta[0], gyro_delta[1], gyro_delta[2]);

	if (accel_delta_length > ACCEL_STATIC_THRESHOLD) {
		hold_time_ms[0] = 0;
		ret &= ~0x01;
	} else {
		if (hold_time_ms[0] < 200) {
			hold_time_ms[0] += 1e3f * (dt_s);
		} else {
			ret |= 0x01;
		}
	}

	if (gyro_delta_length > GYRO_STATIC_THRESHOLD) {
		hold_time_ms[1] = 0;
		ret &= ~0x02;
	} else {
		if (hold_time_ms[1] < 200) {
			hold_time_ms[1] += 1e3f * (dt_s);
		} else {
			ret |= 0x02;
		}
	}

	return ret;
}

int imu_calculate_offset(struct fmu_axis_data *accel_offset,
	struct fmu_axis_data *gyro_offset)
{
	struct fmu_ahrs_data *ahrs, temp;
	u32 i;
	int ret;

	ahrs = kzalloc(sizeof(*ahrs) * AHRS_AVERAGE_CNT, GFP_KERNEL);
	if (ahrs) {
		return -ENOMEM;
	}

	ret = ahrs_container_out(ahrs, AHRS_AVERAGE_CNT);
	if (ret) {
		return ret;
	}

	memset(&temp, 0, sizeof(temp));

	for (i = 0; i < AHRS_AVERAGE_CNT; i++) {
		temp.accel.x += ahrs[i].accel.x;
		temp.accel.y += ahrs[i].accel.y;
		temp.accel.z += ahrs[i].accel.z;
		temp.gyro.x += ahrs[i].gyro.x;
		temp.gyro.y += ahrs[i].gyro.y;
		temp.gyro.z += ahrs[i].gyro.z;
	}

	//	accel_offset->x = temp.accel.x / AHRS_AVERAGE_CNT;
	//	accel_offset->y = temp.accel.y / AHRS_AVERAGE_CNT;
	//	accel_offset->z = temp.accel.z / AHRS_AVERAGE_CNT;

	gyro_offset->x = temp.gyro.x / AHRS_AVERAGE_CNT;
	gyro_offset->y = temp.gyro.y / AHRS_AVERAGE_CNT;
	gyro_offset->z = temp.gyro.z / AHRS_AVERAGE_CNT;

	return 0;
}

void imu_low_pass_filter(struct fmu_axis_data axis,
	struct fmu_axis_data *container)
{
#define LPF_CONTAINER_SIZE		5
#define LPF_DATA_COEF		0.22f
	int i, j;

	for (i = 0; i < 3; i++) {
		container[LPF_CONTAINER_SIZE - 1] = axis;

		for (j = 4; j > 0; j--) {
			container[j - 1].x += LPF_DATA_COEF *
				(container[j].x - container[j - 1].x);
			container[j - 1].y += LPF_DATA_COEF *
				(container[j].y - container[j - 1].y);
			container[j - 1].z += LPF_DATA_COEF *
				(container[j].z - container[j - 1].z);
		}
	}
}

struct fmu_axis_data magnet_data_process(u8 dt, struct fmu_axis_data magnet,
	float vector_z_z, float gyro_degree_x, float gyro_degree_z,
	struct fmu_parameter *para)
{
	float t_length;
	struct fmu_axis_data magnet_fix, magnet_fix_nb;

	magnet_fix.x = (magnet.x - para->magnet_offset.x) * 200 / para->magnet_gain.x;
	magnet_fix.y = (magnet.y - para->magnet_offset.y) * 200 / para->magnet_gain.y;
	magnet_fix.z = (magnet.z - para->magnet_offset.z) * 200 / para->magnet_gain.z;

	if (para->magnet_calibrated != 0) {
		/* 转换坐标轴为ANO坐标 */
		vector_x_matrix_t(&magnet_fix.x, para->iem, &magnet_fix_nb.x);

		return magnet_fix_nb;
	}

	return magnet;
}

//void Mag_Data_Deal_Task(u8 dt, struct fmu_axis_data *magnet,
//	float vector_z_z, float gyro_degree_x, float gyro_degree_z,
//	struct fmu_parameter *para)
//{
//	float rotate_angle;
//	struct fmu_axis_data max, min;
//	if (mag.mag_CALIBRATE == 1 && flag.unlock_sta == 0) {
//		switch (mag_cal_step) {
//		case 0://第一步，水平旋转
//			max.x = max(magnet->x, max.x);
//			max.y = max(magnet->y, max.y);
//			min.x = min(magnet->x, min.x);
//			min.y = min(magnet->y, min.y);

//			if (vector_z_z < 0.985f) { //+-10deg
////				LED_STA.calMag = 100;
//				mag_cal_step = 1;
//				rotate_angle = 0;
//			} else {
////				LED_STA.calMag = 1;
//				rotate_angle += dt * 1e-3f * gyro_degree_z; //角度积分，旋转360度

//				if (imu_abs(rotate_angle) > 360)
//					mag_cal_step = 2;
//			}
//			break;

//		case 1://error
//			Mag_Cal_Reset(2);
//			rotate_angle = 0;
//			mag_cal_step = 0;
//			break;

//		case 2://第二步，竖直旋转，机头朝下
////			LED_STA.calMag = 2;

//			if (vector_z_z < 0.1f) //5.7deg
//				mag_cal_step = 3;
//			break;

//		case 3:
//			mag.mag_CALIBRATE = 2;

//			max.z = max(magnet->z, max.z);
//			min.z = min(magnet->z, min.z);

//			if (vector_z_z > 0.17f) { //10deg
////				LED_STA.calMag = 2;
//				mag_cal_step = 4;
//				mag_cal_angle[1] = 0;
//			} else {
////				LED_STA.calMag = 3;
//				mag_cal_angle[1] += dt * 1e-3f * (gyro_deg_x);	//角度积分，旋转360度

//				if (ABS(mag_cal_angle[1]) > 360) {
//					mag_cal_angle[1] = 0;
//					mag_cal_step = 5;
//				}
//			}
//			break;

//		case 4://error_2，重新开始竖直旋转
//			Mag_Cal_Reset(1);
//			mag_cal_angle[1] = 0;
//			mag_cal_step = 2;
//			break;

//		case 5:
//			for (u8 i = 0; i < 3; i++) {
//				Ano_Parame.set.mag_offset[i] = 0.5f * (max_t[i] + min_t[i]);		//中值校准
//				Ano_Parame.set.mag_gain[i] = 0.5f * (max_t[i] - min_t[i]);		//幅值校准
//			}

//			Mag_Cal_Reset(3);
//			rotate_angle = mag_cal_angle[1] = 0;
//			mag_cal_step = 0;
//			mag.mag_CALIBRATE = 0;
//			LED_STA.calMag = 0;

//			data_save();//保存数据
//			break;

//		default:
//			break;
//		}

//		if (mag_cal_step == 0 || mag_cal_step == 3) {
//			//长时间出错，退出校准逻辑
//			if (cali_cnt < 15000)
//				cali_cnt += dt;
//			else { ////校准错误
//				LED_STA.errOneTime = 1;
//				cali_cnt = 0;
//				LED_STA.calMag = 0;
//				mag.mag_CALIBRATE = 0;
//			}
//		} else
//			cali_cnt = 0;
//	} else {
//		mag_cal_step = 0;

//		t_length = my_3_norm(mag.val[X], mag.val[Y], mag.val[Z]);

//		if (t_length < 150 || t_length > 350) {
//			//state[3] |= (1<<3);//罗盘严重干扰
//			//LED_state = 6;
//		} else {
//			//state[3] &= ~(1<<3);//罗盘无严重干扰

////			if(LED_state == 6)
////			{
////				LED_state = 0;
////			}
//		}
//	}
//}

/* fifo size in elements (ints) */
static DECLARE_KFIFO_PTR(ahrs_data, struct fmu_ahrs_data);

int ahrs_init_container(u32 fifo_size)
{
	int ret;

	ret = kfifo_alloc(&ahrs_data, fifo_size, GFP_KERNEL);
	if (ret) {
		printk(KERN_ERR "error init container\n");
		return ret;
	}

	return 0;
}

void ahrs_fini_container(void)
{
	kfifo_free(&ahrs_data);
}

int ahrs_container_size(void)
{
	return kfifo_len(&ahrs_data);
}

int ahrs_container_out(struct fmu_ahrs_data *output, u32 size)
{
	int ret;

	if (kfifo_len(&ahrs_data) < size) {
		return -ERANGE;
	}

	/* get max of 2 elements from the fifo */
	ret = kfifo_out(&ahrs_data, output, size);
	if (ret < size) {
		pr_err("Can't get %u data, only %u\n", size, ret);
		return -ENODATA;
	}

	return 0;
}

int ahrs_container_in(struct fmu_ahrs_data *input, u32 size)
{
	int	ret;

	/* put values into the fifo */
	ret = kfifo_in(&ahrs_data, input, size);
	if (!ret) {
		return -ERANGE;
	}

	return 0;
}

/* Reference coordinate, defined as ano coordinate, top view,
 * nose direction is x positive direction
 *       +x
 *		  ^
 *        |
 *  +y <--|--
 *        |
 *  */

/* World coordinate plane XY to plane heading coordinate XY */
void world_to_hori_2d(struct space_coordinates *w,
	struct plane_coordinates *ref_ax, struct plane_coordinates *h)
{
	h->x = w->x * ref_ax->x + w->y * ref_ax->y;
	h->y = w->y * ref_ax->x + w->x * ref_ax->y;
}

/* Plane heading coordinate XY to world coordinate plane XY */
void hori_to_world_2d(struct plane_coordinates *h,
	struct plane_coordinates *ref_ax, struct space_coordinates *w)
{
	w->x = h->x * ref_ax->x - h->y * ref_ax->y;
	w->y = h->x * ref_ax->y + h->y * ref_ax->x;
}

/* The carrier coordinates are converted to world coordinates
 * (ANO convention is equivalent to geographic coordinates),
 * and the matrix must be calculated by attitude calculation */
float attitude_matrix[FMU_DIMENSION][FMU_DIMENSION];

struct imu {
	float w;//q0;
	float x;//q1;
	float y;//q2;
	float z;//q3;

	struct space_coordinates vector_x;
	struct space_coordinates vector_y;
	struct space_coordinates vector_z;
	struct plane_coordinates hori_vector;

	struct space_coordinates airframe_accel;
	struct space_coordinates world_accel;
	struct plane_coordinates hori_accel;

	struct space_coordinates world_magnet;
	struct space_coordinates airframe_magnet;

	struct space_coordinates gaccel_deadzone;

	struct space_coordinates world_observe_accel;
	struct space_coordinates airframe_observe_accel;
	struct space_coordinates gravity_accel;

	struct plane_coordinates est_accel_airframe;
	struct plane_coordinates est_accel_hori;
	struct plane_coordinates est_accel_world;
	struct plane_coordinates est_speed_h;
	struct plane_coordinates est_speed_world;

	struct space_coordinates vector_err_integral;

	u16 reset_cnt;

	float rol;
	float pit;
	float yaw;
};

struct imu_state {
	float gkp;
	float gki;

	float mkp;
	float drag_p;

	u8 gravity_rst;
	u8 magnet_rst;
	u8 gravity_fix;
	u8 magnet_fix;

	u8 observe_enable;
};

struct imu imu_data =  {
	1
};

struct imu_state imu_state = {1, 1, 1, 1, 1, 1, 1, 1};

/* In the magnetic geographic coordinates, the direction of the magnetic field
 * in the horizontal plane is always north - south(1, 0) */
struct plane_coordinates normalized_magnet[2] = {
	{
		.x = 1,
		.y = 0
	},
	{
		.x = 1,
		.y = 0
	},
};

float imu_test[3];

void imu_update(struct imu_state *state, struct imu *imu, float dt,
	struct fmu_axis_data *gyro,
	struct fmu_axis_data *accel,
	struct fmu_axis_data *magnet)
{
	static float kp_use = 0, ki_use = 0, mkp_use = 0;
	float q0q0, q0q1, q0q2, q1q1, q1q3, q2q2, q2q3, q3q3, q1q2, q0q3;
	float normalized_accel_coef_r, normalized_accel_coef,
		  normalized_magnet_coef, normalized_q;
	struct space_coordinates normalized_accel;
	struct space_coordinates d_angle;
	float hori_vector_coef;
	struct space_coordinates vector_err, vector_err_integral;
	struct space_coordinates airframe_magnet;
	float magnet_yaw_err, magnet_dot_x_err;
	float imu_reset_val;
	float temp;
	static u16 reset_cnt;
	int i, j;

	q0q0 = imu->w * imu->w;
	q0q1 = imu->w * imu->x;
	q0q2 = imu->w * imu->y;
	q1q1 = imu->x * imu->x;
	q1q3 = imu->x * imu->z;
	q2q2 = imu->y * imu->y;
	q2q3 = imu->y * imu->z;
	q3q3 = imu->z * imu->z;
	q1q2 = imu->x * imu->y;
	q0q3 = imu->w * imu->z;

	if (state->observe_enable) {
		/*
		 * Computed motion acceleration observations in body coordinates.
		 * The coordinate system is northwest sky
		 */
		vector_x_matrix(&imu->world_observe_accel.x,
			attitude_matrix, &imu->airframe_observe_accel.x);
		imu->gravity_accel.x = accel->x - imu->airframe_observe_accel.x;
		imu->gravity_accel.y = accel->y - imu->airframe_observe_accel.y;
		imu->gravity_accel.z = accel->z - imu->airframe_observe_accel.z;
	} else {
		imu->gravity_accel.x = accel->x;
		imu->gravity_accel.y = accel->y;
		imu->gravity_accel.z = accel->z;
	}

	normalized_accel_coef = float_sqrt_reciprocal(
			float_pow(imu->gravity_accel.x, 2) +
			float_pow(imu->gravity_accel.y, 2) +
			float_pow(imu->gravity_accel.z, 2));
	normalized_accel_coef_r = SAFE_DIV(1, normalized_accel_coef, 0);

	/* Normalized accelerometer. */
	normalized_accel.x = imu->gravity_accel.x * normalized_accel_coef;
	normalized_accel.y = imu->gravity_accel.y * normalized_accel_coef;
	normalized_accel.z = imu->gravity_accel.z * normalized_accel_coef;

	/* The x-direction vector in carrier coordinates, normalized. */
	imu->vector_x.x = 1 - (2 * q2q2 + 2 * q3q3);
	imu->vector_x.y = 2 * q1q2 - 2 * q0q3;
	imu->vector_x.z = 2 * q1q3 + 2 * q0q2;

	/* The y-direction vector in carrier coordinates, normalized. */
	imu->vector_y.x = 2 * q1q2 + 2 * q0q3;
	imu->vector_y.y = 1 - (2 * q1q1 + 2 * q3q3);
	imu->vector_y.z = 2 * q2q3 - 2 * q0q1;

	/* The z-direction vector (equivalent gravity vector,
	 * gravitational acceleration vector) in the carrier coordinates,
	 * normalized. */
	imu->vector_z.x = 2 * q1q3 - 2 * q0q2;
	imu->vector_z.y = 2 * q2q3 + 2 * q0q1;
	imu->vector_z.z = 1 - (2 * q1q1 + 2 * q2q2);

	attitude_matrix[0][0] = imu->vector_x.x;
	attitude_matrix[0][1] = imu->vector_x.y;
	attitude_matrix[0][2] = imu->vector_x.z;
	attitude_matrix[1][0] = imu->vector_y.x;
	attitude_matrix[1][1] = imu->vector_y.y;
	attitude_matrix[1][2] = imu->vector_y.z;
	attitude_matrix[2][0] = imu->vector_z.x;
	attitude_matrix[2][1] = imu->vector_z.y;
	attitude_matrix[2][2] = imu->vector_z.z;

	/* horizontal direction vector */
	hori_vector_coef = float_sqrt_reciprocal(
			float_pow(attitude_matrix[0][0], 2) +
			float_pow(attitude_matrix[1][0], 2));
	imu->hori_vector.x = attitude_matrix[0][0] * hori_vector_coef;
	imu->hori_vector.y = attitude_matrix[1][0] * hori_vector_coef;

	/* Calculate the motion acceleration in carrier coordinates.
	 * (Not related to attitude solution) */
	imu->airframe_accel.x = (s32)(accel->x - 981 * imu->vector_z.x);
	imu->airframe_accel.y = (s32)(accel->y - 981 * imu->vector_z.y);
	imu->airframe_accel.z = (s32)(accel->z - 981 * imu->vector_z.z);

	/* Calculates the motion acceleration in world coordinates.
	 * The coordinate system is northwest sky */
	vector_x_matrix_t(&imu->airframe_accel.x, attitude_matrix, &imu->world_accel.x);

	world_to_hori_2d(&imu->world_accel, &imu_data.hori_vector, &imu->hori_accel);

	/* Cross product of the measured value with the equivalent gravity vector
	 * (calculated vector error). */
	vector_err.x = (normalized_accel.y * imu->vector_z.z -
			imu->vector_z.y * normalized_accel.z);
	vector_err.y = -(normalized_accel.x * imu->vector_z.z -
			imu->vector_z.x * normalized_accel.z);
	vector_err.z = -(normalized_accel.y * imu->vector_z.x -
			imu->vector_z.y * normalized_accel.x);

#ifdef USE_MAGNET
	/* The electronic compass is assigned as a float vector */
	airframe_magnet.x = magnet->x;
	airframe_magnet.y = magnet->y;
	airframe_magnet.z = magnet->z;

	if (magnet->x != 0 || magnet->y != 0 || magnet->z != 0) {
		/* Convert compass data in carrier coordinates to geographic coordinates */
		vector_x_matrix_t(&airframe_magnet.x, attitude_matrix, &imu->world_magnet.x);
		/* Calculates the direction vector normalization coefficient
		 * (inverse of the modulus) */
		normalized_magnet_coef = float_sqrt_reciprocal(
				float_pow(imu->world_magnet.x, 2) +
				float_pow(imu->world_magnet.y, 2));
		/* Calculate the north-south orientation vector */
		normalized_magnet[1].x = imu->world_magnet.x * normalized_magnet_coef;
		normalized_magnet[1].y = imu->world_magnet.y * normalized_magnet_coef;
		/* Calculate the north-south orientation error (cross product),
		 * in geographic coordinates, the horizontal magnetic field direction
		 * vector should always be north-south (1,0) */
		magnet_yaw_err = plane_vector_cross_product(&normalized_magnet[1],
				&normalized_magnet[0]);
		/* Calculate the dot product of the north-south direction vector to
		 * determine the same direction or the opposite direction. */
		magnet_dot_x_err = plane_vector_dot_product(&normalized_magnet[1],
				&normalized_magnet[0]);

		/* If it is reversed, directly give the maximum error */
		if (magnet_dot_x_err < 0) {
			magnet_yaw_err = my_sign(magnet_yaw_err) * 1.0f;
		}
	}
#endif

#ifdef USE_EST_DEADZONE
	if (state->gravity_rst == 0 && state->observe_enable == 0) {
		vector_err.x = my_deadzone(vector_err.x, 0, imu->gaccel_deadzone.x);
		vector_err.y = my_deadzone(vector_err.y, 0, imu->gaccel_deadzone.y);
		vector_err.z = my_deadzone(vector_err.z, 0, imu->gaccel_deadzone.z);
	}
#endif

#ifdef USE_LENGTH_LIM
	if (normalized_accel_coef_r > 1060 || normalized_accel_coef_r < 900) {
		vector_err.x = 0;
		vector_err.y = 0;
		vector_err.z = 0;
	}
#endif
	/* Error integral */
	vector_err_integral.x += clamp(vector_err.x, -0.1f, 0.1f) * dt * ki_use;
	vector_err_integral.y += clamp(vector_err.y, -0.1f, 0.1f) * dt * ki_use;
	vector_err_integral.z += clamp(vector_err.z, -0.1f, 0.1f) * dt * ki_use;

	/* Construct incremental rotation (with blend correction). */
	//		d_angle.x = (gyro.x + (vector_err.x + vector_err_integral.x) * kp_use -
	//				magnet_yaw_err * imu->vector_z.x * kmp_use * RAD_PER_DEG) * dT / 2;
	//		d_angle.y = (gyro.y + (vector_err.y + vector_err_integral.y) * kp_use -
	//				magnet_yaw_err * imu->vector_z.y * kmp_use * RAD_PER_DEG) * dT / 2;
	//		d_angle.z = (gyro.z + (vector_err.z + vector_err_integral.z) * kp_use -
	//				magnet_yaw_err * imu->vector_z.z * kmp_use * RAD_PER_DEG) * dT / 2;

#ifdef USE_MAGNET
	d_angle.x = (gyro->x + (vector_err.x + vector_err_integral.x) * kp_use + magnet_yaw_err * imu->vector_z.x * mkp_use) * dt / 2;
	d_angle.y = (gyro->y + (vector_err.y + vector_err_integral.y) * kp_use + magnet_yaw_err * imu->vector_z.y * mkp_use) * dt / 2;
	d_angle.z = (gyro->z + (vector_err.z + vector_err_integral.z) * kp_use + magnet_yaw_err * imu->vector_z.z * mkp_use) * dt / 2;
#else
	d_angle.x = (gyro->x + (vector_err.x + vector_err_integral.x) * kp_use) * dT / 2 ;
	d_angle.y = (gyro->y + (vector_err.y + vector_err_integral.y) * kp_use) * dT / 2 ;
	d_angle.z = (gyro->z + (vector_err.z + vector_err_integral.z) * kp_use) * dT / 2 ;
#endif

	/* Calculate attitude. */
	imu->w = imu->w - imu->x * d_angle.x - imu->y * d_angle.y - imu->z * d_angle.z;
	imu->x = imu->x + imu->w * d_angle.x + imu->y * d_angle.z - imu->z * d_angle.y;
	imu->y = imu->y + imu->w * d_angle.y - imu->x * d_angle.z + imu->z * d_angle.x;
	imu->z = imu->z + imu->w * d_angle.z - imu->y * d_angle.x + imu->x * d_angle.y;

	normalized_q = float_sqrt_reciprocal(
			imu->w * imu->w +
			imu->x * imu->x +
			imu->y * imu->y +
			imu->z * imu->z);
	imu->w *= normalized_q;
	imu->x *= normalized_q;
	imu->y *= normalized_q;
	imu->z *= normalized_q;

	/* Enable Correction */
#ifdef USE_MAGNET
	//magnetic force
	if (state->magnet_fix == 0) {
		//not corrected
		//Compass correction does not reset, clear reset flag
		mkp_use = 0;
		state->magnet_rst = 0;
	} else {
		if (state->magnet_rst) {
			/* Align by Increment */
			mkp_use = 10.0f;
			/* When the error is less than 2, clear the reset flag */
			if (magnet_yaw_err != 0 && imu_abs(magnet_yaw_err) < 0.01f) {
				state->magnet_rst = 0;
			}
		} else { //normal correction
			mkp_use = state->mkp;
		}
	}
#endif
	/* Gravity direction correction */
	if (state->gravity_fix == 0) {
		//not corrected
		kp_use = 0;
	} else {
		//normal correction
		if (state->gravity_rst == 0) {
			kp_use = state->gkp;
			ki_use = state->gki;
		} else {
			//Quick correction, alignment by increments
			kp_use = 10.0f;
			ki_use = 0.0f;
			//			imu->est_speed_world.x = 0;
			//			imu->est_speed_world.y = 0;
			//			imu->est_accel_world.x = 0;
			//			imu->est_accel_world.y = 0;
			//			imu->est_accel_hori.x = 0;
			//			imu->est_accel_hori.y = 0;

			//Calculate whether the static error is reduced
			imu_reset_val = imu_abs(vector_err.x) + imu_abs(vector_err.y);
			imu_reset_val = clamp(imu_reset_val, 0.0f, 1.0f);

			//			if (imu_reset_val < 0.02f && !state->magnet_rst && st_imuData.data_sta != 0) {
			//				/* start timer */
			//				reset_cnt += 2;

			//				if (reset_cnt > 400) {
			//					//Aligned, clear reset mark
			//					reset_cnt = 0;
			//					state->gravity_rst = 0;
			//				}
			//			} else
			//				reset_cnt = 0;
		}
	}
}

void calculate_rpy(void)
{
	/* output attitude angle, Operations that avoid singularities */
	if (imu_abs(imu_data.vector_z.z) > 0.05f) {
		imu_data.pit =  fast_atan(attitude_matrix[2][0],
				float_sqrt(clamp(1.0f - float_pow(attitude_matrix[2][0], 2), 0.0f, 1.0f))) * 57.30f;
		imu_data.rol =  fast_atan(attitude_matrix[2][1],
				attitude_matrix[2][2]) * 57.30f;
		imu_data.yaw = -fast_atan(attitude_matrix[1][0],
				attitude_matrix[0][0]) * 57.30f;
	}
}

static const float fast_atan_table[] = {
	0.000000e+00, 3.921549e-03, 7.842976e-03, 1.176416e-02,
	1.568499e-02, 1.960533e-02, 2.352507e-02, 2.744409e-02,
	3.136226e-02, 3.527947e-02, 3.919560e-02, 4.311053e-02,
	4.702413e-02, 5.093629e-02, 5.484690e-02, 5.875582e-02,
	6.266295e-02, 6.656816e-02, 7.047134e-02, 7.437238e-02,
	7.827114e-02, 8.216752e-02, 8.606141e-02, 8.995267e-02,
	9.384121e-02, 9.772691e-02, 1.016096e-01, 1.054893e-01,
	1.093658e-01, 1.132390e-01, 1.171087e-01, 1.209750e-01,
	1.248376e-01, 1.286965e-01, 1.325515e-01, 1.364026e-01,
	1.402496e-01, 1.440924e-01, 1.479310e-01, 1.517652e-01,
	1.555948e-01, 1.594199e-01, 1.632403e-01, 1.670559e-01,
	1.708665e-01, 1.746722e-01, 1.784728e-01, 1.822681e-01,
	1.860582e-01, 1.898428e-01, 1.936220e-01, 1.973956e-01,
	2.011634e-01, 2.049255e-01, 2.086818e-01, 2.124320e-01,
	2.161762e-01, 2.199143e-01, 2.236461e-01, 2.273716e-01,
	2.310907e-01, 2.348033e-01, 2.385093e-01, 2.422086e-01,
	2.459012e-01, 2.495869e-01, 2.532658e-01, 2.569376e-01,
	2.606024e-01, 2.642600e-01, 2.679104e-01, 2.715535e-01,
	2.751892e-01, 2.788175e-01, 2.824383e-01, 2.860514e-01,
	2.896569e-01, 2.932547e-01, 2.968447e-01, 3.004268e-01,
	3.040009e-01, 3.075671e-01, 3.111252e-01, 3.146752e-01,
	3.182170e-01, 3.217506e-01, 3.252758e-01, 3.287927e-01,
	3.323012e-01, 3.358012e-01, 3.392926e-01, 3.427755e-01,
	3.462497e-01, 3.497153e-01, 3.531721e-01, 3.566201e-01,
	3.600593e-01, 3.634896e-01, 3.669110e-01, 3.703234e-01,
	3.737268e-01, 3.771211e-01, 3.805064e-01, 3.838825e-01,
	3.872494e-01, 3.906070e-01, 3.939555e-01, 3.972946e-01,
	4.006244e-01, 4.039448e-01, 4.072558e-01, 4.105574e-01,
	4.138496e-01, 4.171322e-01, 4.204054e-01, 4.236689e-01,
	4.269229e-01, 4.301673e-01, 4.334021e-01, 4.366272e-01,
	4.398426e-01, 4.430483e-01, 4.462443e-01, 4.494306e-01,
	4.526070e-01, 4.557738e-01, 4.589307e-01, 4.620778e-01,
	4.652150e-01, 4.683424e-01, 4.714600e-01, 4.745676e-01,
	4.776654e-01, 4.807532e-01, 4.838312e-01, 4.868992e-01,
	4.899573e-01, 4.930055e-01, 4.960437e-01, 4.990719e-01,
	5.020902e-01, 5.050985e-01, 5.080968e-01, 5.110852e-01,
	5.140636e-01, 5.170320e-01, 5.199904e-01, 5.229388e-01,
	5.258772e-01, 5.288056e-01, 5.317241e-01, 5.346325e-01,
	5.375310e-01, 5.404195e-01, 5.432980e-01, 5.461666e-01,
	5.490251e-01, 5.518738e-01, 5.547124e-01, 5.575411e-01,
	5.603599e-01, 5.631687e-01, 5.659676e-01, 5.687566e-01,
	5.715357e-01, 5.743048e-01, 5.770641e-01, 5.798135e-01,
	5.825531e-01, 5.852828e-01, 5.880026e-01, 5.907126e-01,
	5.934128e-01, 5.961032e-01, 5.987839e-01, 6.014547e-01,
	6.041158e-01, 6.067672e-01, 6.094088e-01, 6.120407e-01,
	6.146630e-01, 6.172755e-01, 6.198784e-01, 6.224717e-01,
	6.250554e-01, 6.276294e-01, 6.301939e-01, 6.327488e-01,
	6.352942e-01, 6.378301e-01, 6.403565e-01, 6.428734e-01,
	6.453808e-01, 6.478788e-01, 6.503674e-01, 6.528466e-01,
	6.553165e-01, 6.577770e-01, 6.602282e-01, 6.626701e-01,
	6.651027e-01, 6.675261e-01, 6.699402e-01, 6.723452e-01,
	6.747409e-01, 6.771276e-01, 6.795051e-01, 6.818735e-01,
	6.842328e-01, 6.865831e-01, 6.889244e-01, 6.912567e-01,
	6.935800e-01, 6.958943e-01, 6.981998e-01, 7.004964e-01,
	7.027841e-01, 7.050630e-01, 7.073330e-01, 7.095943e-01,
	7.118469e-01, 7.140907e-01, 7.163258e-01, 7.185523e-01,
	7.207701e-01, 7.229794e-01, 7.251800e-01, 7.273721e-01,
	7.295557e-01, 7.317307e-01, 7.338974e-01, 7.360555e-01,
	7.382053e-01, 7.403467e-01, 7.424797e-01, 7.446045e-01,
	7.467209e-01, 7.488291e-01, 7.509291e-01, 7.530208e-01,
	7.551044e-01, 7.571798e-01, 7.592472e-01, 7.613064e-01,
	7.633576e-01, 7.654008e-01, 7.674360e-01, 7.694633e-01,
	7.714826e-01, 7.734940e-01, 7.754975e-01, 7.774932e-01,
	7.794811e-01, 7.814612e-01, 7.834335e-01, 7.853983e-01,
	7.853983e-01
};

float fast_atan(float y, float x)
{
	float x_abs, y_abs, z;
	float alpha, angle, base_angle;
	int index;

	/* don't divide by zero! */
	if (y == 0.0f && x == 0.0f) {
		angle = 0.0f;
	} else {
		/* normalize to +/- 45 degree range */
		y_abs = imu_abs(y);
		x_abs = imu_abs(x);

		/* z = (y_abs < x_abs ? y_abs / x_abs : x_abs / y_abs); */
		if (y_abs < x_abs) {
			z = y_abs / x_abs;
		} else {
			z = x_abs / y_abs;
		}

		/* when ratio approaches the table resolution, the angle is
		 * best approximated with the argument itself...
		 */
		if (z < TAN_MAP_RES) {
			base_angle = z;
		} else {
			/* find index and interpolation value */
			alpha = z * (float) TAN_MAP_SIZE - .5f;
			index = (int) alpha;
			alpha -= (float) index;
			/* determine base angle based on quadrant and
			 * add or subtract table value from base angle based on quadrant
			 */
			base_angle = fast_atan_table[index];
			base_angle += (fast_atan_table[index + 1] - fast_atan_table[index]) * alpha;
		}

		if (x_abs > y_abs) {
			/* -45 -> 45 or 135 -> 225 */
			if (x >= 0.0f) {
				/* -45 -> 45 */
				if (y >= 0.0f) {
					angle = base_angle;    /* 0 -> 45, angle OK */
				} else {
					angle = -base_angle;    /* -45 -> 0, angle = -angle */
				}
			} else {
				/* 135 -> 180 or 180 -> -135 */
				angle = 3.14159265358979323846;

				if (y >= 0.0f) {
					angle -= base_angle;    /* 135 -> 180, angle = 180 - angle */
				} else {
					angle = base_angle - angle;    /* 180 -> -135, angle = angle - 180 */
				}
			}
		} else {
			/* 45 -> 135 or -135 -> -45 */
			if (y >= 0.0f) {
				/* 45 -> 135 */
				angle = 1.57079632679489661923;

				if (x >= 0.0f) {
					angle -= base_angle;    /* 45 -> 90, angle = 90 - angle */
				} else {
					angle += base_angle;    /* 90 -> 135, angle = 90 + angle */
				}
			} else {
				/* -135 -> -45 */
				angle = -1.57079632679489661923;

				if (x >= 0.0f) {
					angle += base_angle;    /* -90 -> -45, angle = -90 + angle */
				} else {
					angle -= base_angle;    /* -135 -> -90, angle = -90 - angle */
				}
			}
		}
	}

#ifdef ZERO_TO_TWOPI
	if (angle < 0) {
		return angle + TWOPI;
	} else {
		return angle;
	}
#else
	return angle;
#endif
}

/**
 * float_pow - computes the exponentiation of the given base and exponent
 * @base: base which will be raised to the given power
 * @exp: power to be raised to
 *
 * Computes: pow(base, exp), i.e. @base raised to the @exp power
 */
float float_pow(float base, unsigned int exp)
{
	float result = 1;

	while (exp) {
		if (exp & 1) {
			result *= base;
		}
		exp >>= 1;
		base *= base;
	}

	return result;
}
EXPORT_SYMBOL_GPL(float_pow);

float float_sqrt_reciprocal(float number)
{
	float x, y;
	long i;

	x = number * 0.5f;
	y = number;
	i = *(long *)&y;
	i = 0x5f3759df - (i >> 1);

	y = *(float *)&i;
	y *= 1.5f - x * y * y;
	y *= 1.5f - x * y * y;

	return y;
}

#define ONE_PI   (3.14159265)
#define TWO_PI   (2.0 * 3.14159265)
#define ANGLE_UNIT (TWO_PI/10.0)

double sinx(double rad)
{
	double sine;

	if (rad < 0) {
		sine = rad * (1.27323954f + 0.405284735f * rad);
	} else {
		sine = rad * (1.27323954f - 0.405284735f * rad);
	}

	if (sine < 0) {
		sine = sine * (-0.225f * (sine + 1) + 1);
	} else {
		sine = sine * (0.225f * (sine - 1) + 1);
	}

	return sine;
}

double siny(double rad)
{
	s8 flag = 1;

	if (rad >= ONE_PI) {
		rad -= ONE_PI;
		flag = -1;
	}

	return sinx(rad) * flag;
}

double cosinx(double rad)
{
	s8 flag = 1;
	rad += ONE_PI / 2.0;

	if (rad >= ONE_PI) {
		flag = -1;
		rad -= ONE_PI;
	}

	return siny(rad) * flag;
}

float deadzone_1(float x, float ref, float zoom)
{
	float t;

	if (x > ref) {
		t = x - zoom;

		if (t < ref) {
			t = ref;
		}
	} else {
		t = x + zoom;

		if (t > ref) {
			t = ref;
		}
	}

	return (t);
}

float deadzone_2(float x, float ref, float zoom)
{
	float t;

	if (x > (-zoom + ref) && x < (zoom + ref)) {
		t = ref;
	} else {
		t = x;
	}

	return (t);
}

float high_pass_filter(float T, float hz, float x,
	float zoom, float range, float *zoom_adj)
{
	if (imu_abs(x) < 0.5f * range * zoom) {
		hz *= 1.2f;
	} else if (imu_abs(x) < range * zoom) {
		hz *= 0.8f;
	}

	else if (imu_abs(x) < zoom) {
		hz *= 0.5f;
	} else if (imu_abs(x) < 2 * zoom) {
		hz *= 0.2f;
	} else {
		hz *= 0.1f;
	}

	*zoom_adj += (1 / (1 + 1 / (hz * 6.28f * T))) * (x - *zoom_adj);
	*zoom_adj = clamp(*zoom_adj, -range * zoom, range * zoom);

	return x - *zoom_adj;
}

double to_180_degrees_db(double x)
{
	return x > 180 ? (x - 360) : (x < -180 ? (x + 360) : x);
}

void length_limit(float in1, float in2, float limit, float *out)
{
	float l = NORMALIZATION_2(in1, in2);
	float l_limit = clamp(l, 0.0f, limit);

	if (l == 0) {
		out[0] = 0;
		out[1] = 0;
	} else {
		out[0] = l_limit / l * in1;
		out[1] = l_limit / l * in2;
	}
}

/* |x2|    |cosx,-sinx|   |x1|
 * |  | =  |          |   |  |
 * |y2|    |sinx, cosx|   |y2|
 * x = +-90 degree rotation, take sin x
 */
void plane_vector_roate(float sinx,
	struct plane_coordinates *in, struct plane_coordinates *out)
{
	out->x = in->x * float_sqrt(1 - float_pow(sinx, 2)) - in->y * sinx;
	out->y = in->y * float_sqrt(1 - float_pow(sinx, 2)) + in->x * sinx;
}

/* va x vb = | va || vb | *sinx * vn,
 *      vn is the unit vector perpendicular to va and vb.
 *      Normalization calculation, take va x vb = sinx;
 *
 * The counterclockwise angle of in1 and in2 is + -
 */
float plane_vector_cross_product(struct plane_coordinates *in1,
	struct plane_coordinates *in2)
{
	return in1->x * in2->y - in1->y * in2->x;
}

/* The angle of in1 and in2 is + -(the actual angle of space) */
float plane_vector_dot_product(struct plane_coordinates *in1,
	struct plane_coordinates *in2)
{
	return in1->x * in2->x + in1->y * in2->y;
}

/* A x B = (AyBz - AzBy)i + (AzBx - AxBz)j + (AxBy - AyBx)k
 * Output sin(x) of xyz error angle x, right-handed spiral
 */
void space_vector_cross_product_err_sinx(
	struct space_coordinates *in1, struct space_coordinates *in2,
	struct space_coordinates *out)
{
	out->x = in1->y * in2->z - in1->z * in2->y;
	out->y = in1->z * in2->x - in1->x * in2->z;
	out->z = in1->x * in2->y - in1->y * in2->x;
}

/* +- are the included angle of in1 and in2 (actual included angle of space) */
float space_vector_dot_product(struct space_coordinates *in1,
	struct space_coordinates *in2)
{
	return in1->x * in2->x + in1->y * in2->y + in1->z * in2->z;
}
