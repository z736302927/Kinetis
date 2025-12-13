
#include <linux/printk.h>
#include <linux/console.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <linux/spi/flash.h>
#include <linux/spi/w25qxxx.h>
#include <linux/i2c.h>
#include <linux/errname.h>

#include <kinetis/ano_protocol.h>
#include <kinetis/tim-task.h>
#include <kinetis/memory_allocator.h>

#include "kinetis-core.h"
#include "kinetis/test-kinetis.h"

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#ifdef CONFIG_FAKE_LIB
#include <fake-mcu/print.h>
#endif


struct task_struct init_task;
bool static_key_initialized = true;

unsigned long read_chip_timer(void)
{
	return 0;
}
struct delay_timer fake_delay_timer = {
	.freq = 1000000,
	.read_current_timer = read_chip_timer
};
unsigned long lpj_fine;

#define STM32_SERIAL_NAME "ttySTM"

int _putc(int ch)
{
	return printf("%c", ch);
}

int _puts(char *string, int cnt)
{
	return 0;
}

int __init initialize_ptr_random(void);
void cm_backtrace_init(const char *firmware_name,
	const char *hardware_ver, const char *software_ver);
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
int __init stm32_spi_driver_init(void);
void __exit stm32_spi_driver_exit(void);
int __init stm32f4_i2c_driver_init(void);
void __exit stm32f4_i2c_driver_exit(void);
int __init stm32_usart_init(void);
int __init stm32f429_pinctrl_init(void);
int __init gpio_keys_init(void);
void __exit gpio_keys_exit(void);
int __init gpio_beeper_platform_driver_init(void);
void __exit gpio_beeper_platform_driver_exit(void);
int __init leds_init(void);
void __exit leds_exit(void);
int __init bmi088_accel_driver_init(void);
void __exit bmi088_accel_driver_exit(void);
int __init bmi088_gyro_driver_init(void);
void __exit bmi088_gyro_driver_exit(void);
int __init ak8975_driver_init(void);
void __exit ak8975_driver_exit(void);

struct platform_device stm32_pinctrl_device = {
	.name	= "stm32h743-pinctrl",
	.id		= 0,
//	.dev	= {
//		.platform_data	= &i2c_gpio_config,
//	}
};

extern struct console stm32_console;

extern struct spi_mem_driver spi_nor_driver;
struct platform_device stm32_spi_device = {
	.name	= "stm32-spi",
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

	kineits->dt.gpio.gpio_bank = 9;
	kineits->dt.gpio.base = kcalloc(9, sizeof(*kineits->dt.gpio.base),
			GFP_KERNEL);
	if (!kineits->dt.gpio.base)
		return -ENOMEM;

//	kineits->dt.gpio.base[0] = (void *)GPIOA;
//	kineits->dt.gpio.base[1] = (void *)GPIOB;
//	kineits->dt.gpio.base[2] = (void *)GPIOC;
//	kineits->dt.gpio.base[3] = (void *)GPIOD;
//	kineits->dt.gpio.base[4] = (void *)GPIOE;
//	kineits->dt.gpio.base[5] = (void *)GPIOF;
//	kineits->dt.gpio.base[6] = (void *)GPIOG;
//	kineits->dt.gpio.base[7] = (void *)GPIOH;
//	kineits->dt.gpio.base[8] = (void *)GPIOI;

	return 0;
}

void *pthread_xtime_update(void *para)
{
	while (1) {
		usleep(USEC_PER_SEC / HZ);
		xtime_update(1);
	}
}

int fake_mcu_glue_func(void)
{
	pthread_t thread;
	int ret = 0;

//	cm_backtrace_init("CmBacktrace", "V1.0.0", "V0.1.0");

	register_current_timer_delay(&fake_delay_timer);

	ret = initialize_ptr_random();
	if (ret)
		return ret;

	timekeeping_init();
	ret = pthread_create(&thread, NULL, pthread_xtime_update, NULL);
	if (ret)
		return ret;

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

//	if (!task_sch_init(3000))
//		return -ENOMEM;
//
//	ret = input_init();
//	if (ret)
//		return ret;
//
//	ret = iio_init();
//	if (ret)
//		return ret;
//
//	ret = gpiolib_dev_init();
//	if (ret)
//		return ret;
//
//	ret = spi_init();
//	if (ret)
//		return ret;
//
//	ret = init_mtd();
//	if (ret)
//		return ret;
//
//	ret = nvmem_init();
//	if (ret)
//		return ret;
//
//	ret = stm32f429_pinctrl_init();
//	if (ret)
//		return ret;
//
//	ret = platform_device_register(&stm32_pinctrl_device);
//	if (ret)
//		return ret;

// 	ret = stm32_usart_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = platform_device_register(&stm32_usart_device);
// 	if (ret)
// 		return ret;

//	ret = spi_mem_driver_register(&spi_nor_driver);
//	if (ret)
//		return ret;
//
//	ret = stm32_spi_driver_init();
//	if (ret)
//		return ret;
//
//	ret = platform_device_register(&stm32_spi_device);
//	if (ret)
//		return ret;
//
//	ret = spi_register_board_info(drone_sensor_spi__info,
//								  ARRAY_SIZE(drone_sensor_spi__info));
//	if (ret)
//		pr_warn("%s: spi info registration failed: %d\n",
//				__func__, ret);
//
//	ret = leds_init();
//	if (ret)
//		return ret;
//
//	ret = bmi088_accel_driver_init();
//	if (ret)
//		return ret;
//
//	ret = bmi088_gyro_driver_init();
//	if (ret)
//		return ret;
//
//	ret = ak8975_driver_init();
//	if (ret)
//		return ret;

	return 0;
}

int main(int argc, char **argv)
{
	int ret;
	float fval = 0.5f;

	ret = fake_mcu_glue_func();
	if (ret)
		goto err;

//     test_memory_allocator();

//	ret = fmu_init();
//	if (ret)
//		goto err;

	pr_info("|---------------------------------|\n");
	pr_info("666, Kineits system has been setup.\n");
	pr_info("|---------------------------------|\n");
	
	pr_info("0.5f: %f\n", fval);

	WARN_ON_ONCE(1);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */
		ret = k_test_case_schedule();
		if (ret)
			break;
		/* USER CODE BEGIN 3 */
	}

err:
	while (1) {
		pr_err("system crash, error code: %s(%d)\n",
			errname(ret), ret);
        mdelay(1000);
	}
}