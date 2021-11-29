#include "fmu_para.h"


static void pid_reset(struct fmu_para *para)
{
	/* Attitude control angle speed ring PID parameters */
	para->pid_att_1level[ROL][KP] = 4.0f;
	para->pid_att_1level[ROL][KI] = 2.0f;
	para->pid_att_1level[ROL][KD] = 0.2f;

	para->pid_att_1level[PIT][KP] = 4.0f;
	para->pid_att_1level[PIT][KI] = 2.0f;
	para->pid_att_1level[PIT][KD] = 0.2f;

	para->pid_att_1level[YAW][KP] = 6.0f;
	para->pid_att_1level[YAW][KI] = 0.5f;
	para->pid_att_1level[YAW][KD] = 0.0f;
	/* Attitude control angle ring PID parameters */
	para->pid_att_2level[ROL][KP] = 7.0f;
	para->pid_att_2level[ROL][KI] = 0.0f;
	para->pid_att_2level[ROL][KD] = 0.00f;

	para->pid_att_2level[PIT][KP] = 7.0f;
	para->pid_att_2level[PIT][KI] = 0.0f;
	para->pid_att_2level[PIT][KD] = 0.00f;

	para->pid_att_2level[YAW][KP] = 5.0f;
	para->pid_att_2level[YAW][KI] = 0.0f;
	para->pid_att_2level[YAW][KD] = 0.5;

	/* Height control height speed ring PID parameters */
	para->pid_alt_1level[KP] = 2.0f;
	para->pid_alt_1level[KI] = 1.0f;
	para->pid_alt_1level[KD] = 0.05f;
	/* Height control height ring PID parameters */
	para->pid_alt_2level[KP] = 1.0f;
	para->pid_alt_2level[KI] = 0;
	para->pid_alt_2level[KD] = 0;

	/* Position control position speed ring PID parameters */
	para->pid_loc_1level[KP] = 0.15f;
	para->pid_loc_1level[KI] = 0.10f;
	para->pid_loc_1level[KD] = 0.00f;
	/* Position control position ring PID parameters */
	para->pid_loc_2level[KP] = 0;
	para->pid_loc_2level[KI] = 0;
	para->pid_loc_2level[KD] = 0;

	/* The GPS position controls the position speed ring PID parameters */
	para->pid_gps_loc_1level[KP] = 0.15f;
	para->pid_gps_loc_1level[KI] = 0.10f;
	para->pid_gps_loc_1level[KD] = 0.00f;
	/* The GPS position controls the position ring PID parameters */
	para->pid_gps_loc_2level[KP] = 0.3f;
	para->pid_gps_loc_2level[KI] = 0;
	para->pid_gps_loc_2level[KD] = 0;

	AnoDTSendStr(USE_HID | USE_U2, SWJ_ADDR, LOG_COLOR_GREEN, "PID reset!");
}

static void para_reset(struct fmu_para *para, u8 mode)
{
	u8 i;

	/* Parameter initialization */
	if (mode >= 1) {
		para->heat_switch = 0;
		para->bat_cell = 3;
		para->warn_power_voltage = 3.50f;
		para->lowest_power_voltage = 3.40f;

		para->auto_take_off_height = 0;
		para->auto_take_off_speed = 150;
		para->auto_landing_speed = 60;

		para->idle_speed_pwm = 20;
	}

	if (mode == 2) {
		para->acc_calibrated = 0;
		para->mag_calibrated = 0;

		para->iem[0][0] = 1;
		para->iem[0][1] = 0;
		para->iem[0][2] = 0;
		para->iem[1][0] = 0;
		para->iem[1][1] = 1;
		para->iem[1][2] = 0;
		para->iem[2][0] = 0;
		para->iem[2][1] = 0;
		para->iem[2][2] = 1;

		for (i = 0; i < 3; i++) {
			para->acc_zero_offset[i] = 0;
			para->acc_sensitivity_ref[i] = 1368;
			para->gyr_zero_offset[i] = 0;
			para->mag_offset[i] = 0;
			para->mag_gain[i] = 200;

			para->body_central_pos_cm[i] = 0;
		}
	}

	AnoDTSendStr(USE_HID | USE_U2, SWJ_ADDR, LOG_COLOR_GREEN, "parameter reset!");
}

static int para_write(struct fmu_para *para)
{
	fmu_pid_init();

	para->frist_init = SW_VER;

	return hydrology_write_store_info(para);
}

static int para_read(struct fmu_para *para)
{
	hydrology_read_store_info(para);

	/*
	 * If the content is not initialized,
	 * parameter initialization is performed
	 */
	if (para->frist_init != SW_VER) {
		para_reset(para, 2);
		pid_reset(para);
		para_write(para);
	}
}

void fmu_para_write_task(struct fmu_para *para, u16 dt_ms)
{
	/*
	 * Because writing flash takes a long time, we made a special logic,
	 * after unlocking, is not to write parameters,
	 * at this time will be placed a need to write flag bit,
	 * such as the aircraft landing lock, and then write parameters,
	 * improve flight safety. In order to avoid updating two parameters in a row,
	 * resulting in flash write twice, we fly control added a delay logic,
	 * parameters changed three seconds before writing operations,
	 * you can write multiple parameters at a time,
	 * reduce the number of flash erasure
	 */
	if (para->state.save_en) {
		if (para->state.save_trig == 1) {
			LED_STA.saving = 1;

			para->state.time_delay = 0;
			para->state.save_trig = 2;
		}

		if (para->state.save_trig == 2) {
			if (para->state.time_delay < 3000)
				para->state.time_delay += dt_ms;
			else {
				para->state.save_trig = 0;
				fmu_write_para();
				AnoDTSendStr(USE_HID | USE_U2, SWJ_ADDR, LOG_COLOR_GREEN, "Set save OK!");
				LED_STA.saving = 0;
			}
		} else
			para->state.time_delay = 0;
	} else {
		para->state.time_delay = 0;
		para->state.save_trig = 0;
	}
}

int fmu_read_para(struct fmu_para *para, u16 id)
{
	int p_val = 0;

	switch (id) {
	/* basic */
	case PARA_HW_TYPE:
		p_val = HW_TYPE;
		break;

	case PARA_HW_VER:
		p_val = HW_VER;
		break;

	case PARA_SW_VER:
		p_val = SW_VER;
		break;

	/* setting */
	case PARA_BAT_CELLS:
		p_val = para->bat_cell;
		break;

	case PARA_LV_WARN_100:
		p_val = 1e2f * para->warn_power_voltage;
		break;

	case PARA_LV_LAND_100:
		p_val = 1e2f * para->lowest_power_voltage;
		break;

	case PARA_HEATSWITCH:
		p_val = para->heat_switch;
		break;

	case PARA_LANDSPEED:
		p_val = para->auto_landing_speed;
		break;

	case PARA_TAKEOFFSPEED:
		p_val = para->auto_take_off_speed;
		break;

	case PARA_TAKEOFFHIGH:
		p_val = para->auto_take_off_height;
		break;

	case PARA_UNLOCKPWM:
		p_val = para->idle_speed_pwm;
		break;

	/* angular velocity */
	case PARA_PID_1_P:
		p_val = 1e3f * para->pid_att_1level[ROL][0];
		break;

	case PARA_PID_1_I:
		p_val = 1e3f * para->pid_att_1level[ROL][1];
		break;

	case PARA_PID_1_D:
		p_val = 1e3f * para->pid_att_1level[ROL][2];
		break;

	case PARA_PID_2_P:
		p_val = 1e3f * para->pid_att_1level[PIT][0];
		break;

	case PARA_PID_2_I:
		p_val = 1e3f * para->pid_att_1level[PIT][1];
		break;

	case PARA_PID_2_D:
		p_val = 1e3f * para->pid_att_1level[PIT][2];
		break;

	case PARA_PID_3_P:
		p_val = 1e3f * para->pid_att_1level[YAW][0];
		break;

	case PARA_PID_3_I:
		p_val = 1e3f * para->pid_att_1level[YAW][1];
		break;

	case PARA_PID_3_D:
		p_val = 1e3f * para->pid_att_1level[YAW][2];
		break;

	/* angular */
	case PARA_PID_4_P:
		p_val = 1e3f * para->pid_att_2level[ROL][0];
		break;

	case PARA_PID_4_I:
		p_val = 1e3f * para->pid_att_2level[ROL][1];
		break;

	case PARA_PID_4_D:
		p_val = 1e3f * para->pid_att_2level[ROL][2];
		break;

	case PARA_PID_5_P:
		p_val = 1e3f * para->pid_att_2level[PIT][0];
		break;

	case PARA_PID_5_I:
		p_val = 1e3f * para->pid_att_2level[PIT][1];
		break;

	case PARA_PID_5_D:
		p_val = 1e3f * para->pid_att_2level[PIT][2];
		break;

	case PARA_PID_6_P:
		p_val = 1e3f * para->pid_att_2level[YAW][0];
		break;

	case PARA_PID_6_I:
		p_val = 1e3f * para->pid_att_2level[YAW][1];
		break;

	case PARA_PID_6_D:
		p_val = 1e3f * para->pid_att_2level[YAW][2];
		break;

	/* Vertical speed */
	case PARA_PID_7_P:
		p_val = 1e3f * para->pid_alt_1level[0];
		break;

	case PARA_PID_7_I:
		p_val = 1e3f * para->pid_alt_1level[1];
		break;

	case PARA_PID_7_D:
		p_val = 1e3f * para->pid_alt_1level[2];
		break;

	/* height */
	case PARA_PID_8_P:
		p_val = 1e3f * para->pid_alt_2level[0];
		break;

//		case PARA_PID_8_I: p_val = 1e3f *para->pid_alt_2level[1]; break;
//		case PARA_PID_8_D: p_val = 1e3f *para->pid_alt_2level[2]; break;
	/* Horizontal speed */
	case PARA_PID_9_P:
		p_val = 1e3f * para->pid_loc_1level[0];
		break;

	case PARA_PID_9_I:
		p_val = 1e3f * para->pid_loc_1level[1];
		break;

//		case PARA_PID_9_D: p_val = 1e3f *para->pid_loc_1level[2];	break;
	/* Horizontal position */
//		case PARA_PID_10_P: p_val = 1e3f *para->pid_loc_2level[0];	break;
//		case PARA_PID_10_I: p_val = 1e3f *para->pid_loc_2level[1];	break;
//		case PARA_PID_10_D: p_val = 1e3f *para->pid_loc_2level[2];	break;
	/* GPS speed ring */
	case PARA_PID_11_P:
		p_val = 1e3f * para->pid_gps_loc_1level[0];
		break;

	case PARA_PID_11_I:
		p_val = 1e3f * para->pid_gps_loc_1level[1];
		break;

//		case PARA_PID_11_D: p_val = 1e3f *para->pid_gps_loc_1level[2];	break;
	/* GPS location ring */
	case PARA_PID_12_P:
		p_val = 1e3f * para->pid_gps_loc_2level[0];
		break;

//		case PARA_PID_12_I: p_val = 1e3f *para->pid_gps_loc_2level[1];	break;
//		case PARA_PID_12_D: p_val = 1e3f *para->pid_gps_loc_2level[2];	break;
	//====sensor_cali
	case CAL_ACC_OFFSET_X:
		p_val = para->acc_zero_offset[0] * 1000;
		break;

	case CAL_ACC_OFFSET_Y:
		p_val = para->acc_zero_offset[1] * 1000;
		break;

	case CAL_ACC_OFFSET_Z:
		p_val = para->acc_zero_offset[2] * 1000;
		break;

	case CAL_ACC_SENSIV_X:
		p_val = para->acc_sensitivity_ref[0] * 1000;
		break;

	case CAL_ACC_SENSIV_Y:
		p_val = para->acc_sensitivity_ref[1] * 1000;
		break;

	case CAL_ACC_SENSIV_Z:
		p_val = para->acc_sensitivity_ref[2] * 1000;
		break;

	case CAL_ACC_IEM_00:
		p_val = para->iem[0][0] * 100000;
		break;

	case CAL_ACC_IEM_01:
		p_val = para->iem[0][1] * 100000;
		break;

	case CAL_ACC_IEM_02:
		p_val = para->iem[0][2] * 100000;
		break;

	case CAL_ACC_IEM_10:
		p_val = para->iem[1][0] * 100000;
		break;

	case CAL_ACC_IEM_11:
		p_val = para->iem[1][1] * 100000;
		break;

	case CAL_ACC_IEM_12:
		p_val = para->iem[1][2] * 100000;
		break;

	case CAL_ACC_IEM_20:
		p_val = para->iem[2][0] * 100000;
		break;

	case CAL_ACC_IEM_21:
		p_val = para->iem[2][1] * 100000;
		break;

	case CAL_ACC_IEM_22:
		p_val = para->iem[2][2] * 100000;
		break;

	case CAL_MAG_OFFSET_X:
		p_val = para->mag_offset[0] * 1000;
		break;

	case CAL_MAG_OFFSET_Y:
		p_val = para->mag_offset[1] * 1000;
		break;

	case CAL_MAG_OFFSET_Z:
		p_val = para->mag_offset[2] * 1000;
		break;

	case CAL_MAG_SENSIV_X:
		p_val = para->mag_gain[0] * 1000;
		break;

	case CAL_MAG_SENSIV_Y:
		p_val = para->mag_gain[1] * 1000;
		break;

	case CAL_MAG_SENSIV_Z:
		p_val = para->mag_gain[2] * 1000;
		break;

	/*
	 * Default must note that if the flight control is not used parameters,
	 * return the 0x80000000, the upper opportunity to judge that
	 * the parameters of the lower machine is not used,
	 * there will be a specific display.
	 */
	default:
		p_val = 0x80000000;
		break;
	}

	return p_val;
}

