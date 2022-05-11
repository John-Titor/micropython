#ifndef MICROPY_INCLUDED_CC16_MPHALPORT_H
#define MICROPY_INCLUDED_CC16_MPHALPORT_H

#include "py/mpconfig.h"

extern volatile uint32_t systick_ms;

extern void mp_hal_set_interrupt_char(int c);	// XXX can we use mp_interrupt_char instead?

static inline mp_uint_t mp_hal_ticks_ms(void) {
    return systick_ms;
}
static inline mp_uint_t mp_hal_ticks_us(void) {
    return systick_ms * 1000;
}
static inline mp_uint_t mp_hal_ticks_cpu(void) {
    return 0;
}


#endif // MICROPY_INCLUDED_CC16_MPHALPORT_H