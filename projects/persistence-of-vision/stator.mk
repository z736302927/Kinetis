
CHIP_SRC_DIR ?= E:\Code\STM32F407ZGT6

# Kinetis arch source + board + fatfs SD
C_SOURCES += $(wildcard ../arch/arm/mach-stm32/stm32h7.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/sd_diskio.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/bsp_driver_sd.c)
C_SOURCES += $(wildcard ../drivers/kinetis/fatfs-intf.c)
C_SOURCES += $(wildcard ../drivers/kinetis/fsm.c)
C_SOURCES += $(wildcard ../drivers/kinetis/hc-05.c)
C_SOURCES += $(wildcard ../drivers/kinetis/led.c)
C_SOURCES += $(wildcard ../drivers/kinetis/mavlink.c)
C_SOURCES += $(wildcard ../drivers/kinetis/bootloader.c)
# C_SOURCES += $(wildcard ../drivers/kinetis/w25qxxx.c)
C_SOURCES += $(wildcard ../drivers/kinetis/lrzsz/*.c)
C_SOURCES += $(wildcard ../drivers/kinetis/kalman-clib/src/matrix.c)
C_SOURCES += $(wildcard ../drivers/kinetis/kalman-clib/src/kalman.c)
C_SOURCES += $(wildcard ../drivers/kinetis/kalman-clib/src/cholesky.c)
C_SOURCES += $(wildcard ../drivers/kinetis/nrf24l01/src/driver_nrf24l01.c)
C_SOURCES += $(wildcard ../drivers/kinetis/nrf24l01_port.c)
C_SOURCES += $(wildcard ../drivers/kinetis/w25qxx/src/driver_w25qxx.c)
C_SOURCES += $(wildcard ../drivers/kinetis/w25qxx_port.c)

# CubeMX core/peripheral sources
C_SOURCES += $(wildcard $(CHIP_SRC_DIR)/Core/Src/*.c)

# HAL driver sources (auto-detect via wildcard — no need to list each file)
C_SOURCES += $(wildcard $(CHIP_SRC_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/*.c)

C_SOURCES += $(wildcard ../drivers/kinetis/CmBacktrace/cm_backtrace/*.c)

EXCLUDE_SOURCES += $(wildcard ../projects/persistence-of-vision/test.c)

# Startup assembly
S_SOURCES += $(CHIP_SRC_DIR)/startup_stm32f407xx.s
S_SOURCES += ../drivers/kinetis/CmBacktrace/cm_backtrace/fault_handler/gcc/cmb_fault.s

C_INCLUDES += -I$(CHIP_SRC_DIR)/Core/Inc
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/CMSIS/Device/ST/STM32F4xx/Include
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/CMSIS/Include

C_INCLUDES += -I../drivers/kinetis/CmBacktrace/cm_backtrace
C_INCLUDES += -I../drivers/kinetis/kalman-clib/include
C_INCLUDES += -I../drivers/kinetis/nrf24l01/src
C_INCLUDES += -I../drivers/kinetis/nrf24l01/interface
C_INCLUDES += -I../drivers/kinetis/w25qxx/src
C_INCLUDES += -I../drivers/kinetis/w25qxx/interface

HEADER_DIRS += ../arch/arm/mach-stm32

HEADERS += $(wildcard ../arch/arm/mach-stm32/*.h)

#--- MCU architecture flags (used by Makefile's CHIP toolchain section) ---
PREFIX = arm-none-eabi-
# CHIP architecture flags (from chip.mk)
CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)
CHIP_ARCH_CFLAGS = $(MCU) -fno-short-enums

# Final target
TARGET = $(BUILD_DIR)/$(PROJECT)

# Macros for gcc
C_DEFS = \
	-D__KERNEL__ \
	-DUSE_HAL_DRIVER \
    -DSTM32F407xx \
    -DUSE_PWR_LDO_SUPPLY \
    -DSTM32_THREAD_SAFE_STRATEGY=2 \
    -DPOV_RUN_MODE=POV_RUN_SLAVE \
	-DCMB_USER_CFG \
	-D_POSIX_C_SOURCE=200809L \
	-DCMB_CPU_PLATFORM_TYPE=CMB_CPU_ARM_CORTEX_M4

# Linker configuration
LDSCRIPT = $(CHIP_SRC_DIR)/STM32F407XX_APP_FLASH.ld
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections -Wl,--no-enum-size-warning
