
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/sched_clock.h>

#include <kinetis/iic_soft.h>
#include <kinetis/spi_soft.h>
#include <kinetis/real-time-clock.h>
#include <kinetis/basic-timer.h>
#include <kinetis/serial-port.h>
#include <kinetis/bootloader.h>
// #include <kinetis/w25qxxx.h>

#include "hall.h"
#include "rotor.h"

#if KINETIS_FAKE_SIM == 0
#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "rtc.h"
#include "tim.h"
#if defined(STM32H743xx)
#include "sdmmc.h"
#include "quadspi.h"
#elif defined(STM32F407xx)
#include "sdio.h"
#endif
#include "dma.h"

#include "stdio.h"

struct transfer_complete_result {
	volatile u8 tx_done;
	volatile u8 rx_done;
	volatile u8 error;
};

u32 hall_read_rotated_time(struct hall_device *dev)
{
	return __HAL_TIM_GET_COUNTER(&htim2);
}

void arm_dwt_init(void)
{
	/* Enable DWT trace unit (TRCENA) */
	writel(readl(&CoreDebug->DEMCR) | CoreDebug_DEMCR_TRCENA_Msk, &CoreDebug->DEMCR);
	/* Reset cycle counter */
	writel(0, &DWT->CYCCNT);
	/* Enable cycle counter (CYCCNTENA) */
	writel(readl(&DWT->CTRL) | DWT_CTRL_CYCCNTENA_Msk, &DWT->CTRL);
}

u64 arm_dwt_read(struct clocksource *cs)
{
	return (u64)DWT->CYCCNT;
}

unsigned long read_chip_timer(void)
{
	return TIM5->CNT;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM4) {
		xtime_update(1);
		update_sched_clock();
	} else if (htim->Instance == TIM5) {
		basic_timer_isr(1000000);
	}
}

struct hall_device *hall_dev;

u8 pov_read_arm_id(void)
{
	u8 id = 0;

	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET) {
		id |= 1;
	}
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET) {
		id |= 2;
	}
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_SET) {
		id |= 4;
	}
	if (id >= POV_ARM_COUNT) {
		pr_err("arm-id %u out of range (max %u)\n", id, POV_ARM_COUNT - 1);
		return 0;
	}

	return id;
}

void pov_sync_zero_pulse_output(void)
{
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_3, GPIO_PIN_SET);
	udelay(1);
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_3, GPIO_PIN_RESET);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM2) {
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
			hall_hall_isr(hall_dev, HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1));
			pov_sync_zero_pulse_output();
		}
	}
}

static void led_hal_set(u8 red, u8 green, u8 blue)
{
	if (red == 255) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
	} else if (red == 0) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
	}
}

void hall_fake_pulse(struct tim_task *task)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
}

void chip_rtc_backup_reg_write()
{
	/* Writes a data in a RTC Backup data Register1 */
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);
}

void chip_rtc_backup_reg_read(u32 *tmp)
{
	/* Read a data in a RTC Backup data Register1 */
	*tmp = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
}

void chip_rtc_calendar_set(struct tm *rtc, u8 format)
{
	RTC_DateTypeDef sdate;
	RTC_TimeTypeDef stime;

	/* Set Date: Wednesday May 1th 2019 */
	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);
	}

	sdate.Year = rtc->tm_year;
	sdate.Month = rtc->tm_mon;
	sdate.Date = rtc->tm_mday;

	if (rtc->tm_wday != 0) {
		sdate.WeekDay = rtc->tm_wday;
	}

	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_SetDate(&hrtc, &sdate, RTC_FORMAT_BCD);
	}

	/* Set Time: 00:00:00 */
	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);
	}

	stime.Hours = rtc->tm_hour;
	stime.Minutes = rtc->tm_min;
	stime.Seconds = rtc->tm_sec;
	stime.TimeFormat = RTC_HOURFORMAT12_AM;
	stime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE ;
	stime.StoreOperation = RTC_STOREOPERATION_RESET;

	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_SetTime(&hrtc, &stime, RTC_FORMAT_BCD);
	}
}

void chip_rtc_calendar_get(struct tm *rtc, u8 format)
{
	RTC_DateTypeDef sdate;
	RTC_TimeTypeDef stime;

	/* Get the RTC current Date */
	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BCD);
	}

	rtc->tm_year = sdate.Year;
	rtc->tm_mon = sdate.Month;
	rtc->tm_mday = sdate.Date;

	if (rtc->tm_wday != 0) {
		rtc->tm_wday = sdate.WeekDay;
	}

	/* Get the RTC current Time */
	if (format == KRTC_FORMAT_BIN) {
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	} else {
		HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BCD);
	}

	rtc->tm_hour = stime.Hours;
	rtc->tm_min = stime.Minutes;
	rtc->tm_sec = stime.Seconds;
}

void chip_rtc_set_time_format(u8 tmp)
{
	/* Placeholder for chip RTC time format setting */
}

u8 chip_rtc_get_time_format(void)
{
	return 0;
}

struct rtc_device general_rtc = {
	.backup_reg_write = chip_rtc_backup_reg_write,
	.backup_reg_read = chip_rtc_backup_reg_read,
	.calendar_set = chip_rtc_calendar_set,
	.calendar_get = chip_rtc_calendar_get,
	.set_time_format = chip_rtc_set_time_format,
	.get_time_format = chip_rtc_get_time_format,
};

static struct transfer_complete_result stm32_i2c_result[2];

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == hi2c1.Instance) {
		stm32_i2c_result[0].tx_done = 1;
	}
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == hi2c1.Instance) {
		stm32_i2c_result[0].rx_done = 1;
	}
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == hi2c1.Instance) {
		pr_err("stm32_i2c: DMA error 0x%lx\n", hi2c->ErrorCode);
		stm32_i2c_result[0].error = 1;
	}
}

static struct transfer_complete_result stm32_spi_result[4] = {
	{ .tx_done = 1, .rx_done = 0, .error = 0 },
	{ .tx_done = 1, .rx_done = 0, .error = 0 },
	{ .tx_done = 1, .rx_done = 0, .error = 0 },
	{ .tx_done = 1, .rx_done = 0, .error = 0 },
};

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == hspi1.Instance) {
		stm32_spi_result[0].tx_done = 1;
	}
#if POV_RUN_MODE == POV_RUN_HOST
	else if (hspi->Instance == hspi2.Instance) {
		stm32_spi_result[1].tx_done = 1;
	} else if (hspi->Instance == hspi3.Instance) {
		stm32_spi_result[2].tx_done = 1;
	} else if (hspi->Instance == hspi4.Instance) {
		stm32_spi_result[3].tx_done = 1;
	}
#endif
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == hspi1.Instance) {
		stm32_spi_result[0].rx_done = 1;
	}
#if POV_RUN_MODE == POV_RUN_HOST
	else if (hspi->Instance == hspi2.Instance) {
		stm32_spi_result[1].rx_done = 1;
	} else if (hspi->Instance == hspi3.Instance) {
		stm32_spi_result[2].rx_done = 1;
	} else if (hspi->Instance == hspi4.Instance) {
		stm32_spi_result[3].rx_done = 1;
	}
#endif
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == hspi1.Instance) {
		pr_err("stm32_spi: DMA error 0x%lx\n", hspi->ErrorCode);
		stm32_spi_result[0].error = 1;
#if POV_RUN_MODE == POV_RUN_HOST
	} else if (hspi->Instance == hspi2.Instance) {
		pr_err("stm32_spi: DMA error 0x%lx\n", hspi->ErrorCode);
		stm32_spi_result[1].error = 1;
	} else if (hspi->Instance == hspi3.Instance) {
		pr_err("stm32_spi: DMA error 0x%lx\n", hspi->ErrorCode);
		stm32_spi_result[2].error = 1;
	} else if (hspi->Instance == hspi4.Instance) {
		pr_err("stm32_spi: DMA error 0x%lx\n", hspi->ErrorCode);
		stm32_spi_result[3].error = 1;
#endif
	}
}

static struct transfer_complete_result stm32_uart_result[5];

struct serial_port *stm32_serial_port[5];

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == huart1.Instance) {
		stm32_uart_result[0].tx_done = 1;
	} else if (huart->Instance == huart2.Instance) {
		stm32_uart_result[1].tx_done = 1;
	}
#if POV_RUN_MODE == POV_RUN_HOST
	else if (huart->Instance == huart4.Instance) {
		stm32_uart_result[3].tx_done = 1;
	} else if (huart->Instance == huart5.Instance) {
		stm32_uart_result[4].tx_done = 1;
	} else if (huart->Instance == huart6.Instance) {
		stm32_uart_result[2].tx_done = 1;
	}
#endif
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, u16 Size)
{
	struct serial_port *serial;
	const char *name;

	if (huart->Instance == huart2.Instance) {
		serial = stm32_serial_port[1];
		name = "uart2";
	}
#if POV_RUN_MODE == POV_RUN_HOST
	else if (huart->Instance == huart6.Instance) {
		serial = stm32_serial_port[2];
		name = "uart6";
	} else if (huart->Instance == huart4.Instance) {
		serial = stm32_serial_port[3];
		name = "uart4";
	} else if (huart->Instance == huart5.Instance) {
		serial = stm32_serial_port[4];
		name = "uart5";
	}
#endif
	else {
		return;
	}

	serial->producer = Size % SERIAL_PORT_BUFFER_SIZE;

	if (serial_port_rx_used(serial) > SERIAL_PORT_BUFFER_SIZE * 95 / 100) {
		serial->ops->disable();
		serial->overflow = 1;
		pr_warn("%s: RX overflow protection, RX auto-disabled\n", name);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	UART_HandleTypeDef *uart;
	struct serial_port *serial;
	const char *name;
	u8 result_idx = 0;

	if (huart->Instance == huart2.Instance) {
		uart = &huart2;
		serial = stm32_serial_port[1];
		name = "uart2";
		result_idx = 1;
	}
#if POV_RUN_MODE == POV_RUN_HOST
	else if (huart->Instance == huart6.Instance) {
		uart = &huart6;
		serial = stm32_serial_port[2];
		name = "uart6";
		result_idx = 2;
	} else if (huart->Instance == huart4.Instance) {
		uart = &huart4;
		serial = stm32_serial_port[3];
		name = "uart4";
		result_idx = 3;
	} else if (huart->Instance == huart5.Instance) {
		uart = &huart5;
		serial = stm32_serial_port[4];
		name = "uart5";
		result_idx = 4;
	}
#endif
	else {
		return;
	}

	if (!(uart->Instance->CR1 & USART_CR1_RE)) {
		pr_debug("%s: error 0x%lx while RX disabled, not restarting\n", name, huart->ErrorCode);
	} else {
		pr_err("%s: DMA error 0x%lx, restarting RX\n", name, huart->ErrorCode);
		HAL_UART_AbortReceive(uart);
		HAL_UART_Init(uart);
		HAL_UARTEx_ReceiveToIdle_DMA(uart, serial->rx_buffer,
			SERIAL_PORT_BUFFER_SIZE);
		__HAL_DMA_DISABLE_IT(uart->hdmarx, DMA_IT_TC | DMA_IT_HT);
		// ATOMIC_CLEAR_BIT(uart->Instance->CR3, USART_CR3_EIE);
	}

	stm32_uart_result[result_idx].error = 1;
}

struct transfer_complete_result sd_dma_result;

#if defined(STM32H743xx)
#define SD_INST                                hsd1.Instance
#elif defined(STM32F407xx)
#define SD_INST                                hsd->Instance
#endif

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
	if (hsd->Instance == SD_INST) {
		sd_dma_result.tx_done = 1;
	}
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
	if (hsd->Instance == SD_INST) {
		sd_dma_result.rx_done = 1;
	}
}

void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
	if (hsd->Instance == SD_INST) {
		pr_err("sd_dma: DMA error 0x%lx\n", hsd->ErrorCode);
		sd_dma_result.error = 1;
	}
}

int fputc(int ch, FILE *f)
{
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART3 and Loop until the end of transmission */
	if (HAL_UART_Transmit(&huart1, (u8 *)&ch, 1, 100) != HAL_OK) {
		return -1;
	}

	return ch;
}

int _putc(int ch)
{
	if (HAL_UART_Transmit(&huart1, (u8 *)&ch, 1, 100) != HAL_OK) {
		return -1;
	}

	return ch;
}

int _puts(char *string, int cnt)
{
	u32 val;
	int ret;

	/* Output cnt characters from string to console */
	if (!string || cnt <= 0) {
		return 0;
	}

	// 	stm32_uart_result[0].tx_done = 0;
	// 	stm32_uart_result[0].error = 0;

	if (HAL_UART_Transmit(&huart1, (u8 *)string, cnt, 1000) != HAL_OK) {
		stm32_uart_result[0].error = 1;
		return -1;
	}

	// 	ret = readl_poll_timeout(&stm32_uart_result[0].tx_done, val,
	// 				 val || stm32_uart_result[0].error, 100, 100000);
	// 	if (ret < 0)
	// 		return -1;

	return cnt;
}

static void iic1_scl_low()
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
}

static void iic1_scl_high()
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
}

static void iic1_sda_low()
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
}

static void iic1_sda_high()
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
}

static void iic1_sda_in()
{
	GPIO_InitTypeDef gpio = {
		.Pull = GPIO_PULLUP,
		.Speed = GPIO_SPEED_FREQ_VERY_HIGH
	};
	/*
	 * It doesn't have to switch direction in
	 * open drain mode.
	 */
	gpio.Pin = GPIO_PIN_9;
	gpio.Mode = GPIO_MODE_INPUT;
	HAL_GPIO_Init(GPIOB, &gpio);
}

