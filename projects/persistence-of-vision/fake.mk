
# fake platform arch source + board + fatfs SD
C_SOURCES += $(wildcard ../arch/arm/fake-mcu/*.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/fake_ram_diskio.c)
C_SOURCES += $(wildcard ../drivers/kinetis/fatfs-intf.c)
C_SOURCES += $(wildcard ../drivers/kinetis/hc-05.c)
C_SOURCES += $(wildcard ../drivers/kinetis/led.c)
C_SOURCES += $(wildcard ../drivers/kinetis/tlc5971.c)
C_SOURCES += $(wildcard ../drivers/kinetis/mavlink.c)
# C_SOURCES += $(wildcard ../drivers/kinetis/w25qxxx.c)
C_SOURCES += $(wildcard ../drivers/kinetis/nrf24l01/src/driver_nrf24l01.c)
C_SOURCES += $(wildcard ../drivers/kinetis/nrf24l01_port.c)
C_SOURCES += $(wildcard ../drivers/kinetis/nrf24l01/interface/driver_nrf24l01_interface_template.c)
C_SOURCES += $(wildcard ../drivers/kinetis/w25qxx/src/driver_w25qxx.c)
C_SOURCES += $(wildcard ../drivers/kinetis/w25qxx_port.c)
C_SOURCES += $(wildcard ../drivers/kinetis/w25qxx/interface/driver_w25qxx_interface_template.c)
C_SOURCES += $(wildcard ../drivers/kinetis/lrzsz/*.c)
C_SOURCES += $(wildcard ../drivers/kinetis/kalman-clib/src/matrix.c)
C_SOURCES += $(wildcard ../drivers/kinetis/kalman-clib/src/kalman.c)
C_SOURCES += $(wildcard ../drivers/kinetis/kalman-clib/src/cholesky.c)

C_INCLUDES += -I../drivers/kinetis/kalman-clib/include
C_INCLUDES += -I../drivers/kinetis/nrf24l01/src
C_INCLUDES += -I../drivers/kinetis/nrf24l01/interface
C_INCLUDES += -I../drivers/kinetis/w25qxx/src
C_INCLUDES += -I../drivers/kinetis/w25qxx/interface

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