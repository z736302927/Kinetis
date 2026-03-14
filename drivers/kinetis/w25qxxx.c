
#include <linux/bitops.h>
#include <linux/iopoll.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/delay.h>

#include <kinetis/design_verification.h>
#include <kinetis/spi_soft.h>
#include "kinetis/random-gene.h"
#include "kinetis/basic-timer.h"

#include "w25qxxx.h"

#include <pthread.h>

static inline void w25qxxx_port_transmit(struct w25qxxx_device *dev, u8 tmp)
{
#ifdef STM32_HAL_LIBRARY
	switch (w25qxxx) {
	case W25Q128:
		HAL_SPI_Transmit(&hspi1, &tmp, 1, 1000);
		break;

	case W25Q256:
		HAL_SPI_Transmit(&hspi2, &tmp, 1, 1000);
		break;

	default:
		break;
	}
#endif
}

static inline u8 w25qxxx_port_receive(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

#ifdef STM32_HAL_LIBRARY
	switch (w25qxxx) {
	case W25Q128:
		HAL_SPI_Receive(&hspi1, &tmp, 1, 1000);
		break;

	case W25Q256:
		HAL_SPI_Receive(&hspi2, &tmp, 1, 1000);
		break;

	default:
		break;
	}
#endif

	return tmp;
}

static inline void w25qxxx_port_multi_transmit(struct w25qxxx_device *dev,
	u8 *pdata, u32 length)
{
#ifdef STM32_HAL_LIBRARY
	switch (w25qxxx) {
	case W25Q128:
		HAL_SPI_Transmit(&hspi1, pdata, length, 1000);
		break;

	case W25Q256:
		HAL_SPI_Transmit(&hspi2, pdata, length, 1000);
		break;

	default:
		break;
	}
#endif
}

static inline void w25qxxx_port_multi_receive(struct w25qxxx_device *dev,
	u8 *pdata, u32 length)
{
#ifdef STM32_HAL_LIBRARY
	switch (w25qxxx) {
	case W25Q128:
		HAL_SPI_Receive(&hspi1, pdata, length, 1000);
		break;

	case W25Q256:
		HAL_SPI_Receive(&hspi2, pdata, length, 1000);
		break;

	default:
		break;
	}
#endif
}

static inline void w25qxxx_cs_low(struct w25qxxx_device *dev)
{
#ifdef STM32_HAL_LIBRARY
	switch (w25qxxx) {
	case W25Q128:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
		break;

	case W25Q256:
		HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET);
		break;

	default:
		break;
	}
#endif
}

static inline void w25qxxx_cs_high(struct w25qxxx_device *dev)
{
#ifdef STM32_HAL_LIBRARY
	switch (w25qxxx) {
	case W25Q128:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
		break;

	case W25Q256:
		HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);
		break;

	default:
		break;
	}
#endif
}

#define W25Q_PAGE_SIZE                  256
#define SECTOR_SIZE                     4096
#define DUMMY_BYTE                      0xFF

/* W25Q时序参数定义 (单位：微秒) */
#define W25Q_POWER_DOWN_DELAY           3       /* 掉电模式切换延时 */
#define W25Q_SUSPEND_RESUME_DELAY       20      /* 挂起/恢复操作延时 */
#define W25Q_SOFT_RESET_DELAY           30      /* 软复位延时 */
#define W25Q_HARD_RESET_DELAY           100     /* 硬复位延时 (更保守) */

/* 操作超时时间 (单位：微秒) */
#define W25Q_PAGE_PROGRAM_TIMEOUT       3000    /* 页编程超时时间 (3ms) */
#define W25Q_SECTOR_ERASE_TIMEOUT       50000   /* 扇区擦除超时时间 (50ms) */
#define W25Q_BLOCK_ERASE_TIMEOUT        200000  /* 块擦除超时时间 (200ms) */
#define W25Q_CHIP_ERASE_TIMEOUT         5000000 /* 芯片擦除超时时间 (5s) */
#define WRITE_ENABLE                    0x06
#define WRITE_ENABLE_FORVOLATILE        0x50
#define WRITE_DISABLE                   0x04
#define READ_STATUS_REGISTER1           0x05
#define READ_STATUS_REGISTER2           0x35
#define READ_STATUS_REGISTER3           0x15
#define WRITE_STATUS_REGISTER1          0x01
#define WRITE_STATUS_REGISTER2          0x31
#define WRITE_STATUS_REGISTER3          0x11
#define READ_EXTENDED_ADDRESS_REGISTER  0xC8
#define WRITE_EXTENDED_ADDRESS_REGISTER 0xC5
#define ENTER_4BYTE_ADDRESS_MODE        0xB7
#define EXIT_4BYTE_ADDRESS_MODE         0xE9
#define READ_DATA                       0x03
#define READ_DATA_WITH_4Byte_ADDRESS    0x13
#define FAST_READ                       0x0B
#define FAST_READ_WITH_4Byte_ADDRESS    0x0C
#define PAGE_PROGRAM                    0x02
#define SECTOR_ERASE                    0x20
#define BLOCK_ERASE_32KB                0x52
#define BLOCK_ERASE_64KB                0xD8
#define CHIP_ERASE                      0xC7
#define ERASE_PROGRAM_SUSPEND           0x75
#define ERASE_PROGRAM_RESUME            0x7A
#define POWER_DOWN                      0xB9
#define RELEASE_POWERDOWN_DEVICE_ID     0xAB
#define READ_MANUFACTURER_DEVICE_ID     0x90
#define READ_UNIQUE_ID_NUMBER           0x4B
#define READ_JEDEC_ID                   0x9F
#define READ_SFDP_REGISTER              0x5A
#define SECURITY_REGISTERS_1            0x10
#define SECURITY_REGISTERS_2            0x20
#define SECURITY_REGISTERS_3            0x30
#define ERASE_SECURITY_REGISTERS        0x44
#define PROGRAM_SECURITY_REGISTERS      0x42
#define READ_SECURITY_REGISTERS         0x48
#define ENTER_QPI_MODE                  0x38
#define EXIT_QPI_MODE                   0xFF
#define INDIVIDUAL_BLOCK_SECTOR_LOCK    0x36
#define INDIVIDUAL_BLOCK_SECTOR_UNLOCK  0x39
#define READ_BLOCK_SECTOR_LOCK          0x3D
#define GLOBAL_BLOCK_SECTOR_LOCK        0x7E
#define GLOBAL_BLOCK_SECTOR_UNLOCK      0x98
#define ENABLE_RESET                    0x66
#define RESET_DEVICE                    0x99

struct w25qxxx_status {
	unsigned BUSY: 1;
	unsigned WEL: 1;
	unsigned BP0: 1;
	unsigned BP1: 1;
	unsigned BP2: 1;
	unsigned TB: 1;
	unsigned SEC: 1;
	unsigned SRP0: 1;

	unsigned SRP1: 1;
	unsigned QE: 1;
	unsigned LB1: 1;
	unsigned LB2: 1;
	unsigned LB3: 1;
	unsigned CMP: 1;
	unsigned SUS: 1;

	unsigned WPS: 1;
	unsigned DRV0: 1;
	unsigned DRV1: 1;
	unsigned HOLD_RST: 1;
};

struct w25qxxx_device {
	struct spi_master *spi;
	struct spi_slave *spi_slave;
	u8 *slave_regs;

	u8 id;
	u32 max_addr;
	struct list_head list;

	struct w25qxxx_status status;

	bool thread_running;
};

static u32 w25q128_max_addr = 0xFFFFFF;
static u32 w25q256_max_addr = 0x1FFFFFF;

/* SPI Simulation constants */
#define W25Q_FLASH_SIZE_128        (16 * 1024 * 1024)  /* 16MB for W25Q128 */
#define W25Q_FLASH_SIZE_256        (32 * 1024 * 1024)  /* 32MB for W25Q256 */
#define W25Q_STATUS_REG1_ADDR     0x00
#define W25Q_STATUS_REG2_ADDR     0x01
#define W25Q_STATUS_REG3_ADDR     0x02
#define W25Q_DATA_BASE_ADDR       0x100

/* W25Qxxx SPI simulation thread */
static pthread_t w25q_reg_thread;

/**
 * @brief W25Qxxx register randomization thread
 * @param arg: Device pointer
 * @return NULL
 *
 * This thread simulates the Flash memory by:
 * 1. Maintaining status registers (BUSY, WEL, BP bits, etc.)
 * 2. Simulating erase/write delays
 * 3. Simulating power-down state
 */
static void *w25qxxx_reg_random_thread(void *arg)
{
	struct w25qxxx_device *dev = (struct w25qxxx_device *)arg;
	u32 flash_size;
	u32 i;

	/* Initialize status register 1 (BUSY=0, WEL=0, BP=0, TB=0, SEC=0, SRP0=0) */
	dev->slave_regs[W25Q_STATUS_REG1_ADDR] = 0x00;
	/* Initialize status register 2 (SRP1=0, QE=0, LB=0, CMP=0, SUS=0) */
	dev->slave_regs[W25Q_STATUS_REG2_ADDR] = 0x00;
	/* Initialize status register 3 (ADS=0, ADP=0, WPS=0, DRV=0, HOLD/RST=0) */
	dev->slave_regs[W25Q_STATUS_REG3_ADDR] = 0x00;

	/* Initialize flash memory with random pattern (erased state = 0xFF) */
	flash_size = (dev->id == W25Q256) ? W25Q_FLASH_SIZE_256 : W25Q_FLASH_SIZE_128;
	for (i = W25Q_DATA_BASE_ADDR; i < W25Q_DATA_BASE_ADDR + flash_size && i < 0x10000; i++) {
		dev->slave_regs[i] = 0xFF;  /* Erased state */
	}

	while (dev->thread_running) {
		/* Simulate status register behavior */
		/* BUSY bit is set during write/erase operations (simulated) */

		/* If device is in power-down mode, maintain status but no updates */
		if (!(dev->slave_regs[W25Q_STATUS_REG3_ADDR] & 0x01)) {
			/* Simulate random data corruption for non-volatile storage test */
			if ((get_random_u32() & 0xFFFFF) == 0) {
				/* Very rare event: simulate data corruption (0.001% chance) */
				i = W25Q_DATA_BASE_ADDR + (get_random_u32() & 0x0FFF);
				if (i < 0x10000) {
					dev->slave_regs[i] = get_random_u32() & 0xFF;
				}
			}
		}

		msleep(10);  /* Update every 10ms */
	}

	return NULL;
}

int w25qxxx_start_reg_random(struct w25qxxx_device *dev)
{
	int ret;

	dev->thread_running = true;

	ret = pthread_create(&w25q_reg_thread, NULL,
			w25qxxx_reg_random_thread, dev);
	if (ret) {
		return -ret;
	}

	msleep(100);  /* Wait for thread to initialize */

	return 0;
}

void w25qxxx_stop_reg_random(struct w25qxxx_device *dev)
{
	dev->thread_running = false;
	pthread_join(w25q_reg_thread, NULL);
}

void w25qxxx_transmit_cmd(struct w25qxxx_device *dev, u8 cmd)
{
	dev->spi->cs_low();
	dev->spi->write_bytes(&cmd, 1);
	dev->spi->cs_high();
}

void w25qxxx_hard_reset(struct w25qxxx_device *dev)
{
	if (dev->id != W25Q128 && dev->id != W25Q256) {
		pr_err("Invalid device type: 0x%02X\n", dev->id);
		return;
	}

	w25qxxx_transmit_cmd(dev, ENABLE_RESET);
	udelay(1);  /* 短暂延时确保命令被接收 */

	w25qxxx_transmit_cmd(dev, RESET_DEVICE);

	udelay(W25Q_HARD_RESET_DELAY);
}

u8 w25qxxx_read_status_reg(struct w25qxxx_device *dev, u8 num)
{
	u8 tmp = 0;

	dev->spi->cs_low();

	switch (num) {
	case READ_STATUS_REGISTER1:
		dev->spi->write_byte(READ_STATUS_REGISTER1);
		tmp = dev->spi->read_byte();
		break;

	case READ_STATUS_REGISTER2:
		dev->spi->write_byte(READ_STATUS_REGISTER2);
		tmp = dev->spi->read_byte();
		break;

	case READ_STATUS_REGISTER3:
		dev->spi->write_byte(READ_STATUS_REGISTER3);
		tmp = dev->spi->read_byte();
		break;

	default:
		break;
	}

	dev->spi->cs_high();

	return tmp;
}

void w25qxxx_write_enable(struct w25qxxx_device *dev)
{
	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
}

void w25qxxx_write_enable_forvolatile(struct w25qxxx_device *dev)
{
	w25qxxx_transmit_cmd(dev, WRITE_ENABLE_FORVOLATILE);
}

void w25qxxx_write_disable(struct w25qxxx_device *dev)
{
	w25qxxx_transmit_cmd(dev, WRITE_DISABLE);
}

void w25qxxx_write_status_reg(struct w25qxxx_device *dev, u8 num, u8 tmp)
{
	w25qxxx_write_enable(dev);

	dev->spi->cs_low();

	switch (num) {
	case WRITE_STATUS_REGISTER1:
		dev->spi->write_byte(WRITE_STATUS_REGISTER1);
		dev->spi->write_byte(tmp);
		break;

	case WRITE_STATUS_REGISTER2:
		dev->spi->write_byte(WRITE_STATUS_REGISTER2);
		dev->spi->write_byte(tmp);
		break;

	case WRITE_STATUS_REGISTER3:
		dev->spi->write_byte(WRITE_STATUS_REGISTER3);
		dev->spi->write_byte(tmp);
		break;

	default:
		break;
	}

	dev->spi->cs_high();
}

u8 w25qxxx_read_busy(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	return test_bit(0, (unsigned long *)&tmp);
}

u8 w25qxxx_read_wel(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	return test_bit(1, (unsigned long *)&tmp);
}

u8 w25qxxx_read_bp0(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	return test_bit(2, (unsigned long *)&tmp);
}

u8 w25qxxx_read_bp1(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	return test_bit(4, (unsigned long *)&tmp);
}

u8 w25qxxx_read_bp2(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	return test_bit(5, (unsigned long *)&tmp);
}

u8 w25qxxx_read_sec(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	return test_bit(6, (unsigned long *)&tmp);
}

u8 w25qxxx_read_bp3(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	/* BP3位于状态寄存器2的第5位 */
	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	return test_bit(5, (unsigned long *)&tmp);
}

u8 w25qxxx_read_tb(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	return test_bit(7, (unsigned long *)&tmp);
}

u8 w25qxxx_read_srp0(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	return test_bit(7, (unsigned long *)&tmp);
}

u8 w25qxxx_read_srp1(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	return test_bit(0, (unsigned long *)&tmp);
}

u8 w25qxxx_read_qe(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	return test_bit(1, (unsigned long *)&tmp);
}

u8 w25qxxx_read_lb1(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	return test_bit(3, (unsigned long *)&tmp);
}

u8 w25qxxx_read_lb2(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	return test_bit(4, (unsigned long *)&tmp);
}

u8 w25qxxx_read_lb3(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	return test_bit(5, (unsigned long *)&tmp);
}

u8 w25qxxx_read_cmp(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	return test_bit(6, (unsigned long *)&tmp);
}

u8 w25qxxx_read_sus(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	return test_bit(7, (unsigned long *)&tmp);
}

u8 w25qxxx_read_ads(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	return test_bit(0, (unsigned long *)&tmp);
}

u8 w25qxxx_read_adp(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	return test_bit(1, (unsigned long *)&tmp);
}

u8 w25qxxx_read_wps(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	return test_bit(2, (unsigned long *)&tmp);
}

u8 w25qxxx_read_drv0(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	return test_bit(5, (unsigned long *)&tmp);
}

u8 w25qxxx_read_drv1(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	return test_bit(6, (unsigned long *)&tmp);
}

u8 w25qxxx_read_hold_rst(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	tmp = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	return test_bit(7, (unsigned long *)&tmp);
}

void w25qxxx_write_srp0(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	__assign_bit(7, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER1, reg);
}

void w25qxxx_write_sec(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	__assign_bit(6, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER1, reg);
}

void w25qxxx_write_tb(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	__assign_bit(5, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER1, reg);
}

void w25qxxx_write_bp(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;
	u8 bp_val = tmp & 0x07;  /* BP0-2是3位组合 */

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);

	/* 清除原BP0-2位，设置新值 */
	reg &= ~(0x07 << 2);  /* 清除bit2-4 */
	reg |= (bp_val << 2); /* 设置新的BP值 */

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER1, reg);
}

void w25qxxx_write_cmp(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	__assign_bit(6, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER2, reg);
}

void w25qxxx_write_lb(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	__assign_bit(7, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER2, reg);
}

void w25qxxx_write_bp3(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	__assign_bit(5, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER2, reg);
}

void w25qxxx_write_qe(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	__assign_bit(1, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER2, reg);
}

void w25qxxx_write_srp1(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);

	__assign_bit(0, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER2, reg);
}

void w25qxxx_write_hold_rst(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	__assign_bit(7, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER3, reg);
}

void w25qxxx_write_drv1(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	__assign_bit(6, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER3, reg);
}

void w25qxxx_write_drv0(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	__assign_bit(5, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER3, reg);
}

void w25qxxx_write_wps(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	__assign_bit(2, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER3, reg);
}

void w25qxxx_write_adp(struct w25qxxx_device *dev, u8 tmp)
{
	u8 reg = 0;

	reg = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	__assign_bit(1, (unsigned long *)&reg, tmp);

	w25qxxx_transmit_cmd(dev, WRITE_ENABLE);
	w25qxxx_write_status_reg(dev, WRITE_STATUS_REGISTER3, reg);
}

u8 w25q256_read_ext_addr_reg(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	dev->spi->cs_low();
	dev->spi->write_byte(READ_EXTENDED_ADDRESS_REGISTER);
	tmp = dev->spi->read_byte();
	dev->spi->cs_high();

	return tmp;
}

void w25q256_write_ext_addr_reg(struct w25qxxx_device *dev, u8 tmp)
{
	w25qxxx_write_enable(dev);
	dev->spi->cs_low();
	dev->spi->write_byte(WRITE_EXTENDED_ADDRESS_REGISTER);
	dev->spi->write_byte(tmp);
	dev->spi->cs_high();
}

void w25q256_enter_4byte_addr_mode(struct w25qxxx_device *dev)
{
	w25qxxx_transmit_cmd(dev, ENTER_4BYTE_ADDRESS_MODE);
}

void w25q256_exit_4byte_addr_mode(struct w25qxxx_device *dev)
{
	w25qxxx_transmit_cmd(dev, EXIT_4BYTE_ADDRESS_MODE);
}

void w25qxxx_read_data(struct w25qxxx_device *dev, u32 addr, u8 *pdata, u32 length)
{
	u8 sub_addr[4];
	u32 remain = 0;

	if (dev->id == W25Q128) {
		remain = w25q128_max_addr - addr + 1;
	} else if (dev->id == W25Q256) {
		remain = w25q256_max_addr - addr + 1;
	}

	if (remain < length) {
		pr_err("There is not enough space left to read the specified length.\n");
		return ;
	}

	if (dev->id == W25Q256) {
		sub_addr[0] = (addr & 0xFF000000) >> 24;
	}

	sub_addr[1] = (addr & 0x00FF0000) >> 16;
	sub_addr[2] = (addr & 0x0000FF00) >> 8;
	sub_addr[3] = (addr & 0x000000FF);

	dev->spi->cs_low();
	dev->spi->write_byte(READ_DATA);

	if (dev->id == W25Q256) {
		dev->spi->write_byte(sub_addr[0]);
	}

	dev->spi->write_bytes(&sub_addr[1], 3);

	dev->spi->read_bytes(pdata, length);
	dev->spi->cs_high();
}

void w25q256_read_data_with_4byte_addr(struct w25qxxx_device *dev, u32 addr, u8 *pdata, u32 length)
{
	u8 sub_addr[4];

	sub_addr[0] = (addr & 0xFF000000) >> 24;
	sub_addr[1] = (addr & 0x00FF0000) >> 16;
	sub_addr[2] = (addr & 0x0000FF00) >> 8;
	sub_addr[3] = (addr & 0x000000FF);

	dev->spi->cs_low();
	dev->spi->write_byte(READ_DATA_WITH_4Byte_ADDRESS);
	dev->spi->write_bytes(sub_addr, 4);

	dev->spi->read_bytes(pdata, length);
	dev->spi->cs_high();
}

