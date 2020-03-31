#include "protocol/hydrology.h"
#include "protocol/hydrologycommand.h"
#include <string.h>
#include "stdint.h"

extern const hydrologyElementInfo Element_table[];

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  .
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#define DEBUG
#include "idebug.h"

#define HydrologyCmd_printf    p_dbg

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


void Hydrology_ReadRS485(char *value, int index)
{

}

void hydrologyChangeMode(char M)
{
//    HYDROLOGY_MODE = M;
}


static int hydrologyConfigSelect(char* guide, char* value)
{
  int ret;

  switch(guide[0])
  {
    case 0x01:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_CENTER_ADDR, value, HYDROLOGY_CENTER_LEN);
      break;
    }
    case 0x02:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_REMOTE_ADDR, value, HYDROLOGY_REMOTE_LEN);
      break;
    }
    case 0x03:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_PASSWORD_ADDR, value, HYDROLOGY_PASSWORD_LEN);
      break;
    }
    case 0x04:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_CENTER1_IP, value, HYDROLOGY_CENTER_IP_LEN);
      break;
    }
    case 0x05:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_BACKUP1_IP, value, HYDROLOGY_BACKUP_IP_LEN);
      break;
    }
    case 0x06:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_CENTER2_IP, value, HYDROLOGY_CENTER_IP_LEN);
      break;
    }
    case 0x07:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_BACKUP2_IP, value, HYDROLOGY_BACKUP_IP_LEN);
      break;
    }
    case 0x08:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_CENTER3_IP, value, HYDROLOGY_CENTER_IP_LEN);
      break;
    }
    case 0x09:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_BACKUP3_IP, value, HYDROLOGY_BACKUP_IP_LEN);
      break;
    }
    case 0x0A:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_CENTER4_IP, value, HYDROLOGY_CENTER_IP_LEN);
      break;
    }
    case 0x0B:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_BACKUP4_IP, value, HYDROLOGY_BACKUP_IP_LEN);
      break;
    }
    case 0x0C:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WORK_MODE, value, HYDROLOGY_WORK_MODE_LEN);
      break;
    }
    case 0x0D:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ELEMENT_SELECT, value, HYDROLOGY_ELEMENT_SELECT_LEN);
      break;
    }
    case 0x0E:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_REPEATER_STATION, value, HYDROLOGY_REPEATER_STATION_LEN);
      break;
    }
    case 0x0F:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_DEVICE_ID, value, HYDROLOGY_DEVICE_ID_LEN);
      break;
    }
    default:
    {
      ret = -1;
      break;
    }
  }
  return ret;
}

static int hydrologyReadSelect(char* guide, char* value)
{
  int ret;

  switch(guide[0])
  {
    case 0x01:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_CENTER_ADDR, value, HYDROLOGY_CENTER_LEN);
      break;
    }
    case 0x02:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_REMOTE_ADDR, value, HYDROLOGY_REMOTE_LEN);
      break;
    }
    case 0x03:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_PASSWORD_ADDR, value, HYDROLOGY_PASSWORD_LEN);
      break;
    }
    case 0x04:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_CENTER1_IP, value, HYDROLOGY_CENTER_IP_LEN);
      break;
    }
    case 0x05:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_BACKUP1_IP, value, HYDROLOGY_BACKUP_IP_LEN);
      break;
    }
    case 0x06:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_CENTER2_IP, value, HYDROLOGY_CENTER_IP_LEN);
      break;
    }
    case 0x07:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_BACKUP2_IP, value, HYDROLOGY_BACKUP_IP_LEN);
      break;
    }
    case 0x08:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_CENTER3_IP, value, HYDROLOGY_CENTER_IP_LEN);
      break;
    }
    case 0x09:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_BACKUP3_IP, value, HYDROLOGY_BACKUP_IP_LEN);
      break;
    }
    case 0x0A:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_CENTER4_IP, value, HYDROLOGY_CENTER_IP_LEN);
      break;
    }
    case 0x0B:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_BACKUP4_IP, value, HYDROLOGY_BACKUP_IP_LEN);
      break;
    }
    case 0x0C:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WORK_MODE, value, HYDROLOGY_WORK_MODE_LEN);
      break;
    }
    case 0x0D:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ELEMENT_SELECT, value, HYDROLOGY_ELEMENT_SELECT_LEN);
      break;
    }
    case 0x0E:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_REPEATER_STATION, value, HYDROLOGY_REPEATER_STATION_LEN);
      break;
    }
    case 0x0F:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_DEVICE_ID, value, HYDROLOGY_DEVICE_ID_LEN);
      break;
    }
    default:
    {
      ret = -1;
      break;
    }
  }
  return ret;
}

