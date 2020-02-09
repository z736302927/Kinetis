#include "ds3231/ds3231.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "iic_soft/iic_soft.h"

#define DEBUG
#include "idebug/idebug.h"

#define ds3231_printf                   p_dbg

#define DS3231_ADDR                     0x00

void ds3231_Delayus(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void ds3231_Delayms(uint32_t ticks)
{
  HAL_Delay(ticks);
}

void ds3231_PortTransmmit(uint8_t Addr, uint8_t Data)
{
  IIC_Soft_WriteSingleByteWithAddr(DS3231_ADDR, Addr, Data);
}

void ds3231_PortReceive(uint8_t Addr, uint8_t *pData)
{
  IIC_Soft_ReadSingleByteWithAddr(DS3231_ADDR, Addr, pData);
}

void ds3231_PortMultiTransmmit(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_WriteMultiByteWithAddr(DS3231_ADDR, Addr, pData, Length);
}

void ds3231_PortMultiReceive(uint8_t Addr, uint8_t *pData, uint32_t Length)
{
  IIC_Soft_ReadMultiByteWithAddr(DS3231_ADDR, Addr, pData, Length);
}
/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#define HOURS24                         0x00
#define HOURS12                         0x01
#define AM                              0x00
#define PM                              0x01
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

uint8_t g_TimeMode = HOURS24;
uint8_t g_TimeRegion = AM;

void ds3231_ReadTimeWithDec(uint8_t *pData)
{
  uint8_t Tmp[7];
  uint8_t Hour10 = 0;
  
  ds3231_PortMultiReceive(SECONDS, Tmp, 7);
  
  if(Tmp[2] & 0x40)
  {
    g_TimeMode = HOURS12;
    Hour10 = (Tmp[2] & 0x10) >> 4;
    if(Tmp[2] & 0x20)
    {
      g_TimeRegion = PM;
    }
    else
    {
      g_TimeRegion = AM;
    }
  }
  else
  {
    g_TimeMode = HOURS24;
    Hour10 = (Tmp[2] & 0x30) >> 4;
  }
  
  pData[0] = (Tmp[0] >> 4) * 10 + (Tmp[0] & 0x0F);
  pData[1] = (Tmp[1] >> 4) * 10 + (Tmp[1] & 0x0F);
  pData[2] = Hour10 * 10 + (Tmp[2] & 0x0F);
  pData[3] = (Tmp[4] >> 4) * 10 + (Tmp[4] & 0x0F);
  pData[4] = ((Tmp[5] & 0x1F) >> 4) * 10 + (Tmp[5] & 0x0F);
  pData[5] = (Tmp[6] >> 4) * 10 + (Tmp[6] & 0x0F);
}

void ds3231_ReadTimeWithString(char *pData)
{
  uint8_t Tmp[6];
  
  ds3231_ReadTimeWithDec(Tmp);
  pData[0] = (Tmp[0] % 10) + '0';
  pData[1] = (Tmp[0] / 10) + '0';
  pData[2] = (Tmp[1] % 10) + '0';
  pData[3] = (Tmp[1] / 10) + '0';
  pData[4] = (Tmp[2] % 10) + '0';
  pData[5] = (Tmp[2] / 10) + '0';
  pData[6] = (Tmp[3] % 10) + '0';
  pData[7] = (Tmp[3] / 10) + '0';
  pData[8] = (Tmp[4] % 10) + '0';
  pData[9] = (Tmp[4] / 10) + '0';
  pData[10] = (Tmp[5] % 10) + '0';
  pData[11] = (Tmp[5] / 10) + '0';
}

void ds3231_ReadWeek(uint8_t *pData)
{
  ds3231_PortReceive(DAY, pData);
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
  if(g_TimeMode == HOURS24)
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
  if(g_TimeMode == HOURS24)
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

static uint8_t Tx_Buffer[256];
static uint8_t Rx_Buffer[256];

void ds3231_Test(void)
{
  uint32_t TmpRngdata = 0;
  uint16_t BufferLength = 0;
  uint32_t TestAddr = 0;
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  BufferLength = TmpRngdata & 0xFF;
  ds3231_printf("BufferLength = %d.", BufferLength);
  
  if(Tx_Buffer == NULL || Rx_Buffer == NULL)
  {
    ds3231_printf("Failed to allocate memory !");
    return;
  }
  memset(Tx_Buffer, 0, BufferLength);
  memset(Rx_Buffer, 0, BufferLength);
  
  HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
  TestAddr = TmpRngdata & 0xFF;
  ds3231_printf("TestAddr = 0x%02X.", TestAddr);

  for(uint16_t i = 0;i < BufferLength;i += 4)
  {
    HAL_RNG_GenerateRandomNumber(&hrng, &TmpRngdata);
    Tx_Buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
    Tx_Buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
    Tx_Buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
    Tx_Buffer[i + 0] = (TmpRngdata & 0x000000FF);
  }
  
  ds3231_WriteData(TestAddr, Tx_Buffer, BufferLength);
  ds3231_ReadData(TestAddr, Rx_Buffer, BufferLength);
  
  for(uint16_t i = 0;i < BufferLength;i++)
  {
    if(Tx_Buffer[i] != Rx_Buffer[i])
    {
      ds3231_printf("Tx_Buffer[%d] = 0x%02X, Rx_Buffer[%d] = 0x%02X", 
                     i, Tx_Buffer[i],
                     i, Rx_Buffer[i]);
      ds3231_printf("Data writes and reads do not match, TEST FAILED !");
      return ;
    }
  }
  
  ds3231_printf("ds3231 Read and write TEST PASSED !");
}

#endif