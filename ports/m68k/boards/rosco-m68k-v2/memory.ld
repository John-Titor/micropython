/*
 * Rosco-m68k v2 memory layout.
 */

MEMORY
{
    /* low 0x2000 reserved for ROM use */
    RAM (xrw)       : ORIGIN = 8K, LENGTH = 1M - 8K 
}

/*
 * Everything goes into RAM for now.
 */
REGION_ALIAS("REGION_TEXT", RAM)
REGION_ALIAS("REGION_DATA", RAM)
