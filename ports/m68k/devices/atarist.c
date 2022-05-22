//
// Hardware support for Atari ST systems
//
// https://temlib.org/AtariForumWiki/index.php/Atari_ST/STe/MSTe/TT/F030_Hardware_Register_Listing
// https://temlib.org/AtariForumWiki/index.php/MFP_MK68901
//

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <py/mpconfig.h>
#include <py/runtime.h>
#include <mphalport.h>

static void atarist_mfp_init(void);
static void atarist_ikbd_init(void);
static void atarist_video_init(void);

#if ATARIST_SERIAL_CONSOLE
static int atarist_mfp_rx_chr(void);
static void atarist_mfp_tx_chr(uint8_t c);
#endif // ATARIST_SERIAL_CONSOLE

static int atarist_ikbd_rx_chr(void);
static void atarist_vid_tx_chr(uint8_t c);
static void atarist_ikbd_handler(void);
static void atarist_ikbd_repeat(int c);
static void atarist_vid_cursor_tick(void);
static void atarist_wait_vsync(uint32_t count);

void m68k_atarist_early_init(void);
__asm__ (
    "   .section .text.startup                  \n"
    "   .align 2                                \n"
    "   .type m68k_atarist_early_init @function \n"
    "   .global m68k_atarist_early_init         \n"
    "m68k_atarist_early_init:                   \n" // reset entrypoint
    "   move.b  #0x5, 0xffff8001                \n" // default memory config, 2x512K banks
    "   bra     m68k_init                       \n" // chain to next stage
    );

void m68k_atarist_init(void) {
    atarist_ikbd_init();
    atarist_mfp_init();
    atarist_video_init();
}

// Receive single character from keyboard, and possibly
// serial console.
int mp_hal_stdin_rx_chr(void) {
    int c;
    for (;;) {
        if ((c = atarist_ikbd_rx_chr()) >= 0) {
            return c;
        }
        #if ATARIST_SERIAL_CONSOLE
        if ((c = atarist_mfp_rx_chr()) >= 0) {
            return c;
        }
        #endif // ATARIST_SERIAL_CONSOLE
    }
}

void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        atarist_vid_tx_chr(*str);
        #if ATARIST_SERIAL_CONSOLE
        atarist_mfp_tx_chr(*str);
        #endif // ATARIST_SERIAL_CONSOLE
        str++;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Atari ST ROM header
//

extern void m68k_start(void);

struct {
    uint16_t bra;
    uint16_t version;
    uint32_t entry;
    uint32_t base;
}
__attribute__((packed))
hatari_rom_header
__attribute__((section(".header")))
__attribute__((used)) = {
    .bra = 0,
    .version = 0x500,
    .entry = (uint32_t)m68k_start,
    .base = 0x00e00000
};

////////////////////////////////////////////////////////////////////////////////
// MC68901 MFP
//

#define MFP_CLOCK       2457600
#define MFP_BASE        0xFFFA01
#define _MFP_REG(_adr)  (*(volatile uint8_t *)(MFP_BASE + (_adr << 1)))

#define MFP_GPIP    _MFP_REG(0x00)          // General purpose I/O
#define MFP_AER     _MFP_REG(0x01)          // Active edge register, 0=high->low, 1=low->high
#define MFP_DDR     _MFP_REG(0x02)          // Data direction register
#define MFP_PPR_CENT_BUSY    0x01           // parallel port bits
#define MFP_PPR_DCD          0x02
#define MFP_PPR_CTS          0x04
#define MFP_PPR_IKBD_MIDI    0x10
#define MFP_PPR_FDC_HDC      0x20
#define MFP_PPR_RI           0x40
#define MFP_PPR_MONO         0x80
#define MFP_IERA    _MFP_REG(0x03)          // Interrupt enable register A
#define MFP_IERB    _MFP_REG(0x04)          // Interrupt enable register B
#define MFP_IPRA    _MFP_REG(0x05)          // Interrupt pending register A
#define MFP_IPRB    _MFP_REG(0x06)          // Interrupt pending register B
#define MFP_ISRA    _MFP_REG(0x07)          // Interrupt in-service register A
#define MFP_ISRB    _MFP_REG(0x08)          // Interrupt in-service register B
#define MFP_IMRA    _MFP_REG(0x09)          // Interrupt mask register A
#define MFP_IMRB    _MFP_REG(0x0a)          // Interrupt mask register B
#define MFP_IA_TIMER_B       0x01
#define MFP_IA_SEND_ERR      0x02
#define MFP_IA_SEND_EMPTY    0x04
#define MFP_IA_RECV_ERR      0x08
#define MFP_IA_RECV_FULL     0x10
#define MFP_IA_TIMER_A       0x20
#define MFP_IA_RI            0x40
#define MFP_IA_MONO          0x80
#define MFP_IB_CENT_BUSY     0x01
#define MFP_IB_DCD           0x02
#define MFP_IB_CTS           0x04
#define MFP_IB_BLITTER       0x08
#define MFP_IB_TIMER_D       0x10
#define MFP_IB_TIMER_C       0x20
#define MFP_IB_IKBD_MIDI     0x40
#define MFP_IB_FDC_HDC       0x80
#define MFP_VR      _MFP_REG(0x0b)          // Vector register
#define MFP_VR_SW_EOI        0x08
#define MFP_TACR    _MFP_REG(0x0c)          // Timer A control register
#define MFP_TBCR    _MFP_REG(0x0d)          // Timer B control register
#define MFP_TCDCR   _MFP_REG(0x0e)          // Timers C and D control registers
#define MFP_TC_OFF           0x00
#define MFP_TC_DIV_4         0x01
#define MFP_TC_DIV_10        0x02
#define MFP_TC_DIV_16        0x03
#define MFP_TC_DIV_50        0x04
#define MFP_TC_DIV_64        0x05
#define MFP_TC_DIV_100       0x06
#define MFP_TC_DELAY_DIV_200 0x07
#define MFP_TC_EVENT_CTR     0x10
#define MFP_TC_EXTEND        0x10
#define MFP_TADR    _MFP_REG(0x0f)          // Timer A data register
#define MFP_TBDR    _MFP_REG(0x10)          // Timer B data register
#define MFP_TCDR    _MFP_REG(0x11)          // Timer C data register
#define MFP_TDDR    _MFP_REG(0x12)          // Timer D data register
#define MFP_SCR     _MFP_REG(0x13)          // Sync character register
#define MFP_UCR     _MFP_REG(0x14)          // USART control register
#define MFP_UCR_PAR_EVEN     0x02
#define MFP_UCR_PAR_EN       0x04
#define MFP_UCR_SYNC         0x00
#define MFP_UCR_STOP_1       0x08
#define MFP_UCR_STOP_1_5     0x10
#define MFP_UCR_STOP_2       0x18
#define MFP_UCR_DATA_8       0x00
#define MFP_UCR_DATA_7       0x20
#define MFP_UCR_DATA_6       0x40
#define MFP_UCR_DATA_5       0x60
#define MFP_UCR_DIV_16       0x80
#define MFP_RSR     _MFP_REG(0x15)          // Receiver status register
#define MFP_RSR_ENABLE       0x01
#define MFP_RSR_SYNC_STRIP   0x02
#define MFP_RSR_IN_PROGRESS  0x04
#define MFP_RSR_MATCH_BREAK  0x08
#define MFP_RSR_FRAME_ERR    0x10
#define MFP_RSR_PARITY_ERR   0x20
#define MFP_RSR_OVERRUN      0x40
#define MFP_RSR_BUFFER_FULL  0x80
#define MFP_TSR     _MFP_REG(0x16)          // Transmitter status register
#define MFP_TSR_ENABLE       0x01
#define MFP_TSR_LOW_BIT      0x02
#define MFP_TSR_HIGH_BIT     0x04
#define MFP_TSR_BREAK        0x08
#define MFP_TSR_EOT          0x10
#define MFP_TSR_AUTO_TA      0x20
#define MFP_TSR_UNDERRUN     0x40
#define MFP_TSR_BUFFER_EMPTY 0x80
#define MFP_UDR     _MFP_REG(0x17)          // USART data register

#define MFP_RX_BUF_SIZE     128
static char mfp_rx_buf[MFP_RX_BUF_SIZE];
static volatile uint16_t mfp_rx_head;
static volatile uint16_t mfp_rx_tail;

void atarist_mfp_init(void) {
    MFP_VR = 64 | MFP_VR_SW_EOI;            // establish vector base
    MFP_IERA = 0;                           // disable MFP interrupts
    MFP_IERB = 0;
    MFP_IMRA = 0;
    MFP_IMRB = 0;
    MFP_TACR = 0;                           // disable timers
    MFP_TBCR = 0;
    MFP_TCDCR = 0;
    MFP_RSR = 0;                             // disable UART
    MFP_TSR = 0;

    // do serial setup
    MFP_UCR = MFP_UCR_STOP_1 | MFP_UCR_DATA_8 | MFP_UCR_DIV_16; // n81
    MFP_TCDCR |= MFP_TC_DIV_4;              // Timer D, /4 prescale
    MFP_TDDR = 1;                           // 1 count: 2457600 / (16 * 4 * 1) = 19200bps
    MFP_IERA |= MFP_IA_RECV_FULL;           // enable serial receive interrupt
    MFP_IMRA |= MFP_IA_RECV_FULL;           // unmask serial receive interrupt
    MFP_RSR = MFP_RSR_ENABLE;               // enable receiver
    MFP_TSR = MFP_TSR_ENABLE;               // enable transmitter

    // do timer C setup for 1ms tick
    MFP_TCDCR |= MFP_TC_DIV_64 << 4;        // Timer C, /10 prescale
    MFP_TCDR = 192;                         // 5ms tick interval
    MFP_IERB |= MFP_IB_TIMER_C;             // enable timer C interrupt
    MFP_IMRB |= MFP_IB_TIMER_C;             // unmask timer C interrupt

    // ACIA init already done, so we can turn on the interrupt
    MFP_IERB |= MFP_IB_IKBD_MIDI;
    MFP_IMRB |= MFP_IB_IKBD_MIDI;
}

M68K_INTERRUPT_HANDLER(69, atarist_mfp_timer_c)
{
    atarist_ikbd_repeat(-1);                // do key-repeat processing
    atarist_vid_cursor_tick();
    m68k_timer_tick();                      // do tick processing

    MFP_ISRB = ~MFP_IB_TIMER_C;             // clear pending interrupt
}

M68K_INTERRUPT_HANDLER(70, atarist_mfp_acia_handler)
{
    MFP_ISRB = ~MFP_IB_IKBD_MIDI;           // clear pending interrupt
    atarist_ikbd_handler();                 // check for ikbd byte
    // atarist_midi_handler();              // check for MIDI byte
}

M68K_INTERRUPT_HANDLER(76, mfp_recv_full)
{
    // receive character
    char c = MFP_UDR;
    if ((mfp_rx_head - mfp_rx_tail) < MFP_RX_BUF_SIZE) {
        mfp_rx_buf[mfp_rx_head++ % MFP_RX_BUF_SIZE] = c;
    }
    if (c == mp_interrupt_char) {
        mp_sched_keyboard_interrupt();
    }

    // receive character
    MFP_ISRB = ~MFP_IA_RECV_FULL;           // clear pending interrupt
}

#if ATARIST_SERIAL_CONSOLE
static int
atarist_mfp_rx_chr(void) {
    if (mfp_rx_head != mfp_rx_tail) {
        return mfp_rx_buf[mfp_rx_tail++ % MFP_RX_BUF_SIZE];
    }
    return -1;
}

static void atarist_mfp_tx_chr(uint8_t c) {
    for (;;) {
        if (MFP_TSR & MFP_TSR_BUFFER_EMPTY) {
            MFP_UDR = c;
            break;
        }
    }
}
#endif // ATARIST_SERIAL_CONSOLE


////////////////////////////////////////////////////////////////////////////////
// 6850 ACIA interface to IKBD
//

#define _ACIA_REG(_adr)  (*(volatile uint8_t *)(_adr))
#define IKBD_CTRL           _ACIA_REG(0xfffc00)
#define IKBD_DATA           _ACIA_REG(0xfffc02)
#define MIDI_CTRL           _ACIA_REG(0xfffc04)
#define MIDI_DATA           _ACIA_REG(0xfffc06)

#define ACIA_CTRL_DIV_1               0x00
#define ACIA_CTRL_DIV_16              0x01
#define ACIA_CTRL_DIV_64              0x02
#define ACIA_CTRL_RESET               0x03
#define ACIA_CTRL_7E2                 0x00
#define ACIA_CTRL_7O2                 0x04
#define ACIA_CTRL_7E1                 0x08
#define ACIA_CTRL_7O1                 0x0c
#define ACIA_CTRL_8N2                 0x10
#define ACIA_CTRL_8N1                 0x14
#define ACIA_CTRL_8E1                 0x18
#define ACIA_CTRL_8O1                 0x1c
#define ACIA_CTRL_TXINT_DIS           0x00
#define ACIA_CTRL_TXINT_EN            0x20
#define ACIA_CTRL_RTS_HIGH            0x40
#define ACIA_CTRL_BREAK               0x60
#define ACIA_CTRL_RXINT_EN            0x80

#define ACIA_CTRL_RX_FULL             0x01
#define ACIA_CTRL_TX_EMPTY            0x02
#define ACIA_CTRL_DCD                 0x04
#define ACIA_CTRL_CTS                 0x08
#define ACIA_CTRL_FRAME_ERR           0x10
#define ACIA_CTRL_OVERRUN             0x20
#define ACIA_CTRL_PARITY_ERR          0x40
#define ACIA_CTRL_INT_REQ             0x80

#define IKBD_BUF_SIZE     128
static uint8_t ikbd_rx_buf[IKBD_BUF_SIZE];
static volatile uint16_t ikbd_rx_head;
static volatile uint16_t ikbd_rx_tail;

static void
atarist_ikbd_send(uint8_t c) {
    while (!(IKBD_CTRL & ACIA_CTRL_TX_EMPTY)) {
        ;
    }
    IKBD_DATA = c;
}

static void atarist_ikbd_init(void) {
    // reset the IKBD ACIA and configure for keyboard comms
    IKBD_CTRL = ACIA_CTRL_RESET;
    IKBD_CTRL = ACIA_CTRL_RXINT_EN | ACIA_CTRL_TXINT_DIS | ACIA_CTRL_DIV_64 | ACIA_CTRL_8N1;

    // reset the MIDI ACIA to avoid spurious interrupts
    MIDI_CTRL = ACIA_CTRL_RESET;

    // do IKBD reset
    atarist_ikbd_send(0x80);
    atarist_ikbd_send(0x01);
}

static void atarist_ikbd_enqueue(uint8_t c) {
    if (c != 0) {
        if ((ikbd_rx_head - ikbd_rx_tail) < IKBD_BUF_SIZE) {
            ikbd_rx_buf[ikbd_rx_head++ % IKBD_BUF_SIZE] = c;
        }
    }
}

static void atarist_ikbd_repeat(int c) {
    static uint8_t current_char;
    static int ticks_to_repeat;

    // set a new character
    if (c >= 0) {
        current_char = c & 0xff;
        if (c > 0) {
            atarist_ikbd_enqueue(c);
            ticks_to_repeat = 100;
        } else {
            ticks_to_repeat = 0;
        }
        return;
    }

    // repeat the current character
    if (ticks_to_repeat > 1) {
        ticks_to_repeat--;
    } else if (ticks_to_repeat == 1) {
        atarist_ikbd_enqueue(current_char);
        ticks_to_repeat = 20;
    }
}

#define IKBD_RELEASE    0x80

#define IKBD_KEY_LSHIFT 0x2a
#define IKBD_KEY_RSHIFT 0x36
#define IKBD_KEY_CTRL   0x1d
#define IKBD_KEY_ALT    0x38
#define IKBD_KEY_CAPS   0x3a

#define _IKBD_ALPHA(_c) _c, (_c - 32), (_c - 96)

