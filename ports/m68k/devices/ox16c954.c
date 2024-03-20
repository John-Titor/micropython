/*
 * Driver for the ox16c954 quad UART that shows up in a number of
 * m68k retro projects.
 */

#include <py/mpconfig.h>

#include "mphalport.h"

#ifndef QUART_VECTOR
# error QUART_VECTOR must be defined.
#endif
#ifndef QUART_STRIDE
# error QUART_STRIDE must be defined.
#endif
#ifndef QUART_BASE
# error QUART_BASE must be defined.
#endif

#define QUART_NUM_CHANNELS          4

/* normally wired with all 4 channels adjacent */
#define _QUART_CHAN(_x)             (QUART_BASE + ((_x) * 8 * QUART_STRIDE))

/* PIO registers */
#define _QUART_REG(_chan, _addr)    (* (volatile uint8_t *)(_QUART_CHAN(_chan) + ((_addr) * QUART_STRIDE)))
#define QUART_THR(_chan)            _QUART_REG(_chan, 0)
#define QUART_RHR(_chan)            _QUART_REG(_chan, 0)
#define QUART_DLL(_chan)            _QUART_REG(_chan, 0)
#define QUART_IER(_chan)            _QUART_REG(_chan, 1)
#define QUART_DLM(_chan)            _QUART_REG(_chan, 1)
#define QUART_ASR(_chan)            _QUART_REG(_chan, 1)
#define QUART_FCR(_chan)            _QUART_REG(_chan, 2)
#define QUART_ISR(_chan)            _QUART_REG(_chan, 2)
#define QUART_EFR(_chan)            _QUART_REG(_chan, 2)
#define QUART_LCR(_chan)            _QUART_REG(_chan, 3)
#define QUART_MCR(_chan)            _QUART_REG(_chan, 4)
#define QUART_LSR(_chan)            _QUART_REG(_chan, 5)
#define LSR_THRE                        (1U<<5)
#define LSR_RXRDY                       (1U<<0)
#define QUART_ICR(_chan)            _QUART_REG(_chan, 5)
#define QUART_MSR(_chan)            _QUART_REG(_chan, 6)
#define QUART_SPR(_chan)            _QUART_REG(_chan, 7)

/* indexed registers */
#define QUART_ACR                   (0)
#define QUART_CPR                   (1)
#define QUART_TCR                   (2)
#define QUART_ID1                   (8)
#define QUART_ID2                   (9)
#define QUART_ID3                   (10)

#define RX_BUF_SIZE     256
static struct {
    char buf[RX_BUF_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} quart_rx[QUART_NUM_CHANNELS];

#ifdef QUART_CONSOLE_CHANNEL

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        for (;;) {
            if (QUART_LSR(QUART_CONSOLE_CHANNEL) & LSR_THRE) {
                QUART_THR(QUART_CONSOLE_CHANNEL) = *str++;
                break;
            }
        }
    }
}

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
#if 0
    for (;;) {
        if (duart_rx_head != duart_rx_tail) {
            c = duart_rx_buf[duart_rx_tail++ % RX_BUF_SIZE];
            break;
        }
    }
#else
    for (;;) {
        if (QUART_LSR(QUART_CONSOLE_CHANNEL) & LSR_RXRDY) {
            c = QUART_RHR(QUART_CONSOLE_CHANNEL);
            break;
        }
    }
#endif

    return c;
}

#endif // QUART_CONSOLE_CHANNEL

void ox16c954_init()
{
    // XXX
}

M68K_INTERRUPT_HANDLER(QUART_VECTOR, quart_handler) {
    for (int i = 0; i < QUART_NUM_CHANNELS; i++) {
        if (QUART_LSR(i) & LSR_RXRDY) {
            if ((quart_rx[i].head - quart_rx[i].tail) < RX_BUF_SIZE) {
                quart_rx[i].buf[quart_rx[i].head++ % RX_BUF_SIZE] = QUART_RHR(i);
            } else {
                // XXX mask interrupt & let FIFO handle it
            }
        }
    }
}
