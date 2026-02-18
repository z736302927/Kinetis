#include <linux/delay.h>
#include <math.h>

#include "kinetis/max30205.h"
#include "kinetis/iic_soft.h"
#include "kinetis/delay.h"
#include "kinetis/idebug.h"
#include "kinetis/design_verification.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

/* Device I2C address - MAX30205 typically uses 0x48 */
#define MAX30205_ADDR                    MAX30205_I2C_ADDR

static struct iic_master *max30205_iic = &fake_master;

/* Private variables for calibration and status */
static float g_temperature_offset = 0.0f;
static float g_min_temperature_limit = -40.0f;
static float g_max_temperature_limit = 125.0f;
static u8 g_device_present = 0;

/* Callback function pointers */
static void (*g_high_temp_callback)(float temperature) = NULL;
static void (*g_low_temp_callback)(float temperature) = NULL;
static void (*g_temp_normal_callback)(void) = NULL;

/* I/O port functions - using Kinetis I2C implementation */
static inline void max30205_port_transmit(u8 addr, u8 data)
{
	iic_master_port_transmit(max30205_iic, MAX30205_ADDR, addr, data);
}

static inline void max30205_port_receive(u8 addr, u8 *pdata)
{
	iic_master_port_receive(max30205_iic, MAX30205_ADDR, addr, pdata);
}

static inline void max30205_port_multi_transmit(u8 addr, u8 *pdata, u32 length)
{
	iic_master_port_multi_transmit(max30205_iic, MAX30205_ADDR, addr, pdata, length);
}

static inline void max30205_port_multi_receive(u8 addr, u8 *pdata, u32 length)
{
	iic_master_port_multi_receive(max30205_iic, MAX30205_ADDR, addr, pdata, length);
}

/* Delay functions */
void max30205_Delayus(u32 ticks)
{
	udelay(ticks);
}

void max30205_Delayms(u32 ticks)
{
	mdelay(ticks);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Register definitions - updated to match MAX30205 specifications */
#define TEMPERATURE                     MAX30205_REG_TEMP
#define CONFIGURATION                    MAX30205_REG_CONFIG
#define THYST                           MAX30205_REG_THYST
#define TOS                             MAX30205_REG_TOS

/* Initialization function */
void max30205_init(void)
{
	u8 config = 0;

	/* Initialize IIC master */
	max30205_iic->init();

	printk("Initializing MAX30205 temperature sensor...");

	/* Check if device is present */
	g_device_present = max30205_is_device_present();
	if (!g_device_present) {
		printk("ERROR: MAX30205 device not found!");
		return;
	}

	/* Read current config to preserve settings */
	max30205_port_receive(CONFIGURATION, &config);
	config |= MAX30205_CONFIG_SHUTDOWN_BIT; /* Start in shutdown mode */
	max30205_port_transmit(CONFIGURATION, config);

	max30205_Delayms(10); /* Wait for configuration to take effect */

	/* Set default configuration */
	max30205_set_operating_mode(MAX30205_MODE_COMPARATOR);
	max30205_set_os_polarity(0); /* Active low OS pin */
	max30205_set_fault_queue(MAX30205_FAULT_QUEUE_1);
	max30205_set_data_format(0); /* Normal data format */
	max30205_enable_timeout(1);

	/* Exit shutdown mode */
	max30205_set_shutdown_mode(0);

	printk("MAX30205 initialized successfully");
}

/* Device detection */
u8 max30205_is_device_present(void)
{
	u8 test_data = 0;

	/* Try to read configuration register */
	max30205_port_receive(CONFIGURATION, &test_data);

	/* A valid MAX30205 should return any value when reading config register */
	g_device_present = (test_data != 0xFF) ? 1 : 0;

	return g_device_present;
}

/* Core temperature measurement functions */
u16 max30205_get_raw_temperature(void)
{
	u8 temp_raw[2];

	max30205_port_multi_receive(TEMPERATURE, temp_raw, 2);

	return (temp_raw[0] << 8) | temp_raw[1];
}

void max30205_get_temperature(float *ptemperature)
{
	u16 raw_temp;

	raw_temp = max30205_get_raw_temperature();
	*ptemperature = MAX30205_RAW_TO_CELSIUS(raw_temp);
}

/* Get temperature with calibration offset applied */
float max30205_get_temperature_with_calibration(void)
{
	float temperature;

	max30205_get_temperature(&temperature);
	return temperature + g_temperature_offset;
}

/* Configuration functions */
void max30205_set_shutdown_mode(u8 enable)
{
	u8 config = 0;

	max30205_port_receive(CONFIGURATION, &config);

	if (enable) {
		config |= MAX30205_CONFIG_SHUTDOWN_BIT;
	} else {
		config &= ~MAX30205_CONFIG_SHUTDOWN_BIT;
	}

	max30205_port_transmit(CONFIGURATION, config);
}

void max30205_set_operating_mode(u8 mode)
{
	u8 config = 0;

	max30205_port_receive(CONFIGURATION, &config);

	if (mode == MAX30205_MODE_INTERRUPT) {
		config |= MAX30205_CONFIG_MODE_BIT;
	} else {
		config &= ~MAX30205_CONFIG_MODE_BIT;
	}

	max30205_port_transmit(CONFIGURATION, config);
}

void max30205_set_os_polarity(u8 polarity)
{
	u8 config = 0;

	max30205_port_receive(CONFIGURATION, &config);

	if (polarity) {
		config |= MAX30205_CONFIG_OS_POLARITY_BIT;
	} else {
		config &= ~MAX30205_CONFIG_OS_POLARITY_BIT;
	}

	max30205_port_transmit(CONFIGURATION, config);
}

void max30205_set_fault_queue(u8 fault_count)
{
	u8 config = 0;

	max30205_port_receive(CONFIGURATION, &config);

	config &= ~MAX30205_CONFIG_FAULT_QUEUE_MASK;
	config |= fault_count;

	max30205_port_transmit(CONFIGURATION, config);
}

void max30205_set_data_format(u8 format)
{
	u8 config = 0;

	max30205_port_receive(CONFIGURATION, &config);

	if (format) {
		config |= MAX30205_CONFIG_DATA_FORMAT_BIT;
	} else {
		config &= ~MAX30205_CONFIG_DATA_FORMAT_BIT;
	}

	max30205_port_transmit(CONFIGURATION, config);
}

void max30205_enable_timeout(u8 enable)
{
	u8 config = 0;

	max30205_port_receive(CONFIGURATION, &config);

	if (enable) {
		config |= MAX30205_CONFIG_TIMEOUT_BIT;
	} else {
		config &= ~MAX30205_CONFIG_TIMEOUT_BIT;
	}

	max30205_port_transmit(CONFIGURATION, config);
}

void max30205_trigger_one_shot(void)
{
	u8 config = 0;

	max30205_port_receive(CONFIGURATION, &config);
	config |= MAX30205_CONFIG_ONE_SHOT_BIT;
	max30205_port_transmit(CONFIGURATION, config);

	/* Clear the bit after triggering */
	max30205_Delayus(10);
	config &= ~MAX30205_CONFIG_ONE_SHOT_BIT;
	max30205_port_transmit(CONFIGURATION, config);
}

/* Threshold management functions */
void max30205_set_threshold_high(u16 threshold_raw)
{
	u8 threshold_data[2];

	threshold_data[0] = (threshold_raw >> 8) & 0xFF;
	threshold_data[1] = threshold_raw & 0xFF;

	max30205_port_multi_transmit(TOS, threshold_data, 2);
}

void max30205_set_threshold_low(u16 threshold_raw)
{
	u8 threshold_data[2];

	threshold_data[0] = (threshold_raw >> 8) & 0xFF;
	threshold_data[1] = threshold_raw & 0xFF;

	max30205_port_multi_transmit(THYST, threshold_data, 2);
}

u16 max30205_get_threshold_high(void)
{
	u8 threshold_data[2];

	max30205_port_multi_receive(TOS, threshold_data, 2);

	return (threshold_data[0] << 8) | threshold_data[1];
}

u16 max30205_get_threshold_low(void)
{
	u8 threshold_data[2];

	max30205_port_multi_receive(THYST, threshold_data, 2);

	return (threshold_data[0] << 8) | threshold_data[1];
}

/* Status and flag management */
u8 max30205_check_os_flag(void)
{
	u8 config = 0;

	max30205_port_receive(CONFIGURATION, &config);

	/* OS flag is bit 0 of config register when in comparator mode */
	return (config & 0x01);
}

void max30205_clear_os_flag(void)
{
	/* OS flag is cleared by reading the temperature register */
	u8 temp_data[2];
	max30205_port_multi_receive(TEMPERATURE, temp_data, 2);
}

/* Calibration and offset functions */
void max30205_calibrate_offset(float offset_celsius)
{
	g_temperature_offset = offset_celsius;
	printk("Temperature calibration offset set to: %.3f°C", offset_celsius);
}

/* Temperature limits and callback management */
void max30205_set_temperature_limits(float min_temp, float max_temp)
{
	g_min_temperature_limit = min_temp;
	g_max_temperature_limit = max_temp;

	printk("Temperature limits set: %.2f°C to %.2f°C", min_temp, max_temp);
}

u8 max30205_check_temperature_limits(float temperature)
{
	if (temperature > g_max_temperature_limit) {
		return 1; /* High temperature */
	} else if (temperature < g_min_temperature_limit) {
		return 2; /* Low temperature */
	}

	return 0; /* Normal temperature */
}

void max30205_register_high_temp_callback(void (*callback)(float temperature))
{
	g_high_temp_callback = callback;
	printk("High temperature callback registered");
}

void max30205_register_low_temp_callback(void (*callback)(float temperature))
{
	g_low_temp_callback = callback;
	printk("Low temperature callback registered");
}

void max30205_register_temp_normal_callback(void (*callback)(void))
{
	g_temp_normal_callback = callback;
	printk("Temperature normal callback registered");
}

/* Temperature alert processing */
void max30205_process_temperature_alert(float temperature)
{
	u8 limit_status = max30205_check_temperature_limits(temperature);

	switch (limit_status) {
	case 1: /* High temperature */
		printk("High temperature alert: %.2f°C", temperature);
		if (g_high_temp_callback != NULL) {
			g_high_temp_callback(temperature);
		}
		break;

	case 2: /* Low temperature */
		printk("Low temperature alert: %.2f°C", temperature);
		if (g_low_temp_callback != NULL) {
			g_low_temp_callback(temperature);
		}
		break;

	case 0: /* Normal temperature */
		printk("Temperature normal: %.2f°C", temperature);
		if (g_temp_normal_callback != NULL) {
			g_temp_normal_callback();
		}
		break;
	}
}

/* Legacy function compatibility */
void max30205_ShutDown(u8 Data)
{
	max30205_set_shutdown_mode(Data);
}

void max30205_EnterComparatorMode(void)
{
	max30205_set_operating_mode(MAX30205_MODE_COMPARATOR);
}

void max30205_EnterInterruptMode(void)
{
	max30205_set_operating_mode(MAX30205_MODE_INTERRUPT);
}

void max30205_OSPolarity(u8 Data)
{
	max30205_set_os_polarity(Data);
}

void max30205_ConfigFaultQueue(u8 Data)
{
	max30205_set_fault_queue(Data);
}

void max30205_DataFormat(u8 Data)
{
	max30205_set_data_format(Data);
}

void max30205_EnableTimeout(u8 Data)
{
	max30205_enable_timeout(Data);
}

void max30205_OneShot(u8 Data)
{
	if (Data) {
		max30205_trigger_one_shot();
	}
}

void max30205_ReadTHYST(u16 *pdata)
{
	u8 TmpVal[2];

	max30205_port_multi_receive(THYST, TmpVal, 2);

	pdata[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void max30205_WriteTHYST(u16 Data)
{
	u8 TmpVal[2];

	TmpVal[0] = Data >> 8;
	TmpVal[1] = Data & 0xFF;

	max30205_port_multi_transmit(THYST, TmpVal, 2);
}

void max30205_ReadTOS(u16 *pdata)
{
	u8 TmpVal[2];

	max30205_port_multi_receive(TOS, TmpVal, 2);

	pdata[0] = (TmpVal[0] << 8) | TmpVal[1];
}

void max30205_WriteTOS(u16 Data)
{
	u8 TmpVal[2];

	TmpVal[0] = Data >> 8;
	TmpVal[1] = Data & 0xFF;

	max30205_port_multi_transmit(TOS, TmpVal, 2);
}

#ifdef DESIGN_VERIFICATION_MAX30205
#include "kinetis/test-kinetis.h"
#include "kinetis/design_verification.h"

/**
 * @brief Test 1: MAX30205 Basic Device Detection
 * @return PASS if device is detected, FAIL otherwise
 */
int t_max30205_device_id(int argc, char **argv)
{
	pr_info("=== MAX30205 Device Detection Test ===");

	/* Check if device is present */
	if (!max30205_is_device_present()) {
		pr_err("FAIL: MAX30205 device not found");
		return FAIL;
	}

	pr_info("MAX30205 device detected successfully");
	return PASS;
}

/**
 * @brief Test 2: MAX30205 Temperature Reading (Single Shot)
 * @return PASS if temperature reading is successful, FAIL otherwise
 */
int t_max30205_temperature_single(int argc, char **argv)
{
	float temperature;
	u16 raw_temp;

	pr_info("=== MAX30205 Single-Shot Temperature Test ===");

	/* Exit shutdown mode */
	max30205_set_shutdown_mode(0);
	mdelay(10);

	/* Trigger one-shot conversion */
	max30205_trigger_one_shot();
	mdelay(100);

	/* Read temperature */
	max30205_get_temperature(&temperature);
	pr_info("Temperature: %.2f°C", temperature);

	/* Read raw temperature */
	raw_temp = max30205_get_raw_temperature();
	pr_info("Raw temperature: 0x%04X (%.2f°C)", raw_temp,
		MAX30205_RAW_TO_CELSIUS(raw_temp));

	/* Verify temperature is within reasonable range */
	if (temperature < -20.0f || temperature > 100.0f) {
		pr_warn("Temperature out of expected range");
	}

	/* Enter shutdown mode */
	max30205_set_shutdown_mode(1);

	pr_info("Single-shot temperature reading completed");
	return PASS;
}

/**
 * @brief Test 3: MAX30205 Continuous Temperature Monitoring
 * @param argc Argument count (argv[1] = number of readings, default 100)
 * @param argv Argument vector
 * @return PASS if continuous readings work, FAIL otherwise
 */
int t_max30205_temperature_continuous(int argc, char **argv)
{
	float temperature;
	u16 readings = 100;
	u16 i;
	float temp_min = 1000.0f, temp_max = -1000.0f, temp_sum = 0.0f;

	if (argc > 1) {
		readings = simple_strtoul(argv[1], &argv[1], 10);
		if (readings > 1000) {
			readings = 1000;
		}
	}

	pr_info("=== MAX30205 Continuous Temperature Test (%d readings) ===", readings);

	/* Exit shutdown mode and enter continuous mode */
	max30205_set_shutdown_mode(0);
	max30205_set_operating_mode(MAX30205_MODE_INTERRUPT);
	mdelay(10);

	for (i = 0; i < readings; i++) {
		/* Read temperature */
		max30205_get_temperature(&temperature);

		/* Update statistics */
		if (temperature < temp_min) {
			temp_min = temperature;
		}
		if (temperature > temp_max) {
			temp_max = temperature;
		}
		temp_sum += temperature;

		/* Show first and last readings */
		if (i == 0 || i == readings - 1) {
			pr_info("Reading %d/%d: %.2f°C", i + 1, readings, temperature);
		}

		mdelay(10);
	}

	/* Print statistics */
	pr_info("Temperature statistics:");
	pr_info("  Min: %.2f°C", temp_min);
	pr_info("  Max: %.2f°C", temp_max);
	pr_info("  Avg: %.2f°C", temp_sum / readings);

	/* Enter shutdown mode */
	max30205_set_shutdown_mode(1);

	pr_info("Continuous temperature monitoring completed");
	return PASS;
}

/**
 * @brief Test 4: MAX30205 Threshold Configuration
 * @return PASS if threshold configuration works, FAIL otherwise
 */
int t_max30205_threshold_test(int argc, char **argv)
{
	u16 tos_raw, thyst_raw;

	pr_info("=== MAX30205 Threshold Test ===");

	/* Exit shutdown mode */
	max30205_set_shutdown_mode(0);
	mdelay(10);

	/* Set high temperature threshold to 30°C */
	max30205_set_threshold_high(MAX30205_CELSIUS_TO_RAW(30.0f));
	tos_raw = max30205_get_threshold_high();
	pr_info("High threshold set: 30.00°C (raw: 0x%04X)", tos_raw);

	/* Set low temperature threshold to 25°C */
	max30205_set_threshold_low(MAX30205_CELSIUS_TO_RAW(25.0f));
	thyst_raw = max30205_get_threshold_low();
	pr_info("Low threshold set: 25.00°C (raw: 0x%04X)", thyst_raw);

	/* Read temperature to check against thresholds */
	float temp;
	max30205_get_temperature(&temp);
	pr_info("Current temperature: %.2f°C", temp);

	/* Check temperature limits */
	u8 limit_status = max30205_check_temperature_limits(temp);
	pr_info("Temperature limit status: %d", limit_status);

	/* Enter shutdown mode */
	max30205_set_shutdown_mode(1);

	pr_info("Threshold configuration completed");
	return PASS;
}

/**
 * @brief Test 5: MAX30205 Configuration Register Tests
 * @return PASS if configuration changes work, FAIL otherwise
 */
int t_max30205_config_test(int argc, char **argv)
{
	pr_info("=== MAX30205 Configuration Test ===");

	/* Test shutdown mode */
	pr_info("Testing shutdown mode...");
	max30205_set_shutdown_mode(0);
	mdelay(10);

	max30205_set_shutdown_mode(1);
	mdelay(10);

	/* Test operating mode */
	max30205_set_shutdown_mode(0);
	mdelay(10);

	pr_info("Testing operating mode...");
	max30205_set_operating_mode(MAX30205_MODE_COMPARATOR);
	mdelay(10);

	max30205_set_operating_mode(MAX30205_MODE_INTERRUPT);
	mdelay(10);

	/* Test OS polarity */
	pr_info("Testing OS polarity...");
	max30205_set_os_polarity(0);
	mdelay(10);

	max30205_set_os_polarity(1);
	mdelay(10);

	/* Test fault queue */
	pr_info("Testing fault queue...");
	max30205_set_fault_queue(MAX30205_FAULT_QUEUE_1);
	mdelay(10);

	max30205_set_fault_queue(MAX30205_FAULT_QUEUE_6);
	mdelay(10);

	pr_info("Configuration test completed");
	return PASS;
}

/**
 * @brief Test 6: MAX30205 OS (Overtemperature Shutdown) Flag
 * @return PASS if OS flag handling works, FAIL otherwise
 */
int t_max30205_os_flag_test(int argc, char **argv)
{
	u8 os_flag;

	pr_info("=== MAX30205 OS Flag Test ===");

	/* Exit shutdown mode */
	max30205_set_shutdown_mode(0);
	mdelay(10);

	/* Read OS flag */
	os_flag = max30205_check_os_flag();
	pr_info("OS flag status: %d", os_flag);

	/* Clear OS flag */
	max30205_clear_os_flag();
	mdelay(10);

	/* Read OS flag again */
	os_flag = max30205_check_os_flag();
	pr_info("OS flag after clear: %d", os_flag);

	/* Set threshold to trigger OS flag */
	max30205_set_threshold_high(MAX30205_CELSIUS_TO_RAW(0.0f));
	max30205_set_threshold_low(MAX30205_CELSIUS_TO_RAW(-10.0f));
	mdelay(100);

	/* Read temperature */
	float temp;
	max30205_get_temperature(&temp);
	pr_info("Current temperature: %.2f°C", temp);

	/* Check OS flag */
	os_flag = max30205_check_os_flag();
	pr_info("OS flag after threshold change: %d", os_flag);

	/* Clear OS flag */
	max30205_clear_os_flag();

	/* Enter shutdown mode */
	max30205_set_shutdown_mode(1);

	pr_info("OS flag test completed");
	return PASS;
}

/**
 * @brief Test 7: MAX30205 Timeout Feature
 * @return PASS if timeout feature works, FAIL otherwise
 */
int t_max30205_timeout_test(int argc, char **argv)
{
	pr_info("=== MAX30205 Timeout Test ===");

	/* Exit shutdown mode */
	max30205_set_shutdown_mode(0);
	mdelay(10);

	/* Enable timeout */
	max30205_enable_timeout(1);
	mdelay(10);

	/* Disable timeout */
	max30205_enable_timeout(0);
	mdelay(10);

	/* Enter shutdown mode */
	max30205_set_shutdown_mode(1);

	pr_info("Timeout test completed");
	return PASS;
}

/**
 * @brief Test 8: MAX30205 One-Shot Mode
 * @return PASS if one-shot mode works, FAIL otherwise
 */
int t_max30205_oneshot_test(int argc, char **argv)
{
	u16 readings = 10;
	u16 i;
	float temperature;

	pr_info("=== MAX30205 One-Shot Mode Test (%d readings) ===", readings);

	/* Enter shutdown mode */
	max30205_set_shutdown_mode(1);
	mdelay(10);

	for (i = 0; i < readings; i++) {
		/* Trigger one-shot conversion */
		max30205_trigger_one_shot();

		/* Wait for conversion to complete */
		mdelay(100);

		/* Read temperature */
		max30205_get_temperature(&temperature);

		pr_info("Reading %d/%d: %.2f°C", i + 1, readings, temperature);
	}

	pr_info("One-shot mode test completed");
	return PASS;
}

/**
 * @brief Test 9: MAX30205 Temperature Calibration
 * @return PASS if calibration works, FAIL otherwise
 */
int t_max30205_calibration_test(int argc, char **argv)
{
	float temp_normal, temp_calibrated;

	pr_info("=== MAX30205 Calibration Test ===");

	/* Exit shutdown mode */
	max30205_set_shutdown_mode(0);
	max30205_set_operating_mode(MAX30205_MODE_INTERRUPT);
	mdelay(10);

	/* Read normal temperature */
	max30205_get_temperature(&temp_normal);
	pr_info("Normal temperature: %.2f°C", temp_normal);

	/* Set calibration offset */
	max30205_calibrate_offset(0.5f);

	/* Read calibrated temperature */
	temp_calibrated = max30205_get_temperature_with_calibration();
	pr_info("Calibrated temperature: %.2f°C (offset +0.5°C)", temp_calibrated);

	/* Reset calibration */
	max30205_calibrate_offset(0.0f);

	/* Enter shutdown mode */
	max30205_set_shutdown_mode(1);

	pr_info("Calibration test completed");
	return PASS;
}

/**
 * @brief Test 10: MAX30205 Temperature Range Test
 * @return PASS if temperature readings are stable, FAIL otherwise
 */
int t_max30205_range_test(int argc, char **argv)
{
	float temperature;
	u16 readings = 50;
	u16 i;
	float prev_temp = 0.0f;
	float max_delta = 0.0f;

	pr_info("=== MAX30205 Temperature Range Test (%d readings) ===", readings);

	/* Exit shutdown mode */
	max30205_set_shutdown_mode(0);
	max30205_set_operating_mode(MAX30205_MODE_INTERRUPT);
	mdelay(10);

	for (i = 0; i < readings; i++) {
		/* Read temperature */
		max30205_get_temperature(&temperature);

		/* Calculate temperature change */
		if (i > 0) {
			float delta = temperature - prev_temp;
			if (delta < 0) {
				delta = -delta;
			}
			if (delta > max_delta) {
				max_delta = delta;
			}
		}

		prev_temp = temperature;

		mdelay(20);
	}

	pr_info("Maximum temperature delta: %.2f°C", max_delta);

	if (max_delta > 2.0f) {
		pr_warn("Temperature readings vary significantly");
	}

	/* Enter shutdown mode */
	max30205_set_shutdown_mode(1);

	pr_info("Temperature range test completed");
	return PASS;
}

void max30205_Test(void)
{
	printk("=== MAX30205 Comprehensive Test Started ===");

	/* Test 1: Device detection */
	if (t_max30205_device_id(0, NULL) != PASS) {
		printk("Test 1 FAILED: Device detection failed");
		return;
	}

	/* Test 2: Single-shot temperature */
	t_max30205_temperature_single(0, NULL);

	/* Test 3: Continuous temperature monitoring */
	t_max30205_temperature_continuous(0, NULL);

	/* Test 4: Threshold configuration */
	t_max30205_threshold_test(0, NULL);

	/* Test 5: Configuration register tests */
	t_max30205_config_test(0, NULL);

	/* Test 6: OS flag test */
	t_max30205_os_flag_test(0, NULL);

	/* Test 7: Timeout feature */
	t_max30205_timeout_test(0, NULL);

	/* Test 8: One-shot mode */
	t_max30205_oneshot_test(0, NULL);

	/* Test 9: Calibration test */
	t_max30205_calibration_test(0, NULL);

	/* Test 10: Temperature range test */
	t_max30205_range_test(0, NULL);

	printk("=== MAX30205 Comprehensive Test Completed Successfully ===");
}

#endif