// rudimentary scan table - will need to be better for readline support
static struct
{
    uint8_t unmod, shift, ctrl;
} ikbd_scan_table[256] = {
    [0x01 ] = {   0x1b,   0x1b,   0x1b    },  // ESC
    [0x02 ] = {   '1',    '!',    0       },  // 1
    [0x03 ] = {   '2',    '@',    0       },  // 2
    [0x04 ] = {   '3',    '#',    0       },  // 3
    [0x05 ] = {   '4',    '$',    0       },  // 4
    [0x06 ] = {   '5',    '%',    0       },  // 5
    [0x07 ] = {   '6',    '^',    0       },  // 6
    [0x08 ] = {   '7',    '&',    0       },  // 7
    [0x09 ] = {   '8',    '*',    0       },  // 8
    [0x0a ] = {   '9',    '(',    0       },  // 9
    [0x0b ] = {   '0',    ')',    0       },  // 0
    [0x0c ] = {   '-',    '_',    0       },  // -
    [0x0d ] = {   '=',    '+',    0       },  // =
    [0x0e ] = {   0x08,   0x08,   0x08    },  // Backspace
    [0x0f ] = {   0x09,   0x09,   0x09    },  // Tab
    [0x10 ] = {   _IKBD_ALPHA('q')        },  // Q
    [0x11 ] = {   _IKBD_ALPHA('w')        },  // W
    [0x12 ] = {   _IKBD_ALPHA('e')        },  // E
    [0x13 ] = {   _IKBD_ALPHA('r')        },  // R
    [0x14 ] = {   _IKBD_ALPHA('t')        },  // T
    [0x15 ] = {   _IKBD_ALPHA('y')        },  // Y
    [0x16 ] = {   _IKBD_ALPHA('u')        },  // U
    [0x17 ] = {   _IKBD_ALPHA('i')        },  // I
    [0x18 ] = {   _IKBD_ALPHA('o')        },  // O
    [0x19 ] = {   _IKBD_ALPHA('p')        },  // P
    [0x1a ] = {   '[',    '{',    0x1b    },  // [
    [0x1b ] = {   ']',    '}',    0       },  // ]
    [0x1c ] = {   0x0d,   0x0a,   0       },  // Return
    [0x1e ] = {   _IKBD_ALPHA('a')        },  // A
    [0x1f ] = {   _IKBD_ALPHA('s')        },  // S
    [0x20 ] = {   _IKBD_ALPHA('d')        },  // D
    [0x21 ] = {   _IKBD_ALPHA('f')        },  // F
    [0x22 ] = {   _IKBD_ALPHA('g')        },  // G
    [0x23 ] = {   _IKBD_ALPHA('h')        },  // H
    [0x24 ] = {   _IKBD_ALPHA('j')        },  // J
    [0x25 ] = {   _IKBD_ALPHA('k')        },  // K
    [0x26 ] = {   _IKBD_ALPHA('l')        },  // L
    [0x27 ] = {   ';',    ':',    0       },  // ;
    [0x28 ] = {   '\'',   '"',    0       },  // '
    [0x29 ] = {   '`',    '~',    0       },  // `
    [0x2b ] = {   '\\',   '|',    0       },  // \ (avoid line continuation)
    [0x2c ] = {   _IKBD_ALPHA('z')        },  // Z
    [0x2d ] = {   _IKBD_ALPHA('x')        },  // X
    [0x2e ] = {   _IKBD_ALPHA('c')        },  // C
    [0x2f ] = {   _IKBD_ALPHA('v')        },  // V
    [0x30 ] = {   _IKBD_ALPHA('b')        },  // B
    [0x31 ] = {   _IKBD_ALPHA('n')        },  // N
    [0x32 ] = {   _IKBD_ALPHA('m')        },  // M
    [0x33 ] = {   ',',    '<',    0       },  // ,
    [0x34 ] = {   '.',    '>',    0       },  // .
    [0x35 ] = {   '/',    '?',    0       },  // ?
    [0x39 ] = {   ' ',    ' ',    0       },  // Space
};

static void atarist_ikbd_handle_scancode(char scancode) {
    static bool lshift, rshift, ctrl, caps;
    bool release = scancode & IKBD_RELEASE;
    uint8_t code = scancode & ~IKBD_RELEASE;

    if (release) {
        // some key was released, cancel repeat
        atarist_ikbd_repeat(0);
    }
    switch (code) {
        case IKBD_KEY_LSHIFT:
            lshift = !release;
            break;
        case IKBD_KEY_RSHIFT:
            rshift = !release;
            break;
        case IKBD_KEY_CTRL:
            ctrl = !release;
            break;
        case IKBD_KEY_CAPS:
            caps = !release;
            break;
        default:
            if (!release) {
                // non-modifier was pressed, what does it translate to?
                uint8_t c = ((ctrl) ? ikbd_scan_table[code].ctrl :
                    (lshift | rshift | caps) ? ikbd_scan_table[code].shift :
                    ikbd_scan_table[code].unmod);

                // start repeating it...
                atarist_ikbd_repeat(c);

                // and schedule a keyboard interrupt if appropriate
                if (c == mp_interrupt_char) {
                    mp_sched_keyboard_interrupt();
                }
            }
            break;
    }
}

static void atarist_ikbd_recv(uint8_t c) {
    static uint8_t buf[8];
    static int expect = 1;
    static int index;

    // stash received byte
    buf[index++] = c;

    // first character may indicate a packet, adjust expected count accordingly
    if (index == 1) {
        switch (c) {
            case 0xf6:      // status response
                expect = 8;
                break;
            case 0xf7:      // absolute mouse packet
                expect = 6;
                break;
            case 0xf8 ... 0xfb: // relative mouse packet
                expect = 3;
                break;
            case 0xfc:      // time-of-day report
                expect = 7;
                break;
            case 0xfd:      // combined joystick report
                expect = 3;
                break;
            case 0xfe:
            case 0xff:      // single joystick report
                expect = 2;
                break;
            default:        // regular scancode
                break;
        }
    }

    // handle complete packet
    if (index == expect) {
        switch (buf[0]) {
            case 0xf0:      // power-on / reset ack
                break;
            case 0xf6:      // status response
                break;
            case 0xf7:      // absolute mouse packet
                break;
            case 0xf8 ... 0xfb: // relative mouse packet
                break;
            case 0xfc:      // time-of-day report
                break;
            case 0xfd:      // combined joystick report
                break;
            case 0xfe:
            case 0xff:      // single joystick report
                break;
            default:        // regular scancode
                atarist_ikbd_handle_scancode(c);
                break;
        }
        // packet complete, reset for next header
        index = 0;
        expect = 1;
    }
}

static void atarist_ikbd_handler(void) {
    if (IKBD_CTRL & ACIA_CTRL_INT_REQ) {
        atarist_ikbd_recv(IKBD_DATA);
    }
}

