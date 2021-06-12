#include "kinetis/iic_soft.h"

#include <linux/random.h>
#include <linux/radix-tree.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/spi/flash.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/bitmap.h>
#include <linux/nvmem-consumer.h>
#include <linux/platform_device.h>
#include <linux/platform_data/i2c-gpio.h>
#include <linux/platform_data/spi-gpio.h>
#include <linux/platform_data/stm32f4.h>
#include <linux/platform_data/invensense_mpu6050.h>
#include <linux/platform_data/at24.h>
#include <linux/platform_data/ssd1307fb.h>
#include <linux/mtd/mtd.h>
#include <linux/rtc.h>
#include <linux/rtc/ds1307.h>
#include <linux/backlight.h>
#include <linux/gpio/machine.h>

static struct i2c_gpio_platform_data i2c_gpio_config = {
	.udelay = 5,
	.timeout = 100,
	.sda_is_open_drain = true,
	.scl_is_open_drain = true,
	.scl_is_output_only = true
};

struct platform_device kinetis_gpio_i2c0 = {
    .name	= "i2c-gpio",
//    .name	= "stm32f4-i2c",
    .id		= 0,
	.dev	= {
		.platform_data	= &i2c_gpio_config,
	}
};

static struct gpiod_lookup_table kinetis_i2c_gpiod_table = {
	.dev_id         = "i2c-gpio.0",
	.table          = {
		GPIO_LOOKUP("GPIOB", 6,
			    "scl", GPIO_ACTIVE_HIGH),
		GPIO_LOOKUP("GPIOB", 7,
			    "sda", GPIO_ACTIVE_HIGH),
	},
};

static struct spi_gpio_platform_data spi_gpio_config = {
	.num_chipselect	= 1,
};

static struct platform_device kinetis_gpio_spi0 = {
    .name	= "spi_gpio",
//    .name	= "stm32f4-spi",
    .id		= 0,
	.dev	= {
		.platform_data	= &spi_gpio_config,
	}
};

static struct gpiod_lookup_table kinetis_spi_gpiod_table = {
	.dev_id         = "spi_gpio.0",
	.table          = {
		GPIO_LOOKUP("GPIOM", 1,
			    "sck", GPIO_ACTIVE_HIGH),
		GPIO_LOOKUP("GPIOM", 2,
			    "mosi", GPIO_ACTIVE_HIGH),
		GPIO_LOOKUP("GPIOM", 3,
			    "miso", GPIO_ACTIVE_HIGH),
		GPIO_LOOKUP("GPIOM", 0,
			    "cs", GPIO_ACTIVE_HIGH),
	},
};

int kineitis_module_boot(void)
{
    struct i2c_gpio_private_data *i2c_gpio_priv;
    struct stm32f4_i2c_dev *stm32f4_i2c_priv;
    struct i2c_client *client_mpu6050;
    struct i2c_client *client_at24;
    int i, ret;

    ret = prandom_init_late();

    if (ret)
        return ret;

//    iic_soft_init();

    radix_tree_init();

    ret = platform_bus_init();

    if (ret)
        return ret;

//    ret = i2c_init();

//    if (ret)
//        return ret;
    
//	gpiod_add_lookup_table(&kinetis_spi_gpiod_table);

//    ret = i2c_gpio_init();

//    if (ret)
//        return ret;

//    ret = i2c_stm32f4_init();

//    if (ret)
//        return ret;

//    ret = platform_device_register(&kinetis_gpio_i2c0);

//    if (ret)
//        return ret;

    ret = spi_init();

    if (ret)
        return ret;
    
	gpiod_add_lookup_table(&kinetis_spi_gpiod_table);

    ret = spi_gpio_init();

    if (ret)
        return ret;

//    ret = stm32_spi_init();

//    if (ret)
//        return ret;

    ret = platform_device_register(&kinetis_gpio_spi0);

    if (ret)
        return ret;

//    ret = iio_init();

//    if (ret)
//        return ret;

//    ret = nvmem_init();

//    if (ret)
//        return ret;

//    if (!strcmp("i2c-gpio", kinetis_gpio_i2c0.name)) {
//        i2c_gpio_priv = platform_get_drvdata(&kinetis_gpio_i2c0);
//        client_at24 = i2c_new_client(&i2c_gpio_priv->adap, "24c02", 0x50);
//    } else if (!strcmp("stm32f4-i2c", kinetis_gpio_i2c0.name)) {
//        stm32f4_i2c_priv = platform_get_drvdata(&kinetis_gpio_i2c0);
//        client_at24 = i2c_new_client(&stm32f4_i2c_priv->adap, "24c02", 0x50);
//    } else
//        return -ENODEV;

//    if (IS_ERR(client_at24))
//        return -ENODEV;

//    ret = at24_init();

//    if (ret)
//        return ret;

//    if (!strcmp("i2c-gpio", kinetis_gpio_i2c0.name)) {
//        i2c_gpio_priv = platform_get_drvdata(&kinetis_gpio_i2c0);
//        client_mpu6050 = i2c_new_client(&i2c_gpio_priv->adap, "mpu6050", 0x68);
//    } else if (!strcmp("stm32f4-i2c", kinetis_gpio_i2c0.name)) {
//        stm32f4_i2c_priv = platform_get_drvdata(&kinetis_gpio_i2c0);
//        client_mpu6050 = i2c_new_client(&stm32f4_i2c_priv->adap, "mpu6050", 0x34);
//    } else
//        return -ENODEV;

//    if (IS_ERR(client_mpu6050))
//        return -ENODEV;

//    ret = inv_mpu_driver_init();

//    if (ret)
//        return ret;

    ret = spi_nor_driver_init();

    if (ret)
        return ret;

    ret = init_mtd();

    if (ret)
        return ret;

//    ret = rtc_init();

//    if (ret)
//        return ret;

//    ret = ds1307_driver_init();

//    if (ret)
//        return ret;

//    ret = backlight_class_init();

//    if (ret)
//        return ret;

//    ret = fbmem_init();

//    if (ret)
//        return ret;

//    ret = ssd1307fb_driver_init();

//    if (ret)
//        return ret;
    
    return 0;
}
