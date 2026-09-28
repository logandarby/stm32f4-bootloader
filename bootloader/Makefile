# Be silent by default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q       := @
NULL    := 2>/dev/null
endif

SRC_DIR        = src
OBJ_DIR        = obj
COMMON_DIR     = ../common
DEFAULT_FW_DIR = ../default_firmware
OPENCM3_DIR    = ../libopencm3

BINARY         = bootloader
DEFAULT_FW_BINARY = $(DEFAULT_FW_DIR)/default_firmware.bin

###############################################################################
# Basic Device Setup

LIBNAME        = opencm3_stm32f4
DEFS           += -DSTM32F4
FP_FLAGS       ?= -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS     = -mthumb -mcpu=cortex-m4 $(FP_FLAGS)

###############################################################################
# Linkerscript

LDSCRIPT       = linkerscript.ld
LDLIBS         += -lcommon
LDLIBS         += -l$(LIBNAME)
LDFLAGS        += -L$(COMMON_DIR)
LDFLAGS        += -L$(OPENCM3_DIR)/lib

###############################################################################
# Includes

DEFS           += -I$(OPENCM3_DIR)/include
DEFS           += -I$(SRC_DIR)
DEFS           += -I$(COMMON_DIR)/src

###############################################################################
# Executables

PREFIX         ?= arm-none-eabi-

CC             := $(PREFIX)gcc
CXX            := $(PREFIX)g++
LD             := $(PREFIX)gcc
AR             := $(PREFIX)ar
AS             := $(PREFIX)as
OBJCOPY        := $(PREFIX)objcopy
OBJDUMP        := $(PREFIX)objdump
GDB            := $(PREFIX)gdb
OPT            := -O0
DEBUG          := -ggdb3
CSTD           ?= -std=c99

###############################################################################
# Source files & Object generation

C_SRCS         := $(shell find $(SRC_DIR) -name '*.c')
CXX_SRCS       := $(shell find $(SRC_DIR) -name '*.cpp' -o -name '*.cxx')
ASM_SRCS       := $(shell find $(SRC_DIR) -name '*.S')

OBJS           := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_SRCS))
OBJS           += $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(filter %.cpp, $(CXX_SRCS)))
OBJS           += $(patsubst $(SRC_DIR)/%.cxx, $(OBJ_DIR)/%.o, $(filter %.cxx, $(CXX_SRCS)))
OBJS           += $(patsubst $(SRC_DIR)/%.S, $(OBJ_DIR)/%.o, $(ASM_SRCS))

DEPS           := $(OBJS:.o=.d)

###############################################################################
# C flags

TGT_CFLAGS     += $(OPT) $(CSTD) $(DEBUG)
TGT_CFLAGS     += $(ARCH_FLAGS)
TGT_CFLAGS     += -Wextra -Wshadow -Wimplicit-function-declaration
TGT_CFLAGS     += -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
TGT_CFLAGS     += -fno-common -ffunction-sections -fdata-sections

###############################################################################
# Preprocessor common flags

TGT_CPPFLAGS   += -MD
TGT_CPPFLAGS   += -Wall -Wundef
TGT_CPPFLAGS   += $(DEFS)

###############################################################################
# Linker flags

TGT_LDFLAGS    += --static -nostartfiles
TGT_LDFLAGS    += -T$(LDSCRIPT)
TGT_LDFLAGS    += $(ARCH_FLAGS) $(DEBUG)
TGT_LDFLAGS    += -Wl,-Map=$(BINARY).map -Wl,--cref
TGT_LDFLAGS    += -Wl,--gc-sections

###############################################################################
# Libraries

LDLIBS         += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

###############################################################################

.SUFFIXES: .elf .bin .hex .srec .list .map
.SECONDARY:

all: elf bin

elf: $(BINARY).elf
bin: $(BINARY).bin
hex: $(BINARY).hex
srec: $(BINARY).srec
list: $(BINARY).list
GENERATED_BINARIES = $(BINARY).elf $(BINARY).bin $(BINARY).hex $(BINARY).srec $(BINARY).list $(BINARY).map

%.bin: %.elf
	$(Q)$(OBJCOPY) -Obinary $< $@
# 	$(Q)python3 ../scripts/pad-bootloader.py

%.hex: %.elf
	$(Q)$(OBJCOPY) -Oihex $< $@

%.srec: %.elf
	$(Q)$(OBJCOPY) -Osrec $< $@

%.list: %.elf
	$(Q)$(OBJDUMP) -S $< > $@

# Ensure libopencm3 is built
$(OPENCM3_DIR)/lib/lib$(LIBNAME).a:
	$(warning $(LIBNAME).a not found, attempting to rebuild in $(OPENCM3_DIR))
	$(MAKE) -C $(OPENCM3_DIR)

# Ensure common directory library is built
$(COMMON_DIR)/libcommon.a:
	$(warning libcommon.a not found, attempting to rebuild in $(COMMON_DIR))
	$(MAKE) -C $(COMMON_DIR)

# Ensure default firmware is built
$(DEFAULT_FW_BINARY):
	$(warning default_firmware.bin not found, attempting to rebuild in $(DEFAULT_FW_DIR))
	$(MAKE) -C $(DEFAULT_FW_DIR)

# Link the final bootloader binary
%.elf %.map: $(OBJS) $(LDSCRIPT) $(OPENCM3_DIR)/lib/lib$(LIBNAME).a $(COMMON_DIR)/libcommon.a Makefile
	$(Q)$(LD) $(TGT_LDFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $*.elf

# Compile C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(Q)$(CC) $(TGT_CFLAGS) $(CFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

# Compile Assembly sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S $(DEFAULT_FW_BINARY)
	@mkdir -p $(@D)
	$(Q)$(CC) $(TGT_CFLAGS) $(CFLAGS) -o $@ -c $<

clean:
	$(Q)$(RM) $(GENERATED_BINARIES)
	$(Q)$(RM) -r $(OBJ_DIR)

.PHONY: clean elf bin hex srec list

-include $(DEPS)