
#include <generated/deconfig.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/crc16.h>

#include <kinetis/design_verification.h>

#include "hydrology.h"
#include "hydrology-config.h"
#include "hydrology-cmd.h"
#include "hydrology-identifier.h"


/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_typeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

int hydrology_host_reset(void)
{
	int ret;
	u8 temp[15];

	temp[0] = 0x50;
	ret = fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
			HYDROLOGY_PDA_RTUTYPE, temp, 1);
	if (ret != true)
		return ret;

	temp[0] = 0x01;
	temp[1] = 0x02;
	temp[2] = 0x03;
	temp[3] = 0x04;
	ret = fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
			HYDROLOGY_BA_CENTER, temp, 4);
	if (ret != true)
		return ret;

	temp[0] = 0x00;
	temp[1] = 0x12;
	temp[2] = 0x34;
	temp[3] = 0x56;
	temp[4] = 0x78;
	ret = fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
			HYDROLOGY_BA_REMOTE, temp, 5);
	if (ret != true)
		return ret;

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

int hydrology_host_init_send(u8 cnt, enum hydrology_body_type funcode)
{
	int i = 0;
	struct hydrology_down_body *down_body;

	g_hydrology.down_packet = kmalloc(sizeof(struct hydrology_packet), __GFP_ZERO);

	if (g_hydrology.down_packet == NULL) {
		pr_err("g_hydrology.down_packet malloc failed\n");
		return false;
	}

	g_hydrology.down_packet->header = kmalloc(sizeof(struct hydrology_down_header), __GFP_ZERO);

	if (g_hydrology.down_packet->header == NULL) {
		pr_err("g_hydrology.down_packet->header malloc failed\n");
		return false;
	}

	g_hydrology.down_packet->body = kmalloc(sizeof(struct hydrology_down_body), __GFP_ZERO);

	if (g_hydrology.down_packet->body == NULL) {
		pr_err("g_hydrology.down_packet->body malloc failed\n");
		return false;
	}

	down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;
	down_body->count = cnt;

	switch (funcode) {
	case LINK_REPORT:
	case TEST_REPORT:
	case EVEN_PERIOD_INFO_REPORT:
	case TIMER_REPORT:
	case ADD_REPORT:
	case HOUR_REPORT:
	case ARTIFICIAL_NUM_REPORT:
	case PICTURE_REPORT:
	case REAL_TIME_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
	case SW_VERSION_REPORT:
	case STATUS_REPORT:
	case SET_CLOCK_REPORT:
	case RECORD_REPORT:
	case TIME_REPORT:
		down_body->count = 0;
		break;

	case PERIOD_REPORT:
	case SPECIFIED_ELEMENT_REPORT:
	case CONFIG_WRITE_REPORT:
	case CONFIG_READ_REPORT:
	case PARA_WRITE_REPORT:
	case PARA_READ_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
	case CHANGE_PASSWORD_REPORT:
		break;

	case INIT_SOLID_STORAGE_REPORT:
	case RESET_REPORT:
	case SET_IC_CARD_REPORT:
	case PUMP_REPORT:
	case VALVE_REPORT:
	case GATE_REPORT:
	case WATER_SETTING_REPORT:
		down_body->count = 1;
		break;
	}

	if (down_body->count > 0) {
		down_body->element = kmalloc(sizeof(struct hydrology_element *) * down_body->count, __GFP_ZERO);

		if (down_body->element == NULL) {
			pr_err("down_body->element malloc failed\n");
			return false;
		}
	}

	for (i = 0; i < down_body->count; ++i) {
		down_body->element[i] = kmalloc(sizeof(struct hydrology_element), __GFP_ZERO);

		if (down_body->element[i] == NULL) {
			pr_err("down_body->element[%d] malloc failed\n", i);
			return false;
		}
	}

	return true;
}

void hydrology_host_exit_send(void)
{
	int i = 0;
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;

	for (i = 0; i < down_body->count; i++) {
		if (down_body->element[i]->value != NULL) {
			kfree(down_body->element[i]->value);
			down_body->element[i]->value = NULL;
		}

		if (down_body->element[i] != NULL) {
			kfree(down_body->element[i]);
			down_body->element[i] = NULL;
		}
	}

	if (down_body->element != NULL) {
		kfree(down_body->element);
		down_body->element = NULL;
	}

	if (g_hydrology.down_packet->header != NULL) {
		kfree(g_hydrology.down_packet->header);
		g_hydrology.down_packet->header = NULL;
	}

	if (g_hydrology.down_packet->body != NULL) {
		kfree(g_hydrology.down_packet->body);
		g_hydrology.down_packet->body = NULL;
	}

	if (g_hydrology.down_packet->buffer != NULL) {
		kfree(g_hydrology.down_packet->buffer);
		g_hydrology.down_packet->buffer = NULL;
	}

	if (g_hydrology.down_packet != NULL) {
		kfree(g_hydrology.down_packet);
		g_hydrology.down_packet = NULL;
	}
}

int hydrology_host_init_receieve()
{
	g_hydrology.up_packet = kmalloc(sizeof(struct hydrology_packet), __GFP_ZERO);

	if (g_hydrology.up_packet == NULL) {
		pr_err("g_hydrology.up_packet malloc failed\n");
		return false;
	}

	g_hydrology.up_packet->header = kmalloc(sizeof(struct hydrology_up_header), __GFP_ZERO);

	if (g_hydrology.up_packet->header == NULL) {
		pr_err("g_hydrology.up_packet->header malloc failed\n");
		return false;
	}

	g_hydrology.up_packet->body = kmalloc(sizeof(struct hydrology_up_body), __GFP_ZERO);

	if (g_hydrology.up_packet->body == NULL) {
		pr_err("g_hydrology.up_packet->body malloc failed\n");
		return false;
	}

	return true;
}

void hydrology_host_exit_receieve()
{
	int i = 0;
	struct hydrology_up_body *upbody = (struct hydrology_up_body *)g_hydrology.up_packet->body;

	for (i = 0; i < upbody->count; i++) {
		if (upbody->element[i]->value != NULL) {
			kfree(upbody->element[i]->value);
			upbody->element[i]->value = NULL;
		}

		if (upbody->element[i] != NULL) {
			kfree(upbody->element[i]);
			upbody->element[i] = NULL;
		}
	}

	if (upbody->element != NULL) {
		kfree(upbody->element);
		upbody->element = NULL;
	}

	if (g_hydrology.up_packet->header != NULL) {
		kfree(g_hydrology.up_packet->header);
		g_hydrology.up_packet->header = NULL;
	}

	if (g_hydrology.up_packet->body != NULL) {
		kfree(g_hydrology.up_packet->body);
		g_hydrology.up_packet->body = NULL;
	}

	if (g_hydrology.up_packet->buffer != NULL) {
		kfree(g_hydrology.up_packet->buffer);
		g_hydrology.up_packet->buffer = NULL;
	}

	if (g_hydrology.up_packet != NULL) {
		kfree(g_hydrology.up_packet);
		g_hydrology.up_packet = NULL;
	}
}

static void hydrology_host_set_down_header_sequence(u16 cnt, u16 Total)
{
	struct hydrology_down_header *header = g_hydrology.down_packet->header;

	header->count_seq[0] = Total >> 4;
	header->count_seq[1] = ((Total & 0x000F) << 4) + (cnt >> 8);
	header->count_seq[2] = cnt & 0x00FF;
}

static void hydrology_host_make_down_header(enum hydrology_mode mode, enum hydrology_body_type funcode)
{
	struct hydrology_down_header *header = (struct hydrology_down_header *)g_hydrology.down_packet->header;

	header->frame_start[0] = SOH;
	header->frame_start[1] = SOH;
	header->len += 2;

	fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
		HYDROLOGY_BA_REMOTE, header->remote_addr, 5);
	header->len += 5;
	fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
		HYDROLOGY_BA_CENTER, &(header->center_addr), 1);
	header->len += 1;

	fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
		HYDROLOGY_BA_PASSWORD, header->password, 2);
	header->len += 2;

	header->funcode = funcode;
	header->len += 1;
	header->dir_len[0] = 8 << 4;
	header->len += 2;

	switch (mode) {
	case HYDROLOGY_M1:
	case HYDROLOGY_M2:
	case HYDROLOGY_M4:
		header->paket_start = STX;
		header->len += 1;
		break;

	case HYDROLOGY_M3:
		header->paket_start = SYN;
		header->len += 1;
		break;
	}
}