static void iic1_sda_out()
{
	GPIO_InitTypeDef gpio = {
		.Pull = GPIO_PULLUP,
		.Speed = GPIO_SPEED_FREQ_VERY_HIGH
	};
	/*
	 * It doesn't have to switch direction in
	 * open drain mode.
	 */
	gpio.Pin = GPIO_PIN_9;
	gpio.Mode = GPIO_MODE_OUTPUT_OD;
	HAL_GPIO_Init(GPIOB, &gpio);
}

static int iic1_sda_read()
{
	return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9);
}

static int iic1_master_init()
{

	return 0;
}

static int iic1_master_transmit(u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
	u32 val;
	int ret;

	stm32_i2c_result[0].tx_done = 0;
	stm32_i2c_result[0].error = 0;

	HAL_I2C_Mem_Write_DMA(&hi2c1, (u16)slave_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, pdata, length);

	ret = readl_poll_timeout(&stm32_i2c_result[0].tx_done, val, val || stm32_i2c_result[0].error, 100, 100000);
	if (ret < 0) {
		// 		HAL_I2C_Abort(&hi2c1, (u16)slave_addr << 1);
		pr_err("stm32_i2c: write to 0x%02X timed out\n", slave_addr);
		return ret;
	}

	return 0;
}

static int iic1_master_receive(u8 slave_addr, u16 reg,
	u8 *pdata, u8 length)
{
	u32 val;
	int ret;

	stm32_i2c_result[0].rx_done = 0;
	stm32_i2c_result[0].error = 0;

	HAL_I2C_Mem_Read_DMA(&hi2c1, (u16)slave_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, pdata, length);

	ret = readl_poll_timeout(&stm32_i2c_result[0].rx_done, val, val || stm32_i2c_result[0].error, 100, 100000);
	if (ret < 0) {
		// 		HAL_I2C_Abort(&hi2c1, (u16)slave_addr << 1);
		pr_err("stm32_i2c: read from 0x%02X timed out\n", slave_addr);
		return ret;
	}

	return 0;
}

struct iic_master iic1_master = {
	.init        = iic1_master_init,
	.sda_out     = iic1_sda_out,
	.sda_in      = iic1_sda_in,
	.sda_high    = iic1_sda_high,
	.sda_low     = iic1_sda_low,
	.sda_read    = iic1_sda_read,
	.scl_high    = iic1_scl_high,
	.scl_low     = iic1_scl_low,
	.write_bytes = iic1_master_transmit,
	.read_bytes  = iic1_master_receive,
};

static int spi1_cs_low(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
	return 0;
}

static int spi1_cs_high(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
	return 0;
}

static int spi1_mosi_low(void)
{
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
	return 0;
}

static int spi1_mosi_high(void)
{
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
	return 0;
}

static int spi1_miso_read(void)
{
	return HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_9) ? 1 : 0;
}

static int spi1_sck_low(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
	return 0;
}

static int spi1_sck_high(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
	return 0;
}

static int spi1_write_bytes(u8 *pdata, u8 length)
{
	// 	u32 val;
	// 	int ret;
	//
	// 	stm32_spi_result[0].tx_done = 0;
	// 	stm32_spi_result[0].error = 0;
	//
	// 	HAL_SPI_Transmit_DMA(&hspi1, pdata, length);
	//
	// 	ret = readl_poll_timeout(&stm32_spi_result[0].tx_done, val, val || stm32_spi_result[0].error, 100, 100000);
	// 	if (ret < 0) {
	// 		HAL_SPI_Abort(&hspi1);
	// 		pr_err("stm32_spi: write timed out (%u bytes)\n", length);
	// 		return ret;
	// 	}

	return 0;
}

static int spi1_read_bytes(u8 *pdata, u8 length)
{
	// 	u32 val;
	// 	int ret;
	//
	// 	stm32_spi_result[0].rx_done = 0;
	// 	stm32_spi_result[0].error = 0;
	//
	// 	HAL_SPI_Receive_DMA(&hspi1, pdata, length);
	//
	// 	ret = readl_poll_timeout(&stm32_spi_result[0].rx_done, val, val || stm32_spi_result[0].error, 100, 100000);
	// 	if (ret < 0) {
	// 		HAL_SPI_Abort(&hspi1);
	// 		pr_err("stm32_spi: read timed out (%u bytes)\n", length);
	// 		return ret;
	// 	}

	return 0;
}

static int spi1_write_byte(u8 data)
{
	// 	u32 val;
	// 	int ret;
	//
	// 	stm32_spi_result[0].tx_done = 0;
	// 	stm32_spi_result[0].error = 0;
	//
	// 	HAL_SPI_Transmit_DMA(&hspi1, &data, 1);
	//
	// 	ret = readl_poll_timeout(&stm32_spi_result[0].tx_done, val, val || stm32_spi_result[0].error, 100, 100000);
	// 	if (ret < 0) {
	// 		HAL_SPI_Abort(&hspi1);
	// 		pr_err("stm32_spi: write timed out (1 bytes)\n");
	// 		return ret;
	// 	}

	return 0;
}

static u8 spi1_read_byte(void)
{
	u8 data;
	// 	u32 val;
	// 	int ret;
	//
	// 	stm32_spi_result[0].rx_done = 0;
	// 	stm32_spi_result[0].error = 0;
	//
	// 	HAL_SPI_Receive_DMA(&hspi1, &data, 1);
	//
	// 	ret = readl_poll_timeout(&stm32_spi_result[0].rx_done, val, val || stm32_spi_result[0].error, 100, 100000);
	// 	if (ret < 0) {
	// 		HAL_SPI_Abort(&hspi1);
	// 		pr_err("stm32_spi: read timed out (1 bytes)\n");
	// 		return 0;
	// 	}

	return data;
}

static void qspi1_read(struct qspi_request_block *qrb)
{
	QSPI_CommandTypeDef qspi_cmd;
	HAL_StatusTypeDef hal_ret;

	qspi_cmd.Instruction = qrb->instruction;
	switch (qrb->instruction_mode) {
	case 0:
		qspi_cmd.InstructionMode = QSPI_INSTRUCTION_NONE;
		break;
	case 1:
		qspi_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		break;
	case 2:
		qspi_cmd.InstructionMode = QSPI_INSTRUCTION_2_LINES;
		break;
	case 4:
		qspi_cmd.InstructionMode = QSPI_INSTRUCTION_4_LINES;
		break;
	}

	qspi_cmd.Address = qrb->address;
	switch (qrb->address_size) {
	case 1:
		qspi_cmd.AddressSize = QSPI_ADDRESS_8_BITS;
		break;
	case 2:
		qspi_cmd.AddressSize = QSPI_ADDRESS_16_BITS;
		break;
	case 3:
		qspi_cmd.AddressSize = QSPI_ADDRESS_24_BITS;
		break;
	case 4:
		qspi_cmd.AddressSize = QSPI_ADDRESS_32_BITS;
		break;
	}
	switch (qrb->address_mode) {
	case 0:
		qspi_cmd.AddressMode = QSPI_ADDRESS_NONE;
		break;
	case 1:
		qspi_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
		break;
	case 2:
		qspi_cmd.AddressMode = QSPI_ADDRESS_2_LINES;
		break;
	case 4:
		qspi_cmd.AddressMode = QSPI_ADDRESS_4_LINES;
		break;
	}

	// 	qspi_cmd.AlternateBytes    = (cmd == 0xEB) ? 0xA0 : 0;
	// 	qspi_cmd.AlternateBytesSize = (cmd == 0xEB) ? QSPI_ALTERNATE_BYTES_1_LINE : 0;
	// 	qspi_cmd.AlternateByteMode = (cmd == 0xEB) ? QSPI_ALTERNATE_BYTES_1_LINE : 0;
	qspi_cmd.AlternateBytes = qrb->alternate_bytes;
	switch (qrb->alternate_bytes_size) {
	case 0:
		qspi_cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_NONE;
		break;
	case 1:
		qspi_cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_1_LINE;
		break;
	case 2:
		qspi_cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_2_LINES;
		break;
	case 4:
		qspi_cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_4_LINES;
		break;
	}
	qspi_cmd.AlternateByteMode = qrb->alternate_byte_mode;

	switch (qrb->transfer_mode) {
	case 0:
		qspi_cmd.DataMode = QSPI_DATA_NONE;
		break;
	case 1:
		qspi_cmd.DataMode = QSPI_DATA_1_LINE;
		break;
	case 2:
		qspi_cmd.DataMode = QSPI_DATA_2_LINES;
		break;
	case 4:
		qspi_cmd.DataMode = QSPI_DATA_4_LINES;
		break;
	}
	qspi_cmd.NbData            = qrb->transfer_length;

	qspi_cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
	qspi_cmd.DdrMode           = qrb->ddr_mode ? QSPI_DDR_MODE_ENABLE : QSPI_DDR_MODE_DISABLE;
	qspi_cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	qspi_cmd.DummyCycles       = qrb->dummy_cycles;

	hal_ret = HAL_QSPI_Command(&hqspi, &qspi_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		pr_err("qspi1_read: HAL_QSPI_Command failed, ret=%d, cmd=0x%02X, addr=0x%08X\n",
			(int)hal_ret, qrb->instruction, qrb->address);
		return;
	}

	if (qrb->transfer_length > 0) {
		hal_ret = HAL_QSPI_Receive(&hqspi, qrb->transfer_buffer, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
		if (hal_ret != HAL_OK) {
			pr_err("qspi1_read: HAL_QSPI_Receive failed, ret=%d, len=%u\n",
				(int)hal_ret, qrb->transfer_length);
		}
	}

	/* Clear residual bits from auto-polling that can corrupt subsequent commands */
	CLEAR_BIT(hqspi.Instance->CR, QUADSPI_CR_PMM | QUADSPI_CR_APMS | QUADSPI_CR_DMAEN);
}

static void qspi1_write(struct qspi_request_block *qrb)
{
	QSPI_CommandTypeDef qspi_cmd;
	HAL_StatusTypeDef hal_ret;

	qspi_cmd.Instruction = qrb->instruction;
	switch (qrb->instruction_mode) {
	case 0:
		qspi_cmd.InstructionMode = QSPI_INSTRUCTION_NONE;
		break;
	case 1:
		qspi_cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		break;
	case 2:
		qspi_cmd.InstructionMode = QSPI_INSTRUCTION_2_LINES;
		break;
	case 4:
		qspi_cmd.InstructionMode = QSPI_INSTRUCTION_4_LINES;
		break;
	}

	qspi_cmd.Address = qrb->address;
	switch (qrb->address_size) {
	case 1:
		qspi_cmd.AddressSize = QSPI_ADDRESS_8_BITS;
		break;
	case 2:
		qspi_cmd.AddressSize = QSPI_ADDRESS_16_BITS;
		break;
	case 3:
		qspi_cmd.AddressSize = QSPI_ADDRESS_24_BITS;
		break;
	case 4:
		qspi_cmd.AddressSize = QSPI_ADDRESS_32_BITS;
		break;
	}
	switch (qrb->address_mode) {
	case 0:
		qspi_cmd.AddressMode = QSPI_ADDRESS_NONE;
		break;
	case 1:
		qspi_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
		break;
	case 2:
		qspi_cmd.AddressMode = QSPI_ADDRESS_2_LINES;
		break;
	case 4:
		qspi_cmd.AddressMode = QSPI_ADDRESS_4_LINES;
		break;
	}

	// 	qspi_cmd.AlternateBytes    = (cmd == 0xEB) ? 0xA0 : 0;
	// 	qspi_cmd.AlternateBytesSize = (cmd == 0xEB) ? QSPI_ALTERNATE_BYTES_1_LINE : 0;
	// 	qspi_cmd.AlternateByteMode = (cmd == 0xEB) ? QSPI_ALTERNATE_BYTES_1_LINE : 0;
	qspi_cmd.AlternateBytes = qrb->alternate_bytes;
	switch (qrb->alternate_bytes_size) {
	case 0:
		qspi_cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_NONE;
		break;
	case 1:
		qspi_cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_1_LINE;
		break;
	case 2:
		qspi_cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_2_LINES;
		break;
	case 4:
		qspi_cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_4_LINES;
		break;
	}
	qspi_cmd.AlternateByteMode = qrb->alternate_byte_mode;

	switch (qrb->transfer_mode) {
	case 0:
		qspi_cmd.DataMode = QSPI_DATA_NONE;
		break;
	case 1:
		qspi_cmd.DataMode = QSPI_DATA_1_LINE;
		break;
	case 2:
		qspi_cmd.DataMode = QSPI_DATA_2_LINES;
		break;
	case 4:
		qspi_cmd.DataMode = QSPI_DATA_4_LINES;
		break;
	}
	qspi_cmd.NbData            = qrb->transfer_length;

	qspi_cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
	qspi_cmd.DdrMode           = qrb->ddr_mode ? QSPI_DDR_MODE_ENABLE : QSPI_DDR_MODE_DISABLE;
	qspi_cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	qspi_cmd.DummyCycles       = qrb->dummy_cycles;

	hal_ret = HAL_QSPI_Command(&hqspi, &qspi_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		pr_err("qspi1_write: HAL_QSPI_Command failed, ret=%d, cmd=0x%02X, addr=0x%08X\n",
			(int)hal_ret, qrb->instruction, qrb->address);
		return;
	}

	if (qrb->transfer_length > 0) {
		hal_ret = HAL_QSPI_Transmit(&hqspi, qrb->transfer_buffer, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
		if (hal_ret != HAL_OK) {
			pr_err("qspi1_write: HAL_QSPI_Transmit failed, ret=%d, len=%u\n",
				(int)hal_ret, qrb->transfer_length);
		}
	}

	/* Clear residual bits from auto-polling that can corrupt subsequent commands */
	CLEAR_BIT(hqspi.Instance->CR, QUADSPI_CR_PMM | QUADSPI_CR_APMS | QUADSPI_CR_DMAEN);
}

static void spi1_set_mode(enum qspi_read_mode mode)
{

}

static int qspi1_poll(struct qspi_request_block *qrb, u32 timeout_us)
{
	QSPI_CommandTypeDef sCommand;
	QSPI_AutoPollingTypeDef sConfig;
	HAL_StatusTypeDef hal_ret;
	int ret = 0;

	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = qrb->instruction;
	sCommand.AddressMode       = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode          = QSPI_DATA_1_LINE;
	sCommand.DummyCycles       = 0;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
	sCommand.NbData            = 1;

	sConfig.Match              = qrb->match;
	sConfig.Mask               = qrb->mask;
	sConfig.MatchMode          = QSPI_MATCH_MODE_AND;
	sConfig.StatusBytesSize    = 1;
	sConfig.Interval           = 0x10;
	sConfig.AutomaticStop      = QSPI_AUTOMATIC_STOP_ENABLE;

	hal_ret = HAL_QSPI_AutoPolling(&hqspi, &sCommand, &sConfig,
			(timeout_us + 999) / 1000);
	if (hal_ret != HAL_OK) {
		ret - ETIMEDOUT;
	}

	CLEAR_BIT(hqspi.Instance->CR, QUADSPI_CR_PMM | QUADSPI_CR_APMS | QUADSPI_CR_DMAEN);

	return ret;
}

static int spi1_master_init(void)
{
	return 0;
}

struct w25qxxx_device *w25qxxx_dev;

struct spi_master spi1_master = {
	.cs_low      = spi1_cs_low,
	.cs_high     = spi1_cs_high,
	.mosi_low    = spi1_mosi_low,
	.mosi_high   = spi1_mosi_high,
	.miso_read   = spi1_miso_read,
	.sck_low     = spi1_sck_low,
	.sck_high    = spi1_sck_high,
	.write_byte  = spi1_write_byte,
	.read_byte   = spi1_read_byte,
	.write_bytes = spi1_write_bytes,
	.read_bytes  = spi1_read_bytes,
	.qspi_read   = qspi1_read,
	.qspi_write  = qspi1_write,
	.qspi_poll   = qspi1_poll,
	.set_mode	 = spi1_set_mode,
	.init        = spi1_master_init,
};

static int spi6_cs_low(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_11, GPIO_PIN_RESET);
	return 0;
}

static int spi6_cs_high(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_11, GPIO_PIN_SET);
	return 0;
}