void fmu_write_para(struct fmu_para *para, u16 id, int val)
{
	/* Trigger write (actually stops triggering delay of 3 seconds before writing) */
	data_save();

	switch (id) {
	case PARA_BAT_CELLS:
		para->bat_cell = clamp(val, 0, 65535);
		break;

	case PARA_LV_WARN_100:
		para->warn_power_voltage = 1e-2f * clamp(val, 0, 65535);
		break;

	case PARA_LV_LAND_100:
		para->lowest_power_voltage = 1e-2f * clamp(val, 0, 65535);
		break;

	case PARA_HEATSWITCH:
		para->heat_switch = clamp(val, 0, 2);
		break;

	case PARA_LANDSPEED:
		para->auto_landing_speed = clamp(val, 0, 65535);
		break;

	case PARA_TAKEOFFSPEED:
		para->auto_take_off_speed = clamp(val, 0, 65535);
		break;

	case PARA_TAKEOFFHIGH:
		para->auto_take_off_height = clamp(val, 0, 65535);
		break;

	case PARA_UNLOCKPWM:
		para->idle_speed_pwm = clamp(val, 0, 65535);
		break;

	/* angular velocity */
	case PARA_PID_1_P:
		para->pid_att_1level[ROL][0] = 1e-3f  * clamp(val, 0, 65535);
		break;

	case PARA_PID_1_I:
		para->pid_att_1level[ROL][1] = 1e-3f  * clamp(val, 0, 65535);
		break;

	case PARA_PID_1_D:
		para->pid_att_1level[ROL][2] = 1e-3f  * clamp(val, 0, 65535);
		break;

	case PARA_PID_2_P:
		para->pid_att_1level[PIT][0] = 1e-3f  * clamp(val, 0, 65535);
		break;

	case PARA_PID_2_I:
		para->pid_att_1level[PIT][1] = 1e-3f  * clamp(val, 0, 65535);
		break;

	case PARA_PID_2_D:
		para->pid_att_1level[PIT][2] = 1e-3f  * clamp(val, 0, 65535);
		break;

	case PARA_PID_3_P:
		para->pid_att_1level[YAW][0] = 1e-3f  * clamp(val, 0, 65535);
		break;

	case PARA_PID_3_I:
		para->pid_att_1level[YAW][1] = 1e-3f  * clamp(val, 0, 65535);
		break;

	case PARA_PID_3_D:
		para->pid_att_1level[YAW][2] = 1e-3f  * clamp(val, 0, 65535);
		break;

	/* angular */
	case PARA_PID_4_P:
		para->pid_att_2level[ROL][0] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_4_I:
		para->pid_att_2level[ROL][1] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_4_D:
		para->pid_att_2level[ROL][2] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_5_P:
		para->pid_att_2level[PIT][0] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_5_I:
		para->pid_att_2level[PIT][1] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_5_D:
		para->pid_att_2level[PIT][2] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_6_P:
		para->pid_att_2level[YAW][0] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_6_I:
		para->pid_att_2level[YAW][1] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_6_D:
		para->pid_att_2level[YAW][2] = 1e-3f * clamp(val, 0, 65535);
		break;

	/* Vertical speed */
	case PARA_PID_7_P:
		para->pid_alt_1level[0] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_7_I:
		para->pid_alt_1level[1] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_7_D:
		para->pid_alt_1level[2] = 1e-3f * clamp(val, 0, 65535);
		break;

	/* height */
	case PARA_PID_8_P:
		para->pid_alt_2level[0] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_8_I:
		para->pid_alt_2level[1] = 0;
		break;//clamp(p_val,0,65535);	break;

	case PARA_PID_8_D:
		para->pid_alt_2level[2] = 0;
		break;//clamp(p_val,0,65535);	break;

	/* Horizontal speed */
	case PARA_PID_9_P:
		para->pid_loc_1level[0] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_9_I:
		para->pid_loc_1level[1] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_9_D:
		para->pid_loc_1level[2] = 1e-3f * clamp(val, 0, 65535);
		break;

	/* Horizontal position */
	case PARA_PID_10_P:
		para->pid_loc_2level[0] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_10_I:
		para->pid_loc_2level[1] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_10_D:
		para->pid_loc_2level[2] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_11_P:
		para->pid_gps_loc_1level[0] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_11_I:
		para->pid_gps_loc_1level[1] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_11_D:
		para->pid_gps_loc_1level[2] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_12_P:
		para->pid_gps_loc_2level[0] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_12_I:
		para->pid_gps_loc_2level[1] = 1e-3f * clamp(val, 0, 65535);
		break;

	case PARA_PID_12_D:
		para->pid_gps_loc_2level[2] = 1e-3f * clamp(val, 0, 65535);
		break;

	/* sensor_cali */
	case CAL_ACC_OFFSET_X:
		para->acc_zero_offset[0] = (float)val / 1000;
		break;

	case CAL_ACC_OFFSET_Y:
		para->acc_zero_offset[1] = (float)val / 1000;
		break;

	case CAL_ACC_OFFSET_Z:
		para->acc_zero_offset[2] = (float)val / 1000;
		break;

	case CAL_ACC_SENSIV_X:
		para->acc_sensitivity_ref[0] = (float)val / 1000;
		break;

	case CAL_ACC_SENSIV_Y:
		para->acc_sensitivity_ref[1] = (float)val / 1000;
		break;

	case CAL_ACC_SENSIV_Z:
		para->acc_sensitivity_ref[2] = (float)val / 1000;
		break;

	case CAL_ACC_IEM_00:
		para->iem[0][0] = (float)val / 100000;
		break;

	case CAL_ACC_IEM_01:
		para->iem[0][1] = (float)val / 100000;
		break;

	case CAL_ACC_IEM_02:
		para->iem[0][2] = (float)val / 100000;
		break;

	case CAL_ACC_IEM_10:
		para->iem[1][0] = (float)val / 100000;
		break;

	case CAL_ACC_IEM_11:
		para->iem[1][1] = (float)val / 100000;
		break;

	case CAL_ACC_IEM_12:
		para->iem[1][2] = (float)val / 100000;
		break;

	case CAL_ACC_IEM_20:
		para->iem[2][0] = (float)val / 100000;
		break;

	case CAL_ACC_IEM_21:
		para->iem[2][1] = (float)val / 100000;
		break;

	case CAL_ACC_IEM_22:
		para->iem[2][2] = (float)val / 100000;
		st_imu_cali.acc_cali_on = 0;
		para->acc_calibrated = 1;
		/* Sensor sensitivity initialization */
		ImuSensitivityInit(para->acc_calibrated,
			(float *)para->acc_sensitivity_ref);
		data_save();
		break;

	case CAL_MAG_OFFSET_X:
		para->mag_offset[0] = (float)val / 1000;
		break;

	case CAL_MAG_OFFSET_Y:
		para->mag_offset[1] = (float)val / 1000;
		break;

	case CAL_MAG_OFFSET_Z:
		para->mag_offset[2] = (float)val / 1000;
		break;

	case CAL_MAG_SENSIV_X:
		para->mag_gain[0] = (float)val / 1000;
		break;

	case CAL_MAG_SENSIV_Y:
		para->mag_gain[1] = (float)val / 1000;
		break;

	case CAL_MAG_SENSIV_Z:
		para->mag_gain[2] = (float)val / 1000;
		mag.mag_CALIBRATE = 0;
		para->mag_calibrated = 1;
		data_save();
		break;

	default:
		break;
	}
}

