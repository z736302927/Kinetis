#include "hydrology-protocol/message.h"
#include "hydrology-protocol/hydrologycommand.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h" 

extern int IsDebug;
extern char IsQuery;
extern char isUARTConfig;

uint8_t ADCElementCount = 0;
uint8_t ISR_COUNTElementCount = 0;
uint8_t IO_STATUSElementCount = 0;
uint8_t RS485ElementCount = 0;

hydrologyHeader g_HydrologyUpHeader;
hydrologyHeader g_HydrologyDownHeader;
packet g_HydrologyMsg;

hydrologyElement inputPara[MAX_ELEMENT];

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  .
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "rtc.h"
#include "ff.h"
#include "fatfs.h"
#include "nb_iot/nb_app.h"

#define DEBUG
#include "idebug/idebug.h"


#define HydrologyMsg_printf    p_dbg

const hydrologyElementInfo Element_table[] = {
  ELEMENT_REGISTER(0x03, HYDROLOGY_ANALOG, 3, 1, HYDROLOGY_ADC),
  ELEMENT_REGISTER(0x18, HYDROLOGY_ANALOG, 4, 1, HYDROLOGY_ADC),
//  ELEMENT_REGISTER(0x61, HYDROLOGY_PULSE, 11, 3, HYDROLOGY_ISR_COUNT),
//  ELEMENT_REGISTER(0x62, HYDROLOGY_PULSE, 11, 3, HYDROLOGY_ISR_COUNT),
//  ELEMENT_REGISTER(0x27, HYDROLOGY_ANALOG, 9, 3, HYDROLOGY_RS485),
//  ELEMENT_REGISTER(0x60, HYDROLOGY_PULSE, 11, 3, HYDROLOGY_RS485),
  ELEMENT_REGISTER(NULL, NULL, NULL, NULL, NULL)
  
};

int SinglePacketSize = 0;
static int RomElementBeginAddr = 0;
static int SinglePacketSendCount = 0;
static int PacketSendTimes = 0;

char HYDROLOGY_MODE = HYDROLOGY_M1;

int Hydrology_ReadStoreInfo(long addr, char *data, int len)
{
  FIL HydrologyFile;
  FRESULT res;
  uint32_t bytesread;
  
  res = f_open(&HydrologyFile, "0:/Hydrology_Data/hydrologyElement.txt", FA_OPEN_EXISTING | FA_READ);    
  if(res == FR_OK)
  {
    f_lseek(&HydrologyFile, addr);
    res = f_read(&HydrologyFile, data, len, (void *)&bytesread); 
    if(res != FR_OK)
      return -1;
  }
  else
  {
    HydrologyMsg_printf("Read message failed.");
    return -1;
  }
  f_close(&HydrologyFile);
  return 0;
}

int Hydrology_WriteStoreInfo(long addr, char *data, int len)
{
  FIL HydrologyFile;
  FRESULT res;
  uint32_t byteswritten;
  
  res = f_open(&HydrologyFile, "0:/Hydrology_Data/hydrologyElement.txt",FA_OPEN_EXISTING | FA_WRITE);
  if (res == FR_OK)
  {
    f_lseek(&HydrologyFile, addr);
    res = f_write(&HydrologyFile, data, len, (void *)&byteswritten);
    if(res != FR_OK)
      return -1;

    f_close(&HydrologyFile);
  }
  else if(res == FR_NO_FILE || res == FR_NO_PATH)
  {
    f_mkdir("0:/Hydrology_Data");
    f_open(&HydrologyFile, "0:/Hydrology_Data/hydrologyElement.txt",FA_CREATE_NEW | FA_WRITE);
    f_lseek(&HydrologyFile, addr);
    res = f_write(&HydrologyFile, data, len, (void *)&byteswritten);
    if(res != FR_OK)
      return -1;

    f_close(&HydrologyFile);
  }
  else
  {  
    HydrologyMsg_printf("Write message failed.");
    return -1;
  }
  return 0;
}

void Hydrology_ReadTime(char* time)
{
  RTC_DateTypeDef sdatestructureget;
  RTC_TimeTypeDef stimestructureget;

  /* Get the RTC current Time */
  HAL_RTC_GetTime(&hrtc,  &stimestructureget, RTC_FORMAT_BCD);
  /* Get the RTC current Date */
  HAL_RTC_GetDate(&hrtc,  &sdatestructureget, RTC_FORMAT_BCD);
  
  time[0] = sdatestructureget.Year;
  time[1] = sdatestructureget.Month;
  time[2] = sdatestructureget.Date;
  time[3] = stimestructureget.Hours;
  time[4] = stimestructureget.Minutes;
  time[5] = stimestructureget.Seconds;
}

void Hydrology_SetTime(char* time_temp)
{
//  uint8_t temp[7];
//  uint8_t time[12];
//  uint8_t i = 0;
//  
//  for(i = 0;i < 12;i++)
//    time[i] = time_temp[i] - '0';
//  
//  temp[0] = (time[0] << 4) + time[1];
//  temp[1] = (time[2] << 4) + time[3];
//  temp[2] = (time[4] << 4) + time[5];
//  temp[3] = (time[6] << 4) + time[7];
//  temp[4] = (time[8] << 4) + time[9];
//  temp[5] = (time[10] << 4) + time[11];

  RTC_CalendarConfig(time_temp[0], time_temp[1], time_temp[2], time_temp[3], time_temp[4], time_temp[5], 0x07);
}

int Hydrology_SendData(char* data, int len)
{
  p_hex(data, len);
  NB_IOT_SendData(data, len);
  return 0;
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

void Hydrology_SetRTUType(char* type)
{
  Hydrology_WriteStoreInfo(HYDROLOGY_RTUTYPE, type, HYDROLOGY_RTUTYPE_LEN);
}

void Hydrology_ReadObservationTime(char id, char* observationtime, int index)
{
  long addr = HYDROLOGY_ANALOG1_OBSERVATION_TIME + index * HYDROLOGY_OBSERVATION_TIME_LEN;
  int i = 0;
  
  while(Element_table[i].ID != 0)
  {
    if(Element_table[i].ID == id)
    {
      switch(Element_table[i].type)
      {
        case HYDROLOGY_ANALOG:
        {
          addr = HYDROLOGY_ANALOG1_OBSERVATION_TIME + index * HYDROLOGY_OBSERVATION_TIME_LEN;
          break;
        }
        case HYDROLOGY_PULSE:
        {
          addr = HYDROLOGY_PULSE1_OBSERVATION_TIME + index * HYDROLOGY_OBSERVATION_TIME_LEN;
          break;
        }
        case HYDROLOGY_SWITCH:
        {
          addr = HYDROLOGY_SWITCH1_OBSERVATION_TIME + index * HYDROLOGY_OBSERVATION_TIME_LEN;
          break;
        }
        default:
          break;
      }
    }
    i++;
  }
  
  Hydrology_ReadStoreInfo(addr, observationtime, HYDROLOGY_OBSERVATION_TIME_LEN);
}

void Hydrology_SetObservationTime(char id, int index)
{
  long addr = HYDROLOGY_ANALOG1_OBSERVATION_TIME + index * HYDROLOGY_OBSERVATION_TIME_LEN;
  int i = 0;
  char observationtime[6] = {0,0,0,0,0,0};
  
  while(Element_table[i].ID != 0)
  {
    if(Element_table[i].ID == id)
    {
      switch(Element_table[i].type)
      {
        case HYDROLOGY_ANALOG:
        {
          addr = HYDROLOGY_ANALOG1_OBSERVATION_TIME + index * HYDROLOGY_OBSERVATION_TIME_LEN;
          break;
        }
        case HYDROLOGY_PULSE:
        {
          addr = HYDROLOGY_PULSE1_OBSERVATION_TIME + index * HYDROLOGY_OBSERVATION_TIME_LEN;
          break;
        }
        case HYDROLOGY_SWITCH:
        {
          addr = HYDROLOGY_SWITCH1_OBSERVATION_TIME + index * HYDROLOGY_OBSERVATION_TIME_LEN;
          break;
        }
        default:
          break;
      }
    }
    i++;
  }
  
  Hydrology_ReadTime(observationtime);
  Hydrology_WriteStoreInfo(addr, observationtime, HYDROLOGY_OBSERVATION_TIME_LEN);
}

static void getBCDnums(double num,  int * intergerpart,  int* decimerpart,  int d,  char* pout_intergerValue,  char * pout_decimerValue)
{
  char strfloat[20];
  int i = 0;
  int len = 0;
  int j = 0;
  int k = 0;

  for ( i = 0; i < 20 ; i++)
  {
    strfloat[i] = 'X';
  }
  sprintf(strfloat,"%f",num);
  for ( i = 0; i < 20; i++)
  {
    if ( 'X' != strfloat[i] )
      len++;
  }
  for ( i = 0 ; i < len ; i++)
  {
    if ( '0' != strfloat[i])
    {
      break;
    }
  }
  len = len - i - 1;

  for ( i = 0; i < len ; i++)
  {
    if ('.' == strfloat[i])
    {
      j = i;
      break;
    }
  }
  if ( i < len ) //
  {
    *decimerpart = len - j - 1;
    if(*decimerpart > d)
    {
      *decimerpart = d;
    }

    *intergerpart = j;

    for ( i = 0; i < j ; i++)
    {
      pout_intergerValue[i] = strfloat[i];
    }
    pout_intergerValue[j] = 0;

    for ( i = j+1; (i < len) && (k < (*decimerpart)); i++)
    {
      pout_decimerValue[k++] = strfloat[i];
    }
    pout_decimerValue[k] = 0;
  }
  else 
  {
    *decimerpart = 0;
    *intergerpart = len;
    sprintf(pout_intergerValue,"%d",(int)num);
  }
}

static int getEvenNum(int num)
{
  if(num%2 == 0)
    return num;
  else 
    return num+1;
}

/* The byte is 5 bits high, D represents the number of data bytes, and 3 bits low, D represents the number of decimal places */
static void getguideid(char* value, char D,  char d)
{
  char high5 = 0;
  char low3 = 0;
  char evenD = 0;

  evenD = getEvenNum(D);

  high5 = evenD/2;
  high5 = high5 << 3;
  low3 = d; //D has to be between 0 and 7 to be represented in 3 digits.
  *value = high5 | low3;
}


static int converToHexElement(double input,  int D, int d, char* out)
{
  char strInterValue[20] = {0}; //StrInterValue represents an integer value
  char strDeciValue[20] = {0};  //StrDeciValue means a small value
  int integer =0;               //Interger represents an integer number
  int decimer =0;               //Interger for integer number decimer for decimal number
  //int intergerValue = 0 ;     //Represents integral value
  //int decimerValue = 0 ;      //Representing a small number
  int total = 0;                //Total represents the total number of input digits (minus the decimal point).
  int evenD = 0;                //Even D
  int difftotal = 0;            //Represents the difference between evenD and total
  int diffInterger = 0;         //Indicates that integer bits need to be completed
  int diffDecimer = 0;          //Indicates that the decimal place needs to be filled
  //int delDecimer = 0 ;        //Represents the number of digits in the decimal place to delete
  int i = 0;
  int j = 0;
  int m = 0;

  char tmp[30];
  for(m = 0 ; m < 30 ; m++)
  {
    tmp[m] = '0';
  }
  getBCDnums(input,&integer,&decimer,  d, strInterValue, strDeciValue);
  evenD = getEvenNum(D);
  total = integer + decimer;

  if ( evenD >= total )         //Input configuration parameters are guaranteed
  {
    difftotal = evenD - total;
    if( d >= decimer )          //This is definitely going to happen, getBCDnums guarantees
    {
      diffDecimer = d - decimer;//The number of digits in the decimal place that need to be filled in
      diffInterger = difftotal - diffDecimer;/* Integer bit needs to fill in the number of digits of 0, 
                                                assuming that difftotal is always greater than diffDecimer */
    }
    /* The current decimal and integer bits are integrated into the TMP array, 
        divided into the following parts 0-- >diffInterger-1 integer fill 0 number, 
        diffInterger-- >diffInterger + interger is the number of integer bits, 
        diffInter+ interge --> evenD is the number of decimal bits */
    memcpy(&tmp[diffInterger],strInterValue,  integer);
    memcpy(&tmp[diffInterger+integer],strDeciValue,  decimer);

    tmp[evenD] = 0 ;

    for ( i = 0; i < evenD ; i=i+2)
    {
      out[j++] = (tmp[i] - '0') * 16 + (tmp[i+1]-'0');
    }
  }
  else //That will not happen now
  {
    return -1;
  }
  return 0;
}

static int mallocElement(char element, char D, char d, hydrologyElement* ele)
{
  ele->guide[0] = element;
  getguideid(&(ele->guide[1]),D, d);

  if ( D%2 == 0 )
  {
    ele->value = (char*)malloc(D/2);
    
    if (ele->value == 0)
    {
      return -1;
    }

    memset(ele->value, 0, D/2);
    
    ele->num = D/2;
  }
  else
  {
    ele->value = (char*)malloc((D+1)/2);
    
    if (ele->value == 0)
    {
      return -1;
    }

    memset(ele->value, 0,(D+1)/2);
    ele->num = (D+1)/2;
  }

  return 0;
}

static int hydrologyJudgeType(char funcode)
{
  int type;
  switch(funcode)
  {
    case LinkMaintenance:
    {
      type = 2;
      break;
    }
    case Test:
    {
      type = 1;
      break;
    }
    case EvenPeriodInformation:
    {
      type = 1;
      break;
    }
    case TimerReport:
    {
      type = 1;
      break;
    }
    case AddReport:
    {
      type = 1;
      break;
    }
    case Hour:
    {
      type = 1;
      break;
    }
    case ArtificialNumber:
    {
      type = 1;
      break;
    }
    case Picture:
    {
      type = 1;
      break;
    }
    case Realtime:
    {
      type = 1;
      break;
    }
    case Period:
    {
      type = 1;
      break;
    }
    case InquireArtificialNumber:
    {
      type = 1;
      break;
    }
    case SpecifiedElement:
    {
      type = 1;
      break;
    }
    case ConfigurationModification:
    {
      type = 3;
      break;
    }
    case ConfigurationRead:
    {
      type = 3;
      break;
    }
    case ParameterModification:
    {
      type = 3;
      break;
    }
    case ParameterRead:
    {
      type = 3;
      break;
    }
    case WaterPumpMotor:
    {
      type = 1;
      break;
    }
    case SoftwareVersion:
    {
      type = 1;
      break;
    }
    case Status:
    {
      type = 1;
      break;
    }
    case InitializeSolidStorage:
    {
      type = 1;
      break;
    }
    case Reset:
    {
      type = 3;
      break;
    }        
    case ChangePassword:
    {
      type = 1;
      break;
    }        
    case SetClock:
    {
      type = 1;
      break;
    }
    case SetICCard:
    {
      type = 1;
      break;
    }
    case Pump:
    {
      type = 1;
      break;
    }
    case Valve:
    {
      type = 1;
      break;
    }        
    case Gate:
    {
      type = 1;
      break;
    }
    case WaterSetting:
    {
      type = 1;
      break;
    }
    case Record:
    {
      type = 1;
      break;
    }       
    case Time:
    {
      type = 1;
      break;
    }
    default:
      break;
  }    
  return type;
  
}

static void Hydrology_ReadAnalog(float *value, int index)
{
  long addr = HYDROLOGY_ANALOG1 + index * 4;
  char temp_value[4];
  
  Hydrology_ReadStoreInfo(addr, temp_value, HYDROLOGY_ANALOG_LEN);
  *value = *((float*)temp_value);
}

static void Hydrology_ReadPulse(long *value, int index)
{
  long addr = HYDROLOGY_PULSE1 + index * 4;
  char temp_value[4];
  
  Hydrology_ReadStoreInfo(addr, temp_value, HYDROLOGY_PULSE_LEN);
  *value = *((long*)temp_value);
}

static void Hydrology_ReadSwitch(int *value)
{
  char temp_value[4];
  
  Hydrology_ReadStoreInfo(HYDROLOGY_SWITCH1, temp_value, HYDROLOGY_SWITCH_LEN);
  *value = *((int*)temp_value);
}

static void Hydrology_ReadRom(long beginaddr, char *value, int index)
{
  long addr = beginaddr + index * SinglePacketSize;
  
  Hydrology_ReadStoreInfo(addr, value, SinglePacketSize);
}

static void Hydrology_CalElementInfo(int *count, char funcode)
{
  int i = 0, acount = 0, pocunt = 0;
  float floatvalue = 0;
  long intvalue1 = 0;
  int intvalue2 = 0;
  int type = 0;
  hydrologyDownBody* downpbody = (hydrologyDownBody*) (g_HydrologyMsg.downbody);

  type = hydrologyJudgeType(funcode);
  
  if(type == 1)
  {
    while(Element_table[i].ID != 0)
    {
      switch(Element_table[i].type)
      {
        case HYDROLOGY_ANALOG:
        {
          Hydrology_ReadAnalog(&floatvalue, acount++);
          mallocElement(Element_table[i].ID, Element_table[i].D, Element_table[i].d,&inputPara[i]);
          converToHexElement((double)floatvalue, Element_table[i].D, Element_table[i].d, inputPara[i].value);
          break;
        }
        case HYDROLOGY_PULSE:
        {
          Hydrology_ReadPulse(&intvalue1, pocunt++);
          mallocElement(Element_table[i].ID, Element_table[i].D, Element_table[i].d,&inputPara[i]);
          converToHexElement((double)intvalue1, Element_table[i].D, Element_table[i].d, inputPara[i].value);
          break;
        }
        case HYDROLOGY_SWITCH:
        {
          Hydrology_ReadSwitch(&intvalue2);
          mallocElement(Element_table[i].ID, Element_table[i].D, Element_table[i].d,&inputPara[i]);
          converToHexElement((double)intvalue2, Element_table[i].D, Element_table[i].d, inputPara[i].value);
          break;
        }
        case HYDROLOGY_STORE:
        {
          inputPara[i].guide[0] = Element_table[i].ID;
          inputPara[i].guide[1] = Element_table[i].ID;
          inputPara[i].value = (char*)malloc(SinglePacketSize);
          if (NULL != inputPara[i].value)
          {
            inputPara[i].num = SinglePacketSize;
            Hydrology_ReadRom(RomElementBeginAddr, inputPara[i].value, SinglePacketSendCount++);
          }
          break;
        }
        default:
          break;
      }
      i++;
      (*count)++;
    }
  }
  else if(type == 2)
  {

  }
  else if(type == 3)
  {
    for(i = 0;i < downpbody->count;i++)
    {
      memcpy(inputPara[i].guide,(downpbody->element)[i].guide, 2);
      
      inputPara[i].num = ((downpbody->element)[i].guide[1] >> 3);
      inputPara[i].value = (char*) malloc((downpbody->element)[i].num);
      if (NULL == inputPara[i].value)
        continue;
      HydrologyReadSuiteElement(funcode, inputPara[i].guide, inputPara[i].value);
      
      (*count)++;
    }    
  }
}

static void hydrologyMakeUpHeader(char funcode)
{
//    char _temp_picturesize[2] = {0x01, 0x5E};
//    int picturesize;
    hydrologyHeader* pheader = (hydrologyHeader*) (g_HydrologyMsg.header);

    Hydrology_ReadStoreInfo(HYDROLOGY_PASSWORD_ADDR, pheader->password, HYDROLOGY_PASSWORD_LEN);
    
    pheader->framestart[0] = HYDROLOGY_SOH1;
    pheader->framestart[1] = HYDROLOGY_SOH2;

    Hydrology_ReadStoreInfo(HYDROLOGY_CENTER_ADDR,&(pheader->centeraddr),HYDROLOGY_CENTER_LEN-3);
    
    Hydrology_ReadStoreInfo(HYDROLOGY_REMOTE_ADDR, pheader->remoteaddr, HYDROLOGY_REMOTE_LEN);
    
    pheader->funcode = funcode;
    pheader->dir_len[0] = 0 << 4;
    
    if(HYDROLOGY_MODE == HYDROLOGY_M3)
    {
      SinglePacketSendCount++;
      pheader->paketstart = HYDROLOGY_SYN;
      
      pheader->count_seq[0] = PacketSendTimes >> 4;
      pheader->count_seq[1] = ((PacketSendTimes & 0x000F) << 4) + (SinglePacketSendCount >> 8);
      pheader->count_seq[2] = SinglePacketSendCount & 0x00FF;
    }
    else
      pheader->paketstart = HYDROLOGY_STX;
    
}

static void getstreamid(char streamid[2])
{
    static unsigned short id = 0 ;
    id++;
    id = id % 65536;
    if(id==0)
      id=1;
    streamid[0]=(id>>8)&0xff;
    streamid[1]=id&0xff;
}

static void hydrologyMakeUpBodyBasicInfo(char funcode)
{
  hydrologyUpBody* pbody = (hydrologyUpBody*) (g_HydrologyMsg.upbody);
  int type = 0;

  type = hydrologyJudgeType(funcode);
  pbody->len = 0;
  if(type == 1)
  {
    getstreamid(pbody->streamid);
    pbody->len += 2;
    Hydrology_ReadTime(pbody->sendtime);
    pbody->len += 6;
    pbody->rtuaddrid[0] = 0xF1;
    pbody->rtuaddrid[1] = 0xF1;
    pbody->len += 2;    
    Hydrology_ReadStoreInfo(HYDROLOGY_REMOTE_ADDR, pbody->rtuaddr, HYDROLOGY_REMOTE_LEN);
    pbody->len += 5;
    Hydrology_ReadStoreInfo(HYDROLOGY_RTUTYPE,&(pbody->rtutype),HYDROLOGY_RTUTYPE_LEN);
    pbody->len += 1;
    pbody->observationtimeid[0]=0xF0;
    pbody->observationtimeid[1]=0xF0;
    pbody->len += 2;
    Hydrology_ReadObservationTime(Element_table[0].ID, pbody->observationtime, 0);
    pbody->len += 5;
    if(SinglePacketSendCount > 1)
      pbody->len = 0;
  }
  else if(type == 2)
  {
    getstreamid(pbody->streamid);
    pbody->len += 2;
    Hydrology_ReadTime(pbody->sendtime);
    pbody->len += 6;
  }
  else if(type == 3)
  {
    getstreamid(pbody->streamid);
    pbody->len += 2;
    Hydrology_ReadTime(pbody->sendtime);
    pbody->len += 6;
    pbody->rtuaddrid[0] = 0xF1;
    pbody->rtuaddrid[1] = 0xF1;
    pbody->len += 2;    
    Hydrology_ReadStoreInfo(HYDROLOGY_REMOTE_ADDR, pbody->rtuaddr, HYDROLOGY_REMOTE_LEN);
    pbody->len += 5;
  }
}

static int hydrologyMakeUpBody(int count)
{
  hydrologyUpBody* pbody = (hydrologyUpBody*) (g_HydrologyMsg.upbody);
  int i;

  pbody->count = count;
  for(i = 0;i < pbody->count;i++)
  {
    memcpy((pbody->element)[i].guide,  inputPara[i].guide, 2);
    (pbody->element)[i].value = (char*) malloc(inputPara[i].num);
    if(NULL == (pbody->element)[i].value)
       return -1;
    memcpy((pbody->element)[i].value, inputPara[i].value,  inputPara[i].num);
    (pbody->element)[i].num = inputPara[i].num;     
    pbody->len += 2 + inputPara[i].num;
  }
  return 0;
}

const char chCRCHTalbe[] =
{
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x00,  0xC1,  0x81,  0x40, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x00,  0xC1,  0x81,  0x40, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x00,  0xC1,  0x81,  0x40, 
0x01,  0xC0,  0x80,  0x41,  0x01,  0xC0,  0x80,  0x41,  0x00,  0xC1,  0x81,  0x40, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x00,  0xC1,  0x81,  0x40, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41, 
0x01,  0xC0,  0x80,  0x41,  0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x00,  0xC1,  0x81,  0x40, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40,  0x01,  0xC0,  0x80,  0x41,  0x01,  0xC0,  0x80,  0x41, 
0x00,  0xC1,  0x81,  0x40};
const char chCRCLTalbe[] =
{
0x00,  0xC0,  0xC1,  0x01,  0xC3,  0x03,  0x02,  0xC2,  0xC6,  0x06,  0x07,  0xC7, 
0x05,  0xC5,  0xC4,  0x04,  0xCC,  0x0C,  0x0D,  0xCD,  0x0F,  0xCF,  0xCE,  0x0E, 
0x0A,  0xCA,  0xCB,  0x0B,  0xC9,  0x09,  0x08,  0xC8,  0xD8,  0x18,  0x19,  0xD9, 
0x1B,  0xDB,  0xDA,  0x1A,  0x1E,  0xDE,  0xDF,  0x1F,  0xDD,  0x1D,  0x1C,  0xDC, 
0x14,  0xD4,  0xD5,  0x15,  0xD7,  0x17,  0x16,  0xD6,  0xD2,  0x12,  0x13,  0xD3, 
0x11,  0xD1,  0xD0,  0x10,  0xF0,  0x30,  0x31,  0xF1,  0x33,  0xF3,  0xF2,  0x32, 
0x36,  0xF6,  0xF7,  0x37,  0xF5,  0x35,  0x34,  0xF4,  0x3C,  0xFC,  0xFD,  0x3D, 
0xFF,  0x3F,  0x3E,  0xFE,  0xFA,  0x3A,  0x3B,  0xFB,  0x39,  0xF9,  0xF8,  0x38, 
0x28,  0xE8,  0xE9,  0x29,  0xEB,  0x2B,  0x2A,  0xEA,  0xEE,  0x2E,  0x2F,  0xEF, 
0x2D,  0xED,  0xEC,  0x2C,  0xE4,  0x24,  0x25,  0xE5,  0x27,  0xE7,  0xE6,  0x26, 
0x22,  0xE2,  0xE3,  0x23,  0xE1,  0x21,  0x20,  0xE0,  0xA0,  0x60,  0x61,  0xA1, 
0x63,  0xA3,  0xA2,  0x62,  0x66,  0xA6,  0xA7,  0x67,  0xA5,  0x65,  0x64,  0xA4, 
0x6C,  0xAC,  0xAD,  0x6D,  0xAF,  0x6F,  0x6E,  0xAE,  0xAA,  0x6A,  0x6B,  0xAB, 
0x69,  0xA9,  0xA8,  0x68,  0x78,  0xB8,  0xB9,  0x79,  0xBB,  0x7B,  0x7A,  0xBA, 
0xBE,  0x7E,  0x7F,  0xBF,  0x7D,  0xBD,  0xBC,  0x7C,  0xB4,  0x74,  0x75,  0xB5, 
0x77,  0xB7,  0xB6,  0x76,  0x72,  0xB2,  0xB3,  0x73,  0xB1,  0x71,  0x70,  0xB0, 
0x50,  0x90,  0x91,  0x51,  0x93,  0x53,  0x52,  0x92,  0x96,  0x56,  0x57,  0x97, 
0x55,  0x95,  0x94,  0x54,  0x9C,  0x5C,  0x5D,  0x9D,  0x5F,  0x9F,  0x9E,  0x5E, 
0x5A,  0x9A,  0x9B,  0x5B,  0x99,  0x59,  0x58,  0x98,  0x88,  0x48,  0x49,  0x89, 
0x4B,  0x8B,  0x8A,  0x4A,  0x4E,  0x8E,  0x8F,  0x4F,  0x8D,  0x4D,  0x4C,  0x8C, 
0x44,  0x84,  0x85,  0x45,  0x87,  0x47,  0x46,  0x86,  0x82,  0x42,  0x43,  0x83, 
0x41,  0x81,  0x80,  0x40};

short hydrologyCRC16(char* pchMsg,  int wDataLen)
{
  char chCRCHi = 0xFF;
  char chCRCLo = 0xFF;
  short wIndex;
  while (wDataLen--)
  {
    wIndex = chCRCLo ^ *pchMsg++ ;
    chCRCLo = chCRCHi ^ chCRCHTalbe[wIndex];
    chCRCHi = chCRCLTalbe[wIndex] ;
  }
  return ((chCRCHi << 8) | chCRCLo) ;
}

int hydrologyCheck(char* input, int inputlen)
{
  short crcRet = 0;
  short inputCrc = 0;
  int bodylen = 0;

  crcRet = hydrologyCRC16(input, inputlen - 2);

  inputCrc = (input[inputlen - 2] << 8) | input[inputlen - 1];

  if(crcRet == inputCrc)
  {
    ;//TraceMsg("CRC check success !",1);
  }
  else
  {
    HydrologyMsg_printf("CRC check fail !");
    return -1;
  }

  if((input[0] == HYDROLOGY_SOH1) && (input[1] == HYDROLOGY_SOH2))
  {
    ;//TraceMsg("Frame head check success !",1);
  }
  else
  {
    HydrologyMsg_printf("Frame head check fail !");
    return -2;
  }

  bodylen = (input[11] & 0x0F) * 256 + input[12];

  if(bodylen == (inputlen - 17))
  {
    ;//TraceMsg("Hydrolog length check success !",1);
  }
  else
  {
    HydrologyMsg_printf("Hydrolog length check fail !");
    return -3;
  }
  return 0;
}

static int hydrologyMakeDownHeader(char* input, int inputlen, int *position, int *bodylen)
{
  hydrologyHeader* pheader = &g_HydrologyDownHeader;

  if(hydrologyCheck(input,  inputlen) == 0)
  {
    ;//TraceMsg("Hydrolog check success !",1);
  }
  else
  {
    HydrologyMsg_printf("Hydrology check fail !");
    return -1;
  }
    
  memcpy(pheader->framestart,&input[*position],2);
  *position += 2;

  memcpy(pheader->remoteaddr,&input[*position],5);
  *position += 5;    

  memcpy(&(pheader->centeraddr),&input[*position],1);
  *position += 1;

  memcpy(pheader->password,&input[*position],2);
  *position += 2;

  memcpy(&(pheader->funcode),&input[*position],1);
  *position += 1;

  memcpy(pheader->dir_len,&input[*position],1);
  pheader->dir_len[0] >>= 4;

  *bodylen = (input[*position] & 0x0F) * 256 + input[*position+1];
  *position += 2;

  memcpy(&(pheader->paketstart),&input[*position],1);
  *position += 1;

  if(pheader->paketstart == HYDROLOGY_SYN)
  {
    memcpy(pheader->count_seq,&input[*position],3);
    *position += 3;
  }
  return 0;
}

static int hydrologyNeedEditElement(char funcode)
{
  int trueorfalse = 0;
  switch(funcode)
  {
    case LinkMaintenance:
    {
      trueorfalse = 1;
      break;
    }
    case Test:
    {
      trueorfalse = 1;
      break;
    }
    case EvenPeriodInformation:
    {
      trueorfalse = 1;
      break;
    }
    case TimerReport:
    {
      trueorfalse = 1;
      break;
    }
    case AddReport:
    {
      trueorfalse = 1;
      break;
    }
    case Hour:
    {
      trueorfalse = 1;
      break;
    }
    case ArtificialNumber:
    {
      trueorfalse = 1;
      break;
    }
    case Picture:
    {
      trueorfalse = 1;
      break;
    }
    case Realtime:
    {
      trueorfalse = 1;
      break;
    }
    case Period:
    {
      trueorfalse = 1;
      break;
    }
    case InquireArtificialNumber:
    {
      trueorfalse = 1;
      break;
    }
    case SpecifiedElement:
    {
      trueorfalse = 1;
      break;
    }
    case ConfigurationModification:
    {
      trueorfalse = 1;
      break;
    }
    case ConfigurationRead:
    {
      trueorfalse = 1;
      break;
    }
    case ParameterModification:
    {
      trueorfalse = 1;
      break;
    }
    case ParameterRead:
    {
      trueorfalse = 1;
      break;
    }
    case WaterPumpMotor:
    {
      trueorfalse = 1;
      break;
    }
    case SoftwareVersion:
    {
      trueorfalse = 1;
      break;
    }
    case Status:
    {
      trueorfalse = 1;
      break;
    }
    case InitializeSolidStorage:
    {
      trueorfalse = 1;
      break;
    }
    case Reset:
    {
      trueorfalse = 0;
      break;
    }        
    case ChangePassword:
    {
      trueorfalse = 1;
      break;
    }        
    case SetClock:
    {
      trueorfalse = 1;
      break;
    }
    case SetICCard:
    {
      trueorfalse = 1;
      break;
    }
    case Pump:
    {
      trueorfalse = 1;
      break;
    }
    case Valve:
    {
      trueorfalse = 1;
      break;
    }        
    case Gate:
    {
      trueorfalse = 1;
      break;
    }
    case WaterSetting:
    {
      trueorfalse = 1;
      break;
    }
    case Record:
    {
      trueorfalse = 1;
      break;
    }       
    case Time:
    {
      trueorfalse = 1;
      break;
    }
    default:
      break;
  }    
  return trueorfalse;
}

static int hydrologyMakeDownBody(char* input, int len, int position)
{
  hydrologyDownBody* pbody = (hydrologyDownBody*) (g_HydrologyMsg.downbody);
  int trueorfalse = 0;

  trueorfalse = hydrologyNeedEditElement(input[10]);

  memcpy(pbody->streamid,&input[position],2);
  position += 2;
  len -= 2;

  memcpy(pbody->sendtime,&input[position],6);
  position += 6;
  len -= 6;

  pbody->count = 0;
  if(!trueorfalse)
    return 0;
  while(len != 0)
  {
    memcpy((pbody->element)[pbody->count].guide,  &input[position],2);
    position += 2;
    len -= 2;
    
    (pbody->element)[pbody->count].num = ((pbody->element)[pbody->count].guide[1] >> 3);
    (pbody->element)[pbody->count].value = (char*) malloc((pbody->element)[pbody->count].num);
    if (NULL == (pbody->element)[pbody->count].value)
      return -1;
    memcpy((pbody->element)[pbody->count].value,&input[position],(pbody->element)[pbody->count].num);
    position += (pbody->element)[pbody->count].num;
    len -= (pbody->element)[pbody->count].num;
    
    pbody->count ++;
    if(len < 0)
      return -2;
  }
  return 0;
}

static void hydrologyMakeUpTail(char* buffer, int *pbuffer, char funcode)
{
  hydrologyHeader* pheader = (hydrologyHeader*) (g_HydrologyMsg.header);
  hydrologyUpBody* uppbody = (hydrologyUpBody*) (g_HydrologyMsg.upbody);
  hydrologyDownBody* downpbody = (hydrologyDownBody*) (g_HydrologyMsg.downbody);
  int i, bodylen;
  int type = 0;

  type = hydrologyJudgeType(funcode);

  memcpy(buffer, g_HydrologyMsg.header, sizeof(hydrologyHeader));
  *pbuffer = sizeof(hydrologyHeader) - 3;

  if(HYDROLOGY_MODE == HYDROLOGY_M3)
  {
    g_HydrologyMsg.end = HYDROLOGY_ETB;
    if(PacketSendTimes == SinglePacketSendCount)
      g_HydrologyMsg.end = HYDROLOGY_ETX;
  }
  else
    g_HydrologyMsg.end = HYDROLOGY_ETX;

  if(type == 1)
  {
    memcpy(&buffer[*pbuffer],g_HydrologyMsg.upbody, 23);
    *pbuffer += 23;
    for(i = 0;i < uppbody->count;i++)
    {
      memcpy(&buffer[*pbuffer],(uppbody->element)[i].guide, 2);
      *pbuffer += 2;
      memcpy(&buffer[*pbuffer],(uppbody->element)[i].value,  (uppbody->element)[i].num);
      *pbuffer += (uppbody->element)[i].num;
    }
    buffer[*pbuffer] = g_HydrologyMsg.end;
    *pbuffer += 1;
    bodylen = *pbuffer - 15;
    pheader->dir_len[0] |= bodylen>>8;
    pheader->dir_len[1] |= bodylen&0xFF;
    memcpy(&buffer[11],pheader->dir_len, 2);
    
    g_HydrologyMsg.crc16 = hydrologyCRC16(buffer,*pbuffer);
    buffer[*pbuffer] = g_HydrologyMsg.crc16 >> 8;
    *pbuffer += 1;
    buffer[*pbuffer] = g_HydrologyMsg.crc16 & 0xFF;
    *pbuffer += 1;
  }
  else if(type == 2)
  {
    memcpy(&buffer[*pbuffer],g_HydrologyMsg.upbody, 8);
    *pbuffer += 8;
    buffer[*pbuffer] = g_HydrologyMsg.end;
    *pbuffer += 1;
    bodylen = *pbuffer - 15;
    pheader->dir_len[0] |= bodylen>>8;
    pheader->dir_len[1] |= bodylen&0xFF;
    memcpy(&buffer[11],pheader->dir_len, 2);
    
    g_HydrologyMsg.crc16 = hydrologyCRC16(buffer,*pbuffer);
    buffer[*pbuffer] = g_HydrologyMsg.crc16 >> 8;
    *pbuffer += 1;
    buffer[*pbuffer] = g_HydrologyMsg.crc16 & 0xFF;
    *pbuffer += 1;
  }
  else if(type == 3)
  {
    memcpy(&buffer[*pbuffer],g_HydrologyMsg.upbody, 15);
    *pbuffer += 15;
    for(i = 0;i < downpbody->count;i++)
    {
      memcpy(&buffer[*pbuffer],(downpbody->element)[i].guide, 2);
      *pbuffer += 2;
      memcpy(&buffer[*pbuffer],(downpbody->element)[i].value,  (downpbody->element)[i].num);
      *pbuffer += (downpbody->element)[i].num;
    }
    buffer[*pbuffer] = g_HydrologyMsg.end;
    *pbuffer += 1;
    bodylen = *pbuffer - 15;
    pheader->dir_len[0] |= bodylen>>8;
    pheader->dir_len[1] |= bodylen&0xFF;
    memcpy(&buffer[11],pheader->dir_len, 2);
    
    g_HydrologyMsg.crc16 = hydrologyCRC16(buffer,*pbuffer);
    buffer[*pbuffer] = g_HydrologyMsg.crc16 >> 8;
    *pbuffer += 1;
    buffer[*pbuffer] = g_HydrologyMsg.crc16 & 0xFF;
    *pbuffer += 1;
  }
}
  

static void hydrologyInitSend(void)
{
   memset(&g_HydrologyUpHeader, 0, sizeof(hydrologyHeader));
   
   g_HydrologyMsg.header = (void*)&g_HydrologyUpHeader;

   g_HydrologyMsg.upbody = (void*)malloc(sizeof(hydrologyUpBody));

   hydrologyUpBody* uppbody = (hydrologyUpBody*) (g_HydrologyMsg.upbody);
   uppbody->count = 0;
   
   if (g_HydrologyMsg.upbody == 0)
      HydrologyMsg_printf("g_HydrologyMsg.upbody malloc failed");
   
   for(int i = 0; i < (sizeof(Element_table) / 5 - 1); ++i)
   {
      switch(Element_table[i].Mode)
      {
        case HYDROLOGY_ADC:
        {
          ADCElementCount++;
          break;
        }
        case HYDROLOGY_ISR_COUNT:
        {
          ISR_COUNTElementCount++;
          break;
        }
        case HYDROLOGY_IO_STATUS:
        {
          IO_STATUSElementCount++;
          break;
        }
        case HYDROLOGY_RS485:
        {
          RS485ElementCount++;
          break;
        }
      }
   }
}

static void hydrologyExitSend(void)
{
  int i = 0;
  hydrologyUpBody* uppbody = (hydrologyUpBody*) (g_HydrologyMsg.upbody);

  for ( i = 0; i < uppbody->count;i++)
  {
    if((uppbody->element)[i].value != NULL)
    {
      free((uppbody->element)[i].value);
      (uppbody->element)[i].value = NULL;
    }
    if(inputPara[i].value != NULL)
    {
      free(inputPara[i].value);
      inputPara[i].value = NULL;
    }
  }
  free(uppbody);
}

int hydrologyProcessSend(char funcode)
{
  int elecount = 0;
  int bufferlen = 0;
  char buffer[300];

  memset(buffer, 0, sizeof(buffer));

  hydrologyInitSend();

  Hydrology_CalElementInfo(&elecount, funcode);

  hydrologyMakeUpHeader(funcode);

  hydrologyMakeUpBodyBasicInfo(funcode);

  hydrologyMakeUpBody(elecount);

  hydrologyMakeUpTail(buffer,&bufferlen, funcode);

  Hydrology_SendData(buffer, bufferlen);

  hydrologyExitSend();

  return 0;
}

static int hydrologyNeedRespond(char funcode)
{
  int trueorfalse = 0;
  switch(funcode)
  {
    case LinkMaintenance:
    {
      trueorfalse = 0;
      break;
    }
    case Test:
    {
      trueorfalse = 0;
      break;
    }
    case EvenPeriodInformation:
    {
      trueorfalse = 1;
      break;
    }
    case TimerReport:
    {
      trueorfalse = 0;
      break;
    }
    case AddReport:
    {
      trueorfalse = 1;
      break;
    }
    case Hour:
    {
      trueorfalse = 0;
      break;
    }
    case ArtificialNumber:
    {
      trueorfalse = 0;
      break;
    }
    case Picture:
    {
      trueorfalse = 1;
      break;
    }
    case Realtime:
    {
      trueorfalse = 1;
      break;
    }
    case Period:
    {
      trueorfalse = 1;
      break;
    }
    case InquireArtificialNumber:
    {
      trueorfalse = 1;
      break;
    }
    case SpecifiedElement:
    {
      trueorfalse = 1;
      break;
    }
    case ConfigurationModification:
    {
      trueorfalse = 1;
      break;
    }
    case ConfigurationRead:
    {
      trueorfalse = 1;
      break;
    }
    case ParameterModification:
    {
      trueorfalse = 1;
      break;
    }
    case ParameterRead:
    {
      trueorfalse = 1;
      break;
    }
    case WaterPumpMotor:
    {
      trueorfalse = 1;
      break;
    }
    case SoftwareVersion:
    {
      trueorfalse = 1;
      break;
    }
    case Status:
    {
      trueorfalse = 1;
      break;
    }
    case InitializeSolidStorage:
    {
      trueorfalse = 1;
      break;
    }
    case Reset:
    {
      trueorfalse = 1;
      break;
    }        
    case ChangePassword:
    {
      trueorfalse = 1;
      break;
    }        
    case SetClock:
    {
      trueorfalse = 1;
      break;
    }
    case SetICCard:
    {
      trueorfalse = 1;
      break;
    }
    case Pump:
    {
      trueorfalse = 1;
      break;
    }
    case Valve:
    {
      trueorfalse = 1;
      break;
    }        
    case Gate:
    {
      trueorfalse = 1;
      break;
    }
    case WaterSetting:
    {
      trueorfalse = 1;
      break;
    }
    case Record:
    {
      trueorfalse = 1;
      break;
    }       
    case Time:
    {
      trueorfalse = 1;
      break;
    }
    default:
      break;
  }    
  return trueorfalse;
}

static void hydrologyQueryRespond(char funcode)
{
  int type = 0;
  int trueorfalse = 0;

  trueorfalse = hydrologyNeedRespond(funcode);
  
  if(trueorfalse)
  {
    type = hydrologyJudgeType(funcode);
    if(type == 1)
    {
      hydrologyProcessSend(funcode);
    }
    else if(type == 2)
    {
      hydrologyProcessSend(funcode);
    }
    else if(type == 3)
    {
      hydrologyProcessSend(funcode);
    }
  }
  
}

static void hydrologyInitReceieve()
{
   memset(&g_HydrologyDownHeader, 0, sizeof(hydrologyHeader));

   g_HydrologyMsg.downbody = (void*)malloc(sizeof(hydrologyDownBody));

   hydrologyDownBody* downpbody = (hydrologyDownBody*) (g_HydrologyMsg.downbody);
   downpbody->count = 0;
   
   if (0 == g_HydrologyMsg.downbody)
      HydrologyMsg_printf("g_HydrologyMsg.downbody malloc failed");
}

static void hydrologyExitReceieve()
{
  int i = 0;

  hydrologyDownBody* downpbody = (hydrologyDownBody*) (g_HydrologyMsg.downbody);
  for ( i = 0; i < downpbody->count;i++)
  {
    if((downpbody->element)[i].value != NULL)
    {
      free((downpbody->element)[i].value);
      (downpbody->element)[i].value = NULL;
    }
  }
  free(downpbody);
}

int hydrologyProcessReceieve(char* input, int inputlen)
{
  int i = 0, bodylen = 0;
    
  hydrologyInitReceieve();
  
  if(hydrologyMakeDownHeader(input, inputlen,&i,&bodylen) != 0)
    return -1;
  
  hydrologyMakeDownBody(input, bodylen, i);
  
  hydrologyCommand(input[10]);
  
  hydrologyQueryRespond(input[10]);
  
  hydrologyCommand(SetClock);

  hydrologyExitReceieve();
  
  return 0;
}
  
  
  

