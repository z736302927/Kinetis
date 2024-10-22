
#include <generated/deconfig.h>
#include <linux/string.h>
#include <linux/slab.h>

#include "hydrology.h"
#include "hydrology-config.h"
#include "hydrology-identifier.h"


/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  .
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */


/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void hydrology_change_mode(u8 M)
{
//    HYDROLOGY_MODE = M;
}

static int hydrology_send_realtime_data(void)
{
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_E_PJ,
		HYDROLOGY_E_PT,
		HYDROLOGY_E_Z,
		HYDROLOGY_E_VT
	};

	if (hydrology_device_process_send(element_table,
			sizeof(element_table) / sizeof(struct hydrology_element_info),
			HYDROLOGY_M4, REAL_TIME_REPORT) == false)
		return false;

	return true;
}

static int hydrology_send_period_data(void)
{
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_E_DRxnn,
		0
	};

	element_table[1].ID = down_body->element[1]->guide[0];
	hydrology_read_specified_element_info(&element_table[1], PERIOD_REPORT, element_table[1].ID);

	if (hydrology_device_process_send(element_table,
			sizeof(element_table) / sizeof(struct hydrology_element_info),
			HYDROLOGY_M4, PERIOD_REPORT) == false)
		return false;

	return true;
}

static int hydrology_send_specified_element(void)
{
	struct hydrology_down_header *header = g_hydrology.down_packet->header;
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;
	struct hydrology_element_info *element_table;
	u8 i;
	int ret;

	if (down_body->count != 0) {
		element_table = kmalloc(sizeof(struct hydrology_element_info) * down_body->count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}
	}

	for (i = 0; i < down_body->count; i++) {
		hydrology_read_specified_element_info(&element_table[i], (enum hydrology_body_type)header->funcode,
			down_body->element[i]->guide[0]);
	}

	ret = hydrology_device_process_send(element_table, down_body->count,
			HYDROLOGY_M4, SPECIFIED_ELEMENT_REPORT);

	kfree(element_table);

	return ret;
}

static int hydrology_basic_info_config(void)
{
	struct hydrology_down_header *header = g_hydrology.down_packet->header;
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;
	struct hydrology_element_info *element_table;
	u8 i;

	if (down_body->count != 0) {
		element_table = kmalloc(sizeof(struct hydrology_element_info) * down_body->count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}
	}

	for (i = 0; i < down_body->count; i++) {
		hydrology_read_specified_element_info(&element_table[i], (enum hydrology_body_type)header->funcode,
			down_body->element[i]->guide[0]);

		fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
			element_table[i].addr,
			down_body->element[i]->value, down_body->element[i]->guide[1] >> 3);
	}

	kfree(element_table);

	return true;
}

static int hydrology_basic_info_read(enum hydrology_body_type funcode)
{
	struct hydrology_down_header *header = g_hydrology.down_packet->header;
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;
	struct hydrology_element_info *element_table;
	u8 i;
	int ret;

	if (down_body->count != 0) {
		element_table = kmalloc(sizeof(struct hydrology_element_info) * down_body->count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}
	}

	for (i = 0; i < down_body->count; i++) {
		hydrology_read_specified_element_info(&element_table[i], (enum hydrology_body_type)header->funcode,
			down_body->element[i]->guide[0]);
	}

	ret = hydrology_device_process_send(element_table, down_body->count,
			HYDROLOGY_M4, funcode);

	kfree(element_table);

	return ret;
}

static int hydrology_set_parameter(void)
{
	struct hydrology_down_header *header = g_hydrology.down_packet->header;
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;
	struct hydrology_element_info *element_table;
	u8 i;

	if (down_body->count != 0) {
		element_table = kmalloc(sizeof(struct hydrology_element_info) * down_body->count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}
	}

	for (i = 0; i < down_body->count; i++) {
		hydrology_read_specified_element_info(&element_table[i], (enum hydrology_body_type)header->funcode,
			down_body->element[i]->guide[0]);

		fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
			element_table[i].addr,
			down_body->element[i]->value, down_body->element[i]->guide[1] >> 3);
	}

	kfree(element_table);

	return true;
}

static int hydrology_read_parameter(enum hydrology_body_type funcode)
{
	struct hydrology_down_header *header = g_hydrology.down_packet->header;
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;
	struct hydrology_element_info *element_table;
	u8 i;
	int ret;

	if (down_body->count != 0) {
		element_table = kmalloc(sizeof(struct hydrology_element_info) * down_body->count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}
	}

	for (i = 0; i < down_body->count; i++) {
		hydrology_read_specified_element_info(&element_table[i], (enum hydrology_body_type)header->funcode,
			down_body->element[i]->guide[0]);
	}

	ret = hydrology_device_process_send(element_table, down_body->count,
			HYDROLOGY_M4, funcode);

	kfree(element_table);

	return ret;
}

