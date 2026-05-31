
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/iopoll.h>

#include <kinetis/iic_soft.h>
#include <kinetis/spi_soft.h>
#include <kinetis/real-time-clock.h>
#include <kinetis/basic-timer.h>
#include <kinetis/serial-port.h>

#include "../../drivers/kinetis/bootloader/bootloader.h"

#include "hall.h"
#include "rotor.h"

#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "rtc.h"
#include "tim.h"
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

unsigned long read_chip_timer(void)
{
	return TIM5->CNT;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) {
        xtime_update(1);
    } else if (htim->Instance == TIM5) {
        basic_timer_isr(1000);
    }
}

struct hall_device *hall_dev;

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        	hall_hall_isr(hall_dev, HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1));
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
	if (hi2c->Instance == hi2c1.Instance)
		stm32_i2c_result[0].tx_done = 1;
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == hi2c1.Instance)
		stm32_i2c_result[0].rx_done = 1;
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == hi2c1.Instance) {
		pr_err("stm32_i2c: DMA error 0x%lx\n", hi2c->ErrorCode);
		stm32_i2c_result[0].error = 1;
	}
}

static struct transfer_complete_result stm32_spi_result[4];

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == hspi1.Instance)
		stm32_spi_result[0].tx_done = 1;
#if POV_RUN_MODE == POV_RUN_HOST
	else if (hspi->Instance == hspi2.Instance)
		stm32_spi_result[1].tx_done = 1;
	else if (hspi->Instance == hspi3.Instance)
		stm32_spi_result[2].tx_done = 1;
	else if (hspi->Instance == hspi4.Instance)
		stm32_spi_result[3].tx_done = 1;
#endif
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == hspi1.Instance)
		stm32_spi_result[0].rx_done = 1;
#if POV_RUN_MODE == POV_RUN_HOST
	else if (hspi->Instance == hspi2.Instance)
		stm32_spi_result[1].rx_done = 1;
	else if (hspi->Instance == hspi3.Instance)
		stm32_spi_result[2].rx_done = 1;
	else if (hspi->Instance == hspi4.Instance)
		stm32_spi_result[3].rx_done = 1;
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

static struct transfer_complete_result stm32_uart_result[3];

struct serial_port *stm32_serial_port[5];

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == huart1.Instance)
		stm32_uart_result[0].tx_done = 1;
	else if (huart->Instance == huart2.Instance)
		stm32_uart_result[1].tx_done = 1;
#if POV_RUN_MODE == POV_RUN_HOST
	else if (huart->Instance == huart6.Instance)
		stm32_uart_result[2].tx_done = 1;
#endif
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, u16 Size)
{
	if (huart->Instance == huart2.Instance) {
		stm32_serial_port[0]->producer = Size % SERIAL_PORT_BUFFER_SIZE;
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == huart2.Instance) {
		pr_err("stm32_uart: DMA error 0x%lx, restarting RX\n", huart->ErrorCode);
		HAL_UART_AbortReceive(&huart2);
		HAL_UART_Init(&huart2);
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, stm32_serial_port[0]->rx_buffer,
					     SERIAL_PORT_BUFFER_SIZE);
		__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_TC);
		// ATOMIC_CLEAR_BIT(huart2.Instance->CR3, USART_CR3_EIE);
		stm32_uart_result[1].error = 1;
	}
}

int fputc(int ch, FILE *f)
{
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART3 and Loop until the end of transmission */
	if (HAL_UART_Transmit(&huart1, (u8 *)&ch, 1, 100) != HAL_OK)
		return -1;

	return ch;
}

int _putc(int ch)
{
	if (HAL_UART_Transmit(&huart1, (u8 *)&ch, 1, 100) != HAL_OK)
		return -1;

	return ch;
}

int _puts(char *string, int cnt)
{
	/* Output cnt characters from string to console */
	if (!string || cnt <= 0)
		return 0;

	if (HAL_UART_Transmit(&huart1, (u8 *)string, cnt, 100) != HAL_OK)
		return 0;

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

struct iic_master general_iic_master = {
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
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	return 0;
}

static int spi1_cs_high(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	return 0;
}

static int spi1_mosi_low(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
	return 0;
}

static int spi1_mosi_high(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
	return 0;
}

static int spi1_miso_read(void)
{
	return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) ? 1 : 0;
}

static int spi1_sck_low(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	return 0;
}

static int spi1_sck_high(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	return 0;
}

