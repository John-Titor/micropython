/*
 * board-specific defines
 */

#define MICROPY_HW_BOARD_NAME               "Rosco-m68k-v2"
#define MICROPY_HW_MCU_NAME                 "mc68000"

// MC68681 config options
#define DUART_BASE                          0xf00001UL
#define DUART_MAP_WORD                      1
#define DUART_VECTOR                        0x50
