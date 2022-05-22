/*
 * board-specific defines
 */

#define MICROPY_HW_BOARD_NAME       "hatari"

// m68k config options
#define M68K_EARLY_STARTUP_HOOK     m68k_atarist_early_init
#define M68K_WITH_NATIVE_FEATURES   (1)
#define M68K_WITH_NATFEAT_STDERR    (0)
#define M68K_MIN_IPL                5   // mask all interrupts other than MFP (not TT VME friendly)
#define M68K_TICK_MS                (5) // 200Hz trades less overhead for reduced time precision

// atari ST config options
#define ATARIST_SERIAL_CONSOLE      (0)
