/*
 * Driver for MC68681 DUART and derivatives + timer.
 */

#include <stdio.h>
#include <py/mpconfig.h>

#include "mphalport.h"

#ifndef DUART_VECTOR
#error DUART_VECTOR must be defined
#endif


#if defined(DUART_MAP_WORD) || defined(DUART_MAP_BYTE)
# if defined(DUART_MAP_WORD)
#  define _DUART_REG(_ofs) (*(volatile uint8_t *)(DUART_BASE + (_ofs << 1)))
# else
#  define _DUART_REG(_ofs) (*(volatile uint8_t *)(DUART_BASE + (_ofs)))
# endif
# define DUART_MRA               _DUART_REG(0x00)
# define DUART_SRA               _DUART_REG(0x01)
# define DUART_CSRA              _DUART_REG(0x02)
# define DUART_CRA               _DUART_REG(0x02)
# define DUART_RBA               _DUART_REG(0x03)
# define DUART_TBA               _DUART_REG(0x03)
# define DUART_IPCR              _DUART_REG(0x04)
# define DUART_ACR               _DUART_REG(0x04)
# define DUART_ISR               _DUART_REG(0x05)
# define DUART_IMR               _DUART_REG(0x05)
# define DUART_CUR               _DUART_REG(0x06)
# define DUART_CTUR              _DUART_REG(0x06)
# define DUART_CLR               _DUART_REG(0x07)
# define DUART_CTLR              _DUART_REG(0x07)
# define DUART_MRB               _DUART_REG(0x08)
# define DUART_SRB               _DUART_REG(0x09)
# define DUART_CSRB              _DUART_REG(0x0a)
# define DUART_CRB               _DUART_REG(0x0a)
# define DUART_RBB               _DUART_REG(0x0b)
# define DUART_TBB               _DUART_REG(0x0b)
# define DUART_IVR               _DUART_REG(0x0c)
# define DUART_IPR               _DUART_REG(0x0d)
# define DUART_OPCR              _DUART_REG(0x0d)
# define DUART_STARTCC           _DUART_REG(0x0e)
# define DUART_OPRSET            _DUART_REG(0x0e)
# define DUART_STOPCC            _DUART_REG(0x0f)
# define DUART_OPRCLR            _DUART_REG(0x0f)
#elif defined(DUART_MAP_ROSCO_M68K_V1)
# define _DUART_REG(_ofs) (*(volatile uint8_t *)(DUART_BASE + (_ofs)))
# define DUART_MRA               _DUART_REG(0x18)
# define DUART_SRA               _DUART_REG(0x1a)
# define DUART_CSRA              _DUART_REG(0x1a)
# define DUART_CRA               _DUART_REG(0x1c)
# define DUART_RBA               _DUART_REG(0x1e)
# define DUART_TBA               _DUART_REG(0x1e)
# define DUART_IPCR              _DUART_REG(0x00)
# define DUART_ACR               _DUART_REG(0x00)
# define DUART_ISR               _DUART_REG(0x02)
# define DUART_IMR               _DUART_REG(0x02)
# define DUART_CUR               _DUART_REG(0x04)
# define DUART_CTUR              _DUART_REG(0x04)
# define DUART_CLR               _DUART_REG(0x06)
# define DUART_CTLR              _DUART_REG(0x06)
# define DUART_MRB               _DUART_REG(0x08)
# define DUART_SRB               _DUART_REG(0x0a)
# define DUART_CSRB              _DUART_REG(0x0a)
# define DUART_CRB               _DUART_REG(0x0c)
# define DUART_RBB               _DUART_REG(0x0e)
# define DUART_TBB               _DUART_REG(0x0e)
# define DUART_IVR               _DUART_REG(0x10)
# define DUART_IPR               _DUART_REG(0x12)
# define DUART_OPCR              _DUART_REG(0x12)
# define DUART_STARTCC           _DUART_REG(0x14)
# define DUART_OPRSET            _DUART_REG(0x14)
# define DUART_STOPCC            _DUART_REG(0x16)
# define DUART_OPRCLR            _DUART_REG(0x16)
#else
# error Must select a DUART_MAP_* option
#endif

/* register bits */
#define DUART_MR1_8BIT              0x03
#define DUART_MR1_NO_PARITY         0x10
#define DUART_MR1_RTS               0x80
#define DUART_MR2_1STOP             0x07
#define DUART_MR2_CTS_ENABLE_TX     0x10
#define DUART_SR_RECEIVED_BREAK     0x80
#define DUART_SR_FRAMING_ERROR      0x40
#define DUART_SR_PARITY_ERROR       0x20
#define DUART_SR_OVERRUN_ERROR      0x10
#define DUART_SR_TRANSMITTER_EMPTY  0x08
#define DUART_SR_TRANSMITTER_READY  0x04
#define DUART_SR_FIFO_FULL          0x02
#define DUART_SR_RECEIVER_READY     0x01
#define DUART_CSR_38400B            0xcc
#define DUART_CR_BRKSTOP            0x70
#define DUART_CR_BRKSTART           0x60
#define DUART_CR_BRKRST             0x50
#define DUART_CR_ERRST              0x40
#define DUART_CR_TXRST              0x30
#define DUART_CR_RXRST              0x20
#define DUART_CR_MRRST              0x10
#define DUART_CR_TXDIS              0x08
#define DUART_CR_TXEN               0x04
#define DUART_CR_RXDIS              0x02
#define DUART_CR_RXEN               0x01
#define DUART_ACR_MODE_CTR_XTAL16   0x30
#define DUART_ACR_MODE_TMR_XTAL     0x60
#define DUART_ACR_MODE_TMR_XTAL16   0x70
#define DUART_INT_TXRDY_A           0x01
#define DUART_INT_RXRDY_A           0x02
#define DUART_INT_CTR               0x08
#define DUART_INT_TXRDY_B           0x10
#define DUART_INT_RXRDY_B           0x20

#define RX_BUF_SIZE     128
static char duart_rx_buf[RX_BUF_SIZE];
static volatile uint16_t duart_rx_head;
static volatile uint16_t duart_rx_tail;

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
    for (;;) {
        if (duart_rx_head != duart_rx_tail) {
            c = duart_rx_buf[duart_rx_tail++ % RX_BUF_SIZE];
            break;
        }
    }
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        for (;;) {
            if (DUART_SRA & DUART_SR_TRANSMITTER_READY) {
                DUART_TBA = *str++;
                break;
            }
        }
    }
}

void mc68681_init(void) {
    /* initialise console UART */
    DUART_CRA = DUART_CR_MRRST;
    DUART_MRA = DUART_MR1_8BIT | DUART_MR1_NO_PARITY | DUART_MR1_RTS;
    DUART_MRA = DUART_MR2_CTS_ENABLE_TX | DUART_MR2_1STOP;
    DUART_IVR = DUART_VECTOR;
    DUART_ACR = DUART_ACR_MODE_TMR_XTAL16;
    DUART_CTLR = 0x80; /* ~1kHz timer interrupt */
    DUART_CTUR = 0x4;
    DUART_CSRA = DUART_CSR_38400B;
    DUART_CRA = DUART_CR_TXEN | DUART_CR_RXEN;

    // clear any pending interrupt
    (void)DUART_STOPCC;

    // interrupts enabled
    DUART_IMR = DUART_INT_RXRDY_A | DUART_INT_CTR;
}

M68K_INTERRUPT_HANDLER(DUART_VECTOR, duart_handler)
{
    uint8_t stat = DUART_ISR;

    if (stat & DUART_INT_RXRDY_A) {
        char c = DUART_RBA;
        if ((duart_rx_head - duart_rx_tail) < RX_BUF_SIZE) {
            duart_rx_buf[duart_rx_head++ % RX_BUF_SIZE] = c;
        }
    }

    if (stat & DUART_INT_CTR) {
        (void)DUART_STOPCC;
        m68k_timer_tick();
    }
}
