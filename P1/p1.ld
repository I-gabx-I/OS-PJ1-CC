ENTRY(main)

SECTIONS
{
    /* El Proceso 1 arranca en su propia dirección base */
    . = 0x82100000; 

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }
}