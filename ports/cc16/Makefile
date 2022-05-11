# Include the core environment definitions; this will set $(TOP).
include ../../py/mkenv.mk

# Include py core make definitions.
include $(TOP)/py/py.mk

# Define toolchain and other tools.
CROSS_COMPILE ?= arm-none-eabi-

# Set CFLAGS.
# XXX S32K could build hardfloat
CFLAGS += -I. -I$(TOP) -I$(BUILD)
CFLAGS += -Wall -Werror -std=c99 -nostdlib
CFLAGS += -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -msoft-float
CSUPEROPT = -Os # save some code space for performance-critical code

# Select debugging or optimisation build.
ifeq ($(DEBUG), 1)
CFLAGS += -Og
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections -ffreestanding
endif

# Set linker flags.
LDFLAGS += -T CC16.ld --gc-sections --nostdlib -Map=$(@:.elf=.map) --cref

# Define the required source files.
SRC_C += lib.c \
	 main.c \
	 mphalport.c \
	 s32k_can.c \
	 s32k_isr.c \
	 shared/libc/printf.c \
	 shared/libc/string0.c \
	 shared/readline/readline.c \
	 shared/runtime/gchelper_native.c \
	 shared/runtime/pyexec.c \
	 shared/runtime/stdout_helpers.c

SRC_S += shared/runtime/gchelper_m3.o

# Define the required object files.
OBJ += $(PY_CORE_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o))

# Define the top-level target, the main firmware.
all: $(BUILD)/firmware.s19

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^
	$(Q)$(SIZE) $@

$(BUILD)/firmware.s19: $(BUILD)/firmware.elf
	$(ECHO) "Create $@"
	$(Q)$(OBJCOPY) -O srec --srec-forceS3 $^ $@

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk
