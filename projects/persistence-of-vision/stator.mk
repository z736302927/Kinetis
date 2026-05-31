
CHIP_SRC_DIR ?= E:\Code\STM32F407ZGT6

# Kinetis arch source + board + fatfs SD
C_SOURCES += $(wildcard ../arch/arm/mach-stm32/stm32h7.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/sd_diskio.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/bsp_driver_sd.c)
C_SOURCES += $(wildcard ../drivers/pinctrl/pinctrl-stm32h743.c)

# CubeMX core/peripheral sources
C_SOURCES += $(wildcard $(CHIP_SRC_DIR)/Core/Src/*.c)

# HAL driver sources (auto-detect via wildcard — no need to list each file)
C_SOURCES += $(wildcard $(CHIP_SRC_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/*.c)

# Startup assembly
S_SOURCES += $(CHIP_SRC_DIR)/startup_stm32f407xx.s

C_INCLUDES += -I$(CHIP_SRC_DIR)/Core/Inc
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/CMSIS/Device/ST/STM32F4xx/Include
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/CMSIS/Include

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
    -DPOV_RUN_MODE=POV_RUN_SLAVE

# Linker configuration
LDSCRIPT = $(CHIP_SRC_DIR)/STM32F407XX_FLASH.ld
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections -Wl,--no-enum-size-warning
