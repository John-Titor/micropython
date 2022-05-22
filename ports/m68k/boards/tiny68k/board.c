/*
 * Board-level support
 */

#include <py/mpconfig.h>

#include <mc68681.h>

void
m68k_board_init(void) {
    mc68681_init();
}
