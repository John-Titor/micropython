CPULEVEL	?= 0
CPUTYPE		 = 680$(CPULEVEL)0
CFLAGS		+= -mcpu=$(CPUTYPE) -DMICROPY_HW_MCU_NAME=\"$(CPUTYPE)\"

SRC_C	+= \
	$(BOARD_DIR)/board.c \
	devices/native_features.c \
	devices/atarist.c \

# For serial console, run:
#   socat -d -d -v pty,rawer,link=/tmp/pseudo-serial open:/dev/tty

RUN_ARGS = \
	--configfile $(BOARD_DIR)/hatari-run.cfg \
	--tos $(BUILD)/firmware.bin \
	--cpulevel $(CPULEVEL)

SERIAL_ARGS = \
	--rs232-in /tmp/pseudo-serial \
	--rs232-out /tmp/pseudo-serial

TRACE_OPTS += cpu_all
TRACE_OPTS += mem
#TRACE_OPTS += cpu_disasm
#TRACE_OPTS += cpu_regs
TRACE_ARGS = --trace-file /tmp/trace.txt $(addprefix --trace +,$(TRACE_OPTS))

run: $(BUILD)/firmware.bin
	hatari $(RUN_ARGS)

trace: $(BUILD)/firmware.bin
	hatari $(RUN_ARGS) $(TRACE_ARGS)
