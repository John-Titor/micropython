/*
 * qemu-virt memory layout for VM with 16M of RAM
 */

MEMORY
{
    /* 0x400 at 0 reserved for copy of vector table */
    RAM (xrw)       : ORIGIN = 0x00000400, LENGTH = 16M - 0x400
}

REGION_ALIAS("REGION_TEXT", RAM)
REGION_ALIAS("REGION_DATA", RAM)
