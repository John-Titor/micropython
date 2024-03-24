/*
 * Driver for the ox16c954 quad UART that shows up in a number of
 * m68k retro projects.
 */

#include <py/mpconfig.h>
#include <py/runtime.h>
#include <mphalport.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef QUART_CLOCK
# error QUART_CLOCK must be defined.
#endif
#ifndef QUART_STRIDE
# error QUART_STRIDE must be defined.
#endif
#ifndef QUART_BASE
# error QUART_BASE must be defined.
#endif

#define QUART_OVERSAMPLE            16      /* fixed for now */

#define QUART_NUM_CHANNELS          4
#define QUART_PRE_SCALE             8       /* 5.3 fixed-point prescaler */

/* normally wired with all 4 channels adjacent */
#define _QUART_CHAN(_x)             (QUART_BASE + ((_x) * 8 * QUART_STRIDE))

/* PIO registers */
#define _QUART_REG(_chan, _addr)    (* (volatile uint8_t *)(_QUART_CHAN(_chan) + ((_addr) * QUART_STRIDE)))
#define QUART_THR(_chan)            _QUART_REG(_chan, 0)
#define QUART_RHR(_chan)            _QUART_REG(_chan, 0)
#define QUART_DLL(_chan)            _QUART_REG(_chan, 0)
#define QUART_IER(_chan)            _QUART_REG(_chan, 1)
#define IER_RXRDY                       (1U<<0)
#define QUART_DLM(_chan)            _QUART_REG(_chan, 1)
#define QUART_ASR(_chan)            _QUART_REG(_chan, 1)
#define QUART_FCR(_chan)            _QUART_REG(_chan, 2)
#define QUART_ISR(_chan)            _QUART_REG(_chan, 2)
#define QUART_EFR(_chan)            _QUART_REG(_chan, 2)
#define QUART_LCR(_chan)            _QUART_REG(_chan, 3)
#define QUART_MCR(_chan)            _QUART_REG(_chan, 4)
#define MCR_INTEN                       (1U<<3)
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

#ifdef QUART_CONSOLE_CHANNEL

static char cons_buf[256];
volatile uint16_t cons_head;
volatile uint16_t cons_tail;
#define CONS_BUF_SIZE   sizeof(cons_buf)

// Send string of given length to console
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

// Receive single character from console
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
    for (;;) {
        if (cons_head != cons_tail) {
            c = cons_buf[cons_tail++ % CONS_BUF_SIZE];
            QUART_IER(QUART_CONSOLE_CHANNEL) |= IER_RXRDY;
            break;
        }
    }
    return c;
}

#endif // QUART_CONSOLE_CHANNEL

#if 0
static uint32_t isqrt(uint32_t n) {
    uint32_t x = n / 2;
    for (;;) {
        uint32_t prev_x = x;
        x = (x + n / x) / 2;
        if (abs(x - prev_x) <= 1) {
            return ((x * x) > n) ? x - 1 : x;
        }
    }
}

struct br_calc_state {
    uint32_t    target_rate;
    int         error;
    uint16_t    divisor;
    uint8_t     prescaler;
};

static void br_config_evaluate(struct br_calc_state *cs, uint16_t divisor, uint8_t prescaler) {
    uint32_t actual_rate = (QUART_CLOCK * QUART_PRE_SCALE) / (16 * divisor * prescaler);
    int error = abs(cs->target_rate - actual_rate);
    if (error < cs->error) {
        cs->error = error;
        cs->divisor = divisor;
        cs->prescaler = prescaler;
    }
}

static bool br_config_calculate(struct br_calc_state *cs) {
    uint32_t n = (QUART_CLOCK * QUART_PRE_SCALE) / (cs->target_rate * QUART_OVERSAMPLE);
    uint32_t lf = isqrt(n);
    while (lf > 0) {
        uint32_t hf = n / lf;
        if ((lf >= QUART_PRE_SCALE) && (lf < 256)) {
            br_config_evaluate(cs, hf, lf);
            br_config_evaluate(cs, hf + 1, lf);
        } else if ((lf >= QUART_PRE_SCALE) && (lf < 255)) {
            br_config_evaluate(cs, lf, hf);
            br_config_evaluate(cs, lf, hf + 1);
        }
        lf -= 1;
    }
    return cs->divisor != 0;
}
#endif

void ox16c954_init()
{
#ifdef QUART_CONSOLE_CHANNEL
    QUART_IER(QUART_CONSOLE_CHANNEL) = IER_RXRDY;
    QUART_MCR(QUART_CONSOLE_CHANNEL) |= MCR_INTEN;
#endif
}

void ox16c954_handler() {
    for (int channel = 0; channel < QUART_NUM_CHANNELS; channel++) {
        if (QUART_LSR(channel) & LSR_RXRDY) {
#ifdef QUART_CONSOLE_CHANNEL
            if (channel == QUART_CONSOLE_CHANNEL) {
                if ((cons_head - cons_tail) < CONS_BUF_SIZE) {
                    char c = QUART_RHR(channel);
                    cons_buf[cons_head++ % CONS_BUF_SIZE] = c;
                    if (c == mp_interrupt_char) {
                        mp_sched_keyboard_interrupt();
                    }
                } else {
                    // mask interrupt & let FIFO handle it
                    QUART_IER(channel) &= ~IER_RXRDY;
                }
            }
#endif
        }
    }
}