static int spi6_mosi_low(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);
	return 0;
}

static int spi6_mosi_high(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET);
	return 0;
}

static int spi6_miso_read(void)
{
	return HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_12) ? 1 : 0;
}

static int spi6_sck_low(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);
	return 0;
}

static int spi6_sck_high(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
	return 0;
}

static int spi6_write_bytes(u8 *pdata, u8 length)
{
	u32 val;
	int ret;

	stm32_spi_result[0].tx_done = 0;
	stm32_spi_result[0].error = 0;

	HAL_SPI_Transmit_DMA(&hspi6, pdata, length);

	ret = readl_poll_timeout(&stm32_spi_result[0].tx_done, val, val || stm32_spi_result[0].error, 100, 100000);
	if (ret < 0) {
		HAL_SPI_Abort(&hspi6);
		pr_err("stm32_spi: write timed out (%u bytes)\n", length);
		return ret;
	}

	return 0;
}

static int spi6_read_bytes(u8 *pdata, u8 length)
{
	u32 val;
	int ret;

	stm32_spi_result[0].rx_done = 0;
	stm32_spi_result[0].error = 0;

	HAL_SPI_Receive_DMA(&hspi6, pdata, length);

	ret = readl_poll_timeout(&stm32_spi_result[0].rx_done, val, val || stm32_spi_result[0].error, 100, 100000);
	if (ret < 0) {
		HAL_SPI_Abort(&hspi6);
		pr_err("stm32_spi: read timed out (%u bytes)\n", length);
		return ret;
	}

	return 0;
}

static int spi6_write_byte(u8 data)
{
	u32 val;
	int ret;

	stm32_spi_result[0].tx_done = 0;
	stm32_spi_result[0].error = 0;

	HAL_SPI_Transmit_DMA(&hspi6, &data, 1);

	ret = readl_poll_timeout(&stm32_spi_result[0].tx_done, val, val || stm32_spi_result[0].error, 100, 100000);
	if (ret < 0) {
		HAL_SPI_Abort(&hspi6);
		pr_err("stm32_spi: write timed out (1 bytes)\n");
		return ret;
	}

	return 0;
}

static u8 spi6_read_byte(void)
{
	u8 data;
	u32 val;
	int ret;

	stm32_spi_result[0].rx_done = 0;
	stm32_spi_result[0].error = 0;

	HAL_SPI_Receive_DMA(&hspi6, &data, 1);

	ret = readl_poll_timeout(&stm32_spi_result[0].rx_done, val, val || stm32_spi_result[0].error, 100, 100000);
	if (ret < 0) {
		HAL_SPI_Abort(&hspi6);
		pr_err("stm32_spi: read timed out (1 bytes)\n");
		return 0;
	}

	return data;
}

static int spi6_master_init(void)
{
	return 0;
}

struct spi_master spi6_master = {
	.cs_low      = spi6_cs_low,
	.cs_high     = spi6_cs_high,
	.mosi_low    = spi6_mosi_low,
	.mosi_high   = spi6_mosi_high,
	.miso_read   = spi6_miso_read,
	.sck_low     = spi6_sck_low,
	.sck_high    = spi6_sck_high,
	.write_byte  = spi6_write_byte,
	.read_byte   = spi6_read_byte,
	.write_bytes = spi6_write_bytes,
	.read_bytes  = spi6_read_bytes,
	.init        = spi6_master_init,
};

uint8_t w25qxx_interface_spi_qspi_init(void)
{
	return 0;
}

uint8_t w25qxx_interface_spi_qspi_deinit(void)
{
	return 0;
}

static volatile u8 flash_wait_for_completion = 1;

void flash_set_wait_for_completion(u8 enable)
{
	flash_wait_for_completion = enable ? 1 : 0;
}

#define QSPI_FUNCTIONAL_MODE_INDIRECT_WRITE    0x00000000U                     /*!<Indirect write mode*/
#define QSPI_FUNCTIONAL_MODE_INDIRECT_READ     ((uint32_t)QUADSPI_CCR_FMODE_0) /*!<Indirect read mode*/
#define QSPI_FUNCTIONAL_MODE_AUTO_POLLING      ((uint32_t)QUADSPI_CCR_FMODE_1) /*!<Automatic polling mode*/
#define QSPI_FUNCTIONAL_MODE_MEMORY_MAPPED     ((uint32_t)QUADSPI_CCR_FMODE)   /*!<Memory-mapped mode*/

static HAL_StatusTypeDef QSPI_WaitFlagStateUntilTimeout(QSPI_HandleTypeDef *hqspi, uint32_t Flag,
	FlagStatus State, uint32_t Tickstart, uint32_t Timeout)
{
	/* Wait until flag is in expected state */
	while ((__HAL_QSPI_GET_FLAG(hqspi, Flag)) != State) {
		/* Check for the Timeout */
		if (Timeout != HAL_MAX_DELAY) {
			if (((HAL_GetTick() - Tickstart) > Timeout) || (Timeout == 0U)) {
				hqspi->State     = HAL_QSPI_STATE_ERROR;
				hqspi->ErrorCode |= HAL_QSPI_ERROR_TIMEOUT;

				return HAL_ERROR;
			}
		}
	}
	return HAL_OK;
}

static void QSPI_Config(QSPI_HandleTypeDef *hqspi, QSPI_CommandTypeDef *cmd, uint32_t FunctionalMode)
{
	if ((cmd->DataMode != QSPI_DATA_NONE) && (FunctionalMode != QSPI_FUNCTIONAL_MODE_MEMORY_MAPPED)) {
		/* Configure QSPI: DLR register with the number of data to read or write */
		WRITE_REG(hqspi->Instance->DLR, (cmd->NbData - 1U));
	}

	if (cmd->InstructionMode != QSPI_INSTRUCTION_NONE) {
		if (cmd->AlternateByteMode != QSPI_ALTERNATE_BYTES_NONE) {
			/* Configure QSPI: ABR register with alternate bytes value */
			WRITE_REG(hqspi->Instance->ABR, cmd->AlternateBytes);

			if (cmd->AddressMode != QSPI_ADDRESS_NONE) {
				/*---- Command with instruction, address and alternate bytes ----*/
				/* Configure QSPI: CCR register with all communications parameters */
				WRITE_REG(hqspi->Instance->CCR, (cmd->DdrMode | cmd->DdrHoldHalfCycle | cmd->SIOOMode |
					cmd->DataMode | (cmd->DummyCycles << QUADSPI_CCR_DCYC_Pos) |
					cmd->AlternateBytesSize | cmd->AlternateByteMode |
					cmd->AddressSize | cmd->AddressMode | cmd->InstructionMode |
					cmd->Instruction | FunctionalMode));

				if (FunctionalMode != QSPI_FUNCTIONAL_MODE_MEMORY_MAPPED) {
					/* Configure QSPI: AR register with address value */
					WRITE_REG(hqspi->Instance->AR, cmd->Address);
				}
			} else {
				/*---- Command with instruction and alternate bytes ----*/
				/* Configure QSPI: CCR register with all communications parameters */
				WRITE_REG(hqspi->Instance->CCR, (cmd->DdrMode | cmd->DdrHoldHalfCycle | cmd->SIOOMode |
					cmd->DataMode | (cmd->DummyCycles << QUADSPI_CCR_DCYC_Pos) |
					cmd->AlternateBytesSize | cmd->AlternateByteMode |
					cmd->AddressMode | cmd->InstructionMode |
					cmd->Instruction | FunctionalMode));

				/* Clear AR register */
				CLEAR_REG(hqspi->Instance->AR);
			}
		} else {
			if (cmd->AddressMode != QSPI_ADDRESS_NONE) {
				/*---- Command with instruction and address ----*/
				/* Configure QSPI: CCR register with all communications parameters */
				WRITE_REG(hqspi->Instance->CCR, (cmd->DdrMode | cmd->DdrHoldHalfCycle | cmd->SIOOMode |
					cmd->DataMode | (cmd->DummyCycles << QUADSPI_CCR_DCYC_Pos) |
					cmd->AlternateByteMode | cmd->AddressSize | cmd->AddressMode |
					cmd->InstructionMode | cmd->Instruction | FunctionalMode));

				if (FunctionalMode != QSPI_FUNCTIONAL_MODE_MEMORY_MAPPED) {
					/* Configure QSPI: AR register with address value */
					WRITE_REG(hqspi->Instance->AR, cmd->Address);
				}
			} else {
				/*---- Command with only instruction ----*/
				/* Configure QSPI: CCR register with all communications parameters */
				WRITE_REG(hqspi->Instance->CCR, (cmd->DdrMode | cmd->DdrHoldHalfCycle | cmd->SIOOMode |
					cmd->DataMode | (cmd->DummyCycles << QUADSPI_CCR_DCYC_Pos) |
					cmd->AlternateByteMode | cmd->AddressMode |
					cmd->InstructionMode | cmd->Instruction | FunctionalMode));

				/* Clear AR register */
				CLEAR_REG(hqspi->Instance->AR);
			}
		}
	} else {
		if (cmd->AlternateByteMode != QSPI_ALTERNATE_BYTES_NONE) {
			/* Configure QSPI: ABR register with alternate bytes value */
			WRITE_REG(hqspi->Instance->ABR, cmd->AlternateBytes);

			if (cmd->AddressMode != QSPI_ADDRESS_NONE) {
				/*---- Command with address and alternate bytes ----*/
				/* Configure QSPI: CCR register with all communications parameters */
				WRITE_REG(hqspi->Instance->CCR, (cmd->DdrMode | cmd->DdrHoldHalfCycle | cmd->SIOOMode |
					cmd->DataMode | (cmd->DummyCycles << QUADSPI_CCR_DCYC_Pos) |
					cmd->AlternateBytesSize | cmd->AlternateByteMode |
					cmd->AddressSize | cmd->AddressMode |
					cmd->InstructionMode | FunctionalMode));

				if (FunctionalMode != QSPI_FUNCTIONAL_MODE_MEMORY_MAPPED) {
					/* Configure QSPI: AR register with address value */
					WRITE_REG(hqspi->Instance->AR, cmd->Address);
				}
			} else {
				/*---- Command with only alternate bytes ----*/
				/* Configure QSPI: CCR register with all communications parameters */
				WRITE_REG(hqspi->Instance->CCR, (cmd->DdrMode | cmd->DdrHoldHalfCycle | cmd->SIOOMode |
					cmd->DataMode | (cmd->DummyCycles << QUADSPI_CCR_DCYC_Pos) |
					cmd->AlternateBytesSize | cmd->AlternateByteMode |
					cmd->AddressMode | cmd->InstructionMode | FunctionalMode));

				/* Clear AR register */
				CLEAR_REG(hqspi->Instance->AR);
			}
		} else {
			if (cmd->AddressMode != QSPI_ADDRESS_NONE) {
				/*---- Command with only address ----*/
				/* Configure QSPI: CCR register with all communications parameters */
				WRITE_REG(hqspi->Instance->CCR, (cmd->DdrMode | cmd->DdrHoldHalfCycle | cmd->SIOOMode |
					cmd->DataMode | (cmd->DummyCycles << QUADSPI_CCR_DCYC_Pos) |
					cmd->AlternateByteMode | cmd->AddressSize |
					cmd->AddressMode | cmd->InstructionMode | FunctionalMode));

				if (FunctionalMode != QSPI_FUNCTIONAL_MODE_MEMORY_MAPPED) {
					/* Configure QSPI: AR register with address value */
					WRITE_REG(hqspi->Instance->AR, cmd->Address);
				}
			} else {
				/*---- Command with only data phase ----*/
				if (cmd->DataMode != QSPI_DATA_NONE) {
					/* Configure QSPI: CCR register with all communications parameters */
					WRITE_REG(hqspi->Instance->CCR, (cmd->DdrMode | cmd->DdrHoldHalfCycle | cmd->SIOOMode |
						cmd->DataMode | (cmd->DummyCycles << QUADSPI_CCR_DCYC_Pos) |
						cmd->AlternateByteMode | cmd->AddressMode |
						cmd->InstructionMode | FunctionalMode));

					/* Clear AR register */
					CLEAR_REG(hqspi->Instance->AR);
				}
			}
		}
	}
}

