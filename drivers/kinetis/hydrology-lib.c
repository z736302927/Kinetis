
#include <generated/deconfig.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/crc16.h>
#include <linux/errno.h>

#include <kinetis/design_verification.h>

#include "hydrology.h"
#include "hydrology-config.h"
#include "hydrology-cmd.h"
#include "hydrology-identifier.h"

#include <kinetis/hydrology-task.h>
#include <kinetis/tim-task.h>
#include <kinetis/real-time-clock.h>


/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  .
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

void hydrology_get_time(u8 *time)
{
	struct tm rtc;

	rtc_calendar_get(&rtc, KRTC_FORMAT_BCD);
	time[0] = rtc.tm_year;
	time[1] = rtc.tm_mon;
	time[2] = rtc.tm_mday;
	time[3] = rtc.tm_hour;
	time[4] = rtc.tm_min;
	time[5] = rtc.tm_sec;
}

void hydrology_set_time(u8 *time)
{
	struct tm rtc;

	rtc.tm_year = time[0];
	rtc.tm_mon = time[1];
	rtc.tm_mday = time[2];
	rtc.tm_hour = time[3];
	rtc.tm_min = time[4];
	rtc.tm_sec = time[5];
	rtc.tm_wday = 0;
	rtc_calendar_set(&rtc, KRTC_FORMAT_BCD);
}

extern void link_packet(struct tim_task *task);

void hydrology_disable_link_packet(void)
{
	tim_task_drop(&g_hydrology.link_pkt);
}

void hydrology_enable_link_packet(void)
{
	tim_task_add(&g_hydrology.link_pkt, "link packet",
		40 * 1000, true, false, link_packet);
}

void hydrology_disconnect_link(void)
{

}

int hydrology_open_port(void)
{
	switch (g_hydrology.source) {
	case MSG_FORM_SERVER:
		break;

	case MSG_FORM_CLIENT:
		break;
	}

	return true;
}

int hydrology_close_port(void)
{
	switch (g_hydrology.source) {
	case MSG_FORM_SERVER:
		break;

	case MSG_FORM_CLIENT:
		break;
	}

	return true;
}

int hydrology_port_transmmit(u8 *pdata, u16 length)
{
//	struct serial_port hydrology_port;

	print_hex_dump(KERN_DEBUG, "hydrology tx: ", DUMP_PREFIX_OFFSET,
		16, 1,
		pdata, length, false);
	pr_err(" \n");

//	hydrology_open_port();

	switch (g_hydrology.source) {
	case MSG_FORM_SERVER:
//            NB_IOT_SendData(Data, length);
		g_hydrology.source = MSG_FORM_SERVER;
		break;

	case MSG_FORM_CLIENT:
//		hydrology_port.port_nbr = 2;
//		hydrology_port.tx_buffer = pdata;
//		hydrology_port.tx_buffer_size = length;
//		serial_port_send(&hydrology_port);
		break;
	}

	return true;
}

