
#include "hydrology.h"
#include "hydrology-config.h"
#include "hydrology-cmd.h"
#include "hydrology-identifier.h"

#include <kinetis/hydrology-task.h>

#include <kinetis/tim-task.h>
#include <kinetis/rtc-task.h>
#include <kinetis/real-time-clock.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  This driver USES single linked list structure to register tasks.Depending on the user's needs, choose to use RTC or timer.
  * @step 3:  Write callback functions and initialization functions, such as function Task_Temperature_Humidit_Callback and HydrologyTask_Init.
  * @step 4:  Modify four areas: XXTask_TypeDef/Task_XX_Callback/HydrologyTask_Init.
  * @step 5:  Finally, HydrologyTask_Init is called in the main function.
  */

static void measure_temperature_humidit(struct tim_task *task)
{
	//    u8 tmpvalue[4] = {0, 0, 0, 0};

	//    SHT20_Read_TempAndRH(&SHT20_Temperature, &SHT20_Humidit);

	//    memcpy(tmpvalue, (char *)(&SHT20_Temperature), 4);
	//    fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
	//		HYDROLOGY_ANALOG1, tmpvalue, HYDROLOGY_ANALOG_LEN);
	//    memcpy(tmpvalue, (char *)(&SHT20_Humidit), 4);
	//    fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
	//		HYDROLOGY_ANALOG2, tmpvalue, HYDROLOGY_ANALOG_LEN);

	//    hydrology_set_observation_time(element_table[0].ID, 0);
}

void link_packet(struct tim_task *task)
{
	pr_err("Send %s packet\n",
		hydrology_type_string(LINK_REPORT));
	hydrology_device_process(NULL, 0, HYDROLOGY_M1, LINK_REPORT);
}

static void test_packet(void)
{
	struct hydrology_element element;
	struct hydrology_element_info element_table[] = {
		HYDROLOGY_E_DT
	};
	double floatvalue = 12;
	u8 i;

	for (i = 0; i < ARRAY_SIZE(element_table); i++) {
		hydrology_malloc_element(element_table[i].ID,
			element_table[i].D, element_table[i].d,
			&element);

		hydrology_convert_to_hex_element(floatvalue,
			element_table[i].D, element_table[i].d,
			element.value);
	}

	pr_err("[%s] Send %s packet\n",
		get_rtc_string(),
		hydrology_type_string(TEST_REPORT));
	hydrology_device_process(element_table, 1, HYDROLOGY_M1, TEST_REPORT);

	for (i = 0; i < ARRAY_SIZE(element_table); i++) {
		hydrology_free_element(&element);
	}
}

static void timer_report_packet(void)
{
	pr_err("[%s] Send %s packet\n",
		get_rtc_string(),
		hydrology_type_string(TIMER_REPORT));
}

static void add_report_packet(void)
{
	pr_err("[%s] Send %s packet\n",
		get_rtc_string(),
		hydrology_type_string(ADD_REPORT));
}

static void hour_packet(void)
{
	pr_err("[%s] Send %s packet\n",
		get_rtc_string(),
		hydrology_type_string(HOUR_REPORT));
}

void hydrology_task_exit(void)
{
	tim_task_drop(&g_hydrology.collecte_data);
	tim_task_drop(&g_hydrology.link_pkt);
	rtc_task_drop(test_packet);
	rtc_task_drop(timer_report_packet);
	rtc_task_drop(add_report_packet);
	rtc_task_drop(hour_packet);
}

int hydrology_task_init(void)
{
	u8 interval;
	int ret;

	/* Check and create all required files if they don't exist */
	ret = fatfs_find_file(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA);
	if (ret) {
		pr_info("Creating %s file...\n", HYDROLOGY_D_FILE_E_DATA);
		ret = fatfs_create_file(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA);
		if (ret) {
			pr_err("Failed to create %s file, error code: %d\n", HYDROLOGY_D_FILE_E_DATA, ret);
			return ret;
		}
	}

	ret = fatfs_find_file(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_INFO);
	if (ret) {
		pr_info("Creating %s file...\n", HYDROLOGY_D_FILE_E_INFO);
		ret = fatfs_create_file(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_INFO);
		if (ret) {
			pr_err("Failed to create %s file, error code: %d\n", HYDROLOGY_D_FILE_E_INFO, ret);
			return ret;
		}
	}

	ret = fatfs_find_file(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_PICTURE);
	if (ret) {
		pr_info("Creating %s file...\n", HYDROLOGY_D_FILE_PICTURE);
		ret = fatfs_create_file(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_PICTURE);
		if (ret) {
			pr_err("Failed to create %s file, error code: %d\n", HYDROLOGY_D_FILE_PICTURE, ret);
			return ret;
		}
	}

	ret = fatfs_find_file(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_RGZS);
	if (ret) {
		pr_info("Creating %s file...\n", HYDROLOGY_D_FILE_RGZS);
		ret = fatfs_create_file(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_RGZS);
		if (ret) {
			pr_err("Failed to create %s file, error code: %d\n", HYDROLOGY_D_FILE_RGZS, ret);
			return ret;
		}
	}

	ret = fatfs_find_file(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA);
	if (ret) {
		pr_info("Creating %s file...\n", HYDROLOGY_H_FILE_E_DATA);
		ret = fatfs_create_file(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_DATA);
		if (ret) {
			pr_err("Failed to create %s file, error code: %d\n", HYDROLOGY_H_FILE_E_DATA, ret);
			return ret;
		}
	}

	ret = fatfs_find_file(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_INFO);
	if (ret) {
		pr_info("Creating %s file...\n", HYDROLOGY_H_FILE_E_INFO);
		ret = fatfs_create_file(HYDROLOGY_FILE_PATH, HYDROLOGY_H_FILE_E_INFO);
		if (ret) {
			pr_err("Failed to create %s file, error code: %d\n", HYDROLOGY_H_FILE_E_INFO, ret);
			return ret;
		}
	}

	tim_task_add(&g_hydrology.collecte_data, "measure temperature humidit",
		60 * 1000, true, false, measure_temperature_humidit);
	tim_task_add(&g_hydrology.link_pkt, "link packet",
		40 * 1000, true, false, link_packet);

	rtc_task_add(0, 0, 0, 0, 1, 0, true, test_packet);

	ret = fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
			HYDROLOGY_PA_TI, &interval, 1);
	if (ret) {
		pr_err("Failed to read interval timer, error code: %d\n", ret);
		/* Use default value if read fails */
		interval = 0;
	}
	if (interval == 0) {
		interval = 5;
	}
	ret = fatfs_write_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
			HYDROLOGY_PA_TI, &interval, 1);
	if (ret) {
		pr_err("Failed to write interval timer, error code: %d\n", ret);
		/* Continue with the value anyway */
	}
	rtc_task_add(0, 0, 0, 0, interval, 0, true, timer_report_packet);

	ret = fatfs_read_store_info(HYDROLOGY_FILE_PATH, HYDROLOGY_D_FILE_E_DATA,
			HYDROLOGY_PA_AI, &interval, 1);
	if (ret) {
		pr_err("Failed to read add interval timer, error code: %d\n", ret);
		/* Use default value if read fails */
		interval = 0;
	}

	if (interval != 0) {
		rtc_task_add(0, 0, 0, 0, interval, 0, true, add_report_packet);
	}

	rtc_task_add(0, 0, 0, 1, 0, 0, true, hour_packet);

	return 0;
}
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */
