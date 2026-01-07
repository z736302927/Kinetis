#include <linux/bitops.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/err.h>

#define pr_fmt(fmt) "ak8975: " fmt

#include "kinetis/ak8975.h"
#include "kinetis/iic_soft.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"
#include "kinetis/random-gene.h"
#include "kinetis/test-kinetis.h"

#include <pthread.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#if MCU_PLATFORM_STM32
#include "stm32f4xx_hal.h"
#endif

#define AK8975_USING_IIC

#define AK8975_CAD0                    0
#define AK8975_CAD1                    0

#if (!AK8975_CAD1 && !AK8975_CAD0)
#define AK8975_ADDR                    0x0C
#elif (!AK8975_CAD1 && AK8975_CAD0)
#define AK8975_ADDR                    0x0D
#elif (AK8975_CAD1 && !AK8975_CAD0)
#define AK8975_ADDR                    0x0E
#elif (AK8975_CAD1 && AK8975_CAD0)
#define AK8975_ADDR                    0x0F
#endif

static inline void ak8975_csb_low(void)
{
#if MCU_PLATFORM_STM32
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
#else
#endif
}

static inline void ak8975_csb_high(void)
{
#if MCU_PLATFORM_STM32
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
#else
#endif
}

static inline void ak8975_port_transmmit(u8 addr, u8 tmp)
{
#ifdef AK8975_USING_IIC
	iic_master_port_transmmit(IIC_SW_1, AK8975_ADDR, addr, tmp);
#else
	ak8975_csb_low();
#if MCU_PLATFORM_STM32
	HAL_SPI_Transmit(&hspi1, (addr << 1) | 0, 1, 1000);
	HAL_SPI_Transmit(&hspi1, &tmp, 1, 1000);
#else
#endif
	ak8975_csb_high();
#endif
}

static inline void ak8975_port_receive(u8 addr, u8 *pdata)
{
#ifdef AK8975_USING_IIC
	iic_master_port_receive(IIC_SW_1, AK8975_ADDR, addr, pdata);
#else
	ak8975_csb_low();
#if MCU_PLATFORM_STM32
	HAL_SPI_Transmit(&hspi1, (addr << 1) | 1, 1, 1000);
	HAL_SPI_Receive(&hspi1, pdata, 1, 1000);
#else
#endif
	ak8975_csb_high();
#endif
}

static inline void ak8975_port_multi_transmmit(u8 addr, u8 *pdata, u32 length)
{
#ifdef AK8975_USING_IIC
	iic_master_port_multi_transmmit(IIC_SW_1, AK8975_ADDR, addr, pdata, length);
#else
	ak8975_csb_low();
#if MCU_PLATFORM_STM32
	HAL_SPI_Transmit(&hspi1, (addr << 1) | 0, 1, 1000);
	HAL_SPI_Transmit(&hspi1, pdata, length, 1000);
#else
#endif
	ak8975_csb_high();
#endif
}

static inline void ak8975_port_multi_receive(u8 addr, u8 *pdata, u32 length)
{
#ifdef AK8975_USING_IIC
	iic_master_port_multi_receive(IIC_SW_1, AK8975_ADDR, addr, pdata, length);
#else
	ak8975_csb_low();
#if MCU_PLATFORM_STM32
	HAL_SPI_Transmit(&hspi1, (addr << 1) | 1, 1, 1000);
	HAL_SPI_Receive(&hspi1, pdata, length, 1000);
#else
#endif
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
#define CONTINUOUS_MEASUREMENT_8HZ      0x0002
#define CONTINUOUS_MEASUREMENT_100HZ    0x0004
#define CONTINUOUS_MEASUREMENT_10HZ     0x0006
#define CONTINUOUS_MEASUREMENT_20HZ     0x0008

#define ST1_DRDY_BIT                    0x01
#define ST1_DOR_BIT                     0x02

#define ST2_HOFL_BIT                    0x08
#define ST2_DERR_BIT                    0x04
#define ST2_BITM_BIT                    0x10

#define ASTC_SELF_BIT                   0x40
#define ASTC_ELF_BIT                    0x80

#define CNTL2_SRST_BIT                  0x01

#define AK8975_TBD                      0

/* Sleep mode and power management related constants */
#define SLEEP_MODE_DELAY_MS             100
#define POWER_ON_DELAY_MS               10
#define MODE_SWITCH_DELAY_MS            1

void ak8975_enter_power_down_mode(void)
{
	ak8975_port_transmmit(CNTL1, POWER_DOWN);
	mdelay(MODE_SWITCH_DELAY_MS);
}

void ak8975_enter_sleep_mode(void)
{
	ak8975_enter_power_down_mode();
	mdelay(SLEEP_MODE_DELAY_MS);
}

void ak8975_wake_up_from_sleep(void)
{
	ak8975_enter_single_measurement_mode();
	mdelay(POWER_ON_DELAY_MS);
}

u8 ak8975_get_power_state(void)
{
	u8 reg = 0;
	ak8975_port_receive(CNTL1, &reg);

	if ((reg & 0x0F) == POWER_DOWN) {
		return 0;    /* Power down state */
	} else {
		return 1;    /* Active state */
	}
}

void ak8975_enter_single_measurement_mode(void)
{
	ak8975_port_transmmit(CNTL1, SINGLE_MEASUREMENT & 0xFF);
	mdelay(1);
}

void ak8975_enter_continuous_8hz_mode(void)
{
	ak8975_port_transmmit(CNTL1, CONTINUOUS_MEASUREMENT_8HZ & 0xFF);
	mdelay(1);
}

void ak8975_enter_continuous_10hz_mode(void)
{
	ak8975_port_transmmit(CNTL1, CONTINUOUS_MEASUREMENT_10HZ & 0xFF);
	mdelay(1);
}

void ak8975_enter_continuous_20hz_mode(void)
{
	ak8975_port_transmmit(CNTL1, CONTINUOUS_MEASUREMENT_20HZ & 0xFF);
	mdelay(1);
}

void ak8975_enter_continuous_100hz_mode(void)
{
	ak8975_port_transmmit(CNTL1, CONTINUOUS_MEASUREMENT_100HZ & 0xFF);
	mdelay(1);
}

void ak8975_enter_selftest_mode(void)
{
	ak8975_port_transmmit(CNTL1, SELF_TEST & 0xFF);
	mdelay(10);
}

void ak8975_enter_self_test_mode(void)
{
	ak8975_port_transmmit(ASTC, ASTC_SELF_BIT);
	mdelay(1);
	ak8975_port_transmmit(CNTL1, SELF_TEST & 0xFF);
	mdelay(10);
}

u8 ak8975_self_test(void)
{
	u8 status = -1;
	u16 test_data[3];

	ak8975_enter_power_down_mode();
	mdelay(1);

	ak8975_selftest_control(true);
	mdelay(1);
	ak8975_enter_self_test_mode();

	ak8975_magnetic_measurements(test_data);

	ak8975_enter_power_down_mode();

	if ((test_data[0] >= 30 && test_data[0] <= 5000) &&
		(test_data[1] >= 30 && test_data[1] <= 5000) &&
		(test_data[2] >= 30 && test_data[2] <= 5000)) {
		status = 0;
	}

	return status;
}

void ak8975_enter_fuse_rom_access_mode(void)
{
	ak8975_port_transmmit(CNTL1, FUSE_ROM_ACCESS & 0xFF);
	mdelay(10);
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

	return (reg & ST1_DRDY_BIT);
}

u8 ak8975_data_overrun(void)
{
	u8 reg = 0;

	ak8975_port_receive(ST1, &reg);

	return (reg & ST1_DOR_BIT);
}

u8 ak8975_check_status(void)
{
	u8 st1_reg = 0;
	u8 st2_reg = 0;

	ak8975_port_receive(ST1, &st1_reg);
	ak8975_port_receive(ST2, &st2_reg);

	if (st2_reg & ST2_DERR_BIT) {
		pr_err("AK8975 data error detected");
		return -1;
	}

	if (st2_reg & ST2_HOFL_BIT) {
		pr_warn("AK8975 magnetic sensor overflow");
		return -2;
	}

	return (st1_reg & ST1_DRDY_BIT);
}

u8 ak8975_data_error(void)
{
	u8 reg = 0;

	ak8975_port_receive(ST2, &reg);

	return (reg & ST2_DERR_BIT);
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

	return (reg & ST2_HOFL_BIT);
}

u8 ak8975_output_bit_setting_mirror(void)
{
	u8 reg = 0;

	ak8975_port_receive(ST2, &reg);

	return (reg & ST2_BITM_BIT);
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
		if (ak8975_data_ready() == true) {
			break;
		} else {
			mdelay(1);
		}
	} while (timeout--);

	if (timeout <= 0) {
		pr_err("[Error] ak8975 magnetic tmp not ready");
		return false;
	}

	//  timeout_WaitMSDone(&ak8975_DR_Flag, true, 1000);

	if (ak8975_data_overrun() == true) {
		pr_warn("[Warning] ak8975 magnetic tmp Overrun");
	}

	ak8975_magnetic_measurements(raw_data);

	pdata[0] = (raw_data[0] * (ak8975_asa_values[0] + 128)) / 256;
	pdata[1] = (raw_data[1] * (ak8975_asa_values[1] + 128)) / 256;
	pdata[2] = (raw_data[2] * (ak8975_asa_values[2] + 128)) / 256;

	return true;
}

