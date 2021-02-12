#include "kinetis/ds3231.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "kinetis/iic_soft.h"
#include "i2c.h"
#include "string.h"
#include <linux/delay.h>
#include "kinetis/idebug.h"

#define DS3231_ADDR                     0x68

static inline void ds3231_PortTransmmit(u8 Addr, u8 Data)
{
    iic_port_transmmit(IIC_1, DS3231_ADDR, Addr, Data);
}

static inline void ds3231_PortReceive(u8 Addr, u8 *pData)
{
    iic_port_transmmit(IIC_1, DS3231_ADDR, Addr, pData);
}

static inline void ds3231_port_multi_transmmit(u8 Addr, u8 *pData, u32 Length)
{
    iic_port_multi_transmmit(IIC_1, DS3231_ADDR, Addr, pData, Length);
}

static inline void ds3231_port_multi_receive(u8 Addr, u8 *pData, u32 Length)
{
    iic_port_multi_receive(IIC_1, DS3231_ADDR, Addr, pData, Length);
}
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

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

volatile u8 g_TimeMode = DS3231_HOURS24;
volatile u8 g_TimeRegion = DS3231_AM;

u8 ds3231_GetTimeMode(void)
{
    return g_TimeMode;
}

void ds3231_SetTimeMode(u8 Data)
{
    g_TimeMode = Data;
}

u8 ds3231_GetTimeRegion(void)
{
    return g_TimeRegion;
}

void ds3231_SetTimeRegion(u8 Data)
{
    g_TimeRegion = Data;
}

void ds3231_ReadTime(u8 *pData, u8 Format)
{
    u8 Tmp[7];
    u8 Hour10 = 0;

    ds3231_port_multi_receive(SECONDS, Tmp, 7);

    if (Tmp[2] & 0x40) {
        g_TimeMode = DS3231_HOURS12;
        Hour10 = (Tmp[2] & 0x10) >> 4;

        if (Tmp[2] & 0x20)
            g_TimeRegion = DS3231_PM;
        else
            g_TimeRegion = DS3231_AM;
    } else {
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

void ds3231_SetTime(u8 *pData, u8 Format)
{
    u8 Tmp[7] = {0, 0, 0, 0, 0, 0, 0};

    ds3231_port_multi_transmmit(SECONDS, Tmp, 3);
    ds3231_port_multi_transmmit(DATE, Tmp, 3);

    Tmp[6] |= (pData[0] / 10) << 4;
    Tmp[6] |= (pData[0] % 10) << 0;
    Tmp[5] |= (pData[1] / 10) << 4;
    Tmp[5] |= (pData[1] % 10) << 0;
    Tmp[4] |= (pData[2] / 10) << 4;
    Tmp[4] |= (pData[2] % 10) << 0;
    ds3231_port_multi_transmmit(DATE, &Tmp[4], 3);

    Tmp[2] |= (pData[3] / 10) << 4;
    Tmp[2] |= (pData[3] % 10) << 0;

    if (g_TimeMode == DS3231_HOURS12) {
        if (g_TimeRegion == DS3231_PM)
            Tmp[2] |= 0x20;
        else
            Tmp[2] &= ~0x20;

        Tmp[2] |= 0x40;
    } else
        Tmp[2] &= ~0x40;

    Tmp[1] |= (pData[4] / 10) << 4;
    Tmp[1] |= (pData[4] % 10) << 0;
    Tmp[0] |= (pData[5] / 10) << 4;
    Tmp[0] |= (pData[5] % 10) << 0;
    ds3231_port_multi_transmmit(SECONDS, Tmp, 3);
}

void ds3231_ReadTimeWithString(char *pData)
{
    u8 Tmp[6];

    ds3231_ReadTime(Tmp, DS3231_FORMAT_BIN);
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
    u8 Tmp[6];

    Tmp[0] = (pData[0] - '0') * 10 + (pData[1] - '0');
    Tmp[1] = (pData[2] - '0') * 10 + (pData[3] - '0');
    Tmp[2] = (pData[4] - '0') * 10 + (pData[5] - '0');
    Tmp[3] = (pData[6] - '0') * 10 + (pData[7] - '0');
    Tmp[4] = (pData[8] - '0') * 10 + (pData[9] - '0');
    Tmp[5] = (pData[10] - '0') * 10 + (pData[11] - '0');
    ds3231_SetTime(Tmp, DS3231_FORMAT_BIN);
}

void ds3231_ReadWeek(u8 *pData)
{
    ds3231_PortReceive(DAY, pData);
}

void ds3231_SetWeek(u8 Data)
{
    ds3231_PortTransmmit(DAY, Data);
}

void ds3231_Alarm1Callback(void)
{
    ;
}

void ds3231_SetAlarm1(u8 *pData, u8 DateorDay, u8 Data)
{
    u8 Tmp[4] = {0, 0, 0, 0};
    u8 Tens = 0, Unit = 0;

    if (Data & ALARM_MASK_1)
        Tmp[0] |= ALARM_MASK;

    if (Data & ALARM_MASK_2)
        Tmp[1] |= ALARM_MASK;

    if (Data & ALARM_MASK_3)
        Tmp[2] |= ALARM_MASK;

    if (Data & ALARM_MASK_4)
        Tmp[3] |= ALARM_MASK;

    Unit = (pData[0] & 0x7F) % 10;
    Tens = (pData[0] & 0x7F) / 10;
    Tmp[0] = (Tens << 4) | Unit;
    Unit = (pData[1] & 0x7F) % 10;
    Tens = (pData[1] & 0x7F) / 10;
    Tmp[1] = (Tens << 4) | Unit;

    if (g_TimeMode == DS3231_HOURS24) {
        Unit = (pData[2] & 0x3F) % 10;
        Tens = (pData[2] & 0x3F) / 10;
        Tmp[2] = (Tens << 4) | Unit;
        Tmp[2] &= ~TIMEMODE_MASK;
    } else {
        if (pData[2] > 12)
            pData[2] -= 12;

        Unit = (pData[2] & 0x1F) % 10;
        Tens = (pData[2] & 0x1F) / 10;
        Tmp[2] = (Tens << 4) | Unit;
        Tmp[2] |= TIMEMODE_MASK;
    }

    if (DateorDay == WEEK_CYCLE) {
        Unit = (pData[3] & 0x0F) % 10;
        Tmp[3] = Unit;
        Tmp[3] |= DY_DT_MASK;
    } else if (DateorDay == MONTH_CYCLE) {
        Unit = (pData[3] & 0x0F) % 10;
        Tens = (pData[3] & 0x0F) / 10;
        Tmp[3] = (Tens << 4) | Unit;
        Tmp[3] &= ~DY_DT_MASK;
    }

    ds3231_port_multi_receive(ALARM_1_SECONDS, Tmp, 4);
}

void ds3231_Alarm2Callback(void)
{
    ;
}

void ds3231_SetAlarm2(u8 *pData, u8 DateorDay, u8 Data)
{
    u8 Tmp[3] = {0, 0, 0};
    u8 Tens = 0, Unit = 0;

    if (Data & ALARM_MASK_2)
        Tmp[0] |= ALARM_MASK;

    if (Data & ALARM_MASK_3)
        Tmp[1] |= ALARM_MASK;

    if (Data & ALARM_MASK_4)
        Tmp[2] |= ALARM_MASK;

    Unit = (pData[0] & 0x7F) % 10;
    Tens = (pData[0] & 0x7F) / 10;
    Tmp[0] = (Tens << 4) | Unit;

    if (g_TimeMode == DS3231_HOURS24) {
        Unit = (pData[1] & 0x3F) % 10;
        Tens = (pData[1] & 0x3F) / 10;
        Tmp[1] = (Tens << 4) | Unit;
        Tmp[1] &= ~TIMEMODE_MASK;
    } else {
        if (pData[1] > 12)
            pData[1] -= 12;

        Unit = (pData[1] & 0x1F) % 10;
        Tens = (pData[1] & 0x1F) / 10;
        Tmp[1] = (Tens << 4) | Unit;
        Tmp[1] |= TIMEMODE_MASK;
    }

    if (DateorDay == WEEK_CYCLE) {
        Unit = (pData[2] & 0x0F) % 10;
        Tmp[2] = Unit;
        Tmp[2] |= DY_DT_MASK;
    } else if (DateorDay == MONTH_CYCLE) {
        Unit = (pData[2] & 0x0F) % 10;
        Tens = (pData[2] & 0x0F) / 10;
        Tmp[2] = (Tens << 4) | Unit;
        Tmp[2] &= ~DY_DT_MASK;
    }

    ds3231_port_multi_receive(ALARM_2_MINUTES, Tmp, 3);
}

void ds3231_EnableOscillator(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL, &TmpReg);

    TmpReg &= ~(0x01 << 7);
    TmpReg |= (0x00 << 7);

    ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_ConvertTemperature(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL, &TmpReg);

    TmpReg &= ~(0x01 << 6);
    TmpReg |= (0x01 << 6);

    ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_RateSelect(u8 Data)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL, &TmpReg);

    switch (Data) {
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

        default:
            break;
    }

    ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_EnableSquareWaveWithBAT(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL, &TmpReg);

    TmpReg &= ~(0x01 << 6);
    TmpReg |= (0x01 << 6);

    ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_EnableInt(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL, &TmpReg);

    TmpReg &= ~(0x01 << 2);
    TmpReg |= (0x01 << 2);

    ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_EnableSquareWave(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL, &TmpReg);

    TmpReg &= ~(0x01 << 2);

    ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_EnableAlarm2Int(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL, &TmpReg);

    TmpReg &= ~(0x01 << 1);
    TmpReg |= (0x01 << 1);

    ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_EnableAlarm1Int(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL, &TmpReg);

    TmpReg &= ~(0x01 << 0);
    TmpReg |= (0x01 << 0);

    ds3231_PortTransmmit(CONTROL, TmpReg);
}

u8 ds3231_OscillatorStopFlag(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL_STATUS, &TmpReg);

    if (TmpReg & 0x80)
        return 1;
    else
        return 0;
}

void ds3231_Enable32kHzOutput(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL, &TmpReg);

    TmpReg &= ~(0x01 << 3);
    TmpReg |= (0x01 << 3);

    ds3231_PortTransmmit(CONTROL, TmpReg);
}

u8 ds3231_WaitBusy(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL_STATUS, &TmpReg);

    if (TmpReg & 0x02)
        return 1;
    else
        return 0;
}

u8 Alarm2Flag = 0;

u8 ds3231_Alarm2Flag(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL_STATUS, &TmpReg);

    if (TmpReg & 0x02)
        return 1;
    else
        return 0;
}

void ds3231_ClearAlarm2Flag(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL, &TmpReg);

    TmpReg &= ~(0x01 << 1);

    ds3231_PortTransmmit(CONTROL, TmpReg);
}

u8 Alarm1Flag = 0;

u8 ds3231_Alarm1Flag(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL_STATUS, &TmpReg);

    if (TmpReg & 0x01)
        return 1;
    else
        return 0;
}

void ds3231_ClearAlarm1Flag(void)
{
    u8 TmpReg = 0;

    ds3231_PortReceive(CONTROL, &TmpReg);

    TmpReg &= ~(0x01 << 0);

    ds3231_PortTransmmit(CONTROL, TmpReg);
}

void ds3231_AgingOffset(u8 *pData)
{
    ds3231_PortReceive(AGING_OFFSET, pData);
}

void ds3231_GetTemperature(float *pData)
{
    u8 TmpVal[2];

    ds3231_port_multi_receive(MSB_OF_TEMP, TmpVal, 2);
    pData[0] = (float)TmpVal[0] + (float)(TmpVal[1] >> 6) * 0.25;
}

#ifdef DESIGN_VERIFICATION_DS3231
#include "kinetis/test-kinetis.h"

int t_ds3231_SetClock(int argc, char **argv)
{
    char Time[16];

    ds3231_SetTimeMode(DS3231_HOURS24);
    ds3231_SetTimeRegion(DS3231_PM);
    ds3231_SetTimeWithString("200308202020");
    ds3231_SetWeek(7);

    ds3231_ReadTimeWithString(Time);

    if (strcmp("200308202020", Time) != 0)
        return false;

    if (g_TimeRegion == DS3231_AM)
        snprintf(&Time[12], 4, " DS3231_AM");
    else
        snprintf(&Time[12], 4, " PM");

    printk(KERN_DEBUG "%s", Time);

    return true;
}

int t_ds3231_GetClock(int argc, char **argv)
{
    char Time[16];
    u8 i = 0;
    u16 times = 1;

    if (argc > 1)
        times = strtoul(argv[1], &argv[1], 10);

    for (i = 0; i < times; i++) {
        ds3231_ReadTimeWithString(Time);

        if (g_TimeRegion == DS3231_AM)
            snprintf(&Time[12], 4, " DS3231_AM");
        else
            snprintf(&Time[12], 4, " PM");

        printk(KERN_DEBUG "%s", Time);
        ds3231_Delayms(1000);
    }

    return true;
}

