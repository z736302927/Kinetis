
#include <generated/deconfig.h>
#include <linux/slab.h>

#include <kinetis/ano_protocol.h>
#include <kinetis/fatfs-intf.h>
#include <kinetis/fmu.h>
#include <kinetis/tim-task.h>


static void fmu_reset_pid(struct fmu_parameter *para)
{
	/* Attitude control angle speed ring PID parameters */
	para->attitude_df.rol.p = 4.0f;
	para->attitude_df.rol.i = 2.0f;
	para->attitude_df.rol.d = 0.2f;

	para->attitude_df.pit.p = 4.0f;
	para->attitude_df.pit.i = 2.0f;
	para->attitude_df.pit.d = 0.2f;

	para->attitude_df.yaw.p = 6.0f;
	para->attitude_df.yaw.i = 0.5f;
	para->attitude_df.yaw.d = 0.0f;
	/* Attitude control angle ring PID parameters */
	para->attitude.rol.p = 7.0f;
	para->attitude.rol.i = 0.0f;
	para->attitude.rol.d = 0.00f;

	para->attitude.pit.p = 7.0f;
	para->attitude.pit.i = 0.0f;
	para->attitude.pit.d = 0.00f;

	para->attitude.yaw.p = 5.0f;
	para->attitude.yaw.i = 0.0f;
	para->attitude.yaw.d = 0.5;

	/* Height control height speed ring PID parameters */
	para->height_df.p = 2.0f;
	para->height_df.i = 1.0f;
	para->height_df.d = 0.05f;
	/* Height control height ring PID parameters */
	para->height.p = 1.0f;
	para->height.i = 0;
	para->height.d = 0;

	/* Position control position speed ring PID parameters */
	para->position_df.p = 0.15f;
	para->position_df.i = 0.10f;
	para->position_df.d = 0.00f;
	/* Position control position ring PID parameters */
	para->position.p = 0;
	para->position.i = 0;
	para->position.d = 0;

	/* The GPS position controls the position speed ring PID parameters */
	para->gps_df.p = 0.15f;
	para->gps_df.i = 0.10f;
	para->gps_df.d = 0.00f;
	/* The GPS position controls the position ring PID parameters */
	para->gps.p = 0.3f;
	para->gps.i = 0;
	para->gps.d = 0;

	ano_send_log_string(ANO_LOG_COLOR_GREEN,
		"PID initialization");
}

static int fmu_reset_para(struct fmu_parameter *para, u8 mode)
{
	/* Parameter initialization */
	if (mode >= 1) {
		para->heat_switch = 0;
		para->battery.cell = 3;
		para->battery.warn_voltage = 3.50f;
		para->battery.lowest_voltage = 3.40f;

		para->auto_take_off_height = 0;
		para->auto_take_off_speed = 150;
		para->auto_landing_speed = 60;

		para->idle_speed_pwm = 20;
	}

	if (mode == 2) {
		para->accel_calibrated = 0;
		para->magnet_calibrated = 0;

		para->iem[0][0] = 1;
		para->iem[0][1] = 0;
		para->iem[0][2] = 0;
		para->iem[1][0] = 0;
		para->iem[1][1] = 1;
		para->iem[1][2] = 0;
		para->iem[2][0] = 0;
		para->iem[2][1] = 0;
		para->iem[2][2] = 1;

		para->accel_offset.x = 0;
		para->accel_offset.y = 0;
		para->accel_offset.z = 0;

		para->gyro_offset.x = 0;
		para->gyro_offset.y = 0;
		para->gyro_offset.z = 0;

		para->magnet_offset.x = 0;
		para->magnet_offset.y = 0;
		para->magnet_offset.z = 0;

		para->magnet_gain.x = 200;
		para->magnet_gain.y = 200;
		para->magnet_gain.z = 200;

		para->gravity_offset.x = 0;
		para->gravity_offset.y = 0;
		para->gravity_offset.z = 0;

		para->accel_sensitivity_ref.x = 1368;
		para->accel_sensitivity_ref.y = 1368;
		para->accel_sensitivity_ref.z = 1368;
	}

	ano_send_log_string(ANO_LOG_COLOR_GREEN,
		"Parameter initialization");

	return 0;
}

static void task_1ms(struct tim_task *task)
{
	struct fmu_core *fmu = get_fmu_core();
	int ret;

	ret = bmi088_accel_get_all_axis();
	if (ret)
		pr_info("Failed to get accel data");

	ret = bmi088_gyro_get_all_axis();
	if (ret)
		pr_info("Failed to get gyro data");

	ahrs_container_in(&fmu->ahrs, 1);

	ret = imu_static_detect(0.001f, &fmu->accel_cmpss.x, &fmu->gyro_degree.x);
	if (ret == 0x03) {
		ret = imu_calculate_offset(&fmu->para.accel_offset,
				&fmu->para.gyro_offset);
		if (ret)
			pr_info("Failed to calculate offset");
	}

}

