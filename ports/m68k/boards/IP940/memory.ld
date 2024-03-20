/*
 * IP940 memory layout for 8/12M boards.
 */

MEMORY
{
	RAM (xrw)	: ORIGIN = 0x01000000, LENGTH = 8M
	ROM (xr)	: ORIGIN = 0x00004000, LENGTH = 2M - 0x4000
}


REGION_ALIAS("REGION_TEXT", RAM)
/*REGION_ALIAS("REGION_TEXT", ROM) XXX enable when ready to try flashing */
REGION_ALIAS("REGION_DATA", RAM)