int t_ds3231_SetAlarm1(int argc, char **argv)
{
    u8 ret = 0;
    u8 dy_dt = 0;
    u8 a1m4 = 0, a1m3 = 0, a1m2 = 0, a1m1 = 0;
    u8 Alarm_Rate = 0;
    u8 Data[4] = {0, 0, 0, 0};
    u8 Time[7] = {0, 0, 0, 0, 0, 0, 0};
    u32 Delta = 0;

    if (argc > 1)
        dy_dt = strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        a1m4 = strtoul(argv[2], &argv[2], 10);

    if (argc > 3)
        a1m3 = strtoul(argv[3], &argv[3], 10);

    if (argc > 4)
        a1m2 = strtoul(argv[4], &argv[4], 10);

    if (argc > 5)
        a1m1 = strtoul(argv[5], &argv[5], 10);

    Alarm_Rate = (a1m4 << 3) | (a1m3 << 2) | (a1m2 << 1) | a1m1;

    ds3231_EnableInt();
    ds3231_EnableAlarm1Int();

    switch (Alarm_Rate) {
        case 0x0F:
            ds3231_SetAlarm1(Data, 0,
                ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3 | ALARM_MASK_4);
            Delta = basic_timer_get_ss();
            Timeout_WaitMSDone(&Alarm1Flag, true, 2000);
            Delta = basic_timer_get_ss() - Delta;
            ds3231_ClearAlarm1Flag();

            if (Delta == 1)
                ret = true;
            else
                ret = false;

            break;

        case 0x0E:
            Data[0] = 0;
            ds3231_SetAlarm1(Data, 0,
                ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3);
            ds3231_ReadTime(Time, DS3231_FORMAT_BIN);
            ds3231_ClearAlarm1Flag();

            if (Time[6] == 0)
                ret = true;
            else
                ret = false;

            break;

        case 0x0C:
            Data[0] = 0;
            Data[1] = 0;
            ds3231_SetAlarm1(Data, 0,
                ALARM_MASK_1 | ALARM_MASK_2);
            ds3231_ReadTime(Time, DS3231_FORMAT_BIN);
            ds3231_ClearAlarm1Flag();

            if (Time[5] == 0 && Time[6] == 0)
                ret = true;
            else
                ret = false;

            break;

        case 0x08:
            Data[0] = 0;
            Data[1] = 0;
            Data[2] = 0;
            ds3231_SetAlarm1(Data, 0,
                ALARM_MASK_1);
            ds3231_ReadTime(Time, DS3231_FORMAT_BIN);
            ds3231_ClearAlarm1Flag();

            if (Time[4] == 0 && Time[5] == 0 && Time[6] == 0)
                ret = true;
            else
                ret = false;

            break;

        case 0x00:
            if (dy_dt) {
                Data[0] = 0;
                Data[1] = 0;
                Data[2] = 0;
                Data[3] = 0;
                ds3231_SetAlarm1(Data, 0,
                    0);
                ds3231_ReadTime(Time, DS3231_FORMAT_BIN);
                ds3231_ClearAlarm1Flag();

                if (Time[3] == 0 && Time[4] == 0 && Time[5] == 0 && Time[6] == 0)
                    ret = true;
                else
                    ret = false;
            } else {
                Data[0] = 0;
                Data[1] = 0;
                Data[2] = 0;
                Data[3] = 0;
                ds3231_SetAlarm1(Data, 0,
                    0);
                ds3231_ReadTime(Time, DS3231_FORMAT_BIN);
                ds3231_ClearAlarm1Flag();

                if (Time[3] == 0 && Time[4] == 0 && Time[5] == 0 && Time[6] == 0)
                    ret = true;
                else
                    ret = false;
            }

            break;
    }
}

int t_ds3231_SetAlarm2(int argc, char **argv)
{
    u8 ret = 0;
    u8 dy_dt = 0;
    u8 a1m4 = 0, a1m3 = 0, a1m2 = 0, a1m1 = 0;
    u8 Alarm_Rate = 0;
    u8 Data[4] = {0, 0, 0, 0};
    u8 Time[7] = {0, 0, 0, 0, 0, 0, 0};
    u32 Delta = 0;

    if (argc > 1)
        dy_dt = strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        a1m4 = strtoul(argv[2], &argv[2], 10);

    if (argc > 3)
        a1m3 = strtoul(argv[3], &argv[3], 10);

    if (argc > 4)
        a1m2 = strtoul(argv[4], &argv[4], 10);

    if (argc > 5)
        a1m1 = strtoul(argv[5], &argv[5], 10);

    Alarm_Rate = (a1m4 << 3) | (a1m3 << 2) | (a1m2 << 1) | a1m1;

    ds3231_EnableInt();
    ds3231_EnableAlarm2Int();

    switch (Alarm_Rate) {
        case 0x0F:
            ds3231_SetAlarm1(Data, 0,
                ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3 | ALARM_MASK_4);
            Delta = basic_timer_get_ss();
            Timeout_WaitMSDone(&Alarm1Flag, true, 2000);
            Delta = basic_timer_get_ss() - Delta;
            ds3231_ClearAlarm1Flag();

            if (Delta == 1)
                ret = true;
            else
                ret = false;

            break;

        case 0x0E:
            Data[0] = 0;
            ds3231_SetAlarm1(Data, 0,
                ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3);
            ds3231_ReadTime(Time, DS3231_FORMAT_BIN);
            ds3231_ClearAlarm1Flag();

            if (Time[6] == 0)
                ret = true;
            else
                ret = false;

            break;

        case 0x0C:
            Data[0] = 0;
            Data[1] = 0;
            ds3231_SetAlarm1(Data, 0,
                ALARM_MASK_1 | ALARM_MASK_2);
            ds3231_ReadTime(Time, DS3231_FORMAT_BIN);
            ds3231_ClearAlarm1Flag();

            if (Time[5] == 0 && Time[6] == 0)
                ret = true;
            else
                ret = false;

            break;

        case 0x08:
            Data[0] = 0;
            Data[1] = 0;
            Data[2] = 0;
            ds3231_SetAlarm1(Data, 0,
                ALARM_MASK_1);
            ds3231_ReadTime(Time, DS3231_FORMAT_BIN);
            ds3231_ClearAlarm1Flag();

            if (Time[4] == 0 && Time[5] == 0 && Time[6] == 0)
                ret = true;
            else
                ret = false;

            break;

        case 0x00:
            if (dy_dt) {
                Data[0] = 0;
                Data[1] = 0;
                Data[2] = 0;
                Data[3] = 0;
                ds3231_SetAlarm1(Data, 0,
                    0);
                ds3231_ReadTime(Time, DS3231_FORMAT_BIN);
                ds3231_ClearAlarm1Flag();

                if (Time[3] == 0 && Time[4] == 0 && Time[5] == 0 && Time[6] == 0)
                    ret = true;
                else
                    ret = false;
            } else {
                Data[0] = 0;
                Data[1] = 0;
                Data[2] = 0;
                Data[3] = 0;
                ds3231_SetAlarm1(Data, 0,
                    0);
                ds3231_ReadTime(Time, DS3231_FORMAT_BIN);
                ds3231_ClearAlarm1Flag();

                if (Time[3] == 0 && Time[4] == 0 && Time[5] == 0 && Time[6] == 0)
                    ret = true;
                else
                    ret = false;
            }

            break;
    }

    return ret;
}

int t_ds3231_SquareWave(int argc, char **argv)
{
    u8 Data = 0;
    u8 rs1 = 0, rs2 = 0;

    if (argc > 1)
        rs2 = strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        rs1 = strtoul(argv[2], &argv[2], 10);

    Data = (rs2 << 1) | rs1;

    switch (Data) {
        case SQUARE_WAVE_1HZ:
            ds3231_RateSelect(SQUARE_WAVE_1HZ);
            break;

        case SQUARE_WAVE_1_024HZ:
            ds3231_RateSelect(SQUARE_WAVE_1_024HZ);
            break;

        case SQUARE_WAVE_4_096HZ:
            ds3231_RateSelect(SQUARE_WAVE_4_096HZ);
            break;

        case SQUARE_WAVE_8_192HZ:
            ds3231_RateSelect(SQUARE_WAVE_8_192HZ);
            break;

        default:
            break;
    }

    ds3231_RateSelect(SQUARE_WAVE_1HZ);
    ds3231_EnableSquareWave();

    return true;
}

int t_ds3231_32kHzWave(int argc, char **argv)
{
    ds3231_Enable32kHzOutput();

    return true;
}

int t_ds3231_GetTemprature(int argc, char **argv)
{
    float Data = 0;

    ds3231_ConvertTemperature();
    ds3231_GetTemperature(&Data);

    printk(KERN_DEBUG "Temperature is %f", Data);

    return true;
}

#endif
