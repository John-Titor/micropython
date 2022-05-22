#include <stdio.h>
#include <py/mpconfig.h>

#include "mphalport.h"

#ifndef M68K_TICK_MS
#define M68K_TICK_MS 1
#endif

/* timer tick count */
static volatile mp_uint_t m68k_timer_ticks;
static volatile mp_int_t m68k_delay_ticks;

/* millisecond timer tick */
void m68k_timer_tick(void) {
    m68k_timer_ticks += M68K_TICK_MS;

    // make a local copy of m68k_delay_ticks to avoid
    // reading it twice due to volatile rules
    mp_int_t dt = m68k_delay_ticks;
    if (dt > 0) {
        m68k_delay_ticks = dt - M68K_TICK_MS;
    }
}

// milliseconds since boot
mp_uint_t mp_hal_ticks_ms(void) {
    return m68k_timer_ticks;
}

// delay at least some number of milliseconds
void mp_hal_delay_ms(mp_uint_t ms) {
    m68k_delay_ticks = ms + M68K_TICK_MS - 1;
    while (m68k_delay_ticks >= 0) {
    }
}
