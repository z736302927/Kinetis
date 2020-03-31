#include "chip/ds3231.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "peripheral/iic_soft.h"
#include "i2c.h"
#include "string.h"

#define DEBUG
#include "idebug.h"

#define ds3231_printf                   p_dbg

#define DS3231_ADDR                     0x68

static inline void ds3231_Delayus(uint32_t ticks)
{
  HAL_Delay(ticks);
}

static inline void ds3231_Delayms(uint32_t ticks)
{
  HAL_Delay(ticks);
}

static inline void ds3231_PortTransmmit(uint8_t Addr, uint8_t Data)
{
#ifdef USING_I2C_SOFT
  IIC_Soft_WriteSingleByteWithAddr(DS3231_ADDR, Addr, Data);
#else
  HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(DS3231_ADDR << 1),
                            (uint16_t)Addr, I2C_MEMADD_SIZE_8BIT, &Data, 1, 10000);
#endif
}

static inline void ds3231_PortReceive(uint8_t Addr, uint8_t *pData)
{
#ifdef USING_I2C_SOFT
  IIC_Soft_ReadSingleByteWithAddr(DS3231_ADDR, Addr, pData);
#else
  HAL_I2C_Mem_Read (&hi2c1, (uint16_t)(DS3231_ADDR << 1),
                            (uint16_t)Addr, I2C_MEMADD_SIZE_8BIT, pData, 1, 10000);
#endif
}

static inline void ds3231_PortMultiTransmmit(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
#ifdef USING_I2C_SOFT
  IIC_Soft_WriteMultiByteWithAddr(DS3231_ADDR, Addr, pData, Length);
#else
  HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(DS3231_ADDR << 1),
                            (uint16_t)Addr, I2C_MEMADD_SIZE_8BIT, pData, Length, 10000);
#endif
}

static inline void ds3231_PortMultiReceive(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
#ifdef USING_I2C_SOFT
  IIC_Soft_ReadMultiByteWithAddr(DS3231_ADDR, Addr, pData, Length);
#else
  HAL_I2C_Mem_Read (&hi2c1, (uint16_t)(DS3231_ADDR << 1),
                            (uint16_t)Addr, I2C_MEMADD_SIZE_8BIT, pData, Length, 10000);
#endif
}
/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define ALARM_MASK                      0x80
#define ALARM_MASK_1                    0x01
#define ALARM_MASK_2                    0x02
#define ALARM_MASK_3                    0x04
#define ALARM_MASK_4                    0x08
#define TIMEMODE_MASK                   0x40
#define WEEK_CYCLE                      0x00
#define MONTH_CYCLE                     0x01
#define DY_DT_MASK                      0x40
#define SQUARE_WAVE_1HZ                 0x00
#define SQUARE_WAVE_1_024HZ             0x01
#define SQUARE_WAVE_4_096HZ             0x02
#define SQUARE_WAVE_8_192HZ             0x03
#define SECONDS                         0x00
#define MINUTES                         0x01
#define HOURS                           0x02
#define DAY                             0x03
#define DATE                            0x04
#define MONTH_CENTURY                   0x05
#define YEAR                            0x06
#define ALARM_1_SECONDS                 0x07
#define ALARM_1_MINUTES                 0x08
#define ALARM_1_HOURS                   0x09
#define ALARM_1_DAY_DATE                0x0A
#define ALARM_2_MINUTES                 0x0B
#define ALARM_2_HOURS                   0x0C
#define ALARM_2_DAY_DATE                0x0D
#define CONTROL                         0x0E
#define CONTROL_STATUS                  0x0F
#define AGING_OFFSET                    0x10
#define MSB_OF_TEMP                     0x11
#define LSB_OF_TEMP                     0x12

volatile uint8_t g_TimeMode = DS3231_HOURS24;
volatile uint8_t g_TimeRegion = DS3231_AM;

uint8_t ds3231_GetTimeMode(void)
{
  return g_TimeMode;
}

void ds3231_SetTimeMode(uint8_t Data)
{
  g_TimeMode = Data;
}

uint8_t ds3231_GetTimeRegion(void)
{
  return g_TimeRegion;
}

void ds3231_SetTimeRegion(uint8_t Data)
{
  g_TimeRegion = Data;
}

