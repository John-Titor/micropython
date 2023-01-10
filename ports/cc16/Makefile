# Include the core environment definitions; this will set $(TOP).
include ../../py/mkenv.mk

# Qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# Files to freeze into the binary
FROZEN_MANIFEST = manifest.py

# Include py core make definitions.
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

# Define toolchain and other tools.
CROSS_COMPILE ?= arm-none-eabi-
MPY_CROSS_MCU_ARCH = armv7m
MPY_CROSS_FLAGS += -march=$(MPY_CROSS_MCU_ARCH)

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(TOP)/lib/cmsis/inc

# Set CFLAGS.
CFLAGS += $(INC)
CFLAGS += -Wall -Werror -std=c99 -nostdlib
CFLAGS += -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 
CFLAGS += -fsingle-precision-constant -Wfloat-conversion -Wdouble-promotion -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS += $(OPT)
CSUPEROPT = -Os

# Select debugging or optimisation build.
ifeq ($(DEBUG), 1)
CFLAGS += -Og
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

# Set linker flags.
LDFLAGS += -T CC16.ld --gc-sections --nostdlib -Map=$(@:.elf=.map) --cref

# Optional libraries
LIBS += $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

# Define the required source files.
SRC_C += \
	cc16_adc.c \
	cc16_can.c \
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
	s32k_isr.c \

# math library
LIBM_SRC_C += \
	lib/libm/acoshf.c \
	lib/libm/asinfacosf.c \
	lib/libm/asinhf.c \
	lib/libm/atan2f.c \
	lib/libm/atanf.c \
	lib/libm/atanhf.c \
	lib/libm/ef_rem_pio2.c \
	lib/libm/ef_sqrt.c \
	lib/libm/erf_lgamma.c \
	lib/libm/fmodf.c \
	lib/libm/fmodf.c \
	lib/libm/kf_cos.c \
	lib/libm/kf_rem_pio2.c \
	lib/libm/kf_sin.c \
	lib/libm/kf_tan.c \
	lib/libm/log1pf.c \
	lib/libm/math.c \
	lib/libm/math.c \
	lib/libm/nearbyintf.c \
	lib/libm/nearbyintf.c \
	lib/libm/roundf.c \
	lib/libm/sf_cos.c \
	lib/libm/sf_erf.c \
	lib/libm/sf_frexp.c \
	lib/libm/sf_ldexp.c \
	lib/libm/sf_modf.c \
	lib/libm/sf_sin.c \
	lib/libm/sf_tan.c \
	lib/libm/wf_lgamma.c \
	lib/libm/wf_tgamma.c \

SHARED_SRC_C += \
	shared/runtime/mpirq.c \
	shared/libc/printf.c \
	shared/libc/string0.c \
	shared/readline/readline.c \
	shared/runtime/gchelper_native.c \
	shared/runtime/interrupt_char.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	shared/runtime/sys_stdio_mphal.c \
	shared/timeutils/timeutils.c \

#	shared/runtime/softtimer.c \

SRC_S += shared/runtime/gchelper_m3.o

SRC_QSTR += \
	$(SRC_C) \
	$(SHARED_SRC_C) \

# Define the required object files.
OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(LIBM_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SHARED_SRC_C:.c=.o))

ifneq ($(FROZEN_MANIFEST),)
CFLAGS += -DMICROPY_MODULE_FROZEN_MPY
CFLAGS += -DMICROPY_MODULE_FROZEN_STR
CFLAGS += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
endif

# Define the top-level target, the main firmware.
all: $(BUILD)/firmware.s19

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.s19: $(BUILD)/firmware.elf
	$(ECHO) "Create $@"
	$(Q)$(OBJCOPY) -O srec --srec-forceS3 $^ $@

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk
