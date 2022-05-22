/*
 * Atari 1040ST memory layout w/256K ROM.
 */

MEMORY
{
    /* 0x400 at 0 reserved for copy of vector table */
    RAM (xrw)       : ORIGIN = 0x00000400, LENGTH = 1M - 0x400
    ROM (xr)        : ORIGIN = 0x00e00000, LENGTH = 256K
}

REGION_ALIAS("REGION_TEXT", ROM)
REGION_ALIAS("REGION_DATA", RAM)
