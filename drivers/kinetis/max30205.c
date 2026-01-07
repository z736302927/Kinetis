#include <linux/delay.h>
#include <math.h>

#include "kinetis/max30205.h"
#include "kinetis/iic_soft.h"
#include "kinetis/delay.h"
#include "kinetis/idebug.h"

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
static inline void max30205_port_transmmit(u8 addr, u8 data)
{
	iic_master_port_transmmit(IIC_SW_1, MAX30205_ADDR, addr, data);
}

static inline void max30205_port_receive(u8 addr, u8 *pdata)
{
	iic_master_port_receive(IIC_SW_1, MAX30205_ADDR, addr, pdata);
}

static inline void max30205_port_multi_transmmit(u8 addr, u8 *pdata, u32 length)
{
	iic_master_port_multi_transmmit(IIC_SW_1, MAX30205_ADDR, addr, pdata, length);
}

static inline void max30205_port_multi_receive(u8 addr, u8 *pdata, u32 length)
{
	iic_master_port_multi_receive(IIC_SW_1, MAX30205_ADDR, addr, pdata, length);
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
	max30205_port_transmmit(CONFIGURATION, config);

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

	max30205_port_transmmit(CONFIGURATION, config);
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

	max30205_port_transmmit(CONFIGURATION, config);
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

	max30205_port_transmmit(CONFIGURATION, config);
}

void max30205_set_fault_queue(u8 fault_count)
{
	u8 config = 0;

	max30205_port_receive(CONFIGURATION, &config);

	config &= ~MAX30205_CONFIG_FAULT_QUEUE_MASK;
	config |= fault_count;

	max30205_port_transmmit(CONFIGURATION, config);
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

	max30205_port_transmmit(CONFIGURATION, config);
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

	max30205_port_transmmit(CONFIGURATION, config);
}

void max30205_trigger_one_shot(void)
{
	u8 config = 0;

	max30205_port_receive(CONFIGURATION, &config);
	config |= MAX30205_CONFIG_ONE_SHOT_BIT;
	max30205_port_transmmit(CONFIGURATION, config);

	/* Clear the bit after triggering */
	max30205_Delayus(10);
	config &= ~MAX30205_CONFIG_ONE_SHOT_BIT;
	max30205_port_transmmit(CONFIGURATION, config);
}

/* Threshold management functions */
void max30205_set_threshold_high(u16 threshold_raw)
{
	u8 threshold_data[2];

	threshold_data[0] = (threshold_raw >> 8) & 0xFF;
	threshold_data[1] = threshold_raw & 0xFF;

	max30205_port_multi_transmmit(TOS, threshold_data, 2);
}

void max30205_set_threshold_low(u16 threshold_raw)
{
	u8 threshold_data[2];

	threshold_data[0] = (threshold_raw >> 8) & 0xFF;
	threshold_data[1] = threshold_raw & 0xFF;

	max30205_port_multi_transmmit(THYST, threshold_data, 2);
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

	max30205_port_multi_transmmit(THYST, TmpVal, 2);
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

	max30205_port_multi_transmmit(TOS, TmpVal, 2);
}

#ifdef DESIGN_VERIFICATION_MAX30205
static u8 tx_buffer[256];
static u8 rx_buffer[256];

void max30205_Test(void)
{
	u32 TmpRngdata = 0;
	u16 BufferLength = 0;
	u32 TestAddr = 0;

	random_get8bit(&hrng, &TmpRngdata);
	BufferLength = TmpRngdata & 0xFF;
	printk("BufferLength = %d.", BufferLength);

	if (tx_buffer == NULL || rx_buffer == NULL) {
		printk("Failed to allocate memory !");
		return;
	}

	memset(tx_buffer, 0, BufferLength);
	memset(rx_buffer, 0, BufferLength);

	random_get8bit(&hrng, &TmpRngdata);
	TestAddr = TmpRngdata & 0xFF;
	printk("TestAddr = 0x%02X.", TestAddr);

	for (u16 i = 0; i < BufferLength; i += 4) {
		random_get8bit(&hrng, &TmpRngdata);
		tx_buffer[i + 3] = (TmpRngdata & 0xFF000000) >> 24;;
		tx_buffer[i + 2] = (TmpRngdata & 0x00FF0000) >> 16;
		tx_buffer[i + 1] = (TmpRngdata & 0x0000FF00) >> 8;
		tx_buffer[i + 0] = (TmpRngdata & 0x000000FF);
	}

	max30205_WriteData(TestAddr, tx_buffer, BufferLength);
	max30205_ReadData(TestAddr, rx_buffer, BufferLength);

	for (u16 i = 0; i < BufferLength; i++) {
		if (tx_buffer[i] != rx_buffer[i]) {
			printk("tx_buffer[%d] = 0x%02X, rx_buffer[%d] = 0x%02X",
				i, tx_buffer[i],
				i, rx_buffer[i]);
			printk("Data writes and reads do not match, TEST FAILED !");
			return ;
		}
	}

	printk("max30205 Read and write TEST PASSED !");
}

#endif
