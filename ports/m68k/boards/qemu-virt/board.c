/*
 * Board-level support for qemu-virt.
 *
 * References:
 *  https://github.com/qemu/qemu/blob/master/hw/m68k/virt.c
 */

#include <py/mpconfig.h>
#include <py/runtime.h> // for assert()
#include <mphalport.h>
#include <stdio.h>

static void bootinfo_parse(void);

static uintptr_t gf_pic_base;
static uintptr_t gf_rtc_base;
static int gf_rtc_vector;
static uintptr_t gf_tty_base;
static int gf_tty_vector;
static uintptr_t qemu_ctrl_base;

#define REG(_dev, _ofs) (*(volatile uint32_t *)((_dev) + (_ofs)))

static void gf_pic_init(void);
static void gf_rtc_init(void);
static void gf_tty_init(void);
static void gf_tty_putc(char c);
static int  gf_tty_getc(void);

void
m68k_board_init(void) {
    bootinfo_parse();
    gf_pic_init();
    gf_tty_init();
    gf_rtc_init();
}

// Receive single character from keyboard
int
mp_hal_stdin_rx_chr(void) {
    int c;
    for (;;) {
        if ((c = gf_tty_getc()) >= 0) {
            return c;
        }
    }
}

// Send string to console
void
mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        gf_tty_putc(*str++);
    }
}

////////////////////////////////////////////////////////////////////////////////
// qemu bootinfo
//

enum
{
    BI_LAST              = 0x0000,
    BI_MACHTYPE          = 0x0001,
    BI_CPUTYPE           = 0x0002,
    BI_FPUTYPE           = 0x0003,
    BI_MPUTYPE           = 0x0004,
    BI_MEMCHUNK          = 0x0005,
    BI_RAMDISK           = 0x0006,
    BI_COMMANDLINE       = 0x0007,
    BI_RNG_SEED          = 0x0008,

    BI_VIRT_QEMU_VERSION = 0x8000,
    BI_VIRT_GF_PIC_BASE  = 0x8001,
    BI_VIRT_GF_RTC_BASE  = 0x8002,
    BI_VIRT_GF_TTY_BASE  = 0x8003,
    BI_VIRT_VIRTIO_BASE  = 0x8004,
    BI_VIRT_CTRL_BASE    = 0x8005,
};

#pragma pack(push, 2)
struct BI_Header
{
    uint16_t tag;
    uint16_t size;
};

struct BI_MemInfo
{
    struct BI_Header hdr;
    uint32_t addr;
    uint32_t size;
};

struct BI_DevInfo
{
    struct BI_Header hdr;
    uint32_t addr;
    uint32_t irq;
};
#pragma pack(pop)

static void
bootinfo_parse(void) {
    extern const uint8_t __end;
    const uint8_t *cursor = &__end;

    for (;;) {
        const struct BI_Header *hdr = (const struct BI_Header *)cursor;
        // const struct BI_MemInfo *mi  = (const struct BI_MemInfo *)cursor;
        const struct BI_DevInfo *di = (const struct BI_DevInfo *)cursor;
        cursor += hdr->size;

        switch (hdr->tag) {
            // XXX BI_RAMDISK?
            // XXX BI_VIRT_VIRTIO?
            case BI_VIRT_GF_PIC_BASE:
                gf_pic_base = di->addr;
                break;
            case BI_VIRT_GF_RTC_BASE:
                assert(di->irq >= 8);
                gf_rtc_base = di->addr;
                gf_rtc_vector = di->irq - 8;
                break;
            case BI_VIRT_GF_TTY_BASE:
                // if we do this, we loose early output because the compiler thinks it's
                // smarter than it is...
                assert(di->irq >= 8);
                gf_tty_base = di->addr;
                gf_tty_vector = di->irq - 8;
                printf("GF_TTY: base 0x%08lx irq %d\n", gf_tty_base, gf_tty_vector);
                break;
            case BI_VIRT_CTRL_BASE:
                qemu_ctrl_base = di->addr;
                break;
            case BI_LAST:
                return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// goldfish interrupt controller
//

enum
{
    PIC_NUM_BANKS           = 6,
    PIC_VECTORS_PER_BANK    = 32,
    PIC_NUM_VECTORS         = PIC_NUM_BANKS * PIC_VECTORS_PER_BANK,
    PIC_REG_STATUS          = 0x00,
    PIC_REG_IRQ_PENDING     = 0x04,
    PIC_REG_IRQ_DISABLE_ALL = 0x08,
    PIC_REG_DISABLE         = 0x0c,
    PIC_REG_ENABLE          = 0x10,
};

typedef void (*gf_pic_handler)(void);
static gf_pic_handler gf_pic_handlers[PIC_NUM_BANKS][PIC_VECTORS_PER_BANK];

#define PIC_REG(_bank, _reg) REG((gf_pic_base + 0x1000 * (_bank)), PIC_REG_##_reg)

static void
gf_pic_interrupt_handler(int bank) {
    assert(bank < PIC_NUM_BANKS);

    for (;;) {
        int pending = __builtin_ffs(PIC_REG(bank, IRQ_PENDING));
        if (pending) {
            gf_pic_handler handler = gf_pic_handlers[bank][pending - 1];
            if (handler) {
                handler();
            } else {
                printf("GF_PIC: unhandled %d,%d\n", bank, pending - 1);
                PIC_REG(bank, DISABLE) = 1 << (pending - 1);
            }
        } else {
            break;
        }
    }
}

M68K_INTERRUPT_HANDLER(25, gf_pic_handler_0)
{
    gf_pic_interrupt_handler(0);
}

M68K_INTERRUPT_HANDLER(26, gf_pic_handler_1)
{
    gf_pic_interrupt_handler(1);
}

M68K_INTERRUPT_HANDLER(27, gf_pic_handler_2)
{
    gf_pic_interrupt_handler(2);
}

M68K_INTERRUPT_HANDLER(28, gf_pic_handler_3)
{
    gf_pic_interrupt_handler(3);
}

M68K_INTERRUPT_HANDLER(29, gf_pic_handler_4)
{
    gf_pic_interrupt_handler(4);
}

M68K_INTERRUPT_HANDLER(30, gf_pic_handler_5)
{
    gf_pic_interrupt_handler(5);
}

static void
gf_pic_init(void) {
    assert(gf_pic_base != 0);
}

static void
gf_pic_attach(int vector, gf_pic_handler handler) {
    assert(vector < PIC_NUM_VECTORS);
    gf_pic_handlers[vector / PIC_VECTORS_PER_BANK][vector % PIC_VECTORS_PER_BANK] = handler;
}

static void
gf_pic_enable(int vector) {
    assert(vector < PIC_NUM_VECTORS);
    if (gf_pic_handlers[vector / PIC_VECTORS_PER_BANK][vector % PIC_VECTORS_PER_BANK]) {
        PIC_REG(vector / PIC_VECTORS_PER_BANK, ENABLE) = (1 << (vector % PIC_VECTORS_PER_BANK));
    }
}

static void
gf_pic_disable(int vector) {
    assert(vector < PIC_NUM_VECTORS);

    PIC_REG(vector / PIC_VECTORS_PER_BANK, DISABLE) = (1 << (vector % PIC_VECTORS_PER_BANK));
}

////////////////////////////////////////////////////////////////////////////////
// goldfish realtime clock
//

enum
{
    RTC_TICK_INTERVAL       = (1000LLU * 1000LLU), // 1ms as nsec
    RTC_REG_TIME_LOW        = 0x00,
    RTC_REG_TIME_HIGH       = 0x04,
    RTC_REG_ALARM_LOW       = 0x08,
    RTC_REG_ALARM_HIGH      = 0x0c,
    RTC_REG_IRQ_ENABLED     = 0x10,
    RTC_REG_CLEAR_ALARM     = 0x14,
    RTC_REG_ALARM_STATUS    = 0x18,
    RTC_REG_CLEAR_INTERRUPT = 0x1c,
};
static volatile uint64_t rtc_last_tick;

#define RTC_REG(_reg) REG(gf_rtc_base, RTC_REG_##_reg)

static void
gf_rtc_next_tick(void) {
    rtc_last_tick += RTC_TICK_INTERVAL;
    RTC_REG(ALARM_HIGH) = rtc_last_tick >> 32;
    RTC_REG(ALARM_LOW) = rtc_last_tick & 0xffffffff;
    RTC_REG(IRQ_ENABLED) = 1;

    // XXX check that we haven't blown past the next tick deadline?
}

static void
gf_rtc_handler(void) {
    RTC_REG(CLEAR_ALARM)     = 0;
    RTC_REG(CLEAR_INTERRUPT) = 0;
    m68k_timer_tick();
    gf_rtc_next_tick();
}

static void
gf_rtc_init(void) {
    gf_pic_attach(gf_rtc_vector, gf_rtc_handler);
    gf_pic_enable(gf_rtc_vector);

    rtc_last_tick = RTC_REG(TIME_LOW);
    rtc_last_tick |= (uint64_t)RTC_REG(TIME_HIGH) << 32;
    gf_rtc_next_tick();
}

////////////////////////////////////////////////////////////////////////////////
// goldfish tty
//

enum
{
    TTY_BUF_SIZE         = 128,
    TTY_PROTOCOL_VERSION = 1,
    TTY_REG_PUT_CHAR     = 0x00,
    TTY_REG_BYTES_READY  = 0x04,
    TTY_REG_CMD          = 0x08,
    TTY_REG_DATA_PTR     = 0x10,
    TTY_REG_DATA_LEN     = 0x14,
    TTY_REG_VERSION      = 0x20,
    TTY_CMD_INT_DISABLE  = 0x00,
    TTY_CMD_INT_ENABLE   = 0x01,
    TTY_CMD_WRITE_BUFFER = 0x02,
    TTY_CMD_READ_BUFFER  = 0x03,
};
static uint8_t tty_buf[TTY_BUF_SIZE];
static volatile uint16_t tty_buf_in;
static volatile uint16_t tty_buf_out;

#define TTY_REG(_reg) REG(gf_tty_base, TTY_REG_##_reg)

static void
gf_tty_handler(void) {
    if (TTY_REG(BYTES_READY) != 0) {
        TTY_REG(DATA_PTR) = (uint32_t)&tty_buf[tty_buf_in++ % TTY_BUF_SIZE];
        TTY_REG(DATA_LEN) = 1;
        TTY_REG(CMD) = TTY_CMD_READ_BUFFER;

        // input buffer full?
        if ((tty_buf_in - tty_buf_out) >= TTY_BUF_SIZE) {
            gf_pic_disable(gf_tty_vector);
        }
    }
}

static void
gf_tty_init(void) {
    assert(gf_tty_base != 0);

    gf_pic_attach(gf_tty_vector, gf_tty_handler);
    gf_pic_enable(gf_tty_vector);
    TTY_REG(CMD) = TTY_CMD_INT_ENABLE;
}

static void
gf_tty_putc(char c) {
    // use default TTY address before bootinfo parsed
    REG((gf_tty_base ? gf_tty_base : 0xff008000U), TTY_REG_PUT_CHAR) = c;
}

static int
gf_tty_getc(void) {
    int c = -1;
    if (tty_buf_out != tty_buf_in) {
        c = tty_buf[tty_buf_out++ % TTY_BUF_SIZE];

        // might have been disabled due to buffer-full condition
        gf_pic_enable(gf_tty_vector);
    }
    return c;
}

////////////////////////////////////////////////////////////////////////////////
// qemu control device
//

enum
{
    CTRL_NOOP  = 0x00,
    CTRL_RESET = 0x01,
    CTRL_HALT  = 0x02,
    CTRL_PANIC = 0x03,
};

__attribute__((noreturn)) void
qemu_exit() {
    REG(qemu_ctrl_base, 0) = CTRL_HALT;
    // does not necessarily exit immediately - spin until it does
    for (;;) {
        ;
    }
}
