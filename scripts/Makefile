PROJECT = Four-Axis-Flight

BUILD_DIR = output

# debug build?
DEBUG = 0
# optimization
OPT = -O1

C_SOURCES += $(wildcard ../drivers/kinetis/*.c)
C_SOURCES += $(wildcard ../arch/arm/mach-stm32/main.c)

C_INCLUDES += -I../include/
C_INCLUDES += -I../include/uapi/
C_INCLUDES += -I../arch/arm/include/
C_INCLUDES += -I../arch/arm/include/uapi/
C_INCLUDES += -I../arch/arm/include/generated/
C_INCLUDES += -I../arch/arm/include/generated/uapi/
C_INCLUDES += -I../arch/arm/mach-stm32

AS_INCLUDES += 

ASM_SOURCES += startup_stm32f407xx.s

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

CC_PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(CC_PREFIX)gcc
AS = $(GCC_PATH)/$(CC_PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(CC_PREFIX)objcopy
SZ = $(GCC_PATH)/$(CC_PREFIX)size
else
CC = gcc
AS = $(CC_PREFIX)gcc -x assembler-with-cpp
CP = $(CC_PREFIX)objcopy
SZ = $(CC_PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
AS_DEFS = 
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F407xx \
-D__KERNEL__

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
# -nostdinc
CFLAGS = $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
#CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# link script
LDSCRIPT = STM32F407VGTx_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs #-T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections

# # default action: build all
# all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).hex $(BUILD_DIR)/$(PROJECT).bin

# # list of objects
# OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
# vpath %.c $(sort $(dir $(C_SOURCES)))
# # list of ASM program objects
# OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
# vpath %.s $(sort $(dir $(ASM_SOURCES)))

# $(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
# 	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

# $(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
# 	$(AS) -c $(CFLAGS) $< -o $@

# $(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS) Makefile
# 	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
# 	$(SZ) $@

# $(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
# 	$(HEX) $< $@

# $(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
# 	$(BIN) $< $@

.PHONY: all clean

# default action: build all
all: $(BUILD_DIR)/$(PROJECT)

# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR) 
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(PROJECT): $(OBJECTS)
	$(CC) -o $@ $^

$(BUILD_DIR):
	mkdir $@		

clean:
	-del /q $(BUILD_DIR)

#######################################--specs=nosys.specs
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***