static int hydrologyBasicInfoConfig(void)
{
  int i = 0;
  int ret;

  hydrologyDownBody* pbody = (hydrologyDownBody*)(g_HydrologyMsg.downbody);

  for(i=0; i< pbody->count; i++)
  {
    ret = hydrologyConfigSelect((pbody->element)[i].guide,  (pbody->element)[i].value);
    if(ret == -1)
      return i+1;
  }
  return 0;
}

static int hydrologyBasicInfoRead(void)
{
  int i = 0;
  int ret;

  hydrologyDownBody* pbody = (hydrologyDownBody*)(g_HydrologyMsg.downbody);

  for(i=0; i< pbody->count; i++)
  {
    ret = hydrologyReadSelect((pbody->element)[i].guide,  (pbody->element)[i].value);
    if(ret == -1)
      return i+1;
  }
  return 0;
}

static int hydrologySetParaSelect(char* guide, char* value)
{
  int ret;

  switch(guide[0])
  {
    case 0x20:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_TIMER_INTERVAL, value, HYDROLOGY_TIMER_INTERVAL_LEN);
      break;
    }
    case 0x21:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ADD_INTERVAL, value, HYDROLOGY_ADD_INTERVAL_LEN);
      break;
    }
    case 0x22:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RAINFALL_BEGIN_TIME, value, HYDROLOGY_RAINFALL_BEGIN_TIME_LEN);
      break;
    }
    case 0x23:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_SAMPLE_INTERVAL, value, HYDROLOGY_SAMPLE_INTERVAL_LEN);
      break;
    }
    case 0x24:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_STORE_INTERVAL, value, HYDROLOGY_WATERLEVEL_STORE_INTERVAL_LEN);
      break;
    }
    case 0x25:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RAINFALL_RESOLUTION, value, HYDROLOGY_RAINFALL_RESOLUTION_LEN);
      break;
    }
    case 0x26:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_RESOLUTION, value, HYDROLOGY_WATERLEVEL_RESOLUTION_LEN);
      break;
    }
    case 0x27:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RAINFALL_ADD_THRESHOLD, value, HYDROLOGY_RAINFALL_ADD_THRESHOLD_LEN);
      break;
    }
    case 0x28:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE1, value, HYDROLOGY_WATERLEVEL_BASICVALUE1_LEN);
      break;
    }
    case 0x29:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE2, value, HYDROLOGY_WATERLEVEL_BASICVALUE2_LEN);
      break;
    }
    case 0x2A:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE3, value, HYDROLOGY_WATERLEVEL_BASICVALUE3_LEN);
      break;
    }
    case 0x2B:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE4, value, HYDROLOGY_WATERLEVEL_BASICVALUE4_LEN);
      break;
    }
    case 0x2C:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE5, value, HYDROLOGY_WATERLEVEL_BASICVALUE5_LEN);
      break;
    }
    case 0x2D:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE6, value, HYDROLOGY_WATERLEVEL_BASICVALUE6_LEN);
      break;
    }
    case 0x2E:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE7, value, HYDROLOGY_WATERLEVEL_BASICVALUE7_LEN);
      break;
    }
    case 0x2F:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE8, value, HYDROLOGY_WATERLEVEL_BASICVALUE8_LEN);
      break;
    }
    case 0x30:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE1, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE1_LEN);
      break;
    }
    case 0x31:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE2, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE2_LEN);
      break;
    }
    case 0x32:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE3, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE3_LEN);
      break;
    }
    case 0x33:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE4, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE4_LEN);
      break;
    }
    case 0x34:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE5, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE5_LEN);
      break;
    }
    case 0x35:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE6, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE6_LEN);
      break;
    }
    case 0x36:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE7, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE7_LEN);
      break;
    }
    case 0x37:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE8, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE8_LEN);
      break;
    }
    case 0x38:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ADD_WATERLEVEL1, value, HYDROLOGY_ADD_WATERLEVEL1_LEN);
      break;
    }
    case 0x39:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ADD_WATERLEVEL2, value, HYDROLOGY_ADD_WATERLEVEL2_LEN);
      break;
    }
    case 0x3A:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ADD_WATERLEVEL3, value, HYDROLOGY_ADD_WATERLEVEL3_LEN);
      break;
    }
    case 0x3B:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ADD_WATERLEVEL4, value, HYDROLOGY_ADD_WATERLEVEL4_LEN);
      break;
    }
    case 0x3C:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ADD_WATERLEVEL5, value, HYDROLOGY_ADD_WATERLEVEL5_LEN);
      break;
    }
    case 0x3D:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ADD_WATERLEVEL6, value, HYDROLOGY_ADD_WATERLEVEL6_LEN);
      break;
    }
    case 0x3E:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ADD_WATERLEVEL7, value, HYDROLOGY_ADD_WATERLEVEL7_LEN);
      break;
    }
    case 0x3F:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ADD_WATERLEVEL8, value, HYDROLOGY_ADD_WATERLEVEL8_LEN);
      break;
    }
    case 0x40:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ADD_ABOVE_THRESHOLD, value, HYDROLOGY_ADD_ABOVE_THRESHOLD_LEN);
      break;
    }
    case 0x41:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_ADD_BELOW_THRESHOLD, value, HYDROLOGY_ADD_BELOW_THRESHOLD_LEN);
      break;
    }
    case 0xA9:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT1, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAA:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT2, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAB:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT3, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAC:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT4, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAD:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT5, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAE:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT6, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAF:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT7, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xB0:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT8, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xB1:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT9, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xB2:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT10, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xB3:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS4851, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB4:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS4852, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB5:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS4853, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB6:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS4854, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB7:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS4855, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB8:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS4856, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB9:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS4857, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBA:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS4858, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBB:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS4859, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBC:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS48510, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBD:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS48511, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBE:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS48512, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBF:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS48513, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC0:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS48514, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC1:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS48515, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC2:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS48516, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC3:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS48517, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC4:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS48518, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC5:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS48519, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC6:
    {
      ret = Hydrology_WriteStoreInfo(HYDROLOGY_RS48520, value, HYDROLOGY_RS485_LEN);
      break;
    }
    default:
    {
      ret = -1;
      break;
    }
  }
  return ret;
}

static int hydrologyReadParaSelect(char* guide, char* value)
{
  int ret;

  switch(guide[0])
  {
    case 0x20:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_TIMER_INTERVAL, value, HYDROLOGY_TIMER_INTERVAL_LEN);
      break;
    }
    case 0x21:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ADD_INTERVAL, value, HYDROLOGY_ADD_INTERVAL_LEN);
      break;
    }
    case 0x22:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RAINFALL_BEGIN_TIME, value, HYDROLOGY_RAINFALL_BEGIN_TIME_LEN);
      break;
    }
    case 0x23:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_SAMPLE_INTERVAL, value, HYDROLOGY_SAMPLE_INTERVAL_LEN);
      break;
    }
    case 0x24:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_STORE_INTERVAL, value, HYDROLOGY_WATERLEVEL_STORE_INTERVAL_LEN);
      break;
    }
    case 0x25:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RAINFALL_RESOLUTION, value, HYDROLOGY_RAINFALL_RESOLUTION_LEN);
      break;
    }
    case 0x26:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_RESOLUTION, value, HYDROLOGY_WATERLEVEL_RESOLUTION_LEN);
      break;
    }
    case 0x27:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RAINFALL_ADD_THRESHOLD, value, HYDROLOGY_RAINFALL_ADD_THRESHOLD_LEN);
      break;
    }
    case 0x28:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE1, value, HYDROLOGY_WATERLEVEL_BASICVALUE1_LEN);
      break;
    }
    case 0x29:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE2, value, HYDROLOGY_WATERLEVEL_BASICVALUE2_LEN);
      break;
    }
    case 0x2A:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE3, value, HYDROLOGY_WATERLEVEL_BASICVALUE3_LEN);
      break;
    }
    case 0x2B:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE4, value, HYDROLOGY_WATERLEVEL_BASICVALUE4_LEN);
      break;
    }
    case 0x2C:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE5, value, HYDROLOGY_WATERLEVEL_BASICVALUE5_LEN);
      break;
    }
    case 0x2D:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE6, value, HYDROLOGY_WATERLEVEL_BASICVALUE6_LEN);
      break;
    }
    case 0x2E:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE7, value, HYDROLOGY_WATERLEVEL_BASICVALUE7_LEN);
      break;
    }
    case 0x2F:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE8, value, HYDROLOGY_WATERLEVEL_BASICVALUE8_LEN);
      break;
    }
    case 0x30:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE1, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE1_LEN);
      break;
    }
    case 0x31:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE2, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE2_LEN);
      break;
    }
    case 0x32:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE3, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE3_LEN);
      break;
    }
    case 0x33:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE4, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE4_LEN);
      break;
    }
    case 0x34:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE5, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE5_LEN);
      break;
    }
    case 0x35:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE6, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE6_LEN);
      break;
    }
    case 0x36:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE7, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE7_LEN);
      break;
    }
    case 0x37:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE8, value, HYDROLOGY_WATERLEVEL_CORRECTVALUE8_LEN);
      break;
    }
    case 0x38:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ADD_WATERLEVEL1, value, HYDROLOGY_ADD_WATERLEVEL1_LEN);
      break;
    }
    case 0x39:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ADD_WATERLEVEL2, value, HYDROLOGY_ADD_WATERLEVEL2_LEN);
      break;
    }
    case 0x3A:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ADD_WATERLEVEL3, value, HYDROLOGY_ADD_WATERLEVEL3_LEN);
      break;
    }
    case 0x3B:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ADD_WATERLEVEL4, value, HYDROLOGY_ADD_WATERLEVEL4_LEN);
      break;
    }
    case 0x3C:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ADD_WATERLEVEL5, value, HYDROLOGY_ADD_WATERLEVEL5_LEN);
      break;
    }
    case 0x3D:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ADD_WATERLEVEL6, value, HYDROLOGY_ADD_WATERLEVEL6_LEN);
      break;
    }
    case 0x3E:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ADD_WATERLEVEL7, value, HYDROLOGY_ADD_WATERLEVEL7_LEN);
      break;
    }
    case 0x3F:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ADD_WATERLEVEL8, value, HYDROLOGY_ADD_WATERLEVEL8_LEN);
      break;
    }
    case 0x40:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ADD_ABOVE_THRESHOLD, value, HYDROLOGY_ADD_ABOVE_THRESHOLD_LEN);
      break;
    }
    case 0x41:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_ADD_BELOW_THRESHOLD, value, HYDROLOGY_ADD_BELOW_THRESHOLD_LEN);
      break;
    }
    case 0xA9:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS485_COUNT1, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAA:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS485_COUNT2, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAB:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS485_COUNT3, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAC:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS485_COUNT4, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAD:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS485_COUNT5, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAE:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS485_COUNT6, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xAF:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS485_COUNT7, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xB0:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS485_COUNT8, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xB1:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS485_COUNT9, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xB2:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS485_COUNT10, value, HYDROLOGY_RS485_COUNT_LEN);
      break;
    }
    case 0xB3:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS4851, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB4:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS4852, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB5:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS4853, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB6:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS4854, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB7:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS4855, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB8:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS4856, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xB9:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS4857, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBA:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS4858, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBB:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS4859, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBC:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS48510, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBD:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS48511, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBE:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS48512, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xBF:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS48513, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC0:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS48514, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC1:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS48515, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC2:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS48516, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC3:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS48517, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC4:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS48518, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC5:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS48519, value, HYDROLOGY_RS485_LEN);
      break;
    }
    case 0xC6:
    {
      ret = Hydrology_ReadStoreInfo(HYDROLOGY_RS48520, value, HYDROLOGY_RS485_LEN);
      break;
    }
    default:
    {
      ret = -1;
      break;
    }
  }
  return ret;
}

static int hydrologySetPara(void)
{
  int i = 0;
  int ret;

  hydrologyDownBody* pbody = (hydrologyDownBody*)(g_HydrologyMsg.downbody);

  for(i=0; i< pbody->count; i++)
  {
    ret = hydrologySetParaSelect((pbody->element)[i].guide,  (pbody->element)[i].value);
    if(ret == -1)
      return i+1;
  }
  return 0;
}

static int hydrologyReadPara(void)
{
  int i = 0;
  int ret;

  hydrologyDownBody* pbody = (hydrologyDownBody*)(g_HydrologyMsg.downbody);

  for(i=0; i< pbody->count; i++)
  {
    ret = hydrologyReadParaSelect((pbody->element)[i].guide,  (pbody->element)[i].value);
    if(ret == -1)
      return i+1;
  }
  return 0;
}

static int hydrologyInitializeSolidStorage(void)
{
  char _src[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int writelen = HYDROLOGY_END - HYDROLOGY_CONFIG_ADDR;
  int writeaddr = HYDROLOGY_CONFIG_ADDR;
  
  if(writelen >= 0x07FF)
    writelen = 0x07FF;
  
  while (writelen > 16)
  {
    Hydrology_WriteStoreInfo(writeaddr,  _src,  16);

    writeaddr = writeaddr + 16;
    writelen = writelen - 16;
  }

  Hydrology_WriteStoreInfo(writeaddr,  _src,  writelen);
    
  return 0;
}

int Hydrology_Reset(void)
{
  char temp[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  
  temp[0] = 0x50;
  Hydrology_WriteStoreInfo(HYDROLOGY_RTUTYPE, temp, HYDROLOGY_RTUTYPE_LEN);
  temp[0] = 0x01;temp[1] = 0x02;temp[2] = 0x03;temp[3] = 0x04;
  Hydrology_WriteStoreInfo(HYDROLOGY_CENTER_ADDR, temp, HYDROLOGY_CENTER_LEN);
  temp[0] = 0x00;temp[1] = 0x12;temp[2] = 0x34;temp[3] = 0x56;temp[4] = 0x78;
  Hydrology_WriteStoreInfo(HYDROLOGY_REMOTE_ADDR, temp, HYDROLOGY_REMOTE_LEN);
  temp[0] = 0x12;temp[1] = 0x34;
  Hydrology_WriteStoreInfo(HYDROLOGY_PASSWORD_ADDR, temp, HYDROLOGY_PASSWORD_LEN);
  temp[0] = 0x02;temp[1] = 0x05;temp[2] = 0x80;temp[3] = 0x49;temp[4] = 0x14;
  temp[5] = 0x02;temp[6] = 0x02;temp[7] = 0x00;temp[8] = 0x89;temp[9] = 0x86;
  Hydrology_WriteStoreInfo(HYDROLOGY_CENTER1_IP, temp, HYDROLOGY_CENTER_IP_LEN);
  temp[0] = 0x02;temp[1] = 0x18;temp[2] = 0x30;temp[3] = 0x92;temp[4] = 0x03;
  temp[5] = 0x30;temp[6] = 0x30;temp[7] = 0x00;temp[8] = 0x66;temp[9] = 0x66;
  Hydrology_WriteStoreInfo(HYDROLOGY_BACKUP1_IP, temp, HYDROLOGY_BACKUP_IP_LEN);
  temp[0] = 0x02;temp[1] = 0x22;temp[2] = 0x21;temp[3] = 0x60;temp[4] = 0x24;
  temp[5] = 0x52;temp[6] = 0x06;temp[7] = 0x00;temp[8] = 0x66;temp[9] = 0x66;
  Hydrology_WriteStoreInfo(HYDROLOGY_CENTER2_IP, temp, HYDROLOGY_CENTER_IP_LEN);
  temp[0] = 0x02;temp[1] = 0x12;temp[2] = 0x00;temp[3] = 0x78;temp[4] = 0x13;
  temp[5] = 0x91;temp[6] = 0x49;temp[7] = 0x00;temp[8] = 0x99;temp[9] = 0x99;
  Hydrology_WriteStoreInfo(HYDROLOGY_BACKUP2_IP, temp, HYDROLOGY_BACKUP_IP_LEN);
  temp[0] = 0x02;temp[1] = 0x12;temp[2] = 0x00;temp[3] = 0x78;temp[4] = 0x13;
  temp[5] = 0x91;temp[6] = 0x49;temp[7] = 0x00;temp[8] = 0x99;temp[9] = 0x99;
  Hydrology_WriteStoreInfo(HYDROLOGY_CENTER3_IP, temp, HYDROLOGY_CENTER_IP_LEN);
  temp[0] = 0x02;temp[1] = 0x12;temp[2] = 0x00;temp[3] = 0x78;temp[4] = 0x13;
  temp[5] = 0x91;temp[6] = 0x49;temp[7] = 0x00;temp[8] = 0x99;temp[9] = 0x99;
  Hydrology_WriteStoreInfo(HYDROLOGY_BACKUP3_IP, temp, HYDROLOGY_BACKUP_IP_LEN);
  temp[0] = 0x02;temp[1] = 0x12;temp[2] = 0x00;temp[3] = 0x78;temp[4] = 0x13;
  temp[5] = 0x91;temp[6] = 0x49;temp[7] = 0x00;temp[8] = 0x99;temp[9] = 0x99;
  Hydrology_WriteStoreInfo(HYDROLOGY_CENTER4_IP, temp, HYDROLOGY_CENTER_IP_LEN);
  temp[0] = 0x02;temp[1] = 0x12;temp[2] = 0x00;temp[3] = 0x78;temp[4] = 0x13;
  temp[5] = 0x91;temp[6] = 0x49;temp[7] = 0x00;temp[8] = 0x99;temp[9] = 0x99;
  Hydrology_WriteStoreInfo(HYDROLOGY_BACKUP4_IP, temp, HYDROLOGY_BACKUP_IP_LEN);
  temp[0] = 0x02;
  Hydrology_WriteStoreInfo(HYDROLOGY_WORK_MODE, temp, HYDROLOGY_WORK_MODE_LEN);
  temp[0] = 0x80;temp[1] = 0x01;temp[2] = 0x06;temp[3] = 0x01;temp[4] = 0x00;
  temp[5] = 0x00;temp[6] = 0x00;temp[7] = 0x00;
  Hydrology_WriteStoreInfo(HYDROLOGY_ELEMENT_SELECT, temp, HYDROLOGY_ELEMENT_SELECT_LEN);
  temp[0] = 0x00;temp[1] = 0x00;temp[2] = 0x00;temp[3] = 0x00;temp[4] = 0x00;temp[5] = 0x00;
  temp[6] = 0x00;temp[7] = 0x00;temp[8] = 0x00;temp[9] = 0x00;temp[10] = 0x00;temp[11] = 0x00;
  Hydrology_WriteStoreInfo(HYDROLOGY_REPEATER_STATION, temp, HYDROLOGY_REPEATER_STATION_LEN);
  temp[0] = '1';temp[1] = '1';temp[2] = '2';temp[3] = '3';temp[4] = '4';temp[5] = '5';
  temp[6] = '6';temp[7] = '7';temp[8] = '8';temp[9] = '9';temp[10] = '0';temp[11] = '1';
  Hydrology_WriteStoreInfo(HYDROLOGY_DEVICE_ID, temp, HYDROLOGY_DEVICE_ID_LEN);
  temp[0] = 0x01;
  Hydrology_WriteStoreInfo(HYDROLOGY_TIMER_INTERVAL, temp, HYDROLOGY_TIMER_INTERVAL_LEN);
  temp[0] = 0x05;
  Hydrology_WriteStoreInfo(HYDROLOGY_ADD_INTERVAL, temp, HYDROLOGY_ADD_INTERVAL_LEN);
  temp[0] = 0x08;
  Hydrology_WriteStoreInfo(HYDROLOGY_RAINFALL_BEGIN_TIME, temp, HYDROLOGY_RAINFALL_BEGIN_TIME_LEN);
  temp[0] = 0x03;temp[1] = 0x00;
  Hydrology_WriteStoreInfo(HYDROLOGY_SAMPLE_INTERVAL, temp, HYDROLOGY_SAMPLE_INTERVAL_LEN);
  temp[0] = 0x05;
  Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_STORE_INTERVAL, temp, HYDROLOGY_WATERLEVEL_STORE_INTERVAL_LEN);
  temp[0] = 0x05;
  Hydrology_WriteStoreInfo(HYDROLOGY_RAINFALL_RESOLUTION, temp, HYDROLOGY_RAINFALL_RESOLUTION_LEN);
  temp[0] = 0x01;
  Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_RESOLUTION, temp, HYDROLOGY_WATERLEVEL_RESOLUTION_LEN);
  temp[0] = 0x01;
  Hydrology_WriteStoreInfo(HYDROLOGY_RAINFALL_ADD_THRESHOLD, temp, HYDROLOGY_RAINFALL_ADD_THRESHOLD_LEN);
  temp[0] = 0x01;temp[1] = 0x00;temp[2] = 0x00;temp[3] = 0x00;
  Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_BASICVALUE1, temp, HYDROLOGY_WATERLEVEL_BASICVALUE1_LEN);
  temp[0] = 0x01;temp[1] = 0x00;temp[2] = 0x00;
  Hydrology_WriteStoreInfo(HYDROLOGY_WATERLEVEL_CORRECTVALUE1, temp, HYDROLOGY_WATERLEVEL_CORRECTVALUE1_LEN);
  temp[0] = 0x25;temp[1] = 0x01;
  Hydrology_WriteStoreInfo(HYDROLOGY_ADD_WATERLEVEL1, temp, HYDROLOGY_ADD_WATERLEVEL1_LEN);
  temp[0] = 0x01;temp[1] = 0x00;
  Hydrology_WriteStoreInfo(HYDROLOGY_ADD_ABOVE_THRESHOLD, temp, HYDROLOGY_ADD_ABOVE_THRESHOLD_LEN);
  temp[0] = 0x03;temp[1] = 0x00;
  Hydrology_WriteStoreInfo(HYDROLOGY_ADD_BELOW_THRESHOLD, temp, HYDROLOGY_ADD_BELOW_THRESHOLD_LEN);
  memcpy(temp,"*WHU-2018-V2.0",14);
  Hydrology_WriteStoreInfo(HYDROLOGY_SOFTWARE_VERSION, temp, HYDROLOGY_SOFTWARE_VERSION_LEN);