static void task_20ms(struct tim_task *task)
{
	int ret;

	ret = spl06_get_temp_and_pressure();
	if (ret)
		pr_info("Failed to get pressure data");

	ret = ak8975_get_all_axis();
	if (ret)
		pr_info("Failed to get magnet data");

}

//TO 20 //XBRG
//float LED_Brightness[FMU_LED_NUM] = {0, 20, 0, 0};

///* led driver, calling it in interrupt every milisecond */
//void led_op_task(struct fmu_status_instruction *led)  //0~20
//{
//	static u16 led_cnt[FMU_LED_NUM];
//	u8 i;

//	for (i = 0; i < FMU_LED_NUM; i++) {
//		if (led_cnt[i] < led->brightness[i])
//			FMU_LED_OPERATION(i, 1)
//			else
//				FMU_LED_OPERATION(i, 0)

//				if (led_cnt[i] >= FMU_LED_ACCURACY)
//					led_cnt[i] = 0;
//				else
//					led_cnt[i]++;
//	}
//}

//static void led_op_switch(struct fmu_status_instruction *led, u8 led_num)
//{
//	u8 i;

//	for (i = 0; i < FMU_LED_NUM; i++) {
//		if (led_num & (1 << i))
//			led->brightness[i] = 20;
//		else
//			led->brightness[i] = 0;
//	}
//}

//static void led_op_breath(struct fmu_status_instruction *led,
//	u8 dt_ms, u8 led_num, u16 T)
//{
//	static u8 dir[FMU_LED_NUM];
//	u8 i;

//	for (i = 0; i < FMU_LED_NUM; i++) {
//		if (led_num & (1 << i)) {
//			switch (dir[i]) {
//			case 0:
//				led->brightness[i] += safe_div(FMU_LED_ACCURACY, ((float)T / (dt_ms)), 0);

//				if (led->brightness[i] > 20)
//					dir[i] = 1;
//				break;
//			case 1:
//				led->brightness[i] -= safe_div(FMU_LED_ACCURACY, ((float)T / (dt_ms)), 0);

//				if (led->brightness[i] < 0)
//					dir[i] = 0;
//				break;
//			default:
//				dir[i] = 0;
//				break;
//			}
//		} else
//			led->brightness[i] = 0;
//	}
//}

///**
// * led_op_flash - led flash mode
// * @dt_ms: call period
// * @led: led number
// * @on_ms: on time
// * @off_ms: off time
// *
// * Return: zero on success, else a negative error code.
// */
//static void led_op_flash(struct fmu_status_instruction *led,
//	u8 dt_ms, u8 led_num, u16 on_ms, u16 off_ms)
//{
//	static u16 time;

//	if (time < on_ms)
//		led_op_switch(led, led_num);
//	else
//		led_op_switch(led, 0);

//	time += dt_ms;

//	if (time >= (on_ms + off_ms))
//		time = 0;
//}

//void fmu_instruction_led(struct fmu_status_instruction *led, u8 dt_ms)
//{
//	static u16 time = 0;
//	static u8 count = 0;
//	u8 flash_cnt;

//	if (led->error_onetime) {
//		led_op_switch(led, FMU_BIT_RLED);
//		time += dt_ms;

//		if (time > 3000) {
//			time = 0;
//			led->error_onetime = 0;
//		}
//	} else if (led->error_mpu || led->error_magnet || led->error_baro) {
//		if (led->error_magnet)
//			flash_cnt = 3;
//		else if (led->error_mpu)
//			flash_cnt = 2;
//		else
//			flash_cnt = 4;

//		if (count < flash_cnt) {
//			if (time < 100)
//				led_op_switch(led, FMU_BIT_RLED);
//			else
//				led_op_switch(led, 0);

//			time += dt_ms;

//			if (time > 400) {
//				time = 0;
//				count++;
//			}
//		} else {
//			time += dt_ms;

//			if (time > 1000) {
//				time = 0;
//				count = 0;
//			}
//		}
//	} else if (led->saving_para) {
//		led->brightness[FMU_G_LED] = 20;
//		led->brightness[FMU_R_LED] = 0;
//		led->brightness[FMU_B_LED] = 0;
//	} else if (led->calibration_accel || led->calibration_gyro || led->reset_imu)
//		led_op_flash(led, dt_ms, FMU_BIT_WLED, 40, 40);
//	else if (led->calibration_magnet) {
//		if (led->calibration_magnet == 1)
//			led_op_breath(led, dt_ms, FMU_BIT_GLED, 300);
//		else if (led->calibration_magnet == 2)
//			led_op_flash(led, dt_ms, FMU_BIT_PLED, 40, 40);
//		else if (led->calibration_magnet == 100)
//			led_op_flash(led, dt_ms, FMU_BIT_RLED, 40, 40);
//		else
//			led_op_breath(led, dt_ms, FMU_BIT_BLED, 300);
//	} else if (led->invalid_rc)
//		led_op_breath(led, dt_ms, FMU_BIT_RLED, 600);
//	else if (led->low_voltage)
//		led_op_flash(led, dt_ms, FMU_BIT_RLED, 100, 100);
//	else {
//		/* No other prompts, normal display mode gear and external optical flow,
//		 * GPS and other states */
//		static u8 status = 0;
//		static u8 mode = 0;
//		/* Flight modes 1, 2 and 3 are displayed.
//		 * White is unlocked and green is unlocked.
//		 * The mode flashes several times */
//		if (status == 0) {
//			if (mode <= flag.flight_mode) {
//				if (time < 60) {
//					if (flag.unlock_sta)
//						led_op_switch(led, FMU_BIT_GLED);
//					else
//						led_op_switch(led, FMU_BIT_WLED);
//				} else
//					led_op_switch(led, 0);

//				time += dt_ms;

//				if (time > 200) {
//					time = 0;
//					mode++;
//				}
//			} else {
//				mode = 0;
//				status = 1;
//			}
//		} else if (status == 1) {	//Display the status of optical flow, GPS, etc
//			if (mode == 0) {	//Judge whether GPS is normal
//				if (switchs.gps_on) {
//					if (time < 60)
//						led_op_switch(led, FMU_BIT_BLED);
//					else
//						led_op_switch(led, 0);

//					time += dt_ms;

//					if (time > 200) {
//						time = 0;
//						mode++;
//					}
//				} else
//					mode = 1;
//			} else if (mode == 1) {	//Judge whether the optical flow is normal
//				if (switchs.of_flow_on > 0 && switchs.of_tof_on > 0) {
//					if (time < 60)
//						led_op_switch(led, FMU_BIT_PLED);
//					else
//						led_op_switch(led, 0);

//					time += dt_ms;

//					if (time > 200) {
//						time = 0;
//						mode++;
//					}
//				} else
//					mode = 2;
//			} else if (mode == 2) {
//				//Judge the external ano of flight control_ Is opmv normal
//				if (switchs.opmv_on && (ano_opmv_cbt_ctrl.target_loss == 0 || ano_opmv_lt_ctrl.target_loss == 0)) {
//					if (time < 60)
//						led_op_switch(led, FMU_BIT_YLED);
//					else
//						led_op_switch(led, 0);

//					time += dt_ms;

//					if (time > 200) {
//						time = 0;
//						mode++;
//					}
//				} else
//					mode = 3;
//			} else if (mode == 3) {
//				status = 2;
//				mode = 0;
//			}
//		} else {
//			//After each cycle, add a long delay to turn off the light
//			led_op_switch(led, 0);
//			time += dt_ms;

//			if (time > 1000) {
//				time = 0;
//				status = 0;
//				mode = 0;
//			}
//		}
//	}
//}

static struct fmu_core *kinetis_fmu;

struct fmu_core *get_fmu_core(void)
{
	return kinetis_fmu;
}

int fmu_init(void)
{
	struct fmu_core *fmu;
	int ret;

	fmu = kzalloc(sizeof(*fmu), GFP_KERNEL);
	if (!fmu)
		return -ENOMEM;
	kinetis_fmu = fmu;

	ret = fatfs_find_file(FMU_FILE_PATH, FMU_PARA_FILE);
	if (ret) {
		ret = fatfs_create_file(FMU_FILE_PATH, FMU_PARA_FILE);
		if (ret)
			return ret;
		ret = fatfs_read_store_info(FMU_FILE_PATH, FMU_PARA_FILE,
				0, (u8 *)&fmu->para, sizeof(fmu->para));
		if (ret)
			return ret;
	}

	if (fmu->para.frist_init) {
		fmu->para.frist_init = true;

		fmu_reset_para(&fmu->para, 2);
		fmu_reset_pid(&fmu->para);
		fmu_ctrl_pid_init(&fmu->ctrl, &fmu->para);

		ret = fatfs_write_store_info(FMU_FILE_PATH, FMU_PARA_FILE,
				0, (u8 *)&fmu->para, sizeof(fmu->para));
		if (ret)
			return ret;
	}

//	ret = ahrs_init_container(1024);
//	if (ret)
//		return 0;

	ret = rc_module_init(&fmu->rc);
	if (ret)
		return 0;

//	ret = fmu_check_esc();
//	if (ret)
//		return 0;

//	ret = sensor_check_battery();
//	if (ret)
//		return 0;

//	ret = sensor_check_magnetometer();
//	if (ret)
//		return 0;

//	ret = sensor_check_pressure();
//	if (ret)
//		return 0;

//	ret = sensor_check_imu();
//	if (ret)
//		return 0;

//	ret = sensor_check_laser();
//	if (ret)
//		return 0;

//	ret = sensor_check_optical_flow();
//	if (ret)
//		return 0;

//	ret = sensor_check_gps();
//	if (ret)
//		return 0;

//	ret = sensor_check_heat_res();
//	if (ret)
//		return 0;

//	ret = sensor_check_ano_dt();
//	if (ret)
//		return 0;

	pr_info("All checker pass, you can fly now!");

	return 0;
}