static int hydrology_send_water_pump_motor_data(void)
{
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_E_VTA,
		HYDROLOGY_E_VTB,
		HYDROLOGY_E_VTC,
		HYDROLOGY_E_VIA,
		HYDROLOGY_E_VIB,
		HYDROLOGY_E_VIC
	};

	if (hydrology_device_process_send(element_table,
			sizeof(element_table) / sizeof(struct hydrology_element_info),
			HYDROLOGY_M4, WATER_PUMP_MOTOR_REPORT) == false)
		return false;

	return true;
}

static int hydrology_send_status_data(void)
{
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_E_ZT,
	};

	if (hydrology_device_process_send(element_table,
			sizeof(element_table) / sizeof(struct hydrology_element_info),
			HYDROLOGY_M4, STATUS_REPORT) == false)
		return false;

	return true;
}

static int hydrology_initialize_solid_storage(void)
{
	int ret;

//    f_unlink(HYDROLOGY_D_FILE_E_DATA);
//    f_unlink(HYDROLOGY_D_FILE_E_DATA);
//    f_unlink(HYDROLOGY_D_FILE_E_DATA);
//    f_unlink(HYDROLOGY_D_FILE_E_DATA);

	return ret;
}

int hydrology_device_reset(void)
{
	int ret;
	u8 temp[256];
	u16 Data[200];
	u16 i, j;

	for (i = 0; i < 10; i++) {
		for (j = 0; j < 200; j++)
			Data[j] = j + i * 200;

		fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_PICTURE,
			i * 200, (u8 *)Data, 200);
	}

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 200; j++)
			Data[j] = j + i * 200;

		fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_RGZS,
			i * 200, (u8 *)Data, 200);
	}

	temp[0] = 0x50;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PDA_RTUTYPE, temp, 1);
	temp[0] = 0x01;
	temp[1] = 0x02;
	temp[2] = 0x03;
	temp[3] = 0x04;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_CENTER, temp, 4);
	temp[0] = 0x00;
	temp[1] = 0x12;
	temp[2] = 0x34;
	temp[3] = 0x56;
	temp[4] = 0x78;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_REMOTE, temp, 5);
	temp[0] = 0x12;
	temp[1] = 0x34;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_PASSWORD, temp, 2);
	temp[0] = 10;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BAL_CENTER1_IP, temp, 1);
	temp[0] = 0x02;
	temp[1] = 0x05;
	temp[2] = 0x80;
	temp[3] = 0x49;
	temp[4] = 0x14;
	temp[5] = 0x02;
	temp[6] = 0x02;
	temp[7] = 0x00;
	temp[8] = 0x89;
	temp[9] = 0x86;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_CENTER1_IP, temp, 10);
	temp[0] = 10;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BAL_BACKUP1_IP, temp, 1);
	temp[0] = 0x02;
	temp[1] = 0x18;
	temp[2] = 0x30;
	temp[3] = 0x92;
	temp[4] = 0x03;
	temp[5] = 0x30;
	temp[6] = 0x30;
	temp[7] = 0x00;
	temp[8] = 0x66;
	temp[9] = 0x66;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_BACKUP1_IP, temp, 10);
	temp[0] = 10;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BAL_CENTER2_IP, temp, 1);
	temp[0] = 0x02;
	temp[1] = 0x22;
	temp[2] = 0x21;
	temp[3] = 0x60;
	temp[4] = 0x24;
	temp[5] = 0x52;
	temp[6] = 0x06;
	temp[7] = 0x00;
	temp[8] = 0x66;
	temp[9] = 0x66;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_CENTER2_IP, temp, 10);
	temp[0] = 10;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BAL_BACKUP2_IP, temp, 1);
	temp[0] = 0x02;
	temp[1] = 0x12;
	temp[2] = 0x00;
	temp[3] = 0x78;
	temp[4] = 0x13;
	temp[5] = 0x91;
	temp[6] = 0x49;
	temp[7] = 0x00;
	temp[8] = 0x99;
	temp[9] = 0x99;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_BACKUP2_IP, temp, 10);
	temp[0] = 10;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BAL_CENTER3_IP, temp, 1);
	temp[0] = 0x02;
	temp[1] = 0x12;
	temp[2] = 0x00;
	temp[3] = 0x78;
	temp[4] = 0x13;
	temp[5] = 0x91;
	temp[6] = 0x49;
	temp[7] = 0x00;
	temp[8] = 0x99;
	temp[9] = 0x99;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_CENTER3_IP, temp, 10);
	temp[0] = 10;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BAL_BACKUP3_IP, temp, 1);
	temp[0] = 0x02;
	temp[1] = 0x12;
	temp[2] = 0x00;
	temp[3] = 0x78;
	temp[4] = 0x13;
	temp[5] = 0x91;
	temp[6] = 0x49;
	temp[7] = 0x00;
	temp[8] = 0x99;
	temp[9] = 0x99;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_BACKUP3_IP, temp, 10);
	temp[0] = 10;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BAL_CENTER4_IP, temp, 1);
	temp[0] = 0x02;
	temp[1] = 0x12;
	temp[2] = 0x00;
	temp[3] = 0x78;
	temp[4] = 0x13;
	temp[5] = 0x91;
	temp[6] = 0x49;
	temp[7] = 0x00;
	temp[8] = 0x99;
	temp[9] = 0x99;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_CENTER4_IP, temp, 10);
	temp[0] = 10;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BAL_BACKUP4_IP, temp, 1);
	temp[0] = 0x02;
	temp[1] = 0x12;
	temp[2] = 0x00;
	temp[3] = 0x78;
	temp[4] = 0x13;
	temp[5] = 0x91;
	temp[6] = 0x49;
	temp[7] = 0x00;
	temp[8] = 0x99;
	temp[9] = 0x99;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_BACKUP4_IP, temp, 10);
	temp[0] = 0x02;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_WORK_MODE, temp, 2);
	temp[0] = 0x80;
	temp[1] = 0x01;
	temp[2] = 0x06;
	temp[3] = 0x01;
	temp[4] = 0x00;
	temp[5] = 0x00;
	temp[6] = 0x00;
	temp[7] = 0x00;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_ELEMENT_SELECT, temp, 8);
	temp[0] = 0x00;
	temp[1] = 0x00;
	temp[2] = 0x00;
	temp[3] = 0x00;
	temp[4] = 0x00;
	temp[5] = 0x00;
	temp[6] = 0x00;
	temp[7] = 0x00;
	temp[8] = 0x00;
	temp[9] = 0x00;
	temp[10] = 0x00;
	temp[11] = 0x00;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_REPEATER_STATION, temp, 12);
	temp[0] = 12;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BAL_DEVICE_ID, temp, 1);
	temp[0] = '1';
	temp[1] = '1';
	temp[2] = '2';
	temp[3] = '3';
	temp[4] = '4';
	temp[5] = '5';
	temp[6] = '6';
	temp[7] = '7';
	temp[8] = '8';
	temp[9] = '9';
	temp[10] = '0';
	temp[11] = '1';
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_DEVICE_ID, temp, 12);
	temp[0] = 0x01;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_TI, temp, 1);
	temp[0] = 0x05;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_AI, temp, 1);
	temp[0] = 0x08;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_RBT, temp, 1);
	temp[0] = 0x03;
	temp[1] = 0x00;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_SI, temp, 2);
	temp[0] = 0x05;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_WSI, temp, 1);
	temp[0] = 0x05;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_RR, temp, 1);
	temp[0] = 0x01;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_WR, temp, 1);
	temp[0] = 0x01;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_RAT, temp, 1);
	temp[0] = 0x01;
	temp[1] = 0x00;
	temp[2] = 0x00;
	temp[3] = 0x00;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_WB1, temp, 4);
	temp[0] = 0x01;
	temp[1] = 0x00;
	temp[2] = 0x00;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_WC1, temp, 3);
	temp[0] = 0x25;
	temp[1] = 0x01;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_WC1, temp, 2);
	temp[0] = 0x01;
	temp[1] = 0x00;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_AAT, temp, 2);
	temp[0] = 0x03;
	temp[1] = 0x00;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PA_ABT, temp, 2);
	temp[0] = strlen("*WHU-2020-V3.0");
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PDA_SW_VERSION_LEN, temp, 1);
	memcpy(temp, "*WHU-2020-V3.0", strlen("*WHU-2020-V3.0"));
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PDA_SW_VERSION, temp,
		strlen("*WHU-2020-V3.0"));

	static struct hydrology_element_info element_table[] = {
		HYDROLOGY_E_TT,
		HYDROLOGY_E_ST,
		HYDROLOGY_E_RGZS,
		HYDROLOGY_E_PIC,
		HYDROLOGY_E_DRP,
		HYDROLOGY_E_DRZ1,
		HYDROLOGY_E_DRZ2,
		HYDROLOGY_E_DRZ3,
		HYDROLOGY_E_DRZ4,
		HYDROLOGY_E_DRZ5,
		HYDROLOGY_E_DRZ6,
		HYDROLOGY_E_DRZ7,
		HYDROLOGY_E_DRZ8,
		HYDROLOGY_E_DATA,
		HYDROLOGY_E_AC,
		HYDROLOGY_E_AI,
		HYDROLOGY_E_C,
		HYDROLOGY_E_DRxnn,
		HYDROLOGY_E_DT,
		HYDROLOGY_E_ED,
		HYDROLOGY_E_EJ,
		HYDROLOGY_E_FL,
		HYDROLOGY_E_GH,
		HYDROLOGY_E_GN,
		HYDROLOGY_E_GS,
		HYDROLOGY_E_GT,
		HYDROLOGY_E_GTP,
		HYDROLOGY_E_H,
		HYDROLOGY_E_HW,
		HYDROLOGY_E_M10,
		HYDROLOGY_E_M20,
		HYDROLOGY_E_M30,
		HYDROLOGY_E_M40,
		HYDROLOGY_E_M50,
		HYDROLOGY_E_M60,
		HYDROLOGY_E_M80,
		HYDROLOGY_E_M100,
		HYDROLOGY_E_MST,
		HYDROLOGY_E_NS,
		HYDROLOGY_E_P1,
		HYDROLOGY_E_P2,
		HYDROLOGY_E_P3,
		HYDROLOGY_E_P6,
		HYDROLOGY_E_P12,
		HYDROLOGY_E_PD,
		HYDROLOGY_E_PJ,
		HYDROLOGY_E_PN01,
		HYDROLOGY_E_PN05,
		HYDROLOGY_E_PN10,
		HYDROLOGY_E_PN30,
		HYDROLOGY_E_PR,
		HYDROLOGY_E_PT,
		HYDROLOGY_E_Q,
		HYDROLOGY_E_Q1,
		HYDROLOGY_E_Q2,
		HYDROLOGY_E_Q3,
		HYDROLOGY_E_Q4,
		HYDROLOGY_E_Q5,
		HYDROLOGY_E_Q6,
		HYDROLOGY_E_Q7,
		HYDROLOGY_E_Q8,
		HYDROLOGY_E_QA,
		HYDROLOGY_E_QZ,
		HYDROLOGY_E_SW,
		HYDROLOGY_E_UC,
		HYDROLOGY_E_UE,
		HYDROLOGY_E_US,
		HYDROLOGY_E_VA,
		HYDROLOGY_E_VJ,
		HYDROLOGY_E_VT,
		HYDROLOGY_E_Z,
		HYDROLOGY_E_ZB,
		HYDROLOGY_E_ZU,
		HYDROLOGY_E_Z1,
		HYDROLOGY_E_Z2,
		HYDROLOGY_E_Z3,
		HYDROLOGY_E_Z4,
		HYDROLOGY_E_Z5,
		HYDROLOGY_E_Z6,
		HYDROLOGY_E_Z7,
		HYDROLOGY_E_Z8,
		HYDROLOGY_E_SQ,
		HYDROLOGY_E_ZT,
		HYDROLOGY_E_pH,
		HYDROLOGY_E_DO,
		HYDROLOGY_E_COND,
		HYDROLOGY_E_TURB,
		HYDROLOGY_E_CODMN,
		HYDROLOGY_E_REDOX,
		HYDROLOGY_E_NH4N,
		HYDROLOGY_E_TP,
		HYDROLOGY_E_TN,
		HYDROLOGY_E_TOC,
		HYDROLOGY_E_CU,
		HYDROLOGY_E_ZN,
		HYDROLOGY_E_SE,
		HYDROLOGY_E_AS,
		HYDROLOGY_E_THG,
		HYDROLOGY_E_CD,
		HYDROLOGY_E_PB,
		HYDROLOGY_E_CHLA,
		HYDROLOGY_E_WP1,
		HYDROLOGY_E_WP2,
		HYDROLOGY_E_WP3,
		HYDROLOGY_E_WP4,
		HYDROLOGY_E_WP5,
		HYDROLOGY_E_WP6,
		HYDROLOGY_E_WP7,
		HYDROLOGY_E_WP8,
		HYDROLOGY_E_SYL1,
		HYDROLOGY_E_SYL2,
		HYDROLOGY_E_SYL3,
		HYDROLOGY_E_SYL4,
		HYDROLOGY_E_SYL5,
		HYDROLOGY_E_SYL6,
		HYDROLOGY_E_SYL7,
		HYDROLOGY_E_SYL8,
		HYDROLOGY_E_SBL1,
		HYDROLOGY_E_SBL2,
		HYDROLOGY_E_SBL3,
		HYDROLOGY_E_SBL4,
		HYDROLOGY_E_SBL5,
		HYDROLOGY_E_SBL6,
		HYDROLOGY_E_SBL7,
		HYDROLOGY_E_SBL8,
		HYDROLOGY_E_VTA,
		HYDROLOGY_E_VTB,
		HYDROLOGY_E_VTC,
		HYDROLOGY_E_VIA,
		HYDROLOGY_E_VIB,
		HYDROLOGY_E_VIC,

		HYDROLOGY_B_CENTER,
		HYDROLOGY_B_REMOTE,
		HYDROLOGY_B_PASSWORD,
		HYDROLOGY_B_CENTER1_IP,
		HYDROLOGY_B_BACKUP1_IP,
		HYDROLOGY_B_CENTER2_IP,
		HYDROLOGY_B_BACKUP2_IP,
		HYDROLOGY_B_CENTER3_IP,
		HYDROLOGY_B_BACKUP3_IP,
		HYDROLOGY_B_CENTER4_IP,
		HYDROLOGY_B_BACKUP4_IP,
		HYDROLOGY_B_WORK_MODE,
		HYDROLOGY_B_ELEMENT_SELECT,
		HYDROLOGY_B_REPEATER_STATION,
		HYDROLOGY_B_DEVICE_ID,

		HYDROLOGY_P_TI,
		HYDROLOGY_P_AI,
		HYDROLOGY_P_RBT,
		HYDROLOGY_P_SI,
		HYDROLOGY_P_WSI,
		HYDROLOGY_P_RR,
		HYDROLOGY_P_WR,
		HYDROLOGY_P_RAT,
		HYDROLOGY_P_WB1,
		HYDROLOGY_P_WB2,
		HYDROLOGY_P_WB3,
		HYDROLOGY_P_WB4,
		HYDROLOGY_P_WB5,
		HYDROLOGY_P_WB6,
		HYDROLOGY_P_WB7,
		HYDROLOGY_P_WB8,
		HYDROLOGY_P_WC1,
		HYDROLOGY_P_WC2,
		HYDROLOGY_P_WC3,
		HYDROLOGY_P_WC4,
		HYDROLOGY_P_WC5,
		HYDROLOGY_P_WC6,
		HYDROLOGY_P_WC7,
		HYDROLOGY_P_WC8,
		HYDROLOGY_P_AW1,
		HYDROLOGY_P_AW2,
		HYDROLOGY_P_AW3,
		HYDROLOGY_P_AW4,
		HYDROLOGY_P_AW5,
		HYDROLOGY_P_AW6,
		HYDROLOGY_P_AW7,
		HYDROLOGY_P_AW8,
		HYDROLOGY_P_AAT,
		HYDROLOGY_P_ABT,
		HYDROLOGY_P_TAT,
		HYDROLOGY_P_FRAT,
		HYDROLOGY_P_GPAT,
		HYDROLOGY_P_PPT,
		HYDROLOGY_P_BAT,
		HYDROLOGY_P_WSWT,
		HYDROLOGY_P_WTAT,
		HYDROLOGY_P_UWLI1,
		HYDROLOGY_P_LWLI1,
		HYDROLOGY_P_UWLI2,
		HYDROLOGY_P_LWLI2,
		HYDROLOGY_P_UWLI3,
		HYDROLOGY_P_LWLI3,
		HYDROLOGY_P_UWLI4,
		HYDROLOGY_P_LWLI4,
		HYDROLOGY_P_UWLI5,
		HYDROLOGY_P_LWLI5,
		HYDROLOGY_P_UWLI6,
		HYDROLOGY_P_LWLI6,
		HYDROLOGY_P_UWLI7,
		HYDROLOGY_P_LWLI7,
		HYDROLOGY_P_UWLI8,
		HYDROLOGY_P_LWLI8,
		HYDROLOGY_P_ULWPI1,
		HYDROLOGY_P_LLWPI1,
		HYDROLOGY_P_ULWPI2,
		HYDROLOGY_P_LLWPI2,
		HYDROLOGY_P_ULWPI3,
		HYDROLOGY_P_LLWPI3,
		HYDROLOGY_P_ULWPI4,
		HYDROLOGY_P_LLWPI4,
		HYDROLOGY_P_ULWPI5,
		HYDROLOGY_P_LLWPI5,
		HYDROLOGY_P_ULWPI6,
		HYDROLOGY_P_LLWPI6,
		HYDROLOGY_P_ULWPI7,
		HYDROLOGY_P_LLWPI7,
		HYDROLOGY_P_ULWPI8,
		HYDROLOGY_P_LLWPI8,
		HYDROLOGY_P_ULWT,
		HYDROLOGY_P_LLWT,
		HYDROLOGY_P_ULpHV,
		HYDROLOGY_P_LLpHV,
		HYDROLOGY_P_ULDO,
		HYDROLOGY_P_LLDO,
		HYDROLOGY_P_ULPI,
		HYDROLOGY_P_LLPI,
		HYDROLOGY_P_ULCO,
		HYDROLOGY_P_LLCO,
		HYDROLOGY_P_ULRP,
		HYDROLOGY_P_LLRP,
		HYDROLOGY_P_ULT,
		HYDROLOGY_P_LLT,
		HYDROLOGY_P_ULAN,
		HYDROLOGY_P_LLAN,
		HYDROLOGY_P_ULTN,
		HYDROLOGY_P_LLTN,
		HYDROLOGY_P_ULC,
		HYDROLOGY_P_LLC,
		HYDROLOGY_P_ULZ,
		HYDROLOGY_P_LLZ,
		HYDROLOGY_P_ULF,
		HYDROLOGY_P_LLF,
		HYDROLOGY_P_ULS,
		HYDROLOGY_P_LLS,
		HYDROLOGY_P_ULA,
		HYDROLOGY_P_LLA,
		HYDROLOGY_P_ULM,
		HYDROLOGY_P_LLM,
		HYDROLOGY_P_ULCA,
		HYDROLOGY_P_LLCA,
		HYDROLOGY_P_ULTO,
		HYDROLOGY_P_LLTO,
		HYDROLOGY_P_ULCH,
		HYDROLOGY_P_LLCH,
		HYDROLOGY_P_ULFL,
		HYDROLOGY_P_RWQM1,
		HYDROLOGY_P_RWQM2,
		HYDROLOGY_P_RWQM3,
		HYDROLOGY_P_RWQM4,
		HYDROLOGY_P_RWQM5,
		HYDROLOGY_P_RWQM6,
		HYDROLOGY_P_RWQM7,
		HYDROLOGY_P_RWQM8,
		HYDROLOGY_P_FVWQ,
		HYDROLOGY_P_DISSS,
		HYDROLOGY_P_TTPRFS,
		HYDROLOGY_P_BVWM1,
		HYDROLOGY_P_BVWM2,
		HYDROLOGY_P_BVWM3,
		HYDROLOGY_P_BVWM4,
		HYDROLOGY_P_BVWM5,
		HYDROLOGY_P_BVWM6,
		HYDROLOGY_P_BVWM7,
		HYDROLOGY_P_BVWM8,
		HYDROLOGY_P_WMRWAV1,
		HYDROLOGY_P_WMRWAV2,
		HYDROLOGY_P_WMRWAV3,
		HYDROLOGY_P_WMRWAV4,
		HYDROLOGY_P_WMRWAV5,
		HYDROLOGY_P_WMRWAV6,
		HYDROLOGY_P_WMRWAV7,
		HYDROLOGY_P_WMRWAV8,

		HYDROLOGY_PD_INIT_MARK,
		HYDROLOGY_PD_RTUTYPE,
		HYDROLOGY_PD_PERIOD_BT,
		HYDROLOGY_PD_PERIOD_ET,
		HYDROLOGY_PD_SW_VERSION_LEN,
		HYDROLOGY_PD_SW_VERSION,
		HYDROLOGY_PD_PUMP_LEN,
		HYDROLOGY_PD_PUMP,
		HYDROLOGY_PD_VALVE_LEN,
		HYDROLOGY_PD_VALVE,
		HYDROLOGY_PD_GATE_LEN,
		HYDROLOGY_PD_GATE,
		HYDROLOGY_PD_WATERSETTING,
		HYDROLOGY_PD_RECORD,
		HYDROLOGY_PD_NEWPASSWORD,
	};

	ret = fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_INFO,
			0, (u8 *)element_table, sizeof(element_table));

	return ret;
}