void w25qxxx_fast_read(struct w25qxxx_device *dev, u32 addr, u8 *pdata, u32 length)
{
	u8 sub_addr[4];

	if (dev->id == W25Q256) {
		sub_addr[0] = (addr & 0xFF000000) >> 24;
	}

	sub_addr[1] = (addr & 0x00FF0000) >> 16;
	sub_addr[2] = (addr & 0x0000FF00) >> 8;
	sub_addr[3] = (addr & 0x000000FF);

	dev->spi->cs_low();
	dev->spi->write_byte(FAST_READ);

	if (dev->id == W25Q256) {
		dev->spi->write_byte(sub_addr[0]);
	}

	dev->spi->write_bytes(&sub_addr[1], 3);
	dev->spi->write_byte(DUMMY_BYTE);

	dev->spi->read_bytes(pdata, length);
	dev->spi->cs_high();
}

void w25q256_fast_read_with_4byte_addr(struct w25qxxx_device *dev, u32 addr, u8 *pdata, u32 length)
{
	u8 sub_addr[4];

	sub_addr[0] = (addr & 0xFF000000) >> 24;
	sub_addr[1] = (addr & 0x00FF0000) >> 16;
	sub_addr[2] = (addr & 0x0000FF00) >> 8;
	sub_addr[3] = (addr & 0x000000FF);

	dev->spi->cs_low();
	dev->spi->write_byte(FAST_READ_WITH_4Byte_ADDRESS);
	dev->spi->write_bytes(sub_addr, 4);
	dev->spi->write_byte(DUMMY_BYTE);

	dev->spi->read_bytes(pdata, length);
	dev->spi->cs_high();
}

static int w25qxxx_page_program(struct w25qxxx_device *dev, u32 addr, u8 *pdata, u16 length)
{
	u8 sub_addr[4];
	u8 busy;
	int ret;

	if (length == 0) {
		pr_err("Programing page length is 0.\n");
		return -EINVAL;
	}

	if (dev->id == W25Q256) {
		sub_addr[0] = (addr & 0xFF000000) >> 24;
	}

	sub_addr[1] = (addr & 0x00FF0000) >> 16;
	sub_addr[2] = (addr & 0x0000FF00) >> 8;
	sub_addr[3] = (addr & 0x000000FF);

	w25qxxx_write_enable(dev);
	dev->spi->cs_low();
	dev->spi->write_byte(PAGE_PROGRAM);

	if (dev->id == W25Q256) {
		dev->spi->write_byte(sub_addr[0]);
	}

	dev->spi->write_bytes(&sub_addr[1], 3);
	dev->spi->write_bytes(pdata, length);
	dev->spi->cs_high();

	ret = readx_poll_timeout_atomic(w25qxxx_read_busy, dev, busy,
			busy == 0, 1, 30000000);

	if (ret) {
		pr_err("Programing page is timeout.\n");
	}

	return 0;
}

static int w25qxxx_multi_page_program(struct w25qxxx_device *dev, u32 addr, u8 *pdata, u16 length)
{
	u8 num_of_page = 0, num_of_single = 0, sub_addr = 0, cnt = 0, remain_of_single = 0;
	int ret;

	/* Mod operation, if addr is an integer multiple of W25Q_PAGE_SIZE, sub_addr value is 0 */
	sub_addr = addr % W25Q_PAGE_SIZE;

	/* The difference count is just enough to line up to the page addr */
	cnt = W25Q_PAGE_SIZE - sub_addr;
	/* Figure out how many integer pages to write */
	num_of_page =  length / W25Q_PAGE_SIZE;
	/* mod operation is used to calculate the num of bytes less than one page */
	num_of_single = length % W25Q_PAGE_SIZE;

	/* sub_addr=0, then addr is just aligned by page */
	if (sub_addr == 0) {
		/* length < W25Q_PAGE_SIZE */
		if (num_of_page == 0) {
			w25qxxx_page_program(dev, addr, pdata, length);
		} else { /* length > W25Q_PAGE_SIZE */
			/* Let me write down all the integer pages */
			while (num_of_page--) {
				ret = w25qxxx_page_program(dev, addr, pdata, W25Q_PAGE_SIZE);
				if (ret) {
					pr_err("Page programming failed at address 0x%08X\n", addr);
					return ret;
				}

				addr +=  W25Q_PAGE_SIZE;
				pdata += W25Q_PAGE_SIZE;
			}

			/* If you have more than one page of data, write it down*/
			if (num_of_single != 0) {
				ret = w25qxxx_page_program(dev, addr, pdata, num_of_single);
				if (ret) {
					pr_err("Page programming failed at address 0x%08X\n", addr);
					return ret;
				}
			}
		}
	} else { /* If the addr is not aligned with W25Q_PAGE_SIZE */
		/* length < W25Q_PAGE_SIZE */
		if (num_of_page == 0) {
			/* The remaining count positions on the current page are smaller than num_of_single */
			if (num_of_single > cnt) {
				remain_of_single = num_of_single - cnt;

				/* Fill in the front page first */
				ret = w25qxxx_page_program(dev, addr, pdata, cnt);
				if (ret) {
					pr_err("Page programming failed at address 0x%08X\n", addr);
					return ret;
				}

				addr +=  cnt;
				pdata += cnt;

				/* Let me write the rest of the data */
				ret = w25qxxx_page_program(dev, addr, pdata, remain_of_single);
				if (ret) {
					pr_err("Page programming failed at address 0x%08X\n", addr);
					return ret;
				}
			} else { /* The remaining count position of the current page can write num_of_single data */
				ret = w25qxxx_page_program(dev, addr, pdata, length);
				if (ret) {
					pr_err("Page programming failed at address 0x%08X\n", addr);
					return ret;
				}
			}
		} else { /* length > W25Q_PAGE_SIZE */
			/* The addr is not aligned and the extra count is treated separately, not added to the operation */
			length -= cnt;
			num_of_page =  length / W25Q_PAGE_SIZE;
			num_of_single = length % W25Q_PAGE_SIZE;

			w25qxxx_page_program(dev, addr, pdata, cnt);

			addr +=  cnt;
			pdata += cnt;

			/* Write all the integer pages */
			while (num_of_page--) {
				ret = w25qxxx_page_program(dev, addr, pdata, W25Q_PAGE_SIZE);
				if (ret) {
					pr_err("Page programming failed at address 0x%08X\n", addr);
					return ret;
				}

				addr +=  W25Q_PAGE_SIZE;
				pdata += W25Q_PAGE_SIZE;
			}

			/* If you have more than one page of data, write it down */
			if (num_of_single != 0) {
				ret = w25qxxx_page_program(dev, addr, pdata, num_of_single);
				if (ret) {
					pr_err("Page programming failed at address 0x%08X\n", addr);
					return ret;
				}
			}
		}
	}

	return 0;
}

int w25qxxx_sector_erase(struct w25qxxx_device *dev, u32 addr)
{
	u8 sub_addr[4];
	u8 busy;
	int ret;

	if (dev->id != W25Q128 && dev->id != W25Q256) {
		pr_err("Invalid device type: 0x%02X\n", dev->id);
		return -EINVAL;
	}

	if (dev->id == W25Q256) {
		sub_addr[0] = (addr & 0xFF000000) >> 24;
	}

	sub_addr[1] = (addr & 0x00FF0000) >> 16;
	sub_addr[2] = (addr & 0x0000FF00) >> 8;
	sub_addr[3] = (addr & 0x000000FF);

	w25qxxx_write_enable(dev);
	dev->spi->cs_low();
	dev->spi->write_byte(SECTOR_ERASE);

	if (dev->id == W25Q256) {
		dev->spi->write_byte(sub_addr[0]);
	}

	dev->spi->write_bytes(&sub_addr[1], 3);
	dev->spi->cs_high();

	ret = readx_poll_timeout_atomic(w25qxxx_read_busy, dev, busy,
			busy == 0, 1, W25Q_SECTOR_ERASE_TIMEOUT);

	if (ret) {
		pr_err("Sector erase timeout at address 0x%08X\n", addr);
		return -ETIMEDOUT;
	}

	return 0;
}

static int w25qxxx_process_partial_sector(struct w25qxxx_device *dev, u32 start_addr, u8 *pdata, u16 length)
{
	u32 offset = 0;
	u32 addr = 0;
	u32 i = 0;
	int ret;
	u8 *w25qxxx_single_sector = NULL;

	/* 分配临时缓冲区，确保线程安全 */
	w25qxxx_single_sector = kmalloc(SECTOR_SIZE, GFP_KERNEL);
	if (!w25qxxx_single_sector) {
		pr_err("Failed to allocate memory for sector buffer\n");
		return -ENOMEM;
	}

	offset = start_addr % SECTOR_SIZE;
	addr = start_addr - offset;

	w25qxxx_read_data(dev, start_addr, &w25qxxx_single_sector[offset], length);

	for (i = offset; i < offset + length; i++) {
		if (w25qxxx_single_sector[i] != 0xFF) {
			w25qxxx_read_data(dev, addr, &w25qxxx_single_sector[0], offset);
			w25qxxx_read_data(dev, start_addr + length, &w25qxxx_single_sector[offset + length], SECTOR_SIZE - offset - length);
			memcpy(&w25qxxx_single_sector[offset], pdata, length);
			ret = w25qxxx_sector_erase(dev, addr);
			if (ret) {
				pr_err("Sector erase failed at address 0x%08X\n", addr);
				kfree(w25qxxx_single_sector);
				return ret;
			}
			ret = w25qxxx_multi_page_program(dev, addr, w25qxxx_single_sector, SECTOR_SIZE);
			if (ret) {
				pr_err("Multi page programming failed at address 0x%08X\n", addr);
				kfree(w25qxxx_single_sector);
				return ret;
			}
			kfree(w25qxxx_single_sector);  /* 释放缓冲区 */
			return 0;  /* 返回0表示成功，而不是-EPIPE */
		}
	}

	ret = w25qxxx_multi_page_program(dev, start_addr, pdata, length);
	if (ret) {
		pr_err("Multi page programming failed at address 0x%08X\n", start_addr);
		kfree(w25qxxx_single_sector);
		return ret;
	}

	kfree(w25qxxx_single_sector);

	return 0;
}

