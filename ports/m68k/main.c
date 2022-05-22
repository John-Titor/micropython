//
// Top-level application logic.
//

#include <stdio.h>

#include <py/builtin.h>
#include <py/compile.h>
#include <py/gc.h>
#include <py/repl.h>
#include <py/mperrno.h>
#include <py/stackctrl.h>
#include <shared/readline/readline.h>
#include <shared/runtime/pyexec.h>
#include <shared/runtime/gchelper.h>

#include "mphalport.h"

extern char _sheap, _eheap, _sstack, _estack;

void m68k_main(void) {

    mp_stack_ctrl_init();
    mp_stack_set_limit(&_estack - &_sstack);

    gc_init(&_sheap, &_eheap);
    mp_init();
    readline_init0();
    pyexec_friendly_repl();
    mp_deinit();
}

void gc_collect(void) {
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    mp_raise_OSError(MP_EPERM);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

void nlr_jump_fail(void *val) {
    static bool died = false;
    if (!died) {
        died = true;
        printf("FATAL: uncaught exception %p\n", val);
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)val);
    } else {
        printf("FATAL: uncaught exception attempting to print prior uncaught exception.\n");
    }
    while (1) {
        ;
    }
}

void NORETURN __fatal_error(const char *msg) {
    while (1) {
        ;
    }
}

void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