static int hydrology_set_password(void)
{
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;

	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_BA_PASSWORD,
		down_body->element[1]->value, down_body->element[1]->num);

	return true;
}

static int hydrology_set_clock(void)
{
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;

	hydrology_set_time(down_body->send_time);

	return true;
}

static int hydrology_set_iccard(void)
{
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;

	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_EA_ZT,
		down_body->element[0]->value, down_body->element[0]->num);

	return true;
}

static int hydrology_set_pump(void)
{
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;

	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PDA_PUMP,
		down_body->element[0]->value, down_body->element[0]->num);

	return true;
}

static int hydrology_set_valve(void)
{
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;

	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PDA_VALVE,
		down_body->element[0]->value, down_body->element[0]->num);

	return true;
}

static int hydrology_set_gate(void)
{
	char gatesize;
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;

	gatesize = down_body->element[0]->guide[0];
	gatesize = ((gatesize - 1) / 8 + 1) + 2 * gatesize + 1;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PDA_GATE,
		down_body->element[0]->value, down_body->element[0]->num);

	return true;
}

static int hydrology_set_watersetting(void)
{
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;

	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PDA_WATERSETTING,
		down_body->element[0]->value, down_body->element[0]->num);

	return true;
}

static int hydrology_record_erc(int index)
{
	u16 erc_cnt = 0;
	int addr = (index - 1) * 2;
	u8 _temp_erc_cnt[2];

	fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_RECORD + addr,
		_temp_erc_cnt, 2);
	erc_cnt = (_temp_erc_cnt[0] << 8) + _temp_erc_cnt[1];
	erc_cnt++;
	_temp_erc_cnt[0] = erc_cnt >> 8;
	_temp_erc_cnt[1] = erc_cnt & 0x00FF;
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		HYDROLOGY_PDA_RECORD + addr, _temp_erc_cnt, 2);

	return true;
}

