/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2021 Damien P. George
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

#include "s32k144.h"

#define MICROPY_CONFIG_ROM_LEVEL            MICROPY_CONFIG_ROM_LEVEL_BASIC_FEATURES

// Memory allocation policies
#define MICROPY_GC_STACK_ENTRY_TYPE         uint16_t
#define MICROPY_GC_ALLOC_THRESHOLD          (0)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT      (32)
#define MICROPY_ALLOC_PATH_MAX              (256)
#define MICROPY_QSTR_BYTES_IN_HASH          (1)

// MicroPython emitters
#define MICROPY_EMIT_THUMB                  (0)
#define MICROPY_EMIT_INLINE_THUMB           (0)
#define MICROPY_MODULE_BUILTIN_INIT         (1)

// Python internal features
#define MICROPY_ENABLE_GC                   (1)
#define MICROPY_ENABLE_FINALIZER            (1)
#define MICROPY_KBD_EXCEPTION               (1)
#define MICROPY_HELPER_REPL                 (1)
#define MICROPY_REPL_AUTO_INDENT            (1)
#define MICROPY_LONGINT_IMPL                (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_ENABLE_SOURCE_LINE          (1)
#define MICROPY_ERROR_REPORTING             (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_FLOAT_IMPL                  (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_USE_INTERNAL_ERRNO          (1)
#define MICROPY_USE_INTERNAL_PRINTF         (1)
//#define MICROPY_ENABLE_SCHEDULER            (1)
//#define MICROPY_SCHEDULER_STATIC_NODES      (1)
#define MICROPY_MODULE_WEAK_LINKS           (1)

// Fine control over Python builtins, classes, modules, etc.
#define MICROPY_PY_FSTRINGS                 (1)
#define MICROPY_PY_BUILTINS_BYTES_HEX       (1)
#define MICROPY_PY_BUILTINS_COMPLEX         (0)
#define MICROPY_PY_BUILTINS_MEMORYVIEW      (1)
#define MICROPY_PY_BUILTINS_INPUT           (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO     (1)
#define MICROPY_PY_ARRAY_SLICE_ASSIGN       (1)
#define MICROPY_PY_MATH                     (1)

#define MICROPY_PY_BUILTINS_HELP            (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT       cc16_help_text
#define MICROPY_PY_BUILTINS_HELP_MODULES    (1)

// Extended modules
#define MICROPY_PY_UTIME_MP_HAL             (1)
#define MICROPY_PY_MACHINE                  (1)

// Hardware definitions
#define MICROPY_HW_BOARD_NAME               "MRS CC16"
#define MICROPY_HW_MCU_NAME                 "S32K144"

#define MP_STATE_PORT                       MP_STATE_VM

// Miscellaneous settings

__attribute__((always_inline)) static inline void enable_irq(uint32_t state) {
    __set_PRIMASK(state);
}

__attribute__((always_inline)) static inline uint32_t disable_irq(void) {
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

#define MICROPY_BEGIN_ATOMIC_SECTION()     disable_irq()
#define MICROPY_END_ATOMIC_SECTION(state)  enable_irq(state)

#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool); \
        mp_handle_pending(true); \
        __WFI(); \
    } while (0);

#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void *)((mp_uint_t)(p) | 1))

#define MP_SSIZE_MAX (0x7fffffff)
typedef intptr_t mp_int_t; // must be pointer size
typedef uintptr_t mp_uint_t; // must be pointer size
typedef long mp_off_t;

// Need to provide a declaration/definition of alloca()
#include <alloca.h>

//#define MICROPY_PORT_ROOT_POINTERS              const char *readline_hist[8];
