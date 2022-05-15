# Include the core environment definitions; this will set $(TOP).
include ../../py/mkenv.mk

# Qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

FROZEN_MANIFEST = manifest.py

# Include py core make definitions.
include $(TOP)/py/py.mk

# Define toolchain and other tools.
CROSS_COMPILE ?= arm-none-eabi-

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(TOP)/lib/cmsis/inc

# Set CFLAGS.
# XXX S32K could build hardfloat
CFLAGS += $(INC)
CFLAGS += -Wall -Werror -std=c99 -nostdlib
CFLAGS += -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CSUPEROPT = -Os

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
SRC_C += \
	cc16_adc.c \
	cc16_ftm.c \
	cc16_input.c \
	cc16_output.c \
	cc16_pin.c \
	cc16_vref.c \
	help.c \
	lib.c \
	main.c \
	modmachine.c \
	mphalport.c \
	s32k_can.c \
	s32k_isr.c \
	shared/libc/printf.c \
	shared/libc/string0.c \
	shared/readline/readline.c \
	shared/runtime/gchelper_native.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	shared/runtime/sys_stdio_mphal.c \
	shared/timeutils/timeutils.c \

SRC_C += $(SRC_MOD) 

SRC_S += shared/runtime/gchelper_m3.o

SRC_QSTR += \
	cc16_input.c \
	cc16_output.c \
	cc16_vref.c \
	modmachine.c \
	shared/runtime/sys_stdio_mphal.c \

SRC_QSTR += \
	$(SRC_MOD)

# Define the required object files.
OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o))

ifneq ($(FROZEN_MANIFEST),)
CFLAGS += -DMICROPY_MODULE_FROZEN_MPY
CFLAGS += -DMICROPY_MODULE_FROZEN_STR
CFLAGS += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
endif

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
