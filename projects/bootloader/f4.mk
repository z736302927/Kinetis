
CHIP_SRC_DIR ?= E:\Code\STM32F407ZGT6

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
C_SOURCES += $(wildcard $(CHIP_SRC_DIR)/Drivers/STM32F4xx_HAL_Driver/Src/*.c)

C_SOURCES += $(wildcard ../drivers/kinetis/CmBacktrace/cm_backtrace/*.c)

# Startup assembly
S_SOURCES += $(CHIP_SRC_DIR)/startup_stm32f407xx.s
S_SOURCES += ../drivers/kinetis/CmBacktrace/cm_backtrace/fault_handler/gcc/cmb_fault.s

C_INCLUDES += -I$(CHIP_SRC_DIR)/Core/Inc
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/CMSIS/Device/ST/STM32F4xx/Include
C_INCLUDES += -I$(CHIP_SRC_DIR)/Drivers/CMSIS/Include

C_INCLUDES += -I../drivers/kinetis/CmBacktrace/cm_backtrace

HEADER_DIRS += ../arch/arm/mach-stm32

HEADERS += $(wildcard ../arch/arm/mach-stm32/*.h)

#--- MCU architecture flags ---
PREFIX = arm-none-eabi-
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
	-DBOOTLOADER_CHIP=BOOTLOADER_CHIP_F4 \
	-DCMB_USER_CFG \
	-D_POSIX_C_SOURCE=200809L \
	-DCMB_CPU_PLATFORM_TYPE=CMB_CPU_ARM_CORTEX_M4

# Linker configuration
LDSCRIPT = $(CHIP_SRC_DIR)/STM32F407XX_FLASH.ld
LIBS = -lc -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) \
	-Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections \
	-Wl,--no-enum-size-warning