static int spi1_write_bytes(u8 *pdata, u8 length)
{
	u32 val;
	int ret;

	stm32_spi_result[0].tx_done = 0;
	stm32_spi_result[0].error = 0;

	HAL_SPI_Transmit_DMA(&hspi1, pdata, length);

	ret = readl_poll_timeout(&stm32_spi_result[0].tx_done, val, val || stm32_spi_result[0].error, 100, 100000);
	if (ret < 0) {
		HAL_SPI_Abort(&hspi1);
		pr_err("stm32_spi: write timed out (%u bytes)\n", length);
		return ret;
	}

	return 0;
}

static int spi1_read_bytes(u8 *pdata, u8 length)
{
	u32 val;
	int ret;

	stm32_spi_result[0].rx_done = 0;
	stm32_spi_result[0].error = 0;

	HAL_SPI_Receive_DMA(&hspi1, pdata, length);

	ret = readl_poll_timeout(&stm32_spi_result[0].rx_done, val, val || stm32_spi_result[0].error, 100, 100000);
	if (ret < 0) {
		HAL_SPI_Abort(&hspi1);
		pr_err("stm32_spi: read timed out (%u bytes)\n", length);
		return ret;
	}

	return 0;
}

static int spi1_master_init(void)
{
	return 0;
}

struct spi_master general_spi_master = {
	.cs_low      = spi1_cs_low,
	.cs_high     = spi1_cs_high,
	.mosi_low    = spi1_mosi_low,
	.mosi_high   = spi1_mosi_high,
	.miso_read   = spi1_miso_read,
	.sck_low     = spi1_sck_low,
	.sck_high    = spi1_sck_high,
	.write_bytes = spi1_write_bytes,
	.read_bytes  = spi1_read_bytes,
	.init        = spi1_master_init,
};

static int usart2_init(struct serial_port *serial)
{
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, serial->rx_buffer, SERIAL_PORT_BUFFER_SIZE);
	/* CIRCULAR mode: suppress DMA TC interrupt, rely on USART IDLE only */
	__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_TC);

	return 0;
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
	if (ret < 0)
		return ret;

	return 0;
}

static int usart2_receive_cb(struct serial_port *serial)
{

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
	case 1:  parity_cfg = UART_PARITY_ODD;  break;
	case 2:  parity_cfg = UART_PARITY_EVEN; break;
	default: parity_cfg = UART_PARITY_NONE; break;
	}

	/* Map data bits */
	switch (data_bits) {
	case 7:  word_length = UART_WORDLENGTH_7B; break;
	default: word_length = UART_WORDLENGTH_8B; break;
	}

	huart2.Init.BaudRate     = baud_rate;
	huart2.Init.WordLength   = word_length;
	huart2.Init.StopBits     = UART_STOPBITS_1;
	huart2.Init.Parity       = parity_cfg;
	huart2.Init.Mode         = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl    = flow_control ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
#if defined(USART_CR1_UE)
	huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
#endif

	while (retry--) {
		if (HAL_UART_Init(&huart2) == HAL_OK)
			break;
		HAL_Delay(10);
	}

	/* Reset buffer to clean state and restart DMA from buffer[0] */
	serial->producer = 0;
	serial->consumer = 0;
	serial->received_size = 0;
	serial->rx_complete = false;

	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, serial->rx_buffer, SERIAL_PORT_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_TC);

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

struct serial_port_ops stm32_usart2_ops = {
	.init			  = usart2_init,
	.transmit_bytes   = usart2_transmit_bytes,
	.receive_callback = usart2_receive_cb,
	.config           = usart2_config,
	.irq_disable      = usart2_irq_disable,
	.irq_enable       = usart2_irq_enable,
	.set_tx           = usart2_set_tx,
};

struct serial_port_ops stm32_usart6_ops = {
	.init			  = usart2_init,
	.transmit_bytes   = usart2_transmit_bytes,
	.receive_callback = usart2_receive_cb,
	.config           = usart2_config,
	.irq_disable      = usart2_irq_disable,
	.irq_enable       = usart2_irq_enable,
	.set_tx           = usart2_set_tx,
};

#elif POV_RUN_MODE == POV_RUN_SLAVE

struct serial_port_ops stm32_usart2_ops = {
	.init			  = usart2_init,
	.transmit_bytes   = usart2_transmit_bytes,
	.receive_callback = usart2_receive_cb,
// 	.config           = usart2_config,
// 	.irq_disable      = usart2_irq_disable,
// 	.irq_enable       = usart2_irq_enable,
// 	.set_tx           = usart2_set_tx,
};

#endif

#if POV_RUN_MODE == POV_RUN_HOST