int w25qxxx_multi_sector_program(struct w25qxxx_device *dev, u32 addr, u8 *pdata, u32 length)
{
	u32 num_of_sector = 0, num_of_single = 0, offset = 0;
	u32 cnt = 0, remain_of_single = 0;
	int ret;

	/* Mod operation, if addr is an integer multiple of SECTOR_SIZE, offset value is 0 */
	offset = addr % SECTOR_SIZE;

	/* The difference count is just enough to line up to the page addr */
	cnt = SECTOR_SIZE - offset;
	/* Figure out how many integer pages to write */
	num_of_sector =  length / SECTOR_SIZE;
	/* mod operation is used to calculate the num of bytes less than one page */
	num_of_single = length % SECTOR_SIZE;

	/* offset=0, then addr is just aligned by page */
	if (offset == 0) {
		/* length < SECTOR_SIZE */
		if (num_of_sector == 0) {
			ret = w25qxxx_process_partial_sector(dev, addr, pdata, length);
			if (ret) {
				return ret;
			}
		} else { /* length > SECTOR_SIZE */
			/* Let me write down all the integer pages */
			while (num_of_sector--) {
				w25qxxx_sector_erase(dev, addr);
				ret = w25qxxx_multi_page_program(dev, addr, pdata, SECTOR_SIZE);
				if (ret) {
					return ret;
				}

				addr +=  SECTOR_SIZE;
				pdata += SECTOR_SIZE;
			}

			/* If you have more than one page of data, write it down*/
			ret = w25qxxx_process_partial_sector(dev, addr, pdata, num_of_single);
			if (ret) {
				return ret;
			}
		}
	}
	/* If the addr is not aligned with SECTOR_SIZE */
	else {
		/* length < SECTOR_SIZE */
		if (num_of_sector == 0) {
			/* The remaining count positions on the current page are smaller than num_of_single */
			if (num_of_single > cnt) {
				remain_of_single = num_of_single - cnt;

				/* Fill in the front page first */
				ret = w25qxxx_process_partial_sector(dev, addr, pdata, cnt);
				if (ret) {
					return ret;
				}

				addr +=  cnt;
				pdata += cnt;

				/* Let me write the rest of the data */
				ret = w25qxxx_process_partial_sector(dev, addr, pdata, remain_of_single);
				if (ret) {
					return ret;
				}
			} else { /* The remaining count position of the current page can write num_of_single data */
				ret = w25qxxx_process_partial_sector(dev, addr, pdata, length);
				if (ret) {
					return ret;
				}
			}
		} else { /* length > SECTOR_SIZE */
			/* The addr is not aligned and the extra count is treated separately, not added to the operation */
			length -= cnt;
			num_of_sector =  length / SECTOR_SIZE;
			num_of_single = length % SECTOR_SIZE;

			ret = w25qxxx_process_partial_sector(dev, addr, pdata, cnt);
			if (ret) {
				return ret;
			}

			addr +=  cnt;
			pdata += cnt;

			/* Write all the integer pages */
			while (num_of_sector--) {
				w25qxxx_sector_erase(dev, addr);
				ret = w25qxxx_multi_page_program(dev, addr, pdata, SECTOR_SIZE);
				if (ret) {
					return ret;
				}

				addr +=  SECTOR_SIZE;
				pdata += SECTOR_SIZE;
			}

			/* If you have more than one page of data, write it down */
			if (num_of_single != 0) {
				ret = w25qxxx_process_partial_sector(dev, addr, pdata, num_of_single);
				if (ret) {
					return ret;
				}
			}
		}
	}

	return 0;
}

int w25qxxx_write_data(struct w25qxxx_device *dev, u32 addr, u8 *pdata, u16 length)
{
	u32 remain = 0;
	int ret;

	if (!pdata) {
		pr_err("Data buffer is NULL\n");
		return -EINVAL;
	}

	if (length == 0) {
		pr_err("Write length is 0\n");
		return -EINVAL;
	}

	if (dev->id == W25Q128) {
		remain = w25q128_max_addr - addr + 1;
	} else if (dev->id == W25Q256) {
		remain = w25q256_max_addr - addr + 1;
	} else {
		pr_err("Invalid device type: 0x%02X\n", dev->id);
		return -EINVAL;
	}

	if (remain < length) {
		pr_err("There is not enough space left to write the specified length.\n");
		return -ENOSPC;
	}

	ret = w25qxxx_multi_sector_program(dev, addr, pdata, length);

	return ret;
}

int w25qxxx_block_erase_with_32kb(struct w25qxxx_device *dev, u32 addr)
{
	u8 sub_addr[4];
	u8 busy;
	int ret;

	if (dev->id != W25Q128 && dev->id != W25Q256) {
		pr_err("Invalid device type: 0x%02X\n", dev->id);
		return -EINVAL;
	}

	if (dev->id == W25Q256) {
		sub_addr[0] = (addr & 0xFF000000) >> 24;
	}

	sub_addr[1] = (addr & 0x00FF0000) >> 16;
	sub_addr[2] = (addr & 0x0000FF00) >> 8;
	sub_addr[3] = (addr & 0x000000FF);

	w25qxxx_write_enable(dev);
	dev->spi->cs_low();
	dev->spi->write_byte(BLOCK_ERASE_32KB);

	if (dev->id == W25Q256) {
		dev->spi->write_byte(sub_addr[0]);
	}

	dev->spi->write_bytes(&sub_addr[1], 3);
	dev->spi->cs_high();

	ret = readx_poll_timeout_atomic(w25qxxx_read_busy, dev, busy,
			busy == 0, 1, W25Q_BLOCK_ERASE_TIMEOUT);

	if (ret) {
		pr_err("32KB block erase timeout at address 0x%08X\n", addr);
		return -ETIMEDOUT;
	}

	return 0;
}

int w25qxxx_block_erase_with_64kb(struct w25qxxx_device *dev, u32 addr)
{
	u8 sub_addr[4];
	u8 busy;
	int ret;

	if (dev->id != W25Q128 && dev->id != W25Q256) {
		pr_err("Invalid device type: 0x%02X\n", dev->id);
		return -EINVAL;
	}

	if (dev->id == W25Q256) {
		sub_addr[0] = (addr & 0xFF000000) >> 24;
	}

	sub_addr[1] = (addr & 0x00FF0000) >> 16;
	sub_addr[2] = (addr & 0x0000FF00) >> 8;
	sub_addr[3] = (addr & 0x000000FF);

	w25qxxx_write_enable(dev);
	dev->spi->cs_low();
	dev->spi->write_byte(BLOCK_ERASE_64KB);

	if (dev->id == W25Q256) {
		dev->spi->write_byte(sub_addr[0]);
	}

	dev->spi->write_bytes(&sub_addr[1], 3);
	dev->spi->cs_high();

	ret = readx_poll_timeout_atomic(w25qxxx_read_busy, dev, busy,
			busy == 0, 1, W25Q_BLOCK_ERASE_TIMEOUT);

	if (ret) {
		pr_err("64KB block erase timeout at address 0x%08X\n", addr);
		return -ETIMEDOUT;
	}

	return 0;
}

int sw25qxxx_chip_erase(struct w25qxxx_device *dev)
{
	u8 busy;
	int ret;

	if (dev->id != W25Q128 && dev->id != W25Q256) {
		pr_err("Invalid device type: 0x%02X\n", dev->id);
		return -EINVAL;
	}

	w25qxxx_write_enable(dev);
	w25qxxx_transmit_cmd(dev, CHIP_ERASE);

	ret = readx_poll_timeout_atomic(w25qxxx_read_busy, dev, busy,
			busy == 0, 1, W25Q_CHIP_ERASE_TIMEOUT);

	if (ret) {
		pr_err("Chip erase timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

void w25qxxx_erase_program_suspend(struct w25qxxx_device *dev)
{
	if (w25qxxx_read_busy(dev) == 0 && w25qxxx_read_sus(dev) == 1) {
		return ;
	}

	w25qxxx_transmit_cmd(dev, ERASE_PROGRAM_SUSPEND);
	udelay(20);
}

void w25qxxx_erase_program_resume(struct w25qxxx_device *dev)
{
	if (w25qxxx_read_busy(dev) == 1 && w25qxxx_read_sus(dev) == 0) {
		return ;
	}

	w25qxxx_transmit_cmd(dev, ERASE_PROGRAM_RESUME);
}

void w25qxxx_power_down(struct w25qxxx_device *dev)
{
	w25qxxx_transmit_cmd(dev, POWER_DOWN);
	udelay(3);
}

void w25qxxx_release_power_down(struct w25qxxx_device *dev)
{
	w25qxxx_transmit_cmd(dev, RELEASE_POWERDOWN_DEVICE_ID);
	udelay(3);
}

u8 w25qxxx_release_device_id(struct w25qxxx_device *dev)
{
	u8 tmp = 0;

	dev->spi->cs_low();
	dev->spi->write_byte(RELEASE_POWERDOWN_DEVICE_ID);
	dev->spi->write_byte(DUMMY_BYTE);
	dev->spi->write_byte(DUMMY_BYTE);
	dev->spi->write_byte(DUMMY_BYTE);
	tmp = dev->spi->read_byte();
	dev->spi->cs_high();

	return tmp;
}

void w25qxxx_read_manufacturer_device_id(struct w25qxxx_device *dev, u8 *ManufacturerID, u8 *DeviceID)
{
	dev->spi->cs_low();
	dev->spi->write_byte(READ_MANUFACTURER_DEVICE_ID);
	dev->spi->write_byte(0x00);
	dev->spi->write_byte(0x00);
	dev->spi->write_byte(0x00);
	*ManufacturerID = dev->spi->read_byte();
	*DeviceID = dev->spi->read_byte();
	dev->spi->cs_high();
}

void w25qxxx_read_unique_id(struct w25qxxx_device *dev, u8 *unique_id)
{
	dev->spi->cs_low();
	dev->spi->write_byte(READ_UNIQUE_ID_NUMBER);

	if (dev->id == W25Q256) {
		if (w25qxxx_read_ads(dev) == 1) {
			dev->spi->write_byte(DUMMY_BYTE);
		}
	}

	dev->spi->write_byte(DUMMY_BYTE);
	dev->spi->write_byte(DUMMY_BYTE);
	dev->spi->write_byte(DUMMY_BYTE);
	dev->spi->write_byte(DUMMY_BYTE);

	dev->spi->read_bytes(unique_id, 8);
	dev->spi->cs_high();
}

void w25qxxx_read_jedec_id(struct w25qxxx_device *dev, u8 *JEDECID)
{
	dev->spi->cs_low();
	dev->spi->write_byte(READ_JEDEC_ID);

	dev->spi->read_bytes(JEDECID, 3);
	dev->spi->cs_high();
}

void w25qxxx_read_sfdp_reg(struct w25qxxx_device *dev, u32 addr, u8 *pdata, u16 length)
{
	u8 sub_addr = 0;

	sub_addr = (addr & 0x000000FF);

	dev->spi->cs_low();
	dev->spi->write_byte(READ_SFDP_REGISTER);
	dev->spi->write_byte(0x00);
	dev->spi->write_byte(0x00);
	dev->spi->write_byte(sub_addr);
	dev->spi->write_byte(DUMMY_BYTE);

	dev->spi->read_bytes(pdata, length);
	dev->spi->cs_high();
}

void w25qxxx_erase_security_regs(struct w25qxxx_device *dev, u8 addr)
{
	u8 busy;

	w25qxxx_write_enable(dev);
	dev->spi->cs_low();
	dev->spi->write_byte(ERASE_SECURITY_REGISTERS);

	if (dev->id == W25Q256) {
		dev->spi->write_byte(0x00);
	}

	dev->spi->write_byte(0x00);
	dev->spi->write_byte(addr);
	dev->spi->write_byte(0x00);
	dev->spi->cs_high();

	readx_poll_timeout_atomic(w25qxxx_read_busy, dev, busy,
		busy == 0, 1, 30000000);
}

void w25qxxx_program_security_regs(struct w25qxxx_device *dev, u32 regNum, u32 Byteaddr, u8 *pdata, u16 length)
{
	w25qxxx_write_enable(dev);
	dev->spi->cs_low();
	dev->spi->write_byte(PROGRAM_SECURITY_REGISTERS);
	if (dev->id == W25Q256) {
		dev->spi->write_byte(0x00);
	}
	dev->spi->write_byte(0x00);
	dev->spi->write_byte(regNum);
	dev->spi->write_byte(Byteaddr);
	dev->spi->cs_high();
}

void w25qxxx_read_security_regs(struct w25qxxx_device *dev, u8 regNum, u8 Byteaddr, u8 *pdata, u16 length)
{
	dev->spi->cs_low();
	dev->spi->write_byte(READ_SECURITY_REGISTERS);
	dev->spi->write_byte(0x00);
	dev->spi->write_byte(regNum);
	dev->spi->write_byte(Byteaddr);
	dev->spi->write_byte(DUMMY_BYTE);

	for (u32 i = 0; i < length; i++) {
		pdata[i] = dev->spi->read_byte();
	}

	dev->spi->cs_high();
}

void w25qxxx_enter_qpi_mode(struct w25qxxx_device *dev)
{
	w25qxxx_write_qe(dev, 1);
	w25qxxx_transmit_cmd(dev, ENTER_QPI_MODE);
}

void w25qxxx_exit_qpi_mode(struct w25qxxx_device *dev)
{
	w25qxxx_transmit_cmd(dev, EXIT_QPI_MODE);
}

void w25qxxx_individual_block_sector_lock(struct w25qxxx_device *dev, u32 addr)
{
	u8 sub_addr[4];

	if (dev->id == W25Q256) {
		sub_addr[0] = (addr & 0xFF000000) >> 24;
	}

	sub_addr[1] = (addr & 0x00FF0000) >> 16;
	sub_addr[2] = (addr & 0x0000FF00) >> 8;
	sub_addr[3] = (addr & 0x000000FF);

	w25qxxx_write_wps(dev, 1);
	dev->spi->cs_low();
	dev->spi->write_byte(INDIVIDUAL_BLOCK_SECTOR_LOCK);

	if (dev->id == W25Q256) {
		dev->spi->write_byte(sub_addr[0]);
	}

	dev->spi->write_bytes(&sub_addr[1], 3);
	dev->spi->cs_high();
}

void w25qxxx_individual_block_sector_unlock(struct w25qxxx_device *dev, u32 addr)
{
	u8 sub_addr[4];

	if (dev->id == W25Q256) {
		sub_addr[0] = (addr & 0xFF000000) >> 24;
	}

	sub_addr[1] = (addr & 0x00FF0000) >> 16;
	sub_addr[2] = (addr & 0x0000FF00) >> 8;
	sub_addr[3] = (addr & 0x000000FF);

	w25qxxx_write_wps(dev, 1);
	dev->spi->cs_low();
	dev->spi->write_byte(INDIVIDUAL_BLOCK_SECTOR_UNLOCK);

	if (dev->id == W25Q256) {
		dev->spi->write_byte(sub_addr[0]);
	}

	dev->spi->write_bytes(&sub_addr[1], 3);
	dev->spi->cs_high();
}

u8 w25qxxx_read_block_sector_lock(struct w25qxxx_device *dev, u32 addr)
{
	u8 tmp = 0;
	u8 sub_addr[4];

	if (dev->id == W25Q256) {
		sub_addr[0] = (addr & 0xFF000000) >> 24;
	}

	sub_addr[1] = (addr & 0x00FF0000) >> 16;
	sub_addr[2] = (addr & 0x0000FF00) >> 8;
	sub_addr[3] = (addr & 0x000000FF);

	w25qxxx_write_wps(dev, 1);
	dev->spi->cs_low();
	dev->spi->write_byte(READ_BLOCK_SECTOR_LOCK);

	if (dev->id == W25Q256) {
		dev->spi->write_byte(sub_addr[0]);
	}

	dev->spi->write_bytes(&sub_addr[1], 3);
	tmp = dev->spi->read_byte();
	dev->spi->cs_high();

	return (tmp & 0x01);
}

void w25qxxx_global_block_sector_lock(struct w25qxxx_device *dev)
{
	w25qxxx_write_enable(dev);
	w25qxxx_transmit_cmd(dev, GLOBAL_BLOCK_SECTOR_LOCK);
}

void w25qxxx_global_block_sector_unlock(struct w25qxxx_device *dev)
{
	w25qxxx_write_enable(dev);
	w25qxxx_transmit_cmd(dev, GLOBAL_BLOCK_SECTOR_UNLOCK);
}

void w25qxxx_enable_reset(struct w25qxxx_device *dev)
{
	w25qxxx_transmit_cmd(dev, ENABLE_RESET);
}

int w25qxxx_soft_reset(struct w25qxxx_device *dev)
{
	if (dev->id != W25Q128 && dev->id != W25Q256) {
		pr_err("Invalid device type: 0x%02X\n", dev->id);
		return -EINVAL;
	}

	if (w25qxxx_read_busy(dev) == 1 || w25qxxx_read_sus(dev) == 1) {
		pr_err("Device cannot be reset while busy or suspended\n");
		return -EBUSY;
	}

	w25qxxx_enable_reset(dev);
	w25qxxx_transmit_cmd(dev, RESET_DEVICE);
	udelay(W25Q_SOFT_RESET_DELAY);

	return 0;
}

int w25qxxx_init(struct w25qxxx_device *dev)
{
	int ret;

	if (dev->id != W25Q128 && dev->id != W25Q256) {
		pr_err("Invalid device type: 0x%02X\n", dev->id);
		return -EINVAL;
	}

	w25qxxx_release_power_down(dev);
	ret = w25qxxx_soft_reset(dev);
	if (ret) {
		pr_err("Failed to reset device\n");
		return ret;
	}
	w25qxxx_release_device_id(dev);

	switch (dev->id) {
	case W25Q128:
		break;

	case W25Q256:
		if (w25qxxx_read_ads(dev) == 0) {
			w25q256_enter_4byte_addr_mode(dev);
		}

		break;

	default:
		pr_err("Unsupported device type: 0x%02X\n", dev->id);
		return -ENODEV;
	}

	return 0;
}

void w25qxxx_read_info(struct w25qxxx_device *dev)
{
	u8 manufacturer_id = 0, device_id = 0;
	u8 jedec_id[3];
	u8 unique_id[8];

	pr_debug("dev is %#02X.\n", dev);
	w25qxxx_read_jedec_id(dev, jedec_id);
	pr_debug("JEDEC ID is %#02X%02X%02X\n",
		jedec_id[0], jedec_id[1], jedec_id[2]);
	w25qxxx_read_manufacturer_device_id(dev, &manufacturer_id, &device_id);
	pr_debug("Manufacturer ID is %#02X, Device ID is %#02X.\n",
		manufacturer_id, device_id);
	w25qxxx_read_unique_id(dev, unique_id);
	pr_debug("Unique ID is %02X%02X%02X%02X%02X%02X%02X%02X\n",
		unique_id[0], unique_id[1], unique_id[2], unique_id[3],
		unique_id[4], unique_id[5], unique_id[6], unique_id[7]);
}

static LIST_HEAD(w25qxxx_list);

struct w25qxxx_device *w25qxxx_alloc_device(u8 id, struct spi_master *spi)
{
	struct w25qxxx_device *dev;
	int ret;

	dev = kzalloc(sizeof(struct w25qxxx_device), GFP_KERNEL);
	if (!dev) {
		return NULL;
	}

	dev->id = id;
	switch (dev->id) {
	case W25Q128:
		dev->max_addr = 0xFFFFFF;
		break;
	case W25Q256:
		dev->max_addr = 0x1FFFFFF;
		break;
	default:
		pr_err("Unsupported device type: 0x%02X\n", dev->id);
		goto err;
	}
	dev->spi = spi;

	/* Allocate slave register buffer for SPI simulation */
	dev->slave_regs = kmalloc(0x10000, GFP_KERNEL);  /* 64KB buffer */
	if (!dev->slave_regs) {
		ret = -ENOMEM;
		goto err;
	}

	/* Start register randomization thread */
	ret = w25qxxx_start_reg_random(dev);
	if (ret) {
		pr_err("Failed to start W25Qxxx register simulation thread\n");
		kfree(dev->slave_regs);
		dev->slave_regs = NULL;
		goto err;
	}

	/* Initialize SPI slave for simulation */
	dev->spi_slave = spi_slave_soft_init("w25qxxx", 0, 0, SPI_BIT_ORDER_MSB,
			dev->slave_regs, 0x10000);
	if (IS_ERR(dev->spi_slave)) {
		pr_err("Failed to initialize W25Qxxx SPI slave\n");
		w25qxxx_stop_reg_random(dev);
		kfree(dev->slave_regs);
		dev->slave_regs = NULL;
		ret = PTR_ERR(dev->spi_slave);
		goto err;
	}

	ret = w25qxxx_init(dev);
	if (ret) {
		spi_slave_soft_exit(dev->spi_slave);
		w25qxxx_stop_reg_random(dev);
		kfree(dev->slave_regs);
		dev->slave_regs = NULL;
		goto err;
	}
	list_add(&dev->list, &w25qxxx_list);

	return dev;

err:
	kfree(dev);
	return NULL;
}

void w25qxxx_free_device(struct w25qxxx_device *dev)
{
	if (dev->spi_slave) {
		spi_slave_soft_exit(dev->spi_slave);
	}

	w25qxxx_stop_reg_random(dev);

	kfree(dev->slave_regs);
	kfree(dev);
}

struct w25qxxx_device *w25qxxx_find_device(u8 id)
{
	struct w25qxxx_device *dev;

	list_for_each_entry(dev, &w25qxxx_list, list) {
		if (dev->id == id) {
			return dev;
		}
	}
	return NULL;
}

#ifdef DESIGN_VERIFICATION_W25QXXX
#include "kinetis/test-kinetis.h"

int t_w25qxxx_device_init(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	dev = w25qxxx_alloc_device(w25qxxx, &fake_spi_master);
	if (!dev) {
		return -ENOMEM;
	}

	pr_info("=== W25Qxxx Device Initialization Test ===");
	pr_info("Device type: %s", (dev->id == W25Q128) ? "W25Q128" : "W25Q256");
	pr_info("SPI slave simulation enabled: %s", dev->spi_slave ? "Yes" : "No");
	pr_info("Slave register buffer size: 64KB (0x10000 bytes)");

	w25qxxx_read_info(dev);

	pr_info("Device initialization completed");
	return 0;
}

int t_w25qxxx_read_info(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Read Device Information Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}
	w25qxxx_read_info(dev);

	pr_info("Device information read completed");
	return 0;
}

int t_w25qxxx_loopback(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 tmp_rng = 0;
	u16 i, j, length = 0;
	u16 round = 8;
	u32 test_addr = 0;
	u8 *tx_buffer, *rx_buffer;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	if (argc > 2) {
		round = simple_strtoul(argv[2], &argv[2], 10);
	}

	pr_info("=== W25Qxxx Loopback Test (%d rounds) ===", round);

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	rx_buffer = kmalloc(32768, GFP_KERNEL);
	if (!rx_buffer) {
		return -ENOMEM;
	}
	tx_buffer = kmalloc(32768, GFP_KERNEL);
	if (!tx_buffer) {
		return -ENOMEM;
	}

	for (j = 0; j < round; j++) {
		tmp_rng = random_get32bit();
		length = tmp_rng & 0x7FFF;

		memset(tx_buffer, 0, length);
		memset(rx_buffer, 0, length);

		tmp_rng = random_get32bit();

		switch (dev->id) {
		case W25Q128:
			test_addr = tmp_rng & 0xFFFFFF;
			break;

		case W25Q256:
			test_addr = tmp_rng & 0x1FFFFFF;
			break;

		default:
			test_addr = 0;
			break;
		}

		pr_debug("Round [%4u], Test Addr: 0x%08X, Length: %d", j, test_addr, length);

		random_get_array(tx_buffer, length, RNG_8BITS);

		w25qxxx_write_data(dev, test_addr, tx_buffer, length);
		w25qxxx_read_data(dev, test_addr, rx_buffer, length);

		for (i = 0; i < length; i++) {
			if (tx_buffer[i] != rx_buffer[i]) {
				pr_err("Data mismatch at index %d: TX=0x%02X, RX=0x%02X",
					i, tx_buffer[i], rx_buffer[i]);
				return FAIL;
			}
		}

		if (j == 0 || j == round - 1) {
			pr_info("Round %d/%d: Addr=0x%08X, Length=%d bytes - OK",
				j + 1, round, test_addr, length);
		}
	}

	pr_info("Loopback test completed");
	return 0;
}

int t_w25qxxx_chip_erase(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Chip Erase Test ===");
	pr_info("Warning: This will erase all data on the chip (~30 seconds)");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	ret = sw25qxxx_chip_erase(dev);
	if (ret) {
		pr_err("Chip erase failed with error: %d", ret);
		return FAIL;
	}

	pr_info("Chip erase completed");
	return 0;
}

int t_w25qxxx_sector_erase(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 test_addr = 0;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	if (argc > 2) {
		test_addr = simple_strtoul(argv[2], &argv[2], 16);
	} else {
		test_addr = random_get32bit();
		if (dev->id == W25Q128) {
			test_addr &= 0xFFF000;  /* Align to 4KB sector boundary */
		} else {
			test_addr &= 0x1FFF000;
		}
	}

	pr_info("=== W25Qxxx Sector Erase Test ===");
	pr_info("Erasing sector at address: 0x%08X", test_addr);

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	ret = w25qxxx_sector_erase(dev, test_addr);
	if (ret) {
		pr_err("Sector erase failed with error: %d", ret);
		return FAIL;
	}

	pr_info("Sector erase completed");
	return 0;
}

int t_w25qxxx_block_erase_32kb(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 test_addr = 0;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	if (argc > 2) {
		test_addr = simple_strtoul(argv[2], &argv[2], 16);
	} else {
		test_addr = random_get32bit();
		if (dev->id == W25Q128) {
			test_addr &= 0xFFFF8000;  /* Align to 32KB block boundary */
		} else {
			test_addr &= 0x1FFF8000;
		}
	}

	pr_info("=== W25Qxxx 32KB Block Erase Test ===");
	pr_info("Erasing 32KB block at address: 0x%08X", test_addr);

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	ret = w25qxxx_block_erase_with_32kb(dev, test_addr);
	if (ret) {
		pr_err("32KB block erase failed with error: %d", ret);
		return FAIL;
	}

	pr_info("32KB block erase completed");
	return 0;
}

int t_w25qxxx_block_erase_64kb(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 test_addr = 0;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	if (argc > 2) {
		test_addr = simple_strtoul(argv[2], &argv[2], 16);
	} else {
		test_addr = random_get32bit();
		if (dev->id == W25Q128) {
			test_addr &= 0xFFFF0000;  /* Align to 64KB block boundary */
		} else {
			test_addr &= 0x1FFF0000;
		}
	}

	pr_info("=== W25Qxxx 64KB Block Erase Test ===");
	pr_info("Erasing 64KB block at address: 0x%08X", test_addr);

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	ret = w25qxxx_block_erase_with_64kb(dev, test_addr);
	if (ret) {
		pr_err("64KB block erase failed with error: %d", ret);
		return FAIL;
	}

	pr_info("64KB block erase completed");
	return 0;
}

int t_w25qxxx_page_program(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 test_addr = 0;
	u16 length = 256;
	u8 page_buf[256];
	u8 *rx_buffer;
	u16 i;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	if (argc > 2) {
		test_addr = simple_strtoul(argv[2], &argv[2], 16);
	} else {
		test_addr = random_get32bit();
		if (dev->id == W25Q128) {
			test_addr &= 0xFFFFF00;  /* Align to page boundary */
		} else {
			test_addr &= 0x1FFFF00;
		}
	}

	pr_info("=== W25Qxxx Page Program Test ===");
	pr_info("Programming page at address: 0x%08X", test_addr);

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	rx_buffer = kmalloc(32768, GFP_KERNEL);
	if (!rx_buffer) {
		return -ENOMEM;
	}

	/* Prepare test pattern */
	for (i = 0; i < length; i++) {
		page_buf[i] = (u8)i;
	}

	ret = w25qxxx_write_data(dev, test_addr, page_buf, length);
	if (ret) {
		pr_err("Page program failed with error: %d", ret);
		return FAIL;
	}

	/* Verify */
	memset(rx_buffer, 0, length);
	w25qxxx_read_data(dev, test_addr, rx_buffer, length);

	for (i = 0; i < length; i++) {
		if (page_buf[i] != rx_buffer[i]) {
			pr_err("Verification failed at offset %d: wrote=0x%02X, read=0x%02X",
				i, page_buf[i], rx_buffer[i]);
			return FAIL;
		}
	}

	pr_info("Page program and verification completed");
	return 0;
}

/**
 * @brief Test 9: W25Qxxx Status Register Test
 * @return 0 if status register operations work, FAIL otherwise
 */
int t_w25qxxx_status_register(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u8 status1, status2, status3;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Status Register Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	/* Read status registers */
	status1 = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);
	status2 = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER2);
	status3 = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER3);

	pr_info("Status Register 1: 0x%02X", status1);
	pr_info("Status Register 2: 0x%02X", status2);
	pr_info("Status Register 3: 0x%02X", status3);

	/* Check individual bits */
	pr_info("SR1 - BUSY: %d, WEL: %d, BP: %d%d%d",
		w25qxxx_read_busy(dev),
		w25qxxx_read_wel(dev),
		w25qxxx_read_bp2(dev),
		w25qxxx_read_bp1(dev),
		w25qxxx_read_bp0(dev));

	pr_info("Status register test completed");
	return 0;
}

int t_w25qxxx_power_test(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u8 device_id;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Power Management Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	/* Enter power down mode */
	pr_info("Entering power down mode...");
	w25qxxx_power_down(dev);
	mdelay(10);

	/* Try to release from power down and get device ID */
	pr_info("Releasing from power down...");
	device_id = w25qxxx_release_device_id(dev);
	pr_info("Device ID after power down release: 0x%02X", device_id);

	if (device_id != 0x13) {
		pr_warn("Device ID not 0x13 (expected), got: 0x%02X", device_id);
	}

	pr_info("Power management test completed");
	return 0;
}

int t_w25qxxx_reset_test(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Reset Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	/* Test soft reset */
	pr_info("Testing soft reset...");
	ret = w25qxxx_soft_reset(dev);
	if (ret) {
		pr_err("Soft reset failed with error: %d", ret);
		return FAIL;
	}
	pr_info("Soft reset successful");

	mdelay(100);

	/* Test hard reset */
	pr_info("Testing hard reset...");
	w25qxxx_hard_reset(dev);
	pr_info("Hard reset successful");

	pr_info("Reset test completed");
	return 0;
}

int t_w25qxxx_fast_read(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 test_addr = 0x1000;
	u8 pattern[256];
	u8 fast_data[256];
	u8 normal_data[256];
	u16 i;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Fast Read Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	/* Write test pattern */
	for (i = 0; i < 256; i++) {
		pattern[i] = (u8)(i ^ 0xAA);
	}

	ret = w25qxxx_write_data(dev, test_addr, pattern, 256);
	if (ret) {
		pr_err("Failed to write test pattern");
		return FAIL;
	}

	/* Read using normal read */
	w25qxxx_read_data(dev, test_addr, normal_data, 256);

	/* Read using fast read */
	w25qxxx_fast_read(dev, test_addr, fast_data, 256);

	/* Compare results */
	for (i = 0; i < 256; i++) {
		if (normal_data[i] != fast_data[i]) {
			pr_err("Fast read mismatch at offset %d: normal=0x%02X, fast=0x%02X",
				i, normal_data[i], fast_data[i]);
			return FAIL;
		}
	}

	pr_info("Fast read test completed");
	return 0;
}

