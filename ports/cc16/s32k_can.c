
#include "py/mphal.h"

#include "s32k144.h"

void
s32k_can_early_init(void)
{
    // configure CAN for 500kBps
    PCC->PCC_FlexCAN0 |= PCC_PCC_FlexCAN0_CGC;  // clock on
    CAN0->CTRL1 |= CAN_CTRL1_CLKSRC;            // select 80Mhz clock
    CAN0->MCR |= CAN_MCR_FRZ;                   // enable freeze mode
    CAN0->MCR &= ~CAN_MCR_MDIS;                 // clear disable bit
    while(CAN0->MCR & CAN_MCR_LPMACK);          // ... and wait for block to leave low-power mode
    CAN0->MCR ^= CAN_MCR_SOFTRST;               // request soft reset
    while(CAN0->MCR & CAN_MCR_SOFTRST);         // ... and wait for it to finish
    while(!(CAN0->MCR & CAN_MCR_FRZACK));       // ... leaving frozen mode
    CAN0->MCR |= CAN_MCR_IRMQ   |               // new-style masking
                 CAN_MCR_SRXDIS |               // disable self-reception
                 CAN_MCR_RFEN   |               // enable receive FIFO
                 CAN_MCR_IDAM(0);               // 32-bit acceptance filters
    CAN0->CBT = CAN_CBT_BTF             |       // configure for 500kBPS @ 80MHz
                CAN_CBT_ERJW(1)         |
                CAN_CBT_EPRESDIV(0x09)  |
                CAN_CBT_EPROPSEG(0x07)  |
                CAN_CBT_EPSEG1(0x04)    |
                CAN_CBT_EPSEG2(0x01);

    CAN0->IFLAG1 |= CAN_IFLAG1_BUF5I;           // clear FIFO rx interrupt
    CAN0->IMASK1 |= CAN_IFLAG1_BUF5I;           // enable FIFO rx interrupt
    CAN0->RAMn24 = (1U << 30) | (0x1ffffffd << 1); // FIFO filter 0 matches console input
    CAN0->RXIMR0 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    CAN0->RAMn25 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 1 matches nothing
    CAN0->RXIMR1 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    CAN0->RAMn26 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 2 matches nothing
    CAN0->RXIMR2 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    CAN0->RAMn27 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 3 matches nothing
    CAN0->RXIMR3 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    CAN0->RAMn28 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 4 matches nothing
    CAN0->RXIMR4 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    CAN0->RAMn29 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 5 matches nothing
    CAN0->RXIMR5 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    CAN0->RAMn30 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 6 matches nothing
    CAN0->RXIMR6 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    CAN0->RAMn31 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 7 matches nothing
    CAN0->RXIMR7 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit

    CAN0->MCR &= ~CAN_MCR_HALT;                 // disable halt mode
    while (CAN0->MCR & CAN_MCR_NOTRDY);         // ... and wait for it to take effect

    CAN0->RAMn32 = 0x08000000;                  // mark console TX buffer as idle
    CAN0->RAMn36 = 0x08000000;                  // mark unused TX buffer as idle
    CAN0->RAMn40 = 0x08000000;                  // mark unused TX buffer as idle
    CAN0->RAMn44 = 0x08000000;                  // mark unused TX buffer as idle
    CAN0->RAMn48 = 0x08000000;                  // mark unused TX buffer as idle
    CAN0->RAMn52 = 0x08000000;                  // mark unused TX buffer as idle
    CAN0->RAMn56 = 0x08000000;                  // mark unused TX buffer as idle
    CAN0->RAMn60 = 0x08000000;                  // mark unused TX buffer as idle

    NVIC_EnableIRQ(CAN0_ORed_0_15_MB_IRQn);
}

// Send string of given length to stdout.
//
void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    while (len) {
        size_t dlc = (len >= 8) ? 8 : len;

        while ((CAN0->RAMn32 & 0x0f000000) != 0x08000000);

        CAN0->RAMn33 = 0x1ffffffe;
        CAN0->RAMn34 = ((len > 0) ? (str[0] << 24) : 0) |
                       ((len > 1) ? (str[1] << 16) : 0) |
                       ((len > 2) ? (str[2] <<  8) : 0) |
                       ((len > 3) ? (str[3] <<  0) : 0);
        CAN0->RAMn35 = ((len > 4) ? (str[4] << 24) : 0) |
                       ((len > 5) ? (str[5] << 16) : 0) |
                       ((len > 6) ? (str[6] <<  8) : 0) |
                       ((len > 7) ? (str[7] <<  0) : 0);
        CAN0->RAMn32 = (0xc << 24) | (1U << 21) | (dlc << 16);

        len -= dlc;
        str += dlc;
    }
}

static volatile struct {
    char    buf[64];
    uint8_t head, tail;
} cons_buf;

#define CONS_PTR_NEXT(_p)   ((_p + 1) % sizeof(cons_buf.buf))
#define CONS_BUF_FULL       (CONS_PTR_NEXT(cons_buf.tail) == cons_buf.head)
#define CONS_BUF_EMPTY      (cons_buf.head == cons_buf.tail)
#define CONS_BUF_PUSH(_v)                                   \
    if (!CONS_BUF_FULL) {                                   \
        cons_buf.buf[cons_buf.tail] = _v;                   \
        cons_buf.tail = CONS_PTR_NEXT(cons_buf.tail);       \
    }
#define CONS_BUF_POP(_v)                                    \
    if (CONS_BUF_EMPTY) {                                   \
        _v = -1;                                            \
    } else {                                                \
        _v = cons_buf.buf[cons_buf.head];                   \
        cons_buf.head = CONS_PTR_NEXT(cons_buf.head);       \
    }

void CAN0_ORed_0_15_MB_Handler(void) {

    while (CAN0->IFLAG1 & CAN_IFLAG1_BUF5I) {
        uint32_t u32[4] = {
            CAN0->RAMn0,   
            CAN0->RAMn1,   
            CAN0->RAMn2,   
            CAN0->RAMn3,   
        };
        CAN0->IFLAG1 |= CAN_IFLAG1_BUF5I;

        if ((u32[0] & (1U << 21)) &&
            ((u32[1] & 0x1fffffff) == 0x1ffffffd)) {
            uint8_t dlc = (u32[0] >> 16) & 0xf;
            if (dlc > 0) {
                CONS_BUF_PUSH((u32[2] >> 24) & 0xff);
                if (dlc > 1) {
                    CONS_BUF_PUSH((u32[2] >> 16) & 0xff);
                    if (dlc > 2) {
                        CONS_BUF_PUSH((u32[2] >> 8) & 0xff);
                        if (dlc > 3) {
                            CONS_BUF_PUSH((u32[2] >> 0) & 0xff);
                            if (dlc > 4) {
                                CONS_BUF_PUSH((u32[3] >> 24) & 0xff);
                                if (dlc > 5) {
                                    CONS_BUF_PUSH((u32[3] >> 16) & 0xff);
                                    if (dlc > 6) {
                                        CONS_BUF_PUSH((u32[3] >> 8) & 0xff);
                                        if (dlc > 7) {
                                            CONS_BUF_PUSH((u32[3] >> 0) & 0xff);
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

// Get a single character, blocking until one is available
//
int mp_hal_stdin_rx_chr(void) {
    int c;

    do {
        CONS_BUF_POP(c);
    } while (c == -1);
    return c;
}

