ENTRY(main)

SECTIONS
{
    /* El Proceso 2 arranca en su propia dirección base */
    . = 0x82200000; 

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