int hydrology_port_receive(u8 **ppdata, u16 *plength, u32 Timeout)
{
//	struct serial_port hydrology_port;
	u32 Refer = 0;
	u32 Delta = 0;
	int ret;

	switch (g_hydrology.source) {
	case MSG_FORM_SERVER:
		g_hydrology.source = MSG_FORM_SERVER;
		break;

	case MSG_FORM_CLIENT:
//		memset(&hydrology_port, 0, sizeof(struct serial_port));
//		hydrology_port.port_nbr = 2;
//		hydrology_port.tmp_buffer_size = 300;
//		hydrology_port.rx_scan_interval = 10;
//		hydrology_port.end_char = NULL;
//		hydrology_port.end_char_size = 0;
//		serial_port_open(&hydrology_port);

//		Refer = basic_timer_get_ss();

//		for (;;) {
//			if (serial_port_receive(&hydrology_port) == true) {
//				print_hex_dump(KERN_DEBUG, "hydrology rx: ", DUMP_PREFIX_OFFSET,
//					16, 1,
//					hydrology_port.rx_buffer, hydrology_port.rx_buffer_size, false);
//				*ppdata = (u8 *)hydrology_port.rx_buffer;
//				*plength = hydrology_port.rx_buffer_size;
//				serial_port_close(&hydrology_port);
//				ret = true;
//				break;
//			} else {
//				Delta = basic_timer_get_ss() >= Refer ?
//					basic_timer_get_ss() - Refer :
//					basic_timer_get_ss() + (DELAY_TIMER_UNIT - Refer);

//				if (Delta > Timeout) {
//					pr_err("[warning]Receive data timeout.\n");
//					serial_port_close(&hydrology_port);
//					ret = false;
//					break;
//				}
//			}
//		}

		break;
	}

	return ret;
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

struct hydrology g_hydrology;

int hydrology_resource_init(void)
{
	int ret;
	u8 tmp;
	u32 flash_size, min_size;

	flash_size = fatfs_get_flash_size();

	min_size = HYDROLOGY_END;
	min_size += sizeof(struct hydrology_element_info) * ELEMENT_COUNT;
	min_size += HYDROLOGY_D_PIC_REVSPACE;
	min_size += HYDROLOGY_D_RGZS_REVSPACE;

	if (min_size >= flash_size) {
		pr_err("ERR Current flash size is %.2f KB\n", (float)flash_size / 1024);
		pr_err("ERR Flash size minimum requirement %.2f KB\n", (float)min_size / 1024);
		return -ENOMEM;
	}

	ret = fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
			HYDROLOGY_PDA_INIT_MARK, &tmp, 1);

	if (ret) {
		pr_err("It is first time to use hydrology device\n");
		pr_err("// Writing to flash\n");
		ret = hydrology_device_reset();

		if (ret)
			goto err;

		ret = hydrology_host_reset();

		if (ret)
			goto err;

		pr_err("// Wrote to flash.\n");
	}

	return 0;
err:
	pr_err("Failed to init hydrology resource, "
		"error code: %d\n", ret);

	return ret;
}

void hydrology_device_shundown(void)
{
	hydrology_task_exit();
}

int hydrology_device_setup(void)
{
	int ret;

	ret = hydrology_task_init();

	if (ret)
		return ret;

	ret = hydrology_resource_init();

	return ret;
}

int hydrology_device_reboot(void)
{
	static bool status = false;
	int ret;

	if (status == true)
		hydrology_device_shundown();

	ret = hydrology_device_setup();
	status = true;

	return ret;
}

void hydrology_get_observation_time(struct hydrology_element_info *element, u8 *observation_time)
{
	u32 addr = 0;

	if (element->D % 2 == 0)
		addr = element->D / 2 + element->addr;
	else
		addr = (element->D + 1) / 2 + element->addr;

	fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		addr, observation_time, 5);
}

void hydrology_set_observation_time(struct hydrology_element_info *element)
{
	long addr = 0;
	u8 observation_time[6] = {0, 0, 0, 0, 0, 0};

	if (element->D % 2 == 0)
		addr = element->D / 2 + element->addr;
	else
		addr = (element->D + 1) / 2 + element->addr;

	hydrology_get_time(observation_time);
	fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
		addr, observation_time, 5);
}

void hydrology_get_bcd_nums(double num, int *intergerpart, int *decimerpart,
	int d, char *pout_intergerValue, u8 *pout_decimerValue)
{
	char strfloat[20];
	int i = 0;
	int len = 0;
	int j = 0;
	int k = 0;

	for (i = 0; i < 20; i++)
		strfloat[i] = 'X';

	sprintf(strfloat, "%f", num);

	for (i = 0; i < 20; i++) {
		if ('X' != strfloat[i])
			len++;
	}

	for (i = 0; i < len; i++) {
		if ('0' != strfloat[i])
			break;
	}

	len = len - i - 1;

	for (i = 0; i < len; i++) {
		if ('.' == strfloat[i]) {
			j = i;
			break;
		}
	}

	if (i < len) {
		*decimerpart = len - j - 1;

		if (*decimerpart > d)
			*decimerpart = d;

		*intergerpart = j;

		for (i = 0; i < j; i++)
			pout_intergerValue[i] = strfloat[i];

		pout_intergerValue[j] = 0;

		for (i = j + 1; (i < len) && (k < (*decimerpart)); i++)
			pout_decimerValue[k++] = strfloat[i];

		pout_decimerValue[k] = 0;
	} else {
		*decimerpart = 0;
		*intergerpart = len;
		sprintf(pout_intergerValue, "%d", (int)num);
	}
}

int hydrology_get_even_num(int num)
{
	if (num % 2 == 0)
		return num;
	else
		return num + 1;
}

/* The byte is 5 bits high, D represents the number of data bytes, and 3 bits low,
 * D represents the number of decimal place.
 */
void hydrology_get_guide_id(u8 *value, u8 D, u8 d)
{
	u8 high5 = 0;
	u8 low3 = 0;
	u8 evenD = 0;

	evenD = hydrology_get_even_num(D);

	high5 = evenD / 2;
	high5 = high5 << 3;
	/* D has to be between 0 and 7 to be represented in 3 digits. */
	low3 = d;
	*value = high5 | low3;
}

int hydrology_convert_to_hex_element(double input, int D, int d, u8 *out)
{
	/* StrInterValue represents an integer value */
	char strInterValue[20] = {0};
	/* StrDeciValue means a small value */
	u8 strDeciValue[20] = {0};
	/* Interger represents an integer number */
	int integer = 0;
	/* Interger for integer number decimer for decimal number */
	int decimer = 0;
	/* Represents integral value */
	//int intergerValue = 0;
	/* Representing a small number */
	//int decimerValue = 0;
	/* Total represents the total number of input digits (minus the decimal point). */
	int total = 0;
	/* Even D */
	int evenD = 0;
	/* Represents the difference between evenD and total */
	int difftotal = 0;
	/* Indicates that integer bits need to be completed */
	int diffInterger = 0;
	/* Indicates that the decimal place needs to be filled */
	int diffDecimer = 0;
	/* Represents the number of digits in the decimal place to delete */
	//int delDecimer = 0;
	int i = 0;
	int j = 0;
	int m = 0;

	u8 tmp[30];

	for (m = 0; m < 30; m++)
		tmp[m] = '0';

	hydrology_get_bcd_nums(input, &integer, &decimer, d, strInterValue, strDeciValue);
	evenD = hydrology_get_even_num(D);
	total = integer + decimer;

	/* Input configuration parameters are guaranteed */
	if (evenD >= total) {
		difftotal = evenD - total;

		/*This is definitely going to happen, hydrology_get_bcd_nums guarantees */
		if (d >= decimer) {
			/* The number of digits in the decimal place that need to be filled in */
			diffDecimer = d - decimer;
			/* Integer bit needs to fill in the number of digits of 0,
			 * assuming that difftotal is always greater than diffDecimer
			 */
			diffInterger = difftotal - diffDecimer;
		}

		/* The current decimal and integer bits are integrated into the TMP array,
		 * divided into the following parts 0-- >diffInterger-1 integer fill 0 number,
		 * diffInterger-- >diffInterger + interger is the number of integer bits,
		 * diffInter+ interge --> evenD is the number of decimal bits
		 */
		memcpy(&tmp[diffInterger], strInterValue, integer);
		memcpy(&tmp[diffInterger + integer], strDeciValue, decimer);

		tmp[evenD] = 0;

		for (i = 0; i < evenD; i = i + 2)
			out[j++] = (tmp[i] - '0') * 16 + (tmp[i + 1] - '0');
	}
	/* That will not happen now */
	else
		return false;

	return true;
}

int hydrology_malloc_element(u8 guide, u8 D, u8 d,
	struct hydrology_element *element)
{
	element->guide[0] = guide;
	hydrology_get_guide_id(&(element->guide[1]), D, d);

	if (D % 2 == 0) {
		element->value = (u8 *)kmalloc(D / 2, __GFP_ZERO);

		if (element->value == NULL) {
			pr_err("element->value malloc failed\n");
			return false;
		}

		element->num = D / 2;
	} else {
		element->value = (u8 *)kmalloc((D + 1) / 2, __GFP_ZERO);

		if (element->value == NULL) {
			pr_err("element->value malloc failed\n");
			return false;
		}

		element->num = (D + 1) / 2;
	}

	return true;
}

void hydrology_free_element(struct hydrology_element *element)
{
	kfree(element->value);
}

//int hydrology_ReadAnalog(float *value, int index)
//{
//    long addr = HYDROLOGY_ANALOG1 + index * 4;
//    u8 temp_value[4];
//    int ret;
//
//    ret = fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
//            addr, temp_value, HYDROLOGY_ANALOG_LEN);
//
//    if(ret == true)
//        *value = *((float *)temp_value);
//    else
//        *value = 0;
//
//    return ret;
//}
//
//int hydrology_ReadPulse(long *value, int index)
//{
//    long addr = HYDROLOGY_PULSE1 + index * 4;
//    u8 temp_value[4];
//    int ret;
//
//    ret = fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
//            addr, temp_value, HYDROLOGY_PULSE_LEN);
//
//    if(ret == true)
//        *value = *((long *)temp_value);
//    else
//        *value = 0;
//
//    return ret;
//}
//
//int hydrology_ReadSwitch(int *value)
//{
//    u8 temp_value[4];
//    int ret;
//
//    ret = fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
//            HYDROLOGY_SWITCH1, temp_value, HYDROLOGY_SWITCH_LEN);
//
//    if(ret == true)
//        *value = *((int *)temp_value);
//    else
//        *value = 0;
//
//    return ret;
//}

//extern int IsDebug;
//extern u8 IsQuery;
//extern u8 isUARTConfig;
//
//u8 ADCelementCount = 0;
//u8 ISR_COUNTelementCount = 0;
//u8 IO_STATUSelementCount = 0;
//u8 RS485elementCount = 0;

//int hydrology_CalelementInfo(void)
//{
//    int i = 0, acount = 0, pocunt = 0;
//    float floatvalue = 0;
//    long intvalue1 = 0;
//    int intvalue2 = 0;
//    struct hydrology_up_body *upbody = (struct hydrology_up_body *)g_hydrology.up_packet->body;
//    struct hydrology_down_body *down_body = (struct hydrology_down_body *)g_hydrology.down_packet->body;

//    switch(funcode)
//    {
//        case LINK_REPORT:
//            break;

//        case EVEN_PERIOD_INFO_REPORT:
//        case HOUR_REPORT:
//            g_hydrology.epi =
//                (HydrologyEvenPeriodInfo **)kmalloc(sizeof(HydrologyEvenPeriodInfo *) * upbody->count);

//            if(g_hydrology.epi == NULL)
//            {
//    pr_err("g_hydrology.epi malloc failed\n", i);
//                return false;
//            }

//            fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_FILE_EPI,
//					0, g_hydrology.epi, 12 * 2 * upbody->count);
//            break;

//        case TEST_REPORT:
//        case TIMER_REPORT:
//        case ADD_REPORT:
//            for(i = 0; i < upbody->count; i++)
//            {
//                switch(element_table[i].type)
//                {
//                    case HYDROLOGY_ANALOG:
//                        if(hydrology_ReadAnalog(&floatvalue, acount++) == false)
//                            return false;

//                        if(hydrology_malloc_element(element_table[i].ID,
//                                element_table[i].D, element_table[i].d,
//                                upbody->element[i]) == false)
//                            return false;

//                        hydrology_convert_to_hex_element((double)floatvalue,
//                            element_table[i].D, element_table[i].d,
//                            upbody->element[i]->value);
//                        break;

//                    case HYDROLOGY_PULSE:
//                        if(hydrology_ReadPulse(&intvalue1, pocunt++) == false)
//                            return false;

//                        if(hydrology_malloc_element(element_table[i].ID,
//                                element_table[i].D, element_table[i].d,
//                                upbody->element[i]) == false)
//                            return false;

//                        hydrology_convert_to_hex_element((double)intvalue1,
//                            element_table[i].D, element_table[i].d,
//                            upbody->element[i]->value);
//                        break;

//                    case HYDROLOGY_SWITCH:
//                        if(hydrology_ReadSwitch(&intvalue2) == false)
//                            return false;

//                        if(hydrology_malloc_element(element_table[i].ID,
//                                element_table[i].D, element_table[i].d,
//                                upbody->element[i]) == false)
//                            return false;

//                        hydrology_convert_to_hex_element((double)intvalue2,
//                            element_table[i].D, element_table[i].d,
//                            upbody->element[i]->value);
//                        break;
//                }
//            }

//            break;

//        case ARTIFICIAL_NUM_REPORT:
//        case PICTURE_REPORT:
//            upbody->element[i]->guide[0] = element_table[i].ID;
//            upbody->element[i]->guide[1] = element_table[i].ID;

////            if(endpoint->CurrentTimes != endpoint->TotalTimes)
////                maxpacket = endpoint->MaxPacket;
////            else
////                maxpacket = endpoint->TotalPacket % endpoint->MaxPacket;
////
////            upbody->element[i]->value = (u8 *)kmalloc(maxpacket, __GFP_ZERO);
////
////            if(upbody->element[i]->value == NULL)
////            {
////    pr_err("upbody->element[%d]->value malloc failed\n", i);
////                return false;
////            }
////            else
////            {
////                upbody->element[i]->num = maxpacket;
////                fatfs_read_store_info(HYDROLOGY_FILE_PATH, "HydrologyPicture.jpg",
////                    endpoint->CurrentTimes * endpoint->MaxPacket,
////                    upbody->element[i]->value, maxpacket);
////            }

//            upbody->element[i]->value =
//                (u8 *)kmalloc(strlen("HydrologyPicture.jpg"), __GFP_ZERO);

//            if(upbody->element[i]->value == NULL)
//            {
//    pr_err("upbody->element[%d]->value malloc failed\n", i);
//                return false;
//            }
//            else
//            {
//                hydrology_read_file_size("HydrologyPicture.jpg",
//                    &(upbody->element[i]->num));
//                memcpy(upbody->element[i]->value, "HydrologyPicture.jpg",
//                    strlen("HydrologyPicture.jpg"));
//            }


//            break;

//        case REAL_TIME_REPORT:
//        case PERIOD_REPORT:
//        case INQUIRE_ARTIFICIAL_NUM_REPORT:
//        case SPECIFIED_ELEMENT_REPORT:
//        case CONFIG_WRITE_REPORT:
//        case CONFIG_READ_REPORT:
//        case PARA_WRITE_REPORT:
//        case PARA_READ_REPORT:
//            for(i = 0; i < down_body->count; i++)
//            {
//                upbody->element[i]->num = (down_body->element[i]->guide[1] >> 3);
//                upbody->element[i]->value = (u8 *)kmalloc(upbody->element[i]->num, __GFP_ZERO);

//                if(upbody->element[i]->value == NULL)
//                    continue;

//                HydrologyReadSuiteelement(funcode,
//                    upbody->element[i]->guide, upbody->element[i]->value);
//            }

//            break;

//        case WATER_PUMP_MOTOR_REPORT:
//        case SW_VERSION_REPORT:
//        case STATUS_REPORT:
//        case INIT_SOLID_STORAGE_REPORT:
//        case RESET_REPORT:
//        case CHANGE_PASSWORD_REPORT:
//        case SET_CLOCK_REPORT:
//        case SET_IC_CARD_REPORT:
//        case PUMP_REPORT:
//        case VALVE_REPORT:
//        case GATE_REPORT:
//        case WATER_SETTING_REPORT:
//        case RECORD_REPORT:
//        case TIME_REPORT:

//            break;
//    }

//    return true;
//}

void hydrology_get_stream_id(u8 *stream_id)
{
	static unsigned short id = 0;
	id++;
	id = id % 65536;

	if (id == 0)
		id = 1;

	stream_id[0] = (id >> 8) & 0xff;
	stream_id[1] = id & 0xff;
}

int hydrology_read_specified_element_info(struct hydrology_element_info *element,
	enum hydrology_body_type funcode, u16 index)
{
	u32 addr = 0;
	int ret;

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
	case PERIOD_REPORT:
	case INQUIRE_ARTIFICIAL_NUM_REPORT:
	case SPECIFIED_ELEMENT_REPORT:
	case WATER_PUMP_MOTOR_REPORT:
	case STATUS_REPORT:
	case SET_IC_CARD_REPORT:
		if (index > 0x75)
			index -= 0xF0;
		else
			index += 0x0D;

		addr = index * sizeof(struct hydrology_element_info);
		break;

	case CONFIG_WRITE_REPORT:
	case CONFIG_READ_REPORT:
		index += 131;
		addr = (index - 1) * sizeof(struct hydrology_element_info);
		break;

	case PARA_WRITE_REPORT:
	case PARA_READ_REPORT:
		index += 131 + 15;
		index -= 0x20;
		addr = index * sizeof(struct hydrology_element_info);
		break;

	case SW_VERSION_REPORT:
	case PUMP_REPORT:
	case VALVE_REPORT:
	case GATE_REPORT:
	case WATER_SETTING_REPORT:
	case RECORD_REPORT:
		index += 131 + 15;
		index -= 0x20;
		addr = index * sizeof(struct hydrology_element_info);
		break;

	case CHANGE_PASSWORD_REPORT:
		if (index == 0x03) {
			index += 131;
			addr = (index - 1) * sizeof(struct hydrology_element_info);
		} else {
			index += 131 + 15;
			index -= 0x20;
			addr = index * sizeof(struct hydrology_element_info);
		}

		break;

	case INIT_SOLID_STORAGE_REPORT:
	case RESET_REPORT:
	case SET_CLOCK_REPORT:
	case TIME_REPORT:
		break;
	}

	ret = fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_INFO,
			addr, (u8 *)element, sizeof(struct hydrology_element_info));

	return ret;
}

#ifdef DESIGN_VERIFICATION_HYDROLOGY
#include "kinetis/test-kinetis.h"

int t_hydrology_device_m1m2(enum hydrology_mode mode);
int t_hydrology_host_m1m2m3(enum hydrology_mode mode);
int t_hydrology_device_m3(void);
int t_hydrology_device_m4(void);
int t_hydrology_host_m4(void);

int t_hydrology_init(int argc, char **argv)
{
	int ret;

	ret = hydrology_device_reboot();

	if (ret)
		return FAIL;

	return PASS;
}

int t_hydrology(int argc, char **argv)
{
	int ret = false;
	u8 host = false;
	enum hydrology_mode mode = HYDROLOGY_M1;

	if (argc > 1) {
		if (!strcmp(argv[1], "host"))
			host = true;
		else if (!strcmp(argv[1], "device"))
			host = false;
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "m1"))
			mode = HYDROLOGY_M1;
		else if (!strcmp(argv[2], "m2"))
			mode = HYDROLOGY_M2;
		else if (!strcmp(argv[2], "m3"))
			mode = HYDROLOGY_M3;
		else if (!strcmp(argv[2], "m4"))
			mode = HYDROLOGY_M4;
	}

	if (host == false) {
		switch (mode) {
		case HYDROLOGY_M1:
			ret = t_hydrology_device_m1m2(HYDROLOGY_M1);
			break;

		case HYDROLOGY_M2:
			ret = t_hydrology_device_m1m2(HYDROLOGY_M2);
			break;

		case HYDROLOGY_M3:
			ret = t_hydrology_device_m3();
			break;

		case HYDROLOGY_M4:
			ret = t_hydrology_device_m4();
			break;
		}
	} else {
		switch (mode) {
		case HYDROLOGY_M1:
			ret = t_hydrology_host_m1m2m3(HYDROLOGY_M1);
			break;

		case HYDROLOGY_M2:
			ret = t_hydrology_host_m1m2m3(HYDROLOGY_M2);
			break;

		case HYDROLOGY_M3:
			ret = t_hydrology_host_m1m2m3(HYDROLOGY_M3);
			break;

		case HYDROLOGY_M4:
			ret = t_hydrology_host_m4();
			break;
		}
	}

	if (ret == true)
		return PASS;
	else
		return FAIL;
}

#endif

