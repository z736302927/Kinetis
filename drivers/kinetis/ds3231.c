#include "kinetis/ds3231.h"
#include "kinetis/iic_soft.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"

#include "string.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "i2c.h"

#define DS3231_ADDR                     0x68

static inline void ds3231_port_transmmit(u8 addr, u8 tmp)
{
    iic_port_transmmit(IIC_1, DS3231_ADDR, addr, tmp);
}

static inline void ds3231_port_receive(u8 addr, u8 *pdata)
{
    iic_port_receive(IIC_1, DS3231_ADDR, addr, pdata);
}

static inline void ds3231_port_multi_transmmit(u8 addr, u8 *pdata, u32 length)
{
    iic_port_multi_transmmit(IIC_1, DS3231_ADDR, addr, pdata, length);
}

static inline void ds3231_port_multi_receive(u8 addr, u8 *pdata, u32 length)
{
    iic_port_multi_receive(IIC_1, DS3231_ADDR, addr, pdata, length);
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

static u8 g_time_mode = DS3231_HOURS24;
static u8 g_time_region = DS3231_AM;

u8 ds3231_get_time_mode(void)
{
    return g_time_mode;
}

void ds3231_set_time_mode(u8 tmp)
{
    g_time_mode = tmp;
}

u8 ds3231_get_time_region(void)
{
    return g_time_region;
}

void ds3231_set_time_region(u8 tmp)
{
    g_time_region = tmp;
}

void ds3231_get_time(u8 *pdata, u8 format)
{
    u8 tmp[7];
    u8 hour10 = 0;

    ds3231_port_multi_receive(SECONDS, tmp, 7);

    if (tmp[2] & 0x40) {
        g_time_mode = DS3231_HOURS12;
        hour10 = (tmp[2] & 0x10) >> 4;

        if (tmp[2] & 0x20)
            g_time_region = DS3231_PM;
        else
            g_time_region = DS3231_AM;
    } else {
        g_time_mode = DS3231_HOURS24;
        hour10 = (tmp[2] & 0x30) >> 4;
    }

    pdata[0] = (tmp[0] >> 4) * 10 + (tmp[0] & 0x0F);
    pdata[1] = (tmp[1] >> 4) * 10 + (tmp[1] & 0x0F);
    pdata[2] = hour10 * 10 + (tmp[2] & 0x0F);
    pdata[3] = (tmp[4] >> 4) * 10 + (tmp[4] & 0x0F);
    pdata[4] = ((tmp[5] & 0x1F) >> 4) * 10 + (tmp[5] & 0x0F);
    pdata[5] = (tmp[6] >> 4) * 10 + (tmp[6] & 0x0F);
}

void ds3231_set_time(u8 *pdata, u8 format)
{
    u8 tmp[7] = {0, 0, 0, 0, 0, 0, 0};

    ds3231_port_multi_transmmit(SECONDS, tmp, 3);
    ds3231_port_multi_transmmit(DATE, tmp, 3);

    tmp[6] |= (pdata[0] / 10) << 4;
    tmp[6] |= (pdata[0] % 10) << 0;
    tmp[5] |= (pdata[1] / 10) << 4;
    tmp[5] |= (pdata[1] % 10) << 0;
    tmp[4] |= (pdata[2] / 10) << 4;
    tmp[4] |= (pdata[2] % 10) << 0;
    ds3231_port_multi_transmmit(DATE, &tmp[4], 3);

    tmp[2] |= (pdata[3] / 10) << 4;
    tmp[2] |= (pdata[3] % 10) << 0;

    if (g_time_mode == DS3231_HOURS12) {
        if (g_time_region == DS3231_PM)
            tmp[2] |= 0x20;
        else
            tmp[2] &= ~0x20;

        tmp[2] |= 0x40;
    } else
        tmp[2] &= ~0x40;

    tmp[1] |= (pdata[4] / 10) << 4;
    tmp[1] |= (pdata[4] % 10) << 0;
    tmp[0] |= (pdata[5] / 10) << 4;
    tmp[0] |= (pdata[5] % 10) << 0;
    ds3231_port_multi_transmmit(SECONDS, tmp, 3);
}

void ds3231_get_time_with_string(char *pdata)
{
    u8 tmp[6];

    ds3231_get_time(tmp, DS3231_FORMAT_BIN);
    pdata[11] = (tmp[0] % 10) + '0';
    pdata[10] = (tmp[0] / 10) + '0';
    pdata[9] = (tmp[1] % 10) + '0';
    pdata[8] = (tmp[1] / 10) + '0';
    pdata[7] = (tmp[2] % 10) + '0';
    pdata[6] = (tmp[2] / 10) + '0';
    pdata[5] = (tmp[3] % 10) + '0';
    pdata[4] = (tmp[3] / 10) + '0';
    pdata[3] = (tmp[4] % 10) + '0';
    pdata[2] = (tmp[4] / 10) + '0';
    pdata[1] = (tmp[5] % 10) + '0';
    pdata[0] = (tmp[5] / 10) + '0';
}

void ds3231_set_time_with_string(char *pdata)
{
    u8 tmp[6];

    tmp[0] = (pdata[0] - '0') * 10 + (pdata[1] - '0');
    tmp[1] = (pdata[2] - '0') * 10 + (pdata[3] - '0');
    tmp[2] = (pdata[4] - '0') * 10 + (pdata[5] - '0');
    tmp[3] = (pdata[6] - '0') * 10 + (pdata[7] - '0');
    tmp[4] = (pdata[8] - '0') * 10 + (pdata[9] - '0');
    tmp[5] = (pdata[10] - '0') * 10 + (pdata[11] - '0');
    ds3231_set_time(tmp, DS3231_FORMAT_BIN);
}

void ds3231_get_week(u8 *pdata)
{
    ds3231_port_receive(DAY, pdata);
}

void ds3231_set_week(u8 tmp)
{
    ds3231_port_transmmit(DAY, tmp);
}

void ds3231_alarm1_callback(void)
{
    ;
}

void ds3231_set_alarm1(u8 *pdata, u8 date_or_day, u8 alarm)
{
    u8 tmp[4] = {0, 0, 0, 0};
    u8 tens = 0, unit = 0;

    if (alarm & ALARM_MASK_1)
        tmp[0] |= ALARM_MASK;

    if (alarm & ALARM_MASK_2)
        tmp[1] |= ALARM_MASK;

    if (alarm & ALARM_MASK_3)
        tmp[2] |= ALARM_MASK;

    if (alarm & ALARM_MASK_4)
        tmp[3] |= ALARM_MASK;

    unit = (pdata[0] & 0x7F) % 10;
    tens = (pdata[0] & 0x7F) / 10;
    tmp[0] = (tens << 4) | unit;
    unit = (pdata[1] & 0x7F) % 10;
    tens = (pdata[1] & 0x7F) / 10;
    tmp[1] = (tens << 4) | unit;

    if (g_time_mode == DS3231_HOURS24) {
        unit = (pdata[2] & 0x3F) % 10;
        tens = (pdata[2] & 0x3F) / 10;
        tmp[2] = (tens << 4) | unit;
        tmp[2] &= ~TIMEMODE_MASK;
    } else {
        if (pdata[2] > 12)
            pdata[2] -= 12;

        unit = (pdata[2] & 0x1F) % 10;
        tens = (pdata[2] & 0x1F) / 10;
        tmp[2] = (tens << 4) | unit;
        tmp[2] |= TIMEMODE_MASK;
    }

    if (date_or_day == WEEK_CYCLE) {
        unit = (pdata[3] & 0x0F) % 10;
        tmp[3] = unit;
        tmp[3] |= DY_DT_MASK;
    } else if (date_or_day == MONTH_CYCLE) {
        unit = (pdata[3] & 0x0F) % 10;
        tens = (pdata[3] & 0x0F) / 10;
        tmp[3] = (tens << 4) | unit;
        tmp[3] &= ~DY_DT_MASK;
    }

    ds3231_port_multi_receive(ALARM_1_SECONDS, tmp, 4);
}

void ds3231_alarm2_callback(void)
{
    ;
}

void ds3231_set_alarm2(u8 *pdata, u8 DateorDay, u8 alarm)
{
    u8 tmp[3] = {0, 0, 0};
    u8 tens = 0, unit = 0;

    if (alarm & ALARM_MASK_2)
        tmp[0] |= ALARM_MASK;

    if (alarm & ALARM_MASK_3)
        tmp[1] |= ALARM_MASK;

    if (alarm & ALARM_MASK_4)
        tmp[2] |= ALARM_MASK;

    unit = (pdata[0] & 0x7F) % 10;
    tens = (pdata[0] & 0x7F) / 10;
    tmp[0] = (tens << 4) | unit;

    if (g_time_mode == DS3231_HOURS24) {
        unit = (pdata[1] & 0x3F) % 10;
        tens = (pdata[1] & 0x3F) / 10;
        tmp[1] = (tens << 4) | unit;
        tmp[1] &= ~TIMEMODE_MASK;
    } else {
        if (pdata[1] > 12)
            pdata[1] -= 12;

        unit = (pdata[1] & 0x1F) % 10;
        tens = (pdata[1] & 0x1F) / 10;
        tmp[1] = (tens << 4) | unit;
        tmp[1] |= TIMEMODE_MASK;
    }

    if (DateorDay == WEEK_CYCLE) {
        unit = (pdata[2] & 0x0F) % 10;
        tmp[2] = unit;
        tmp[2] |= DY_DT_MASK;
    } else if (DateorDay == MONTH_CYCLE) {
        unit = (pdata[2] & 0x0F) % 10;
        tens = (pdata[2] & 0x0F) / 10;
        tmp[2] = (tens << 4) | unit;
        tmp[2] &= ~DY_DT_MASK;
    }

    ds3231_port_multi_receive(ALARM_2_MINUTES, tmp, 3);
}

void ds3231_enable_oscillator(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL, &reg);

    reg &= ~(0x01 << 7);
    reg |= (0x00 << 7);

    ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_convert_temperature(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL, &reg);

    reg &= ~(0x01 << 6);
    reg |= (0x01 << 6);

    ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_rate_select(u8 tmp)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL, &reg);

    switch (tmp) {
        case SQUARE_WAVE_1HZ:
            reg &= ~(0x00 << 3);
            reg |= (0x00 << 3);
            break;

        case SQUARE_WAVE_1_024HZ:
            reg &= ~(0x01 << 3);
            reg |= (0x01 << 3);
            break;

        case SQUARE_WAVE_4_096HZ:
            reg &= ~(0x20 << 3);
            reg |= (0x20 << 3);
            break;

        case SQUARE_WAVE_8_192HZ:
            reg &= ~(0x03 << 3);
            reg |= (0x03 << 3);
            break;

        default:
            break;
    }

    ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_square_wave_with_bat(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL, &reg);

    reg &= ~(0x01 << 6);
    reg |= (0x01 << 6);

    ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_int(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL, &reg);

    reg &= ~(0x01 << 2);
    reg |= (0x01 << 2);

    ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_square_wave(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL, &reg);

    reg &= ~(0x01 << 2);

    ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_alarm2_int(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL, &reg);

    reg &= ~(0x01 << 1);
    reg |= (0x01 << 1);

    ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_enable_alarm1_int(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL, &reg);

    reg &= ~(0x01 << 0);
    reg |= (0x01 << 0);

    ds3231_port_transmmit(CONTROL, reg);
}

