#pragma once

//
// Default feature config - boards can override this
//
#define MICROPY_CONFIG_ROM_LEVEL            MICROPY_CONFIG_ROM_LEVEL_FULL_FEATURES
#define MICROPY_ENABLE_COMPILER             (1)
#define MICROPY_HELPER_REPL                 (1)
#define MICROPY_REPL_INFO                   (1)
#define MICROPY_ERROR_REPORTING             (MICROPY_ERROR_REPORTING_DETAILED)
// #define MICROPY_REPL_EVENT_DRIVEN          (1) // XXX investigate
#define MICROPY_EVENT_POLL_HOOK             do {} while (0);
#define MICROPY_READER_VFS                  (0)
#define MICROPY_VFS                         (0)
#define MICROPY_VFS_FAT                     (0)

//
// M68K options that can be set in <mpconfigboard.h>
//
// #define M68K_EARLY_STARTUP_HOOK			// label to branch to to perform early startup - must chain to m68k_init
// #define M68K_HAVE_FPU					// defined if FPU is used
// #define M68K_WITH_NATIVE_FEATURES		// enable NatFeat support for emulator targets
// #define M68K_WITH_NATFEAT_STDERR			// output over NatFeat stderr channel
// #define M68K_MIN_IPL						// minimum IPL to set when enabling interrupts, do not use parens in definition
// #define M68K_TICK_MS                     // milliseconds per call to m68k_timer_tick (defaults to 1)
// #define ATARIST_SERIAL_CONSOLE		    // Use the Atari ST MFP serial port for the console
// #define ATARIST_IKBD_CONSOLE			    // Use the Atari ST IKBD for console input

#include <mpconfigboard.h>

//
// M68k common config - shared by all boards, not overrideable
//
#define MICROPY_PY_SYS_PLATFORM             "m68k"
#define MICROPY_OPT_COMPUTED_GOTO           (1)
#define MICROPY_ENABLE_GC                   (1)
#define MICROPY_GCREGS_SETJMP               (1)
#define MICROPY_FLOAT_IMPL                  (MICROPY_FLOAT_IMPL_FLOAT)
#define MP_NEED_LOG2                                            (1)     // not supplied by the float library

// Default alignment is 2; could use -malign-int to get 4-alignment but this seems to
// trigger a crash in gcc 11.2.0 so align objects up instead.
#define MICROPY_OBJ_BASE_ALIGNMENT          __attribute__((aligned(4)))

//
// Includes for the below types, defines, inlines, etc.
//
#include <stdbool.h>
#include <stdint.h>

//
// MP-required types, magic numbers, etc.
//
typedef intptr_t mp_int_t;                  // must be pointer size
typedef uintptr_t mp_uint_t;                // must be pointer size
typedef long mp_off_t;
#define MP_SSIZE_MAX __LONG_MAX__
#define INT_FMT "%ld"
#define UINT_FMT "%lu"

//
// Interrupt / atomic sections
//
static inline mp_uint_t m68k_interrupt_disable() {
    uint16_t state;
    __asm__ volatile (
        "move.w %%sr, %0        \n"
        "move.w #0x2700, %%sr   \n"
        : "=d" (state) : : "memory");
    return state & 0x0700;
}

// restore interrupts to previous state
static inline void m68k_interrupt_enable(mp_uint_t state) {
    __asm__ volatile ("move.w %0, %%sr" : : "d" (state | 0x2000) : "memory");
}

#define MICROPY_BEGIN_ATOMIC_SECTION() m68k_interrupt_disable()
#define MICROPY_END_ATOMIC_SECTION(_s) m68k_interrupt_enable(_s)

// we need to provide a declaration/definition of alloca()
#include <alloca.h>

// MP internal magic
#define MP_STATE_PORT MP_STATE_VM
