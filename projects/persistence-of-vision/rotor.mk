
CHIP_SRC_DIR ?= E:\Code\STM32H743IITx

# Kinetis arch source + board + fatfs SD
C_SOURCES += $(wildcard ../arch/arm/mach-stm32/stm32h7.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/sd_diskio.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/bsp_driver_sd.c)
C_SOURCES += $(wildcard ../drivers/pinctrl/pinctrl-stm32h743.c)

# CubeMX core/peripheral sources
C_SOURCES += $(wildcard $(CHIP_SRC_DIR)/Core/Src/*.c)

# HAL driver sources (auto-detect via wildcard — no need to list each file)
C_SOURCES += $(wildcard $(CHIP_SRC_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/*.c)

# Startup assembly
S_SOURCES += $(CHIP_SRC_DIR)/startup_stm32h743xx.s

C_INCLUDES += -I$(CHIP_SRC_DIR)/Core/Inc
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/STM32H7xx_HAL_Driver/Inc
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/CMSIS/Device/ST/STM32H7xx/Include
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/CMSIS/Include

HEADER_DIRS += ../arch/arm/mach-stm32

HEADERS += $(wildcard ../arch/arm/mach-stm32/*.h)

#--- MCU architecture flags (used by Makefile's CHIP toolchain section) ---
PREFIX = arm-none-eabi-
# CHIP architecture flags (from chip.mk)
CPU = -mcpu=cortex-m7
FPU = -mfpu=fpv5-d16
FLOAT-ABI = -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)
CHIP_ARCH_CFLAGS = $(MCU) -fno-short-enums

# Final target
TARGET = $(BUILD_DIR)/$(PROJECT)

# Macros for gcc
C_DEFS = \
	-D__KERNEL__ \
	-DUSE_HAL_DRIVER \
    -DSTM32H743xx \
    -DUSE_PWR_LDO_SUPPLY \
    -DPOV_RUN_MODE=POV_RUN_HOST

# Linker configuration
LDSCRIPT = $(CHIP_SRC_DIR)/STM32H743XX_FLASH.ld
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections -Wl,--no-enum-size-warning