static int hydrology_host_make_down_body(struct hydrology_element_info *element_table,
	enum hydrology_mode mode, enum hydrology_body_type funcode)
{
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;
	int i;

	down_body->len = 0;

	switch (funcode) {
	case LINK_REPORT:
		break;

	case TEST_REPORT:
	case EVEN_PERIOD_INFO_REPORT:
	case TIMER_REPORT:
	case ADD_REPORT:
	case HOUR_REPORT:
	case ARTIFICIAL_NUM_REPORT:
	case PICTURE_REPORT:
	case REAL_TIME_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
	case SW_VERSION_REPORT:
	case STATUS_REPORT:
	case SET_CLOCK_REPORT:
	case RECORD_REPORT:
	case TIME_REPORT:
		hydrology_get_stream_id(down_body->stream_id);
		down_body->len += 2;
		hydrology_get_time(down_body->send_time);
		down_body->len += 6;
		break;

	case PERIOD_REPORT:
		down_body->len += 8;
		hydrology_get_stream_id(down_body->stream_id);
		down_body->len += 2;
		hydrology_get_time(down_body->send_time);
		down_body->len += 6;

		if (hydrology_malloc_element(element_table[0].ID,
				element_table[0].D, element_table[0].d,
				down_body->element[0]) == false)
			return false;

		fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
			element_table[0].addr,
			down_body->element[0]->value, down_body->element[0]->num);
		down_body->len += down_body->element[0]->num + 2;

		down_body->element[1]->guide[0] = element_table[1].ID;
		hydrology_get_guide_id(&(down_body->element[1]->guide[1]),
			element_table[1].D, element_table[1].d);
		down_body->len += 2;
		break;

	case SPECIFIED_ELEMENT_REPORT:
	case CONFIG_READ_REPORT:
	case PARA_READ_REPORT:
	case INIT_SOLID_STORAGE_REPORT:
	case RESET_REPORT:
		hydrology_get_stream_id(down_body->stream_id);
		down_body->len += 2;
		hydrology_get_time(down_body->send_time);
		down_body->len += 6;

		for (i = 0; i < down_body->count; i++) {
			down_body->element[i]->guide[0] = element_table[i].ID;
			hydrology_get_guide_id(&(down_body->element[i]->guide[1]),
				element_table[i].D, element_table[i].d);

			down_body->len += 2;
		}

		break;

	case CONFIG_WRITE_REPORT:
	case PARA_WRITE_REPORT:
	case CHANGE_PASSWORD_REPORT:
	case SET_IC_CARD_REPORT:
		hydrology_get_stream_id(down_body->stream_id);
		down_body->len += 2;
		hydrology_get_time(down_body->send_time);
		down_body->len += 6;

		for (i = 0; i < down_body->count; i++) {
			if (hydrology_malloc_element(element_table[i].ID,
					element_table[i].D, element_table[i].d,
					down_body->element[i]) == false)
				return false;

			fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
				element_table[i].addr,
				down_body->element[i]->value, down_body->element[i]->num);

			down_body->len += down_body->element[i]->num + 2;
		}

		break;

	case PUMP_REPORT:
		hydrology_get_stream_id(down_body->stream_id);
		down_body->len += 2;
		hydrology_get_time(down_body->send_time);
		down_body->len += 6;

		if (mode == HYDROLOGY_M4) {
			fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
				HYDROLOGY_PDA_PUMP_LEN, down_body->element[0]->guide, 1);
			down_body->element[0]->num = down_body->element[0]->guide[0];
			down_body->element[0]->value = kmalloc(down_body->element[0]->num, __GFP_ZERO);

			if (NULL == down_body->element[0]->value) {
				pr_err("down_body->element[0]->value malloc failed\n");
				return false;
			}

			fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
				HYDROLOGY_PDA_PUMP,
				down_body->element[0]->value, down_body->element[0]->num);
			down_body->len += down_body->element[0]->num + 1;
		}

		break;

	case VALVE_REPORT:
		hydrology_get_stream_id(down_body->stream_id);
		down_body->len += 2;
		hydrology_get_time(down_body->send_time);
		down_body->len += 6;

		if (mode == HYDROLOGY_M4) {
			fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
				HYDROLOGY_PDA_VALVE_LEN, down_body->element[0]->guide, 1);
			down_body->element[0]->num = down_body->element[0]->guide[0];
			down_body->element[0]->value = kmalloc(down_body->element[0]->num, __GFP_ZERO);

			if (NULL == down_body->element[0]->value) {
				pr_err("down_body->element[0]->value malloc failed\n");
				return false;
			}

			fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
				HYDROLOGY_PDA_VALVE,
				down_body->element[0]->value, down_body->element[0]->num);
			down_body->len += down_body->element[0]->num + 1;
		}

		break;

	case GATE_REPORT:
		hydrology_get_stream_id(down_body->stream_id);
		down_body->len += 2;
		hydrology_get_time(down_body->send_time);
		down_body->len += 6;

		if (mode == HYDROLOGY_M4) {
			fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
				HYDROLOGY_PDA_GATE_LEN, down_body->element[0]->guide, 1);

			if (down_body->element[0]->guide[0] % 8 == 0)
				down_body->element[0]->num = down_body->element[0]->guide[0] / 8;
			else
				down_body->element[0]->num = down_body->element[0]->guide[0] / 8 + 1;

			down_body->element[0]->value = kmalloc(down_body->element[0]->num, __GFP_ZERO);

			if (NULL == down_body->element[0]->value) {
				pr_err("down_body->element[0]->value malloc failed\n");
				return false;
			}

			fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
				HYDROLOGY_PDA_GATE,
				down_body->element[0]->value, down_body->element[0]->num);
			down_body->len += down_body->element[0]->num + 1;
		}

		break;

	case WATER_SETTING_REPORT:
		hydrology_get_stream_id(down_body->stream_id);
		down_body->len += 2;
		hydrology_get_time(down_body->send_time);
		down_body->len += 6;
		down_body->element[0]->guide[0] = 1;
		down_body->element[0]->num = down_body->element[0]->guide[0];
		down_body->element[0]->value = kmalloc(down_body->element[0]->num, __GFP_ZERO);

		if (NULL == down_body->element[0]->value) {
			pr_err("down_body->element[0]->value malloc failed\n");
			return false;
		}

		fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA,
			HYDROLOGY_PDA_WATERSETTING,
			down_body->element[0]->value, down_body->element[0]->num);
		down_body->len += down_body->element[0]->num;
		break;
	}

	return true;
}

static int hydrology_host_make_down_tail_and_send(enum hydrology_mode mode,
	enum hydrology_body_type funcode, u8 End)
{
	u8 *buffer;
	u16 i;
	u16 buffer_size;
	u16 pointer;
	u8 stime[6] = {0, 0, 0, 0, 0, 0};
	u8 ctime[6] = {0, 0, 0, 0, 0, 0};
	struct hydrology_down_header *header = (struct hydrology_down_header *)g_hydrology.down_packet->header;
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;

	buffer_size = header->len + down_body->len + 3;
	g_hydrology.down_packet->buffer = kmalloc(buffer_size, __GFP_ZERO);

	if (g_hydrology.down_packet->buffer == NULL) {
		pr_err("g_hydrology.down_packet->buffer malloc failed\n");
		return false;
	}

	buffer = g_hydrology.down_packet->buffer;

	header->dir_len[0] |= (down_body->len) >> 8;
	header->dir_len[1] |= (down_body->len) & 0xFF;
	memcpy(buffer, header, sizeof(struct hydrology_down_header) - 4);
	pointer = sizeof(struct hydrology_down_header) - 4;

	switch (funcode) {
	case LINK_REPORT:
		break;

	case TEST_REPORT:
	case EVEN_PERIOD_INFO_REPORT:
	case TIMER_REPORT:
	case ADD_REPORT:
	case HOUR_REPORT:
	case ARTIFICIAL_NUM_REPORT:
	case PICTURE_REPORT:
	case REAL_TIME_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
	case SW_VERSION_REPORT:
	case STATUS_REPORT:
	case SET_CLOCK_REPORT:
	case RECORD_REPORT:
	case TIME_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;
		break;

	case PERIOD_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;
		hydrology_get_time(stime);
		memcpy(ctime, stime, 6);
		ctime[3]--;
		memcpy(&buffer[pointer], stime, 4);
		pointer += 4;
		memcpy(&buffer[pointer], ctime, 4);
		pointer += 4;

		memcpy(&buffer[pointer], down_body->element[0]->guide, 2);
		pointer += 2;
		memcpy(&buffer[pointer], down_body->element[0]->value, down_body->element[0]->num);
		pointer += down_body->element[0]->num;
		memcpy(&buffer[pointer], down_body->element[1]->guide, 2);
		pointer += 2;

		break;

	case SPECIFIED_ELEMENT_REPORT:
	case CONFIG_READ_REPORT:
	case PARA_READ_REPORT:
	case INIT_SOLID_STORAGE_REPORT:
	case RESET_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;

		for (i = 0; i < down_body->count; i++) {
			memcpy(&buffer[pointer], down_body->element[i]->guide, 2);
			pointer += 2;
		}

		break;

	case CONFIG_WRITE_REPORT:
	case PARA_WRITE_REPORT:
	case CHANGE_PASSWORD_REPORT:
	case SET_IC_CARD_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;

		for (i = 0; i < down_body->count; i++) {
			memcpy(&buffer[pointer], down_body->element[i]->guide, 2);
			pointer += 2;
			memcpy(&buffer[pointer], down_body->element[i]->value, down_body->element[i]->num);
			pointer += down_body->element[i]->num;
		}

		break;

	case PUMP_REPORT:
	case VALVE_REPORT:
	case GATE_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;

		if (mode == HYDROLOGY_M4) {
			memcpy(&buffer[pointer], down_body->element[0]->guide, 1);
			pointer += 1;
			memcpy(&buffer[pointer], down_body->element[0]->value, down_body->element[0]->num);
			pointer += down_body->element[0]->num;
		}

		break;

	case WATER_SETTING_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;
		memcpy(&buffer[pointer], down_body->element[0]->value, down_body->element[0]->num);
		pointer += down_body->element[0]->num;
		break;
	}

	g_hydrology.down_packet->end = End;
	buffer[pointer] = g_hydrology.down_packet->end;
	pointer += 1;

	g_hydrology.down_packet->crc16 = crc16(0xFFFF, buffer, pointer);
	buffer[pointer] = g_hydrology.down_packet->crc16 >> 8;
	pointer += 1;
	buffer[pointer] = g_hydrology.down_packet->crc16 & 0xFF;
	pointer += 1;

	g_hydrology.down_packet->len = pointer;

	if (hydrology_port_transmmit(g_hydrology.down_packet->buffer,
			g_hydrology.down_packet->len) == false)
		return false;

	return true;
}

static int hydrology_host_make_err_down_tail_and_send(enum hydrology_mode mode,
	enum hydrology_body_type funcode, u8 Err_Packet)
{
	u8 *buffer;
	u16 i;
	u16 buffer_size;
	u16 pointer;
	u8 stime[6] = {0, 0, 0, 0, 0, 0};
	u8 ctime[6] = {0, 0, 0, 0, 0, 0};
	struct hydrology_down_header *header = (struct hydrology_down_header *)g_hydrology.down_packet->header;
	struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;

	buffer_size = header->len + down_body->len + 3;
	g_hydrology.down_packet->buffer = kmalloc(buffer_size, __GFP_ZERO);

	if (g_hydrology.down_packet->buffer == NULL) {
		pr_err("g_hydrology.down_packet->buffer malloc failed\n");
		return false;
	}

	buffer = g_hydrology.down_packet->buffer;

	hydrology_host_set_down_header_sequence(Err_Packet, 0);

	header->dir_len[0] |= (down_body->len) >> 8;
	header->dir_len[1] |= (down_body->len) & 0xFF;
	memcpy(buffer, header, sizeof(struct hydrology_down_header) - 1);
	pointer = sizeof(struct hydrology_down_header) - 1;

	switch (funcode) {
	case LINK_REPORT:
		break;

	case TEST_REPORT:
	case EVEN_PERIOD_INFO_REPORT:
	case TIMER_REPORT:
	case ADD_REPORT:
	case HOUR_REPORT:
	case ARTIFICIAL_NUM_REPORT:
	case PICTURE_REPORT:
	case REAL_TIME_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
	case SW_VERSION_REPORT:
	case STATUS_REPORT:
	case SET_CLOCK_REPORT:
	case RECORD_REPORT:
	case TIME_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;
		break;

	case PERIOD_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;
		hydrology_get_time(stime);
		memcpy(ctime, stime, 6);
		ctime[3]--;
		memcpy(&buffer[pointer], stime, 4);
		pointer += 4;
		memcpy(&buffer[pointer], ctime, 4);
		pointer += 4;

		memcpy(&buffer[pointer], down_body->element[0]->guide, 2);
		pointer += 2;
		memcpy(&buffer[pointer], down_body->element[0]->value, down_body->element[0]->num);
		pointer += down_body->element[0]->num;
		memcpy(&buffer[pointer], down_body->element[1]->guide, 2);
		pointer += 2;

		break;

	case SPECIFIED_ELEMENT_REPORT:
	case CONFIG_READ_REPORT:
	case PARA_READ_REPORT:
	case INIT_SOLID_STORAGE_REPORT:
	case RESET_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;

		for (i = 0; i < down_body->count; i++) {
			memcpy(&buffer[pointer], down_body->element[i]->guide, 2);
			pointer += 2;
		}

		break;

	case CONFIG_WRITE_REPORT:
	case PARA_WRITE_REPORT:
	case CHANGE_PASSWORD_REPORT:
	case SET_IC_CARD_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;

		for (i = 0; i < down_body->count; i++) {
			memcpy(&buffer[pointer], down_body->element[i]->guide, 2);
			pointer += 2;
			memcpy(&buffer[pointer], down_body->element[i]->value, down_body->element[i]->num);
			pointer += down_body->element[i]->num;
		}

		break;

	case PUMP_REPORT:
	case VALVE_REPORT:
	case GATE_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;

		if (mode == HYDROLOGY_M4) {
			memcpy(&buffer[pointer], down_body->element[0]->guide, 1);
			pointer += 1;
			memcpy(&buffer[pointer], down_body->element[0]->value, down_body->element[0]->num);
			pointer += down_body->element[0]->num;
		}

		break;

	case WATER_SETTING_REPORT:
		memcpy(&buffer[pointer], down_body, 8);
		pointer += 8;
		memcpy(&buffer[pointer], down_body->element[0]->value, down_body->element[0]->num);
		pointer += down_body->element[0]->num;
		break;
	}

	g_hydrology.down_packet->end = NAK;
	buffer[pointer] = g_hydrology.down_packet->end;
	pointer += 1;

	g_hydrology.down_packet->crc16 = crc16(0xFFFF, buffer, pointer);
	buffer[pointer] = g_hydrology.down_packet->crc16 >> 8;
	pointer += 1;
	buffer[pointer] = g_hydrology.down_packet->crc16 & 0xFF;
	pointer += 1;

	g_hydrology.down_packet->len = pointer;

	if (hydrology_port_transmmit(g_hydrology.down_packet->buffer,
			g_hydrology.down_packet->len) == false)
		return false;

	return true;
}

int hydrology_host_process_send(struct hydrology_element_info *element_table, u8 cnt,
	enum hydrology_mode mode, enum hydrology_body_type funcode, u8 end)
{
	if (hydrology_host_init_send(cnt, funcode) == false)
		return false;

	hydrology_host_make_down_header(mode, funcode);

	if (hydrology_host_make_down_body(element_table, mode, funcode) == false)
		return false;

	if (hydrology_host_make_down_tail_and_send(mode, funcode, end) == false)
		return false;

	hydrology_host_exit_send();

	return true;
}

static int hydrology_host_check_up_packet(u8 *input, int inputlen)
{
	u16 crcRet = 0;
	u16 inputCrc = 0;
	u16 bodylen = 0;

	crcRet = crc16(0xFFFF, input, inputlen - 2);

	inputCrc = (input[inputlen - 2] << 8) | input[inputlen - 1];

	if (crcRet != inputCrc) {
		pr_err("Device crc(0x%04x) != Host crc(0x%04x)\n",
			inputCrc, crcRet);
		pr_err("CRC check failed !\n");
		return false;
	}

	if ((input[0] != SOH) || (input[1] != SOH)) {
		pr_err("Device Frame head(0x%02x, 0x%02x) != Host Frame head(0x%02x, 0x%02x)\n",
			input[0], input[1], SOH, SOH);
		pr_err("Frame head check failed !\n");
		return false;
	}

	bodylen = (input[11] & 0x0F) * 256 + input[12];

	if (bodylen != (inputlen - 17)) {
		pr_err("Device length(0x%x) != Host length(0x%x)\n",
			bodylen, inputlen - 17);
		pr_err("Hydrolog length check failed !\n");
		return false;
	}

	return true;
}

static int hydrology_host_make_up_header(u8 *input, int inputlen, int *position, int *bodylen)
{
	struct hydrology_up_header *header = (struct hydrology_up_header *)g_hydrology.up_packet->header;

	if (hydrology_host_check_up_packet(input, inputlen) != true) {
		pr_err("Hydrology check fail !\n");
		return false;
	}

	memcpy(header->frame_start, &input[*position], 2);
	*position += 2;

	memcpy(&(header->center_addr), &input[*position], 1);
	*position += 1;

	memcpy(header->remote_addr, &input[*position], 5);
	*position += 5;

	memcpy(header->password, &input[*position], 2);
	*position += 2;

	memcpy(&(header->funcode), &input[*position], 1);
	*position += 1;

	memcpy(header->dir_len, &input[*position], 1);
	header->dir_len[0] >>= 4;

	*bodylen = (input[*position] & 0x0F) * 256 + input[*position + 1];
	*position += 2;

	memcpy(&(header->paket_start), &input[*position], 1);
	*position += 1;

	if (header->paket_start == SYN) {
		memcpy(header->count_seq, &input[*position], 3);
		*position += 3;
	}

	return true;
}

static int hydrology_host_make_up_body(u8 *input, int len, int position,
	enum hydrology_mode mode, enum hydrology_body_type funcode)
{
	u32 i, j, offset;
	int32_t tmp_len;
	u32 tmp_position;
	struct hydrology_up_body *upbody = (struct hydrology_up_body *)g_hydrology.up_packet->body;

	memcpy(upbody->stream_id, &input[position], 2);
	position += 2;
	len -= 2;

	memcpy(upbody->send_time, &input[position], 6);
	position += 6;
	len -= 6;

	switch (funcode) {
	case LINK_REPORT:
		break;

	case TEST_REPORT:
	case EVEN_PERIOD_INFO_REPORT:
	case TIMER_REPORT:
	case ADD_REPORT:
	case HOUR_REPORT:
	case PICTURE_REPORT:
	case REAL_TIME_REPORT:
	case PERIOD_REPORT:
	case SPECIFIED_ELEMENT_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
		memcpy(upbody->rtu_addr_id, &input[position], 2);
		position += 2;
		len -= 2;

		memcpy(upbody->rtu_addr, &input[position], 5);
		position += 5;
		len -= 5;

		memcpy(&upbody->rtu_type, &input[position], 1);
		position += 1;
		len -= 1;

		memcpy(upbody->observationtimeid, &input[position], 2);
		position += 2;
		len -= 2;

		memcpy(upbody->observation_time, &input[position], 5);
		position += 5;
		len -= 5;
		break;

	case CONFIG_WRITE_REPORT:
	case CONFIG_READ_REPORT:
	case PARA_WRITE_REPORT:
	case PARA_READ_REPORT:
	case SW_VERSION_REPORT:
	case STATUS_REPORT:
	case INIT_SOLID_STORAGE_REPORT:
	case RESET_REPORT:
	case CHANGE_PASSWORD_REPORT:
	case SET_CLOCK_REPORT:
	case SET_IC_CARD_REPORT:
	case PUMP_REPORT:
	case VALVE_REPORT:
	case GATE_REPORT:
	case WATER_SETTING_REPORT:
	case RECORD_REPORT:
	case TIME_REPORT:
		memcpy(upbody->rtu_addr_id, &input[position], 2);
		position += 2;
		len -= 2;

		memcpy(upbody->rtu_addr, &input[position], 5);
		position += 5;
		len -= 5;
		break;

	case ARTIFICIAL_NUM_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
		break;
	}

	upbody->count = 0;

	switch (funcode) {
	case LINK_REPORT:
		break;

	case TEST_REPORT:
	case TIMER_REPORT:
	case ADD_REPORT:
	case HOUR_REPORT:
	case REAL_TIME_REPORT:
	case PERIOD_REPORT:
	case SPECIFIED_ELEMENT_REPORT:
	case CONFIG_WRITE_REPORT:
	case PARA_WRITE_REPORT:
	case CONFIG_READ_REPORT:
	case PARA_READ_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
	case STATUS_REPORT:
	case CHANGE_PASSWORD_REPORT:
	case SET_IC_CARD_REPORT:
		tmp_len = len;
		tmp_position = position;

		while (tmp_len > 0) {
			offset = (input[tmp_position + 1] >> 3) + 2;
			tmp_position += offset;
			tmp_len -= offset;

			upbody->count++;
		}

		if (upbody->count == 0)
			return false;

		break;

	case EVEN_PERIOD_INFO_REPORT:
		tmp_len = len;
		tmp_position = position;
		offset = (input[tmp_position + 1] >> 3) + 2;
		tmp_position += offset;

		for (i = 1; offset < tmp_len; i++) {
			offset += (input[tmp_position + 1] >> 3) + 2;
			tmp_position += 2;
		}

		upbody->count = i;

		if (upbody->count == 0)
			return false;

		break;

	case ARTIFICIAL_NUM_REPORT:
	case PICTURE_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
	case SW_VERSION_REPORT:
	case PUMP_REPORT:
	case VALVE_REPORT:
	case GATE_REPORT:
	case WATER_SETTING_REPORT:
	case RECORD_REPORT:
		upbody->count = 1;
		break;

	case INIT_SOLID_STORAGE_REPORT:
	case RESET_REPORT:
	case SET_CLOCK_REPORT:
	case TIME_REPORT:
		break;
	}

	if (upbody->count > 0) {
		upbody->element = kmalloc(sizeof(struct hydrology_element *) * upbody->count, __GFP_ZERO);

		if (upbody->element == NULL) {
			pr_err("upbody->element malloc failed\n");
			return false;
		}
	}

	for (i = 0; i < upbody->count; ++i) {
		upbody->element[i] = kmalloc(sizeof(struct hydrology_element), __GFP_ZERO);

		if (upbody->element[i] == NULL) {
			pr_err("upbody->element[%d] malloc failed\n", i);
			return false;
		}
	}

