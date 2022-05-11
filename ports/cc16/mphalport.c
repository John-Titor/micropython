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

#include "cc16.h"

// Set up the S32K144 MCU
void cc16_init(void) {

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

    s32k_can_early_init();
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
