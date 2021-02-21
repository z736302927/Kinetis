#include "kinetis/ak8975.h"
#include "kinetis/iic_soft.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"

#include <linux/bitops.h>
#include <linux/printk.h>

#include "stdlib.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "stm32f4xx_hal.h"

#define AK8975_USING_IIC

#define AK8975_CAD0                    0
#define AK8975_CAD1                    0

#if (!AK8975_CAD0 && !AK8975_CAD0)
#define AK8975_ADDR                    0x0C
#elif (!AK8975_CAD0 && AK8975_CAD0)
#define AK8975_ADDR                    0x0D
#elif (AK8975_CAD0 && !AK8975_CAD0)
#define AK8975_ADDR                    0x0E
#elif (AK8975_CAD0 && AK8975_CAD0)
#define AK8975_ADDR                    0x0F
#endif

static inline void ak8975_csb_low(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

static inline void ak8975_csb_high(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

static inline void ak8975_port_transmmit(u8 addr, u8 tmp)
{
#ifdef AK8975_USING_IIC
    iic_port_transmmit(IIC_1, AK8975_ADDR, addr, tmp);
#else
    ak8975_csb_low();
    HAL_SPI_Transmit(&hspi1, (addr << 1) | 0, 1, 1000);
    HAL_SPI_Transmit(&hspi1, &tmp, 1, 1000);
    ak8975_csb_high();
#endif
}

static inline void ak8975_port_receive(u8 addr, u8 *pdata)
{
#ifdef AK8975_USING_IIC
    iic_port_receive(IIC_1, AK8975_ADDR, addr, pdata);
#else
    ak8975_csb_low();
    HAL_SPI_Transmit(&hspi1, (addr << 1) | 1, 1, 1000);
    HAL_SPI_Receive(&hspi1, pdata, 1, 1000);
    ak8975_csb_high();
#endif
}

static inline void ak8975_port_multi_transmmit(u8 addr, u8 *pdata, u32 length)
{
#ifdef AK8975_USING_IIC
    iic_port_multi_transmmit(IIC_1, AK8975_ADDR, addr, pdata, length);
#else
    ak8975_csb_low();
    HAL_SPI_Transmit(&hspi1, (addr << 1) | 0, 1, 1000);
    HAL_SPI_Transmit(&hspi1, pdata, length, 1000);
    ak8975_csb_high();
#endif
}

static inline void ak8975_port_multi_receive(u8 addr, u8 *pdata, u32 length)
{
#ifdef AK8975_USING_IIC
    iic_port_multi_receive(IIC_1, AK8975_ADDR, addr, pdata, length);
#else
    ak8975_csb_low();
    HAL_SPI_Transmit(&hspi1, (addr << 1) | 1, 1, 1000);
    HAL_SPI_Receive(&hspi1, pdata, length, 1000);
    ak8975_csb_high();
#endif
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define WIA                             0x00
#define INFO                            0x01
#define ST1                             0x02
#define HXL                             0x03
#define HXH                             0x04
#define HYL                             0x05
#define HYH                             0x06
#define HZL                             0x07
#define HZH                             0x08
#define ST2                             0x09
#define CNTL1                           0x0A
#define CNTL2                           0x0B
#define ASTC                            0x0C
#define TS1                             0x0D
#define TS2                             0x0E
#define I2CDIS                          0x0F
#define ASAX                            0x10
#define ASAY                            0x11
#define ASAZ                            0x12

#define AKM_DEVID                       0x48
#define POWER_DOWN                      0x0000
#define SINGLE_MEASUREMENT              0x0001
#define SELF_TEST                       0x1000
#define FUSE_ROM_ACCESS                 0x1111
#define AK8975_TBD                      0

void ak8975_enter_power_down_mode(void)
{
    ak8975_port_transmmit(CNTL1, POWER_DOWN);
}

void ak8975_enter_single_measurement_mode(void)
{
    ak8975_port_transmmit(CNTL1, SINGLE_MEASUREMENT);
}

void ak8975_enter_selftest_mode(void)
{
    u8 tmp[2];

    tmp[0] = SELF_TEST & 0xFF;
    tmp[1] = SELF_TEST >> 8;

    ak8975_port_multi_transmmit(CNTL1, tmp, 2);
}

void ak8975_enter_fuse_rom_access_mode(void)
{
    u8 tmp[2];

    tmp[0] = FUSE_ROM_ACCESS & 0xFF;
    tmp[1] = FUSE_ROM_ACCESS >> 8;

    ak8975_port_multi_transmmit(CNTL1, tmp, 2);
}

void ak8975_who_am_i(u8 *pdata)
{
    ak8975_port_receive(WIA, pdata);
}

void ak8975_device_information(u8 *pdata)
{
    ak8975_port_receive(INFO, pdata);
}

u8 ak8975_data_ready(void)
{
    u8 reg = 0;

    ak8975_port_receive(ST1, &reg);

    return test_bit(0, (unsigned long *)&reg);
}

u8 ak8975_data_overrun(void)
{
    u8 reg = 0;

    ak8975_port_receive(ST1, &reg);

    return test_bit(1, (unsigned long *)&reg);
}

void ak8975_magnetic_measurements(u16 *pdata)
{
    u8 tmp[6];

    ak8975_port_multi_receive(HXL, tmp, 6);

    pdata[0] = (tmp[1] << 8) | tmp[0];
    pdata[1] = (tmp[3] << 8) | tmp[2];
    pdata[2] = (tmp[5] << 8) | tmp[4];
}

u8 ak8975_magnetic_sensor_overflow(void)
{
    u8 reg = 0;

    ak8975_port_receive(ST2, &reg);

    return test_bit(3, (unsigned long *)&reg);
}

u8 ak8975_output_bit_setting_mirror(void)
{
    u8 reg = 0;

    ak8975_port_receive(ST2, &reg);

    return test_bit(4, (unsigned long *)&reg);
}

void ak8975_operation_mode_setting(u8 tmp)
{
    u8 reg = 0;

    ak8975_port_receive(CNTL1, &reg);

    set_mask_bits(&reg, 0x0F, tmp);

    ak8975_port_transmmit(CNTL1, reg);
}

void ak8975_output_bit_setting(u8 tmp)
{
    u8 reg = 0;

    ak8975_port_receive(CNTL1, &reg);

    set_mask_bits(&reg, 0x10, tmp);

    ak8975_port_transmmit(CNTL1, reg);
}

void ak8975_soft_reset(u8 tmp)
{
    u8 reg = 0;

    ak8975_port_receive(CNTL2, &reg);

    set_mask_bits(&reg, 0x01, tmp);

    ak8975_port_transmmit(CNTL2, reg);
}

void ak8975_selftest_control(u8 tmp)
{
    u8 reg = 0;

    ak8975_port_receive(ASTC, &reg);

    set_mask_bits(&reg, 0x40, tmp);

    ak8975_port_transmmit(ASTC, reg);
}

void ak8975_i2c_disable(void)
{
    ak8975_port_transmmit(I2CDIS, 0x1B);
}

void ak8975_i2c_enable(void)
{
    ak8975_soft_reset(1);
}

static u8 ak8975_asa_values[3];

void ak8975_sensitivity_adjustment_values(u8 *pdata)
{
    ak8975_port_multi_receive(ASAX, pdata, 3);
}

static volatile u8 ak8975_dr_flag = 0;

u8 ak8975_magnetic_adjusted_measurements(u16 *pdata)
{
    u16 raw_data[3];
    u32 timeout = 1000;

    ak8975_enter_single_measurement_mode();

    do {
        if (ak8975_data_ready() == true)
            break;
        else
            mdelay(1);
    } while (timeout--);

    if (timeout <= 0) {
        printk(KERN_ERR "[Error] ak8975 magnetic tmp not ready");
        return false;
    }

//  timeout_WaitMSDone(&ak8975_DR_Flag, true, 1000);

    if (ak8975_data_overrun() == true)
        printk(KERN_DEBUG "[Warning] ak8975 magnetic tmp Overrun");

    ak8975_magnetic_measurements(raw_data);

    pdata[0] = raw_data[0] * ((ak8975_asa_values[0] - 128) / 256 + 1);
    pdata[1] = raw_data[1] * ((ak8975_asa_values[1] - 128) / 256 + 1);
    pdata[2] = raw_data[2] * ((ak8975_asa_values[2] - 128) / 256 + 1);

    return true;
}

void ak8975_init(void)
{
    ak8975_enter_fuse_rom_access_mode();
    ak8975_sensitivity_adjustment_values(ak8975_asa_values);
    printk(KERN_DEBUG "ak8975 Adjustment Values %x, %x, %x",
        ak8975_asa_values[0], ak8975_asa_values[1], ak8975_asa_values[2]);
    ak8975_enter_power_down_mode();
}

#ifdef DESIGN_VERIFICATION_AK8975
#include "kinetis/test-kinetis.h"

int t_ak8975_basic_info(int argc, char **argv)
{
    u8 tmp = 0;

    ak8975_who_am_i(&tmp);
    printk(KERN_DEBUG "Device ID of AKM8975 is 0x%x", tmp);

    if (tmp == AKM_DEVID) {
        printk(KERN_ERR "Device ID of AKM8975 is not correct");
        return FAIL;
    }

    ak8975_device_information(&tmp);
    printk(KERN_DEBUG "Device information for AKM8975 %x", tmp);

    if (tmp != 0x00)
        return PASS;
    else
        return FAIL;
}

int t_ak8975_magnetic(int argc, char **argv)
{
    u16 magnetic[3] = {0, 0, 0};
    u16 times = 128;
    u8 i = 0;
    u32 timeout = 1000;

    if (argc > 1)
        times = strtoul(argv[1], &argv[1], 10);

    ak8975_enter_power_down_mode();

    for (i = 0; i < times; i++) {
        ak8975_enter_single_measurement_mode();

        do {
            if (ak8975_data_ready() == true)
                break;
            else
                mdelay(1);
        } while (timeout--);

        if (timeout <= 0) {
            printk(KERN_ERR "[Error] ak8975 magnetic tmp not ready");
            return FAIL;
        }

//    timeout_WaitMSDone(&ak8975_DR_Flag, true, 1000);

        if (ak8975_data_overrun() == true)
            printk(KERN_WARNING "[Warning] ak8975 magnetic tmp Overrun");

        ak8975_magnetic_measurements(magnetic);
        printk(KERN_DEBUG "ak8975 magnetic tmp %x, %x, %x",
            magnetic[0], magnetic[1], magnetic[2]);
    }

    return PASS;
}

int t_ak8975_selftest(int argc, char **argv)
{
    u16 magnetic[3] = {0, 0, 0};
    u32 timeout = 1000;

    ak8975_enter_power_down_mode();
    ak8975_selftest_control(true);
    ak8975_enter_selftest_mode();

    do {
        if (ak8975_data_ready() == true)
            break;
        else
            mdelay(1);
    } while (timeout--);

    if (timeout <= 0) {
        printk(KERN_ERR "[Error] ak8975 magnetic tmp not ready");
        return FAIL;
    }

//  timeout_WaitMSDone(&ak8975_DR_Flag, true, 1000);

    ak8975_magnetic_measurements(magnetic);
    ak8975_selftest_control(false);
    printk(KERN_DEBUG "ak8975 Selftest magnetic tmp %x, %x, %x",
        magnetic[0], magnetic[1], magnetic[2]);

    if (magnetic[0] <= AK8975_TBD &&
        magnetic[1] <= AK8975_TBD &&
        magnetic[2] <= AK8975_TBD)
        printk(KERN_DEBUG "AK8975/B is working normally");
    else
        return FAIL;

    return PASS;
}

int t_ak8975_fuse_rom_access(int argc, char **argv)
{
    u8 magnetic[3] = {0, 0, 0};

    ak8975_enter_fuse_rom_access_mode();
    ak8975_sensitivity_adjustment_values(magnetic);
    printk(KERN_DEBUG "ak8975 Adjustment Values %x, %x, %x",
        magnetic[0], magnetic[1], magnetic[2]);
    ak8975_enter_power_down_mode();

    return PASS;
}

#endif