u8 ds3231_oscillator_stop_flag(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL_STATUS, &reg);

    if (reg & 0x80)
        return 1;
    else
        return 0;
}

void ds3231_enable_32khz_output(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL, &reg);

    reg &= ~(0x01 << 3);
    reg |= (0x01 << 3);

    ds3231_port_transmmit(CONTROL, reg);
}

u8 ds3231_wait_busy(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL_STATUS, &reg);

    if (reg & 0x02)
        return 1;
    else
        return 0;
}

static u8 alarm2_flag = 0;

u8 ds3231_alarm2_flag(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL_STATUS, &reg);

    if (reg & 0x02)
        return 1;
    else
        return 0;
}

void ds3231_clear_alarm2_flag(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL, &reg);

    reg &= ~(0x01 << 1);

    ds3231_port_transmmit(CONTROL, reg);
}

static u8 alarm1_flag = 0;

u8 ds3231_alarm1_flag(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL_STATUS, &reg);

    if (reg & 0x01)
        return 1;
    else
        return 0;
}

void ds3231_clear_alarm1_flag(void)
{
    u8 reg = 0;

    ds3231_port_receive(CONTROL, &reg);

    reg &= ~(0x01 << 0);

    ds3231_port_transmmit(CONTROL, reg);
}

void ds3231_aging_offset(u8 *pdata)
{
    ds3231_port_receive(AGING_OFFSET, pdata);
}

void ds3231_get_temperature(float *pdata)
{
    u8 val[2];

    ds3231_port_multi_receive(MSB_OF_TEMP, val, 2);
    *pdata = (float)val[0] + (float)(val[1] >> 6) * 0.25;
}

#ifdef DESIGN_VERIFICATION_DS3231
#include "kinetis/test-kinetis.h"

#include <linux/iopoll.h>
#include <linux/printk.h>

#include "stdlib.h"

int t_ds3231_set_clock(int argc, char **argv)
{
    char time[16];

    ds3231_set_time_mode(DS3231_HOURS24);
    ds3231_set_time_region(DS3231_PM);
    ds3231_set_time_with_string("200308202020");
    ds3231_set_week(7);

    ds3231_get_time_with_string(time);

    if (strcmp("200308202020", time) != 0)
        return false;

    if (g_time_region == DS3231_AM)
        snprintf(&time[12], 4, " DS3231_AM");
    else
        snprintf(&time[12], 4, " PM");

    printk(KERN_DEBUG "%s", time);

    return PASS;
}

int t_ds3231_get_clock(int argc, char **argv)
{
    char time[16];
    u8 i = 0;
    u16 times = 1;

    if (argc > 1)
        times = strtoul(argv[1], &argv[1], 10);

    for (i = 0; i < times; i++) {
        ds3231_get_time_with_string(time);

        if (g_time_region == DS3231_AM)
            snprintf(&time[12], 4, " DS3231_AM");
        else
            snprintf(&time[12], 4, " PM");

        printk(KERN_DEBUG "%s", time);
        mdelay(1000);
    }

    return PASS;
}

int t_ds3231_set_alarm1(int argc, char **argv)
{
    u8 ret = 0;
    u8 dy_dt = 0;
    u8 a1m4 = 0, a1m3 = 0, a1m2 = 0, a1m1 = 0;
    u8 alarm_rate = 0;
    u8 tmp[4] = {0, 0, 0, 0};
    u8 time[7] = {0, 0, 0, 0, 0, 0, 0};
    u8 flag;
    u32 delta = 0;

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

    alarm_rate = (a1m4 << 3) | (a1m3 << 2) | (a1m2 << 1) | a1m1;

    ds3231_enable_int();
    ds3231_enable_alarm1_int();

    switch (alarm_rate) {
        case 0x0F:
            ds3231_set_alarm1(tmp, 0,
                ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3 | ALARM_MASK_4);
            delta = basic_timer_get_ss();
            readb_poll_timeout_atomic(&alarm1_flag, flag, flag == true, 0, 2000);
            delta = basic_timer_get_ss() - delta;
            ds3231_clear_alarm1_flag();

            if (delta == 1)
                ret = PASS;
            else
                ret = FAIL;

            break;

        case 0x0E:
            tmp[0] = 0;
            ds3231_set_alarm1(tmp, 0,
                ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3);
            ds3231_get_time(time, DS3231_FORMAT_BIN);
            ds3231_clear_alarm1_flag();

            if (time[6] == 0)
                ret = PASS;
            else
                ret = FAIL;

            break;

        case 0x0C:
            tmp[0] = 0;
            tmp[1] = 0;
            ds3231_set_alarm1(tmp, 0,
                ALARM_MASK_1 | ALARM_MASK_2);
            ds3231_get_time(time, DS3231_FORMAT_BIN);
            ds3231_clear_alarm1_flag();

            if (time[5] == 0 && time[6] == 0)
                ret = PASS;
            else
                ret = FAIL;

            break;

        case 0x08:
            tmp[0] = 0;
            tmp[1] = 0;
            tmp[2] = 0;
            ds3231_set_alarm1(tmp, 0,
                ALARM_MASK_1);
            ds3231_get_time(time, DS3231_FORMAT_BIN);
            ds3231_clear_alarm1_flag();

            if (time[4] == 0 && time[5] == 0 && time[6] == 0)
                ret = PASS;
            else
                ret = FAIL;

            break;

        case 0x00:
            if (dy_dt) {
                tmp[0] = 0;
                tmp[1] = 0;
                tmp[2] = 0;
                tmp[3] = 0;
                ds3231_set_alarm1(tmp, 0,
                    0);
                ds3231_get_time(time, DS3231_FORMAT_BIN);
                ds3231_clear_alarm1_flag();

                if (time[3] == 0 && time[4] == 0 && time[5] == 0 && time[6] == 0)
                    ret = PASS;
                else
                    ret = FAIL;
            } else {
                tmp[0] = 0;
                tmp[1] = 0;
                tmp[2] = 0;
                tmp[3] = 0;
                ds3231_set_alarm1(tmp, 0,
                    0);
                ds3231_get_time(time, DS3231_FORMAT_BIN);
                ds3231_clear_alarm1_flag();

                if (time[3] == 0 && time[4] == 0 && time[5] == 0 && time[6] == 0)
                ret = PASS;
            else
                ret = FAIL;
            }

            break;
    }
    
    return ret;
}

int t_ds3231_set_alarm2(int argc, char **argv)
{
    u8 ret = 0;
    u8 dy_dt = 0;
    u8 a1m4 = 0, a1m3 = 0, a1m2 = 0, a1m1 = 0;
    u8 alarm_rate = 0;
    u8 tmp[4] = {0, 0, 0, 0};
    u8 time[7] = {0, 0, 0, 0, 0, 0, 0};
    u8 flag;
    u32 delta = 0;

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

    alarm_rate = (a1m4 << 3) | (a1m3 << 2) | (a1m2 << 1) | a1m1;

    ds3231_enable_int();
    ds3231_enable_alarm2_int();

    switch (alarm_rate) {
        case 0x0F:
            ds3231_set_alarm2(tmp, 0,
                ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3 | ALARM_MASK_4);
            delta = basic_timer_get_ss();
            readb_poll_timeout_atomic(&alarm2_flag, flag, flag == true, 0, 2000);
            delta = basic_timer_get_ss() - delta;
            ds3231_clear_alarm2_flag();

            if (delta == 1)
                ret = true;
            else
                ret = false;

            break;

        case 0x0E:
            tmp[0] = 0;
            ds3231_set_alarm2(tmp, 0,
                ALARM_MASK_1 | ALARM_MASK_2 | ALARM_MASK_3);
            ds3231_get_time(time, DS3231_FORMAT_BIN);
            ds3231_clear_alarm2_flag();

            if (time[6] == 0)
                ret = true;
            else
                ret = false;

            break;

        case 0x0C:
            tmp[0] = 0;
            tmp[1] = 0;
            ds3231_set_alarm2(tmp, 0,ALARM_MASK_1 | ALARM_MASK_2);
            ds3231_get_time(time, DS3231_FORMAT_BIN);
            ds3231_clear_alarm2_flag();

            if (time[5] == 0 && time[6] == 0)
                ret = true;
            else
                ret = false;

            break;

        case 0x08:
            tmp[0] = 0;
            tmp[1] = 0;
            tmp[2] = 0;
            ds3231_set_alarm2(tmp, 0,ALARM_MASK_1);
            ds3231_get_time(time, DS3231_FORMAT_BIN);
            ds3231_clear_alarm2_flag();

            if (time[4] == 0 && time[5] == 0 && time[6] == 0)
                ret = true;
            else
                ret = false;

            break;

        case 0x00:
            if (dy_dt) {
                tmp[0] = 0;
                tmp[1] = 0;
                tmp[2] = 0;
                tmp[3] = 0;
                ds3231_set_alarm2(tmp, 0, 0);
                ds3231_get_time(time, DS3231_FORMAT_BIN);
                ds3231_clear_alarm2_flag();

                if (time[3] == 0 && time[4] == 0 && time[5] == 0 && time[6] == 0)
                    ret = true;
                else
                    ret = false;
            } else {
                tmp[0] = 0;
                tmp[1] = 0;
                tmp[2] = 0;
                tmp[3] = 0;
                ds3231_set_alarm2(tmp, 0,0);
                ds3231_get_time(time, DS3231_FORMAT_BIN);
                ds3231_clear_alarm2_flag();

                if (time[3] == 0 && time[4] == 0 && time[5] == 0 && time[6] == 0)
                    ret = true;
                else
                    ret = false;
            }

            break;
    }

    return ret;
}

int t_ds3231_square_wave(int argc, char **argv)
{
    u8 tmp = 0;
    u8 rs1 = 0, rs2 = 0;

    if (argc > 1)
        rs2 = strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        rs1 = strtoul(argv[2], &argv[2], 10);

    tmp = (rs2 << 1) | rs1;

    switch (tmp) {
        case SQUARE_WAVE_1HZ:
            ds3231_rate_select(SQUARE_WAVE_1HZ);
            break;

        case SQUARE_WAVE_1_024HZ:
            ds3231_rate_select(SQUARE_WAVE_1_024HZ);
            break;

        case SQUARE_WAVE_4_096HZ:
            ds3231_rate_select(SQUARE_WAVE_4_096HZ);
            break;

        case SQUARE_WAVE_8_192HZ:
            ds3231_rate_select(SQUARE_WAVE_8_192HZ);
            break;

        default:
            break;
    }

    ds3231_rate_select(SQUARE_WAVE_1HZ);
    ds3231_enable_square_wave();

    return PASS;
}

int t_ds3231_32khz_wave(int argc, char **argv)
{
    ds3231_enable_32khz_output();

    return PASS;
}

int t_ds3231_get_temprature(int argc, char **argv)
{
    float tmp = 0;

    ds3231_convert_temperature();
    ds3231_get_temperature(&tmp);

    printk(KERN_DEBUG "Temperature is %f", tmp);

    return PASS;
}

#endif