void ds3231_ReadTimeWithDec(uint8_t *pData)
{
  uint8_t Tmp[7];
  uint8_t Hour10 = 0;
  
  ds3231_PortMultiReceive(SECONDS, Tmp, 7);
  
  if(Tmp[2] & 0x40)
  {
    g_TimeMode = DS3231_HOURS12;
    Hour10 = (Tmp[2] & 0x10) >> 4;
    if(Tmp[2] & 0x20)
    {
      g_TimeRegion = DS3231_PM;
    }
    else
    {
      g_TimeRegion = DS3231_AM;
    }
  }
  else
  {
    g_TimeMode = DS3231_HOURS24;
    Hour10 = (Tmp[2] & 0x30) >> 4;
  }
  
  pData[0] = (Tmp[0] >> 4) * 10 + (Tmp[0] & 0x0F);
  pData[1] = (Tmp[1] >> 4) * 10 + (Tmp[1] & 0x0F);
  pData[2] = Hour10 * 10 + (Tmp[2] & 0x0F);
  pData[3] = (Tmp[4] >> 4) * 10 + (Tmp[4] & 0x0F);
  pData[4] = ((Tmp[5] & 0x1F) >> 4) * 10 + (Tmp[5] & 0x0F);
  pData[5] = (Tmp[6] >> 4) * 10 + (Tmp[6] & 0x0F);
}

void ds3231_SetTimeWithDec(uint8_t *pData)
{
  uint8_t Tmp[7] = {0,0,0,0,0,0,0};
  
  ds3231_PortMultiTransmmit(SECONDS, Tmp, 3);
  ds3231_PortMultiTransmmit(DATE, Tmp, 3);
  
  Tmp[6] |= (pData[0] / 10) << 4;
  Tmp[6] |= (pData[0] % 10) << 0;
  Tmp[5] |= (pData[1] / 10) << 4;
  Tmp[5] |= (pData[1] % 10) << 0;
  Tmp[4] |= (pData[2] / 10) << 4;
  Tmp[4] |= (pData[2] % 10) << 0;
  ds3231_PortMultiTransmmit(DATE, &Tmp[4], 3);
  
  Tmp[2] |= (pData[3] / 10) << 4;
  Tmp[2] |= (pData[3] % 10) << 0;
  if(g_TimeMode == DS3231_HOURS12)
  {
    if(g_TimeRegion == DS3231_PM)
    {
      Tmp[2] |= 0x20;
    }
    else
    {
      Tmp[2] &= ~0x20;
    }
    Tmp[2] |= 0x40;
  }
  else
  {
    Tmp[2] &= ~0x40;
  }
  Tmp[1] |= (pData[4] / 10) << 4;
  Tmp[1] |= (pData[4] % 10) << 0;
  Tmp[0] |= (pData[5] / 10) << 4;
  Tmp[0] |= (pData[5] % 10) << 0;
  ds3231_PortMultiTransmmit(SECONDS, Tmp, 3);
}

void ds3231_ReadTimeWithString(char *pData)
{
  uint8_t Tmp[6];
  
  ds3231_ReadTimeWithDec(Tmp);
  pData[11] = (Tmp[0] % 10) + '0';
  pData[10] = (Tmp[0] / 10) + '0';
  pData[9] = (Tmp[1] % 10) + '0';
  pData[8] = (Tmp[1] / 10) + '0';
  pData[7] = (Tmp[2] % 10) + '0';
  pData[6] = (Tmp[2] / 10) + '0';
  pData[5] = (Tmp[3] % 10) + '0';
  pData[4] = (Tmp[3] / 10) + '0';
  pData[3] = (Tmp[4] % 10) + '0';
  pData[2] = (Tmp[4] / 10) + '0';
  pData[1] = (Tmp[5] % 10) + '0';
  pData[0] = (Tmp[5] / 10) + '0';
}

void ds3231_SetTimeWithString(char *pData)
{
  uint8_t Tmp[6];
  
  Tmp[0] = (pData[0] - '0') * 10 + (pData[1] - '0');
  Tmp[1] = (pData[2] - '0') * 10 + (pData[3] - '0');
  Tmp[2] = (pData[4] - '0') * 10 + (pData[5] - '0');
  Tmp[3] = (pData[6] - '0') * 10 + (pData[7] - '0');
  Tmp[4] = (pData[8] - '0') * 10 + (pData[9] - '0');
  Tmp[5] = (pData[10] - '0') * 10 + (pData[11] - '0');
  ds3231_SetTimeWithDec(Tmp);
}

