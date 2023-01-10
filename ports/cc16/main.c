
#include "py/builtin.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/stackctrl.h"
#include "shared/runtime/gchelper.h"
#include "shared/runtime/pyexec.h"

#include "cc16.h"

extern uint32_t _heap_start;
extern uint32_t _heap_end;

// Main entry point: initialise the runtime and execute demo strings.
void cc16_main(void) {

    // ensure ABI-compatible stack alignment
    S32_SCB->CCR |= S32_SCB_CCR_STKALIGN;

    // basic core init
    mp_stack_ctrl_init();

    // restart loop
    for (;;) {
        gc_init(&_heap_start, &_heap_end);
        mp_init();
        // mp_stack_set_limit(8192);

        // hardware init
        cc16_can_configure();
        cc16_pin_init();
        cc16_input_configure();
        cc16_output_configure();
        cc16_vref_configure();
        cc16_adc_configure();
        cc16_ftm_configure();

        // Execute _boot.py to set up the filesystem.
        // pyexec_frozen_module("_boot.py");

        // Execute user scripts.
        // pyexec_file_if_exists("boot.py");
        // pyexec_file_if_exists("main.py");

        for (;;) {
            if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
                if (pyexec_raw_repl() != 0) {
                    break;
                }
            } else {
                if (pyexec_friendly_repl() != 0) {
                    break;
                }
            }
        }
        mp_printf(MP_PYTHON_PRINTER, "MPY: soft reboot\n");
        // XXX hardware de-init
        gc_sweep_all();
        mp_deinit();
    }
}

// Handle uncaught exceptions (should never happen...)
void nlr_jump_fail(void *val) {
    mp_hal_stdout_tx_str("FATAL\r\n");
    for (;;) {
    }
}

// There is no filesystem so stat'ing returns nothing.
mp_import_stat_t mp_import_stat(const char *path) {
   return MP_IMPORT_STAT_NO_EXIST;
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

// Do a garbage collection cycle
void gc_collect(void) {
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
}
