PROJECT = Four-Axis-Flight
BUILD_DIR = output

# Detect OS
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(shell uname -s)
endif

# Build configuration
DEBUG = 0
# OPT = -O0 -g3    # 无优化，完整调试信息 - 用于开发调试
# OPT = -O1 -g3    # 基本优化，完整调试信息 - 平衡性能与调试
# OPT = -O2 -g3    # 标准优化，完整调试信息 - 发布前的测试
# OPT = -O3 -g0    # 最大优化，无调试信息 - 最终发布版本
# OPT = -Os -g1    # 尺寸优化，基本调试信息 - 嵌入式系统
# Optimization level based on DEBUG
ifeq ($(DEBUG), 1)
OPT = -O0 -g3
else
OPT = -O0 -g3
endif

# Compiler flags -Wall
COMMON_FLAGS = --std=gnu17 -xc -fdata-sections -ffunction-sections -fdiagnostics-color=always
# COMMON_FLAGS += -fsanitize=address -fsanitize=undefined
WARN_FLAGS = -w -Wextra -Wno-unused-parameter
CFLAGS_BASE = $(COMMON_FLAGS) $(WARN_FLAGS) $(OPT)

# Source files
C_SOURCES += $(wildcard ../arch/arm/fake-mcu/*.c)
C_SOURCES += $(wildcard ../arch/arm/lib/*.c)
C_SOURCES += $(wildcard ../arch/arm/mm/*.c)
C_SOURCES += $(wildcard ../kernel/irq/*.c)
C_SOURCES += $(wildcard ../kernel/locking/*.c)
C_SOURCES += $(wildcard ../kernel/printk/*.c)
C_SOURCES += $(wildcard ../kernel/sched/*.c)
C_SOURCES += $(wildcard ../kernel/time/*.c)
C_SOURCES += $(wildcard ../kernel/*.c)
C_SOURCES += $(wildcard ../drivers/base/*.c)
C_SOURCES += $(wildcard ../drivers/char/random.c)
C_SOURCES += $(wildcard ../drivers/pinctrl/*.c)
C_SOURCES += $(wildcard ../drivers/gpio/gpiolib-devres.c)
C_SOURCES += $(wildcard ../drivers/gpio/gpiolib-legacy.c)
C_SOURCES += $(wildcard ../drivers/gpio/gpiolib.c)
C_SOURCES += $(wildcard ../drivers/tty/serdev/*.c)
C_SOURCES += $(wildcard ../drivers/tty/serial/*.c)
C_SOURCES += $(wildcard ../drivers/tty/*.c)
C_SOURCES += $(wildcard ../lib/math/div64.c)
#C_SOURCES += $(wildcard ../lib/lzo/*.c)
C_SOURCES += $(wildcard ../lib/shall/*.c)
C_SOURCES += $(wildcard ../lib/random32.c)
C_SOURCES += $(wildcard ../mm/*.c)
C_SOURCES += $(wildcard ../fs/*.c)
C_SOURCES += $(wildcard ../fs/fatfs/*.c)
C_SOURCES += $(wildcard ../fs/fatfs/drivers/fake_ram_diskio.c)
C_SOURCES += $(wildcard ../drivers/kinetis/*.c)
#C_SOURCES += $(wildcard ../drivers/kinetis/flight/*.c)
# Add other necessary source files...

# Exclude unnecessary files
EXCLUDE_SOURCES += ../drivers/pinctrl/devicetree.c
EXCLUDE_SOURCES += ../drivers/tty/tty_audit.c
EXCLUDE_SOURCES += $(wildcard ../lib/shall/crc-t10dif.c)
EXCLUDE_SOURCES += $(wildcard ../lib/shall/crc32test.c)
EXCLUDE_SOURCES += $(wildcard ../lib/shall/crc64.c)
EXCLUDE_SOURCES += $(wildcard ../lib/shall/dump_stack.c)
EXCLUDE_SOURCES += $(wildcard ../lib/shall/libcrc32c.c)
EXCLUDE_SOURCES += $(wildcard ../lib/shall/list-test.c)
EXCLUDE_SOURCES += $(wildcard ../lib/shall/rbtree_test.c)
# EXCLUDE_SOURCES += ../lib/shall/checksum.c
EXCLUDE_SOURCES += ../arch/arm/lib/csumpartial.c
EXCLUDE_SOURCES += ../mm/dmapool.c
# EXCLUDE_SOURCES += $(wildcard ../lib/shall/iov_iter.c)
# EXCLUDE_SOURCES += $(wildcard ../lib/shall/klist.c)
# EXCLUDE_SOURCES += $(wildcard ../lib/shall/refcount.c)
# EXCLUDE_SOURCES += $(wildcard ../lib/shall/strncpy_from_user.c)
# EXCLUDE_SOURCES += $(wildcard ../lib/shall/strnlen_user.c)
# EXCLUDE_SOURCES += $(wildcard ../lib/shall/vsprintf.c)
EXCLUDE_SOURCES += $(wildcard ../drivers/kinetis/flight/filter.c)
EXCLUDE_SOURCES += $(wildcard ../drivers/kinetis/flight/fmu_task.c)

C_SOURCES := $(filter-out $(EXCLUDE_SOURCES),$(C_SOURCES))

# Include directories
C_INCLUDES += -I../include/
C_INCLUDES += -I../include/uapi/
C_INCLUDES += -I../arch/arm/include/
C_INCLUDES += -I../arch/arm/include/uapi/
C_INCLUDES += -I../arch/arm/include/generated/
C_INCLUDES += -I../arch/arm/include/generated/uapi/
C_INCLUDES += -I../arch/arm/mach-stm32
C_INCLUDES += -include generated/deconfig.h
C_INCLUDES += -include kinetis/step-debug.h
# Add other necessary include directories...

# Define header directories for dependency tracking
HEADER_DIRS += ../include
HEADER_DIRS += ../include/uapi
HEADER_DIRS += ../arch/arm/include
HEADER_DIRS += ../arch/arm/include/uapi
HEADER_DIRS += ../arch/arm/include/generated
HEADER_DIRS += ../arch/arm/include/generated/uapi
HEADER_DIRS += ../arch/arm/mach-stm32

# Find all header files in include directories and subdirectories
# Manually list common directories and use wildcard for each level
HEADERS += $(wildcard ../include/*.h)
HEADERS += $(wildcard ../include/*/*.h)
HEADERS += $(wildcard ../include/*/*/*.h)
HEADERS += $(wildcard ../include/uapi/*.h)
HEADERS += $(wildcard ../include/uapi/*/*.h)
HEADERS += $(wildcard ../arch/arm/include/*.h)
HEADERS += $(wildcard ../arch/arm/include/*/*.h)
HEADERS += $(wildcard ../arch/arm/include/*/*/*.h)
HEADERS += $(wildcard ../arch/arm/include/uapi/*.h)
HEADERS += $(wildcard ../arch/arm/include/uapi/*/*.h)
HEADERS += $(wildcard ../arch/arm/include/generated/*.h)
HEADERS += $(wildcard ../arch/arm/include/generated/uapi/*.h)
HEADERS += $(wildcard ../arch/arm/mach-stm32/*.h)

# Toolchain - use local compiler
ifeq ($(DETECTED_OS),Windows)
    # Use gcc or cl on Windows
    ifdef GCC_PATH
    CC = $(GCC_PATH)/gcc
    else
    CC = gcc
    endif
else
    # Use gcc on Linux
    CC = gcc
endif

AS = $(CC)
CP = echo  # no objcopy needed
SZ = echo  # no size needed

# Final target is .exe file
TARGET = $(BUILD_DIR)/$(PROJECT).exe

# Macros for gcc
C_DEFS =  \
-DFOUR_AXIS_FLIGHT \
-DFOUR_AXIS_FLIGHT_VERSION=\"1.0.0\" \
-D__KERNEL__ \
-DCONFIG_FAKE_LIB \
-DCONFIG_SIMULATION \
-DUSE_HAL_DRIVER \
-DSTM32F407xx

# Dependency flags for automatic header dependency tracking
DEPFLAGS = -MMD -MP

# Compiler flags
CFLAGS = $(C_DEFS) $(C_INCLUDES) $(CFLAGS_BASE) -fdata-sections -ffunction-sections

# Linker configuration - standard linking
LIBS = -lm
LIBDIR = 
LDFLAGS = -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections

# Object files - simplified path structure
C_OBJS = $(addprefix $(BUILD_DIR)/obj/, $(notdir $(C_SOURCES:.c=.o)))
OBJS = $(C_OBJS)

# Create directory mapping for each source file
vpath %.c $(sort $(dir $(C_SOURCES)))

# OS-specific commands
ifeq ($(DETECTED_OS),Windows)
    # Windows mkdir command
    MKDIR = cmd /c "if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))"
    RMDIR = if exist $(subst /,\,$(1)) rmdir /s /q $(subst /,\,$(1))
    ECHO = echo
    # Path handling for Windows
    FIXPATH = $(subst /,\,$(1))
    # Run command for Windows
    RUN_CMD = $(call FIXPATH, $(TARGET))
else
    MKDIR = mkdir -p $(1)
    RMDIR = rm -rf $(1)
    ECHO = echo
    FIXPATH = $(1)
    # Run command for Linux
    RUN_CMD = $(TARGET)
endif

# Default target - build .exe
all: $(TARGET)
	@$(ECHO) "Build completed: $(TARGET)"
	@$(ECHO) "Auto-running the program..."
	@$(MAKE) run

# Create necessary directories
$(BUILD_DIR):
	@$(ECHO) "Creating directory: $(BUILD_DIR)"
	$(call MKDIR, $(BUILD_DIR))

$(BUILD_DIR)/obj:
	@$(ECHO) "Creating directory: $(BUILD_DIR)/obj"
	$(call MKDIR, $(BUILD_DIR)/obj)

# Create object files from C sources with dependency tracking
$(BUILD_DIR)/obj/%.o: %.c | $(BUILD_DIR)/obj
	@$(ECHO) "Compiling C: $< -> $@"
#	@$(ECHO) "Command: $(CC) -c $(CFLAGS) $(DEPFLAGS) -o $@ $<"
	@$(CC) -c $(CFLAGS) $(DEPFLAGS) -o $@ $<

# Link object files to create executable
$(TARGET): $(OBJS) | $(BUILD_DIR)
#	@$(ECHO) "Linking: $(words $(OBJS)) object files -> $@"
#	@$(ECHO) "Command: $(CC) $(OBJS) $(LDFLAGS) -o $@ $(LIBS)"
	@$(CC) $(OBJS) $(LDFLAGS) -o $@ $(LIBS)

# Force rebuild if headers change
# $(OBJS): $(HEADERS)  # 注释掉这行，让GCC的自动依赖机制来处理真正的依赖关系

# Clean target
clean:
	@$(ECHO) "Cleaning build directory: $(BUILD_DIR)"
	$(call RMDIR, $(BUILD_DIR))

# Run target
run: $(TARGET)
	@$(RUN_CMD)
#	@$(ECHO) "Running $(TARGET) and saving output to $(BUILD_DIR)/$(PROJECT).log..."
#	-@$(RUN_CMD) > $(BUILD_DIR)/$(PROJECT).log 2>&1 || $(ECHO) "Program finished with exit code: $$?"
#	@$(ECHO) "Output saved to $(BUILD_DIR)/$(PROJECT).log"

# Debug target - show detailed information
debug:
	@$(ECHO) "=== Debug Information ==="
	@$(ECHO) "OS: $(DETECTED_OS)"
	@$(ECHO) "CC: $(CC)"
	@$(ECHO) "C_SOURCES: $(C_SOURCES)"
	@$(ECHO) "OBJS: $(OBJS)"
	@$(ECHO) "BUILD_DIR: $(BUILD_DIR)"
	@$(ECHO) "Current directory: $(shell pwd)"

# Print sources (for debugging)
print-sources:
	@$(ECHO) "C Sources:"
ifeq ($(DETECTED_OS),Windows)
	@for %%i in ($(C_SOURCES)) do @$(ECHO)   %%i
else
	@for src in $(C_SOURCES); do $(ECHO) "  $$src"; done
endif

# Print object files (for debugging)
print-objects:
	@$(ECHO) "Object files:"
ifeq ($(DETECTED_OS),Windows)
	@for %%i in ($(OBJS)) do @$(ECHO)   %%i
else
	@for obj in $(OBJS); do $(ECHO) "  $$obj"; done
endif

# Print OS info (for debugging)
print-os:
	@$(ECHO) "Detected OS: $(DETECTED_OS)"

# Print variables (for debugging)
print-vars:
	@$(ECHO) "CC: $(CC)"
	@$(ECHO) "CFLAGS: $(CFLAGS)"
	@$(ECHO) "LDFLAGS: $(LDFLAGS)"
	@$(ECHO) "C_SOURCES count: $(words $(C_SOURCES))"
	@$(ECHO) "OBJS count: $(words $(OBJS))"
	@$(ECHO) "HEADERS count: $(words $(HEADERS))"

# Print headers (for debugging)
print-headers:
	@$(ECHO) "Header files:"
ifeq ($(DETECTED_OS),Windows)
	@for %%i in ($(HEADERS)) do @$(ECHO)   %%i
else
	@for hdr in $(HEADERS); do $(ECHO) "  $$hdr"; done
endif

# Check dependency files
check-deps:
	@$(ECHO) "Checking dependency files..."
ifeq ($(DETECTED_OS),Windows)
	@if exist "$(BUILD_DIR)\obj\*.d" (
		@for %%f in ($(BUILD_DIR)\obj\*.d) do @$(ECHO) Found: %%f
	) else (
		@$(ECHO) No dependency files found yet. Run 'make' first.
	)
else
	@if [ -n "$(wildcard $(BUILD_DIR)/obj/*.d)" ]; then \
		for dep in $(BUILD_DIR)/obj/*.d; do \
			$(ECHO) "Found: $$dep"; \
		done; \
	else \
		$(ECHO) "No dependency files found yet. Run 'make' first."; \
	fi
endif

# Check if source files exist
check-sources:
	@$(ECHO) "Checking source files..."
ifeq ($(DETECTED_OS),Windows)
	@set /a missing=0
	@for %%f in ($(C_SOURCES)) do @if not exist %%f ( \
		@$(ECHO) Missing: %%f & \
		@set /a missing=1 \
	)
	@if %missing% equ 1 ( \
		@$(ECHO) Error: Some source files are missing! & \
		@exit 1 \
	) else ( \
		@$(ECHO) All source files found. \
	)
else
	@missing=0; \
	for src in $(C_SOURCES); do \
		if [ ! -f "$$src" ]; then \
			$(ECHO) "Missing: $$src"; \
			missing=1; \
		fi; \
	done; \
	if [ $$missing -eq 1 ]; then \
		$(ECHO) "Error: Some source files are missing!"; \
		exit 1; \
	else \
		$(ECHO) "All source files found."; \
	fi
endif

# Test directory creation
test-dirs:
	@$(ECHO) "Testing directory creation..."
	$(call MKDIR, $(BUILD_DIR))
	$(call MKDIR, $(BUILD_DIR)/obj)
ifeq ($(DETECTED_OS),Windows)
	@if exist $(BUILD_DIR)/obj ( \
		@$(ECHO) Directory creation test: PASSED \
	) else ( \
		@$(ECHO) Directory creation test: FAILED \
	)
else
	@if [ -d "$(BUILD_DIR)/obj" ]; then \
		$(ECHO) "Directory creation test: PASSED"; \
	else \
		$(ECHO) "Directory creation test: FAILED"; \
	fi
endif

# Manual directory creation target
create-dirs:
	@$(ECHO) "Manually creating directories..."
	$(call MKDIR, $(BUILD_DIR))
	$(call MKDIR, $(BUILD_DIR)/obj)
	@$(ECHO) "Directories created: $(BUILD_DIR) and $(BUILD_DIR)/obj"

# Test command display
test-cmd:
	@$(ECHO) "Testing command display..."
	@$(ECHO) "CC: $(CC)"
	@$(ECHO) "CFLAGS: $(CFLAGS)"
	@$(ECHO) "Sample compile command: $(CC) -c $(CFLAGS) -o test.o test.c"

# Print compile and link commands
print-cmd:
	@$(ECHO) "=== Compile and Link Commands ==="
	@$(ECHO) ""
	@$(ECHO) "Compiler: $(CC)"
	@$(ECHO) ""
	@$(ECHO) "Compile command:"
	@$(ECHO) "  $(CC) -c $(CFLAGS) -o <object_file> <source_file>"
	@$(ECHO) ""
	@$(ECHO) "Link command:"
	@$(ECHO) "  $(CC) <object_files> $(LDFLAGS) -o $(TARGET) $(LIBS)"
	@$(ECHO) ""
	@$(ECHO) "Current CFLAGS:"
	@$(ECHO) "  $(CFLAGS)"
	@$(ECHO) ""
	@$(ECHO) "Current LDFLAGS:"
	@$(ECHO) "  $(LDFLAGS)"
	@$(ECHO) ""
	@$(ECHO) "Libraries:"
	@$(ECHO) "  $(LIBS)"

# Include dependency files
# Include auto-generated dependency files (.d files) for each object file
# The - before include prevents make from failing if the files don't exist yet
-include $(C_OBJS:.o=.d)

.PHONY: all clean run debug print-sources print-objects print-os print-vars check-sources test-dirs create-dirs test-cmd print-cmd print-headers check-deps