HAL_StatusTypeDef HAL_QSPI_Command_Custom(QSPI_HandleTypeDef *hqspi, QSPI_CommandTypeDef *cmd, uint32_t Timeout)
{
	HAL_StatusTypeDef status;
	uint32_t tickstart = HAL_GetTick();

	if (hqspi->State == HAL_QSPI_STATE_READY) {
		hqspi->ErrorCode = HAL_QSPI_ERROR_NONE;

		/* Update QSPI state */
		hqspi->State = HAL_QSPI_STATE_BUSY;

		/* Wait till BUSY flag reset */
		status = QSPI_WaitFlagStateUntilTimeout(hqspi, QSPI_FLAG_BUSY, RESET, tickstart, Timeout);

		if (status == HAL_OK) {
			/* Call the configuration function */
			QSPI_Config(hqspi, cmd, QSPI_FUNCTIONAL_MODE_INDIRECT_WRITE);

			if (cmd->DataMode == QSPI_DATA_NONE) {
				/* When there is no data phase, the transfer start as soon as the configuration is done
				so wait until TC flag is set to go back in idle state */
				status = QSPI_WaitFlagStateUntilTimeout(hqspi, QSPI_FLAG_TC, SET, tickstart, Timeout);

				if (status == HAL_OK) {
					__HAL_QSPI_CLEAR_FLAG(hqspi, QSPI_FLAG_TC);

					/* Update QSPI state */
					hqspi->State = HAL_QSPI_STATE_READY;
				}
			} else {
				/* Update QSPI state */
				hqspi->State = HAL_QSPI_STATE_READY;
			}
		}
	} else {
		status = HAL_BUSY;
	}

	/* Return function status */
	return status;
}

static void QSPI_DMAAbortCplt(MDMA_HandleTypeDef *hmdma)
{
	QSPI_HandleTypeDef* hqspi = (QSPI_HandleTypeDef*)(hmdma->Parent);

	hqspi->RxXferCount = 0U;
	hqspi->TxXferCount = 0U;

	if (hqspi->State == HAL_QSPI_STATE_ABORT) {
		/* MDMA Abort called by QSPI abort */
		/* Clear interrupt */
		__HAL_QSPI_CLEAR_FLAG(hqspi, QSPI_FLAG_TC);

		/* Enable the QSPI Transfer Complete Interrupt */
		__HAL_QSPI_ENABLE_IT(hqspi, QSPI_IT_TC);

		/* Configure QSPI: CR register with Abort request */
		SET_BIT(hqspi->Instance->CR, QUADSPI_CR_ABORT);
	} else {
		/* MDMA Abort called due to a transfer error interrupt */
		/* Change state of QSPI */
		hqspi->State = HAL_QSPI_STATE_READY;

		/* Error callback */
#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
		hqspi->ErrorCallback(hqspi);
#else
		HAL_QSPI_ErrorCallback(hqspi);
#endif
	}
}

HAL_StatusTypeDef HAL_QSPI_Abort_IT_Custom(QSPI_HandleTypeDef *hqspi)
{
	HAL_StatusTypeDef status = HAL_OK;

	/* Check if the state is in one of the busy states */
	if (((uint32_t)hqspi->State & 0x2U) != 0U) {
		/* Update QSPI state */
		hqspi->State = HAL_QSPI_STATE_ABORT;

		/* Disable all interrupts */
		__HAL_QSPI_DISABLE_IT(hqspi, (QSPI_IT_TO | QSPI_IT_SM | QSPI_IT_FT | QSPI_IT_TC | QSPI_IT_TE));

		if ((hqspi->Instance->CR & QUADSPI_CR_DMAEN) != 0U) {
			/* Disable using MDMA by clearing DMAEN, note that DMAEN bit is "reserved"
			   but no impact on H7 HW and it minimize the cost in the footprint */
			CLEAR_BIT(hqspi->Instance->CR, QUADSPI_CR_DMAEN);

			/* Abort MDMA channel */
			hqspi->hmdma->XferAbortCallback = QSPI_DMAAbortCplt;
			if (HAL_MDMA_Abort_IT(hqspi->hmdma) != HAL_OK) {
				/* Change state of QSPI */
				hqspi->State = HAL_QSPI_STATE_READY;

				/* Abort Complete callback */
#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
				hqspi->AbortCpltCallback(hqspi);
#else
				HAL_QSPI_AbortCpltCallback(hqspi);
#endif
			}
		} else {
			if (__HAL_QSPI_GET_FLAG(hqspi, QSPI_FLAG_BUSY) != RESET) {
				/* Clear interrupt */
				__HAL_QSPI_CLEAR_FLAG(hqspi, QSPI_FLAG_TC);

				/* Enable the QSPI Transfer Complete Interrupt */
				__HAL_QSPI_ENABLE_IT(hqspi, QSPI_IT_TC);

				/* Configure QSPI: CR register with Abort request */
				SET_BIT(hqspi->Instance->CR, QUADSPI_CR_ABORT);
			} else {
				/* Change state of QSPI */
				hqspi->State = HAL_QSPI_STATE_READY;
			}
		}
	}
	return status;
}

static void QSPI_DMARxCplt(MDMA_HandleTypeDef *hmdma)
{
	QSPI_HandleTypeDef* hqspi = (QSPI_HandleTypeDef*)(hmdma->Parent);
	hqspi->RxXferCount = 0U;

	/* Enable the QSPI transfer complete Interrupt */
	__HAL_QSPI_ENABLE_IT(hqspi, QSPI_IT_TC);
}

static void QSPI_DMATxCplt(MDMA_HandleTypeDef *hmdma)
{
	QSPI_HandleTypeDef* hqspi = (QSPI_HandleTypeDef*)(hmdma->Parent);
	hqspi->TxXferCount = 0U;

	/* Enable the QSPI transfer complete Interrupt */
	__HAL_QSPI_ENABLE_IT(hqspi, QSPI_IT_TC);
}

static void QSPI_DMAError(MDMA_HandleTypeDef *hmdma)
{
	QSPI_HandleTypeDef* hqspi = (QSPI_HandleTypeDef*)(hmdma->Parent);

	hqspi->RxXferCount = 0U;
	hqspi->TxXferCount = 0U;
	hqspi->ErrorCode   |= HAL_QSPI_ERROR_DMA;

	/* Disable using MDMA by clearing DMAEN, note that DMAEN bit is "reserved"
	   but no impact on H7 HW and it minimize the cost in the footprint */
	CLEAR_BIT(hqspi->Instance->CR, QUADSPI_CR_DMAEN);

	/* Abort the QSPI */
	(void)HAL_QSPI_Abort_IT_Custom(hqspi);

}

static void MDMA_SetConfig(MDMA_HandleTypeDef *hmdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t BlockDataLength, uint32_t BlockCount)
{
	uint32_t addressMask;

	/* Configure the MDMA Channel data length */
	MODIFY_REG(hmdma->Instance->CBNDTR, MDMA_CBNDTR_BNDT, (BlockDataLength & MDMA_CBNDTR_BNDT));

	/* Configure the MDMA block repeat count */
	MODIFY_REG(hmdma->Instance->CBNDTR, MDMA_CBNDTR_BRC, ((BlockCount - 1U) << MDMA_CBNDTR_BRC_Pos) & MDMA_CBNDTR_BRC);

	/* Clear all interrupt flags */
	__HAL_MDMA_CLEAR_FLAG(hmdma, MDMA_FLAG_TE | MDMA_FLAG_CTC | MDMA_CISR_BRTIF | MDMA_CISR_BTIF | MDMA_CISR_TCIF);

	/* Configure MDMA Channel destination address */
	hmdma->Instance->CDAR = DstAddress;

	/* Configure MDMA Channel Source address */
	hmdma->Instance->CSAR = SrcAddress;

	addressMask = SrcAddress & 0xFF000000U;
	if ((addressMask == 0x20000000U) || (addressMask == 0x00000000U)) {
		/*The AHBSbus is used as source (read operation) on channel x */
		hmdma->Instance->CTBR |= MDMA_CTBR_SBUS;
	} else {
		/*The AXI bus is used as source (read operation) on channel x */
		hmdma->Instance->CTBR &= (~MDMA_CTBR_SBUS);
	}

	addressMask = DstAddress & 0xFF000000U;
	if ((addressMask == 0x20000000U) || (addressMask == 0x00000000U)) {
		/*The AHB bus is used as destination (write operation) on channel x */
		hmdma->Instance->CTBR |= MDMA_CTBR_DBUS;
	} else {
		/*The AXI bus is used as destination (write operation) on channel x */
		hmdma->Instance->CTBR &= (~MDMA_CTBR_DBUS);
	}

	/* Set the linked list register to the first node of the list */
	hmdma->Instance->CLAR = (uint32_t)hmdma->FirstLinkedListNodeAddress;
}

HAL_StatusTypeDef HAL_MDMA_Start_IT_Custom(MDMA_HandleTypeDef *hmdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t BlockDataLength, uint32_t BlockCount)
{

	if (HAL_MDMA_STATE_READY == hmdma->State) {
		/* Change MDMA peripheral state */
		hmdma->State = HAL_MDMA_STATE_BUSY;

		/* Initialize the error code */
		hmdma->ErrorCode = HAL_MDMA_ERROR_NONE;

		/* Disable the peripheral */
		__HAL_MDMA_DISABLE(hmdma);

		/* Configure the source, destination address and the data length */
		MDMA_SetConfig(hmdma, SrcAddress, DstAddress, BlockDataLength, BlockCount);

		/* Enable Common interrupts i.e Transfer Error IT and Channel Transfer Complete IT*/
		__HAL_MDMA_ENABLE_IT(hmdma, (MDMA_IT_TE | MDMA_IT_CTC));

		if (hmdma->XferBlockCpltCallback != NULL) {
			/* if Block transfer complete Callback is set enable the corresponding IT*/
			__HAL_MDMA_ENABLE_IT(hmdma, MDMA_IT_BT);
		}

		if (hmdma->XferRepeatBlockCpltCallback != NULL) {
			/* if Repeated Block transfer complete Callback is set enable the corresponding IT*/
			__HAL_MDMA_ENABLE_IT(hmdma, MDMA_IT_BRT);
		}

		if (hmdma->XferBufferCpltCallback != NULL) {
			/* if buffer transfer complete Callback is set enable the corresponding IT*/
			__HAL_MDMA_ENABLE_IT(hmdma, MDMA_IT_BFTC);
		}

		/* Enable the Peripheral */
		__HAL_MDMA_ENABLE(hmdma);

		if (hmdma->Init.Request == MDMA_REQUEST_SW) {
			/* activate If SW request mode*/
			hmdma->Instance->CCR |=  MDMA_CCR_SWRQ;
		}
	} else {
		/* Return error status */
		return HAL_BUSY;
	}

	return HAL_OK;
}