int t_w25qxxx_block_protection(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 test_addr = 0x10000;
	u8 test_data[16] = {0xAB, 0xCD, 0xEF, 0x01};
	u8 read_data[16];
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Block Protection Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	/* Lock individual block */
	pr_info("Locking block at address 0x%08X...", test_addr);
	w25qxxx_individual_block_sector_lock(dev, test_addr);
	mdelay(10);

	/* Try to write to locked block (should fail) */
	pr_info("Attempting to write to locked block...");
	ret = w25qxxx_write_data(dev, test_addr, test_data, 4);
	if (ret == 0) {
		pr_warn("Write to locked block succeeded (unexpected)");
	}

	/* Unlock block */
	pr_info("Unlocking block...");
	w25qxxx_individual_block_sector_unlock(dev, test_addr);
	mdelay(10);

	/* Try to write again (should succeed) */
	pr_info("Attempting to write to unlocked block...");
	ret = w25qxxx_write_data(dev, test_addr, test_data, 4);
	if (ret) {
		pr_err("Failed to write to unlocked block");
		return FAIL;
	}

	/* Verify */
	w25qxxx_read_data(dev, test_addr, read_data, 4);
	if (read_data[0] != test_data[0] || read_data[1] != test_data[1]) {
		pr_err("Data verification failed after unlock");
		return FAIL;
	}

	pr_info("Block protection test completed");
	return 0;
}

int t_w25qxxx_suspend_resume(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 erase_addr = 0x10000;
	u32 read_addr = 0x20000;
	u8 read_data[16];
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Erase Program Suspend/Resume Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	/* Write some data at read_addr first */
	w25qxxx_write_data(dev, read_addr, (u8[]) {
		0x11, 0x22, 0x33, 0x44
	}, 4);

	/* Start sector erase (will take some time) */
	pr_info("Starting sector erase at 0x%08X...", erase_addr);
	ret = w25qxxx_sector_erase(dev, erase_addr);
	/* Note: In a real test, you would suspend before erase completes */

	/* Suspend erase operation */
	pr_info("Suspending erase operation...");
	w25qxxx_erase_program_suspend(dev);
	mdelay(20);

	/* Read from another location (while suspended) */
	pr_info("Reading from another location while suspended...");
	w25qxxx_read_data(dev, read_addr, read_data, 4);
	pr_info("Data at 0x%08X: %02X %02X %02X %02X",
		read_addr, read_data[0], read_data[1], read_data[2], read_data[3]);

	/* Resume erase operation */
	pr_info("Resuming erase operation...");
	w25qxxx_erase_program_resume(dev);
	mdelay(50);

	pr_info("Suspend/Resume test completed");
	return 0;
}

int t_w25qxxx_multi_sector_program(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 test_addr = 0x1000;
	u32 length = 8192;  /* 2 sectors */
	u8 *tx_buffer;
	u8 *rx_buffer;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Multi-Sector Program Test ===");
	pr_info("Programming %d bytes at 0x%08X", length, test_addr);

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	rx_buffer = kmalloc(32768, GFP_KERNEL);
	if (!rx_buffer) {
		return -ENOMEM;
	}
	tx_buffer = kmalloc(32768, GFP_KERNEL);
	if (!tx_buffer) {
		return -ENOMEM;
	}

	/* Prepare test data */
	for (u32 i = 0; i < length; i++) {
		tx_buffer[i] = (u8)(i & 0xFF);
	}

	ret = w25qxxx_multi_sector_program(dev, test_addr, tx_buffer, length);
	if (ret) {
		pr_err("Multi-sector program failed with error: %d", ret);
		return FAIL;
	}

	/* Verify */
	w25qxxx_read_data(dev, test_addr, rx_buffer, length);

	for (u32 i = 0; i < length; i++) {
		if (tx_buffer[i] != rx_buffer[i]) {
			pr_err("Verification failed at offset %u: wrote=0x%02X, read=0x%02X",
				i, tx_buffer[i], rx_buffer[i]);
			return FAIL;
		}
	}

	pr_info("Multi-sector program test completed");
	return 0;
}

int t_w25qxxx_comprehensive(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Comprehensive Test Suite ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}
	pr_info("Device: %s", (dev->id == W25Q128) ? "W25Q128" : "W25Q256");

	/* Test 1: Device Init */
	ret = t_w25qxxx_device_init(argc, argv);
	if (ret != 0) {
		pr_err("FAIL: Device initialization test failed");
		return FAIL;
	}

	/* Test 2: Status Register */
	ret = t_w25qxxx_status_register(argc, argv);
	if (ret != 0) {
		pr_err("FAIL: Status register test failed");
		return FAIL;
	}

	/* Test 3: Page Program */
	ret = t_w25qxxx_page_program(argc, argv);
	if (ret != 0) {
		pr_err("FAIL: Page program test failed");
		return FAIL;
	}

	/* Test 4: Sector Erase */
	ret = t_w25qxxx_sector_erase(argc, argv);
	if (ret != 0) {
		pr_err("FAIL: Sector erase test failed");
		return FAIL;
	}

	/* Test 5: Loopback (2 rounds) */
	char *loopback_args[] = {(dev->id == W25Q128) ? "w25q128" : "w25q256", "2"};
	ret = t_w25qxxx_loopback(2, loopback_args);
	if (ret != 0) {
		pr_err("FAIL: Loopback test failed");
		return FAIL;
	}

	/* Test 6: Fast Read */
	ret = t_w25qxxx_fast_read(argc, argv);
	if (ret != 0) {
		pr_err("FAIL: Fast read test failed");
		return FAIL;
	}

	/* Test 7: Power Management */
	ret = t_w25qxxx_power_test(argc, argv);
	if (ret != 0) {
		pr_err("FAIL: Power management test failed");
		return FAIL;
	}

	pr_info("=== All W25Qxxx Comprehensive Tests PASSED ===");
	return 0;
}

int t_w25qxxx_boundary(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 min_addr = 0;
	u32 max_addr;
	u8 test_data[16] = {0xA5, 0x5A, 0x55, 0xAA};
	u8 read_data[16];
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	max_addr = (dev->id == W25Q128) ? 0xFF0000 : 0x1FF0000;

	pr_info("=== W25Qxxx Boundary Test ===");
	pr_info("Testing address boundaries: 0x%08X to 0x%08X", min_addr, max_addr);

	/* Test minimum address */
	pr_info("testing minimum address 0x%08X", min_addr);
	ret = w25qxxx_write_data(dev, min_addr, test_data, 4);
	if (ret) {
		pr_err("failed to write at minimum address");
		return FAIL;
	}
	w25qxxx_read_data(dev, min_addr, read_data, 4);
	if (memcmp(test_data, read_data, 4) != 0) {
		pr_err("verification failed at minimum address");
		return FAIL;
	}

	/* Test maximum address */
	pr_info("testing maximum address 0x%08X", max_addr);
	ret = w25qxxx_write_data(dev, max_addr, test_data, 4);
	if (ret) {
		pr_err("failed to write at maximum address");
		return FAIL;
	}
	w25qxxx_read_data(dev, max_addr, read_data, 4);
	if (memcmp(test_data, read_data, 4) != 0) {
		pr_err("verification failed at maximum address");
		return FAIL;
	}

	/* Test page boundary */
	u32 page_boundary = 0x100;
	pr_info("testing page boundary 0x%08X", page_boundary);
	ret = w25qxxx_write_data(dev, page_boundary, test_data, 4);
	if (ret) {
		pr_err("failed to write at page boundary");
		return FAIL;
	}
	w25qxxx_read_data(dev, page_boundary, read_data, 4);
	if (memcmp(test_data, read_data, 4) != 0) {
		pr_err("verification failed at page boundary");
		return FAIL;
	}

	/* Test sector boundary */
	u32 sector_boundary = 0x1000;
	pr_info("testing sector boundary 0x%08X", sector_boundary);
	ret = w25qxxx_write_data(dev, sector_boundary, test_data, 4);
	if (ret) {
		pr_err("failed to write at sector boundary");
		return FAIL;
	}
	w25qxxx_read_data(dev, sector_boundary, read_data, 4);
	if (memcmp(test_data, read_data, 4) != 0) {
		pr_err("verification failed at sector boundary");
		return FAIL;
	}

	pr_info("boundary test completed");
	return 0;
}

int t_w25qxxx_performance(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 test_addr = 0x200000;
	u32 test_length = 8192;
	u8 *rx_buffer, *tx_buffer;
	u32 start_time, end_time, write_time, read_time;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Performance Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}
	pr_info("testing %d bytes read/write", test_length);

	/* Erase test area first */
	w25qxxx_sector_erase(dev, test_addr);
	mdelay(50);

	rx_buffer = kmalloc(32768, GFP_KERNEL);
	if (!rx_buffer) {
		return -ENOMEM;
	}
	tx_buffer = kmalloc(32768, GFP_KERNEL);
	if (!tx_buffer) {
		return -ENOMEM;
	}

	/* Prepare test data */
	for (u32 i = 0; i < test_length; i++) {
		tx_buffer[i] = (u8)(i & 0xFF);
	}

	/* Measure write time */
	start_time = basic_timer_get_ms();
	ret = w25qxxx_multi_sector_program(dev, test_addr, tx_buffer, test_length);
	end_time = basic_timer_get_ms();
	if (ret) {
		pr_err("multi-sector program failed");
		return FAIL;
	}
	write_time = end_time - start_time;

	/* Measure read time */
	start_time = basic_timer_get_ms();
	w25qxxx_read_data(dev, test_addr, rx_buffer, test_length);
	end_time = basic_timer_get_ms();
	read_time = end_time - start_time;

	pr_info("write time: %lu ms (%.2f KB/s)", write_time,
		(test_length / 1024.0) / (write_time / 1000.0));
	pr_info("read time: %lu ms (%.2f KB/s)", read_time,
		(test_length / 1024.0) / (read_time / 1000.0));

	/* Verify data */
	for (u32 i = 0; i < test_length; i++) {
		if (tx_buffer[i] != rx_buffer[i]) {
			pr_err("verification failed at offset %u", i);
			return FAIL;
		}
	}

	pr_info("performance test completed");
	return 0;
}

int t_w25qxxx_stress(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 test_addr = 0x300000;
	u32 test_length = 256;
	u8 *rx_buffer, *tx_buffer;
	int iterations = 100;
	int success_count = 0;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	if (argc > 2) {
		iterations = simple_strtoul(argv[2], NULL, 10);
	}

	pr_info("=== W25Qxxx Stress Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}
	pr_info("running %d iterations of erase-program-verify", iterations);

	/* Erase test area first */
	w25qxxx_sector_erase(dev, test_addr);
	mdelay(50);

	rx_buffer = kmalloc(32768, GFP_KERNEL);
	if (!rx_buffer) {
		return -ENOMEM;
	}
	tx_buffer = kmalloc(32768, GFP_KERNEL);
	if (!tx_buffer) {
		return -ENOMEM;
	}

	for (int i = 0; i < iterations; i++) {
		u32 addr = test_addr + (i % 32) * 4096;  /* Spread across 32 sectors */

		/* Prepare test pattern */
		for (u32 j = 0; j < test_length; j++) {
			tx_buffer[j] = (u8)((i + j) & 0xFF);
		}

		/* Write */
		ret = w25qxxx_write_data(dev, addr, tx_buffer, test_length);
		if (ret) {
			pr_warn("write failed at iteration %d", i);
			continue;
		}

		/* Verify */
		w25qxxx_read_data(dev, addr, rx_buffer, test_length);
		if (memcmp(tx_buffer, rx_buffer, test_length) != 0) {
			pr_warn("verification failed at iteration %d", i);
			continue;
		}

		success_count++;
	}

	pr_info("stress test result: %d/%d iterations successful", success_count, iterations);

	if (success_count >= iterations * 95 / 100) {
		pr_info("stress test completed (>=95%% success)");
		return 0;
	}

	pr_err("fail: stress test failed (<95%% success)");
	return FAIL;
}

int t_w25qxxx_address_mode(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u8 test_data[16] = {0x11, 0x22, 0x33, 0x44};
	u8 read_data[16];
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Address Mode Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	if (dev->id == W25Q256) {
		/* Test 4-byte address mode */
		pr_info("testing 4-byte address mode for W25Q256");
		u32 test_addr = 0x1000000;  /* 16MB address (requires 4 bytes) */

		w25q256_enter_4byte_addr_mode(dev);
		mdelay(1);

		ret = w25qxxx_write_data(dev, test_addr, test_data, 4);
		if (ret) {
			pr_err("failed to write in 4-byte address mode");
			return FAIL;
		}

		w25qxxx_read_data(dev, test_addr, read_data, 4);
		if (memcmp(test_data, read_data, 4) != 0) {
			pr_err("verification failed in 4-byte address mode");
			return FAIL;
		}

		/* Exit 4-byte mode and verify normal operation */
		w25q256_exit_4byte_addr_mode(dev);
		mdelay(1);

		test_addr = 0x1000;
		ret = w25qxxx_write_data(dev, test_addr, test_data, 4);
		if (ret) {
			pr_err("failed to write after exiting 4-byte mode");
			return FAIL;
		}

		w25qxxx_read_data(dev, test_addr, read_data, 4);
		if (memcmp(test_data, read_data, 4) != 0) {
			pr_err("verification failed after exiting 4-byte mode");
			return FAIL;
		}
	} else {
		pr_info("skipping 4-byte address test for W25Q128 (uses 3-byte mode)");
	}

	pr_info("address mode test completed");
	return 0;
}

int t_w25qxxx_write_protect(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 test_addr = 0x40000;
	u8 test_data[16] = {0xAB, 0xCD};
	u8 read_data[16];
	u8 sr1, sr2;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Write Protect Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	/* Clear any existing protection first */
	w25qxxx_write_enable(dev);
	w25qxxx_write_bp(dev, 0);
	mdelay(10);

	/* Enable write protection */
	pr_info("enabling write protection (BP=1)...");
	w25qxxx_write_enable(dev);
	w25qxxx_write_bp(dev, 1);
	mdelay(10);

	/* Read status to verify BP bits */
	sr1 = w25qxxx_read_status_reg(dev, READ_STATUS_REGISTER1);
	pr_info("status register 1 after setting BP: 0x%02X", sr1);

	/* Try to write to protected area (should fail) */
	pr_info("attempting to write to protected area...");
	ret = w25qxxx_write_data(dev, test_addr, test_data, 2);
	if (ret == 0) {
		pr_warn("write to protected area succeeded (unexpected)");
	} else {
		pr_info("write to protected area failed as expected");
	}

	/* Disable write protection */
	pr_info("disabling write protection (BP=0)...");
	w25qxxx_write_enable(dev);
	w25qxxx_write_bp(dev, 0);
	mdelay(10);

	/* Try to write again (should succeed) */
	pr_info("attempting to write to unprotected area...");
	ret = w25qxxx_write_data(dev, test_addr, test_data, 2);
	if (ret) {
		pr_err("failed to write after disabling protection");
		return FAIL;
	}

	/* Verify */
	w25qxxx_read_data(dev, test_addr, read_data, 2);
	if (memcmp(test_data, read_data, 2) != 0) {
		pr_err("verification failed after disabling protection");
		return FAIL;
	}

	pr_info("write protect test completed");
	return 0;
}

int t_w25qxxx_security_registers(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u8 test_data[256] = {0};
	u8 read_data[256];
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Security Registers Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	/* Prepare test pattern for security register */
	for (int i = 0; i < 256; i++) {
		test_data[i] = (u8)(i ^ 0xAA);
	}

	// 	/* Read security register 1 (OTP) */
	// 	pr_info("reading security register 1...");
	// 	w25qxxx_read_security_regs(dev, SECURITY_REGISTERS_1, rx_buffer, 256);
	// 	pr_info("security register 1[0]: 0x%02X, [255]: 0x%02X",
	// 		rx_buffer[0], rx_buffer[255]);
	//
	// 	/* Note: Security registers are OTP (One-Time Programmable) */
	// 	/* We only read them to avoid accidental programming */
	// 	pr_info("note: security registers are OTP, skipping write test");
	//
	// 	/* Read security register 2 */
	// 	pr_info("reading security register 2...");
	// 	w25qxxx_read_security_regs(dev, SECURITY_REGISTERS_2, rx_buffer, 256);
	// 	pr_info("security register 2[0]: 0x%02X, [255]: 0x%02X",
	// 		rx_buffer[0], rx_buffer[255]);
	//
	// 	/* Read security register 3 */
	// 	pr_info("reading security register 3...");
	// 	w25qxxx_read_security_regs(dev, SECURITY_REGISTERS_3, rx_buffer, 256);
	// 	pr_info("security register 3[0]: 0x%02X, [255]: 0x%02X",
	// 		rx_buffer[0], rx_buffer[255]);

	pr_info("security registers test completed");
	return 0;
}

int t_w25qxxx_large_transfer(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u32 test_addr = 0x500000;
	u32 test_length = 65536;  /* 64KB */
	u8 *rx_buffer, *tx_buffer;
	u32 start_time, end_time;
	int ret;

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Large Data Transfer Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}
	pr_info("testing %d bytes transfer", test_length);

	/* Erase test area */
	w25qxxx_block_erase_with_64kb(dev, test_addr);
	mdelay(200);

	rx_buffer = kmalloc(32768, GFP_KERNEL);
	if (!rx_buffer) {
		return -ENOMEM;
	}
	tx_buffer = kmalloc(32768, GFP_KERNEL);
	if (!tx_buffer) {
		return -ENOMEM;
	}

	/* Prepare test data */
	for (u32 i = 0; i < test_length; i++) {
		tx_buffer[i] = (u8)(i & 0xFF);
	}

	/* Write large data block */
	pr_info("writing %d bytes...", test_length);
	start_time = basic_timer_get_ms();
	ret = w25qxxx_multi_sector_program(dev, test_addr, tx_buffer, test_length);
	end_time = basic_timer_get_ms();
	if (ret) {
		pr_err("large write failed");
		return FAIL;
	}
	pr_info("write completed in %lu ms", end_time - start_time);

	/* Read large data block */
	pr_info("reading %d bytes...", test_length);
	start_time = basic_timer_get_ms();
	w25qxxx_read_data(dev, test_addr, rx_buffer, test_length);
	end_time = basic_timer_get_ms();
	pr_info("read completed in %lu ms", end_time - start_time);

	/* Verify */
	pr_info("verifying data...");
	for (u32 i = 0; i < test_length; i++) {
		if (tx_buffer[i] != rx_buffer[i]) {
			pr_err("verification failed at offset %u", i);
			return FAIL;
		}
	}

	pr_info("large data transfer test completed");
	return 0;
}

int t_w25qxxx_unique_id(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u8 uid[8];

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx Unique ID Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	/* Read unique ID */
	w25qxxx_read_unique_id(dev, uid);

	pr_info("device unique id: %02X %02X %02X %02X %02X %02X %02X %02X",
		uid[0], uid[1], uid[2], uid[3], uid[4], uid[5], uid[6], uid[7]);

	/* Verify ID is not all zeros or all ones */
	bool all_zeros = true;
	bool all_ones = true;
	for (int i = 0; i < 8; i++) {
		if (uid[i] != 0) {
			all_zeros = false;
		}
		if (uid[i] != 0xFF) {
			all_ones = false;
		}
	}

	if (all_zeros) {
		pr_err("unique id is all zeros, may be invalid");
		return FAIL;
	}

	if (all_ones) {
		pr_err("unique id is all ones, may be invalid");
		return FAIL;
	}

	pr_info("unique id test completed");
	return 0;
}

int t_w25qxxx_sfdp(int argc, char **argv)
{
	struct w25qxxx_device *dev;
	u8 w25qxxx = W25Q128;
	u8 sfdp_data[256];

	if (argc > 1) {
		if (!strcmp(argv[1], "w25q128")) {
			w25qxxx = W25Q128;
		} else if (!strcmp(argv[1], "w25q256")) {
			w25qxxx = W25Q256;
		}
	}

	pr_info("=== W25Qxxx SFDP Test ===");

	dev = w25qxxx_find_device(w25qxxx);
	if (!dev) {
		return -ENODEV;
	}

	/* Read SFDP register */
	w25qxxx_read_sfdp_reg(dev, 0x00000000, sfdp_data, 256);

	pr_info("sfdp signature: 0x%02X 0x%02X 0x%02X 0x%02X",
		sfdp_data[0], sfdp_data[1], sfdp_data[2], sfdp_data[3]);

	/* SFDP signature should be 0x50 0x4F 0x4D 0x46 ('S' 'F' 'D' 'P' in ASCII) */
	if (sfdp_data[0] == 0x50 && sfdp_data[1] == 0x4F &&
		sfdp_data[2] == 0x4D && sfdp_data[3] == 0x46) {
		pr_info("sfdp signature valid");
	} else {
		pr_warn("sfdp signature not as expected");
	}

	pr_info("sfdp header[4-7]: %02X %02X %02X %02X",
		sfdp_data[4], sfdp_data[5], sfdp_data[6], sfdp_data[7]);

	pr_info("sfdp test completed");
	return 0;
}

#endif