static int hydrology_send_password(void)
{
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_B_PASSWORD,
	};

	if (hydrology_device_process_send(element_table,
			sizeof(element_table) / sizeof(struct hydrology_element_info),
			HYDROLOGY_M4, CHANGE_PASSWORD_REPORT) == false)
		return false;

	return true;
}

static int hydrology_send_iccard(void)
{
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_E_ZT,
	};

	if (hydrology_device_process_send(element_table,
			sizeof(element_table) / sizeof(struct hydrology_element_info),
			HYDROLOGY_M4, SET_IC_CARD_REPORT) == false)
		return false;

	return true;
}

static int hydrology_send_pump(void)
{
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_PD_PUMP,
	};

	if (hydrology_device_process_send(element_table,
			sizeof(element_table) / sizeof(struct hydrology_element_info),
			HYDROLOGY_M4, PUMP_REPORT) == false)
		return false;

	return true;
}

static int hydrology_send_valve(void)
{
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_PD_VALVE,
	};

	if (hydrology_device_process_send(element_table,
			sizeof(element_table) / sizeof(struct hydrology_element_info),
			HYDROLOGY_M4, VALVE_REPORT) == false)
		return false;

	return true;
}

static int hydrology_send_gate(void)
{
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_PD_GATE,
	};

	if (hydrology_device_process_send(element_table,
			sizeof(element_table) / sizeof(struct hydrology_element_info),
			HYDROLOGY_M4, GATE_REPORT) == false)
		return false;

	return true;
}

static int hydrology_send_water_setting(void)
{
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_PD_WATERSETTING,
	};

	if (hydrology_device_process_send(element_table,
			sizeof(element_table) / sizeof(struct hydrology_element_info),
			HYDROLOGY_M4, WATER_SETTING_REPORT) == false)
		return false;

	return true;
}

static int hydrology_send_record_erc(void)
{
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_PD_RECORD,
	};

	if (hydrology_device_process_send(element_table,
			sizeof(element_table) / sizeof(struct hydrology_element_info),
			HYDROLOGY_M4, RECORD_REPORT) == false)
		return false;

	return true;
}

int hydrology_execute_command(enum hydrology_body_type funcode)
{
	int ret = false;

	switch (funcode) {
	case TEST_REPORT:
	case EVEN_PERIOD_INFO_REPORT:
	case TIMER_REPORT:
	case ADD_REPORT:
	case HOUR_REPORT:
	case ARTIFICIAL_NUM_REPORT:
	case PICTURE_REPORT:
	case REAL_TIME_REPORT:
	case PERIOD_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
	case SPECIFIED_ELEMENT_REPORT:
	case CONFIG_READ_REPORT:
	case PARA_READ_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
	case SW_VERSION_REPORT:
	case STATUS_REPORT:
	case RECORD_REPORT:
	case TIME_REPORT:
		break;

	case CONFIG_WRITE_REPORT:
		hydrology_basic_info_config();
		break;

	case PARA_WRITE_REPORT:
		hydrology_set_parameter();
		hydrology_record_erc(ERC2);
		break;

	case INIT_SOLID_STORAGE_REPORT:
		hydrology_initialize_solid_storage();
		hydrology_record_erc(ERC5);
		hydrology_device_reboot();
		break;

	case RESET_REPORT:
		hydrology_device_reset();
		hydrology_device_reboot();
		break;

	case CHANGE_PASSWORD_REPORT:
		hydrology_set_password();
		hydrology_record_erc(ERC5);
		break;

	case SET_CLOCK_REPORT:
		hydrology_set_clock();
		break;

	case SET_IC_CARD_REPORT:
		hydrology_set_iccard();
		break;

	case PUMP_REPORT:
		hydrology_set_pump();
		break;

	case VALVE_REPORT:
		hydrology_set_valve();
		break;

	case GATE_REPORT:
		hydrology_set_gate();
		break;

	case WATER_SETTING_REPORT:
		hydrology_set_watersetting();
		break;

	default:
		break;
	}

	return ret;
}

int hydrology_response_downstream(enum hydrology_body_type funcode)
{
	int ret = false;

	switch (funcode) {
	case LINK_REPORT:
	case TEST_REPORT:
	case EVEN_PERIOD_INFO_REPORT:
	case TIMER_REPORT:
	case ADD_REPORT:
	case HOUR_REPORT:
	case ARTIFICIAL_NUM_REPORT:
	case PICTURE_REPORT:
		break;

	case REAL_TIME_REPORT:
		hydrology_send_realtime_data();
		break;

	case PERIOD_REPORT:
		hydrology_send_period_data();
		break;

	case INQUIRE_ARTIFICIAL_NUM_REPORT:
		return hydrology_device_process_send(NULL, 0, HYDROLOGY_M4, INQUIRE_ARTIFICIAL_NUM_REPORT);

	case SPECIFIED_ELEMENT_REPORT:
		hydrology_send_specified_element();
		break;

	case CONFIG_WRITE_REPORT:
		hydrology_basic_info_read(CONFIG_WRITE_REPORT);
		break;

	case CONFIG_READ_REPORT:
		hydrology_basic_info_read(CONFIG_READ_REPORT);
		break;

	case PARA_WRITE_REPORT:
		hydrology_read_parameter(PARA_WRITE_REPORT);
		break;

	case PARA_READ_REPORT:
		hydrology_read_parameter(PARA_READ_REPORT);
		break;

	case WATER_PUMP_MOTOR_REPORT:
		hydrology_send_water_pump_motor_data();
		break;

	case SW_VERSION_REPORT:
		return hydrology_device_process_send(NULL, 0, HYDROLOGY_M4, SW_VERSION_REPORT);

	case STATUS_REPORT:
		hydrology_send_status_data();
		break;

	case INIT_SOLID_STORAGE_REPORT:
		return hydrology_device_process_send(NULL, 0, HYDROLOGY_M4, INIT_SOLID_STORAGE_REPORT);

	case RESET_REPORT:
		return hydrology_device_process_send(NULL, 0, HYDROLOGY_M4, RESET_REPORT);

	case CHANGE_PASSWORD_REPORT:
		hydrology_send_password();
		break;

	case SET_CLOCK_REPORT:
		return hydrology_device_process_send(NULL, 0, HYDROLOGY_M4, SET_CLOCK_REPORT);

	case SET_IC_CARD_REPORT:
		hydrology_send_iccard();
		break;

	case PUMP_REPORT:
		hydrology_send_pump();
		break;

	case VALVE_REPORT:
		hydrology_send_valve();
		break;

	case GATE_REPORT:
		hydrology_send_gate();
		break;

	case WATER_SETTING_REPORT:
		hydrology_send_water_setting();
		break;

	case RECORD_REPORT:
		hydrology_send_record_erc();
		break;

	case TIME_REPORT:
		return hydrology_device_process_send(NULL, 0, HYDROLOGY_M4, TIME_REPORT);
	}

	return ret;
}

int hydrology_response_upstream(enum hydrology_body_type funcode, u8 End)
{
	int ret = false;

	switch (funcode) {
	case LINK_REPORT:
	case REAL_TIME_REPORT:
	case PERIOD_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
	case SPECIFIED_ELEMENT_REPORT:
	case CONFIG_WRITE_REPORT:
	case CONFIG_READ_REPORT:
	case PARA_WRITE_REPORT:
	case PARA_READ_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
	case SW_VERSION_REPORT:
	case STATUS_REPORT:
	case INIT_SOLID_STORAGE_REPORT:
	case RESET_REPORT:
	case CHANGE_PASSWORD_REPORT:
	case SET_CLOCK_REPORT:
	case SET_IC_CARD_REPORT:
	case WATER_SETTING_REPORT:
	case RECORD_REPORT:
	case TIME_REPORT:
		break;

	case TEST_REPORT:
		return hydrology_host_process_send(NULL, 0, HYDROLOGY_M2, TEST_REPORT, End);

	case EVEN_PERIOD_INFO_REPORT:
		return hydrology_host_process_send(NULL, 0, HYDROLOGY_M2, EVEN_PERIOD_INFO_REPORT, End);

	case TIMER_REPORT:
		return hydrology_host_process_send(NULL, 0, HYDROLOGY_M2, TIMER_REPORT, End);

	case ADD_REPORT:
		return hydrology_host_process_send(NULL, 0, HYDROLOGY_M2, ADD_REPORT, End);

	case HOUR_REPORT:
		return hydrology_host_process_send(NULL, 0, HYDROLOGY_M2, HOUR_REPORT, End);

	case ARTIFICIAL_NUM_REPORT:
		return hydrology_host_process_send(NULL, 0, HYDROLOGY_M2, ARTIFICIAL_NUM_REPORT, End);

	case PICTURE_REPORT:
		return hydrology_host_process_send(NULL, 0, HYDROLOGY_M2, PICTURE_REPORT, End);

	case PUMP_REPORT:
		return hydrology_host_process_send(NULL, 0, HYDROLOGY_M2, PUMP_REPORT, End);

	case VALVE_REPORT:
		return hydrology_host_process_send(NULL, 0, HYDROLOGY_M2, VALVE_REPORT, End);

	case GATE_REPORT:
		return hydrology_host_process_send(NULL, 0, HYDROLOGY_M2, GATE_REPORT, End);
	}

	return ret;
}

