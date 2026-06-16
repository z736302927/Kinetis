
CHIP_SRC_DIR ?= E:\Code\STM32H743IITx
# WIFI_SDK_DIR ?= E:\Code\wifi-ap6181-h743-code\WiFi_SDK
# LWIP_CORE_DIR = $(WIFI_SDK_DIR)/WICED/network/LwIP/ver2.0.3/lwip/src/core
# LWIP_IPV4_DIR = $(WIFI_SDK_DIR)/WICED/network/LwIP/ver2.0.3/lwip/src/core/ipv4
# LWIP_API_DIR = $(WIFI_SDK_DIR)/WICED/network/LwIP/ver2.0.3/lwip/src/api

# Kinetis arch source + board + fatfs SD
C_SOURCES += $(wildcard ../arch/arm/mach-stm32/stm32h7.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/sd_diskio.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/flash_diskio.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/bsp_driver_sd.c)
C_SOURCES += $(wildcard ../drivers/kinetis/fatfs-intf.c)
C_SOURCES += $(wildcard ../drivers/kinetis/fsm.c)
C_SOURCES += $(wildcard ../drivers/kinetis/hc-05.c)
C_SOURCES += $(wildcard ../drivers/kinetis/led.c)
C_SOURCES += $(wildcard ../drivers/kinetis/mavlink.c)
C_SOURCES += $(wildcard ../drivers/kinetis/w25qxxx.c)
C_SOURCES += $(wildcard ../drivers/kinetis/mt6701.c)
C_SOURCES += $(wildcard ../drivers/kinetis/lrzsz/*.c)
C_SOURCES += $(wildcard ../drivers/kinetis/kalman-clib/src/matrix.c)
C_SOURCES += $(wildcard ../drivers/kinetis/kalman-clib/src/kalman.c)
C_SOURCES += $(wildcard ../drivers/kinetis/kalman-clib/src/cholesky.c)

# CubeMX core/peripheral sources
C_SOURCES += $(wildcard $(CHIP_SRC_DIR)/Core/Src/*.c)

# HAL driver sources (auto-detect via wildcard — no need to list each file)
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
C_INCLUDES += -I../drivers/kinetis/kalman-clib/include

# # Override include: our FreeRTOS/rtos stubs must be found FIRST
# C_INCLUDES += -I../drivers/kinetis/wifi

# C_INCLUDES += -I$(WIFI_SDK_DIR)/include
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/WWD/include
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/WWD/internal
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/RTOS/FreeRTOS/WICED
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/RTOS/FreeRTOS/WWD
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/platform/MCU/STM32H7xx
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/platform/MCU/STM32H7xx/GCC
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/network/LwIP
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/network/LwIP/WICED
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/network/LwIP/ver2.0.3/lwip/src/include
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/network/LwIP/ver2.0.3/lwip/src/include/lwip
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/network/LwIP/ver2.0.3/lwip/src/include/netif
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/network/LwIP/ver2.0.3/arch
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/network/LwIP/ver2.0.3/arch/include
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/internal
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/internal/bus_protocols/SDIO

# C_SOURCES += $(wildcard ../drivers/kinetis/wifi/*.c)

# # lwIP core source files (must be individually listed for ARM GCC)
# C_SOURCES += $(LWIP_CORE_DIR)/init.c
# C_SOURCES += $(LWIP_CORE_DIR)/def.c
# C_SOURCES += $(LWIP_CORE_DIR)/dns.c
# C_SOURCES += $(LWIP_CORE_DIR)/inet_chksum.c
# C_SOURCES += $(LWIP_CORE_DIR)/ip.c
# C_SOURCES += $(LWIP_CORE_DIR)/mem.c
# C_SOURCES += $(LWIP_CORE_DIR)/memp.c
# C_SOURCES += $(LWIP_CORE_DIR)/netif.c
# C_SOURCES += $(LWIP_CORE_DIR)/pbuf.c
# C_SOURCES += $(LWIP_CORE_DIR)/raw.c
# C_SOURCES += $(LWIP_CORE_DIR)/stats.c
# C_SOURCES += $(LWIP_CORE_DIR)/sys.c
# C_SOURCES += $(LWIP_CORE_DIR)/tcp.c
# C_SOURCES += $(LWIP_CORE_DIR)/tcp_in.c
# C_SOURCES += $(LWIP_CORE_DIR)/tcp_out.c
# C_SOURCES += $(LWIP_CORE_DIR)/timeouts.c
# C_SOURCES += $(LWIP_CORE_DIR)/udp.c

# C_SOURCES += $(LWIP_IPV4_DIR)/autoip.c
# C_SOURCES += $(LWIP_IPV4_DIR)/dhcp.c
# C_SOURCES += $(LWIP_IPV4_DIR)/etharp.c
# C_SOURCES += $(LWIP_IPV4_DIR)/icmp.c
# C_SOURCES += $(LWIP_IPV4_DIR)/igmp.c
# C_SOURCES += $(LWIP_IPV4_DIR)/ip4.c
# C_SOURCES += $(LWIP_IPV4_DIR)/ip4_addr.c
# C_SOURCES += $(LWIP_IPV4_DIR)/ip4_frag.c

# C_SOURCES += $(LWIP_API_DIR)/api_lib.c
# C_SOURCES += $(LWIP_API_DIR)/api_msg.c
# C_SOURCES += $(LWIP_API_DIR)/err.c
# C_SOURCES += $(LWIP_API_DIR)/netbuf.c
# C_SOURCES += $(LWIP_API_DIR)/netdb.c
# C_SOURCES += $(LWIP_API_DIR)/netifapi.c
# C_SOURCES += $(LWIP_API_DIR)/sockets.c
# C_SOURCES += $(LWIP_API_DIR)/tcpip.c

# C_SOURCES += $(WIFI_SDK_DIR)/WICED/network/LwIP/WICED/wiced_network.c

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
    -DPOV_RUN_MODE=POV_RUN_HOST \
	-DCMB_USER_CFG \
	-D_POSIX_C_SOURCE=200809L \
	-DCMB_CPU_PLATFORM_TYPE=CMB_CPU_ARM_CORTEX_M7 \
	-DWICED_DISABLE_WATCHDOG \
	-DWICED_DISABLE_STDIO \
	-DWICED_DISABLE_MCU_POWERSAVE \
	-DWPRINT_MACRO

# Linker configuration
LDSCRIPT = $(CHIP_SRC_DIR)/STM32H743XX_APP_FLASH.ld
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections -Wl,--no-enum-size-warning
