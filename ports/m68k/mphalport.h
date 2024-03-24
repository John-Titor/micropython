#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <shared/runtime/interrupt_char.h>

extern void m68k_board_early_init(void);
extern void m68k_board_init(void);
extern void m68k_init(void);
extern void m68k_main(void);
extern void m68k_timer_tick(void);

// may already be defined by a standard library header
#ifndef __CONCAT
#    define __CONCAT(_x, _y)  _x##_y
#    define __CONCAT2(_x, _y) __CONCAT(_x, _y)
#endif
#ifndef __STR
#    define __STR(_x) #_x
#endif
#ifndef __XSTR
#    define __XSTR(_x) __STR(_x)
#endif

// define an interrupt handler
#define M68K_INTERRUPT_HANDLER(_vector, _label)                                                                        \
    static void                            _label(void);                                                               \
    void                                   m68k_interrupt_handler_##_vector(void) __attribute__((alias(#_label)));     \
    __attribute__((interrupt,used)) static void _label(void)

// read SR
static inline uint16_t
m68k_get_sr()
{
    uint16_t result;
    __asm__ volatile("move.w %%sr, %0" : "=d"(result) : : "memory");
    return result;
}

// write SR
static inline void
m68k_set_sr(uint16_t value)
{
    __asm__ volatile("move.w %0, %%sr" : : "d"(value) : "memory");
}

extern mp_uint_t mp_hal_ticks_ms(void);
extern void      mp_hal_delay_ms(mp_uint_t ms);
static inline mp_uint_t
mp_hal_ticks_us(void)
{
    return mp_hal_ticks_ms() * 1000;
}
static inline mp_uint_t
mp_hal_ticks_cpu(void)
{
    return mp_hal_ticks_ms();
}
static inline void
mp_hal_delay_us(mp_uint_t us)
{
    return mp_hal_delay_ms((us + 999) / 1000);
}

#define mp_hal_stdio_poll(poll_flags) (0) // not implemented
