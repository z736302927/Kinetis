
#--- MCU architecture flags (used by Makefile's CHIP toolchain section) ---
CHIP_CPU       := -mcpu=cortex-m7
CHIP_FPU       := -mfpu=fpv5-d16
CHIP_FLOAT_ABI := -mfloat-abi=hard

#--- Linker script basename (searched under CHIP_SRC_DIR) ---
CHIP_LDSCRIPT  := $(CHIP_SRC_DIR)/STM32H743XX_FLASH.ld

#--- Compiler defines (appended to Makefile's C_DEFS via $(CHIP_DEFS)) ---
CHIP_DEFS      := -DUSE_HAL_DRIVER -DSTM32H743xx -DUSE_PWR_LDO_SUPPLY

# fake platform arch source + board + fatfs SD
C_SOURCES += $(wildcard ../arch/arm/fake-mcu/*.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/fake_ram_diskio.c)

# Final target
TARGET = $(BUILD_DIR)/$(PROJECT).exe

# Macros for gcc
C_DEFS =  \
    -D__KERNEL__ \
    -DCONFIG_FAKE_LIB \
    -DCONFIG_SIMULATION \
    -DKINETIS_FAKE_SIM=1 \
    -DUSE_HAL_DRIVER \
    -DSTM32F407xx

# Linker configuration
LIBS = -lm
LIBDIR =
LDFLAGS = -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections