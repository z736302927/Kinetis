
#include <generated/deconfig.h>
#include <linux/printk.h>
#include <linux/console.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <linux/spi/flash.h>
#include <linux/spi/w25qxxx.h>
#include <linux/i2c.h>

#include "kinetis-core.h"
#include "stdio.h"
#include "usart.h"
#include "tim.h"
//#include "spi.h"

#include <cm_backtrace.h>

struct task_struct init_task;

#define STM32_SERIAL_NAME "ttySTM"

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
int fputc(int ch, FILE *f)
{
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART3 and Loop until the end of transmission */
	HAL_UART_Transmit(&huart1, (u8 *)&ch, 1, 100);

	return ch;
}

int _putc(int ch)
{
	return HAL_UART_Transmit(&huart1, (u8 *)&ch, 1, 100);
}

/*
 * SPI Devices:
 * SPI0: 1M Flash Winbond w25q32bv
 */
static const struct flash_platform_data w25q256_spi_flash_data = {
	.type		= "w25q256",
};

struct spi_board_info __initdata w25q256_spi_flash_info[] = {
	{
		.modalias       = "w25q256",
		.platform_data  = &w25q256_spi_flash_data,
		.irq            = -1,
		.max_speed_hz   = 100000000,
		.bus_num        = 0,
		.chip_select    = 0,
	},
};

/*
 * I2C Devices:
 * I2C0: 256 Bytes nvem
 */
struct i2c_board_info __initdata fire_i2c_board_info[] = {
	{I2C_BOARD_INFO("24c02", 0x50)},
	{I2C_BOARD_INFO("mpu6050", 0x68)},
};

//static int w25qxxx_port_transmmit(u8 w25qxxx,
//								  void *pdata, u32 length)
//{
//    switch (w25qxxx) {
//        case W25Q256:
//            HAL_SPI_Transmit(&hspi5, pdata, length, 1000);
//            break;
//
//        default:
//            break;
//    }
//
//	return 0;
//}
//
//static int w25qxxx_port_receive(u8 w25qxxx,
//								void *pdata, u32 length)
//{
//    switch (w25qxxx) {
//        case W25Q256:
//            HAL_SPI_Receive(&hspi5, pdata, length, 1000);
//            break;
//
//        default:
//            break;
//    }
//
//	return 0;
//}
//
//static void w25qxxx_set_cs(u8 id, bool level)
//{
//    switch (id) {
//        case W25Q256:
//            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10,
//                level ? GPIO_PIN_SET : GPIO_PIN_RESET);
//            break;
//
//        default:
//            break;
//    }
//}
//
//static void w25qxxx_hard_reset(u8 id)
//{
//
//}
//
//const struct w25qxxx_ops w25qxxx_io = {
//	.transmmit = w25qxxx_port_transmmit,
//	.receive = w25qxxx_port_receive,
//	.set_cs = w25qxxx_set_cs,
//	.hw_reset = w25qxxx_hard_reset,
//};

int __init initialize_ptr_random(void);
void __init timekeeping_init(void);
int __init find_bit_test(void);
void __init radix_tree_init(void);
int __init btree_module_init(void);
int __init software_node_init(void);
int __init devices_init(void);
int __init buses_init(void);
int __init platform_bus_init(void);
int __init input_init(void);
void __exit input_exit(void);
int __init iio_init(void);
void __exit iio_exit(void);
int __init gpiolib_dev_init(void);
int __init spi_init(void);
int __init i2c_init(void);
int __init init_mtd(void);
int __init nvmem_init(void);
int __init gpiolib_dev_init(void);
int __init at24_init(void);
void __exit at24_exit(void);
int __init stm32_qspi_driver_init(void);
void __exit stm32_qspi_driver_exit(void);
int __init stm32f4_i2c_driver_init(void);
void __exit stm32f4_i2c_driver_exit(void);
int __init stm32_usart_init(void);
int __init stm32h743_pinctrl_init(void);
int __init dht11_driver_init(void);
void __exit dht11_driver_exit(void);
int __init gpio_keys_init(void);
void __exit gpio_keys_exit(void);
int __init gpio_beeper_platform_driver_init(void);
void __exit gpio_beeper_platform_driver_exit(void);
int __init leds_init(void);
void __exit leds_exit(void);
int __init inv_mpu_driver_init(void);
void __exit inv_mpu_driver_exit(void);

struct platform_device stm32_pinctrl_device = {
	.name	= "stm32h743-pinctrl",
	.id		= 0,
//	.dev	= {
//		.platform_data	= &i2c_gpio_config,
//	}
};

extern struct console stm32_console;

extern struct spi_mem_driver spi_nor_driver;
struct platform_device stm32_qspi_device = {
	.name	= "stm32-qspi",
	.id		= 0,
//	.dev	= {
//		.platform_data	= &i2c_gpio_config,
//	}
};

struct platform_device stm32f4_i2c_device = {
	.name	= "stm32f4-i2c",
	.id		= 0,
//	.dev	= {
//		.platform_data	= &i2c_gpio_config,
//	}
};

struct platform_device stm32_usart_device = {
	.name	= "stm32-usart",
	.id		= 0,
//	.dev	= {
//		.platform_data	= &i2c_gpio_config,
//	}
};

static struct kineits_system stm32_common_val;

struct kineits_system *lib_get_stm32_val(void)
{
	return &stm32_common_val;
}

static int fill_stm32_dt(void)
{
	struct kineits_system *kineits = lib_get_stm32_val();
	
	kineits->dt.at24.byte_len = 256;
	kineits->dt.at24.page_size = 8;

	kineits->dt.gpio.gpio_bank = 11;
	kineits->dt.gpio.base = kcalloc(11, sizeof(*kineits->dt.gpio.base),
		GFP_KERNEL);
	if (!kineits->dt.gpio.base)
		return -ENOMEM;

	kineits->dt.gpio.base[0] = (void *)GPIOA;
	kineits->dt.gpio.base[1] = (void *)GPIOB;
	kineits->dt.gpio.base[2] = (void *)GPIOC;
	kineits->dt.gpio.base[3] = (void *)GPIOD;
	kineits->dt.gpio.base[4] = (void *)GPIOE;
	kineits->dt.gpio.base[5] = (void *)GPIOF;
	kineits->dt.gpio.base[6] = (void *)GPIOG;
	kineits->dt.gpio.base[7] = (void *)GPIOH;
	kineits->dt.gpio.base[8] = (void *)GPIOI;
	kineits->dt.gpio.base[9] = (void *)GPIOJ;
	kineits->dt.gpio.base[10] = (void *)GPIOK;

	return 0;
}

int stm32h7_glue_func(void)
{
	struct stm32f4_i2c_dev *stm32f4_i2c_priv;
    struct i2c_client *client_mpu6050;
    struct i2c_client *client_at24;
	int ret = 0;

	cm_backtrace_init("CmBacktrace", "V1.0.0", "V0.1.0");

	ret = initialize_ptr_random();
	if (ret)
		return ret;

	timekeeping_init();
	HAL_TIM_Base_Start_IT(&htim2);

	register_console(&stm32_console);

	ret = fill_stm32_dt();
	if (ret)
		return ret;

//    ret = find_bit_test();
//    if (ret)
//        return ret;

	radix_tree_init();

	ret = btree_module_init();
	if (ret)
		return ret;

	ret = software_node_init();
	if (ret)
		return ret;

	ret = devices_init();
	if (ret)
		return ret;

	ret = buses_init();
	if (ret)
		return ret;

	ret = platform_bus_init();
	if (ret)
		return ret;

	ret = input_init();
	if (ret)
		return ret;

	ret = iio_init();
	if (ret)
		return ret;

	ret = gpiolib_dev_init();
	if (ret)
		return ret;

	ret = spi_init();
	if (ret)
		return ret;

	ret = i2c_init();
	if (ret)
		return ret;

	ret = init_mtd();
	if (ret)
		return ret;

	ret = nvmem_init();
	if (ret)
		return ret;

	ret = at24_init();
	if (ret)
		return ret;

	ret = stm32h743_pinctrl_init();
	if (ret)
		return ret;

	ret = platform_device_register(&stm32_pinctrl_device);
	if (ret)
		return ret;

//	ret = stm32_usart_init();
//	if (ret)
//		return ret;

	ret = platform_device_register(&stm32_usart_device);
	if (ret)
		return ret;

	ret = stm32f4_i2c_driver_init();
	if (ret)
		return ret;

	ret = platform_device_register(&stm32f4_i2c_device);
	if (ret)
		return ret;

	ret = i2c_register_board_info(1, fire_i2c_board_info,
								  ARRAY_SIZE(fire_i2c_board_info));
	if (ret)
		pr_warn("%s: i2c info registration failed: %d\n",
				__func__, ret);

	stm32f4_i2c_priv = platform_get_drvdata(&stm32f4_i2c_device);
    if (IS_ERR(stm32f4_i2c_priv))
        return -ENODEV;
	client_at24 = i2c_new_client_device(&stm32f4_i2c_priv->adap, &fire_i2c_board_info[0]);
    if (IS_ERR(client_at24))
        return -ENODEV;
	client_mpu6050 = i2c_new_client_device(&stm32f4_i2c_priv->adap, &fire_i2c_board_info[1]);
    if (IS_ERR(client_mpu6050))
        return -ENODEV;

	ret = inv_mpu_driver_init();
	if (ret)
		return ret;

	ret = spi_mem_driver_register(&spi_nor_driver);
	if (ret)
		return ret;

	ret = stm32_qspi_driver_init();
	if (ret)
		return ret;

	ret = platform_device_register(&stm32_qspi_device);
	if (ret)
		return ret;

	ret = spi_register_board_info(w25q256_spi_flash_info,
								  ARRAY_SIZE(w25q256_spi_flash_info));
	if (ret)
		pr_warn("%s: spi info registration failed: %d\n",
				__func__, ret);

	ret = dht11_driver_init();
	if (ret)
		return ret;

	ret = gpio_keys_init();
	if (ret)
		return ret;

	ret = gpio_beeper_platform_driver_init();
	if (ret)
		return ret;

	ret = leds_init();
	if (ret)
		return ret;

	return 0;
}