static int atarist_ikbd_rx_chr(void) {
    if (ikbd_rx_head != ikbd_rx_tail) {
        return ikbd_rx_buf[ikbd_rx_tail++ % IKBD_BUF_SIZE];
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////////
// Atari ST video console
//

// framebuffer has to be 256-aligned for ST
__attribute__((aligned(256)))
static uint8_t atarist_framebuffer[32000];

#define _SHIFTER_REG(_adr)  (*(volatile uint8_t *)(_adr))
#define _SHIFTER_WREG(_adr)  (*(volatile uint16_t *)(_adr))

#define SHIFTER_VIDEO_BASE_HIGH     _SHIFTER_REG(0xff8201)
#define SHIFTER_VIDEO_BASE_MID      _SHIFTER_REG(0xff8203)
#define SHIFTER_SYNC_MODE           _SHIFTER_REG(0xff820a)
#define SHIFTER_SYNC_60HZ                        0x00
#define SHIFTER_SYNC_50HZ                        0x02
#define SHIFTER_PALETTE(_x)         _SHIFTER_WREG(0xff8240 + (2 * _x))
#define SHIFTER_RESOLUTION          _SHIFTER_REG(0xff8260)
#define SHIFTER_RESOLUTION_320x200               0x00
#define SHIFTER_RESOLUTION_640x200               0x01
#define SHIFTER_RESOLUTION_640x400               0x02

// https://github.com/rene-d/fontino
static const char font8x8[128][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0000 (uni0000.dup1)
    {0x7e, 0x81, 0xa5, 0x81, 0xbd, 0x99, 0x81, 0x7e},  // 0001 (uni0001)
    {0x7e, 0xff, 0xdb, 0xff, 0xc3, 0xe7, 0xff, 0x7e},  // 0002 (uni0002)
    {0x6c, 0xfe, 0xfe, 0xfe, 0x7c, 0x38, 0x10, 0x00},  // 0003 (uni0003)
    {0x10, 0x38, 0x7c, 0xfe, 0x7c, 0x38, 0x10, 0x00},  // 0004 (uni0004)
    {0x38, 0x7c, 0x38, 0xfe, 0xfe, 0xd6, 0x10, 0x38},  // 0005 (uni0005)
    {0x10, 0x10, 0x38, 0x7c, 0xfe, 0x7c, 0x10, 0x38},  // 0006 (uni0006)
    {0x00, 0x00, 0x18, 0x3c, 0x3c, 0x18, 0x00, 0x00},  // 0007 (uni0007)
    {0xff, 0xff, 0xe7, 0xc3, 0xc3, 0xe7, 0xff, 0xff},  // 0008 (uni0008)
    {0x00, 0x3c, 0x66, 0x42, 0x42, 0x66, 0x3c, 0x00},  // 0009 (uni0009)
    {0xff, 0xc3, 0x99, 0xbd, 0xbd, 0x99, 0xc3, 0xff},  // 000a (uni000A)
    {0x0f, 0x03, 0x05, 0x7d, 0x84, 0x84, 0x84, 0x78},  // 000b (uni000B)
    {0x3c, 0x42, 0x42, 0x42, 0x3c, 0x18, 0x7e, 0x18},  // 000c (uni000C)
    {0x3f, 0x21, 0x3f, 0x20, 0x20, 0x60, 0xe0, 0xc0},  // 000d (uni000D)
    {0x3f, 0x21, 0x3f, 0x21, 0x23, 0x67, 0xe6, 0xc0},  // 000e (uni000E)
    {0x18, 0xdb, 0x3c, 0xe7, 0xe7, 0x3c, 0xdb, 0x18},  // 000f (uni000F)
    {0x80, 0xe0, 0xf8, 0xfe, 0xf8, 0xe0, 0x80, 0x00},  // 0010 (uni0010)
    {0x02, 0x0e, 0x3e, 0xfe, 0x3e, 0x0e, 0x02, 0x00},  // 0011 (uni0011)
    {0x18, 0x3c, 0x7e, 0x18, 0x18, 0x7e, 0x3c, 0x18},  // 0012 (uni0012)
    {0x24, 0x24, 0x24, 0x24, 0x24, 0x00, 0x24, 0x00},  // 0013 (uni0013)
    {0x7f, 0x92, 0x92, 0x72, 0x12, 0x12, 0x12, 0x00},  // 0014 (uni0014)
    {0x3e, 0x63, 0x38, 0x44, 0x44, 0x38, 0xcc, 0x78},  // 0015 (uni0015)
    {0x00, 0x00, 0x00, 0x00, 0x7e, 0x7e, 0x7e, 0x00},  // 0016 (uni0016)
    {0x18, 0x3c, 0x7e, 0x18, 0x7e, 0x3c, 0x18, 0xff},  // 0017 (uni0017)
    {0x10, 0x38, 0x7c, 0x54, 0x10, 0x10, 0x10, 0x00},  // 0018 (uni0018)
    {0x10, 0x10, 0x10, 0x54, 0x7c, 0x38, 0x10, 0x00},  // 0019 (uni0019)
    {0x00, 0x18, 0x0c, 0xfe, 0x0c, 0x18, 0x00, 0x00},  // 001a (uni001A)
    {0x00, 0x30, 0x60, 0xfe, 0x60, 0x30, 0x00, 0x00},  // 001b (uni001B)
    {0x00, 0x00, 0x40, 0x40, 0x40, 0x7e, 0x00, 0x00},  // 001c (uni001C)
    {0x00, 0x24, 0x66, 0xff, 0x66, 0x24, 0x00, 0x00},  // 001d (uni001D)
    {0x00, 0x10, 0x38, 0x7c, 0xfe, 0xfe, 0x00, 0x00},  // 001e (uni001E)
    {0x00, 0xfe, 0xfe, 0x7c, 0x38, 0x10, 0x00, 0x00},  // 001f (uni001F)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0020 (space)
    {0x10, 0x38, 0x38, 0x10, 0x10, 0x00, 0x10, 0x00},  // 0021 (exclam)
    {0x24, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0022 (quotedbl)
    {0x24, 0x24, 0x7e, 0x24, 0x7e, 0x24, 0x24, 0x00},  // 0023 (numbersign)
    {0x18, 0x3e, 0x40, 0x3c, 0x02, 0x7c, 0x18, 0x00},  // 0024 (dollar)
    {0x00, 0x62, 0x64, 0x08, 0x10, 0x26, 0x46, 0x00},  // 0025 (percent)
    {0x30, 0x48, 0x30, 0x56, 0x88, 0x88, 0x76, 0x00},  // 0026 (ampersand)
    {0x10, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0027 (quotesingle)
    {0x10, 0x20, 0x40, 0x40, 0x40, 0x20, 0x10, 0x00},  // 0028 (parenleft)
    {0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20, 0x00},  // 0029 (parenright)
    {0x00, 0x44, 0x38, 0xfe, 0x38, 0x44, 0x00, 0x00},  // 002a (asterisk)
    {0x00, 0x10, 0x10, 0x7c, 0x10, 0x10, 0x00, 0x00},  // 002b (plus)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x20},  // 002c (comma)
    {0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00},  // 002d (hyphen)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x00},  // 002e (period)
    {0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00},  // 002f (slash)
    {0x3c, 0x42, 0x46, 0x4a, 0x52, 0x62, 0x3c, 0x00},  // 0030 (zero)
    {0x10, 0x30, 0x50, 0x10, 0x10, 0x10, 0x7c, 0x00},  // 0031 (one)
    {0x3c, 0x42, 0x02, 0x0c, 0x30, 0x42, 0x7e, 0x00},  // 0032 (two)
    {0x3c, 0x42, 0x02, 0x1c, 0x02, 0x42, 0x3c, 0x00},  // 0033 (three)
    {0x08, 0x18, 0x28, 0x48, 0xfe, 0x08, 0x1c, 0x00},  // 0034 (four)
    {0x7e, 0x40, 0x7c, 0x02, 0x02, 0x42, 0x3c, 0x00},  // 0035 (five)
    {0x1c, 0x20, 0x40, 0x7c, 0x42, 0x42, 0x3c, 0x00},  // 0036 (six)
    {0x7e, 0x42, 0x04, 0x08, 0x10, 0x10, 0x10, 0x00},  // 0037 (seven)
    {0x3c, 0x42, 0x42, 0x3c, 0x42, 0x42, 0x3c, 0x00},  // 0038 (eight)
    {0x3c, 0x42, 0x42, 0x3e, 0x02, 0x04, 0x38, 0x00},  // 0039 (nine)
    {0x00, 0x10, 0x10, 0x00, 0x00, 0x10, 0x10, 0x00},  // 003a (colon)
    {0x00, 0x10, 0x10, 0x00, 0x00, 0x10, 0x10, 0x20},  // 003b (semicolon)
    {0x08, 0x10, 0x20, 0x40, 0x20, 0x10, 0x08, 0x00},  // 003c (less)
    {0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00},  // 003d (equal)
    {0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10, 0x00},  // 003e (greater)
    {0x3c, 0x42, 0x02, 0x04, 0x08, 0x00, 0x08, 0x00},  // 003f (question)
    {0x3c, 0x42, 0x5e, 0x52, 0x5e, 0x40, 0x3c, 0x00},  // 0040 (at)
    {0x18, 0x24, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x00},  // 0041 (A)
    {0x7c, 0x22, 0x22, 0x3c, 0x22, 0x22, 0x7c, 0x00},  // 0042 (B)
    {0x1c, 0x22, 0x40, 0x40, 0x40, 0x22, 0x1c, 0x00},  // 0043 (C)
    {0x78, 0x24, 0x22, 0x22, 0x22, 0x24, 0x78, 0x00},  // 0044 (D)
    {0x7e, 0x22, 0x28, 0x38, 0x28, 0x22, 0x7e, 0x00},  // 0045 (E)
    {0x7e, 0x22, 0x28, 0x38, 0x28, 0x20, 0x70, 0x00},  // 0046 (F)
    {0x1c, 0x22, 0x40, 0x40, 0x4e, 0x22, 0x1e, 0x00},  // 0047 (G)
    {0x42, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42, 0x00},  // 0048 (H)
    {0x38, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00},  // 0049 (I)
    {0x0e, 0x04, 0x04, 0x04, 0x44, 0x44, 0x38, 0x00},  // 004a (J)
    {0x62, 0x24, 0x28, 0x30, 0x28, 0x24, 0x63, 0x00},  // 004b (K)
    {0x70, 0x20, 0x20, 0x20, 0x20, 0x22, 0x7e, 0x00},  // 004c (L)
    {0x63, 0x55, 0x49, 0x41, 0x41, 0x41, 0x41, 0x00},  // 004d (M)
    {0x62, 0x52, 0x4a, 0x46, 0x42, 0x42, 0x42, 0x00},  // 004e (N)
    {0x18, 0x24, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00},  // 004f (O)
    {0x7c, 0x22, 0x22, 0x3c, 0x20, 0x20, 0x70, 0x00},  // 0050 (P)
    {0x3c, 0x42, 0x42, 0x42, 0x4a, 0x3c, 0x03, 0x00},  // 0051 (Q)
    {0x7c, 0x22, 0x22, 0x3c, 0x28, 0x24, 0x72, 0x00},  // 0052 (R)
    {0x3c, 0x42, 0x40, 0x3c, 0x02, 0x42, 0x3c, 0x00},  // 0053 (S)
    {0x7f, 0x49, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00},  // 0054 (T)
    {0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00},  // 0055 (U)
    {0x41, 0x41, 0x41, 0x41, 0x22, 0x14, 0x08, 0x00},  // 0056 (V)
    {0x41, 0x41, 0x41, 0x49, 0x49, 0x49, 0x36, 0x00},  // 0057 (W)
    {0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41, 0x00},  // 0058 (X)
    {0x41, 0x22, 0x14, 0x08, 0x08, 0x08, 0x1c, 0x00},  // 0059 (Y)
    {0x7f, 0x42, 0x04, 0x08, 0x10, 0x21, 0x7f, 0x00},  // 005a (Z)
    {0x78, 0x40, 0x40, 0x40, 0x40, 0x40, 0x78, 0x00},  // 005b (bracketleft)
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00},  // 005c (backslash)
    {0x78, 0x08, 0x08, 0x08, 0x08, 0x08, 0x78, 0x00},  // 005d (bracketright)
    {0x10, 0x28, 0x44, 0x82, 0x00, 0x00, 0x00, 0x00},  // 005e (asciicircum)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff},  // 005f (underscore)
    {0x10, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0060 (grave)
    {0x00, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x3f, 0x00},  // 0061 (a)
    {0x60, 0x20, 0x20, 0x2e, 0x31, 0x31, 0x2e, 0x00},  // 0062 (b)
    {0x00, 0x00, 0x3c, 0x42, 0x40, 0x42, 0x3c, 0x00},  // 0063 (c)
    {0x06, 0x02, 0x02, 0x3a, 0x46, 0x46, 0x3b, 0x00},  // 0064 (d)
    {0x00, 0x00, 0x3c, 0x42, 0x7e, 0x40, 0x3c, 0x00},  // 0065 (e)
    {0x0c, 0x12, 0x10, 0x38, 0x10, 0x10, 0x38, 0x00},  // 0066 (f)
    {0x00, 0x00, 0x3d, 0x42, 0x42, 0x3e, 0x02, 0x7c},  // 0067 (g)
    {0x60, 0x20, 0x2c, 0x32, 0x22, 0x22, 0x62, 0x00},  // 0068 (h)
    {0x10, 0x00, 0x30, 0x10, 0x10, 0x10, 0x38, 0x00},  // 0069 (i)
    {0x02, 0x00, 0x06, 0x02, 0x02, 0x42, 0x42, 0x3c},  // 006a (j)
    {0x60, 0x20, 0x24, 0x28, 0x30, 0x28, 0x26, 0x00},  // 006b (k)
    {0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00},  // 006c (l)
    {0x00, 0x00, 0x76, 0x49, 0x49, 0x49, 0x49, 0x00},  // 006d (m)
    {0x00, 0x00, 0x5c, 0x62, 0x42, 0x42, 0x42, 0x00},  // 006e (n)
    {0x00, 0x00, 0x3c, 0x42, 0x42, 0x42, 0x3c, 0x00},  // 006f (o)
    {0x00, 0x00, 0x6c, 0x32, 0x32, 0x2c, 0x20, 0x70},  // 0070 (p)
    {0x00, 0x00, 0x36, 0x4c, 0x4c, 0x34, 0x04, 0x0e},  // 0071 (q)
    {0x00, 0x00, 0x6c, 0x32, 0x22, 0x20, 0x70, 0x00},  // 0072 (r)
    {0x00, 0x00, 0x3e, 0x40, 0x3c, 0x02, 0x7c, 0x00},  // 0073 (s)
    {0x10, 0x10, 0x7c, 0x10, 0x10, 0x12, 0x0c, 0x00},  // 0074 (t)
    {0x00, 0x00, 0x42, 0x42, 0x42, 0x46, 0x3a, 0x00},  // 0075 (u)
    {0x00, 0x00, 0x41, 0x41, 0x22, 0x14, 0x08, 0x00},  // 0076 (v)
    {0x00, 0x00, 0x41, 0x49, 0x49, 0x49, 0x36, 0x00},  // 0077 (w)
    {0x00, 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00},  // 0078 (x)
    {0x00, 0x00, 0x42, 0x42, 0x42, 0x3e, 0x02, 0x7c},  // 0079 (y)
    {0x00, 0x00, 0x7c, 0x08, 0x10, 0x20, 0x7c, 0x00},  // 007a (z)
    {0x0c, 0x10, 0x10, 0x60, 0x10, 0x10, 0x0c, 0x00},  // 007b (braceleft)
    {0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x10, 0x00},  // 007c (bar)
    {0x30, 0x08, 0x08, 0x06, 0x08, 0x08, 0x30, 0x00},  // 007d (braceright)
    {0x32, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 007e (asciitilde)
    {0x00, 0x08, 0x14, 0x22, 0x41, 0x41, 0x7f, 0x00},  // 007f (uni007F)
};

#define VID_HOR_RES 640                                     // X resolution
#define VID_VERT_RES 400                                    // Y resolution
#define VID_FONT_WIDTH 8                                    // font cell width
#define VID_FONT_HEIGHT 8                                   // font cell height
#define VID_NUM_COL (VID_HOR_RES / VID_FONT_WIDTH)          // number of text columns
#define VID_NUM_ROW (VID_VERT_RES / VID_FONT_HEIGHT)        // number of text rows
#define VID_LINE_STRIDE (VID_HOR_RES / VID_FONT_WIDTH)      // stride between lines in a character cell
#define VID_ROW_STRIDE (VID_LINE_STRIDE * VID_FONT_HEIGHT)  // stride between character cell rows

static struct {
    uint8_t curs_row;
    uint8_t curs_col;
    volatile bool cursor_on;
    volatile bool cursor_enabled;
} atarist_vid_state;

static void atarist_vid_cursor_toggle(void) {
    // Only draw the cursor if it's on the screen
    if ((atarist_vid_state.curs_row < VID_NUM_ROW) && (atarist_vid_state.curs_col < VID_NUM_COL)) {
        uint8_t *fb_data = &atarist_framebuffer[atarist_vid_state.curs_row * VID_ROW_STRIDE + atarist_vid_state.curs_col];

        for (int i = 0; i < VID_FONT_HEIGHT; i++) {
            *fb_data = ~*fb_data;
            fb_data += VID_LINE_STRIDE;
        }
        atarist_vid_state.cursor_on = !atarist_vid_state.cursor_on;
    }
}

static void atarist_vid_cursor_tick(void) {
    if (atarist_vid_state.cursor_enabled) {
        static uint16_t prescale;

        if (prescale == 0) {
            atarist_vid_cursor_toggle();
            prescale = 40;
        } else {
            prescale--;
        }
    }
}

static void atarist_vid_scroll(void) {
    uint8_t *src = &atarist_framebuffer[VID_ROW_STRIDE];
    uint8_t *dst = &atarist_framebuffer[0];

    memmove(dst, src, (VID_NUM_ROW - 1) * VID_ROW_STRIDE);

    uint8_t *last_row = &atarist_framebuffer[(VID_NUM_ROW - 1) * VID_ROW_STRIDE];
    memset(last_row, 0, VID_ROW_STRIDE);
}

static void atarist_vid_draw_char(uint8_t row, uint8_t col, uint8_t c) {
    // Only draw the character if it's on the screen
    if ((row < VID_NUM_ROW) && (col < VID_NUM_COL)) {
        uint8_t *fb_data = &atarist_framebuffer[row * VID_ROW_STRIDE + col];

        for (int i = 0; i < VID_FONT_HEIGHT; i++) {
            *fb_data = font8x8[c][i];
            fb_data += VID_LINE_STRIDE;
        }
    }
}

static void atarist_vid_raw_handler(uint8_t c) {
    switch (c) {
        case '\b':
            if (atarist_vid_state.curs_col > 0) {
                atarist_vid_state.curs_col--;
            } else if (atarist_vid_state.curs_row > 0) {
                atarist_vid_state.curs_row--;
                atarist_vid_state.curs_col = VID_NUM_COL - 1;
            }
            break;

        case '\r':
            atarist_vid_state.curs_col = 0;
            break;

        case '\n':
            if (atarist_vid_state.curs_row < (VID_NUM_ROW - 1)) {
                atarist_vid_state.curs_row += 1;
            } else {
                atarist_vid_scroll();
            }
            break;

        default:
            if (c < 128) {
                uint8_t row = atarist_vid_state.curs_row;
                uint8_t col = atarist_vid_state.curs_col;

                atarist_vid_draw_char(row, col, c);
                if (++col >= VID_NUM_COL) {
                    col = 0;
                    if (row < (VID_NUM_ROW - 1)) {
                        row++;
                    } else {
                        atarist_vid_scroll();
                    }
                }
                atarist_vid_state.curs_row = row;
                atarist_vid_state.curs_col = col;
            }
            break;
    }
}


// number of CSI parameters we handle
#define NUM_CSI_PARAM 2

static bool atarist_vid_ansi_handler(uint8_t c) {
    static enum {
        P_IDLE,
        P_ESC,
        P_CSI
    } phase;
    static unsigned param[NUM_CSI_PARAM];
    static int param_index;

    switch (phase) {
        case P_IDLE:
            // escape?
            if (c == '\x1b') {
                phase = P_ESC;
                return true;
            }
            break;
        case P_ESC:
            // CSI?
            if (c == '[') {
                phase = P_CSI;
                for (int i = 0; i < NUM_CSI_PARAM; i++) {
                    param[i] = 0;
                    param_index = 0;
                }
                return true;
            }
            break;
        case P_CSI:
            if ((c >= 0x30) && (c <= 0x3f)) {
                // parameter character
                if (param_index < NUM_CSI_PARAM) {
                    if (c == ';') {
                        param_index++;
                    } else {
                        param[param_index] *= 10;
                        param[param_index] += c - '0';
                    }
                }
                return true;
            } else if ((c >= 0x40) && (c <= 0x7e)) {
                // operation character
                switch (c) {
                    case 'D':        // cursor back n
                        if (param[0] == 0) {
                            param[0] = 1;
                        }
                        while (param[0]--) {
                            atarist_vid_raw_handler('\b');
                        }
                        break;
                    case 'K':        // clear from cursor to end of line
                        for (int i = atarist_vid_state.curs_col; i < VID_NUM_COL; i++) {
                            atarist_vid_draw_char(atarist_vid_state.curs_row, i, ' ');
                        }
                        break;
                    default:
                        printf("\nCSI %c?\n", c);
                        break;
                }
                phase = P_IDLE;
                return true;
            }
            break;
        default:
            break;
    }
    phase = P_IDLE;
    return false;
}

static void atarist_vid_tx_chr(uint8_t c) {

    // save the current cursor state and un-draw it if it's currently drawn,
    // as handlers may move the cursor...
    bool cursor_enabled = atarist_vid_state.cursor_enabled;
    atarist_vid_state.cursor_enabled = false;
    if (atarist_vid_state.cursor_on) {
        atarist_vid_cursor_toggle();
    }

    // give the ANSI seuqence handler a shot at the character first
    if (!atarist_vid_ansi_handler(c)) {
        // handle the character 'raw'
        atarist_vid_raw_handler(c);
    }

    // restore cursor state, let the ticker re-draw it
    atarist_vid_state.cursor_enabled = cursor_enabled;
}

static void atarist_video_init(void) {

    // On ST, important to do init while nothing is displayed (i.e. during
    // the VBI). GLUE can want two VSYNCs before it works properly, so just
    // wait for a few out of an excess of caution.
    atarist_wait_vsync(3);

    // init 640x400 mono mode
    SHIFTER_RESOLUTION = SHIFTER_RESOLUTION_640x400;
    SHIFTER_SYNC_MODE = SHIFTER_SYNC_60HZ;
    SHIFTER_PALETTE(0) = 0x777;
    SHIFTER_PALETTE(1) = 0x000;
    SHIFTER_VIDEO_BASE_HIGH = (((uintptr_t)atarist_framebuffer) >> 16) & 0xff;
    SHIFTER_VIDEO_BASE_MID = (((uintptr_t)atarist_framebuffer) >> 8) & 0xff;

    atarist_vid_state.cursor_enabled = true;
}

////////////////////////////////////////////////////////////////////////////////
// Atari ST horizontal / vertical blanking interrupts
//

static volatile uint32_t atarist_vsync_count;

static void atarist_wait_vsync(uint32_t count) {
    uint16_t old_sr = m68k_get_sr();

    atarist_vsync_count = count;
    m68k_set_sr(0x2300);
    while (atarist_vsync_count > 0) {
        ;
    }
    m68k_set_sr(old_sr);
}

M68K_INTERRUPT_HANDLER(26, atarist_hsync) {
}

M68K_INTERRUPT_HANDLER(28, atarist_vsync)
{
    if (atarist_vsync_count > 0) {
        atarist_vsync_count--;
    }
}
