#include "hydrology-protocol/hydrologytask.h"
#include "hydrology-protocol/message.h"
#include "hydrology-protocol/hydrologycommand.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  This driver USES single linked list structure to register tasks.Depending on the user's needs, choose to use RTC or timer.
  * @step 3:  Write callback functions and initialization functions, such as function Task_Temperature_Humidit_Callback and HydrologyTask_Init.
  * @step 4:  Modify four areas: XXTask_TypeDef/Task_XX_Callback/HydrologyTask_Init.
  * @step 5:  Finally, HydrologyTask_Init is called in the main function.
  */

#include "bsp_timtask/bsp_timtask.h"
#include "bsp_rtctask/bsp_rtctask.h"
#include "sht20/sht20.h"
#include "string.h"

struct TimTask_TypeDef HydrologyTask_LinkMaintenance;
struct TimTask_TypeDef Task_Temperature_Humidit;

struct RTCTask_TypeDef HydrologyTask_Test;
struct RTCTask_TypeDef HydrologyTask_TimerReport;
struct RTCTask_TypeDef HydrologyTask_Hour;

float SHT20_Temperature = 0;
float SHT20_Humidit = 0;

void Task_Temperature_Humidit_Callback(void)
{
  char tmpvalue[4] = {0,0,0,0};
  
  SHT20_Read_TempAndRH(&SHT20_Temperature, &SHT20_Humidit);
  
  memcpy(tmpvalue, (char*)(&SHT20_Temperature), 4);
  Hydrology_WriteStoreInfo(HYDROLOGY_ANALOG1, tmpvalue, HYDROLOGY_ANALOG_LEN);
  memcpy(tmpvalue, (char*)(&SHT20_Humidit), 4);
  Hydrology_WriteStoreInfo(HYDROLOGY_ANALOG2, tmpvalue, HYDROLOGY_ANALOG_LEN);
  
  Hydrology_SetObservationTime(Element_table[0].ID, 0);
}

void HydrologyTask_LinkMaintenance_Callback(void)
{
  hydrologyProcessSend(LinkMaintenance);
}

void HydrologyTask_Test_Callback(void)
{
  hydrologyProcessSend(Test);
}

void HydrologyTask_TimerReport_Callback(void)
{
  hydrologyProcessSend(TimerReport);
}

void HydrologyTask_Hour_Callback(void)
{
  hydrologyProcessSend(Hour);
}

void HydrologyTask_Init(void)
{
//  TimTask_Init(&HydrologyTask_LinkMaintenance, HydrologyTask_LinkMaintenance_Callback, 40 * 1000, 40 * 1000); 
//  TimTask_Start(&HydrologyTask_LinkMaintenance);
  
  TimTask_Init(&Task_Temperature_Humidit, Task_Temperature_Humidit_Callback, 60 * 1000, 60 * 1000); 
  TimTask_Start(&Task_Temperature_Humidit);
 
//  RTCTask_Init(&HydrologyTask_Test, HydrologyTask_Test_Callback, 0, 1, 0);
//  RTCTask_Start(&HydrologyTask_Test);
  
  RTCTask_Init(&HydrologyTask_TimerReport, HydrologyTask_TimerReport_Callback, 0, 5, 0);
  RTCTask_Start(&HydrologyTask_TimerReport);
  
//  RTCTask_Init(&HydrologyTask_Hour, HydrologyTask_Hour_Callback, 1, 0, 0);
//  RTCTask_Start(&HydrologyTask_Hour);
}
/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/