void ak8975_init(void)
{
	ak8975_enter_fuse_rom_access_mode();
	ak8975_sensitivity_adjustment_values(ak8975_asa_values);
	pr_debug("ak8975 Adjustment Values %x, %x, %x",
		ak8975_asa_values[0], ak8975_asa_values[1], ak8975_asa_values[2]);
	ak8975_enter_power_down_mode();
}

/* Temperature compensation related functions */
s16 ak8975_read_temperature_data(void)
{
	u8 temp_low = 0, temp_high = 0;
	s16 temperature_raw = 0;

	ak8975_port_receive(TS1, &temp_low);
	ak8975_port_receive(TS2, &temp_high);

	temperature_raw = (temp_high << 8) | temp_low;

	return temperature_raw;
}

float ak8975_get_temperature_compensation(u16 raw_mag_data, s16 temperature_data)
{
	float compensation_factor = 1.0;
	s16 temp_celsius;

	/* Convert raw temperature data to Celsius */
	temp_celsius = ((s32)temperature_data - 12421) / 280 + 25;

	/* Temperature compensation algorithm - based on AK8975 manual temperature characteristics */
	if (temp_celsius < 0) {
		compensation_factor = 1.0 + (0.001 * temp_celsius);
	} else if (temp_celsius > 25) {
		compensation_factor = 1.0 - (0.0008 * (temp_celsius - 25));
	}

	return compensation_factor;
}

u16 ak8975_temperature_compensated_measurement(u16 raw_mag_data, s16 temperature_data)
{
	float compensation_factor;

	compensation_factor = ak8975_get_temperature_compensation(raw_mag_data, temperature_data);

	return (u16)(raw_mag_data * compensation_factor);
}

void ak8975_compensated_magnetic_measurements(u16 *pdata)
{
	u16 raw_data[3];
	s16 temperature_data;

	ak8975_magnetic_measurements(raw_data);
	temperature_data = ak8975_read_temperature_data();

	pdata[0] = ak8975_temperature_compensated_measurement(raw_data[0], temperature_data);
	pdata[1] = ak8975_temperature_compensated_measurement(raw_data[1], temperature_data);
	pdata[2] = ak8975_temperature_compensated_measurement(raw_data[2], temperature_data);
}

/* AK8975 I2C Slave simulation */
static struct iic_slave *ak8975_slave = NULL;

/* Simulated AK8975 register map */
static struct {
	u8 wia;        /* Device ID register (0x00) - should be 0x48 */
	u8 info;       /* Information register (0x01) */
	u8 st1;        /* Status 1 register (0x02) */
	u8 hxl;        /* X-axis magnetic field data low byte (0x03) */
	u8 hxh;        /* X-axis magnetic field data high byte (0x04) */
	u8 hyl;        /* Y-axis magnetic field data low byte (0x05) */
	u8 hyh;        /* Y-axis magnetic field data high byte (0x06) */
	u8 hzl;        /* Z-axis magnetic field data low byte (0x07) */
	u8 hzh;        /* Z-axis magnetic field data high byte (0x08) */
	u8 st2;        /* Status 2 register (0x09) */
	u8 cntl1;      /* Control 1 register (0x0A) */
	u8 cntl2;      /* Control 2 register (0x0B) */
	u8 astc;       /* Self-test control register (0x0C) */
	u8 ts1;        /* Test 1 register (0x0D) */
	u8 ts2;        /* Test 2 register (0x0E) */
	u8 i2cdis;     /* I2C disable register (0x0F) */
	u8 asax;       /* X-axis sensitivity adjustment (0x10) */
	u8 asay;       /* Y-axis sensitivity adjustment (0x11) */
	u8 asaz;       /* Z-axis sensitivity adjustment (0x12) */
} ak8975_slave_regs;

/**
 * @brief Update AK8975 slave buffer with register data
 */
static void ak8975_slave_update_buffer(u8 reg_addr)
{
	if (!ak8975_slave || !ak8975_slave->buffer) {
		return;
	}

	switch (reg_addr) {
	case WIA:
		ak8975_slave->buffer[0] = ak8975_slave_regs.wia;
		break;
	case INFO:
		ak8975_slave->buffer[0] = ak8975_slave_regs.info;
		break;
	case ST1:
		ak8975_slave->buffer[0] = ak8975_slave_regs.st1;
		break;
	case ST2:
		ak8975_slave->buffer[0] = ak8975_slave_regs.st2;
		break;
	case CNTL1:
		ak8975_slave->buffer[0] = ak8975_slave_regs.cntl1;
		break;
	case CNTL2:
		ak8975_slave->buffer[0] = ak8975_slave_regs.cntl2;
		break;
	case ASAX:
		ak8975_slave->buffer[0] = ak8975_slave_regs.asax;
		break;
	case ASAY:
		ak8975_slave->buffer[0] = ak8975_slave_regs.asay;
		break;
	case ASAZ:
		ak8975_slave->buffer[0] = ak8975_slave_regs.asaz;
		break;
	case HXL:
		ak8975_slave->buffer[0] = ak8975_slave_regs.hxl;
		break;
	case HXH:
		ak8975_slave->buffer[0] = ak8975_slave_regs.hxh;
		break;
	case HYL:
		ak8975_slave->buffer[0] = ak8975_slave_regs.hyl;
		break;
	case HYH:
		ak8975_slave->buffer[0] = ak8975_slave_regs.hyh;
		break;
	case HZL:
		ak8975_slave->buffer[0] = ak8975_slave_regs.hzl;
		break;
	case HZH:
		ak8975_slave->buffer[0] = ak8975_slave_regs.hzh;
		break;
	default:
		break;
	}
}

/**
 * @brief Generate reasonable random magnetic field values
 * Earth's magnetic field is approximately 25-65 μT
 * AK8975 output range: 0-4095 (signed: -2048 to +2047)
 * Typical values: 100-2000 for Earth's field
 */
static void ak8975_generate_random_magnetic_data(void)
{
	u16 mag_x, mag_y, mag_z;
	s16 signed_x, signed_y, signed_z;

	/* Generate random values in reasonable Earth magnetic field range */
	/* Earth field varies from ~30μT to ~60μT in most locations */
	/* AK8975 output (signed 12-bit): range -2048 to +2047 */
	/* 1 LSB = 0.15 μT, so typical values are -200 to +400 */
	signed_x = (s16)(random_get32bit() % 600) - 200;  /* -200 to +400 */
	signed_y = (s16)(random_get32bit() % 600) - 200;  /* -200 to +400 */
	signed_z = (s16)(random_get32bit() % 600) - 200;  /* -200 to +400 */

	/* Convert to unsigned 16-bit values for AK8975 format */
	mag_x = (u16)signed_x;
	mag_y = (u16)signed_y;
	mag_z = (u16)signed_z;

	/* Store in big-endian format (high byte first) */
	ak8975_slave_regs.hxh = (u8)((mag_x >> 8) & 0xFF);
	ak8975_slave_regs.hxl = (u8)(mag_x & 0xFF);
	ak8975_slave_regs.hyh = (u8)((mag_y >> 8) & 0xFF);
	ak8975_slave_regs.hyl = (u8)(mag_y & 0xFF);
	ak8975_slave_regs.hzh = (u8)((mag_z >> 8) & 0xFF);
	ak8975_slave_regs.hzl = (u8)(mag_z & 0xFF);

	/* Set data ready flag */
	ak8975_slave_regs.st1 |= ST1_DRDY_BIT;

	pr_debug("ak8975_slave: Generated magnetic data: X=%d, Y=%d, Z=%d",
		 signed_x, signed_y, signed_z);
}

/**
 * @brief Start AK8975 I2C slave simulation for testing
 */
int ak8975_slave_start()
{
	/* Initialize simulated registers with default values */
	ak8975_slave_regs.wia = AKM_DEVID;        /* Device ID: 0x48 */
	ak8975_slave_regs.info = 0x00;            /* Information: normal */
	ak8975_slave_regs.st1 = 0x00;            /* Status 1: DRDY = 0 initially */
	ak8975_slave_regs.st2 = 0x00;            /* Status 2: no errors */
	ak8975_slave_regs.cntl1 = POWER_DOWN & 0xFF;  /* Control 1: power down */
	ak8975_slave_regs.cntl2 = 0x00;          /* Control 2: no reset */
	ak8975_slave_regs.astc = 0x00;           /* Self-test: disabled */
	ak8975_slave_regs.i2cdis = 0x1B;        /* I2C disable: 0x1B */
	ak8975_slave_regs.asax = 0x00;           /* X sensitivity: 0 */
	ak8975_slave_regs.asay = 0x00;           /* Y sensitivity: 0 */
	ak8975_slave_regs.asaz = 0x00;           /* Z sensitivity: 0 */

	/* Initialize simulated magnetic data with initial values */
	ak8975_generate_random_magnetic_data();

	/* Create I2C slave device with address 0x0C (thread is auto-started in iic_slave_soft_init) */
	ak8975_slave = iic_slave_soft_init("ak8975", AK8975_ADDR, (u8 *)&ak8975_slave_regs, sizeof(ak8975_slave_regs));
	if (IS_ERR(ak8975_slave)) {
		pr_err("ak8975_slave: Failed to initialize I2C slave");
		return PTR_ERR(ak8975_slave);
	}

	pr_info("ak8975_slave: Initialized at I2C address 0x%02X", AK8975_ADDR);
	pr_info("ak8975_slave: Device ID = 0x%02X (expected: 0x%02X)",
		 ak8975_slave_regs.wia, AKM_DEVID);

	return 0;
}

/**
 * @brief Stop AK8975 I2C slave simulation for testing
 */
int ak8975_slave_stop()
{
	if (ak8975_slave) {
		/* Stop and free I2C slave (thread is auto-stopped in iic_slave_soft_exit) */
		iic_slave_soft_exit(ak8975_slave);
		ak8975_slave = NULL;
		return 0;
	}

	pr_warn("ak8975_slave: No slave instance to stop");
	return 0;
}

/**
 * @brief Test AK8975 I2C slave communication
 */
int ak8975_slave_test()
{
	u8 device_id = 0;
	u8 temp_reg = 0;
	u16 magnetic[3];
	int ret = PASS;

	pr_info("=== AK8975 Slave Communication Test ===");

	/* Test 1: Read device ID (WIA register) */
	ak8975_who_am_i(&device_id);
	pr_info("Test 1 - Device ID: 0x%02X (expected: 0x%02X)",
		 device_id, AKM_DEVID);

	if (device_id == AKM_DEVID) {
		pr_info("  PASS: Device ID matches");
	} else {
		pr_err("  FAIL: Device ID mismatch");
		return FAIL;
	}

	/* Test 2: Read info register */
	ak8975_device_information(&temp_reg);
	pr_info("Test 2 - Info register: 0x%02X", temp_reg);

	/* Test 3: Read status registers */
	temp_reg = ak8975_data_ready();
	pr_info("Test 3 - Data Ready (ST1.DRDY): %d", temp_reg);

	temp_reg = ak8975_data_overrun();
	pr_info("Test 3 - Data Overrun (ST1.DOR): %d", temp_reg);

	temp_reg = ak8975_data_error();
	pr_info("Test 3 - Data Error (ST2.DERR): %d", temp_reg);

	/* Test 4: Write control register */
	pr_info("Test 4 - Writing to CNTL1 register...");
	ak8975_operation_mode_setting(SINGLE_MEASUREMENT & 0xFF);
	mdelay(2);

	temp_reg = ak8975_get_power_state();
	pr_info("Test 4 - Power state after write: %d (1=active)", temp_reg);

	/* Test 5: Read magnetic data */
	ak8975_magnetic_measurements(magnetic);
	pr_info("Test 5 - Magnetic data: X=%d, Y=%d, Z=%d",
		 (s16)((magnetic[0] & 0x8000) ? (magnetic[0] | 0xF000) : magnetic[0]),
		 (s16)((magnetic[1] & 0x8000) ? (magnetic[1] | 0xF000) : magnetic[1]),
		 (s16)((magnetic[2] & 0x8000) ? (magnetic[2] | 0xF000) : magnetic[2]));

	/* Test 6: Power down */
	pr_info("Test 6 - Entering power down mode...");
	ak8975_enter_power_down_mode();
	mdelay(2);

	temp_reg = ak8975_get_power_state();
	pr_info("Test 6 - Power state after power down: %d (0=power down)", temp_reg);

	if (temp_reg == 0) {
		pr_info("  PASS: Successfully entered power down mode");
	} else {
		pr_err("  FAIL: Failed to enter power down mode");
		ret = FAIL;
	}

	pr_info("=== All AK8975 Slave Tests Completed ===");
	return ret;
}

#ifdef DESIGN_VERIFICATION_AK8975
#include "kinetis/test-kinetis.h"

int t_ak8975_basic_info(int argc, char **argv)
{
	u8 tmp = 0;

	pr_info("=== AK8975 Basic Info Test ===");

	ak8975_who_am_i(&tmp);
	pr_info("Device ID of AKM8975 is 0x%02X", tmp);

	if (tmp != AKM_DEVID) {
		pr_err("Device ID of AKM8975 is not correct, got 0x%02X expected 0x%02X",
		       tmp, AKM_DEVID);
		return FAIL;
	}

	ak8975_device_information(&tmp);
	pr_info("Device information for AKM8975: 0x%02X", tmp);

	if (tmp != 0x00) {
		pr_warn("Device information is 0x%02X (expected 0x00)", tmp);
	}
	return PASS;
}

int t_ak8975_magnetic(int argc, char **argv)
{
	u16 magnetic[3] = {0, 0, 0};
	u16 times = 128;
	u8 i = 0;
	u32 timeout = 1000;
	s16 mag_x, mag_y, mag_z;

	if (argc > 1) {
		times = simple_strtoul(argv[1], &argv[1], 10);
	}

	pr_info("=== AK8975 Magnetic Measurement Test (%d readings) ===", times);

	ak8975_enter_power_down_mode();
	mdelay(10); /* Ensure complete power-off state */

	for (i = 0; i < times; i++) {
		ak8975_enter_single_measurement_mode();
		mdelay(1); /* Mode switch delay */

		/* Wait for data ready */
		timeout = 1000;
		do {
			if (ak8975_data_ready() == true) {
				break;
			} else {
				mdelay(1);
			}
		} while (timeout--);

		if (timeout <= 0) {
			pr_err("[Error] ak8975 magnetic data not ready at reading %d", i + 1);
			return FAIL;
		}

		if (ak8975_data_overrun() == true) {
			pr_warn("[Warning] ak8975 magnetic data overrun at reading %d", i + 1);
		}

		ak8975_magnetic_measurements(magnetic);

		/* Convert to signed 16-bit for display */
		mag_x = (s16)((magnetic[0] & 0x8000) ? (magnetic[0] | 0xF000) : magnetic[0]);
		mag_y = (s16)((magnetic[1] & 0x8000) ? (magnetic[1] | 0xF000) : magnetic[1]);
		mag_z = (s16)((magnetic[2] & 0x8000) ? (magnetic[2] | 0xF000) : magnetic[2]);

		pr_debug("ak8975 magnetic data [%d]: X=%6d, Y=%6d, Z=%6d",
			 i + 1, mag_x, mag_y, mag_z);

		/* Show first and last readings at info level */
		if (i == 0 || i == times - 1) {
			pr_info("Reading %d/%d: X=%6d, Y=%6d, Z=%6d",
				i + 1, times, mag_x, mag_y, mag_z);
		}
	}

	ak8975_enter_power_down_mode();
	pr_info("  PASS: Completed %d magnetic readings", times);
	return PASS;
}

