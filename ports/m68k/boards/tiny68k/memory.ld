/*
 * Tiny68k memory layout.
 */

MEMORY
{
    RAM (xrw)       : ORIGIN = 0, LENGTH = 14M 
}

/*
 * Everything goes into RAM
 */
REGION_ALIAS("REGION_TEXT", RAM)
REGION_ALIAS("REGION_DATA", RAM)