//  temp[0] = 1;
//  Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT1, temp, HYDROLOGY_RS485_COUNT_LEN);
//  temp[0] = 0x01;temp[1] = 0x03;temp[2] = 0x00;temp[3] = 0x03;temp[4] = 5;
//  Hydrology_WriteStoreInfo(HYDROLOGY_RS4851, temp, HYDROLOGY_RS485_LEN);
  temp[0] = 1;
  Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT1, temp, HYDROLOGY_RS485_COUNT_LEN);
  temp[0] = 0x01;temp[1] = 0x03;temp[2] = 0x00;temp[3] = 0x00;temp[4] = 2;
  Hydrology_WriteStoreInfo(HYDROLOGY_RS4851, temp, HYDROLOGY_RS485_LEN);
  temp[0] = 1;
  Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT2, temp, HYDROLOGY_RS485_COUNT_LEN);
  temp[0] = 0x01;temp[1] = 0x03;temp[2] = 0x00;temp[3] = 0x08;temp[4] = 2;
  Hydrology_WriteStoreInfo(HYDROLOGY_RS4852, temp, HYDROLOGY_RS485_LEN);
//  temp[0] = 1;
//  Hydrology_WriteStoreInfo(HYDROLOGY_RS485_COUNT3, temp, HYDROLOGY_RS485_COUNT_LEN);
//  temp[0] = 0x01;temp[1] = 0x03;temp[2] = 0x00;temp[3] = 0x1E;temp[4] = 2;
//  Hydrology_WriteStoreInfo(HYDROLOGY_RS4853, temp, HYDROLOGY_RS485_LEN);
  
  return 0;
}

static int Hydrology_SetPassword(void)
{
  hydrologyDownBody* pbody = (hydrologyDownBody*)(g_HydrologyMsg.downbody);

  Hydrology_WriteStoreInfo(HYDROLOGY_PASSWORD_ADDR,(pbody->element)[0].value, HYDROLOGY_PASSWORD_LEN);
          
  return 0;
}

static int hydrologySetClock(void)
{
  hydrologyDownBody* pbody = (hydrologyDownBody*)(g_HydrologyMsg.downbody);

  Hydrology_SetTime(pbody->sendtime);
          
  return 0;
}

static int hydrologySetICCard(void)
{
  hydrologyDownBody* pbody = (hydrologyDownBody*)(g_HydrologyMsg.downbody);

  Hydrology_WriteStoreInfo(HYDROLOGY_STATUS_ALARM_INFO,(pbody->element)[0].value, HYDROLOGY_STATUS_ALARM_INFO_LEN);
          
  return 0;
}

static int hydrologyPump(void)
{
  hydrologyDownBody* pbody = (hydrologyDownBody*)(g_HydrologyMsg.downbody);

  Hydrology_WriteStoreInfo(HYDROLOGY_PUMP,(pbody->element)[0].value, HYDROLOGY_PUMP_LEN);
          
  return 0;
}

static int hydrologyValve(void)
{
  hydrologyDownBody* pbody = (hydrologyDownBody*)(g_HydrologyMsg.downbody);

  Hydrology_WriteStoreInfo(HYDROLOGY_VALVE,(pbody->element)[0].value, HYDROLOGY_VALVE_LEN);
          
  return 0;
}

static int hydrologyGate(void)
{
  char gatesize;
  hydrologyDownBody* pbody = (hydrologyDownBody*)(g_HydrologyMsg.downbody);
  
  gatesize = (pbody->element)[0].guide[0];
  gatesize = ((gatesize - 1)/8 + 1) + 2*gatesize + 1;
  Hydrology_WriteStoreInfo(HYDROLOGY_GATE,(pbody->element)[0].value, HYDROLOGY_GATE_LEN);
          
  return 0;
}

static int hydrologyWaterSetting(void)
{
  hydrologyDownBody* pbody = (hydrologyDownBody*)(g_HydrologyMsg.downbody);

  Hydrology_WriteStoreInfo(HYDROLOGY_WATERSETTING,(pbody->element)[0].value, HYDROLOGY_WATERSETTING);
          
  return 0;
}

static int HydrologyRecord(int index)
{
  uint16_t ERC_Couter = 0;
  int addr = (index - 1) * 2;
  char _temp_ERC_Couter[2];

  Hydrology_ReadStoreInfo(HYDROLOGY_RECORD + addr, _temp_ERC_Couter, 2);
  ERC_Couter = (_temp_ERC_Couter[0] << 8) + _temp_ERC_Couter[1];
  ERC_Couter++;
  _temp_ERC_Couter[0] = ERC_Couter >> 8;
  _temp_ERC_Couter[1] = ERC_Couter & 0x00FF;
  Hydrology_WriteStoreInfo(HYDROLOGY_RECORD + addr, _temp_ERC_Couter, 2);
                  
  return 0;
}

