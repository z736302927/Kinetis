
CHIP_SRC_DIR ?= E:\Code\STM32H743IITx

# Kinetis arch source + board + fatfs SD
C_SOURCES += $(wildcard ../arch/arm/mach-stm32/stm32h7.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/sd_diskio.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/bsp_driver_sd.c)
C_SOURCES += $(wildcard ../drivers/kinetis/bootloader.c)
C_SOURCES += $(wildcard ../drivers/kinetis/fatfs-intf.c)
C_SOURCES += $(wildcard ../drivers/kinetis/fsm.c)
C_SOURCES += $(wildcard ../drivers/kinetis/hc-05.c)
C_SOURCES += $(wildcard ../drivers/kinetis/led.c)
C_SOURCES += $(wildcard ../drivers/kinetis/mavlink.c)
C_SOURCES += $(wildcard ../drivers/kinetis/w25qxxx.c)
C_SOURCES += $(wildcard ../drivers/kinetis/lrzsz/*.c)

# CubeMX core/peripheral sources
C_SOURCES += $(wildcard $(CHIP_SRC_DIR)/Core/Src/*.c)

# HAL driver sources
C_SOURCES += $(wildcard $(CHIP_SRC_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/*.c)

C_SOURCES += $(wildcard ../drivers/kinetis/CmBacktrace/cm_backtrace/*.c)

# Startup assembly
S_SOURCES += $(CHIP_SRC_DIR)/startup_stm32h743xx.s
S_SOURCES += ../drivers/kinetis/CmBacktrace/cm_backtrace/fault_handler/gcc/cmb_fault.s

C_INCLUDES += -I$(CHIP_SRC_DIR)/Core/Inc
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/STM32H7xx_HAL_Driver/Inc
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/CMSIS/Device/ST/STM32H7xx/Include
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/CMSIS/Include

C_INCLUDES += -I../drivers/kinetis/CmBacktrace/cm_backtrace

HEADER_DIRS += ../arch/arm/mach-stm32
HEADERS += $(wildcard ../arch/arm/mach-stm32/*.h)

#--- MCU architecture flags ---
PREFIX = arm-none-eabi-
CPU = -mcpu=cortex-m7
FPU = -mfpu=fpv5-d16
FLOAT-ABI = -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)
CHIP_ARCH_CFLAGS = $(MCU) -fno-short-enums

# Linker script:
# NOTE: For production bootloader, use a custom linker script that limits
# FLASH to the first 64KB (0x08000000, LENGTH=64K) to ensure the bootloader
# fits within its reserved region and firmware starts at the correct offset.
LDSCRIPT = $(CHIP_SRC_DIR)/STM32H743XX_FLASH.ld

# Final target
TARGET = $(BUILD_DIR)/$(PROJECT)

# Macros for gcc
C_DEFS = \
	-D__KERNEL__ \
	-DUSE_HAL_DRIVER \
	-DSTM32H743xx \
	-DUSE_PWR_LDO_SUPPLY \
	-DBOOTLOADER_CHIP=BOOTLOADER_CHIP_H7 \
	-DCMB_USER_CFG \
	-D_POSIX_C_SOURCE=200809L \
	-DCMB_CPU_PLATFORM_TYPE=CMB_CPU_ARM_CORTEX_M7

# Linker configuration
LIBS = -lc -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) \
	-Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections \
	-Wl,--no-enum-size-warning
