ENTRY(start)

SECTIONS
{
    . = 0x10000; 

    .text : { 
        *(.text.boot)   
        *(.text) 
    }
    .data : { *(.data) }
    
    . = ALIGN(4);
    bss_start = .;
    .bss : { *(.bss) *(COMMON) }
    . = ALIGN(4);
    bss_end = .;

    /* Alineacion de 8 bytes OBLIGATORIA para la funcion PRINT */
    . = ALIGN(8);
    . = . + 0x1000; /* 4KB de Stack para Interrupciones (IRQ) */
    _irq_stack_top = .;
    . = . + 0x2000; /* 8KB de Stack para el OS (SVC) */
    _os_stack_top = .;
}