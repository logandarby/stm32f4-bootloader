# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
endif

PREFIX      ?= arm-none-eabi-

CC          := $(PREFIX)gcc
AR          := $(PREFIX)ar

SRC_DIR     := src
OBJ_DIR     := obj

LIBNAME     := common
LIB_FILE    := lib$(LIBNAME).a

OPENCM3_DIR := ../libopencm3

###############################################################################
# Basic Device Setup

DEFS        += -DSTM32F4

FP_FLAGS    ?= -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS  := -mthumb -mcpu=cortex-m4 $(FP_FLAGS)

###############################################################################
# Includes

DEFS        += -I$(OPENCM3_DIR)/include
DEFS        += -I$(SRC_DIR)

###############################################################################
# Source files & Object generation

C_SRCS     := $(shell find $(SRC_DIR) -name '*.c')
OBJS       := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SRCS))
DEPS       := $(OBJS:.o=.d)

###############################################################################
# C flags

OPT         ?= -Os
DEBUG       ?= -ggdb3
CSTD        ?= -std=c99

TGT_CFLAGS  += $(OPT) $(CSTD) $(DEBUG)
TGT_CFLAGS  += $(ARCH_FLAGS)
TGT_CFLAGS  += -Wextra -Wshadow -Wimplicit-function-declaration
TGT_CFLAGS  += -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
TGT_CFLAGS  += -fno-common -ffunction-sections -fdata-sections

###############################################################################
# Preprocessor flags

TGT_CPPFLAGS += -MD
TGT_CPPFLAGS += -Wall -Wundef
TGT_CPPFLAGS += $(DEFS)

###############################################################################
# Build

all: $(LIB_FILE)

$(LIB_FILE): $(OBJS)
	$(Q)echo "AR $@"
	$(Q)$(AR) rcs $@ $^

# Compile C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) $(TGT_CFLAGS) $(CFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

###############################################################################
# Cleanup

clean:
	$(Q)$(RM) -r $(OBJ_DIR)
	$(Q)$(RM) $(LIB_FILE)

.PHONY: all clean

-include $(DEPS)
