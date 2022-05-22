/*
 * board-specific defines
 */

#define MICROPY_HW_BOARD_NAME               "tiny68k"
#define MICROPY_HW_MCU_NAME                 "mc68000"

// MC68681 config options
#define DUART_BASE                          0xfff001UL
#define DUART_MAP_WORD                      1
#define DUART_VECTOR                        64