	switch (funcode) {
	case LINK_REPORT:
		break;

	case TEST_REPORT:
	case TIMER_REPORT:
	case ADD_REPORT:
	case REAL_TIME_REPORT:
	case HOUR_REPORT:
	case PERIOD_REPORT:
	case SPECIFIED_ELEMENT_REPORT:
	case CONFIG_WRITE_REPORT:
	case CONFIG_READ_REPORT:
	case PARA_WRITE_REPORT:
	case PARA_READ_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
	case STATUS_REPORT:
	case CHANGE_PASSWORD_REPORT:
	case SET_IC_CARD_REPORT:
		for (i = 0; i < upbody->count; ++i) {
			memcpy(upbody->element[i]->guide, &input[position], 2);
			position += 2;
			len -= 2;

			upbody->element[i]->num =
				(upbody->element[i]->guide[1] >> 3);
			upbody->element[i]->value = kmalloc(upbody->element[i]->num, __GFP_ZERO);

			if (NULL == upbody->element[i]->value) {
				pr_err("upbody->element[%d]->value malloc failed\n", i);
				return false;
			}

			memcpy(upbody->element[i]->value, &input[position],
				upbody->element[i]->num);
			position += upbody->element[i]->num;
			len -= upbody->element[i]->num;
		}

		break;

	case EVEN_PERIOD_INFO_REPORT:
		for (i = 0; i < upbody->count; ++i) {
			memcpy(upbody->element[i]->guide, &input[position], 2);
			position += 2;
			len -= 2;

			upbody->element[i]->num =
				(upbody->element[i]->guide[1] >> 3);
			upbody->element[i]->value = kmalloc(upbody->element[i]->num, __GFP_ZERO);

			if (NULL == upbody->element[i]->value) {
				pr_err("upbody->element[%d]->value malloc failed\n", i);
				return false;
			}

			if (i == 0) {
				memcpy(upbody->element[i]->guide, &input[position],
					upbody->element[i]->num);
				position += upbody->element[i]->num;
				len -= upbody->element[i]->num;
			}
		}

		for (i = 0; i < 12; ++i) {
			for (j = 1; j < upbody->count; ++j) {
				memcpy(&upbody->element[j]->value[upbody->element[j]->num / 12 * i],
					&input[position], upbody->element[j]->num / 12);
				position += upbody->element[j]->num / 12;
				len -= upbody->element[j]->num / 12;
			}
		}

		break;

	case ARTIFICIAL_NUM_REPORT:
	case PICTURE_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
		memcpy(upbody->element[0]->guide, &input[position], 2);
		position += 2;
		len -= 2;

		upbody->element[0]->num = len;
		upbody->element[0]->value = kmalloc(upbody->element[0]->num, __GFP_ZERO);

		if (NULL == upbody->element[0]->value) {
			pr_err("upbody->element[0]->value malloc failed\n");
			return false;
		}

		memcpy(upbody->element[0]->value, &input[position],
			upbody->element[0]->num);
		position += upbody->element[0]->num;
		len -= upbody->element[0]->num;
		break;

	case SW_VERSION_REPORT:
	case PUMP_REPORT:
	case VALVE_REPORT:
		memcpy(upbody->element[0]->guide, &input[position], 1);
		position += 1;
		len -= 1;
		upbody->element[0]->num = upbody->element[0]->guide[0];
		upbody->element[0]->value = kmalloc(upbody->element[0]->num, __GFP_ZERO);

		if (NULL == upbody->element[0]->value) {
			pr_err("upbody->element[0]->value malloc failed\n");
			return false;
		}

		memcpy(upbody->element[0]->value, &input[position],
			upbody->element[0]->num);
		position += upbody->element[0]->num;
		len -= upbody->element[0]->num;
		break;

	case INIT_SOLID_STORAGE_REPORT:
	case RESET_REPORT:
	case SET_CLOCK_REPORT:
	case TIME_REPORT:
		break;

	case GATE_REPORT:
		memcpy(upbody->element[0]->guide, &input[position], 1);
		position += 1;
		len -= 1;

		if (upbody->element[0]->guide[0] % 8 == 0)
			upbody->element[0]->num = upbody->element[0]->guide[0] / 8;
		else
			upbody->element[0]->num = upbody->element[0]->guide[0] / 8 + 1;

		upbody->element[0]->value = kmalloc(upbody->element[0]->num, __GFP_ZERO);

		if (NULL == upbody->element[0]->value) {
			pr_err("upbody->element[0]->value malloc failed\n");
			return false;
		}

		memcpy(upbody->element[0]->value, &input[position],
			upbody->element[0]->num);
		position += upbody->element[0]->num;
		len -= upbody->element[0]->num;
		break;

	case WATER_SETTING_REPORT:
		upbody->element[0]->num = 1;
		upbody->element[0]->value = kmalloc(upbody->element[0]->num, __GFP_ZERO);

		if (NULL == upbody->element[0]->value) {
			pr_err("upbody->element[0]->value malloc failed\n");
			return false;
		}

		memcpy(upbody->element[0]->value, &input[position],
			upbody->element[0]->num);
		position += upbody->element[0]->num;
		len -= upbody->element[0]->num;
		break;

	case RECORD_REPORT:
		upbody->element[0]->num = 64;
		upbody->element[0]->value = kmalloc(upbody->element[0]->num, __GFP_ZERO);

		if (NULL == upbody->element[0]->value) {
			pr_err("upbody->element[0]->value malloc failed\n");
			return false;
		}

		memcpy(upbody->element[0]->value, &input[position],
			upbody->element[0]->num);
		position += upbody->element[0]->num;
		len -= upbody->element[0]->num;
		break;
	}

	return true;
}

extern float strtof(const char *str, char **endptr);