char IsQuery = 0;
int hydrologyCommand(int type)
{
  switch(type)
  {
    case Picture:
    {
//     Picture_Flag = 1;
      hydrologyChangeMode(HYDROLOGY_M3);
      break;
    }        
    case ConfigurationModification:
    {
      hydrologyBasicInfoConfig();
      break;
    }
    case ConfigurationRead:
    {
      hydrologyBasicInfoRead();
      IsQuery = 1;
      break;
    }
    case ParameterModification:
    {
      hydrologySetPara();
      HydrologyRecord(ERC2);
      break;
    }
    case ParameterRead:
    {
      hydrologyReadPara();
      IsQuery = 1;
      break;
    }
    case InitializeSolidStorage:
    {
      hydrologyInitializeSolidStorage();
      HydrologyRecord(ERC1);
      break;
    }
    case Reset:
    {
      hydrologySetClock();
      Hydrology_Reset();
      break;
    }        
    case ChangePassword:
    {
      Hydrology_SetPassword();
      HydrologyRecord(ERC5);
      break;
    }        
    case SetClock:
    {
      hydrologySetClock();
      break;
    }
    case SetICCard:
    {
      hydrologySetICCard();
      break;
    }
    case Pump:
    {
      hydrologyPump();
      break;
    }
    case Valve:
    {
      hydrologyValve();
      break;
    }        
    case Gate:
    {
      hydrologyGate();
      break;
    }
    case WaterSetting:
    {
      hydrologyWaterSetting();
      break;
    }        
  }
  return 0;
}

int HydrologyReadSuiteElement(int type, char* guide, char* value)
{
  switch(type)
  {
    case Picture:
    {
//     Picture_Flag = 1;
      hydrologyChangeMode(HYDROLOGY_M3);
      break;
    }        
    case ConfigurationModification:
    {
      hydrologyReadSelect(guide, value);
      break;
    }
    case ConfigurationRead:
    {
      hydrologyReadSelect(guide, value);
      break;
    }
    case ParameterModification:
    {
      hydrologyReadParaSelect(guide, value);
      break;
    }
    case ParameterRead:
    {
      hydrologyReadParaSelect(guide, value);
      break;
    }
    case InitializeSolidStorage:
    {
      hydrologyInitializeSolidStorage();
      HydrologyRecord(ERC1);
      break;
    }
    case Reset:
    {
      ;
      break;
    }        
    case ChangePassword:
    {
      Hydrology_SetPassword();
      HydrologyRecord(ERC5);
      break;
    }        
    case SetClock:
    {
      hydrologySetClock();
      break;
    }
    case SetICCard:
    {
      hydrologySetICCard();
      break;
    }
    case Pump:
    {
      hydrologyPump();
      break;
    }
    case Valve:
    {
      hydrologyValve();
      break;
    }        
    case Gate:
    {
      hydrologyGate();
      break;
    }
    case WaterSetting:
    {
      hydrologyWaterSetting();
      break;
    }        
  }                
  return 0;
}