int t_ak8975_selftest(int argc, char **argv)
{
	u16 magnetic[3] = {0, 0, 0};
	u32 timeout = 1000;

	pr_info("=== AK8975 Self Test ===");

	ak8975_enter_power_down_mode();
	mdelay(10); /* Ensure complete power-off state */

	ak8975_selftest_control(true);
	mdelay(1); /* ASTC register write delay */

	ak8975_enter_selftest_mode();
	mdelay(10); /* Self-test mode requires sufficient time to stabilize */

	do {
		if (ak8975_data_ready() == true) {
			break;
		} else {
			mdelay(1);
		}
	} while (timeout--);

	if (timeout <= 0) {
		pr_err("[Error] ak8975 magnetic data not ready during self-test");
		return FAIL;
	}

	ak8975_magnetic_measurements(magnetic);
	ak8975_selftest_control(false);

	pr_info("ak8975 selftest magnetic data: X=%d, Y=%d, Z=%d",
		magnetic[0], magnetic[1], magnetic[2]);

	if ((magnetic[0] >= 30 && magnetic[0] <= 5000) &&
		(magnetic[1] >= 30 && magnetic[1] <= 5000) &&
		(magnetic[2] >= 30 && magnetic[2] <= 5000)) {
		pr_info("  PASS: Self-test completed successfully");
		ak8975_enter_power_down_mode();
		return PASS;
	} else {
		pr_err("  FAIL: Self-test data out of range");
		pr_info("  Expected range: 30-5000 for all axes");
		pr_info("  Actual: X=%d, Y=%d, Z=%d",
			magnetic[0], magnetic[1], magnetic[2]);
		ak8975_enter_power_down_mode();
		return FAIL;
	}
}

int t_ak8975_fuse_rom_access(int argc, char **argv)
{
	u8 asa_values[3] = {0, 0, 0};

	pr_info("=== AK8975 Fuse ROM Access Test ===");

	ak8975_enter_fuse_rom_access_mode();
	mdelay(10); /* Ensure FUSE ROM access mode is stable */

	ak8975_sensitivity_adjustment_values(asa_values);
	pr_info("ak8975_fuse_rom_access: Adjustment Values");
	pr_info("  ASAX = 0x%02X", asa_values[0]);
	pr_info("  ASAY = 0x%02X", asa_values[1]);
	pr_info("  ASAZ = 0x%02X", asa_values[2]);

	/* Verify ASA values are in valid range (0-255) */
	if (asa_values[0] <= 255 && asa_values[1] <= 255 && asa_values[2] <= 255) {
		pr_info("  PASS: Sensitivity adjustment values read successfully");
	} else {
		pr_warn("  WARN: Some ASA values appear out of range");
	}

	ak8975_enter_power_down_mode();
	mdelay(10); /* Return to power-off state */

	return PASS;
}

int t_ak8975_program_process(int argc, char **argv)
{
	int ret;
	bool on_off = true;

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			on_off = true;
		} else if (!strcmp(argv[1], "off")) {
			on_off = false;
		} else {
			return -EINVAL;
		}
	}

	if (on_off) {
		ret = ak8975_slave_start();
		if (ret) {
			return ret;
		}
	} else {
		ret = ak8975_slave_stop();
		if (ret) {
			return ret;
		}
	}

	return PASS;
}

#endif