int hydrology_host_print_up_packet(void)
{
	struct hydrology_up_header *header = (struct hydrology_up_header *)g_hydrology.up_packet->header;
	struct hydrology_up_body *upbody = (struct hydrology_up_body *)g_hydrology.up_packet->body;
	u16 total_pkt, current_pkt;
	u16 i, j, k, cnt;
	u16 stream_id;
	struct hydrology_element_info *element_table;
	float value;
	char **pbuffer;
	u32 status_val;
	u16 record_val;
	char *version;

	pr_info("Center Address@%02X\n", header->center_addr);
	pr_info("Remote Address@%02X%02X%02X%02X%02X\n",
		header->remote_addr[0], header->remote_addr[1], header->remote_addr[2],
		header->remote_addr[3], header->remote_addr[4]);
	pr_info("Password: %02X%02X\n",
		header->password[0], header->password[1]);
	pr_info("Packet type: %s\n", hydrology_type_string(header->funcode));

	if (header->dir_len[0] & 0x80)
		pr_info("Downstream packet\n");
	else
		pr_info("Upstream packet\n");

	total_pkt = (header->count_seq[0] << 4) + (header->count_seq[1] >> 4);
	current_pkt = (header->count_seq[1] & 0x0F) + header->count_seq[2];
	pr_info("Total packet number: %u\n", total_pkt);
	pr_info("Current packet number: %u\n", current_pkt);

	stream_id = (upbody->stream_id[0] << 8) + upbody->stream_id[1];
	pr_info("Stream ID: %u\n", stream_id);
	pr_info("Packet send time: 20%02X/%02X/%02X %02X:%02X:%02X\n",
		upbody->send_time[0], upbody->send_time[1], upbody->send_time[2],
		upbody->send_time[3], upbody->send_time[4], upbody->send_time[5]);

	switch ((enum hydrology_body_type)header->funcode) {
	case LINK_REPORT:
	case ARTIFICIAL_NUM_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
		break;

	case CONFIG_WRITE_REPORT:
	case CONFIG_READ_REPORT:
	case PARA_WRITE_REPORT:
	case PARA_READ_REPORT:
	case SW_VERSION_REPORT:
	case STATUS_REPORT:
	case INIT_SOLID_STORAGE_REPORT:
	case RESET_REPORT:
	case SET_CLOCK_REPORT:
	case SET_IC_CARD_REPORT:
	case PUMP_REPORT:
	case VALVE_REPORT:
	case GATE_REPORT:
	case WATER_SETTING_REPORT:
	case RECORD_REPORT:
	case TIME_REPORT:
		pr_info("RTU Address@%02X%02X%02X%02X%02X\n",
			upbody->rtu_addr[0], upbody->rtu_addr[1], upbody->rtu_addr[2],
			upbody->rtu_addr[3], upbody->rtu_addr[4]);
		break;

	default:
		pr_info("RTU Address@%02X%02X%02X%02X%02X\n",
			upbody->rtu_addr[0], upbody->rtu_addr[1], upbody->rtu_addr[2],
			upbody->rtu_addr[3], upbody->rtu_addr[4]);
		pr_info("RTU type: %s\n", hydrology_type_rtu_string(upbody->rtu_type));
		pr_info("element sample time: 20%02X/%02X/%02X %02X:%02X\n",
			upbody->observation_time[0], upbody->observation_time[1], upbody->observation_time[2],
			upbody->observation_time[3], upbody->observation_time[4]);
		pr_info("element count: %u\n", upbody->count);
		break;
	}

	if (upbody->count != 0) {
		element_table = kmalloc(sizeof(struct hydrology_element_info) * upbody->count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}
	}

	switch ((enum hydrology_body_type)header->funcode) {
	case LINK_REPORT:
	case INIT_SOLID_STORAGE_REPORT:
	case RESET_REPORT:
	case SET_CLOCK_REPORT:
	case TIME_REPORT:
		break;

	case TEST_REPORT:
	case TIMER_REPORT:
	case ADD_REPORT:
	case REAL_TIME_REPORT:
	case SPECIFIED_ELEMENT_REPORT:
	case PARA_WRITE_REPORT:
	case PARA_READ_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
		pbuffer = kmalloc(sizeof(*pbuffer), __GFP_ZERO);

		if (NULL == pbuffer) {
			pr_err("pbuffer malloc failed\n");
			return false;
		}

		for (i = 0; i < upbody->count; ++i) {
			hydrology_read_specified_element_info(&element_table[i], (enum hydrology_body_type)header->funcode,
				upbody->element[i]->guide[0]);
			pr_info("element[%u].ID: %02X, D: %u, d: %u, addr@%08X\n",
				i, element_table[i].ID, element_table[i].D, element_table[i].d, element_table[i].addr);

			*pbuffer = kmalloc(upbody->element[i]->num, __GFP_ZERO);

			if (NULL == *pbuffer) {
				pr_err("*pbuffer malloc failed\n");
				return false;
			}

			memcpy(*pbuffer, upbody->element[i]->value, upbody->element[i]->num);
			value = strtof(*pbuffer, pbuffer);
			kfree(*pbuffer);

			for (j = 0; j < element_table[i].d; ++j)
				value /= 10;

			pr_info("element[%u].value: %f\n", i, value);
		}

		kfree(pbuffer);

		break;

	case EVEN_PERIOD_INFO_REPORT:
		hydrology_read_specified_element_info(&element_table[0], (enum hydrology_body_type)header->funcode,
			upbody->element[0]->guide[0]);
		pr_info("element[0].ID: %02X, D: %u, d: %u, addr@%08X\n",
			element_table[0].ID, element_table[0].D, element_table[0].d, element_table[0].addr);
		pr_info("TIME_REPORT step: %u:%u:%u\n",
			upbody->element[0]->value[0], upbody->element[0]->value[1], upbody->element[0]->value[2]);

		for (i = 1; i < upbody->count; ++i) {
			hydrology_read_specified_element_info(&element_table[i], (enum hydrology_body_type)header->funcode,
				upbody->element[i]->guide[0]);
			pr_info("element[%u].ID: %02X, D: %u, d: %u, addr@%08X\n",
				i, element_table[i].ID, element_table[i].D, element_table[i].d, element_table[i].addr);

			pr_info("element[%u].value: \n", i);

			for (j = 0, k = 0; j < 12; ++j, k += 2) {
				if (upbody->element[i]->num == 12)
					pr_info("[%u]%02X\n", j, upbody->element[i]->value[j]);
				else
					pr_info("[%u]%02X%02X\n", j,
						upbody->element[i]->value[k], upbody->element[i]->value[k + 1]);
			}
		}

		break;

	case HOUR_REPORT:
		for (i = 0; i < upbody->count; ++i) {
			hydrology_read_specified_element_info(&element_table[i], (enum hydrology_body_type)header->funcode,
				upbody->element[i]->guide[0]);
			pr_info("element[%u].ID: %02X, D: %u, d: %u, addr@%08X\n",
				i, element_table[i].ID, element_table[i].D, element_table[i].d, element_table[i].addr);

			pr_info("element[%u].value: \n", i);

			for (j = 0; j < 12; ++j, k += 2) {
				if (upbody->element[i]->num == 12)
					pr_info("[%u]%02X\n", j, upbody->element[i]->value[j]);
				else
					pr_info("[%u]%02X%02X\n", j,
						upbody->element[i]->value[k], upbody->element[i]->value[k + 1]);
			}
		}

		break;

	case ARTIFICIAL_NUM_REPORT:
	case PICTURE_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
		break;

	case PERIOD_REPORT:
		pr_info("TIME_REPORT step: %u:%u:%u\n",
			upbody->element[0]->value[0], upbody->element[0]->value[1], upbody->element[0]->value[2]);

		hydrology_read_specified_element_info(&element_table[1], (enum hydrology_body_type)header->funcode,
			upbody->element[1]->guide[0]);
		pr_info("element[1].ID: %02X, D: %u, d: %u, addr@%08X\n",
			element_table[1].ID, element_table[1].D, element_table[1].d, element_table[1].addr);

		pr_info("element[1].value: \n");

		for (j = 0; j < 12; ++j, k += 2) {
			if (upbody->element[1]->num == 12)
				pr_info("[%u]%02X\n", j, upbody->element[1]->value[j]);
			else
				pr_info("[%u]%02X%02X\n", j,
					upbody->element[1]->value[k], upbody->element[1]->value[k + 1]);
		}

		break;

	case CONFIG_WRITE_REPORT:
	case CONFIG_READ_REPORT:
		for (i = 0; i < upbody->count; ++i) {
			hydrology_read_specified_element_info(&element_table[i], (enum hydrology_body_type)header->funcode,
				upbody->element[i]->guide[0]);
			pr_info("element[%u].ID: %02X, D: %u, d: %u, addr@%08X\n",
				i, element_table[i].ID, element_table[i].D, element_table[i].d, element_table[i].addr);

			pr_info("element[%u].value: \n", i);

			for (j = 0; j < upbody->element[i]->num; ++j)
				printk("%02X\n", upbody->element[i]->value[j]);
		}

		break;

	case SW_VERSION_REPORT:
		version = kmalloc(upbody->element[0]->num + 1, __GFP_ZERO);
		memcpy(version, upbody->element[0]->value, upbody->element[0]->num);
		pr_info("Software version: %s\n", version);
		kfree(version);
		break;

	case STATUS_REPORT:
	case SET_IC_CARD_REPORT:
		hydrology_read_specified_element_info(&element_table[0], (enum hydrology_body_type)header->funcode,
			upbody->element[0]->guide[0]);
		pr_info("element[0].ID: %02X, D: %u, d: %u, addr@%08X\n",
			element_table[0].ID, element_table[0].D, element_table[0].d, element_table[0].addr);

		status_val = *((u32 *)upbody->element[0]->value);

		if (status_val & 0x0001)
			pr_info("BIT[0]: 1, AC charging status: Power off\n");
		else
			pr_info("BIT[0]: 0, AC charging status: Normal\n");

		if (status_val & 0x0002)
			pr_info("BIT[1]: 1, Battery voltage status: Low power\n");
		else
			pr_info("BIT[1]: 0, Battery voltage status: Normal\n");

		if (status_val & 0x0004)
			pr_info("BIT[2]: 1, Water level over limit alarm status: Alert\n");
		else
			pr_info("BIT[2]: 0, Water level over limit alarm status: Normal\n");

		if (status_val & 0x0008)
			pr_info("BIT[3]: 1, Flow overrun alarm status: Alert\n");
		else
			pr_info("BIT[3]: 0, Flow overrun alarm status: Normal\n");

		if (status_val & 0x0010)
			pr_info("BIT[4]: 1, Water quality limit alarm status: Alert\n");
		else
			pr_info("BIT[4]: 0, Water quality limit alarm status: Normal\n");

		if (status_val & 0x0020)
			pr_info("BIT[5]: 1, Flow meter status: Broken\n");
		else
			pr_info("BIT[5]: 0, Flow meter status: Normal\n");

		if (status_val & 0x0040)
			pr_info("BIT[6]: 1, Water level meter status: Broken\n");
		else
			pr_info("BIT[6]: 0, Water level meter status: Normal\n");

		if (status_val & 0x0080)
			pr_info("BIT[7]: 1, Terminal box door status: Shut off\n");
		else
			pr_info("BIT[7]: 0, Terminal box door status: Power on\n");

		if (status_val & 0x0100)
			pr_info("BIT[8]: 1, Memory status: Abnormal\n");
		else
			pr_info("BIT[8]: 0, Memory status: Normal\n");

		if (status_val & 0x0200)
			pr_info("BIT[9]: 1, IC card function is effective: IC Card normal\n");
		else
			pr_info("BIT[9]: 0, IC card function is effective: Shut off\n");

		if (status_val & 0x0400)
			pr_info("BIT[10]: 1, Working state of water pump: Water pump power off\n");
		else
			pr_info("BIT[10]: 0, Working state of water pump: Water pump power on\n");

		if (status_val & 0x0800)
			pr_info("BIT[11]: 1, Remaining water alarm: Water yield overlimit\n");
		else
			pr_info("BIT[11]: 0, Remaining water alarm: Water yield normal\n");

		break;

	case CHANGE_PASSWORD_REPORT:
		hydrology_read_specified_element_info(&element_table[0], (enum hydrology_body_type)header->funcode,
			upbody->element[0]->guide[0]);
		pr_info("element[0].ID: %02X, D: %u, d: %u, addr@%08X\n",
			element_table[0].ID, element_table[0].D, element_table[0].d, element_table[0].addr);
		pr_info("New password: %02X%02X\n",
			upbody->element[0]->value[0], upbody->element[0]->value[1]);

		break;

	case PUMP_REPORT:
		pr_info("Total count: %u\n", upbody->element[0]->guide[0] * 8);

		for (i = 0; i < upbody->element[0]->guide[0]; ++i) {
			for (j = 0; j < 8; ++j) {
				if (upbody->element[0]->value[i] & (1 << j))
					pr_info("Pump[%u]: Open\n", i * 8 + j);
				else
					pr_info("Pump[%u]: Close\n", i * 8 + j);
			}
		}

		break;

	case VALVE_REPORT:
		pr_info("Total count: %u\n", upbody->element[0]->guide[0] * 8);

		for (i = 0; i < upbody->element[0]->guide[0]; ++i) {
			for (j = 0; j < 8; ++j) {
				if (upbody->element[0]->value[i] & (1 << j))
					pr_info("VALVE_REPORT[%u]: Open\n", i * 8 + j);
				else
					pr_info("VALVE_REPORT[%u]: Close\n", i * 8 + j);
			}
		}

		break;

	case GATE_REPORT:
		pr_info("Total count: %u\n", upbody->element[0]->guide[0]);

		if (upbody->element[0]->guide[0] % 8 == 0)
			cnt = upbody->element[0]->guide[0] / 8;
		else
			cnt = upbody->element[0]->guide[0] / 8 + 1;

		for (i = 0, k = 0; i < cnt; ++i) {
			for (j = 0; j < 8; ++j, ++k) {
				if (k == upbody->element[0]->guide[0])
					break;

				if (upbody->element[0]->value[i] & (1 << j))
					pr_info("GATE_REPORT[%u]: Open\n", k);
				else
					pr_info("GATE_REPORT[%u]: Close\n", k);
			}
		}

		break;

	case WATER_SETTING_REPORT:
		if (upbody->element[0]->value[0])
			pr_info("Water value: Enter\n");
		else
			pr_info("Water value: Exit\n");

		break;

	case RECORD_REPORT:
		record_val = (upbody->element[0]->value[0] << 8) + upbody->element[0]->value[1];
		pr_info("ERC1: Historical data initialization record: %u\n", record_val);
		record_val = (upbody->element[0]->value[2] << 8) + upbody->element[0]->value[3];
		pr_info("ERC2: Parameter change record: %u\n", record_val);
		record_val = (upbody->element[0]->value[4] << 8) + upbody->element[0]->value[5];
		pr_info("ERC3: State quantity displacement record: %u\n", record_val);
		record_val = (upbody->element[0]->value[6] << 8) + upbody->element[0]->value[7];
		pr_info("ERC4: Sensor and instrument fault record: %u\n", record_val);
		record_val = (upbody->element[0]->value[8] << 8) + upbody->element[0]->value[9];
		pr_info("ERC5: Password modification record: %u\n", record_val);
		record_val = (upbody->element[0]->value[10] << 8) + upbody->element[0]->value[11];
		pr_info("ERC6: Terminal fault record: %u\n", record_val);
		record_val = (upbody->element[0]->value[12] << 8) + upbody->element[0]->value[13];
		pr_info("ERC7: AC power loss record: %u\n", record_val);
		record_val = (upbody->element[0]->value[14] << 8) + upbody->element[0]->value[15];
		pr_info("ERC8: Low battery voltage alarm record: %u\n", record_val);
		record_val = (upbody->element[0]->value[16] << 8) + upbody->element[0]->value[17];
		pr_info("ERC9: Illegal opening record of terminal box: %u\n", record_val);
		record_val = (upbody->element[0]->value[18] << 8) + upbody->element[0]->value[19];
		pr_info("ERC10: Water pump fault record: %u\n", record_val);
		record_val = (upbody->element[0]->value[20] << 8) + upbody->element[0]->value[21];
		pr_info("ERC11: The remaining water volume exceeds the limit alarm record: %u\n", record_val);
		record_val = (upbody->element[0]->value[22] << 8) + upbody->element[0]->value[23];
		pr_info("ERC12: Water level over-limit alarm record: %u\n", record_val);
		record_val = (upbody->element[0]->value[24] << 8) + upbody->element[0]->value[25];
		pr_info("ERC13: Water pressure limit alarm record: %u\n", record_val);
		record_val = (upbody->element[0]->value[26] << 8) + upbody->element[0]->value[27];
		pr_info("ERC14: Water quality parameter exceeding limit alarm record: %u\n", record_val);
		record_val = (upbody->element[0]->value[28] << 8) + upbody->element[0]->value[29];
		pr_info("ERC15: Data error record: %u\n", record_val);
		record_val = (upbody->element[0]->value[30] << 8) + upbody->element[0]->value[31];
		pr_info("ERC16: Message record: %u\n", record_val);
		record_val = (upbody->element[0]->value[32] << 8) + upbody->element[0]->value[33];
		pr_info("ERC17: Receive message record: %u\n", record_val);
		record_val = (upbody->element[0]->value[34] << 8) + upbody->element[0]->value[35];
		pr_info("ERC18: Send message error record: %u\n", record_val);
		record_val = (upbody->element[0]->value[36] << 8) + upbody->element[0]->value[37];
		break;
	}

	return true;
}

int hydrology_host_process_receieve(u8 *input, int inputlen, enum hydrology_mode mode)
{
	struct hydrology_up_header *header = NULL;
	int i = 0, bodylen = 0;

	if (hydrology_host_init_receieve() == false)
		return false;

	header = (struct hydrology_up_header *)g_hydrology.up_packet->header;

	if (hydrology_host_make_up_header(input, inputlen, &i, &bodylen) == false)
		return false;

	if (hydrology_host_make_up_body(input, bodylen, i, mode, (enum hydrology_body_type)header->funcode) == false)
		return false;

	if (hydrology_host_print_up_packet() == false)
		return false;

	g_hydrology.up_packet->end = input[inputlen - 3];

	switch (g_hydrology.up_packet->end) {
	case ETX:
		pr_err("[ETX]Wait disconnecting...\n");
		hydrology_disable_link_packet();

		switch (mode) {
		case HYDROLOGY_M1:
		case HYDROLOGY_M4:
			break;

		case HYDROLOGY_M2:
		case HYDROLOGY_M3:
			hydrology_response_upstream((enum hydrology_body_type)header->funcode, EOT);
			break;
		}

		break;

	case ETB:
		pr_err("[ETB]Stay connecting...\n");
		hydrology_enable_link_packet();

		switch (mode) {
		case HYDROLOGY_M1:
		case HYDROLOGY_M4:
			break;

		case HYDROLOGY_M2:
		case HYDROLOGY_M3:
			hydrology_response_upstream((enum hydrology_body_type)header->funcode, ACK);
			break;
		}

		break;

	default:
		printk(KERN_ERR "Unknown end packet identifier\n");
		break;
	}

	printk(KERN_ERR " \n");

	hydrology_host_exit_receieve();

	return true;
}

void hydrology_host_process_end_identifier(u8 End)
{
	switch (End) {
	case ETX:
		pr_err("[ETX]Wait disconnecting...\n");
		hydrology_disable_link_packet();
		break;

	case ETB:
		pr_err("[ETB]Stay connecting...\n");
		hydrology_enable_link_packet();
		break;

	default:
		printk(KERN_ERR "Unknown end packet identifier\n");
		break;
	}
}

int hydrology_host_process_m3_err_packet(struct hydrology_element_info *element_table, u8 cnt,
	enum hydrology_body_type funcode, u8 cerr, u16 Err_Packet)
{
	u8 **ppdata;
	u16 length;

	if (hydrology_host_init_send(cnt, funcode) == false)
		return false;

	hydrology_host_make_down_header(HYDROLOGY_M3, funcode);

	if (hydrology_host_make_down_body(element_table, HYDROLOGY_M3, funcode) == false)
		return false;

	if (hydrology_host_make_err_down_tail_and_send(HYDROLOGY_M3, funcode, Err_Packet) == false)
		return false;

	hydrology_host_exit_send();

	cerr++;

	if (hydrology_port_receive(ppdata, &length, HYDROLOGY_D_PORT_TIMEOUT) == true) {
		if (hydrology_host_process_receieve(*ppdata, length, HYDROLOGY_M3) == true) {
			switch (ppdata[0][length - 3]) {
			case EOT:
				pr_err("[EOT]Link is disconnecting\n");
				hydrology_disconnect_link();
				break;

			case ESC:
				pr_err("[ESC]Transfer is over, keep on live within 10 minutes\n");
				hydrology_enable_link_packet();
				break;

			default:
				printk(KERN_ERR "Unknown end packet identifier\n");
				break;
			}
		} else
			return false;
	} else {
		pr_err("Receive data timeout, retry times %d.\n",
			cerr);

		if (cerr >= 3) {
			hydrology_disconnect_link();
			return false;
		}

		hydrology_host_process_m3_err_packet(element_table, cnt, funcode, cerr, Err_Packet);
	}

	return true;
}

int hydrology_host_process_m1m2(enum hydrology_mode mode)
{
	u8 **ppdata;
	u16 length;

	for (;;) {
		if (hydrology_port_receive(ppdata, &length, HYDROLOGY_H_PORT_TIMEOUT) == true)
			hydrology_host_process_receieve(*ppdata, length, mode);
		else {
			pr_err("[Warning]Port is going to be closed.\n");
			return false;
		}
	}
}

int hydrology_host_process_m3(void)
{
	u8 cerr = 0;
	u8 **ppdata;
	u16 length;
	u16 i, packet_cnt;
	u32 bit_map[128];

	memset(bit_map, 0, sizeof(bit_map));

	do {
		if (hydrology_port_receive(ppdata, &length, HYDROLOGY_H_PORT_TIMEOUT) == true) {
			if (hydrology_host_process_receieve(*ppdata, length, HYDROLOGY_M3) == true)
				hydrology_host_process_end_identifier(ppdata[0][length - 3]);
			else {
				bit_map[packet_cnt / 32] = 1 << (packet_cnt % 32);
				return false;
			}

			packet_cnt++;
		} else {
			pr_err("Receive data timeout.\n");
			return false;
		}
	} while (ppdata[0][length - 3] == ETB);

	for (i = 0; i < packet_cnt; i++) {
		if (bit_map[i / 32] & (1 << (i % 32))) {
			pr_err("Packet %u error, request device to resend\n", i);
			hydrology_host_process_m3_err_packet(NULL, 0, (enum hydrology_body_type)ppdata[0][10], cerr, i + 1);
		}
	}

	return true;
}

int hydrology_host_process_m4(struct hydrology_element_info *element_table, u8 cnt,
	enum hydrology_body_type funcode)
{
	u8 **ppdata;
	u16 length;

	if (hydrology_host_process_send(element_table, cnt, HYDROLOGY_M4, funcode, ENQ) == false)
		return false;

	do {
		if (hydrology_port_receive(ppdata, &length, HYDROLOGY_H_PORT_TIMEOUT) == true) {
			if (hydrology_host_process_receieve(*ppdata, length, HYDROLOGY_M4) == true)
				hydrology_host_process_end_identifier(ppdata[0][length - 3]);
			else
				return false;
		} else {
			pr_err("Receive data timeout.\n");
			return false;
		}
	} while (ppdata[0][length - 3] == ETB);

	return true;
}

int hydrology_host_process(struct hydrology_element_info *element_table, u8 cnt,
	enum hydrology_mode mode, enum hydrology_body_type funcode)
{
	u8 ret = false;

	switch (mode) {
	case HYDROLOGY_M1:
		ret = hydrology_host_process_m1m2(HYDROLOGY_M1);
		break;

	case HYDROLOGY_M2:
		ret = hydrology_host_process_m1m2(HYDROLOGY_M2);
		break;

	case HYDROLOGY_M3:
		ret = hydrology_host_process_m3();
		break;

	case HYDROLOGY_M4:
		ret = hydrology_host_process_m4(element_table, cnt, funcode);
		break;
	}

	return ret;
}

#ifdef DESIGN_VERIFICATION_HYDROLOGY
#include "kinetis/test-kinetis.h"
#include "kinetis/random-gene.h"

int t_hydrology_host_m1m2m3(enum hydrology_mode mode)
{
	g_hydrology.source = MSG_FORM_CLIENT;

	return hydrology_host_process(NULL, 0, mode, (enum hydrology_body_type)NULL);
}

int t_hydrology_host_random_element(enum hydrology_mode mode, enum hydrology_body_type funcode)
{
	struct hydrology_element_info *element_table;
	u8 s_guide[] = {0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC};
	u8 i, j, count, guide;
	int ret;

	switch (funcode) {
	case LINK_REPORT:
	case TEST_REPORT:
	case EVEN_PERIOD_INFO_REPORT:
	case TIMER_REPORT:
	case ADD_REPORT:
	case HOUR_REPORT:
	case ARTIFICIAL_NUM_REPORT:
	case PICTURE_REPORT:
		ret = true;
		break;

	case PERIOD_REPORT:
		count = 2;
		element_table = kmalloc(sizeof(struct hydrology_element_info) * count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}

		hydrology_read_specified_element_info(&element_table[0], funcode, 0x04);
		j = random_get8bit() % (sizeof(s_guide) - 1);
		hydrology_read_specified_element_info(&element_table[1], funcode, s_guide[j]);

		ret = hydrology_host_process(element_table, count, mode, funcode);

		kfree(element_table);
		break;

	case SPECIFIED_ELEMENT_REPORT:
		count = random_get8bit() % (117 - 100);

		if (count == 0)
			count = 1;

		element_table = kmalloc(sizeof(struct hydrology_element_info) * count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}

		for (i = 0;;) {
			guide = random_get8bit() % 0x75;

			if (!guide)
				continue;

			hydrology_read_specified_element_info(&element_table[i], funcode, guide);

			if (i == count - 1)
				break;
			else
				i++;
		}

		ret = hydrology_host_process(element_table, count, mode, funcode);

		kfree(element_table);
		break;

	case CONFIG_WRITE_REPORT:
	case CONFIG_READ_REPORT:
		count = random_get8bit() % 15;

		if (count == 0)
			count = 1;

		element_table = kmalloc(sizeof(struct hydrology_element_info) * count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}

		for (i = 0;;) {
			guide = random_get8bit() % 0x0F;

			if (!guide)
				continue;

			hydrology_read_specified_element_info(&element_table[i], funcode, guide);

			if (i == count - 1)
				break;
			else
				i++;
		}

		ret = hydrology_host_process(element_table, count, mode, funcode);

		kfree(element_table);
		break;

	case PARA_WRITE_REPORT:
	case PARA_READ_REPORT:
		count = random_get8bit() % (137 - 120);

		if (count == 0)
			count = 1;

		element_table = kmalloc(sizeof(struct hydrology_element_info) * count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}

		for (i = 0;;) {
			guide = random_get8bit() % 0xA8;

			if (guide < 0x20)
				continue;

			hydrology_read_specified_element_info(&element_table[i], funcode, guide);

			if (i == count - 1)
				break;
			else
				i++;
		}

		ret = hydrology_host_process(element_table, count, mode, funcode);

		kfree(element_table);
		break;

	case STATUS_REPORT:
	case SET_IC_CARD_REPORT:
		count = 1;
		element_table = kmalloc(sizeof(struct hydrology_element_info) * count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}

		hydrology_read_specified_element_info(&element_table[0], funcode, 0x45);

		ret = hydrology_host_process(element_table, count, mode, funcode);

		kfree(element_table);
		break;

	case INIT_SOLID_STORAGE_REPORT:
		count = 1;
		element_table = kmalloc(sizeof(struct hydrology_element_info) * count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}

		hydrology_read_specified_element_info(&element_table[0], funcode, 0x97);

		ret = hydrology_host_process(element_table, count, mode, funcode);

		kfree(element_table);
		break;

	case RESET_REPORT:
		count = 1;
		element_table = kmalloc(sizeof(struct hydrology_element_info) * count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}

		hydrology_read_specified_element_info(&element_table[0], funcode, 0x98);

		ret = hydrology_host_process(element_table, count, mode, funcode);

		kfree(element_table);
		break;

	case CHANGE_PASSWORD_REPORT:
		count = 2;
		element_table = kmalloc(sizeof(struct hydrology_element_info) * count, __GFP_ZERO);

		if (element_table == NULL) {
			pr_err("element_table malloc failed\n");
			return false;
		}

		hydrology_read_specified_element_info(&element_table[0], funcode, 0x03);
		hydrology_read_specified_element_info(&element_table[1], funcode, 0xB7);

		ret = hydrology_host_process(element_table, count, mode, funcode);

		kfree(element_table);
		break;

	case REAL_TIME_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
	case SW_VERSION_REPORT:
	case SET_CLOCK_REPORT:
	case PUMP_REPORT:
	case VALVE_REPORT:
	case GATE_REPORT:
	case WATER_SETTING_REPORT:
	case RECORD_REPORT:
	case TIME_REPORT:
		ret = hydrology_host_process(NULL, 0, mode, funcode);
		break;
	}

	return ret;
}

int t_hydrology_host_m4(void)
{
	if (t_hydrology_host_random_element(HYDROLOGY_M4, REAL_TIME_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, PERIOD_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, INQUIRE_ARTIFICIAL_NUM_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, SPECIFIED_ELEMENT_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, CONFIG_WRITE_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, CONFIG_READ_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, PARA_WRITE_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, PARA_READ_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, WATER_PUMP_MOTOR_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, SW_VERSION_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, STATUS_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, INIT_SOLID_STORAGE_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, RESET_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, CHANGE_PASSWORD_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, SET_CLOCK_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, SET_IC_CARD_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, PUMP_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, VALVE_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, GATE_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, WATER_SETTING_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, RECORD_REPORT) == false)
		return false;

	if (t_hydrology_host_random_element(HYDROLOGY_M4, TIME_REPORT) == false)
		return false;

	return true;
}

#endif

