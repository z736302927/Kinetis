
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

# Linker configuration
LIBS = -lm
LIBDIR =
LDFLAGS = -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections