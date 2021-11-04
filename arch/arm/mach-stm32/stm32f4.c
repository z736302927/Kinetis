
#include <generated/deconfig.h>
#include <linux/printk.h>
#include <linux/console.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <linux/spi/flash.h>
#include <linux/spi/w25qxxx.h>

#include <fatfs/ff.h>

#include "stdio.h"
#include "spi.h"
#include "usart.h"
#include "tim.h"

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

static void stm32_usart_console_write(struct console *co, const char *s,
    unsigned int cnt)
{
    unsigned int i;

    for (i = 0; i < cnt; i++)
        HAL_UART_Transmit(&huart1, (u8 *)&s[i], 1, 100);

    if (s[i] == '\n')
        HAL_UART_Transmit(&huart1, (u8 *)'\r', 1, 100);
}

static int stm32_usart_console_setup(struct console *co, char *options)
{
    return 0;
}

static struct console stm32_console = {
    .name		= STM32_SERIAL_NAME,
//	.device		= uart_console_device,
    .write		= stm32_usart_console_write,
    .setup		= stm32_usart_console_setup,
    .flags		= CON_PRINTBUFFER,
    .index		= -1,
//	.data		= &stm32_usart_driver,
};

/*
 * SPI Devices:
 * SPI0: 1M Flash Winbond w25q32bv
 */
static const struct flash_platform_data w25q256_spi_flash_data = {
    .type		= "w25q128",
};

struct spi_board_info __initdata w25q256_spi_flash_info[] = {
    {
        .modalias       = "w25q128",
        .platform_data  = &w25q256_spi_flash_data,
        .irq            = -1,
        .max_speed_hz   = 100000000,
        .bus_num        = 0,
        .chip_select    = 0,
    },
};

static int w25qxxx_port_transmmit(u8 w25qxxx,
    void *pdata, u32 length)
{
    switch (w25qxxx) {
        case W25Q128:
            HAL_SPI_Transmit(&hspi1, pdata, length, 1000);
            break;

//        case W25Q256:
//            HAL_SPI_Transmit(&hspi2, pdata, length, 1000);
//            break;

        default:
            break;
    }

    return 0;
}

static int w25qxxx_port_receive(u8 w25qxxx,
    void *pdata, u32 length)
{
    switch (w25qxxx) {
        case W25Q128:
            HAL_SPI_Receive(&hspi1, pdata, length, 1000);
            break;

//        case W25Q256:
//            HAL_SPI_Receive(&hspi2, pdata, length, 1000);
//            break;

        default:
            break;
    }

    return 0;
}

static void w25qxxx_set_cs(u8 id, bool level)
{
    switch (id) {
        case W25Q128:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14,
                level ? GPIO_PIN_SET : GPIO_PIN_RESET);
            break;

//        case W25Q256:
//            HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6,
//                level ? GPIO_PIN_SET : GPIO_PIN_RESET);
//            break;

        default:
            break;
    }
}

static void w25qxxx_hard_reset(u8 id)
{

}

const struct w25qxxx_ops w25qxxx_io = {
    .transmmit = w25qxxx_port_transmmit,
    .receive = w25qxxx_port_receive,
    .set_cs = w25qxxx_set_cs,
    .hw_reset = w25qxxx_hard_reset,
};

#define KERNEL_ADDR     0x68000000
#define DTB_ADDR        0x68300000

static void start_kernel(void)
{
    void (*kernel)(u32 reserved, u32 mach, u32 dt) =
        (void (*)(u32, u32, u32))(KERNEL_ADDR | 1);

    kernel(0, ~0UL, DTB_ADDR);
}

static int load_linux(void)
{
    FIL file;
    FRESULT res;
    u32 bytes_read;

    res = f_open(&file, "0:/xipImage",
            FA_OPEN_EXISTING | FA_READ);

    if (res == FR_OK) {
        res = f_read(&file, (void *)KERNEL_ADDR,
                file.obj.objsize, &bytes_read);

        if (res != FR_OK)
            return -EFAULT;
    } else {
        pr_err("Failed to load xipImages.\n");
        return -EINVAL;
    }

    f_close(&file);

    res = f_open(&file, "0:/stm32f407-disco.dtb",
            FA_OPEN_EXISTING | FA_READ);

    if (res == FR_OK) {
        res = f_read(&file, (void *)DTB_ADDR,
                file.obj.objsize, &bytes_read);

        if (res != FR_OK)
            return -EFAULT;
    } else {
        pr_err("Failed to load stm32f407-disco.dtb.\n");
        return -EINVAL;
    }

    f_close(&file);

    start_kernel();

    return 0;
}


int __init initialize_ptr_random(void);
void __init timekeeping_init(void);
int __init find_bit_test(void);
void __init radix_tree_init(void);
int __init software_node_init(void);
int __init devices_init(void);
int __init buses_init(void);
int __init platform_bus_init(void);
int __init gpiolib_dev_init(void);
int __init spi_init(void);
int __init init_mtd(void);
int __init nvmem_init(void);
int __init gpiolib_dev_init(void);
int fatfs_init(void);

extern struct spi_mem_driver spi_nor_driver;
extern struct platform_driver stm32_spi_driver;

struct platform_device stm32_spi_device = {
    .name	= "spi-stm32",
    .id		= 0,
    .dev	= {
        .platform_data	= "st,stm32f4-spi",
    }
};

int stm32h7_glue_func(void)
{
    int ret = 0;

    asm volatile("dsb");
    asm volatile("isb");

    ret = initialize_ptr_random();

    if (ret)
        return ret;

    timekeeping_init();
    HAL_TIM_Base_Start_IT(&htim2);

    register_console(&stm32_console);

//    ret = find_bit_test();
//    if (ret)
//        return ret;

    radix_tree_init();

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

    ret = gpiolib_dev_init();

    if (ret)
        return ret;

    ret = fatfs_init();

    if (ret)
        return ret;

    ret = load_linux();

    if (ret)
        return ret;

//    ret = spi_init();
//    if (ret)
//        return ret;
//
//    ret = platform_driver_register(&stm32_spi_driver);
//    if (ret)
//        return ret;

//    ret = platform_device_register(&stm32_spi_device);
//    if (ret)
//        return ret;
//
//    ret = init_mtd();
//    if (ret)
//        return ret;
//
//    ret = nvmem_init();
//    if (ret)
//        return ret;
//
//    ret = spi_mem_driver_register(&spi_nor_driver);
//    if (ret)
//        return ret;

//	ret = spi_register_board_info(w25q256_spi_flash_info,
//				      ARRAY_SIZE(w25q256_spi_flash_info));
//	if (ret)
//		pr_warn("%s: spi info registration failed: %d\n",
//			__func__, ret);

    return ret;
}

