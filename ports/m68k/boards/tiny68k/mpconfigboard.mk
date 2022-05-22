EMULATOR ?= py68k.py
CFLAGS	+= -mcpu=68000

SRC_C	+= \
	$(BOARD_DIR)/board.c

DRIVERS_SRC_C += \
	devices/mc68681.c \

# Run emulation build on a POSIX system with suitable terminal settings
run: $(BUILD)/firmware.elf
	$(EMULATOR) --target tiny68k --load $(BUILD)/firmware.elf --load-address=0
