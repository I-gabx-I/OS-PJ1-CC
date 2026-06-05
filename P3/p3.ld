ENTRY(main)

SECTIONS
{
    /* El Proceso 3 arranca en su propia dirección base */
    . = 0x82300000;

    .text : {
        *(.text.boot)
        *(.text)
    }

    .rodata : { *(.rodata*) }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }
}
