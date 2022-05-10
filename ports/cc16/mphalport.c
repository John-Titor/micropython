/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "py/mpconfig.h"
#include "py/mphal.h"

#include "s32k144.h"

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;

void Reset_Handler(void) __attribute__((naked));
void main(void);

static void cc16_init(void);

// Very simple ARM vector table.
const uint32_t isr_vector[] __attribute__((section(".isr_vector"))) = {
    (uint32_t)&_estack,
    (uint32_t)&Reset_Handler,
};

// The CPU runs this function after a reset.
void Reset_Handler(void) {
    // Set stack pointer.
    __asm volatile ("ldr sp, =_estack");

    // Copy .data section from flash to RAM.
    memcpy(&_sdata, &_sidata, (char *)&_edata - (char *)&_sdata);

    // Zero out .bss section.
    memset(&_sbss, 0, (char *)&_ebss - (char *)&_sbss);

    // SCB->CCR: enable 8-byte stack alignment for IRQ handlers, in accord with EABI.
    *((volatile uint32_t *)0xe000ed14) |= 1 << 9;

    // Initialise the cpu and peripherals.
    cc16_init();

    // Now that there is a basic system up and running, call the main application code.
    main();

    // This function must not return.
    for (;;) {
    }
}

// Set up the S32K144 MCU
static void cc16_init(void) {

    // clocking is handled by the ROM, nothing to do 

    // turn off the watchdog
    WDOG->CNT = 0xD928C520;         // unlock
    (void)WDOG->CNT;                // force ordering
    WDOG->CS &= ~WDOG_CS_EN;        // disable


    // configure OUT1 on PortD:14
    PCC->PCC_PORTD |= PCC_PCC_PORTD_CGC;        // clock on
    PORTD->PCR14 = PORT_PCR14_MUX(1);           // GPIO
    PTD->PDDR = (1U << 14);                     // direction = out
    PTD->PSOR = (1U << 14);                     // set

    // configure CAN for 500kBps
    PCC->PCC_FlexCAN0 |= PCC_PCC_FlexCAN0_CGC;  // clock on
    CAN0->CTRL1 |= CAN_CTRL1_CLKSRC;
    CAN0->MCR |= CAN_MCR_FRZ;
    CAN0->MCR &= ~CAN_MCR_MDIS;
    while(CAN0->MCR & CAN_MCR_LPMACK);
    CAN0->MCR ^= CAN_MCR_SOFTRST;
    while(CAN0->MCR & CAN_MCR_SOFTRST);
    while(!(CAN0->MCR & CAN_MCR_FRZACK));
    CAN0->MCR |= CAN_MCR_IRMQ   |
                 CAN_MCR_SRXDIS |
                 CAN_MCR_RFEN   |
                 0;
    CAN0->CBT = CAN_CBT_BTF             | 
                CAN_CBT_ERJW(1)         |
                CAN_CBT_EPRESDIV(0x09)  |
                CAN_CBT_EPROPSEG(0x07)  |
                CAN_CBT_EPSEG1(0x04)    |
                CAN_CBT_EPSEG2(0x01);
    CAN0->MCR &= ~CAN_MCR_HALT;
    while (CAN0->MCR & CAN_MCR_NOTRDY);
    CAN0->RAMn32 = 0x08000000;
}

static void can_write_chars(const char *str, size_t len)
{
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
    CAN0->RAMn32 = (0xc << 24) | (1U << 21) | (len << 16);
}

static int can_read_char()
{
    // buffer up to 7 extra bytes from a received message
    static char stash[7];
    static uint8_t stashlen;

    // return a buffered byte
    if (stashlen) {
        return stash[--stashlen];
    }

    for (;;) {
        // wait for a message
        while (!(CAN0->IFLAG1 & CAN_IFLAG1_BUF5I));

        // fetch the interesting parts of the message
        uint32_t dlc = (CAN0->RAMn0 >> 16) & 0xf;
        uint32_t id = CAN0->RAMn1;
        uint32_t dat0 = CAN0->RAMn2;
        uint32_t dat1 = CAN0->RAMn3;

        // and pop it from the FIFO
        CAN0->IFLAG1 = CAN_IFLAG1_BUF5I;

        // is this a message of interest?
        if ((id & 0x1fffffff) == 0x1ffffffd) {
            // save excess bytes
            switch (dlc) {
            case 8:
                stash[6] = (dat1 >>  0) & 0xff;
            case 7:
                stash[5] = (dat1 >>  8) & 0xff;
            case 6:
                stash[4] = (dat1 >> 16) & 0xff;
            case 5:
                stash[3] = (dat1 >> 24) & 0xff;
            case 4:
                stash[2] = (dat0 >>  0) & 0xff;
            case 3:
                stash[1] = (dat0 >>  8) & 0xff;
            case 2:
                stash[0] = (dat0 >> 16) & 0xff;
            case 1:
                // record how many excess bytes
                stashlen = dlc - 1;

                // and return the first from the message
                return (dat0 >> 24) & 0xff;
            }
        }
    }
}

// Get a single character, blocking until one is available
int mp_hal_stdin_rx_chr(void) {
    return can_read_char();
}

// Send string of given length to stdout.
void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    while (len) {
        size_t group_size = (len >= 8) ? 8 : len;
        can_write_chars(str, group_size);
        len -= group_size;
        str += group_size;
    }
}

static const struct
{
    uint32_t    header_key;            
    uint32_t    header_crc;            
    uint32_t    app_header_version;    
    uint32_t    application_crc;       
    uint32_t    application_length;    
    uint8_t     sw_version[32];         
} flash_header __attribute__((used, section(".application_header"))) = { 
    .header_key = 0x12345678,
    .app_header_version = 1,
};
