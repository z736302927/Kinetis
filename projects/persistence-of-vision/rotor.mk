CHIP_SRC_DIR ?= E:\Code\STM32H743IITx
WIFI_SDK_DIR ?= E:\Code\wifi-ap6181-h743-code\WiFi_SDK
LWIP_ROOT ?= $(WIFI_SDK_DIR)/WICED/network/LwIP/ver2.0.3

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
C_SOURCES += $(wildcard ../drivers/kinetis/tlc5971.c)
# C_SOURCES += $(wildcard ../drivers/kinetis/w25qxxx.c)
C_SOURCES += $(wildcard ../drivers/kinetis/mt6701.c)
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
C_SOURCES += $(wildcard $(CHIP_SRC_DIR)/Drivers/STM32H7xx_HAL_Driver/Src/*.c)

C_SOURCES += $(wildcard ../drivers/kinetis/CmBacktrace/cm_backtrace/*.c)

# # --- NoOS RTOS abstraction layer ---
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/RTOS/NoOS/WWD/wwd_rtos.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/RTOS/NoOS/WWD/Cortex_M7/noos.c
# # --- WWD Core ---
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_management.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_wifi.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_thread.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_sdpcm.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_wifi_sleep.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_internal.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_ap_common.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_eapol.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_clm.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_logging.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_debug.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_rtos_interface.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/wwd_thread_internal.c
# # --- STM32H7xx Platform ---
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/platform/MCU/STM32H7xx/WWD/wwd_SDIO.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/platform/MCU/STM32H7xx/WWD/wwd_platform.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/platform/MCU/STM32H7xx/platform_init.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/platform/MCU/wwd_resources.c
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/platform/MCU/wwd_platform_separate_mcu.c
# # --- Bus protocol ---
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/bus_protocols/wwd_bus_common.c
# # --- SDIO bus protocol ---
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/WWD/internal/bus_protocols/SDIO/wwd_bus_protocol.c
# # --- WWD LwIP Buffer ---
# C_SOURCES += $(WIFI_SDK_DIR)/WICED/network/LwIP/WWD/wwd_buffer.c
# # --- LwIP Core ---
# C_SOURCES += $(LWIP_ROOT)/src/core/init.c
# C_SOURCES += $(LWIP_ROOT)/src/core/def.c
# C_SOURCES += $(LWIP_ROOT)/src/core/dns.c
# C_SOURCES += $(LWIP_ROOT)/src/core/inet_chksum.c
# C_SOURCES += $(LWIP_ROOT)/src/core/ip.c
# C_SOURCES += $(LWIP_ROOT)/src/core/mem.c
# C_SOURCES += $(LWIP_ROOT)/src/core/memp.c
# C_SOURCES += $(LWIP_ROOT)/src/core/netif.c
# C_SOURCES += $(LWIP_ROOT)/src/core/pbuf.c
# C_SOURCES += $(LWIP_ROOT)/src/core/raw.c
# C_SOURCES += $(LWIP_ROOT)/src/core/stats.c
# C_SOURCES += $(LWIP_ROOT)/src/core/sys.c
# C_SOURCES += $(LWIP_ROOT)/src/core/tcp.c
# C_SOURCES += $(LWIP_ROOT)/src/core/tcp_in.c
# C_SOURCES += $(LWIP_ROOT)/src/core/tcp_out.c
# C_SOURCES += $(LWIP_ROOT)/src/core/timeouts.c
# C_SOURCES += $(LWIP_ROOT)/src/core/udp.c
# C_SOURCES += $(LWIP_ROOT)/src/core/ipv4/autoip.c
# C_SOURCES += $(LWIP_ROOT)/src/core/ipv4/dhcp.c
# C_SOURCES += $(LWIP_ROOT)/src/core/ipv4/etharp.c
# C_SOURCES += $(LWIP_ROOT)/src/core/ipv4/icmp.c
# C_SOURCES += $(LWIP_ROOT)/src/core/ipv4/igmp.c
# C_SOURCES += $(LWIP_ROOT)/src/core/ipv4/ip4.c
# C_SOURCES += $(LWIP_ROOT)/src/core/ipv4/ip4_addr.c
# C_SOURCES += $(LWIP_ROOT)/src/core/ipv4/ip4_frag.c
# C_SOURCES += $(LWIP_ROOT)/src/netif/ethernet.c

# C_SOURCES += $(wildcard ../projects/persistence-of-vision/wifi/*.c)

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
C_INCLUDES += -I../drivers/kinetis/nrf24l01/src
C_INCLUDES += -I../drivers/kinetis/nrf24l01/interface
C_INCLUDES += -I../drivers/kinetis/w25qxx/src
C_INCLUDES += -I../drivers/kinetis/w25qxx/interface

# # WICED / WWD includes
# C_INCLUDES += -I$(WIFI_SDK_DIR)/include
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/WWD/include
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/WWD/include/network
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/WWD/include/platform
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/WWD/include/RTOS
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/WWD/internal
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/WWD/internal/bus_protocols
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/WWD/internal/bus_protocols/SDIO
# # NoOS RTOS includes
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/RTOS/NoOS/WWD
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/RTOS/NoOS/WWD/Cortex_M7
# # STM32H7xx platform includes
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/platform/MCU/STM32H7xx
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/platform/MCU/STM32H7xx/GCC
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/platform/MCU/STM32H7xx/peripherals
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/platform/MCU/STM32H7xx/peripherals/hal
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/platform/MCU
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/platform/include
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/platform/GCC
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/network/LwIP
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/network/LwIP/WICED
# C_INCLUDES += -I$(WIFI_SDK_DIR)/WICED/network/LwIP/WWD

# C_INCLUDES += -I$(LWIP_ROOT)/src/include
# C_INCLUDES += -I$(LWIP_ROOT)/src/include/lwip
# C_INCLUDES += -I$(LWIP_ROOT)/src/include/netif

# C_INCLUDES += -I../projects/persistence-of-vision/wifi
# C_INCLUDES += -I../projects/persistence-of-vision/wifi/arch

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
