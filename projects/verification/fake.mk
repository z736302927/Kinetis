
# fake platform arch source + board + fatfs SD
C_SOURCES += $(wildcard ../arch/arm/fake-mcu/*.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/fake_ram_diskio.c)
# C_SOURCES += $(wildcard ../drivers/mtd/mtdcore.c)
# C_SOURCES += $(wildcard ../drivers/mtd/mtdpart.c)
# C_SOURCES += $(wildcard ../drivers/mtd/spi-nor/spi-nor-core.c)
# C_SOURCES += $(wildcard ../drivers/mtd/spi-nor/sfdp.c)
# C_SOURCES += $(wildcard ../drivers/mtd/spi-nor/winbond.c)
# C_SOURCES += $(wildcard ../drivers/spi/spi-core.c)
# C_SOURCES += $(wildcard ../drivers/spi/spi-mem.c)
# C_SOURCES += $(wildcard ../drivers/spi/spi-stm32-qspi.c)
# C_SOURCES += $(wildcard ../drivers/i2c/i2c-boardinfo.c)
# C_SOURCES += $(wildcard ../drivers/i2c/i2c-core-base.c)
# C_SOURCES += $(wildcard ../drivers/i2c/i2c-core-slave.c)
# C_SOURCES += $(wildcard ../drivers/i2c/i2c-core-smbus.c)
# C_SOURCES += $(wildcard ../drivers/i2c/i2c-mux.c)
# C_SOURCES += $(wildcard ../drivers/i2c/algos/i2c-algo-bit.c)
# C_SOURCES += $(wildcard ../drivers/i2c/mux/i2c-mux-gpio.c)
# C_SOURCES += $(wildcard ../drivers/i2c/mux/i2c-mux-pinctrl.c)
# C_SOURCES += $(wildcard ../drivers/i2c/mux/i2c-mux-reg.c)
# C_SOURCES += $(wildcard ../drivers/i2c/busses/i2c-stm32f4.c)
# C_SOURCES += $(wildcard ../drivers/misc/eeprom/at24.c)
# C_SOURCES += $(wildcard ../drivers/iio/humidity/dht11.c)
# C_SOURCES += $(wildcard ../drivers/iio/imu/inv_mpu6050/inv_mpu_acpi.c)
# C_SOURCES += $(wildcard ../drivers/iio/imu/inv_mpu6050/inv_mpu_aux.c)
# C_SOURCES += $(wildcard ../drivers/iio/imu/inv_mpu6050/inv_mpu_core.c)
# C_SOURCES += $(wildcard ../drivers/iio/imu/inv_mpu6050/inv_mpu_12c.c)
# C_SOURCES += $(wildcard ../drivers/iio/imu/inv_mpu6050/inv_mpu_magn.c)
# C_SOURCES += $(wildcard ../drivers/iio/imu/inv_mpu6050/inv_mpu_ring.c)
# C_SOURCES += $(wildcard ../drivers/iio/imu/inv_mpu6050/inv_mpu_trigger.c)
# C_SOURCES += $(wildcard ../drivers/iio/imu/bmi160/*.c)
# C_SOURCES += $(wildcard ../drivers/iio/imu/magnetometer/ak8975.c)
# C_SOURCES += $(wildcard ../drivers/iio/industrialio-buffer.c)
# C_SOURCES += $(wildcard ../drivers/iio/industrialio-core.c)
# C_SOURCES += $(wildcard ../drivers/iio/inkern.c)
# C_SOURCES += $(wildcard ../drivers/input/keyboard/gpio_keys.c)
# C_SOURCES += $(wildcard ../drivers/input/keyboard/gpio_keys_polled.c)
# C_SOURCES += $(wildcard ../drivers/input/misc/gpio-beeper.c)
# C_SOURCES += $(wildcard ../drivers/input/input.c)
# C_SOURCES += $(wildcard ../drivers/input/input-poller.c)
# C_SOURCES += $(wildcard ../drivers/input/ff-core.c)
# C_SOURCES += $(wildcard ../drivers/input/ff-memless.c)
# C_SOURCES += $(wildcard ../drivers/input/input-mt.c)
# C_SOURCES += $(wildcard ../drivers/leds/led-class.c)
# C_SOURCES += $(wildcard ../drivers/leds/led-class-flash.c)
# C_SOURCES += $(wildcard ../drivers/leds/led-class-multicolor.c)
# C_SOURCES += $(wildcard ../drivers/leds/led-core.c)
# C_SOURCES += $(wildcard ../drivers/leds/led-triggers.c)
# C_SOURCES += $(wildcard ../drivers/leds/trigger/ledtrig-heartbeat.c)
C_SOURCES += $(wildcard ../drivers/kinetis/*.c)
C_SOURCES += $(wildcard ../drivers/kinetis/lrzsz/*.c)

# Final target
TARGET = $(BUILD_DIR)/$(PROJECT).exe

# Macros for gcc
C_DEFS =  \
    -D__KERNEL__ \
    -DCONFIG_FAKE_LIB \
    -DCONFIG_SIMULATION \
    -DKINETIS_FAKE_SIM=1 \

# Linker configuration
LIBS = -lm
LIBDIR =
LDFLAGS = -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections