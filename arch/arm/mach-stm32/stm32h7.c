
#include <linux/printk.h>
#include <linux/console.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <linux/spi/flash.h>
#include <linux/spi/w25qxxx.h>
#include <linux/i2c.h>
#include <linux/errname.h>
#include <linux/random.h>
#include <linux/prandom.h>
#include <linux/utsname.h>

#include <kinetis/ano_protocol.h>
#include <kinetis/tim-task.h>
#include "kinetis/rtc-task.h"
#include <kinetis/memory_allocator.h>
#include <kinetis/real-time-clock.h>
#include <kinetis/basic-timer.h>

#include "kinetis-core.h"

#include "gpio.h"

DEFINE_RWLOCK(tasklist_lock);
DEFINE_SPINLOCK(mmlist_lock);

struct uts_namespace init_uts_ns = {
	.kref = {
		.refcount = ATOMIC_INIT(1),
	},
	.name = {
		.sysname = "FakeOS",
		.nodename = "fake-device",
		.release = "1.0.0",
		.version = "#1 Mon Jan 1 00:00:00 UTC 2024",
		.machine = "arm-fake-mcu",
		.domainname = "(none)",
	},
	.user_ns = NULL,
	.ucounts = NULL,
	.ns = {
		.inum = 0,
		.ops = NULL,
	},
};
struct nsproxy init_nsproxy = {
	.count = ATOMIC_INIT(1),
	.uts_ns = &init_uts_ns,
};
struct task_struct init_task = {
	.nsproxy = &init_nsproxy
};
bool static_key_initialized = true;

/* Fake system state for embedded systems */
enum system_states system_state = SYSTEM_BOOTING;

/* Fake interrupt registers for entropy collection */
DEFINE_PER_CPU(struct pt_regs *, __irq_regs);

/* Fake CPU masks for uniprocessor system */
struct cpumask __cpu_possible_mask;
struct cpumask __cpu_online_mask;
struct cpumask __cpu_present_mask;
struct cpumask __cpu_active_mask;
atomic_t __num_online_cpus = ATOMIC_INIT(1);

unsigned long read_chip_timer(void);

struct delay_timer general_delay_timer = {
	.freq = 1000000,
	.read_current_timer = read_chip_timer
};
unsigned long lpj_fine;

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

//static int w25qxxx_port_transmit(u8 w25qxxx,
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
//	.transmit = w25qxxx_port_transmit,
//	.receive = w25qxxx_port_receive,
//	.set_cs = w25qxxx_set_cs,
//	.hw_reset = w25qxxx_hard_reset,
//};

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
// 	kineits->dt.gpio.base[9] = (void *)GPIOJ;
// 	kineits->dt.gpio.base[10] = (void *)GPIOK;

	return 0;
}

int stm32h7_glue_func(void)
{
	struct stm32f4_i2c_dev *stm32f4_i2c_priv;
    struct i2c_client *client_mpu6050;
    struct i2c_client *client_at24;
	u64 entropy[16];
	struct tm ts;
	int ret = 0;

//	cm_backtrace_init("CmBacktrace", "V1.0.0", "V0.1.0");

	/* Initialize CPU masks for uniprocessor system */
	cpumask_set_cpu(0, &__cpu_possible_mask);
	cpumask_set_cpu(0, &__cpu_online_mask);
	cpumask_set_cpu(0, &__cpu_present_mask);
	cpumask_set_cpu(0, &__cpu_active_mask);

	register_current_timer_delay(&general_delay_timer);

	prandom_init_early();

	ret = initialize_ptr_random();
	if (ret)
		return ret;

	init_timers();

	timekeeping_init();
// 	HAL_TIM_Base_Start_IT(&htim2);

	ret = rand_initialize();
	if (ret)
		return ret;

	/* Add enough entropy to initialize CRNG in fake environment */
	/* CRNG requires at least 64 bytes (CRNG_INIT_CNT_THRESH) */
	rtc_calendar_get(&general_rtc, &ts, KRTC_FORMAT_BIN);
	for (int i = 0; i < 16; i++) {
		u64 ts_val = (u64)(ts.tm_year * 365ULL * 86400ULL +
				    ts.tm_mon * 30ULL * 86400ULL +
				    ts.tm_mday * 86400ULL +
				    ts.tm_hour * 3600ULL +
				    ts.tm_min * 60ULL +
				    ts.tm_sec);
		entropy[i] = 0x1234567890abcdefULL ^ ts_val * (i + 1);
	}
	add_device_randomness(entropy, sizeof(entropy));

	if (rng_is_initialized()) {
		pr_info("RNG is initialized and ready\n");
	} else {
		pr_info("RNG is not yet initialized\n");
	}

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

// 	ret = input_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = iio_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = gpiolib_dev_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = spi_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = i2c_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = init_mtd();
// 	if (ret)
// 		return ret;
// 
// 	ret = nvmem_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = at24_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = stm32h743_pinctrl_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = platform_device_register(&stm32_pinctrl_device);
// 	if (ret)
// 		return ret;
// 
//	ret = stm32_usart_init();
//	if (ret)
//		return ret;
// 
// 	ret = platform_device_register(&stm32_usart_device);
// 	if (ret)
// 		return ret;
// 
// 	ret = stm32f4_i2c_driver_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = platform_device_register(&stm32f4_i2c_device);
// 	if (ret)
// 		return ret;
// 
// 	ret = i2c_register_board_info(1, fire_i2c_board_info,
// 								  ARRAY_SIZE(fire_i2c_board_info));
// 	if (ret)
// 		pr_warn("%s: i2c info registration failed: %d\n",
// 				__func__, ret);
// 
// 	stm32f4_i2c_priv = platform_get_drvdata(&stm32f4_i2c_device);
//     if (IS_ERR(stm32f4_i2c_priv))
//         return -ENODEV;
// 	client_at24 = i2c_new_client_device(&stm32f4_i2c_priv->adap, &fire_i2c_board_info[0]);
//     if (IS_ERR(client_at24))
//         return -ENODEV;
// 	client_mpu6050 = i2c_new_client_device(&stm32f4_i2c_priv->adap, &fire_i2c_board_info[1]);
//     if (IS_ERR(client_mpu6050))
//         return -ENODEV;
// 
// 	ret = inv_mpu_driver_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = spi_mem_driver_register(&spi_nor_driver);
// 	if (ret)
// 		return ret;
// 
// 	ret = stm32_qspi_driver_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = platform_device_register(&stm32_qspi_device);
// 	if (ret)
// 		return ret;
// 
// 	ret = spi_register_board_info(w25q256_spi_flash_info,
// 								  ARRAY_SIZE(w25q256_spi_flash_info));
// 	if (ret)
// 		pr_warn("%s: spi info registration failed: %d\n",
// 				__func__, ret);
// 
// 	ret = dht11_driver_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = gpio_keys_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = gpio_beeper_platform_driver_init();
// 	if (ret)
// 		return ret;
// 
// 	ret = leds_init();
// 	if (ret)
// 		return ret;

	return 0;
}

int main(int argc, char **argv)
{
	int ret;

	ret = board_init();
	if (ret)
		goto err;

	/* Kernel and subsystem initialization */
	ret = stm32h7_glue_func();
	if (ret)
		goto err;

	pr_info("|-----------------------------------------|\n");
	pr_info("|  Kinetis system has been setup.         |\n");
	pr_info("|  Project: %-30s|\n", PROJECT_NAME);
	pr_info("|-----------------------------------------|\n");

	ret = app_main();

err:
	pr_err("system crash, error code: %s(%d)\n",
		errname(ret), ret);
	return ret;
}

