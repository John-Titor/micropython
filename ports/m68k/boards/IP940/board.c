/*
 * Board-level support for IP940.
 *
 * References:
 *  https://github.com/John-Titor/IP940
 *
 * TODO:
 *  - UART init
 *  - UART library
 *  - interrupt-driven UART console
 *  - board library
 *    - direct register / memory access?
 *  - CF driver
 *  - filesystem config
 */

#include <py/mpconfig.h>
#include <py/runtime.h>
#include <mphalport.h>

#define IP940_TIMER_STOP    0x0210003b  // any access stops CPLD timers
#define IP940_TIMER_START   0x0210003f  // any access starts CPLD timers

extern void ox16c954_init(void);
extern void ox16c954_handler(void);

/*
 * Replicate the IP940 startup code.
 *
 * Note:
 *  - 040 UM insists the MMU does not need to be turned on for the TTRs to work.
 */
void m68k_ip940_early_init(void);
__asm__ (
    "   .section .text.startup                  \n"
    "   .align 2                                \n"
    "   .type m68k_ip940_early_init @function   \n"
    "   .global m68k_ip940_early_init           \n"
    "m68k_ip940_early_init:                     \n"
    "   cinva   %bc                             \n"
    "   moveq   #0,%d0                          \n" // ensure MMU is off
    "   movec   %d0,%tc                         \n"
    "   pflusha                                 \n"
    "   movec   %d0,%srp                        \n"
    "   movec   %d0,%urp                        \n"
    "   move.l  #0x0001c020,%d0                 \n" // data: ROM/RAM cacheable, writeback
    "   movec   %d0,%dtt0                       \n"
    "   move.l  #0x00ffc060,%d0                 \n" // data: default uncacheable, serialized
    "   movec   %d0,%dtt1                       \n"
    "   move.l  #0x00ffc000,%d0                 \n" // code: default cacheable, writethrough
    "   movec   %d0,%itt0                       \n"
    "   moveq   #0,%d0                          \n"
    "   movec   %d0,%itt1                       \n"
    "   move.l  #0x0000c000,%d0                 \n" // enable MMU
    "   movec   %d0,%tc                         \n"
    "   move.l  #0x80008000,%d0                 \n" // enable caches
    "   movec   %d0,%cacr                       \n"
    "   bra     m68k_init                       \n" // chain to next stage
    );

void
m68k_board_init(void) {
    ox16c954_init();                                // init the QUART
    *(volatile uint8_t *)IP940_TIMER_START = 0;     // start the 50/200Hz timers
}

M68K_INTERRUPT_HANDLER(27, ip940_quart) {
    ox16c954_handler();
}

M68K_INTERRUPT_HANDLER(28, ip940_50hz) {
    // do nothing here - interrupt auto-clears
}

M68K_INTERRUPT_HANDLER(30, ip940_200hz) {
    m68k_timer_tick();
}

