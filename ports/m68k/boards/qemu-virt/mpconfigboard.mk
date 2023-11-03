# Must use 020 or better due to peripherals being mapped above 0xff00'0000
#
CPUTYPE			 = 68020
CFLAGS			+= -mcpu=$(CPUTYPE) -DMICROPY_HW_MCU_NAME=\"$(CPUTYPE)\"

SRC_C	+= \
	$(BOARD_DIR)/board.c

QEMU_MEM_SIZE		 = 16M
QEMU_MEM_FILE		 = /tmp/micropython-qemu.ram
RUN_ARGS		+= -M virt,memory-backend=virt.ram
RUN_ARGS		+= -m $(QEMU_MEM_SIZE)
RUN_ARGS		+= -cpu m$(CPUTYPE)
RUN_ARGS		+= -object memory-backend-file,size=$(QEMU_MEM_SIZE),id=virt.ram,mem-path=$(QEMU_MEM_FILE),share=on,prealloc=on
RUN_ARGS		+= -nographic
RUN_ARGS		+= -serial mon:stdio
RUN_ARGS		+= -kernel $(BUILD)/firmware.elf

TRACE_ARGS		 = -accel tcg,one-insn-per-tb=on,thread=single -d exec,cpu,in_asm -D /tmp/micropython-qemu.log

run: $(BUILD)/firmware.bin
	qemu-system-m68k $(RUN_ARGS)

trace: $(BUILD)/firmware.bin
	qemu-system-m68k $(RUN_ARGS) $(TRACE_ARGS)
