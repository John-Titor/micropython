CPUTYPE			 = 68040
CFLAGS			+= -mcpu=$(CPUTYPE) -DMICROPY_HW_MCU_NAME=\"$(CPUTYPE)\"

SRC_C	+= \
	$(BOARD_DIR)/board.c \
	devices/ox16c954.c

.PHONY: srec
srec: $(BUILD)/firmware.s19

$(BUILD)/firmware.s19: $(BUILD)/firmware.elf
	$(OBJCOPY) -O srec --srec-forceS3 $< $@