void ds3231_ReadWeek(uint8_t *pData)
{
  ds3231_PortReceive(DAY, pData);
}

void ds3231_SetWeek(uint8_t Data)
{
  ds3231_PortTransmmit(DAY, Data);
}

void ds3231_SetAlarm1(uint8_t *pData, uint8_t DateorDay, uint8_t Data)
{
  uint8_t Tmp[4] = {0,0,0,0};
  uint8_t Tens = 0,Unit = 0;

  if(Data & ALARM_MASK_1)
  {
    Tmp[0] |= ALARM_MASK;
  }
  if(Data & ALARM_MASK_2)
  {
    Tmp[1] |= ALARM_MASK;
  }
  if(Data & ALARM_MASK_3)
  {
    Tmp[2] |= ALARM_MASK;
  }
  if(Data & ALARM_MASK_4)
  {
    Tmp[3] |= ALARM_MASK;
  }
  
  Unit = (pData[0] & 0x7F) % 10;
  Tens = (pData[0] & 0x7F) / 10;
  Tmp[0] = (Tens << 4) | Unit;
  Unit = (pData[1] & 0x7F) % 10;
  Tens = (pData[1] & 0x7F) / 10;
  Tmp[1] = (Tens << 4) | Unit;
  if(g_TimeMode == DS3231_HOURS24)
  {
    Unit = (pData[2] & 0x3F) % 10;
    Tens = (pData[2] & 0x3F) / 10;
    Tmp[2] = (Tens << 4) | Unit;
    Tmp[2] &= ~TIMEMODE_MASK;
  }
  else
  {
    if(pData[2] > 12)
    {
      pData[2] -= 12;
    }
    Unit = (pData[2] & 0x1F) % 10;
    Tens = (pData[2] & 0x1F) / 10;
    Tmp[2] = (Tens << 4) | Unit;
    Tmp[2] |= TIMEMODE_MASK;
  }
  if(DateorDay == WEEK_CYCLE)
  {
    Unit = (pData[3] & 0x0F) % 10;
    Tmp[3] = Unit;
    Tmp[3] |= DY_DT_MASK;
  }
  else if(DateorDay == MONTH_CYCLE)
  {
    Unit = (pData[3] & 0x0F) % 10;
    Tens = (pData[3] & 0x0F) / 10;
    Tmp[3] = (Tens << 4) | Unit;
    Tmp[3] &= ~DY_DT_MASK;
  }
  
  ds3231_PortMultiReceive(ALARM_1_SECONDS, Tmp, 4);
}

void ds3231_SetAlarm2(uint8_t *pData, uint8_t DateorDay, uint8_t Data)
{
  uint8_t Tmp[3] = {0,0,0};
  uint8_t Tens = 0,Unit = 0;
  
  if(Data & ALARM_MASK_2)
  {
    Tmp[0] |= ALARM_MASK;
  }
  if(Data & ALARM_MASK_3)
  {
    Tmp[1] |= ALARM_MASK;
  }
  if(Data & ALARM_MASK_4)
  {
    Tmp[2] |= ALARM_MASK;
  }
  
  Unit = (pData[0] & 0x7F) % 10;
  Tens = (pData[0] & 0x7F) / 10;
  Tmp[0] = (Tens << 4) | Unit;
  if(g_TimeMode == DS3231_HOURS24)
  {
    Unit = (pData[1] & 0x3F) % 10;
    Tens = (pData[1] & 0x3F) / 10;
    Tmp[1] = (Tens << 4) | Unit;
    Tmp[1] &= ~TIMEMODE_MASK;
  }
  else
  {
    if(pData[1] > 12)
    {
      pData[1] -= 12;
    }
    Unit = (pData[1] & 0x1F) % 10;
    Tens = (pData[1] & 0x1F) / 10;
    Tmp[1] = (Tens << 4) | Unit;
    Tmp[1] |= TIMEMODE_MASK;
  }
  if(DateorDay == WEEK_CYCLE)
  {
    Unit = (pData[2] & 0x0F) % 10;
    Tmp[2] = Unit;
    Tmp[2] |= DY_DT_MASK;
  }
  else if(DateorDay == MONTH_CYCLE)
  {
    Unit = (pData[2] & 0x0F) % 10;
    Tens = (pData[2] & 0x0F) / 10;
    Tmp[2] = (Tens << 4) | Unit;
    Tmp[2] &= ~DY_DT_MASK;
  }
  
  ds3231_PortMultiReceive(ALARM_2_MINUTES, Tmp, 3);
}

