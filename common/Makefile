PREFIX     ?= arm-none-eabi-
CC         := $(PREFIX)gcc
AR         := $(PREFIX)ar

SRC_DIR    = src
OBJ_DIR    = obj
LIBNAME    = common
LIB_FILE   = lib$(LIBNAME).a

C_SRCS     := $(shell find $(SRC_DIR) -name '*.c')
OBJS       := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_SRCS))

# Inherit compiler flags or define basic ones for Cortex-M4
FP_FLAGS   ?= -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS = -mthumb -mcpu=cortex-m4 $(FP_FLAGS)
CFLAGS     ?= -Os -ggdb3 -std=c99 $(ARCH_FLAGS) -I$(SRC_DIR) -Wall -Wextra -ffunction-sections -fdata-sections

all: $(LIB_FILE)

$(LIB_FILE): $(OBJS)
	@echo "AR $@"
	@$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	@rm -rf $(OBJ_DIR) $(LIB_FILE)

.PHONY: all clean