#define STM32_FLASH_SECTOR_SIZE  0x20000u
#define STM32_FLASH_WORD_SIZE    32u

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

	start_sector = (addr - FLASH_BANK1_BASE) / STM32_FLASH_SECTOR_SIZE;
	end_sector   = (addr + size - 1 - FLASH_BANK1_BASE) /  STM32_FLASH_SECTOR_SIZE;

	HAL_FLASH_Unlock();

	for (sector = start_sector; sector <= end_sector; sector++) {
		erase_init.TypeErase     = FLASH_TYPEERASE_SECTORS;
		erase_init.Banks         = FLASH_BANK_1;
		erase_init.Sector        = sector;
		erase_init.NbSectors     = 1;
		erase_init.VoltageRange  = FLASH_VOLTAGE_RANGE_3; /* 32-bit */

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
		u32 word_off   = flash_addr & (STM32_FLASH_WORD_SIZE - 1);
		u32 copy_sz;
		int ret;

		if (word_off != 0 ||
		    (size - offset) < STM32_FLASH_WORD_SIZE) {
			/* Unaligned or partial: read-modify-write */
			u32 aligned_addr = flash_addr &
					   ~(STM32_FLASH_WORD_SIZE - 1);
			u8 buf[STM32_FLASH_WORD_SIZE];

			memcpy(buf, (void *)aligned_addr,
			       STM32_FLASH_WORD_SIZE);
			copy_sz = (size - offset < STM32_FLASH_WORD_SIZE - word_off) ? (size - offset) : (STM32_FLASH_WORD_SIZE - word_off);
			memcpy(buf + word_off, data + offset, copy_sz);

			ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
						aligned_addr, (u32)buf);
		} else {
			/* Aligned full flash-word */
			ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
						flash_addr,
						(u32)(data + offset));
			copy_sz = STM32_FLASH_WORD_SIZE;
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

	memcpy(buf, (u8 *)addr, size);
	
	return 0;
}

static void stm32_disable_irq(void)
{
	__disable_irq();
}

static void stm32_cleanup_hw_resource(void)
{
	/*
	 * De-initialize all HAL peripherals that were initialized during
	 * board bring-up so the new firmware starts with a clean slate.
	 *
	 * NOTE: actual peripheral handles (e.g. huart1, hi2c1) are
	 * generated by STM32CubeMX in main.h / main.c — call their
	 * MX_*_DeInit() or HAL_*_DeInit() as needed per project.
	 */
	HAL_RCC_DeInit();
}

static void stm32_disable_cache(void)
{
	SCB_DisableDCache();
	SCB_DisableICache();
}

static void stm32_set_stack_pointer(u64 ptr)
{
	__set_MSP((u32)ptr);
}

static void stm32_redirect_isr_table(u64 addr)
{
	SCB->VTOR = (u32)addr;
}

static void stm32_system_reset(void)
{
	HAL_NVIC_SystemReset();
}

struct flash_ops general_flash_ops = {
	.erase                = stm32_flash_erase,
	.write                = stm32_flash_write,
	.read                 = stm32_flash_read,
};

struct bootloader_ops general_bl_ops = {
	.disable_irq          = stm32_disable_irq,
	.cleanup_hw_resource  = stm32_cleanup_hw_resource,
	.disable_cache        = stm32_disable_cache,
	.set_stack_pointer    = stm32_set_stack_pointer,
	.redirect_isr_table   = stm32_redirect_isr_table,
	.system_reset         = stm32_system_reset,
};

int pov_spi_multi_wait_complete(u32 timeout_us)
{
	u64 timeout = basic_timer_get_us() + timeout_us;
	int ret;

	for (u8 num = 0; num < 4; num++) {
		if (stm32_spi_result[num].tx_done)
			continue;
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
	if (pov_spi_multi_wait_complete(1000) != 0)
		return -ETIMEDOUT;

	/* Pulse LAT pin: HIGH then LOW */
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
	udelay(1);  /* Minimum LAT pulse width */
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);

	for (u8 i = 0; i < 4; i++) {
		stm32_spi_result[i].tx_done = 0;
		stm32_spi_result[i].error = 0;
	}

	HAL_SPI_Transmit_DMA(&hspi1, (u8 *)rotor->rgb_strip[0], POV_BYTES_PER_GROUP);
	HAL_SPI_Transmit_DMA(&hspi2, (u8 *)rotor->rgb_strip[1], POV_BYTES_PER_GROUP);
	HAL_SPI_Transmit_DMA(&hspi3, (u8 *)rotor->rgb_strip[2], POV_BYTES_PER_GROUP);
	HAL_SPI_Transmit_DMA(&hspi4, (u8 *)rotor->rgb_strip[3], POV_BYTES_PER_GROUP);

	return 0;
}

#elif POV_RUN_MODE == POV_RUN_SLAVE

#endif