void ds3231_EnableOscillator(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL, &TmpReg);
  
  TmpReg &= ~(0x01 << 7);
  TmpReg |= (0x00 << 7);
  
  ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_ConvertTemperature(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL, &TmpReg);
  
  TmpReg &= ~(0x01 << 6);
  TmpReg |= (0x01 << 6);
  
  ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_RateSelect(uint8_t Data)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL, &TmpReg);

  switch(Data)
  {
    case SQUARE_WAVE_1HZ:
      TmpReg &= ~(0x00 << 3);
      TmpReg |= (0x00 << 3);
      break;
      
    case SQUARE_WAVE_1_024HZ:
      TmpReg &= ~(0x01 << 3);
      TmpReg |= (0x01 << 3);
      break;
      
    case SQUARE_WAVE_4_096HZ:
      TmpReg &= ~(0x20 << 3);
      TmpReg |= (0x20 << 3);
      break;
      
    case SQUARE_WAVE_8_192HZ:
      TmpReg &= ~(0x03 << 3);
      TmpReg |= (0x03 << 3);
      break;
      
    default:break;
  }
  
  ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_EnableSquareWaveWithBAT(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL, &TmpReg);
  
  TmpReg &= ~(0x01 << 6);
  TmpReg |= (0x01 << 6);
  
  ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_EnableInt(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL, &TmpReg);
  
  TmpReg &= ~(0x01 << 2);
  TmpReg |= (0x01 << 2);
  
  ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_EnableSquareWave(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL, &TmpReg);
  
  TmpReg &= ~(0x01 << 2);
  
  ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_EnableAlarm2Int(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL, &TmpReg);
  
  TmpReg &= ~(0x01 << 1);
  TmpReg |= (0x01 << 1);
  
  ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_EnableAlarm1Int(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL, &TmpReg);
  
  TmpReg &= ~(0x01 << 0);
  TmpReg |= (0x01 << 0);
  
  ds3231_PortTransmmit(CONTROL, TmpReg);
}

uint8_t ds3231_OscillatorStopFlag(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL_STATUS, &TmpReg);
  
  if(TmpReg & 0x80)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void ds3231_Enable32kHzOutput(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL, &TmpReg);
  
  TmpReg &= ~(0x01 << 3);
  TmpReg |= (0x01 << 3);
  
  ds3231_PortTransmmit(CONTROL, TmpReg);
}

uint8_t ds3231_WaitBusy(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL_STATUS, &TmpReg);
  
  if(TmpReg & 0x02)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

uint8_t ds3231_Alarm2Flag(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL_STATUS, &TmpReg);
  
  if(TmpReg & 0x02)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void ds3231_ClearAlarm2Flag(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL, &TmpReg);
  
  TmpReg &= ~(0x01 << 1);
  
  ds3231_PortTransmmit(CONTROL, TmpReg);
}

uint8_t ds3231_Alarm1Flag(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL_STATUS, &TmpReg);
  
  if(TmpReg & 0x01)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void ds3231_ClearAlarm1Flag(void)
{
  uint8_t TmpReg = 0;
  
  ds3231_PortReceive(CONTROL, &TmpReg);
  
  TmpReg &= ~(0x01 << 0);
  
  ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_AgingOffset(uint8_t *pData)
{
  ds3231_PortReceive(AGING_OFFSET, pData);
}

void ds3231_GetTemperature(float *pData)
{
  uint8_t TmpVal[2];
  
  ds3231_PortMultiReceive(MSB_OF_TEMP, TmpVal, 2);
  pData[0] = (float)TmpVal[0] + (float)(TmpVal[1] >> 6) * 0.25;
}


#if 0

static uint8_t Tx_Buffer[20];
static uint8_t Rx_Buffer[20];

void ds3231_Test(void)
{
  char Time[16];
  
  ds3231_SetTimeMode(DS3231_HOURS24);
  ds3231_SetTimeRegion(PM);
  ds3231_SetTimeWithString("200308202020");
  ds3231_SetWeek(7);
  
  while(1)
  {
    ds3231_ReadTimeWithString(Time);
    
    if(g_TimeRegion == DS3231_AM)
    {
      snprintf(&Time[12], 4, " DS3231_AM");
    }
    else
    {
      snprintf(&Time[12], 4, " PM");
    }
    ds3231_printf("%s", Time);
    ds3231_Delayms(1000);
  }
}

#endif