/*
 * board-specific defines
 */

#define MICROPY_HW_BOARD_NAME       "IP940"

#define M68K_TICK_MS                (5)     // use the 200Hz timer
#define M68K_EARLY_STARTUP_HOOK     m68k_ip940_early_init

#define QUART_STRIDE                (4)
#define QUART_BASE                  (0x02110003)
#define QUART_CONSOLE_CHANNEL       (0)