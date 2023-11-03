/*
 * M68K vanilla segment layout.
 *
 * Board must lay out memory, with REGION_TEXT suitable for immutable code and data,
 * and REGION_DATA suitable for writable data.
 *
 * It's expected that vectors will either be at zero, or redirected from zero in
 * standard m68k fashion. The vector table is not expected to be writable.
 */

/* destination for 68000 vector copy, because the compiler has feelings about NULL */
m68k_live_vectors = 0x00000000;

ENTRY(m68k_start)

/* define output sections */
SECTIONS
{
    .text :
    {
        KEEP(*(.header))    /* optional ROM image header */
        KEEP(*(.vectors))   /* vector table */
        *(.text.startup)    /* startup code to allow use of BRA between stages */
        *(.text*)           /* .text* sections (code) */
        *(.rodata*)         /* .rodata* sections (constants, strings, etc.) */
        /* XXX C++ exception tables? */
        _etext = .;
    } > REGION_TEXT

    /* This is the initialized data segment */
    .data : AT (ADDR(.text) + SIZEOF(.text))
    {
        _sdata = .;
        *(.data*)           /* .data* sections */
        _edata = .;
    } > REGION_DATA

    /* Uninitialized data segment */
    .bss (NOLOAD) :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > REGION_DATA

    .stack (NOLOAD) : ALIGN(4)
    {
        _sstack = .;
        . = 0x10000;        /* 64K of stack */
        _estack = .;
    } > REGION_DATA

    .heap (NOLOAD) : ALIGN(4)
    {
        _sheap = .;
        /* get smart(er) later and make the heap grow to consume all memory */
        . = (512 * 1024);
        _eheap = .;
        __end = .;
    } > REGION_DATA

    /DISCARD/ : {
        *(.comment)
        *(.debug*)
    }
}
