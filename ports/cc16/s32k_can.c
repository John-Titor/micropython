// CAN support for the CC16
//
// Mailbox allocation:
//
// 0-7: RX FIFO and 8 FIFO mask registers
// 8: console RX mailbox
// 9: console TX mailbox
// 10-15 / 10-31: free for buffered tx?

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/stream.h"

#include "s32k144.h"

typedef union {
    struct {
        uint32_t    timestamp : 16;
        uint32_t    dlc : 4;
        uint32_t    rtr : 1;
        uint32_t    ide : 1;
        uint32_t    srr : 1;
        uint32_t    : 1;
        uint32_t    code : 4;
        uint32_t    : 1;
        uint32_t    esi : 1;
        uint32_t    brs : 1;
        uint32_t    edl : 1;

        union {
            struct {
                uint32_t    id_ext : 29;
                uint32_t    prio : 3;
            };
            struct {
                uint32_t    : 18;
                uint32_t    id : 11;
                uint32_t    : 3;
            };
        };
        uint8_t     data3;
        uint8_t     data2;
        uint8_t     data1;
        uint8_t     data0;
        uint8_t     data7;
        uint8_t     data6;
        uint8_t     data5;
        uint8_t     data4;
    };
    uint32_t    u32[4];    
} Mailbox_t;

enum {
    // RX mailbox code values
    RX_INACTIVE = 0x0,
    RX_EMPTY    = 0x4,
    RX_FULL     = 0x2,
    RX_OVERRUN  = 0x6,
    RX_RANSWER  = 0xa,
    RX_BUSY     = 0x1,

    // TX mailbox code values
    TX_INACTIVE = 0x8,
    TX_ABORT    = 0x9,
    TX_DATA     = 0xc,
    TX_TANSWER  = 0xe
};

#define RX_FIFO_INTERRUPT       CAN_IFLAG1_BUF5I
#define RX_CONS_INTERRUPT       (1U << 8)

#define MBOX_ADDR(_can, _index) (&(_can)->RAMn0 + (4 * (_index)))
#define MBOX_FIFO(_can)         MBOX_ADDR(_can, 0)
#define MBOX_CONS_RECV(_can)    MBOX_ADDR(_can, 8)
#define MBOX_CONS_SEND(_can)    MBOX_ADDR(_can, 9)

static void _can_configure(volatile CAN_regs_t *can) {

    // configure CAN for 500kBps
    can->CTRL1 |= CAN_CTRL1_CLKSRC;            // select 80Mhz clock
    can->MCR |= CAN_MCR_FRZ;                   // enable freeze mode
    can->MCR &= ~CAN_MCR_MDIS;                 // clear disable bit
    while (can->MCR & CAN_MCR_LPMACK) {
        ;                                       // ... and wait for block to leave low-power mode
    }
    can->MCR ^= CAN_MCR_SOFTRST;               // request soft reset
    while (can->MCR & CAN_MCR_SOFTRST) {
        ;                                       // ... and wait for it to finish
    }
    while (!(can->MCR & CAN_MCR_FRZACK)) {
        ;                                       // ... entering frozen mode
    }
    can->MCR |= CAN_MCR_IRMQ |                 // new-style masking
        CAN_MCR_SRXDIS |                        // disable self-reception
        CAN_MCR_RFEN |                          // enable receive FIFO
        CAN_MCR_IDAM(0);                        // 32-bit acceptance filters
    can->CBT = CAN_CBT_BTF |                   // configure for 500kBPS @ 80MHz
        CAN_CBT_ERJW(1) |
        CAN_CBT_EPRESDIV(0x09) |
        CAN_CBT_EPROPSEG(0x07) |
        CAN_CBT_EPSEG1(0x04) |
        CAN_CBT_EPSEG2(0x01);
    can->CTRL2 |= CAN_CTRL2_MRP;               // prefer individual mailboxes over FIFO for rx

    // clear and enable interrupts
    can->IFLAG1 |= RX_FIFO_INTERRUPT | RX_CONS_INTERRUPT;
    can->IMASK1 |= RX_FIFO_INTERRUPT | RX_CONS_INTERRUPT;

    // set initial RX FIFO masks
    can->RAMn24 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 0 matches nothing
    can->RXIMR0 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn25 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 1 matches nothing
    can->RXIMR1 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn26 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 2 matches nothing
    can->RXIMR2 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn27 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 3 matches nothing
    can->RXIMR3 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn28 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 4 matches nothing
    can->RXIMR4 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn29 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 5 matches nothing
    can->RXIMR5 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn30 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 6 matches nothing
    can->RXIMR6 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn31 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 7 matches nothing
    can->RXIMR7 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit

    if (can == CAN0) {
        // set console RX mask for mailbox 8
        can->RXIMR8 = 0x1fffffff;                  // console RX buffer mask
        can->RAMn33 = 0x1ffffffd;                  // console RX buffer ID
    }

    can->MCR &= ~CAN_MCR_HALT;                      // disable halt mode
    while (can->MCR & CAN_MCR_NOTRDY) {
        ;                                           // ... and wait for it to take effect
    }

    if (can == CAN0) {
        can->RAMn32 = (RX_EMPTY << 24) | (1 << 21); // mark console RX buffer as idle
    } else {
        can->RAMn32 = RX_INACTIVE    << 24;         // mark unused buffer as idle
    }
    can->RAMn36 = TX_INACTIVE << 24;                // mark console TX buffer as idle
    can->RAMn40 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
    can->RAMn44 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
    can->RAMn48 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
    can->RAMn52 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
    can->RAMn56 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
    can->RAMn60 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
   
}

void s32k_can_configure(void) {

    PCC->PCC_FlexCAN0 |= PCC_PCC_FlexCAN0_CGC;  // clock on
    PCC->PCC_FlexCAN1 |= PCC_PCC_FlexCAN1_CGC;  // clock on

    _can_configure(CAN0);
    _can_configure(CAN1);

    NVIC_EnableIRQ(CAN0_ORed_0_15_MB_IRQn);
    NVIC_EnableIRQ(CAN1_ORed_0_15_MB_IRQn);
}

// Send string of given length to stdout.
//
void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    while (len) {
        size_t dlc = (len >= 8) ? 8 : len;

        while ((CAN0->RAMn36 & 0x0f000000) != 0x08000000) {
            ;                                               // wait for TX buffer

        }
        CAN0->RAMn37 = 0x1ffffffe;                          // CAN stdout ID
        CAN0->RAMn38 = ((len > 0) ? (str[0] << 24) : 0) |   // pack bytes big-endian
            ((len > 1) ? (str[1] << 16) : 0) |
            ((len > 2) ? (str[2] << 8) : 0) |
            ((len > 3) ? (str[3] << 0) : 0);
        CAN0->RAMn39 = ((len > 4) ? (str[4] << 24) : 0) |
            ((len > 5) ? (str[5] << 16) : 0) |
            ((len > 6) ? (str[6] << 8) : 0) |
            ((len > 7) ? (str[7] << 0) : 0);
        CAN0->RAMn36 = (0xc << 24) | (1U << 21) | (dlc << 16); // start transmission

        len -= dlc;
        str += dlc;
    }
}

// CAN console input buffer.
//
static volatile struct {
    char buf[128];
    uint8_t head, tail;
    int interrupt;
} cons_buf;

#define CONS_PTR_NEXT(_p)   ((_p + 1) % sizeof(cons_buf.buf))
#define CONS_BUF_FULL       (CONS_PTR_NEXT(cons_buf.tail) == cons_buf.head)
#define CONS_BUF_EMPTY      (cons_buf.head == cons_buf.tail)
#define CONS_BUF_POP(_v)                                    \
    if (CONS_BUF_EMPTY) {                                   \
        _v = -1;                                            \
    } else {                                                \
        _v = cons_buf.buf[cons_buf.head];                   \
        cons_buf.head = CONS_PTR_NEXT(cons_buf.head);       \
    }

// Receive a console byte
//
static inline void cons_buf_push(char c) {
    if (c == cons_buf.interrupt) {
        mp_sched_keyboard_interrupt();
    } else if (!CONS_BUF_FULL) {
        cons_buf.buf[cons_buf.tail] = c;
        cons_buf.tail = CONS_PTR_NEXT(cons_buf.tail);
    }
}

void mp_hal_set_interrupt_char(int c) {
    cons_buf.interrupt = c;
}

// CAN interrupt handlers
//

static void _can_interrupt(volatile CAN_regs_t *can) {

    if (can == CAN0) {
        while (can->IFLAG1 & RX_CONS_INTERRUPT) {
            volatile uint32_t *RAMn = MBOX_CONS_RECV(can);
            Mailbox_t mbox = { .u32 = { RAMn[0], RAMn[1], RAMn[2], RAMn[3]} };

            can->IFLAG1 = RX_CONS_INTERRUPT;

            if (mbox.dlc > 0) {
                cons_buf_push(mbox.data0);
                if (mbox.dlc > 1) {
                    cons_buf_push(mbox.data1);
                    if (mbox.dlc > 2) {
                        cons_buf_push(mbox.data2);
                        if (mbox.dlc > 3) {
                            cons_buf_push(mbox.data3);
                            if (mbox.dlc > 4) {
                                cons_buf_push(mbox.data4);
                                if (mbox.dlc > 5) {
                                    cons_buf_push(mbox.data5);
                                    if (mbox.dlc > 6) {
                                        cons_buf_push(mbox.data6);
                                        if (mbox.dlc > 7) {
                                            cons_buf_push(mbox.data7);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void CAN0_ORed_0_15_MB_Handler(void) {
    _can_interrupt(CAN0);
}

void CAN1_ORed_0_15_MB_Handler(void) {
    _can_interrupt(CAN1);
}

// Get a single character, blocking until one is available
//
int mp_hal_stdin_rx_chr(void) {
    int c;

    do {
        CONS_BUF_POP(c);
    } while (c == -1);
    return c;
}

// Check for an available byte
//
uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    if (!CONS_BUF_EMPTY) {
        ret |= MP_STREAM_POLL_RD;
    }
    return ret;
}