HAL_StatusTypeDef HAL_QSPI_Transmit_DMA_Custom(QSPI_HandleTypeDef *hqspi, uint8_t *pData)
{
	HAL_StatusTypeDef status = HAL_OK;
	uint32_t data_size = (READ_REG(hqspi->Instance->DLR) + 1U);

	if (hqspi->State == HAL_QSPI_STATE_READY) {
		/* Clear the error code */
		hqspi->ErrorCode = HAL_QSPI_ERROR_NONE;

		if (pData != NULL) {
			/* Configure counters of the handle */
			hqspi->TxXferCount = data_size;

			/* Update state */
			hqspi->State = HAL_QSPI_STATE_BUSY_INDIRECT_TX;

			/* Clear interrupt */
			__HAL_QSPI_CLEAR_FLAG(hqspi, (QSPI_FLAG_TE | QSPI_FLAG_TC));

			/* Configure size and pointer of the handle */
			hqspi->TxXferSize = hqspi->TxXferCount;
			hqspi->pTxBuffPtr = pData;

			/* Configure QSPI: CCR register with functional mode as indirect write */
			MODIFY_REG(hqspi->Instance->CCR, QUADSPI_CCR_FMODE, QSPI_FUNCTIONAL_MODE_INDIRECT_WRITE);

			/* Set the QSPI MDMA transfer complete callback */
			hqspi->hmdma->XferCpltCallback = QSPI_DMATxCplt;

			/* Set the MDMA error callback */
			hqspi->hmdma->XferErrorCallback = QSPI_DMAError;

			/* Clear the MDMA abort callback */
			hqspi->hmdma->XferAbortCallback = NULL;

			/* In Transmit mode , the MDMA destination is the QSPI DR register : Force the MDMA Destination Increment to disable */
			MODIFY_REG(hqspi->hmdma->Instance->CTCR, (MDMA_CTCR_DINC | MDMA_CTCR_DINCOS), MDMA_DEST_INC_DISABLE);

			/* Update MDMA configuration with the correct SourceInc field for Write operation */
			if (hqspi->hmdma->Init.SourceDataSize == MDMA_SRC_DATASIZE_BYTE) {
				MODIFY_REG(hqspi->hmdma->Instance->CTCR, (MDMA_CTCR_SINC | MDMA_CTCR_SINCOS), MDMA_SRC_INC_BYTE);
			} else if (hqspi->hmdma->Init.SourceDataSize == MDMA_SRC_DATASIZE_HALFWORD) {
				MODIFY_REG(hqspi->hmdma->Instance->CTCR, (MDMA_CTCR_SINC | MDMA_CTCR_SINCOS), MDMA_SRC_INC_HALFWORD);
			} else if (hqspi->hmdma->Init.SourceDataSize == MDMA_SRC_DATASIZE_WORD) {
				MODIFY_REG(hqspi->hmdma->Instance->CTCR, (MDMA_CTCR_SINC | MDMA_CTCR_SINCOS), MDMA_SRC_INC_WORD);
			} else {
				/* in case of incorrect source data size */
				hqspi->ErrorCode |= HAL_QSPI_ERROR_DMA;
				status = HAL_ERROR;
			}

			/* Enable the QSPI transfer error Interrupt */
			__HAL_QSPI_ENABLE_IT(hqspi, QSPI_IT_TE);

			/* Use DMAEN bit with no impact on H7 HW to record MDMA transfer request */
			SET_BIT(hqspi->Instance->CR, QUADSPI_CR_DMAEN);

			/* Enable the QSPI transmit MDMA */
			if (HAL_MDMA_Start_IT_Custom(hqspi->hmdma, (uint32_t)pData, (uint32_t)&hqspi->Instance->DR, hqspi->TxXferSize, 1) == HAL_OK) {
			} else {
				/* Clear DMAEN bit with no impact on H7 HW to cancel MDMA transfer request */
				CLEAR_BIT(hqspi->Instance->CR, QUADSPI_CR_DMAEN);

				status = HAL_ERROR;
				hqspi->ErrorCode |= HAL_QSPI_ERROR_DMA;
				hqspi->State = HAL_QSPI_STATE_READY;
			}
		} else {
			hqspi->ErrorCode |= HAL_QSPI_ERROR_INVALID_PARAM;
			status = HAL_ERROR;
		}
	} else {
		status = HAL_BUSY;
	}

	return status;
}

HAL_StatusTypeDef HAL_QSPI_Receive_DMA_Custom(QSPI_HandleTypeDef *hqspi, uint8_t *pData)
{
	HAL_StatusTypeDef status = HAL_OK;
	uint32_t addr_reg = READ_REG(hqspi->Instance->AR);
	uint32_t data_size = (READ_REG(hqspi->Instance->DLR) + 1U);

	if (hqspi->State == HAL_QSPI_STATE_READY) {
		/* Clear the error code */
		hqspi->ErrorCode = HAL_QSPI_ERROR_NONE;

		if (pData != NULL) {
			/* Configure counters of the handle */
			hqspi->RxXferCount = data_size;
			/* Update state */
			hqspi->State = HAL_QSPI_STATE_BUSY_INDIRECT_RX;

			/* Clear interrupt */
			__HAL_QSPI_CLEAR_FLAG(hqspi, (QSPI_FLAG_TE | QSPI_FLAG_TC));

			/* Configure size and pointer of the handle */
			hqspi->RxXferSize = hqspi->RxXferCount;
			hqspi->pRxBuffPtr = pData;

			/* Set the QSPI MDMA transfer complete callback */
			hqspi->hmdma->XferCpltCallback = QSPI_DMARxCplt;

			/* Set the MDMA error callback */
			hqspi->hmdma->XferErrorCallback = QSPI_DMAError;

			/* Clear the MDMA abort callback */
			hqspi->hmdma->XferAbortCallback = NULL;

			/* In Receive mode , the MDMA source is the QSPI DR register : Force the MDMA Source Increment to disable */
			MODIFY_REG(hqspi->hmdma->Instance->CTCR, (MDMA_CTCR_SINC | MDMA_CTCR_SINCOS), MDMA_SRC_INC_DISABLE);

			/* Update MDMA configuration with the correct DestinationInc field for read operation */
			if (hqspi->hmdma->Init.DestDataSize == MDMA_DEST_DATASIZE_BYTE) {
				MODIFY_REG(hqspi->hmdma->Instance->CTCR, (MDMA_CTCR_DINC | MDMA_CTCR_DINCOS), MDMA_DEST_INC_BYTE);
			} else if (hqspi->hmdma->Init.DestDataSize == MDMA_DEST_DATASIZE_HALFWORD) {
				MODIFY_REG(hqspi->hmdma->Instance->CTCR, (MDMA_CTCR_DINC | MDMA_CTCR_DINCOS), MDMA_DEST_INC_HALFWORD);
			} else if (hqspi->hmdma->Init.DestDataSize == MDMA_DEST_DATASIZE_WORD) {
				MODIFY_REG(hqspi->hmdma->Instance->CTCR, (MDMA_CTCR_DINC | MDMA_CTCR_DINCOS), MDMA_DEST_INC_WORD);
			} else {
				/* in case of incorrect destination data size */
				hqspi->ErrorCode |= HAL_QSPI_ERROR_DMA;
				status = HAL_ERROR;
			}
			/* Configure QSPI: CCR register with functional as indirect read */
			MODIFY_REG(hqspi->Instance->CCR, QUADSPI_CCR_FMODE, QSPI_FUNCTIONAL_MODE_INDIRECT_READ);

			/* Start the transfer by re-writing the address in AR register */
			WRITE_REG(hqspi->Instance->AR, addr_reg);

			/* Enable the QSPI transfer error Interrupt */
			__HAL_QSPI_ENABLE_IT(hqspi, QSPI_IT_TE);

			/* Use DMAEN bit with no impact on H7 HW to record MDMA transfer request */
			SET_BIT(hqspi->Instance->CR, QUADSPI_CR_DMAEN);

			/* Enable the MDMA */
			if (HAL_MDMA_Start_IT(hqspi->hmdma, (uint32_t)&hqspi->Instance->DR, (uint32_t)pData, hqspi->RxXferSize, 1) == HAL_OK) {
			} else {
				/* Clear DMAEN bit with no impact on H7 HW to cancel MDMA transfer request */
				CLEAR_BIT(hqspi->Instance->CR, QUADSPI_CR_DMAEN);

				status = HAL_ERROR;
				hqspi->ErrorCode |= HAL_QSPI_ERROR_DMA;
				hqspi->State = HAL_QSPI_STATE_READY;
			}
		} else {
			hqspi->ErrorCode |= HAL_QSPI_ERROR_INVALID_PARAM;
			status = HAL_ERROR;
		}
	} else {
		status = HAL_BUSY;
	}

	return status;
}

uint8_t w25qxx_interface_spi_qspi_write_read(uint8_t instruction, uint8_t instruction_line,
	uint32_t address, uint8_t address_line, uint8_t address_len,
	uint32_t alternate, uint8_t alternate_line, uint8_t alternate_len,
	uint8_t dummy, uint8_t *in_buf, uint32_t in_len,
	uint8_t *out_buf, uint32_t out_len, uint8_t data_line)
{
	QSPI_CommandTypeDef cmd_handler;
	u32 val;
	int ret;

	if ((in_len != 0) && (out_len != 0)) {
		return 2;
	}

	cmd_handler.Instruction = instruction;
	if (instruction_line == 0) {
		cmd_handler.InstructionMode = QSPI_INSTRUCTION_NONE;
	} else if (instruction_line == 1) {
		cmd_handler.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	} else if (instruction_line == 2) {
		cmd_handler.InstructionMode = QSPI_INSTRUCTION_2_LINES;
	} else if (instruction_line == 4) {
		cmd_handler.InstructionMode = QSPI_INSTRUCTION_4_LINES;
	} else {
		return 2;
	}

	if (address_line == 0) {
		cmd_handler.AddressMode = QSPI_ADDRESS_NONE;
	} else if (address_line == 1) {
		cmd_handler.AddressMode = QSPI_ADDRESS_1_LINE;
	} else if (address_line == 2) {
		cmd_handler.AddressMode = QSPI_ADDRESS_2_LINES;
	} else if (address_line == 4) {
		cmd_handler.AddressMode = QSPI_ADDRESS_4_LINES;
	} else {
		return 2;
	}

	if (address_len == 0) {
		cmd_handler.Address = address & 0xFF;
		cmd_handler.AddressSize = QSPI_ADDRESS_8_BITS;
	} else if (address_len == 1) {
		cmd_handler.Address = address & 0xFF;
		cmd_handler.AddressSize = QSPI_ADDRESS_8_BITS;
	} else if (address_len == 2) {
		cmd_handler.Address = address & 0xFFFF;
		cmd_handler.AddressSize = QSPI_ADDRESS_16_BITS;
	} else if (address_len == 3) {
		cmd_handler.Address = address & 0xFFFFFF;
		cmd_handler.AddressSize = QSPI_ADDRESS_24_BITS;
	} else if (address_len == 4) {
		cmd_handler.Address = address & 0xFFFFFFFF;
		cmd_handler.AddressSize = QSPI_ADDRESS_32_BITS;
	} else {
		return 2;
	}

	cmd_handler.AlternateBytes = alternate;
	if (alternate_line == 0) {
		cmd_handler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	} else if (alternate_line == 1) {
		cmd_handler.AlternateByteMode = QSPI_ALTERNATE_BYTES_1_LINE;
	} else if (alternate_line == 2) {
		cmd_handler.AlternateByteMode = QSPI_ALTERNATE_BYTES_2_LINES;
	} else if (alternate_line == 4) {
		cmd_handler.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
	} else {
		return 2;
	}

	if (alternate_len == 0) {
		cmd_handler.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
	} else if (alternate_len == 1) {
		cmd_handler.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
	} else if (alternate_len == 2) {
		cmd_handler.AlternateBytesSize = QSPI_ALTERNATE_BYTES_16_BITS;
	} else if (alternate_len == 3) {
		cmd_handler.AlternateBytesSize = QSPI_ALTERNATE_BYTES_24_BITS;
	} else if (alternate_len == 4) {
		cmd_handler.AlternateBytesSize = QSPI_ALTERNATE_BYTES_32_BITS;
	} else {
		return 2;
	}

	cmd_handler.DummyCycles = dummy;

	if (data_line == 0) {
		cmd_handler.DataMode = QSPI_DATA_NONE;
	} else if (data_line == 1) {
		cmd_handler.DataMode = QSPI_DATA_1_LINE;
	} else if (data_line == 2) {
		cmd_handler.DataMode = QSPI_DATA_2_LINES;
	} else if (data_line == 4) {
		cmd_handler.DataMode = QSPI_DATA_4_LINES;
	} else {
		return 2;
	}

	cmd_handler.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	cmd_handler.DdrMode = QSPI_DDR_MODE_DISABLE;
	cmd_handler.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;

	if (!flash_wait_for_completion) {
		ret = readl_poll_timeout((volatile u32 *)&hqspi.State, val,
			val == HAL_QSPI_STATE_READY, 100, 1000000);
		if (ret < 0) {
			HAL_MDMA_Abort(&hmdma_quadspi_fifo_th);
			return 1;
		}
	}

	if (HAL_QSPI_Command_Custom(&hqspi, &cmd_handler, 1000) != HAL_OK) {
		return 1;
	}

	if (in_len != 0) {
		hqspi.Instance->DLR = in_len - 1;
		if (HAL_QSPI_Transmit_DMA_Custom(&hqspi, in_buf) != HAL_OK) {
			return 1;
		}
		ret = readl_poll_timeout((volatile u32 *)&hqspi.State, val,
			val == HAL_QSPI_STATE_READY, 100, 1000000);
		if (ret < 0) {
			HAL_MDMA_Abort(&hmdma_quadspi_fifo_th);
			return 1;
		}
	}

	if (out_len != 0) {
		hqspi.Instance->DLR = out_len - 1;
		if (HAL_QSPI_Receive_DMA_Custom(&hqspi, out_buf) != HAL_OK) {
			return 1;
		}
		if (flash_wait_for_completion) {
			ret = readl_poll_timeout((volatile u32 *)&hqspi.State, val,
				val == HAL_QSPI_STATE_READY, 100, 1000000);
			if (ret < 0) {
				HAL_MDMA_Abort(&hmdma_quadspi_fifo_th);
				return 1;
			}
		}
	}

	return 0;
}

void w25qxx_interface_delay_ms(uint32_t ms)
{
	mdelay(ms);
}

void w25qxx_interface_delay_us(uint32_t us)
{
	udelay(us);
}

void w25qxx_interface_debug_print(const char *const fmt, ...)
{
	pr_debug("%s", fmt);
}

w25qxx_handle_t *ld_w25qxx_handle;

static int spi7_cs_low(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_11, GPIO_PIN_RESET);
	return 0;
}

static int spi7_cs_high(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_11, GPIO_PIN_SET);
	return 0;
}

static int spi7_mosi_low(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);
	return 0;
}

static int spi7_mosi_high(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET);
	return 0;
}

static int spi7_miso_read(void)
{
	return HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_12) ? 1 : 0;
}

static int spi7_sck_low(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);
	return 0;
}

static int spi7_sck_high(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
	return 0;
}

static int spi7_master_init(void)
{
	return 0;
}

struct spi_master spi7_master = {
	.cs_low      = spi7_cs_low,
	.cs_high     = spi7_cs_high,
	.mosi_low    = spi7_mosi_low,
	.mosi_high   = spi7_mosi_high,
	.miso_read   = spi7_miso_read,
	.sck_low     = spi7_sck_low,
	.sck_high    = spi7_sck_high,
	.init        = spi7_master_init,
};

u8 nrf24l01_interface_spi_init(void)
{
	return spi_master_soft_init(&spi7_master, 0, 0, SPI_BIT_ORDER_MSB, 10);
}

u8 nrf24l01_interface_spi_deinit(void)
{
	return 0;
}

u8 nrf24l01_interface_spi_read(u8 reg, u8 *buf, uint16_t len)
{
	return spi_master_port_receive(&spi7_master, reg, buf, (u8)len);
}

u8 nrf24l01_interface_spi_write(u8 reg, u8 *buf, uint16_t len)
{
	return spi_master_port_transmit(&spi7_master, reg, buf, (u8)len);
}

u8 nrf24l01_interface_gpio_init(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);

	return 0;
}

u8 nrf24l01_interface_gpio_deinit(void)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);

	return 0;
}

u8 nrf24l01_interface_gpio_write(u8 value)
{
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, value ? GPIO_PIN_SET : GPIO_PIN_RESET);

	return 0;
}

void nrf24l01_interface_delay_ms(uint32_t ms)
{
	mdelay(ms);
}

void nrf24l01_interface_debug_print(const char *const fmt, ...)
{
	pr_debug("%s", fmt);
}

static int usart2_init(struct serial_port *serial)
{
	stm32_uart_port[1] = serial;
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, serial->rx_buffer, SERIAL_PORT_BUFFER_SIZE);
	/* CIRCULAR mode: suppress DMA TC and HT interrupts, rely on USART IDLE only */
	__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_TC | DMA_IT_HT);

	return stm32_uart_result[1].error ? -EIO : 0;
}

static int usart2_transmit_bytes(const u8 *data, u16 size)
{
	u32 val;
	int ret;

	stm32_uart_result[1].tx_done = 0;
	stm32_uart_result[1].error = 0;

	if (HAL_UART_Transmit_DMA(&huart2, data, size) != HAL_OK) {
		stm32_uart_result[1].error = 1;
		return -EIO;
	}

	ret = readl_poll_timeout(&stm32_uart_result[1].tx_done, val, val || stm32_uart_result[1].error, 100, 100000);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int usart2_receive_cb(struct serial_port *serial)
{

	return 0;
}

static int usart2_rx_disable(void)
{
	CLEAR_BIT(huart2.Instance->CR1, USART_CR1_RE);

	return 0;
}

static int usart2_rx_enable(void)
{
	SET_BIT(huart2.Instance->CR1, USART_CR1_RE);

	return 0;
}

#if POV_RUN_MODE == POV_RUN_HOST
static int usart2_config(struct serial_port *serial, u32 baud_rate,
	u8 parity, u8 data_bits, u8 flow_control)
{
	u32 word_length;
	u32 parity_cfg;
	int retry = 3;

	/* Halt any in-flight DMA */
	HAL_UART_Abort(&huart2);

	/* Map parity: 0=None, 1=Odd, 2=Even */
	switch (parity) {
	case 1:
		parity_cfg = UART_PARITY_ODD;
		break;
	case 2:
		parity_cfg = UART_PARITY_EVEN;
		break;
	default:
		parity_cfg = UART_PARITY_NONE;
		break;
	}

	/* Map data bits */
	switch (data_bits) {
	case 7:
		word_length = UART_WORDLENGTH_7B;
		break;
	default:
		word_length = UART_WORDLENGTH_8B;
		break;
	}

	huart2.Init.BaudRate     = baud_rate;
	huart2.Init.WordLength   = word_length;
	huart2.Init.StopBits     = UART_STOPBITS_1;
	huart2.Init.Parity       = parity_cfg;
	huart2.Init.Mode         = UART_MODE_TX_RX;
	/* flow_control: 0=None, 1=XON/XOFF (handled in software), 2=RTS/CTS */
	huart2.Init.HwFlowCtl    = (flow_control == 2) ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
#if defined(USART_CR1_UE)
	huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
#endif

	while (retry--) {
		if (HAL_UART_Init(&huart2) == HAL_OK) {
			break;
		}
		HAL_Delay(10);
	}

	/* Reset buffer to clean state and restart DMA from buffer[0] */
	serial->producer = 0;
	serial->consumer = 0;
	serial->received_size = 0;
	serial->rx_complete = false;

	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, serial->rx_buffer, SERIAL_PORT_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_TC | DMA_IT_HT);

	pr_info("usart2: configured %lu baud, parity=%u\n",
		(unsigned long)baud_rate, parity);
	return 0;
}

static void usart2_irq_disable(void)
{
	HAL_NVIC_DisableIRQ(USART2_IRQn);
#if defined(STM32H743xx)
	HAL_NVIC_DisableIRQ(DMA2_Stream0_IRQn);
	HAL_NVIC_DisableIRQ(DMA2_Stream1_IRQn);
#elif defined(STM32F407xx)
	HAL_NVIC_DisableIRQ(DMA1_Stream5_IRQn);
	HAL_NVIC_DisableIRQ(DMA1_Stream6_IRQn);
#endif
}

static void usart2_irq_enable(void)
{
	HAL_NVIC_EnableIRQ(USART2_IRQn);
#if defined(STM32H743xx)
	HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
	HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
#elif defined(STM32F407xx)
	HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
#endif
}

static void usart2_set_tx(u8 state)
{
	if (state) {
		/* Release: restore TE bit (make sure transmitter is enabled) */
		SET_BIT(huart2.Instance->CR1, USART_CR1_TE);
	} else {
		/* Assert break via RQR (STM32H7 has no CR1_SBK bit) */
		SET_BIT(huart2.Instance->RQR, USART_RQR_SBKRQ);
	}
}

static void usart2_update_producer(struct serial_port *serial)
{
	serial->producer = (SERIAL_PORT_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart2_rx)) % SERIAL_PORT_BUFFER_SIZE;
}

struct serial_port_ops usart2_serial_port_ops = {
	.init			  = usart2_init,
	.disable          = usart2_rx_disable,
	.enable           = usart2_rx_enable,
	.transmit_bytes   = usart2_transmit_bytes,
	.receive_callback = usart2_receive_cb,
	.update_producer  = usart2_update_producer,
	.config           = usart2_config,
	.irq_disable      = usart2_irq_disable,
	.irq_enable       = usart2_irq_enable,
	.set_tx           = usart2_set_tx,
};

static int usart6_init(struct serial_port *serial)
{
	stm32_uart_port[2] = serial;
	HAL_UARTEx_ReceiveToIdle_DMA(&huart6, serial->rx_buffer, SERIAL_PORT_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_usart6_rx, DMA_IT_TC | DMA_IT_HT);

	return 0;
}

static int usart6_transmit_bytes(const u8 *data, u16 size)
{
	u32 val;
	int ret;

	stm32_uart_result[2].tx_done = 0;
	stm32_uart_result[2].error = 0;

	if (HAL_UART_Transmit_DMA(&huart6, data, size) != HAL_OK) {
		stm32_uart_result[2].error = 1;
		return -EIO;
	}

	ret = readl_poll_timeout(&stm32_uart_result[2].tx_done, val,
			val || stm32_uart_result[2].error, 100, 100000);
	if (ret < 0) {
		return ret;
	}

	return stm32_uart_result[2].error ? -EIO : 0;
}

static int usart6_receive_cb(struct serial_port *serial)
{
	return 0;
}

static int usart6_rx_disable(void)
{
	CLEAR_BIT(huart6.Instance->CR1, USART_CR1_RE);

	return 0;
}

static int usart6_rx_enable(void)
{
	SET_BIT(huart6.Instance->CR1, USART_CR1_RE);

	return 0;
}

static int usart6_config(struct serial_port *serial, u32 baud_rate,
	u8 parity, u8 data_bits, u8 flow_control)
{
	u32 word_length;
	u32 parity_cfg;
	int retry = 3;

	HAL_UART_Abort(&huart6);

	switch (parity) {
	case 1:
		parity_cfg = UART_PARITY_ODD;
		break;
	case 2:
		parity_cfg = UART_PARITY_EVEN;
		break;
	default:
		parity_cfg = UART_PARITY_NONE;
		break;
	}

	switch (data_bits) {
	case 7:
		word_length = UART_WORDLENGTH_7B;
		break;
	default:
		word_length = UART_WORDLENGTH_8B;
		break;
	}

	huart6.Init.BaudRate     = baud_rate;
	huart6.Init.WordLength   = word_length;
	huart6.Init.StopBits     = UART_STOPBITS_1;
	huart6.Init.Parity       = parity_cfg;
	huart6.Init.Mode         = UART_MODE_TX_RX;
	huart6.Init.HwFlowCtl    = (flow_control == 2) ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
	huart6.Init.OverSampling = UART_OVERSAMPLING_16;
#if defined(USART_CR1_UE)
	huart6.Init.ClockPrescaler = UART_PRESCALER_DIV1;
#endif

	while (retry--) {
		if (HAL_UART_Init(&huart6) == HAL_OK) {
			break;
		}
		HAL_Delay(10);
	}

	serial->producer = 0;
	serial->consumer = 0;
	serial->received_size = 0;
	serial->rx_complete = false;

	HAL_UARTEx_ReceiveToIdle_DMA(&huart6, serial->rx_buffer, SERIAL_PORT_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_usart6_rx, DMA_IT_TC | DMA_IT_HT);

	pr_info("usart6: configured %lu baud, parity=%u\n",
		(unsigned long)baud_rate, parity);
	return 0;
}

static void usart6_irq_disable(void)
{
	HAL_NVIC_DisableIRQ(USART6_IRQn);
#if defined(STM32H743xx)
	HAL_NVIC_DisableIRQ(DMA2_Stream6_IRQn);
	HAL_NVIC_DisableIRQ(DMA2_Stream7_IRQn);
#elif defined(STM32F407xx)
	HAL_NVIC_DisableIRQ(DMA2_Stream6_IRQn);
	HAL_NVIC_DisableIRQ(DMA2_Stream7_IRQn);
#endif
}

static void usart6_irq_enable(void)
{
	HAL_NVIC_EnableIRQ(USART6_IRQn);
#if defined(STM32H743xx)
	HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
	HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
#elif defined(STM32F407xx)
	HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
	HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
#endif
}

static void usart6_set_tx(u8 state)
{
	if (state) {
		SET_BIT(huart6.Instance->CR1, USART_CR1_TE);
	} else {
		SET_BIT(huart6.Instance->RQR, USART_RQR_SBKRQ);
	}
}

static void usart6_update_producer(struct serial_port *serial)
{
	serial->producer = (SERIAL_PORT_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart6_rx)) % SERIAL_PORT_BUFFER_SIZE;
}

struct serial_port_ops usart6_serial_port_ops = {
	.init			  = usart6_init,
	.disable          = usart6_rx_disable,
	.enable           = usart6_rx_enable,
	.transmit_bytes   = usart6_transmit_bytes,
	.receive_callback = usart6_receive_cb,
	.update_producer  = usart6_update_producer,
	.config           = usart6_config,
	.irq_disable      = usart6_irq_disable,
	.irq_enable       = usart6_irq_enable,
	.set_tx           = usart6_set_tx,
};

static int uart4_init(struct serial_port *serial)
{
	stm32_uart_port[3] = serial;
	HAL_UARTEx_ReceiveToIdle_DMA(&huart4, serial->rx_buffer, SERIAL_PORT_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_uart4_rx, DMA_IT_TC | DMA_IT_HT);

	return 0;
}

static int uart4_transmit_bytes(const u8 *data, u16 size)
{
	u32 val;
	int ret;

	stm32_uart_result[3].tx_done = 0;
	stm32_uart_result[3].error = 0;

	if (HAL_UART_Transmit_DMA(&huart4, data, size) != HAL_OK) {
		stm32_uart_result[3].error = 1;
		return -EIO;
	}

	ret = readl_poll_timeout(&stm32_uart_result[3].tx_done, val,
			val || stm32_uart_result[3].error, 100, 100000);
	if (ret < 0) {
		return ret;
	}

	return stm32_uart_result[3].error ? -EIO : 0;
}

static int uart4_receive_cb(struct serial_port *serial)
{
	return 0;
}

static int uart4_rx_disable(void)
{
	CLEAR_BIT(huart4.Instance->CR1, USART_CR1_RE);

	return 0;
}

static int uart4_rx_enable(void)
{
	SET_BIT(huart4.Instance->CR1, USART_CR1_RE);

	return 0;
}

static int uart4_config(struct serial_port *serial, u32 baud_rate,
	u8 parity, u8 data_bits, u8 flow_control)
{
	u32 word_length;
	u32 parity_cfg;
	int retry = 3;

	HAL_UART_Abort(&huart4);

	switch (parity) {
	case 1:
		parity_cfg = UART_PARITY_ODD;
		break;
	case 2:
		parity_cfg = UART_PARITY_EVEN;
		break;
	default:
		parity_cfg = UART_PARITY_NONE;
		break;
	}

	switch (data_bits) {
	case 7:
		word_length = UART_WORDLENGTH_7B;
		break;
	default:
		word_length = UART_WORDLENGTH_8B;
		break;
	}

	huart4.Init.BaudRate     = baud_rate;
	huart4.Init.WordLength   = word_length;
	huart4.Init.StopBits     = UART_STOPBITS_1;
	huart4.Init.Parity       = parity_cfg;
	huart4.Init.Mode         = UART_MODE_TX_RX;
	huart4.Init.HwFlowCtl    = (flow_control == 2) ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
	huart4.Init.OverSampling = UART_OVERSAMPLING_16;
#if defined(USART_CR1_UE)
	huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
#endif

	while (retry--) {
		if (HAL_UART_Init(&huart4) == HAL_OK) {
			break;
		}
		HAL_Delay(10);
	}

	serial->producer = 0;
	serial->consumer = 0;
	serial->received_size = 0;
	serial->rx_complete = false;

	HAL_UARTEx_ReceiveToIdle_DMA(&huart4, serial->rx_buffer, SERIAL_PORT_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_uart4_rx, DMA_IT_TC | DMA_IT_HT);

	pr_info("uart4: configured %lu baud, parity=%u\n",
		(unsigned long)baud_rate, parity);
	return 0;
}

static void uart4_irq_disable(void)
{
	HAL_NVIC_DisableIRQ(UART4_IRQn);
	/* DMA IRQ: verify Stream assignment in CubeMX, then fill in. */
}

static void uart4_irq_enable(void)
{
	HAL_NVIC_EnableIRQ(UART4_IRQn);
}

static void uart4_set_tx(u8 state)
{
	if (state) {
		SET_BIT(huart4.Instance->CR1, USART_CR1_TE);
	} else {
		SET_BIT(huart4.Instance->RQR, USART_RQR_SBKRQ);
	}
}

static void uart4_update_producer(struct serial_port *serial)
{
	serial->producer = (SERIAL_PORT_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart4_rx)) % SERIAL_PORT_BUFFER_SIZE;
}

struct serial_port_ops uart4_serial_port_ops = {
	.init			  = uart4_init,
	.disable          = uart4_rx_disable,
	.enable           = uart4_rx_enable,
	.transmit_bytes   = uart4_transmit_bytes,
	.receive_callback = uart4_receive_cb,
	.update_producer  = uart4_update_producer,
	.config           = uart4_config,
	.irq_disable      = uart4_irq_disable,
	.irq_enable       = uart4_irq_enable,
	.set_tx           = uart4_set_tx,
};

static int uart5_init(struct serial_port *serial)
{
	stm32_uart_port[4] = serial;
	HAL_UARTEx_ReceiveToIdle_DMA(&huart5, serial->rx_buffer, SERIAL_PORT_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_uart5_rx, DMA_IT_TC | DMA_IT_HT);

	return 0;
}

static int uart5_transmit_bytes(const u8 *data, u16 size)
{
	u32 val;
	int ret;

	stm32_uart_result[4].tx_done = 0;
	stm32_uart_result[4].error = 0;

	if (HAL_UART_Transmit_DMA(&huart5, data, size) != HAL_OK) {
		stm32_uart_result[4].error = 1;
		return -EIO;
	}

	ret = readl_poll_timeout(&stm32_uart_result[4].tx_done, val,
			val || stm32_uart_result[4].error, 100, 100000);
	if (ret < 0) {
		return ret;
	}

	return stm32_uart_result[4].error ? -EIO : 0;
}

static int uart5_receive_cb(struct serial_port *serial)
{
	return 0;
}

static int uart5_rx_disable(void)
{
	CLEAR_BIT(huart5.Instance->CR1, USART_CR1_RE);

	return 0;
}

static int uart5_rx_enable(void)
{
	SET_BIT(huart5.Instance->CR1, USART_CR1_RE);

	return 0;
}

static int uart5_config(struct serial_port *serial, u32 baud_rate,
	u8 parity, u8 data_bits, u8 flow_control)
{
	u32 word_length;
	u32 parity_cfg;
	int retry = 3;

	HAL_UART_Abort(&huart5);

	switch (parity) {
	case 1:
		parity_cfg = UART_PARITY_ODD;
		break;
	case 2:
		parity_cfg = UART_PARITY_EVEN;
		break;
	default:
		parity_cfg = UART_PARITY_NONE;
		break;
	}

	switch (data_bits) {
	case 7:
		word_length = UART_WORDLENGTH_7B;
		break;
	default:
		word_length = UART_WORDLENGTH_8B;
		break;
	}

	huart5.Init.BaudRate     = baud_rate;
	huart5.Init.WordLength   = word_length;
	huart5.Init.StopBits     = UART_STOPBITS_1;
	huart5.Init.Parity       = parity_cfg;
	huart5.Init.Mode         = UART_MODE_TX_RX;
	huart5.Init.HwFlowCtl    = (flow_control == 2) ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
	huart5.Init.OverSampling = UART_OVERSAMPLING_16;
#if defined(USART_CR1_UE)
	huart5.Init.ClockPrescaler = UART_PRESCALER_DIV1;
#endif

	while (retry--) {
		if (HAL_UART_Init(&huart5) == HAL_OK) {
			break;
		}
		HAL_Delay(10);
	}

	serial->producer = 0;
	serial->consumer = 0;
	serial->received_size = 0;
	serial->rx_complete = false;

	HAL_UARTEx_ReceiveToIdle_DMA(&huart5, serial->rx_buffer, SERIAL_PORT_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_uart5_rx, DMA_IT_TC | DMA_IT_HT);

	pr_info("uart5: configured %lu baud, parity=%u\n",
		(unsigned long)baud_rate, parity);
	return 0;
}

static void uart5_irq_disable(void)
{
	HAL_NVIC_DisableIRQ(UART5_IRQn);
	/* DMA IRQ: verify Stream assignment in CubeMX, then fill in. */
}

static void uart5_irq_enable(void)
{
	HAL_NVIC_EnableIRQ(UART5_IRQn);
}

static void uart5_set_tx(u8 state)
{
	if (state) {
		SET_BIT(huart5.Instance->CR1, USART_CR1_TE);
	} else {
		SET_BIT(huart5.Instance->RQR, USART_RQR_SBKRQ);
	}
}

static void uart5_update_producer(struct serial_port *serial)
{
	serial->producer = (SERIAL_PORT_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart5_rx)) % SERIAL_PORT_BUFFER_SIZE;
}

struct serial_port_ops uart5_serial_port_ops = {
	.init			  = uart5_init,
	.disable          = uart5_rx_disable,
	.enable           = uart5_rx_enable,
	.transmit_bytes   = uart5_transmit_bytes,
	.receive_callback = uart5_receive_cb,
	.update_producer  = uart5_update_producer,
	.config           = uart5_config,
	.irq_disable      = uart5_irq_disable,
	.irq_enable       = uart5_irq_enable,
	.set_tx           = uart5_set_tx,
};

typedef struct {
	__IO uint32_t ISR;   /*!< DMA interrupt status register */
	__IO uint32_t Reserved0;
	__IO uint32_t IFCR;  /*!< DMA interrupt flag clear register */
} DMA_Base_Registers;

static void SPI_CloseTransfer(SPI_HandleTypeDef *hspi)
{
	uint32_t itflag = hspi->Instance->SR;

	__HAL_SPI_CLEAR_EOTFLAG(hspi);
	__HAL_SPI_CLEAR_TXTFFLAG(hspi);

	/* Disable SPI peripheral */
	__HAL_SPI_DISABLE(hspi);

	/* Disable ITs */
	__HAL_SPI_DISABLE_IT(hspi, (SPI_IT_EOT | SPI_IT_TXP | SPI_IT_RXP | SPI_IT_DXP | SPI_IT_UDR | SPI_IT_OVR | \
		SPI_IT_FRE | SPI_IT_MODF));

	/* Disable Tx DMA Request */
	CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);

	/* Report UnderRun error for non RX Only communication */
	if (hspi->State != HAL_SPI_STATE_BUSY_RX) {
		if ((itflag & SPI_FLAG_UDR) != 0UL) {
			SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_UDR);
			__HAL_SPI_CLEAR_UDRFLAG(hspi);
		}
	}

	/* Report OverRun error for non TX Only communication */
	if (hspi->State != HAL_SPI_STATE_BUSY_TX) {
		if ((itflag & SPI_FLAG_OVR) != 0UL) {
			SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_OVR);
			__HAL_SPI_CLEAR_OVRFLAG(hspi);
		}

#if (USE_SPI_CRC != 0UL)
		/* Check if CRC error occurred */
		if (hspi->Init.CRCCalculation == SPI_CRCCALCULATION_ENABLE) {
			if ((itflag & SPI_FLAG_CRCERR) != 0UL) {
				SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_CRC);
				__HAL_SPI_CLEAR_CRCERRFLAG(hspi);
			}
		}
#endif /* USE_SPI_CRC */
	}

	/* SPI Mode Fault error interrupt occurred -------------------------------*/
	if ((itflag & SPI_FLAG_MODF) != 0UL) {
		SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_MODF);
		__HAL_SPI_CLEAR_MODFFLAG(hspi);
	}

	/* SPI Frame error interrupt occurred ------------------------------------*/
	if ((itflag & SPI_FLAG_FRE) != 0UL) {
		SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FRE);
		__HAL_SPI_CLEAR_FREFLAG(hspi);
	}

	hspi->TxXferCount = (uint16_t)0UL;
	hspi->RxXferCount = (uint16_t)0UL;
}

static void SPI_DMATransmitCplt(DMA_HandleTypeDef *hdma)
{
	SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)hdma->Parent;

	if (hspi->State != HAL_SPI_STATE_ABORT) {
		__HAL_SPI_ENABLE_IT(hspi, SPI_IT_EOT);
	}
}

static void SPI_DMAHalfTransmitCplt(DMA_HandleTypeDef *hdma)
{
}

static void SPI_DMAError(DMA_HandleTypeDef *hdma)
{
	SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)hdma->Parent;

	/* if DMA error is FIFO error ignore it */
	if (HAL_DMA_GetError(hdma) != HAL_DMA_ERROR_FE) {
		/* Call SPI standard close procedure */
		SPI_CloseTransfer(hspi);

		SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_DMA);
		hspi->State = HAL_SPI_STATE_READY;
		HAL_SPI_ErrorCallback(hspi);
	}
}

static void HAL_DMA_Start_IT_Custom(DMA_HandleTypeDef *hdma,
	uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
	DMA_Base_Registers *regs_dma = (DMA_Base_Registers *)hdma->StreamBaseAddress;

	/* Disable the peripheral */
	__HAL_DMA_DISABLE(hdma);

	/* Clear all interrupt flags at correct offset within the register */
	regs_dma->IFCR = 0x3FUL << (hdma->StreamIndex & 0x1FU);

	/* Clear DBM bit */
	((DMA_Stream_TypeDef *)hdma->Instance)->CR &= (uint32_t)(~DMA_SxCR_DBM);

	/* Configure DMA Stream data length */
	((DMA_Stream_TypeDef *)hdma->Instance)->NDTR = DataLength;

	/* Configure DMA Stream destination address */
	((DMA_Stream_TypeDef *)hdma->Instance)->PAR = DstAddress;

	/* Configure DMA Stream source address */
	((DMA_Stream_TypeDef *)hdma->Instance)->M0AR = SrcAddress;

	/* Enable Common interrupts*/
	MODIFY_REG(((DMA_Stream_TypeDef *)hdma->Instance)->CR,
		(DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_HT), (DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_HT));

	/* Enable the Peripheral */
	__HAL_DMA_ENABLE(hdma);
}

static HAL_StatusTypeDef HAL_SPI_Transmit_DMA_Custom(SPI_HandleTypeDef *hspi,
	const u8 *pData, u16 Size)
{
	/* Set the transaction information */
	hspi->State       = HAL_SPI_STATE_BUSY_TX;
	hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
	hspi->pTxBuffPtr  = (const uint8_t *)pData;
	hspi->TxXferSize  = Size;
	hspi->TxXferCount = Size;

	if (hspi->Init.DataSize <= SPI_DATASIZE_8BIT) {
		if (hspi->hdmatx->Init.MemDataAlignment == DMA_MDATAALIGN_HALFWORD) {
			hspi->TxXferCount = (hspi->TxXferCount + 1UL) >> 1UL;
		}
		if (hspi->hdmatx->Init.MemDataAlignment == DMA_MDATAALIGN_WORD) {
			hspi->TxXferCount = (hspi->TxXferCount + 3UL) >> 2UL;
		}
	} else if (hspi->Init.DataSize <= SPI_DATASIZE_16BIT) {
		hspi->TxXferCount = (hspi->TxXferCount + 1UL) >> 1UL;
		if (hspi->hdmatx->Init.MemDataAlignment == DMA_MDATAALIGN_WORD) {
			hspi->TxXferCount = (hspi->TxXferCount + 1UL) >> 1UL;
		}
	}

	SPI_1LINE_TX(hspi);

	/* Set the SPI TxDMA Half transfer complete callback */
	hspi->hdmatx->XferHalfCpltCallback = SPI_DMAHalfTransmitCplt;

	/* Set the SPI TxDMA transfer complete callback */
	hspi->hdmatx->XferCpltCallback = SPI_DMATransmitCplt;

	/* Set the DMA error callback */
	hspi->hdmatx->XferErrorCallback = SPI_DMAError;

	/* Set the DMA AbortCpltCallback */
	hspi->hdmatx->XferAbortCallback = NULL;

	/* Clear TXDMAEN bit*/
	CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN);

	HAL_DMA_Start_IT_Custom(hspi->hdmatx,
		(uint32_t)hspi->pTxBuffPtr, (uint32_t)&hspi->Instance->TXDR, hspi->TxXferCount);

	MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, hspi->TxXferCount);

	/* Enable Tx DMA Request */
	SET_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN);

	/* Enable the SPI Error Interrupt Bit */
	__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_UDR | SPI_IT_FRE | SPI_IT_MODF));

	/* Enable SPI peripheral */
	__HAL_SPI_ENABLE(hspi);

	/* Master transfer start */
	SET_BIT(hspi->Instance->CR1, SPI_CR1_CSTART);

	return HAL_OK;
}

int pov_spi_multi_wait_complete(u32 timeout_us)
{
	u64 timeout = basic_timer_get_us() + timeout_us;
	int ret;

	for (u8 num = 0; num < 4; num++) {
		if (stm32_spi_result[num].tx_done) {
			continue;
		}
		while (stm32_spi_result[num].tx_done == 0) {
			if (basic_timer_get_us() > timeout) {
				pr_err("stm32_spi: SPI%d transfer timed out\n", num + 1);
				return -ETIMEDOUT;
			}
			if (stm32_spi_result[num].error) {
				pr_err("stm32_spi: SPI%d transfer error\n", num + 1);
				return -EPIPE;
			}
		}
	}

	return 0;
}

int pov_spi_multi_transmit(struct pov_rotor *rotor)
{
	u8 pong;

	if (pov_spi_multi_wait_complete(1000) != 0) {
		return -ETIMEDOUT;
	}

	/* Pulse LAT pin: HIGH then LOW */
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
	udelay(1);  /* Minimum LAT pulse width */
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);

	for (u8 i = 0; i < 4; i++) {
		stm32_spi_result[i].tx_done = 0;
		stm32_spi_result[i].error = 0;
	}

	pong = rotor->prefetch_ping ^ 1;
	HAL_SPI_Transmit_DMA_Custom(&hspi1, (u8 *)&rotor->led_driver_frame[pong][rotor->prefetch_sent_index][0], POV_LED_DRIVER_GROUP_BYTES);
	HAL_SPI_Transmit_DMA_Custom(&hspi2, (u8 *)&rotor->led_driver_frame[pong][rotor->prefetch_sent_index][1], POV_LED_DRIVER_GROUP_BYTES);
	HAL_SPI_Transmit_DMA_Custom(&hspi3, (u8 *)&rotor->led_driver_frame[pong][rotor->prefetch_sent_index][2], POV_LED_DRIVER_GROUP_BYTES);
	HAL_SPI_Transmit_DMA_Custom(&hspi4, (u8 *)&rotor->led_driver_frame[pong][rotor->prefetch_sent_index][3], POV_LED_DRIVER_GROUP_BYTES);

	return 0;
}

#elif POV_RUN_MODE == POV_RUN_SLAVE

struct serial_port_ops usart2_serial_port_ops = {
	.init			  = usart2_init,
	.disable          = usart2_rx_disable,
	.enable           = usart2_rx_enable,
	.transmit_bytes   = usart2_transmit_bytes,
	.receive_callback = usart2_receive_cb,
	// 	.config           = usart2_config,
	// 	.irq_disable      = usart2_irq_disable,
	// 	.irq_enable       = usart2_irq_enable,
	// 	.set_tx           = usart2_set_tx,
};

#endif

/*********************************************************************
 * Flash Sector Sizes
 * H7: Uniform 128KB sectors (bank 1, sectors 0-7)
 *     Program via 256-bit FLASHWORD (32 bytes)
 * F4: Mixed: 4×16KB + 1×64KB + 7×128KB = 1MB
 *     Program via 32-bit word
 *********************************************************************/
#ifdef STM32H743xx
#define BL_FLASH_SECTOR_SIZE                   0x20000u  /* 128KB per sector */
#define BL_FLASH_WORD_SIZE                     32u       /* 256-bit FLASHWORD */
#define BL_FLASH_VOLTAGE_RANGE                 FLASH_VOLTAGE_RANGE_3
#define BL_FLASH_TYPEPROGRAM                   FLASH_TYPEPROGRAM_FLASHWORD

static int stm32_flash_erase(u64 addr, u32 size)
{
	FLASH_EraseInitTypeDef erase_init;
	u32 sector_error;
	u32 start_sector, end_sector, sector;

	if (addr + size > (u64)FLASH_BANK1_BASE + ROM_TOTAL_SIZE) {
		pr_err("stm32_flash_erase: addr 0x%llx+%u out of range\n",
			addr, size);
		return -EINVAL;
	}

	start_sector = (u32)(addr - FLASH_BANK1_BASE) / BL_FLASH_SECTOR_SIZE;
	end_sector   = (u32)(addr + size - 1 - FLASH_BANK1_BASE) / BL_FLASH_SECTOR_SIZE;

	HAL_FLASH_Unlock();

	for (sector = start_sector; sector <= end_sector; sector++) {
		erase_init.TypeErase     = FLASH_TYPEERASE_SECTORS;
		erase_init.Banks         = FLASH_BANK_1;
		erase_init.Sector        = sector;
		erase_init.NbSectors     = 1;
		erase_init.VoltageRange  = BL_FLASH_VOLTAGE_RANGE;

		if (HAL_FLASHEx_Erase(&erase_init, &sector_error) != HAL_OK) {
			pr_err("stm32_flash_erase: sector %u failed, err 0x%lx\n",
				sector, HAL_FLASH_GetError());
			HAL_FLASH_Lock();
			return -EIO;
		}
	}

	HAL_FLASH_Lock();
	return 0;
}

static int stm32_flash_write(u64 addr, const u8 *data, u32 size)
{
	u32 offset = 0;

	if (addr + size > (u64)FLASH_BANK1_BASE + ROM_TOTAL_SIZE) {
		pr_err("stm32_flash_write: addr 0x%llx+%u out of range\n",
			addr, size);
		return -EINVAL;
	}

	HAL_FLASH_Unlock();

	while (offset < size) {
		u32 flash_addr = (u32)(addr + offset);
		u32 word_off   = flash_addr & (BL_FLASH_WORD_SIZE - 1);
		u32 copy_sz;
		int ret;

		if (word_off != 0 ||
			(size - offset) < BL_FLASH_WORD_SIZE) {
			/* Unaligned or partial: read-modify-write */
			u32 aligned_addr = flash_addr &
				~(BL_FLASH_WORD_SIZE - 1);
			u8 buf[32];  /* 256-bit max */

			memcpy(buf, (void *)(uintptr_t)aligned_addr,
				BL_FLASH_WORD_SIZE);
			copy_sz = (size - offset < BL_FLASH_WORD_SIZE - word_off)
				? (size - offset)
				: (BL_FLASH_WORD_SIZE - word_off);
			memcpy(buf + word_off, data + offset, copy_sz);

			ret = HAL_FLASH_Program(BL_FLASH_TYPEPROGRAM,
					aligned_addr,
					(u32)(uintptr_t)buf);
		} else {
			/* Aligned full flash-word */
			ret = HAL_FLASH_Program(BL_FLASH_TYPEPROGRAM,
					flash_addr,
					(u32)(uintptr_t)(data + offset));
			copy_sz = BL_FLASH_WORD_SIZE;
		}

		if (ret != HAL_OK) {
			pr_err("stm32_flash_write: failed at 0x%08x, err 0x%lx\n",
				flash_addr, HAL_FLASH_GetError());
			HAL_FLASH_Lock();
			return -EIO;
		}

		offset += copy_sz;
	}

	HAL_FLASH_Lock();
	return 0;
}

static int stm32_flash_read(u64 addr, u8 *buf, u32 size)
{
	if (addr + size > (u64)FLASH_BANK1_BASE + ROM_TOTAL_SIZE) {
		pr_err("stm32_flash_read: addr 0x%llx+%u out of range\n",
			addr, size);
		return -EINVAL;
	}

	memcpy(buf, (void *)(uintptr_t)addr, size);
	return 0;
}

#elif defined(STM32F407xx)
#define BL_FLASH_SECTOR_SIZE_REF               0x20000u  /* default sector size reference */
#define BL_FLASH_WORD_SIZE                     4u        /* 32-bit word */
#define BL_FLASH_VOLTAGE_RANGE                 FLASH_VOLTAGE_RANGE_3
#define BL_FLASH_TYPEPROGRAM                   FLASH_TYPEPROGRAM_WORD
#define BL_F4_SECTOR_COUNT                     12

const u32 stm32_sector_size[BL_F4_SECTOR_COUNT] = {
	0x4000,  0x4000,  0x4000,  0x4000,   /* 16KB × 4 */
	0x10000,                             /* 64KB × 1 */
	0x20000, 0x20000, 0x20000,           /* 128KB × 7 */
	0x20000, 0x20000, 0x20000, 0x20000
};

static int stm32_flash_get_sector(u32 addr)
{
	u32 offset = addr - FLASH_BASE;
	u32 accum = 0;

	for (u8 i = 0; i < BL_F4_SECTOR_COUNT; i++) {
		accum += stm32_sector_size[i];
		if (offset < accum) {
			return i;
		}
	}
	return -EINVAL;
}

static int stm32_flash_erase(u64 addr, u32 size)
{
	FLASH_EraseInitTypeDef erase_init;
	u32 sector_error;
	s32 start_sector, end_sector;
	s32 sector;

	if (addr + size > (u64)FLASH_BASE + ROM_TOTAL_SIZE) {
		pr_err("stm32_flash_erase: addr 0x%llx+%u out of range\n",
			addr, size);
		return -EINVAL;
	}

	start_sector = stm32_flash_get_sector((u32)addr);
	end_sector   = stm32_flash_get_sector((u32)(addr + size - 1));

	if (start_sector < 0 || end_sector < 0) {
		pr_err("stm32_flash_erase: invalid sector range\n");
		return -EINVAL;
	}

	HAL_FLASH_Unlock();

	for (sector = start_sector; sector <= end_sector; sector++) {
		erase_init.TypeErase     = FLASH_TYPEERASE_SECTORS;
		erase_init.Banks         = FLASH_BANK_1;
		erase_init.Sector        = (u32)sector;
		erase_init.NbSectors     = 1;
		erase_init.VoltageRange  = BL_FLASH_VOLTAGE_RANGE;

		if (HAL_FLASHEx_Erase(&erase_init, &sector_error) != HAL_OK) {
			pr_err("stm32_flash_erase: sector %d failed, err 0x%lx\n",
				sector, HAL_FLASH_GetError());
			HAL_FLASH_Lock();
			return -EIO;
		}
	}

	HAL_FLASH_Lock();
	return 0;
}

static int stm32_flash_write(u64 addr, const u8 *data, u32 size)
{
	u32 offset = 0;

	if (addr + size > (u64)FLASH_BASE + ROM_TOTAL_SIZE) {
		pr_err("stm32_flash_write: addr 0x%llx+%u out of range\n",
			addr, size);
		return -EINVAL;
	}

	HAL_FLASH_Unlock();

	while (offset < size) {
		u32 flash_addr   = (u32)(addr + offset);
		u32 aligned_addr = flash_addr & ~(BL_FLASH_WORD_SIZE - 1);
		u32 word_off     = flash_addr & (BL_FLASH_WORD_SIZE - 1);
		u32 copy_sz;
		u32 data_word;
		int ret;

		copy_sz = BL_FLASH_WORD_SIZE - word_off;
		if (copy_sz > (size - offset)) {
			copy_sz = size - offset;
		}

		if (word_off != 0 || copy_sz < BL_FLASH_WORD_SIZE) {
			/* Unaligned or partial: read-modify-write */
			memcpy(&data_word, (void *)(uintptr_t)aligned_addr,
				sizeof(data_word));
			memcpy((u8 *)&data_word + word_off,
				data + offset, copy_sz);

			ret = HAL_FLASH_Program(BL_FLASH_TYPEPROGRAM,
					aligned_addr, data_word);
		} else {
			/* Aligned full word */
			memcpy(&data_word, data + offset, sizeof(data_word));
			ret = HAL_FLASH_Program(BL_FLASH_TYPEPROGRAM,
					flash_addr, data_word);
		}

		if (ret != HAL_OK) {
			pr_err("stm32_flash_write: failed at 0x%08x, err 0x%lx\n",
				flash_addr, HAL_FLASH_GetError());
			HAL_FLASH_Lock();
			return -EIO;
		}

		offset += copy_sz;
	}

	HAL_FLASH_Lock();
	return 0;
}

static int stm32_flash_read(u64 addr, u8 *buf, u32 size)
{
	if (addr + size > (u64)FLASH_BASE + ROM_TOTAL_SIZE) {
		pr_err("stm32_flash_read: addr 0x%llx+%u out of range\n",
			addr, size);
		return -EINVAL;
	}

	memcpy(buf, (void *)(uintptr_t)addr, size);
	return 0;
}

#endif /* STM32 chip selection */

struct flash_ops general_flash_ops = {
	.erase = stm32_flash_erase,
	.write = stm32_flash_write,
	.read  = stm32_flash_read,
};

static void stm32_system_reset(void)
{
	HAL_NVIC_SystemReset();
}

struct bootloader_ops general_bootloader_ops = {
	.system_reset        = stm32_system_reset,
	.flash               = &general_flash_ops,
};
#else

u8 pov_read_arm_id(void)
{
	static u8 arm_id = 0;

	arm_id = (arm_id + 1) & 0xFF;

	return arm_id - 1;
}

void hall_fake_pulse(struct tim_task *task)
{
	struct hall_device *hall_dev = container_of(task, struct hall_device, fake_pulse_task);

	/* Walk the priv chain: stator -> master arm -> slave arms.
	 * On real hardware all arms share the same physical hall signal. */
	for (struct hall_device *dev = hall_dev; dev; dev = dev->priv)
		hall_hall_isr(dev, task->interval_us);
}

#endif