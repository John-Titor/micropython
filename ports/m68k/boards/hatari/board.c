/*
 * Board-level support
 *
 * References:
 *  https://temlib.org/AtariForumWiki/index.php/Atari_ST/STe/MSTe/TT/F030_Hardware_Register_Listing
 */

#include <py/mpconfig.h>
#include <devices/atarist.h>
#include <mphalport.h>

void
m68k_board_init(void) {
    m68k_atarist_init();
}
