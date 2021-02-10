#include "task/hydrology-task.h"
#include "kinetis/hydrology.h"
#include "kinetis/hydrology-config.h"
#include "kinetis/hydrology-cmd.h"
#include "kinetis/hydrology-identifier.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  This driver USES single linked list structure to register tasks.Depending on the user's needs, choose to use RTC or timer.
  * @step 3:  Write callback functions and initialization functions, such as function Task_Temperature_Humidit_Callback and HydrologyTask_Init.
  * @step 4:  Modify four areas: XXTask_TypeDef/Task_XX_Callback/HydrologyTask_Init.
  * @step 5:  Finally, HydrologyTask_Init is called in the main function.
  */

#include "task/timtask.h"
#include "task/rtctask.h"
#include "kinetis/sht20.h"
#include "string.h"
#include "kinetis/idebug.h"

struct TimTask_TypeDef HydrologyTask_LinkMaintenance;
struct TimTask_TypeDef Task_Temperature_Humidit;

struct RTCTask_TypeDef HydrologyTask_Test;
struct RTCTask_TypeDef HydrologyTask_TimerReport;
struct RTCTask_TypeDef HydrologyTask_AddReport;
struct RTCTask_TypeDef HydrologyTask_Hour;

float SHT20_Temperature = 0;
float SHT20_Humidit = 0;

void Task_Temperature_Humidit_Callback(void)
{
//    u8 tmpvalue[4] = {0, 0, 0, 0};

//    SHT20_Read_TempAndRH(&SHT20_Temperature, &SHT20_Humidit);

//    memcpy(tmpvalue, (char *)(&SHT20_Temperature), 4);
//    Hydrology_WriteStoreInfo("HYDROLOGY_D_FILE_E_DATA", HYDROLOGY_ANALOG1, tmpvalue, HYDROLOGY_ANALOG_LEN);
//    memcpy(tmpvalue, (char *)(&SHT20_Humidit), 4);
//    Hydrology_WriteStoreInfo("HYDROLOGY_D_FILE_E_DATA", HYDROLOGY_ANALOG2, tmpvalue, HYDROLOGY_ANALOG_LEN);

//    Hydrology_SetObservationTime(Element_table[0].ID, 0);
}

void HydrologyTask_LinkMaintenance_Callback(void)
{
    kinetis_debug_trace(KERN_DEBUG, "Send packet, function code = LinkMaintenance");
    HydrologyD_Process(NULL, 0, HYDROLOGY_M1, LinkMaintenance);
}

void HydrologyTask_Test_Callback(void)
{
    float floatvalue;
    u8 i;
    HydrologyElement Elment;
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_E_DT,
        NULL
    };


    floatvalue = 12;

    for (i = 0; i < 1; i++) {
        Hydrology_MallocElement(Element_table[i].ID,
            Element_table[i].D, Element_table[i].d,
            &Elment);

        Hydrology_ConvertToHexElement((double)floatvalue,
            Element_table[i].D, Element_table[i].d,
            Elment.value);
    }

    kinetis_debug_trace(KERN_DEBUG, "Send packet, function code = Test");
    HydrologyD_Process(Element_table, 1, HYDROLOGY_M1, Test);
}

void HydrologyTask_TimerReport_Callback(void)
{
    kinetis_debug_trace(KERN_DEBUG, "Send packet, function code = TimerReport");
}

void HydrologyTask_AddReport_Callback(void)
{
    kinetis_debug_trace(KERN_DEBUG, "Send packet, function code = AddReport");
}

void HydrologyTask_Hour_Callback(void)
{
    kinetis_debug_trace(KERN_DEBUG, "Send packet, function code = Hour");
}

void HydrologyTask_Deinit(void)
{
    TimTask_Deinit(&Task_Temperature_Humidit);
    TimTask_Stop(&Task_Temperature_Humidit);

    TimTask_Deinit(&HydrologyTask_LinkMaintenance);

//  RTCTask_Deinit(&HydrologyTask_Test);
//  RTCTask_Stop(&HydrologyTask_Test);

    RTCTask_Deinit(&HydrologyTask_TimerReport);
    RTCTask_Stop(&HydrologyTask_TimerReport);

    RTCTask_Deinit(&HydrologyTask_Hour);
    RTCTask_Stop(&HydrologyTask_Hour);
}

void HydrologyTask_Init(void)
{
    u8 interval;

    TimTask_Init(&Task_Temperature_Humidit, Task_Temperature_Humidit_Callback, 60 * 1000, 60 * 1000);
    TimTask_Start(&Task_Temperature_Humidit);

    TimTask_Init(&HydrologyTask_LinkMaintenance, HydrologyTask_LinkMaintenance_Callback, 40 * 1000, 40 * 1000);

//  RTCTask_Init(&HydrologyTask_Test, HydrologyTask_Test_Callback, 0, 1, 0);
//  RTCTask_Start(&HydrologyTask_Test);

    Hydrology_read_store_info(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_TI, &interval, 1);
    RTCTask_Init(&HydrologyTask_TimerReport, HydrologyTask_TimerReport_Callback, interval, 0, 0);
    RTCTask_Start(&HydrologyTask_TimerReport);

    Hydrology_read_store_info(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_AI, &interval, 1);

    if (interval != 0) {
        RTCTask_Init(&HydrologyTask_AddReport, HydrologyTask_AddReport_Callback, 0, interval, 0);
        RTCTask_Start(&HydrologyTask_AddReport);
    }

    RTCTask_Init(&HydrologyTask_Hour, HydrologyTask_Hour_Callback, 1, 0, 0);
    RTCTask_Start(&HydrologyTask_Hour);
}
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */


