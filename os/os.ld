ENTRY(start) /* Asumiendo que tu punto de entrada en root.s se llama start */

SECTIONS
{
    /* El OS arranca en esta dirección base */
    . = 0x82000000; 

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    /* Definimos los límites del BSS para limpiarlo en root.s */
    . = ALIGN(4);
    bss_start = .;
    .bss : {
        *(.bss)
        *(COMMON)
    }
    . = ALIGN(4);
    bss_end = .;

    /* El stack del OS empieza en esta dirección (crece hacia abajo) */
    _os_stack_top = 0x82010000 + 0x2000; /* 0x2000 son los 8KB */
}