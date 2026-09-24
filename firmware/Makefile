# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q		:= @
NULL		:= 2>/dev/null
endif

SRC_DIR        = src
OBJ_DIR        = obj
COMMON_DIR     = ../common
BOOTLOADER_DIR = ../bootloader
OPENCM3_DIR    = ../libopencm3

BOOTLOADER_BIN = $(BOOTLOADER_DIR)/bootloader.bin
BINARY         = firmware

###############################################################################
# Basic Device Setup

LIBNAME         = opencm3_stm32f4
DEFS            += -DSTM32F4
FP_FLAGS        ?= -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS      = -mthumb -mcpu=cortex-m4 $(FP_FLAGS)

###############################################################################
# Linkerscript

LDSCRIPT        = linkerscript.ld
LDLIBS          += -l$(LIBNAME)
LDLIBS          += -lcommon
LDFLAGS         += -L$(OPENCM3_DIR)/lib
LDFLAGS					+= -L$(COMMON_DIR)

###############################################################################
# Includes

DEFS            += -I$(OPENCM3_DIR)/include
DEFS            += -I$(SRC_DIR)
DEFS						+= -I$(COMMON_DIR)/src

###############################################################################
# Executables

PREFIX          ?= arm-none-eabi-

CC              := $(PREFIX)gcc
CXX             := $(PREFIX)g++
LD              := $(PREFIX)gcc
AR              := $(PREFIX)ar
AS              := $(PREFIX)as
OBJCOPY         := $(PREFIX)objcopy
OBJDUMP         := $(PREFIX)objdump
GDB             := $(PREFIX)gdb
STFLASH         := $(shell which st-flash)
OPT             := -Os
DEBUG           := -ggdb3
CSTD            ?= -std=c99

###############################################################################
# Source files & Object generation

C_SRCS          := $(shell find $(SRC_DIR) -name '*.c')
CXX_SRCS        := $(shell find $(SRC_DIR) -name '*.cpp' -o -name '*.cxx')
ASM_SRCS        := $(shell find $(SRC_DIR) -name '*.S')

OBJS            := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_SRCS))
OBJS            += $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(filter %.cpp, $(CXX_SRCS)))
OBJS            += $(patsubst $(SRC_DIR)/%.cxx, $(OBJ_DIR)/%.o, $(filter %.cxx, $(CXX_SRCS)))
OBJS            += $(patsubst $(SRC_DIR)/%.S, $(OBJ_DIR)/%.o, $(ASM_SRCS))

DEPS            := $(OBJS:.o=.d)

###############################################################################
# C flags

TGT_CFLAGS      += $(OPT) $(CSTD) $(DEBUG)
TGT_CFLAGS      += $(ARCH_FLAGS)
TGT_CFLAGS      += -Wextra -Wshadow -Wimplicit-function-declaration
TGT_CFLAGS      += -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
TGT_CFLAGS      += -fno-common -ffunction-sections -fdata-sections

###############################################################################
# C++ flags

TGT_CXXFLAGS    += $(OPT) $(CXXSTD) $(DEBUG)
TGT_CXXFLAGS    += $(ARCH_FLAGS)
TGT_CXXFLAGS    += -Wextra -Wshadow -Wredundant-decls -Weffc++
TGT_CXXFLAGS    += -fno-common -ffunction-sections -fdata-sections

###############################################################################
# Preprocessor common flags

TGT_CPPFLAGS    += -MD
TGT_CPPFLAGS    += -Wall -Wundef
TGT_CPPFLAGS    += $(DEFS)

###############################################################################
# Linker flags

TGT_LDFLAGS     += --static -nostartfiles
TGT_LDFLAGS     += -T$(LDSCRIPT)
TGT_LDFLAGS     += $(ARCH_FLAGS) $(DEBUG)
TGT_LDFLAGS     += -Wl,-Map=$(BINARY).map -Wl,--cref
TGT_LDFLAGS     += -Wl,--gc-sections
ifeq ($(V),99)
TGT_LDFLAGS     += -Wl,--print-gc-sections
endif

###############################################################################
# Libraries

LDLIBS          += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

###############################################################################

.SUFFIXES: .elf .bin .hex .srec .list .map .images
.SECONDEXPANSION:
.SECONDARY:

all: elf bin

elf: $(BINARY).elf
bin: $(BINARY).bin
hex: $(BINARY).hex
srec: $(BINARY).srec
list: $(BINARY).list
GENERATED_BINARIES=$(BINARY).elf $(BINARY).bin $(BINARY).hex $(BINARY).srec $(BINARY).list $(BINARY).map

images: $(BINARY).images
flash: $(BINARY).flash

$(OPENCM3_DIR)/lib/lib$(LIBNAME).a:
ifeq (,$(wildcard $@))
	$(warning $(LIBNAME).a not found, attempting to rebuild in $(OPENCM3_DIR))
	$(MAKE) -C $(OPENCM3_DIR)
endif

print-%:
	@echo $*=$($*)

%.images: %.bin %.hex %.srec %.list %.map
	@#printf "*** $* images generated ***\n"

%.bin: %.elf
	$(Q)$(OBJCOPY) -Obinary $< $@

%.hex: %.elf
	$(Q)$(OBJCOPY) -Oihex $< $@

%.srec: %.elf
	$(Q)$(OBJCOPY) -Osrec $< $@

%.list: %.elf
	$(Q)$(OBJDUMP) -S $< > $@

# Link the final binary
%.elf %.map: $(OBJS) $(LDSCRIPT) $(OPENCM3_DIR)/lib/lib$(LIBNAME).a $(COMMON_DIR)/libcommon.a $(BOOTLOADER_BIN) Makefile
	$(Q)$(LD) $(TGT_LDFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $*.elf

# Compile C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(Q)$(CC) $(TGT_CFLAGS) $(CFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

# Compile Assembly sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(@D)
	$(Q)$(CC) $(TGT_CFLAGS) $(CFLAGS) -o $@ -c $<

# Compile C++ sources (.cxx)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cxx
	@mkdir -p $(@D)
	$(Q)$(CXX) $(TGT_CXXFLAGS) $(CXXFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

# Compile C++ sources (.cpp)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(Q)$(CXX) $(TGT_CXXFLAGS) $(CXXFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

# Make sure common directory is compiled
$(COMMON_DIR)/libcommon.a:
	$(warning libcommon.a not found, attempting to rebuild in $(COMMON_DIR))
	$(MAKE) -C $(COMMON_DIR)

# Make sure bootloader directory is compiled
$(BOOTLOADER_BIN):
	$(warning bootloader.bin not found, attempting to rebuild in $(BOOTLOADER_DIR))
	$(MAKE) -C $(BOOTLOADER_DIR)

$(OBJ_DIR)/bootloader.o: $(BOOTLOADER_BIN)

clean:
	$(Q)$(RM) $(GENERATED_BINARIES) generated.*
	$(Q)$(RM) -r $(OBJ_DIR)

.PHONY: images clean elf bin hex srec list

-include $(DEPS)