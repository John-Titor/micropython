/*
 * Linker file for the S32K144 as used in the CC16.
 */

MEMORY
{
    /* Flash - immutable */
    bootrom              : ORIGIN = 0x00000000, LENGTH = 0x0000f000
    eeprom_readonly      : ORIGIN = 0x0000f000, LENGTH = 0x00000200
    flash_calibration    : ORIGIN = 0x0000f200, LENGTH = 0x00000e00

    /* Flash - writable */
    application_header   : ORIGIN = 0x00010000, LENGTH = 0x00001000
    vector_table         : ORIGIN = 0x00011000, LENGTH = 0x00000400
    application_flash    : ORIGIN = 0x00011410, LENGTH = 0x0006bbf0

    /* FlexRAM */
    flexram              : ORIGIN = 0x14000000, LENGTH = 0x00001000

    /* SRAM - combined low and high blocks = 60K */
    sram                 : ORIGIN = 0x1fff8000, LENGTH = 0x0000f000
}

_estack = ORIGIN(sram) + LENGTH(sram);

SECTIONS
{
    /*
     * -> flash metadata
     */
    . = ORIGIN(application_header);
    .application_header :
    {
        KEEP(*(.application_header))
        . = ORIGIN(application_header) + LENGTH(application_header);
    } > application_header

    . = ORIGIN(vector_table);
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } > vector_table
    __vector_table = ORIGIN(vector_table);

    /*
     * -> flash application
     */
    . = ORIGIN(application_flash);
    .text :
    {
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _sidata = .;
    } > application_flash

    /*
     * -> SRAM
     */
    .data : AT ( _sidata )
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > sram

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > sram

    .heap :
    {
        . = ALIGN(8);
        _heap_start = .;
    } > sram
    _heap_end = ORIGIN(sram) + LENGTH(sram) - 2k;
}
