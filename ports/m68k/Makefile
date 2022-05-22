BOARD ?= hatari
BOARD_DIR ?= boards/$(BOARD)
BUILD ?= build-$(BOARD)

include ../../py/mkenv.mk
include $(BOARD_DIR)/mpconfigboard.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h
QSTR_GLOBAL_DEPENDENCIES = $(BOARD_DIR)/mpconfigboard.h

FROZEN_MANIFEST ?= boards/manifest.py

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

CROSS_COMPILE ?= m68k-unknown-elf-
MPY_CROSS_FLAGS += -march=m68000

INC += -I.
INC += -Iinclude
INC += -I$(BOARD_DIR)
INC += -I$(TOP)
INC += -I$(BUILD)

CFLAGS += $(INC)
CFLAGS += $(BOARD_CFLAGS)
CFLAGS += $(COPT)
CFLAGS += -Wall -Werror -std=c99
CFLAGS += -nostdlib -ffunction-sections -fdata-sections
CFLAGS += -msoft-float	# no hard-float library...
CFLAGS += -Os # -DNDEBUG
LDSCRIPTS += $(BOARD_DIR)/memory.ld sections.ld
LDFLAGS += -nostdlib $(addprefix -T,$(LDSCRIPTS)) -Map=$@.map --cref --gc-sections

# for py68k sim purposes - need a minimal amount of stack and relocations
LDFLAGS += -zstack-size=32 --emit-relocs -e m68k_start
LIBS += $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

SRC_C += \
	exceptions.c \
	main.c \
	mphalport.c \

SHARED_SRC_C += \
	shared/libc/printf.c \
	shared/libc/string0.c \
	shared/readline/readline.c \
	shared/runtime/gchelper_generic.c \
	shared/runtime/interrupt_char.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	shared/runtime/sys_stdio_mphal.c \

LIBM_SRC_C = \
	lib/libm/math.c \
	lib/libm/fmodf.c \
	lib/libm/nearbyintf.c \
	lib/libm/ef_sqrt.c \
	lib/libm/erf_lgamma.c \
	lib/libm/kf_rem_pio2.c \
	lib/libm/kf_sin.c \
	lib/libm/kf_cos.c \
	lib/libm/kf_tan.c \
	lib/libm/ef_rem_pio2.c \
	lib/libm/sf_erf.c \
	lib/libm/sf_sin.c \
	lib/libm/sf_cos.c \
	lib/libm/sf_tan.c \
	lib/libm/sf_frexp.c \
	lib/libm/sf_modf.c \
	lib/libm/sf_ldexp.c \
	lib/libm/acoshf.c \
	lib/libm/asinfacosf.c \
	lib/libm/asinhf.c \
	lib/libm/atanf.c \
	lib/libm/atanhf.c \
	lib/libm/atan2f.c \
	lib/libm/log1pf.c \
	lib/libm/roundf.c \
	lib/libm/wf_lgamma.c \
	lib/libm/wf_tgamma.c \

SRC_S = \
	lib/setjmp.S \

SRC_QSTR += $(SRC_C) $(SHARED_SRC_C)

OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SHARED_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIBM_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(DRIVERS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S:.S=.o))

ifneq ($(FROZEN_MANIFEST),)
CFLAGS += -DMICROPY_MODULE_FROZEN_MPY
CFLAGS += -DMICROPY_MODULE_FROZEN_STR
CFLAGS += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
endif

all: $(BUILD)/firmware.bin

$(BUILD)/_frozen_mpy.c: $(TOP)/tests/frozen/frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

$(BUILD)/firmware.elf: $(OBJ) $(LDSCRIPTS)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $(filter %.o,$^) $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(OBJCOPY) -O binary -j .text -j .data $^ $(BUILD)/firmware.bin

$(BUILD)/firmware.dfu: $(BUILD)/firmware.bin
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) $(DFU) -b 0x08000000:$(BUILD)/firmware.bin $@

deploy: $(BUILD)/firmware.dfu
	$(ECHO) "Writing $< to the board"
	$(Q)$(PYTHON) $(PYDFU) -u $<

include $(TOP)/py/mkrules.mk
