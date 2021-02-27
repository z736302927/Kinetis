#include "kinetis/hydrology-task.h"
#include "kinetis/hydrology.h"
#include "kinetis/hydrology-config.h"
#include "kinetis/hydrology-cmd.h"
#include "kinetis/hydrology-identifier.h"
#include "kinetis/tim-task.h"
#include "kinetis/rtc-task.h"
#include "kinetis/sht20.h"
#include "kinetis/idebug.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  This driver USES single linked list structure to register tasks.Depending on the user's needs, choose to use RTC or timer.
  * @step 3:  Write callback functions and initialization functions, such as function Task_Temperature_Humidit_Callback and HydrologyTask_Init.
  * @step 4:  Modify four areas: XXTask_TypeDef/Task_XX_Callback/HydrologyTask_Init.
  * @step 5:  Finally, HydrologyTask_Init is called in the main function.
  */

#include "string.h"

float SHT20_Temperature = 0;
float SHT20_Humidit = 0;

static void measure_temperature_humidit(void)
{
//    u8 tmpvalue[4] = {0, 0, 0, 0};

//    SHT20_Read_TempAndRH(&SHT20_Temperature, &SHT20_Humidit);

//    memcpy(tmpvalue, (char *)(&SHT20_Temperature), 4);
//    hydrology_write_store_info("HYDROLOGY_D_FILE_E_DATA", HYDROLOGY_ANALOG1, tmpvalue, HYDROLOGY_ANALOG_LEN);
//    memcpy(tmpvalue, (char *)(&SHT20_Humidit), 4);
//    hydrology_write_store_info("HYDROLOGY_D_FILE_E_DATA", HYDROLOGY_ANALOG2, tmpvalue, HYDROLOGY_ANALOG_LEN);

//    hydrology_set_observation_time(element_table[0].ID, 0);
}

void link_packet(void)
{
    printk(KERN_DEBUG "Send %s packet\n", hydrology_type_string(LINK_REPORT));
    hydrology_device_process(NULL, 0, HYDROLOGY_M1, LINK_REPORT);
}

static void test_packet(void)
{
    float floatvalue;
    u8 i;
    struct hydrology_element Elment;
    struct hydrology_element_info element_table[] = {
        HYDROLOGY_E_DT,
        NULL
    };


    floatvalue = 12;

    for (i = 0; i < 1; i++) {
        hydrology_malloc_element(element_table[i].ID,
            element_table[i].D, element_table[i].d,
            &Elment);

        hydrology_convert_to_hex_element((double)floatvalue,
            element_table[i].D, element_table[i].d,
            Elment.value);
    }

    printk(KERN_DEBUG "Send %s packet\n", hydrology_type_string(TEST_REPORT));
    hydrology_device_process(element_table, 1, HYDROLOGY_M1, TEST_REPORT);
}

static void timer_report_packet(void)
{
    printk(KERN_DEBUG "Send %s packet\n", hydrology_type_string(TIMER_REPORT));
}

static void add_report_packet(void)
{
    printk(KERN_DEBUG "Send %s packet\n", hydrology_type_string(ADD_REPORT));
}

static void hour_packet(void)
{
    printk(KERN_DEBUG "Send %s packet\n", hydrology_type_string(HOUR_REPORT));
}

void hydrology_task_exit(void)
{
    tim_task_drop(measure_temperature_humidit);
    tim_task_drop(link_packet);
    rtc_task_drop(test_packet);
    rtc_task_drop(timer_report_packet);
    rtc_task_drop(add_report_packet);
    rtc_task_drop(hour_packet);
}

int hydrology_task_init(void)
{
    u8 interval;
    tim_task_add(60 * 1000, true, measure_temperature_humidit);
    tim_task_add(40 * 1000, true, link_packet);

    rtc_task_add(0, 0, 0, 0, 1, 0, true, test_packet);

    hydrology_read_store_info(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_TI, &interval, 1);
    rtc_task_add(0, 0, 0, 0, interval, 0, true, timer_report_packet);

    hydrology_read_store_info(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_AI, &interval, 1);

    if (interval != 0)
        rtc_task_add(0, 0, 0, 0, interval, 0, true, add_report_packet);

    rtc_task_add(0, 0, 0, 1, 0, 0, true, hour_packet);
    
    return 0;
}
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


